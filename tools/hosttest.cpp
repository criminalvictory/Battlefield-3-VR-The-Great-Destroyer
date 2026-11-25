// Hosting a gun that lies (src/hostrules.h, src/gunhost.cpp), without the game.
//   h1-h13, the pure rules: a draw's bones on the gun's frame and placed again (moved whole with the frame, zeros kept, bit for bit with an exact
//   pose), the gun's frame in a draw's space with the origins' whole metres, what becomes of a lying gun at a change of gun (flow (b)'s keep, the
//   spent AT4's junk, as before otherwise), the constant buffers' rules, the relaxed depth test (and its reversed depth), a pass and once in it, the
//   hosted list against loose's own results (an origin's metre jump, gone, broken numbers), the rehost after a new life, loose's SetKeep (a gun
//   made junk goes after its 30 s once unseen, never in view; a magazine kept lies on).
//   w1-w14, the graphics part on a software graphics card (WARP), the game's way of drawing a first-person gun mimicked (a skinned mesh, its bones
//   in a 4096-byte per-draw block, its 352-byte view constants, its material's constants, a texture, a depth pass then a colour pass into three
//   targets testing its depth EQUAL, stencil 32): a lying gun's draws kept as the mod draws them (gunhost::Begin, Note, End), then drawn by Host
//   right after another gun's draws in each pass: the same pixels as the game's own draws of it (an exact pose: bit for bit), moved with its body,
//   drawn with the host pass's own view, its material's constants and mesh as kept though the game's changed, on a deferred context too; every
//   state Host touches put back; nothing drawn into another kind of pass, nor without view constants of their size; its colour without its depth
//   pass hosted (relaxed: drawn; exact: not); the notes' overflow and cancel; Sweep and Clear.
// Prints PASS or FAIL per check; the exit code is the failures. WARP not there: the w checks are skipped (said).

#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../src/gunbody.h"
#include "../src/gunhost.h"
#include "../src/hostrules.h"
#include "../src/loose.h"

namespace {

int g_failures = 0, g_checks = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        ++g_failures;
}
template <typename... Args>
void CheckF(bool ok, const char* format, Args... args) {
    char what[600];
    std::snprintf(what, sizeof(what), format, args...);
    Check(ok, what);
}

constexpr float kPi = 3.14159265f;

void Identity(float* m) {
    const float id[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, id, sizeof(id));
}
// A turn of angle about the unit axis a, then a move (row-major 3x4).
void Pose(float* m, const float* a, float angle, float x, float y, float z) {
    const float c = std::cos(angle), s = std::sin(angle), u = 1.0f - c;
    const float r[12] = {u * a[0] * a[0] + c,        u * a[0] * a[1] - s * a[2], u * a[0] * a[2] + s * a[1], x,
                         u * a[0] * a[1] + s * a[2], u * a[1] * a[1] + c,        u * a[1] * a[2] - s * a[0], y,
                         u * a[0] * a[2] - s * a[1], u * a[1] * a[2] + s * a[0], u * a[2] * a[2] + c,        z};
    std::memcpy(m, r, sizeof(r));
}
float MaxDiff(const float* a, const float* b, int n) {
    float d = 0.0f;
    for (int i = 0; i < n; ++i)
        d = std::max(d, std::fabs(a[i] - b[i]));
    return d;
}
void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

// ---------------------------------------------------------------- loose's ground for h12, h13
int Flat(void*, const float*, loose::Plane* planes, int most) {
    if (most < 1)
        return 0;
    planes[0] = loose::Plane{};
    planes[0].normal[1] = 1.0f;
    planes[0].firm = true;
    return 1;
}
loose::Frame LooseFrame(const float* look) {
    loose::Frame f;
    f.dt = 1.0f / 72.0f;
    Identity(f.roomToArms);
    f.floor = &Flat;
    f.eye[0] = 0.0f, f.eye[1] = 1.6f, f.eye[2] = 0.0f;
    f.look[0] = look[0], f.look[1] = look[1], f.look[2] = look[2];
    return f;
}

void PureChecks() {
    const float axis[3] = {0.267261f, 0.534522f, 0.801784f};  // (1 2 3) made unit
    // ---- h1: bones on the gun's frame and placed again ----
    {
        float pose[12], bones[hostrules::kBones * 12], rel[hostrules::kBones * 12], back[hostrules::kBones * 12];
        Pose(pose, axis, 0.7f, -0.41f, 0.92f, 1.3f);
        for (int k = 0; k < hostrules::kBones; ++k) {
            float local[12];
            Pose(local, axis, 0.01f * static_cast<float>(k), 0.01f * k, -0.02f * k, 0.03f);
            hostrules::Compose(pose, local, bones + k * 12);
        }
        std::memset(bones + 7 * 12, 0, 48);  // a part folded onto one point
        float hidden[12] = {1e-4f, 0, 0, 0.2f, 0, 1e-4f, 0, 0.1f, 0, 0, 1e-4f, 0.3f};  // the game's own "hidden": a scale of 1e-4
        hostrules::Compose(pose, hidden, bones + 9 * 12);
        const bool ok = hostrules::Rel(pose, bones, hostrules::kBones, rel);
        hostrules::Place(pose, rel, hostrules::kBones, back);
        float worst = 0.0f;
        for (int k = 0; k < hostrules::kBones; ++k)
            if (k != 7)
                worst = std::max(worst, MaxDiff(bones + k * 12, back + k * 12, 12));
        CheckF(ok && worst < 2e-6f, "h1 a palette's bones on the gun's frame and placed again by the same pose: back within %.1e (2e-6)", worst);
        CheckF(hostrules::AllZero(rel + 7 * 12) && hostrules::AllZero(back + 7 * 12), "h1 ... a bone all zeros (a part folded onto one point) stays all zeros");
        CheckF(MaxDiff(rel + 9 * 12, hidden, 12) < 1e-6f, "h1 ... the game's hidden part (a scale of 1e-4) kept as it was on the frame (%.1e off)", MaxDiff(rel + 9 * 12, hidden, 12));
        float flat[12] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
        CheckF(!hostrules::Rel(flat, bones, 1, rel), "h1 ... a pose that can't be inverted (flat): nothing");
        float broken[12];
        std::memcpy(broken, pose, sizeof(broken));
        broken[3] = std::nanf("");
        CheckF(!hostrules::Rel(broken, bones, 1, rel), "h1 ... a pose with a number not finite: nothing");
        // An exact pose (a quarter turn, moves of whole halves and quarters): bit for bit.
        float exact[12] = {0, -1, 0, -0.25f, 1, 0, 0, 0.5f, 0, 0, 1, 0.125f}, ebones[24], erel[24], eback[24];
        float a[12] = {1, 0, 0, 0.5f, 0, 1, 0, 0.25f, 0, 0, 1, 0}, b[12] = {0, 0, 1, 0, 0, 1, 0, 0.125f, -1, 0, 0, 0.5f};
        hostrules::Compose(exact, a, ebones);
        hostrules::Compose(exact, b, ebones + 12);
        hostrules::Rel(exact, ebones, 2, erel);
        hostrules::Place(exact, erel, 2, eback);
        CheckF(std::memcmp(ebones, eback, sizeof(ebones)) == 0, "h1 ... with an exact pose (a quarter turn, moves of halves and quarters): bit for bit");
    }
    // ---- h2: moved whole with the frame ----
    {
        float p1[12], p2[12], bone[12], rel[12], moved[12], local[12];
        Pose(p1, axis, 0.3f, 0.1f, 0.2f, 0.3f);
        Pose(p2, axis, -1.1f, 2.5f, -0.4f, 7.0f);
        Pose(local, axis, 0.2f, 0.05f, 0.1f, 0.6f);
        hostrules::Compose(p1, local, bone);
        hostrules::Rel(p1, bone, 1, rel);
        hostrules::Place(p2, rel, 1, moved);
        float want[12];
        hostrules::Compose(p2, local, want);
        CheckF(MaxDiff(moved, want, 12) < 4e-6f, "h2 a bone kept at one pose and placed at another moves whole with the gun's frame (%.1e off)", MaxDiff(moved, want, 12));
        const float point[3] = {0.01f, 0.02f, 0.7f};
        float onFrame[3], world1[3], world2[3];
        Transform(local, point, onFrame);
        Transform(p2, onFrame, world1);
        Transform(moved, point, world2);
        CheckF(MaxDiff(world1, world2, 3) < 4e-6f, "h2 ... a vertex of it lands where the frame's move puts it (%.1e m)", MaxDiff(world1, world2, 3));
    }
    // ---- h3: the gun's frame in a draw's space, the origins' metres ----
    {
        float drawn[12], bone[12], out[12], want[12];
        Identity(drawn);
        Pose(bone, axis, 0.4f, 0.3f, -0.2f, 0.1f);
        const int origin[3] = {-1450, 94, -2014};
        const float seen[3] = {-1451.0f, 94.0f, -2013.0f};
        hostrules::FrameInDraw(drawn, bone, origin, seen, out);
        std::memcpy(want, bone, sizeof(want));
        want[3] += 1.0f, want[11] -= 1.0f;
        CheckF(MaxDiff(out, want, 12) < 1e-6f, "h3 loose's pose with its origin (-1450 94 -2014) in a draw whose origin is (-1451 94 -2013): moved by (1 0 -1) m");
        float turn[12];
        Pose(turn, axis, 1.2f, 0.5f, 0.6f, 0.7f);
        hostrules::FrameInDraw(turn, bone, origin, seen, out);
        hostrules::Compose(turn, want, want);
        CheckF(MaxDiff(out, want, 12) < 1e-6f, "h3 ... then through that draw's way from the world (skin.cpp ToArms's lying launcher's): %.1e off", MaxDiff(out, want, 12));
    }
    // ---- h4: the same inputs, the same bits (a depth draw's and a colour draw's bones of one bone, placed in one pass of one eye) ----
    {
        float pose[12], host[12], bone[12], rel[12], a[12], b[12];
        Pose(pose, axis, 0.9f, -0.31f, 0.27f, 0.6f);
        Pose(host, axis, -0.2f, 0.8f, 0.3f, -1.2f);
        Pose(bone, axis, 0.33f, 0.2f, 0.1f, 0.4f);
        hostrules::Compose(pose, bone, bone);
        hostrules::Rel(pose, bone, 1, rel);
        hostrules::Place(host, rel, 1, a);
        float rel2[12];
        hostrules::Rel(pose, bone, 1, rel2);
        hostrules::Place(host, rel2, 1, b);
        CheckF(std::memcmp(a, b, sizeof(a)) == 0, "h4 a bone kept from the same draw and placed by the same pose twice: bit for bit (a depth draw's and a colour draw's of one eye alike)");
    }
    // ---- h5: what becomes of a lying gun at a change of gun ----
    {
        using hostrules::Fate;
        hostrules::FateIn base;
        base.on = true, base.lies = true, base.ready = true, base.slotsRead = 4, base.inSlots = true;
        hostrules::FateIn in = base;
        in.taken = true;
        CheckF(hostrules::AtChange(in) == Fate::kKeep, "h5 flow (b): a scene's gift took its slot: kept lying, drawn by the mod till the game gives it back");
        in = base, in.inSlots = false;
        CheckF(hostrules::AtChange(in) == Fate::kJunk, "h5 the spent AT4 thrown, the kit's M4A1 back: its gun in none of the slots: junk (30 s, then gone unseen)");
        in = base;
        CheckF(hostrules::AtChange(in) == Fate::kAsBefore, "h5 a dropped gun of another slot (the Mk11 while the D-pad brings the M4A1): in its slot still: as before");
        in = base, in.inSlots = false, in.slotsRead = 0;
        CheckF(hostrules::AtChange(in) == Fate::kAsBefore, "h5 ... no slot read (no soldier, the read failing): never junk, as before");
        in = base, in.taken = true, in.on = false;
        CheckF(hostrules::AtChange(in) == Fate::kAsBefore, "h5 lyinghost 0: flow (b) as before (nothing drawn meanwhile)");
        in = base, in.taken = true, in.ready = false;
        CheckF(hostrules::AtChange(in) == Fate::kAsBefore, "h5 its draws not kept (none lying lately, not a held gun's): as before");
        in = base, in.taken = true, in.lies = false;
        CheckF(hostrules::AtChange(in) == Fate::kAsBefore, "h5 nothing lying: as before");
        in = base, in.inSlots = false, in.retired = true;
        CheckF(hostrules::AtChange(in) == Fate::kAsBefore, "h5 in a pickup the game swapped it into (the game draws it there): as before");
    }
    // ---- h6: the constant buffers' rules ----
    {
        using hostrules::Cb;
        CheckF(hostrules::RuleOf(true, 4096, false, true, false) == Cb::kPalette && hostrules::RuleOf(true, 4096, true, true, false) == Cb::kPalette,
               "h6 the game's per-draw block bound for the draw: the mod's bones (whatever else it looks like)");
        CheckF(hostrules::RuleOf(true, 352, false, false, false) == Cb::kView && hostrules::RuleOf(true, 80, true, false, false) == Cb::kView,
               "h6 the 352-byte view constants, or a buffer the first-person views went into: the pass's own");
        CheckF(hostrules::RuleOf(true, 64, false, false, true) == Cb::kOriginal, "h6 an immutable one: the game's own (it can't change)");
        CheckF(hostrules::RuleOf(true, 64, false, false, false) == Cb::kCopy && hostrules::RuleOf(true, 4096, false, false, false) == Cb::kCopy,
               "h6 any other (a material's, another 4096-byte block): copied on the card as it was for that draw");
        CheckF(hostrules::RuleOf(false, 352, true, true, true) == Cb::kNone, "h6 nothing bound: nothing");
    }
    // ---- h7: the relaxed depth test ----
    {
        hostrules::Depth equal;
        equal.enable = true, equal.writes = false, equal.func = hostrules::kEqual;
        const hostrules::Depth r = hostrules::Relaxed(equal, false, hostrules::kLess);
        CheckF(r.func == hostrules::kLessEqual && r.writes && r.enable, "h7 a colour draw testing EQUAL against its depth pass: LESS_EQUAL, its depth written");
        const hostrules::Depth rr = hostrules::Relaxed(equal, false, hostrules::kGreaterEqual);
        CheckF(rr.func == hostrules::kGreaterEqual && rr.writes, "h7 ... with a reversed depth (its depth draws test GREATER_EQUAL): GREATER_EQUAL");
        const hostrules::Depth u = hostrules::Relaxed(equal, false, 0);
        CheckF(u.func == hostrules::kLessEqual, "h7 ... the depth's way not seen yet: LESS_EQUAL");
        const hostrules::Depth e = hostrules::Relaxed(equal, true, hostrules::kLess);
        CheckF(e.func == hostrules::kEqual && !e.writes, "h7 lyinghostexact 1: EQUAL, no depth written, as the game draws it");
        hostrules::Depth less;
        less.func = hostrules::kLess, less.writes = true;
        const hostrules::Depth l = hostrules::Relaxed(less, false, hostrules::kLess);
        CheckF(l.func == hostrules::kLess && l.writes, "h7 any other test (LESS, writing): as it was");
        hostrules::Depth off;
        off.enable = false, off.func = hostrules::kEqual;
        CheckF(hostrules::Relaxed(off, false, hostrules::kLess).func == hostrules::kEqual, "h7 depth not tested at all: as it was");
    }
    // ---- h8: a pass ----
    {
        hostrules::Pass gb;
        gb.targets = 3, gb.format[0] = 28, gb.format[1] = 24, gb.format[2] = 10;
        gb.depth = true, gb.depthFormat = 45, gb.width = 4320, gb.height = 2160, gb.samples = 1, gb.colour = true;
        hostrules::Pass same = gb, depthPass = gb, other = gb, smallPass = gb;
        depthPass.colour = false;
        other.format[2] = 28;
        smallPass.width = 2160;
        CheckF(hostrules::SamePass(gb, same) && hostrules::PassHash(gb) == hostrules::PassHash(same), "h8 the same targets and depth: the same pass, the same hash");
        CheckF(!hostrules::SamePass(gb, depthPass) && hostrules::PassHash(gb) != hostrules::PassHash(depthPass),
               "h8 the same targets bound with no colour written (a depth pass's): another pass");
        CheckF(!hostrules::SamePass(gb, other) && !hostrules::SamePass(gb, smallPass), "h8 a target of another format, a depth of another size: another pass");
    }
    // ---- h9: once a pass ----
    {
        hostrules::Once once;
        hostrules::PassKey k{100, 0x1234, 77};
        const bool first = once.First(k), again = once.First(k);
        hostrules::PassKey eye = k, pass = k, frame = k;
        eye.view = 0x5678, pass.pass = 78, frame.frame = 101;
        CheckF(first && !again, "h9 the gun the game draws drawn twice in one pass of one view: hosted once");
        CheckF(once.First(eye) && once.First(pass) && once.First(frame), "h9 ... the other eye's view, another pass, the next frame: each its own");
        for (uint32_t f = 200; f < 220; ++f)
            once.First(hostrules::PassKey{f, 1, 1});
        CheckF(once.First(k), "h9 ... and a pass remembered no longer than 16 passes back (a frame's are a handful)");
    }
    // ---- h10, h11: the hosted list against loose's results ----
    {
        hostrules::List l;
        hostrules::Hosted a, b, z;
        a.looseId = 3, a.kind = 22194, a.keep = true, a.name = "M4A1";
        b.looseId = 5, b.kind = 15987, b.keep = false, b.name = "AT4";
        z.looseId = 0, z.kind = 1;
        CheckF(hostrules::Add(l, a) && hostrules::Add(l, b) && !hostrules::Add(l, a) && !hostrules::Add(l, z), "h10 two hosted; the same body twice, or none, refused");
        hostrules::Hosted more[3];
        for (int i = 0; i < 3; ++i)
            more[i].looseId = 10 + i, more[i].kind = 1;
        const bool third = hostrules::Add(l, more[0]), fourth = hostrules::Add(l, more[1]), fifth = hostrules::Add(l, more[2]);
        CheckF(third && fourth && !fifth && l.n == hostrules::kMostHosted, "h10 ... room for %d at once, no more", hostrules::kMostHosted);
        hostrules::Drop(l, 10), hostrules::Drop(l, 11);
        static loose::Result r;
        r = loose::Result{};
        r.poses = 2;
        r.pose[0].id = 3, Identity(r.pose[0].bone), r.pose[0].bone[3] = 0.25f;
        r.pose[1].id = 99;
        const int origin[3] = {7, 0, -3};
        int dropped[hostrules::kMostHosted] = {};
        int gone = hostrules::Refresh(l, r, origin, dropped);
        CheckF(gone == 1 && dropped[0] == 5 && l.n == 1 && l.h[0].looseId == 3 && l.h[0].have && l.h[0].bone[3] == 0.25f && l.h[0].origin[0] == 7 && l.h[0].origin[2] == -3,
               "h10 after loose's Step: the one among its poses takes its pose and that Step's origin; the one not among them (a new life's Clear) dropped, said");
        r.gone = 1, r.goneId[0] = 3;
        gone = hostrules::Refresh(l, r, origin, dropped);
        CheckF(gone == 1 && dropped[0] == 3 && l.n == 0, "h10 ... one told gone (no ground under it 5 s, a junk one's 30 s and out of view) dropped though posed");
        hostrules::Add(l, a);
        r = loose::Result{};
        r.poses = 1, r.pose[0].id = 3, Identity(r.pose[0].bone), r.pose[0].bone[7] = std::nanf("");
        gone = hostrules::Refresh(l, r, origin, dropped);
        CheckF(gone == 1 && l.n == 0, "h10 ... a pose with a number not finite: dropped, never drawn torn");
        hostrules::RehostIn in;
        in.on = in.record = in.ready = in.body = in.placed = true, in.kind = 22194, in.drawn = 15987;
        const bool yes = hostrules::Rehost(in);
        in.drawn = 22194;
        const bool drawn = hostrules::Rehost(in);
        in.drawn = 15987, in.hosted = true;
        const bool hosted = hostrules::Rehost(in);
        in.hosted = false, in.ready = false;
        const bool unready = hostrules::Rehost(in);
        CheckF(yes && !drawn && !hosted && !unready, "h11 after a new life on its level (the RPG checkpoint): put down again while the game draws the AT4; not when the game "
               "draws the M4A1 itself, nor with a body hosted already, nor with nothing kept to draw it with");
    }
    // ---- h12: loose's SetKeep (a gun made junk) ----
    {
        loose::Clear();
        loose::SetLie(30.0f);
        const float lookAway[3] = {0.0f, 0.0f, -1.0f}, lookAt[3] = {0.0f, -0.8f, 0.6f};
        loose::Frame f = LooseFrame(lookAway);
        float bone[12];
        Identity(bone);
        bone[3] = 0.0f, bone[7] = 0.3f, bone[11] = 1.0f;  // (dropped 30 cm up, 1 m off along +z: away from a look along -z)
        const int gun = loose::Place(gunbody::kM416, 5, bone, 30, f);
        const loose::Spec* mag = loose::SpecFor("m4a1");
        float magBone[12];
        Identity(magBone);
        magBone[3] = 0.4f, magBone[7] = 0.3f, magBone[11] = 1.0f;
        const int kept = mag ? loose::Place(*mag, 1, magBone, 0, f) : 0;
        CheckF(gun && kept && !loose::SetKeep(12345, false), "h12 a gun and a magazine on the floor; an unknown body's keep: no such thing");
        auto run = [&](float seconds, const float* look) {
            loose::Frame step = LooseFrame(look);
            for (int i = 0; i < static_cast<int>(seconds * 72.0f); ++i)
                loose::Step(step);
        };
        run(5.0f, lookAway);
        loose::SetKeep(gun, false);  // (the kit took its gun back: junk)
        loose::SetKeep(kept, true);  // (a magazine made one that lies till it's taken)
        run(20.0f, lookAway);
        const bool gunAt25 = loose::Has(gun);
        run(10.0f, lookAt);  // (35 s: its 30 s from its rest are past, but it's looked at)
        const bool gunSeen = loose::Has(gun);
        run(2.0f, lookAway);
        const bool gunGone = !loose::Has(gun);
        CheckF(gunAt25 && gunSeen && gunGone, "h12 a gun made junk lies its 30 s from its rest (there at 25 s), never goes in view (there at 35 s looked at), then goes unseen");
        run(5.0f, lookAway);
        CheckF(loose::Has(kept), "h12 ... a magazine made one that's kept lies on past its 30 s, unseen (42 s)");
        loose::Clear();
    }
    // ---- h13: a hosted gun stays put in the world as the arms' space's origin jumps a metre ----
    {
        loose::Clear();
        const float look[3] = {0.0f, 0.0f, -1.0f};
        loose::Frame f = LooseFrame(look);
        float bone[12];
        Identity(bone);
        bone[7] = 0.3f, bone[11] = 1.0f;
        const int id = loose::Place(gunbody::kM416, 5, bone, 30, f);
        hostrules::List l;
        hostrules::Hosted h;
        h.looseId = id, h.kind = 12768, h.keep = true;
        hostrules::Add(l, h);
        for (int i = 0; i < 144; ++i)
            hostrules::Refresh(l, loose::Step(f), f.origin, nullptr);
        float before[12], after[12];
        const float drawn[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        const float seen0[3] = {0.0f, 0.0f, 0.0f};
        hostrules::FrameInDraw(drawn, l.h[0].bone, l.h[0].origin, seen0, before);
        f.origin[0] = 1, f.origin[2] = -1;  // (the player walked across a metre line: the arms' space's origin jumps)
        hostrules::Refresh(l, loose::Step(f), f.origin, nullptr);
        const float seen1[3] = {1.0f, 0.0f, -1.0f};
        hostrules::FrameInDraw(drawn, l.h[0].bone, l.h[0].origin, seen1, after);
        float worldBefore[3] = {before[3], before[7], before[11]}, worldAfter[3] = {after[3] + 1.0f, after[7], after[11] - 1.0f};
        CheckF(l.n == 1 && MaxDiff(worldBefore, worldAfter, 3) < 1e-4f, "h13 the hosted gun where it lies in the world as the origin jumps a metre: %.2f mm apart",
               MaxDiff(worldBefore, worldAfter, 3) * 1000.0f);
        loose::Clear();
    }
}

// ---------------------------------------------------------------- the graphics part on WARP

template <typename T>
void SafeRelease(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

constexpr char kShader[] = R"(
cbuffer View : register(b0) { float4 viewPad[11]; row_major float4x4 vp; float4 viewPad2[7]; };
cbuffer Bones : register(b1) { float4 bones[256]; };
cbuffer Material : register(b2) { float4 tint; };
struct VsIn { float3 pos : POSITION; float2 uv : TEXCOORD0; uint bone : BLENDINDICES0; };
struct VsOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; float4 tint : COLOR0; };
VsOut Vs(VsIn i) {
    VsOut o;
    float4 p = float4(i.pos, 1);
    float3 w = float3(dot(bones[i.bone * 3 + 0], p), dot(bones[i.bone * 3 + 1], p), dot(bones[i.bone * 3 + 2], p));
    o.pos = mul(vp, float4(w, 1));
    o.uv = i.uv;
    o.tint = tint;
    return o;
}
Texture2D tex : register(t0);
SamplerState smp : register(s0);
struct PsOut { float4 a : SV_Target0; float4 b : SV_Target1; float4 c : SV_Target2; };
PsOut Ps(VsOut i) {
    PsOut o;
    o.a = i.tint;
    o.b = tex.Sample(smp, i.uv);
    o.c = float4(i.uv, 0.5, 1);
    return o;
}
)";

struct Vertex {
    float pos[3];
    float uv[2];
    uint32_t bone;
};
// A box (8 corners, 12 triangles) about c, half sizes h, on a bone; its indices from base.
void Box(std::vector<Vertex>& v, std::vector<uint16_t>& ix, const float* c, const float* h, uint32_t bone) {
    const uint16_t first = static_cast<uint16_t>(v.size());
    for (int k = 0; k < 8; ++k) {
        Vertex x{};
        x.pos[0] = c[0] + ((k & 1) ? h[0] : -h[0]);
        x.pos[1] = c[1] + ((k & 2) ? h[1] : -h[1]);
        x.pos[2] = c[2] + ((k & 4) ? h[2] : -h[2]);
        x.uv[0] = (k & 1) ? 1.0f : 0.0f;
        x.uv[1] = (k & 2) ? 1.0f : 0.0f;
        x.bone = bone;
        v.push_back(x);
    }
    const uint16_t faces[36] = {0, 1, 3, 0, 3, 2, 4, 6, 7, 4, 7, 5, 0, 4, 5, 0, 5, 1, 2, 3, 7, 2, 7, 6, 0, 2, 6, 0, 6, 4, 1, 5, 7, 1, 7, 3};
    for (uint16_t f : faces)
        ix.push_back(static_cast<uint16_t>(first + f));
}

void STDMETHODCALLTYPE TestDraw(ID3D11DeviceContext* c, UINT n, UINT s, INT b) {
    c->DrawIndexed(n, s, b);
}
HRESULT STDMETHODCALLTYPE TestMap(ID3D11DeviceContext* c, ID3D11Resource* r, UINT sub, D3D11_MAP t, UINT f, D3D11_MAPPED_SUBRESOURCE* m) {
    return c->Map(r, sub, t, f, m);
}
void STDMETHODCALLTYPE TestUnmap(ID3D11DeviceContext* c, ID3D11Resource* r, UINT sub) {
    c->Unmap(r, sub);
}

constexpr UINT kSize = 64;
constexpr uint32_t kGun = 22194;     // the lying gun's kind (as the M4A1's)
constexpr UINT kColourCount = 72, kColourStart = 0, kDepthCount = 72, kDepthStart = 72;  // its two draws, as the game's two passes'
constexpr UINT kHostCount = 36;      // the gun the game draws (as the AT4)

struct Gpu {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* imm = nullptr;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* layout = nullptr;
    ID3D11Buffer* view = nullptr;      // 352 bytes, dynamic: the per-view constants
    ID3D11Buffer* palette = nullptr;   // 4096 bytes, dynamic: the game's per-draw block
    ID3D11Buffer* mod = nullptr;       // 4096 bytes, dynamic: the mod's own (DrawWithPalette's)
    ID3D11Buffer* material = nullptr;  // 16 bytes, dynamic: the tint
    ID3D11Buffer* little = nullptr;     // 64 bytes: no view of the views' size
    ID3D11Buffer* gunVb = nullptr;
    ID3D11Buffer* gunIb = nullptr;
    ID3D11Buffer* hostVb = nullptr;
    ID3D11Buffer* hostIb = nullptr;
    ID3D11Texture2D* rt[3] = {};
    ID3D11RenderTargetView* rtv[3] = {};
    ID3D11Texture2D* depthTex = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ID3D11Texture2D* tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11DepthStencilState* depthWrite = nullptr;
    ID3D11DepthStencilState* depthEqual = nullptr;
    ID3D11RasterizerState* rs = nullptr;
    ID3D11Texture2D* staging = nullptr;
    std::vector<uint16_t> gunIndices;
    std::vector<Vertex> gunVertices;
    void Release() {
        SafeRelease(vs);
        SafeRelease(ps), SafeRelease(layout), SafeRelease(view), SafeRelease(palette), SafeRelease(mod), SafeRelease(material), SafeRelease(little);
        SafeRelease(gunVb), SafeRelease(gunIb), SafeRelease(hostVb), SafeRelease(hostIb);
        for (int i = 0; i < 3; ++i)
            SafeRelease(rtv[i]), SafeRelease(rt[i]);
        SafeRelease(dsv), SafeRelease(depthTex), SafeRelease(srv), SafeRelease(tex), SafeRelease(sampler);
        SafeRelease(depthWrite), SafeRelease(depthEqual), SafeRelease(rs), SafeRelease(staging);
        SafeRelease(imm), SafeRelease(device);
    }
};

ID3D11Buffer* MakeBuffer(ID3D11Device* d, UINT size, D3D11_USAGE usage, UINT bind, const void* data) {
    D3D11_BUFFER_DESC desc{size, usage, bind, usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0u, 0, 0};
    D3D11_SUBRESOURCE_DATA init{data, 0, 0};
    ID3D11Buffer* b = nullptr;
    if (FAILED(d->CreateBuffer(&desc, data ? &init : nullptr, &b)))
        return nullptr;
    return b;
}

void Write(ID3D11DeviceContext* c, ID3D11Buffer* b, const void* data, UINT size) {
    D3D11_MAPPED_SUBRESOURCE m{};
    if (SUCCEEDED(c->Map(b, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        std::memcpy(m.pData, data, size);
        c->Unmap(b, 0);
    }
}

// An orthographic view (x, y as they are; z from -5..5 into 0..1), moved by dx in the picture.
void ViewBytes(float dx, uint8_t* out) {
    std::memset(out, 0, 352);
    const float vp[16] = {1.0f, 0, 0, dx, 0, 1.0f, 0, 0, 0, 0, 0.1f, 0.5f, 0, 0, 0, 1};
    std::memcpy(out + 176, vp, sizeof(vp));
}

bool Setup(Gpu& g) {
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &g.device, nullptr, &g.imm)))
        return false;
    ID3DBlob* vsb = nullptr;
    ID3DBlob* psb = nullptr;
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(kShader, sizeof(kShader) - 1, "host", nullptr, nullptr, "Vs", "vs_4_0", 0, 0, &vsb, &err)) ||
        FAILED(D3DCompile(kShader, sizeof(kShader) - 1, "host", nullptr, nullptr, "Ps", "ps_4_0", 0, 0, &psb, &err))) {
        if (err)
            std::printf("shader: %s\n", static_cast<const char*>(err->GetBufferPointer())), err->Release();
        SafeRelease(vsb), SafeRelease(psb);
        return false;
    }
    g.device->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g.vs);
    g.device->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g.ps);
    const D3D11_INPUT_ELEMENT_DESC elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    g.device->CreateInputLayout(elements, 3, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g.layout);
    vsb->Release(), psb->Release();
    g.view = MakeBuffer(g.device, 352, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, nullptr);
    g.palette = MakeBuffer(g.device, 4096, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, nullptr);
    g.mod = MakeBuffer(g.device, 4096, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, nullptr);
    g.material = MakeBuffer(g.device, 16, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, nullptr);
    g.little = MakeBuffer(g.device, 64, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, nullptr);
    // The lying gun: a body on bone 0 and a sight on bone 1; its colour draw's indices, then its depth pass's (the same triangles, as the game's two passes).
    {
        const float c0[3] = {0, 0, 0}, h0[3] = {0.5f, 0.125f, 0.125f}, c1[3] = {0.125f, 0.25f, 0}, h1[3] = {0.0625f, 0.0625f, 0.0625f};
        Box(g.gunVertices, g.gunIndices, c0, h0, 0);
        Box(g.gunVertices, g.gunIndices, c1, h1, 1);
        const std::vector<uint16_t> colour = g.gunIndices;
        g.gunIndices.insert(g.gunIndices.end(), colour.begin(), colour.end());
        g.gunVb = MakeBuffer(g.device, static_cast<UINT>(g.gunVertices.size() * sizeof(Vertex)), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, g.gunVertices.data());
        g.gunIb = MakeBuffer(g.device, static_cast<UINT>(g.gunIndices.size() * 2), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, g.gunIndices.data());
    }
    {
        std::vector<Vertex> v;
        std::vector<uint16_t> ix;
        const float c[3] = {0, 0, 0}, h[3] = {0.1f, 0.1f, 0.1f};
        Box(v, ix, c, h, 0);
        g.hostVb = MakeBuffer(g.device, static_cast<UINT>(v.size() * sizeof(Vertex)), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, v.data());
        g.hostIb = MakeBuffer(g.device, static_cast<UINT>(ix.size() * 2), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, ix.data());
    }
    for (int i = 0; i < 3; ++i) {
        D3D11_TEXTURE2D_DESC t{kSize, kSize, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET, 0, 0};
        g.device->CreateTexture2D(&t, nullptr, &g.rt[i]);
        g.device->CreateRenderTargetView(g.rt[i], nullptr, &g.rtv[i]);
    }
    {
        D3D11_TEXTURE2D_DESC t{kSize, kSize, 1, 1, DXGI_FORMAT_D24_UNORM_S8_UINT, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_DEPTH_STENCIL, 0, 0};
        g.device->CreateTexture2D(&t, nullptr, &g.depthTex);
        g.device->CreateDepthStencilView(g.depthTex, nullptr, &g.dsv);
        D3D11_TEXTURE2D_DESC s{kSize, kSize, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, {1, 0}, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0};
        g.device->CreateTexture2D(&s, nullptr, &g.staging);
    }
    {
        uint32_t texels[16];
        for (int i = 0; i < 16; ++i)
            texels[i] = 0xFF000000u | (static_cast<uint32_t>(i * 16) << 16) | (static_cast<uint32_t>(255 - i * 16) << 8) | static_cast<uint32_t>((i * 37) & 0xFF);
        D3D11_TEXTURE2D_DESC t{4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, {1, 0}, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0};
        D3D11_SUBRESOURCE_DATA init{texels, 16, 0};
        g.device->CreateTexture2D(&t, &init, &g.tex);
        g.device->CreateShaderResourceView(g.tex, nullptr, &g.srv);
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        g.device->CreateSamplerState(&sd, &g.sampler);
    }
    {
        D3D11_DEPTH_STENCIL_DESC d{};
        d.DepthEnable = TRUE;
        d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        d.DepthFunc = D3D11_COMPARISON_LESS;
        d.StencilEnable = TRUE;
        d.StencilReadMask = d.StencilWriteMask = 0xFF;
        d.FrontFace = d.BackFace = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS};
        g.device->CreateDepthStencilState(&d, &g.depthWrite);
        d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        d.DepthFunc = D3D11_COMPARISON_EQUAL;
        g.device->CreateDepthStencilState(&d, &g.depthEqual);
        D3D11_RASTERIZER_DESC r{};
        r.FillMode = D3D11_FILL_SOLID;
        r.CullMode = D3D11_CULL_NONE;
        r.DepthClipEnable = TRUE;
        g.device->CreateRasterizerState(&r, &g.rs);
    }
    return g.vs && g.ps && g.layout && g.view && g.palette && g.mod && g.material && g.gunVb && g.gunIb && g.hostVb && g.hostIb && g.rtv[2] && g.dsv &&
           g.srv && g.sampler && g.depthWrite && g.depthEqual && g.rs && g.staging;
}

gunhost::Fns TestFns() {
    gunhost::Fns f;
    f.draw = &TestDraw;
    f.map = &TestMap;
    f.unmap = &TestUnmap;
    f.viewKind = nullptr;  // (by the 352-byte size alone, as skin.cpp's ViewBufferKind would have it for a view uploaded)
    return f;
}

// The lying gun's palette: its body (bone 0) at pose, its sight (bone 1) raised 3.125 cm on it, as the mod draws it lying; the rest zeros.
void GunPalette(const float* pose, uint8_t* out) {
    std::memset(out, 0, 4096);
    float sight[12] = {1, 0, 0, 0, 0, 1, 0, 0.03125f, 0, 0, 1, 0}, b1[12];
    hostrules::Compose(pose, sight, b1);
    std::memcpy(out, pose, 48);
    std::memcpy(out + 48, b1, 48);
}
// The gun the game draws (as the AT4): its body at a corner of the picture.
void HostPalette(uint8_t* out) {
    std::memset(out, 0, 4096);
    const float at[12] = {1, 0, 0, 0.75f, 0, 1, 0, -0.75f, 0, 0, 1, 0.5f};
    std::memcpy(out, at, sizeof(at));
}

struct Frame {
    float viewDx = 0.0f;
    float tint[4] = {0.9f, 0.2f, 0.1f, 1.0f};
    float hostTint[4] = {0.1f, 0.3f, 0.9f, 1.0f};
};

void Clear(ID3D11DeviceContext* c, Gpu& g) {
    const float zero[4] = {};
    for (auto* v : g.rtv)
        c->ClearRenderTargetView(v, zero);
    c->ClearDepthStencilView(g.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
// The state common to the game's draws of a pass (its view, its shaders' resources).
void Common(ID3D11DeviceContext* c, Gpu& g, const Frame& fr) {
    uint8_t view[352];
    ViewBytes(fr.viewDx, view);
    Write(c, g.view, view, sizeof(view));
    c->IASetInputLayout(g.layout);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(g.vs, nullptr, 0);
    ID3D11Buffer* cbs[3] = {g.view, g.palette, g.material};
    c->VSSetConstantBuffers(0, 3, cbs);
    c->PSSetConstantBuffers(0, 3, cbs);
    c->PSSetShaderResources(0, 1, &g.srv);
    c->PSSetSamplers(0, 1, &g.sampler);
    c->RSSetState(g.rs);
    D3D11_VIEWPORT vp{0, 0, static_cast<float>(kSize), static_cast<float>(kSize), 0, 1};
    c->RSSetViewports(1, &vp);
}
void DepthPass(ID3D11DeviceContext* c, Gpu& g) {
    c->OMSetRenderTargets(0, nullptr, g.dsv);
    c->PSSetShader(nullptr, nullptr, 0);
    c->OMSetDepthStencilState(g.depthWrite, 32);
}
void ColourPass(ID3D11DeviceContext* c, Gpu& g) {
    c->OMSetRenderTargets(3, g.rtv, g.dsv);
    c->PSSetShader(g.ps, nullptr, 0);
    c->OMSetDepthStencilState(g.depthEqual, 32);
}
// One of the game's draws: its per-draw block uploaded, its mesh, its material.
void GameDraw(ID3D11DeviceContext* c, Gpu& g, ID3D11Buffer* vb, ID3D11Buffer* ib, const uint8_t* palette, const float* tint, UINT count, UINT start) {
    Write(c, g.palette, palette, 4096);
    Write(c, g.material, tint, 16);
    const UINT stride = sizeof(Vertex), offset = 0;
    c->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    c->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
    c->DrawIndexed(count, start, 0);
}
// The game draws the lying gun as the mod places it (DrawHeldPart: the game's block uploaded, the mod's bound in its place for the draw, noted, then
// the game's back), and the mod keeps it (End).
void LyingDraw(ID3D11DeviceContext* c, Gpu& g, const float* pose, const float* tint, UINT count, UINT start, bool keep, unsigned long long now) {
    uint8_t game[4096], placed[4096];
    float held[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};  // (the game's own bones for it: at its hold, anywhere)
    GunPalette(held, game);
    GunPalette(pose, placed);
    Write(c, g.palette, game, 4096);
    Write(c, g.material, tint, 16);
    const UINT stride = sizeof(Vertex), offset = 0;
    c->IASetVertexBuffers(0, 1, &g.gunVb, &stride, &offset);
    c->IASetIndexBuffer(g.gunIb, DXGI_FORMAT_R16_UINT, 0);
    if (keep)
        gunhost::Begin();
    Write(c, g.mod, placed, 4096);
    c->VSSetConstantBuffers(1, 1, &g.mod);
    if (keep)
        gunhost::Note(c, count, start, 0, placed, 4096);
    c->DrawIndexed(count, start, 0);
    c->VSSetConstantBuffers(1, 1, &g.palette);
    if (keep)
        gunhost::End(c, TestFns(), kGun, count, start, 0, pose, g.palette, now);
}

using Image = std::vector<uint32_t>;
void ReadBack(Gpu& g, Image (&out)[3]) {
    for (int i = 0; i < 3; ++i) {
        g.imm->CopyResource(g.staging, g.rt[i]);
        D3D11_MAPPED_SUBRESOURCE m{};
        out[i].assign(kSize * kSize, 0);
        if (SUCCEEDED(g.imm->Map(g.staging, 0, D3D11_MAP_READ, 0, &m))) {
            for (UINT y = 0; y < kSize; ++y)
                std::memcpy(out[i].data() + y * kSize, static_cast<const uint8_t*>(m.pData) + y * m.RowPitch, kSize * 4);
            g.imm->Unmap(g.staging, 0);
        }
    }
}
int Differ(const Image (&a)[3], const Image (&b)[3]) {
    int n = 0;
    for (int i = 0; i < 3; ++i)
        for (size_t p = 0; p < a[i].size(); ++p)
            n += a[i][p] != b[i][p];
    return n;
}
int Covered(const Image& a) {
    int n = 0;
    for (uint32_t p : a)
        n += p != 0;
    return n;
}

// The game draws both guns itself: the reference.
void GameFrame(ID3D11DeviceContext* c, Gpu& g, const Frame& fr, const float* gunPose, Image (&out)[3]) {
    Clear(c, g);
    Common(c, g, fr);
    uint8_t gun[4096], host[4096];
    GunPalette(gunPose, gun);
    HostPalette(host);
    DepthPass(c, g);
    GameDraw(c, g, g.gunVb, g.gunIb, gun, fr.tint, kDepthCount, kDepthStart);
    GameDraw(c, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
    ColourPass(c, g);
    GameDraw(c, g, g.gunVb, g.gunIb, gun, fr.tint, kColourCount, kColourStart);
    GameDraw(c, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
    ReadBack(g, out);
}
// The game draws the lying gun (the mod placing it, keeping it): one frame.
void KeepFrame(ID3D11DeviceContext* c, Gpu& g, const Frame& fr, const float* gunPose, unsigned long long now) {
    Clear(c, g);
    Common(c, g, fr);
    DepthPass(c, g);
    LyingDraw(c, g, gunPose, fr.tint, kDepthCount, kDepthStart, true, now);
    ColourPass(c, g);
    LyingDraw(c, g, gunPose, fr.tint, kColourCount, kColourStart, true, now);
}
// The game draws only the other gun; the mod hosts the lying one after its draw in each pass (as skin.cpp's HostLying).
struct HostedFrame {
    int depthDraws = 0, colourDraws = 0;
    const char* depthWhy = "";
    const char* colourWhy = "";
};
HostedFrame HostFrame(ID3D11DeviceContext* c, Gpu& g, const Frame& fr, const float* frame, unsigned long long now, bool depthToo = true) {
    HostedFrame out;
    Clear(c, g);
    Common(c, g, fr);
    uint8_t host[4096];
    HostPalette(host);
    const gunhost::Fns fns = TestFns();
    DepthPass(c, g);
    GameDraw(c, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
    if (depthToo) {
        const gunhost::HostOut o = gunhost::Host(c, fns, kGun, frame, now);
        out.depthDraws = o.draws, out.depthWhy = o.why;
    }
    ColourPass(c, g);
    GameDraw(c, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
    const gunhost::HostOut o = gunhost::Host(c, fns, kGun, frame, now);
    out.colourDraws = o.draws, out.colourWhy = o.why;
    return out;
}

// Everything Host touches, as bound: compared before and after.
struct Bound {
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology{};
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ibFormat{};
    UINT ibOffset = 0;
    ID3D11Buffer* vb[8] = {};
    UINT stride[8] = {}, offset[8] = {};
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* vsCb[14] = {};
    ID3D11Buffer* psCb[14] = {};
    ID3D11ShaderResourceView* psSrv[32] = {};
    ID3D11SamplerState* psSampler[16] = {};
    ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT mask = 0;
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ID3D11RenderTargetView* rtv[8] = {};
    ID3D11DepthStencilView* dsv = nullptr;
    void Take(ID3D11DeviceContext* c) {
        c->IAGetInputLayout(&layout);
        c->IAGetPrimitiveTopology(&topology);
        c->IAGetIndexBuffer(&ib, &ibFormat, &ibOffset);
        c->IAGetVertexBuffers(0, 8, vb, stride, offset);
        c->VSGetShader(&vs, nullptr, nullptr);
        c->PSGetShader(&ps, nullptr, nullptr);
        c->VSGetConstantBuffers(0, 14, vsCb);
        c->PSGetConstantBuffers(0, 14, psCb);
        c->PSGetShaderResources(0, 32, psSrv);
        c->PSGetSamplers(0, 16, psSampler);
        c->RSGetState(&rs);
        c->OMGetBlendState(&blend, factor, &mask);
        c->OMGetDepthStencilState(&depth, &ref);
        c->OMGetRenderTargets(8, rtv, &dsv);
    }
    bool Same(const Bound& o) const {
        bool same = layout == o.layout && topology == o.topology && ib == o.ib && ibFormat == o.ibFormat && ibOffset == o.ibOffset && vs == o.vs && ps == o.ps &&
                    rs == o.rs && blend == o.blend && mask == o.mask && depth == o.depth && ref == o.ref && dsv == o.dsv && std::memcmp(factor, o.factor, sizeof(factor)) == 0;
        for (int i = 0; i < 8; ++i)
            same = same && vb[i] == o.vb[i] && stride[i] == o.stride[i] && offset[i] == o.offset[i] && rtv[i] == o.rtv[i];
        for (int i = 0; i < 14; ++i)
            same = same && vsCb[i] == o.vsCb[i] && psCb[i] == o.psCb[i];
        for (int i = 0; i < 32; ++i)
            same = same && psSrv[i] == o.psSrv[i];
        for (int i = 0; i < 16; ++i)
            same = same && psSampler[i] == o.psSampler[i];
        return same;
    }
    void Release() {
        SafeRelease(layout), SafeRelease(ib), SafeRelease(vs), SafeRelease(ps), SafeRelease(rs), SafeRelease(blend), SafeRelease(depth), SafeRelease(dsv);
        for (auto*& p : vb)
            SafeRelease(p);
        for (auto*& p : vsCb)
            SafeRelease(p);
        for (auto*& p : psCb)
            SafeRelease(p);
        for (auto*& p : psSrv)
            SafeRelease(p);
        for (auto*& p : psSampler)
            SafeRelease(p);
        for (auto*& p : rtv)
            SafeRelease(p);
    }
};

void GpuChecks() {
    Gpu g;
    if (!Setup(g)) {
        std::printf("SKIP w1-w14: no software graphics card (WARP) or its shader compiler here\n");
        g.Release();
        return;
    }
    gunhost::Clear();
    gunhost::SetExact(false);
    const unsigned long long t0 = 1000000;
    Frame fr;
    // An exact lying pose (a quarter turn about the view's z, moves of quarters and eighths): its bones kept and placed again bit for bit.
    const float exact[12] = {0, -1, 0, -0.25f, 1, 0, 0, -0.125f, 0, 0, 1, 0.25f};
    Image ref[3], got[3];
    GameFrame(g.imm, g, fr, exact, ref);
    const int gunPixels = Covered(ref[0]);
    CheckF(gunPixels > 300, "w1 the game draws both guns (the lying one in both its passes, the other at a corner): %d pixels covered", gunPixels);

    // ---- w2: kept, and Ready ----
    CheckF(!gunhost::Ready(kGun, t0), "w2 nothing kept before the gun lies: not ready");
    KeepFrame(g.imm, g, fr, exact, t0);
    gunhost::KindInfo info;
    const bool have = gunhost::Info(kGun, info);
    CheckF(have && info.colour == 1 && info.depth == 1 && info.subs == 2 && gunhost::Ready(kGun, t0 + 10),
           "w2 its two draws kept as the mod drew them lying (%d colour, %d depth, %d of the mod's draws): %s", info.colour, info.depth, info.subs, info.draws);
    CheckF(info.meshCopied == 2 && info.meshHeld == 0, "w2 ... its mesh (vertices and indices) copied on the graphics card (%d copied, %d held)", info.meshCopied, info.meshHeld);
    CheckF(std::strstr(info.cbs, "0:352 view") && std::strstr(info.cbs, "1:4096 palette") && std::strstr(info.cbs, "2:16 copy"),
           "w2 ... its constant buffers:%s (the view the pass's, the per-draw block the mod's bones, the material copied)", info.cbs);

    // ---- w3: hosted, the same pixels as the game's own ----
    // (The game's buffers changed after the keep, as the next draws would change them: its material another tint, its mesh's vertices garbage.)
    {
        const float other[4] = {0.0f, 1.0f, 0.0f, 1.0f};
        Write(g.imm, g.material, other, 16);
        std::vector<Vertex> junk(g.gunVertices.size());
        for (Vertex& v : junk)
            v.pos[0] = v.pos[1] = v.pos[2] = 9.0f;
        g.imm->UpdateSubresource(g.gunVb, 0, nullptr, junk.data(), 0, 0);
    }
    HostedFrame h = HostFrame(g.imm, g, fr, exact, t0 + 20);
    ReadBack(g, got);
    CheckF(h.depthDraws == 1 && h.colourDraws == 1, "w3 hosted after the other gun's depth draw (%d drawn: %s) and its colour draw (%d drawn: %s)", h.depthDraws,
           h.depthWhy, h.colourDraws, h.colourWhy);
    CheckF(Differ(ref, got) == 0, "w3 ... the same pixels in all three targets as the game's own draws of it there (%d differ; the material's tint and the mesh as kept, "
           "though the game's changed since)", Differ(ref, got));
    // Put the game's mesh back for the references below.
    g.imm->UpdateSubresource(g.gunVb, 0, nullptr, g.gunVertices.data(), 0, 0);

    // ---- w4: moved with its body ----
    {
        const float a[3] = {0.0f, 0.0f, 1.0f};
        float moved[12];
        Pose(moved, a, 0.6f, 0.31f, 0.17f, -0.4f);
        GameFrame(g.imm, g, fr, moved, ref);
        HostFrame(g.imm, g, fr, moved, t0 + 30);
        ReadBack(g, got);
        const int differ = Differ(ref, got);
        CheckF(differ <= 6, "w4 placed where loose has it now (another pose): as the game's own draws of it there, %d of %d pixels differing (6: a last bit at an edge)",
               differ, 3 * static_cast<int>(kSize * kSize));
    }
    // ---- w5: the host pass's own view ----
    {
        Frame other = fr;
        other.viewDx = -0.125f;
        GameFrame(g.imm, g, other, exact, ref);
        HostFrame(g.imm, g, other, exact, t0 + 40);
        ReadBack(g, got);
        CheckF(Differ(ref, got) == 0, "w5 drawn with the host pass's own view constants (the view moved a sixteenth of the picture): as the game's own (%d differ)", Differ(ref, got));
    }
    // ---- w6: every state Host touches put back ----
    {
        Clear(g.imm, g);
        Common(g.imm, g, fr);
        ColourPass(g.imm, g);
        uint8_t host[4096];
        HostPalette(host);
        GameDraw(g.imm, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
        Bound before, after;
        before.Take(g.imm);
        const gunhost::HostOut o = gunhost::Host(g.imm, TestFns(), kGun, exact, t0 + 50);
        after.Take(g.imm);
        CheckF(o.draws == 1 && before.Same(after), "w6 after a host (%d drawn): the input stage, both shaders and all they had bound, the rasterizer, blend and depth "
               "states, the targets as they were", o.draws);
        before.Release(), after.Release();
    }
    // ---- w7: on a deferred context (the game records on several) ----
    {
        ID3D11DeviceContext* deferred = nullptr;
        if (SUCCEEDED(g.device->CreateDeferredContext(0, &deferred)) && deferred) {
            GameFrame(g.imm, g, fr, exact, ref);
            gunhost::Clear();
            KeepFrame(deferred, g, fr, exact, t0 + 60);  // (kept on the deferred context: its copies made there)
            ID3D11CommandList* list = nullptr;
            deferred->FinishCommandList(FALSE, &list);
            if (list)
                g.imm->ExecuteCommandList(list, FALSE), list->Release();
            HostFrame(deferred, g, fr, exact, t0 + 70);
            deferred->FinishCommandList(FALSE, &list);
            if (list)
                g.imm->ExecuteCommandList(list, FALSE), list->Release();
            ReadBack(g, got);
            CheckF(Differ(ref, got) == 0, "w7 kept and hosted on a deferred context, its command lists run on the immediate one: as the game's own (%d differ)", Differ(ref, got));
            deferred->Release();
        } else {
            Check(false, "w7 a deferred context made");
        }
    }
    // ---- w8: nothing into another kind of pass ----
    {
        Clear(g.imm, g);
        Common(g.imm, g, fr);
        g.imm->OMSetRenderTargets(2, g.rtv, g.dsv);  // (two targets: no pass of its draws')
        g.imm->PSSetShader(g.ps, nullptr, 0);
        const gunhost::HostOut o = gunhost::Host(g.imm, TestFns(), kGun, exact, t0 + 80);
        CheckF(o.draws == 0 && std::strstr(o.why, "pass") != nullptr, "w8 a pass of another kind (two targets): nothing drawn (%s)", o.why);
        g.imm->OMSetRenderTargets(0, nullptr, nullptr);
        const gunhost::HostOut none = gunhost::Host(g.imm, TestFns(), kGun, exact, t0 + 80);
        CheckF(none.draws == 0, "w8 ... no depth target bound: nothing (%s)", none.why);
    }
    // ---- w9: no view constants of their size ----
    {
        Clear(g.imm, g);
        Common(g.imm, g, fr);
        ColourPass(g.imm, g);
        g.imm->VSSetConstantBuffers(0, 1, &g.little);
        g.imm->PSSetConstantBuffers(0, 1, &g.little);
        gunhost::TakeStats();
        Bound before, after;
        before.Take(g.imm);
        const gunhost::HostOut o = gunhost::Host(g.imm, TestFns(), kGun, exact, t0 + 90);
        after.Take(g.imm);
        const gunhost::Stats s = gunhost::TakeStats();
        CheckF(o.draws == 0 && s.noView == 1 && before.Same(after), "w9 no view constants of the kept draw's size in the pass: nothing drawn (counted %u), everything as it was",
               s.noView);
        before.Release(), after.Release();
    }
    // ---- w10: its colour without its depth pass hosted ----
    {
        HostFrame(g.imm, g, fr, exact, t0 + 100, false);
        ReadBack(g, got);
        const int relaxed = Covered(got[0]);
        gunhost::SetExact(true);
        HostFrame(g.imm, g, fr, exact, t0 + 110, false);
        ReadBack(g, got);
        const int exactly = Covered(got[0]);
        gunhost::SetExact(false);
        Image hostOnly[3];
        {
            Clear(g.imm, g);
            Common(g.imm, g, fr);
            uint8_t host[4096];
            HostPalette(host);
            DepthPass(g.imm, g);
            GameDraw(g.imm, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
            ColourPass(g.imm, g);
            GameDraw(g.imm, g, g.hostVb, g.hostIb, host, fr.hostTint, kHostCount, 0);
            ReadBack(g, hostOnly);
        }
        const int other = Covered(hostOnly[0]);
        CheckF(relaxed == gunPixels && exactly == other, "w10 its depth pass not hosted in that eye's frame: drawn all the same, relaxed (%d pixels, the game's both guns %d); "
               "lyinghostexact 1 draws none of it (%d, the other gun's alone %d)", relaxed, gunPixels, exactly, other);
    }
    // ---- w11: the notes ----
    {
        ID3D11RasterizerState* rs = g.rs;
        g.imm->RSSetState(rs);
        rs->AddRef();
        const ULONG before = rs->Release();
        gunhost::Begin();
        uint8_t bytes[4096] = {};
        for (int i = 0; i < 3; ++i)
            gunhost::Note(g.imm, 36, 0, 0, bytes, 4096);
        gunhost::Cancel();
        rs->AddRef();
        const ULONG afterCancel = rs->Release();
        CheckF(before == afterCancel && !gunhost::Noting(), "w11 notes cancelled (a way out of the draw between): their rasterizer states let go of (%lu, %lu)", before, afterCancel);
        gunhost::KindInfo was;
        gunhost::Info(kGun, was);
        gunhost::Begin();
        for (int i = 0; i < gunhost::kMostSubs + 1; ++i)
            gunhost::Note(g.imm, 36, 0, 0, bytes, 4096);
        float id[12];
        Identity(id);
        gunhost::End(g.imm, TestFns(), kGun, 36, 0, 0, id, g.palette, t0 + 120);
        gunhost::KindInfo now;
        gunhost::Info(kGun, now);
        CheckF(now.colour + now.depth == was.colour + was.depth && std::strstr(now.draws, "36/0") == nullptr,
               "w11 more of the mod's draws for one draw than kept (%d): nothing kept of it", gunhost::kMostSubs + 1);
        gunhost::End(g.imm, TestFns(), kGun, 36, 0, 0, id, g.palette, t0 + 120);  // (no Begin: nothing)
        gunhost::Info(kGun, now);
        CheckF(std::strstr(now.draws, "36/0") == nullptr, "w11 ... and an End with no notes keeps nothing");
    }
    // ---- w12: a draw whose per-draw block isn't bound: its state refused, said ----
    {
        gunhost::TakeStats();
        Clear(g.imm, g);
        Common(g.imm, g, fr);
        ColourPass(g.imm, g);
        gunhost::Begin();
        uint8_t bytes[4096] = {};
        gunhost::Note(g.imm, 36, 0, 0, bytes, 4096);
        float id[12];
        Identity(id);
        gunhost::End(g.imm, TestFns(), 4242, 36, 0, 0, id, g.mod, t0 + 130);  // (the mod's own buffer named, not the one bound)
        const gunhost::Stats s = gunhost::TakeStats();
        CheckF(s.refused == 1 && std::strstr(s.refusal, "per-draw block") != nullptr && !gunhost::Ready(4242, t0 + 130),
               "w12 a draw whose per-draw block isn't bound in its vertex shader: not kept (%s)", s.refusal);
    }
    // ---- w13: Sweep ----
    {
        const uint32_t hosted[1] = {kGun};
        gunhost::Sweep(t0 + 200 + hostrules::kIdleMs + 1000, hosted, 1);
        const bool kept = gunhost::Ready(kGun, t0 + 200);
        gunhost::Sweep(t0 + 200 + hostrules::kIdleMs + 1000, nullptr, 0);
        gunhost::KindInfo none;
        const bool gone = !gunhost::Info(kGun, none);
        CheckF(kept && gone, "w13 a kind idle %llu s: kept while hosted, let go of once not", hostrules::kIdleMs / 1000);
    }
    // ---- w14: Clear ----
    {
        KeepFrame(g.imm, g, fr, exact, t0 + 300);
        const bool ready = gunhost::Ready(kGun, t0 + 300);
        gunhost::Clear();
        CheckF(ready && !gunhost::Ready(kGun, t0 + 300), "w14 lyinghost 0: everything kept let go of (Clear)");
        HostedFrame after = HostFrame(g.imm, g, fr, exact, t0 + 310);
        CheckF(after.colourDraws == 0 && after.depthDraws == 0, "w14 ... nothing drawn by Host after (%s)", after.colourWhy);
    }
    gunhost::Clear();
    g.Release();
}

}  // namespace

int main() {
    PureChecks();
    GpuChecks();
    std::printf("%d of %d checks failed\n", g_failures, g_checks);
    return g_failures;
}

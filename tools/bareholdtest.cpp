// Blackburn's bare arms with a gun in hand, as the gloves under another numbering (src/barehold.h, header only, as skin.cpp's DrawBareArms runs it) without the game.
// Semper Fidelis's train after the grapple, 2026-11-18: the bare right hand was a fist under the rear of the receiver and the left flat and open, where the gloves get
// the game's hold on the gun and the fitted hands. Each check prints PASS, FAIL or SKIP, then the totals; the exit code is the number failed.
//   h1  the table: every one of the gloves' 45 bones is given by ToGloves exactly once (42 from the hands' bones, the three cuff bones 4, 5 and 44 from the bare
//       forearm's 49, 50 and 23); the bare bones with no glove's number are the forearms' own, 21-25 and 47-51; the palms 41 and 15 are the gloves' 1 and 32, the
//       wrist ends 46 and 20 the cuffs' 0 and 37; each hand's bones stay on their own side
//   h2  there and back: a palette of random bones put ToGloves and FromGloves comes back the same on the 42 mapped bones, and FromGloves leaves the forearms' own
//       as they were
//   h3  the bare forearms' draw's layout (ToForearmsDraw): its entries 0 and 12, which the rig's folds are measured by, are the bare forearms' middle bones 50 and 23,
//       every entry with vertices is a bone of its own side, the entries without are left as given
//   h4  FromRig, both hands there, each forearm solved from the hand drawn: every hand bone is its glove's; the forearm's bones are the solved frames (wrist end on
//       twist bone B, the next on A, the middle on the elbow end); with no cap the rim is the elbow end's frame, with all of it the rim's points lie within 3 mm
//       of the elbow joint
//   h5  FromRig, a forearm solved from another hand (its wrist 5 cm from the drawn hand's): not used; the game's own forearm goes whole with the hand (each of its
//       bones keeps its place on the palm bone), nothing tears at the wrist; a forearm 1 cm off is still used
//   h6  FromRig, a hand folded away: every bone of that side, hand and forearm, lands every point on the fold point (nothing drawn), the other side untouched by it
//   h7  the fitted hands (src/magposes: the AKS-74u's and the UMP-45's magazine holds, the AKS-74u's lever fists over and under, the UMP-45's lever hand) put on the
//       left glove as hands.cpp does (leftPose times each bone) come out on the bare left hand bone for bone, no bone left as the game's own. (Said with it: how
//       far each one's furthest fingertip is from the palm, beside the game's own handguard hand recorded on the train.)
//   h8  the recordings (build\survey: AKS74u_partial.bin, UMP45_partial.bin, M1014_partial.bin handcuffed, aks_test_idle.bin with the cuffs off; SKIP without them):
//       a  with tools\baregloves.ps1's copy beside it (AKS74u_partial_gloves.bin, UMP45_partial_gloves.bin): ToGloves gives that script's gloves' record, byte for
//          byte on the 42 mapped bones, for every bare-arms record
//       b  the rig can learn the game's hold from the bare draw: the two palms, read as LearnRig reads the gloves' (bone 37 with the right palm's point, bone 1 with
//          the left's), rest on the gun's frame within a centimetre for a second and more (NoteHeldPalms' rule), the right behind and below the left
//       c  placed as the rig places the gloves (each bone where the game's rests on the frame, the gun at another place and turn), FromRig puts every bare hand bone
//          on the held gun exactly as the game has it on its own: the right hand round the pistol grip, the left on the handguard; the two eyes' records of a frame
//          give the same hold (within a millimetre)
//   h9  every fitted hand's table drawn on bare skin is rigid (the review, 2026-11-18: the M1014's shell pinch and the AKS-74u's under-hand on the bolt handle, made on
//       the glove at palm width 1.15 with the fingers slimmed to 0.88, went onto the bare hand whole: the palm 15% wider at once). As hands.cpp puts them on skin
//       (handpose::SkinTable):
//       a  every table the left glove's fingers can be given (src/magposes' magazines, levers, lid and belt, the shell pinch, the launcher's case): each bone's
//          determinant within 0.001 of 1; a table rigid as made comes back bit for bit; the widened ones are named
//       b  the widening taken out is the same hand made on skin: handpose::Pose's hand at width 1.15, glove-slimmed, any fingers (and moved whole anywhere), comes back
//          as Pose's at width 1 with the fingers' whole width, every bone within 0.1 mm over the hand
// Usage: bareholdtest [<folder with the recordings>] (build\survey by default, from the project's folder). Reads only.

#include "../src/barehold.h"
#include "../src/handpose.h"
#include "../src/magposes.h"
#include "../src/rocketposes.h"
#include "../src/shellposes.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace {

using barehold::kBareBones;
using barehold::kBareGlove;
using barehold::kGloveBones;

int g_checks = 0, g_failed = 0, g_skipped = 0;
void Check(bool ok, const char* id, const char* format, ...) {
    char text[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %-4s %s\n", ok ? "PASS" : "FAIL", id, text);
}
void Skip(const char* id, const char* what) {
    ++g_skipped;
    std::printf("SKIP %-4s %s\n", id, what);
}

// skin.cpp's points in the gloves' bind pose: the right hand root's vertices' centre, the left hand's centre.
constexpr float kRightPalm[3] = {-0.505f, 1.129f, 0.150f}, kLeftPalm[3] = {0.537f, 1.098f, 0.207f};

using barehold::detail::Compose;
using barehold::detail::Invert;
using barehold::detail::Point;

float Dist(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
float Apart(const float* a, const float* b) {  // two bones: the worst entry apart
    float worst = 0;
    for (int i = 0; i < 12; ++i)
        worst = std::max(worst, std::fabs(a[i] - b[i]));
    return worst;
}
void Rigid(std::mt19937& g, float* m, float reach = 1.0f) {  // a random turn and place
    std::uniform_real_distribution<float> u(-1, 1);
    float q[4] = {u(g), u(g), u(g), u(g)};
    const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]) + 1e-9f;
    const float x = q[0] / n, y = q[1] / n, z = q[2] / n, w = q[3] / n;
    const float r[12] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), u(g) * reach, 2 * (x * y + z * w), 1 - 2 * (x * x + z * z),
                         2 * (y * z - x * w),     u(g) * reach,        2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), u(g) * reach};
    std::memcpy(m, r, sizeof(r));
}
int SideOfBare(int k) {  // 0 left, 1 right
    return k >= barehold::kBareSide[0][0] ? 0 : 1;
}
int SideOfGlove(int g) {
    return g < handpose::kRightFirst ? 0 : 1;
}

void Table() {
    int given[kGloveBones] = {}, wrongSide = 0, forearmOnly = 0, forearmWrong = 0;
    for (int k = 0; k < kBareBones; ++k) {
        if (kBareGlove[k] >= 0) {
            ++given[kBareGlove[k]];
            wrongSide += SideOfBare(k) != SideOfGlove(kBareGlove[k]);
        } else {
            ++forearmOnly;
            forearmWrong += !((k >= 21 && k <= 25) || (k >= 47 && k <= 51)) || !barehold::ForearmOnly(k);
        }
    }
    for (const auto& only : barehold::kGloveOnly) {
        ++given[only[0]];
        wrongSide += SideOfBare(only[1]) != SideOfGlove(only[0]) || !barehold::ForearmOnly(only[1]) || !barehold::GloveOnly(only[0]);
    }
    int notOnce = 0;
    for (int g : given)
        notOnce += g != 1;
    Check(notOnce == 0 && wrongSide == 0 && forearmOnly == 10 && forearmWrong == 0 && barehold::Mapped() == 42 && kBareGlove[41] == 1 && kBareGlove[15] == 32 &&
              kBareGlove[46] == 0 && kBareGlove[20] == 37 && barehold::kBarePalm[0] == 41 && barehold::kBarePalm[1] == 15,
          "h1", "the table: %d of the gloves' 45 bones not given exactly once (0 wanted), %d on the wrong side, %d bare bones mapped (42), %d the forearms' own (10: 21-25 and "
                "47-51, %d elsewhere); the palms 41 -> %d and 15 -> %d (1 and 32), the wrist ends 46 -> %d and 20 -> %d (0 and 37)",
          notOnce, wrongSide, barehold::Mapped(), forearmOnly, forearmWrong, kBareGlove[41], kBareGlove[15], kBareGlove[46], kBareGlove[20]);
}

void RoundTrip() {
    std::mt19937 g(3);
    float bare[kBareBones * 12], gloves[kGloveBones * 12], back[kBareBones * 12], marker[12];
    for (int k = 0; k < kBareBones; ++k)
        Rigid(g, bare + k * 12);
    for (int i = 0; i < 12; ++i)
        marker[i] = -7.0f - i;
    for (int k = 0; k < kGloveBones; ++k)
        std::memcpy(gloves + k * 12, marker, sizeof(marker));
    for (int k = 0; k < kBareBones; ++k)
        std::memcpy(back + k * 12, marker, sizeof(marker));
    barehold::ToGloves(bare, gloves);
    barehold::FromGloves(gloves, back);
    int differ = 0, touched = 0, unset = 0;
    for (int k = 0; k < kGloveBones; ++k)
        unset += !std::memcmp(gloves + k * 12, marker, sizeof(marker));
    for (int k = 0; k < kBareBones; ++k) {
        if (kBareGlove[k] >= 0)
            differ += std::memcmp(back + k * 12, bare + k * 12, 48) != 0;
        else
            touched += std::memcmp(back + k * 12, marker, sizeof(marker)) != 0;
    }
    Check(differ == 0 && touched == 0 && unset == 0, "h2", "there and back: %d of the 42 mapped bones not the same (0 wanted), %d of the forearms' own touched by FromGloves (0), %d of "
                                                           "the gloves' bones left unset by ToGloves (0)",
          differ, touched, unset);
}

void ForearmsDraw() {
    std::mt19937 g(5);
    float bare[kBareBones * 12], draw[barehold::kForearmsDrawBones * 12], marker[12];
    for (int k = 0; k < kBareBones; ++k)
        Rigid(g, bare + k * 12);
    for (int i = 0; i < 12; ++i)
        marker[i] = -3.0f - i;
    for (int k = 0; k < barehold::kForearmsDrawBones; ++k)
        std::memcpy(draw + k * 12, marker, sizeof(marker));
    barehold::ToForearmsDraw(bare, draw);
    // skin.cpp's kArmDraws[2] roles: "LLLLL-L-LLRRRRR-RR".
    const char* roles = "LLLLL-L-LLRRRRR-RR";
    int wrong = 0;
    for (int k = 0; k < barehold::kForearmsDrawBones; ++k) {
        const int from = barehold::kForearmsDraw[k];
        if (roles[k] == '-')
            wrong += from >= 0 || std::memcmp(draw + k * 12, marker, sizeof(marker)) != 0;
        else
            wrong += from < 0 || SideOfBare(from) != (roles[k] == 'L' ? 0 : 1) || std::memcmp(draw + k * 12, bare + from * 12, 48) != 0;
    }
    Check(wrong == 0 && barehold::kForearmsDraw[0] == 50 && barehold::kForearmsDraw[12] == 23, "h3",
          "the bare forearms' draw's layout: %d of its 18 entries wrong (0 wanted); the folds' entries 0 and 12 are the bare arms' %d and %d (50 and 23)", wrong,
          barehold::kForearmsDraw[0], barehold::kForearmsDraw[12]);
}

// A solved arm for a hand: armik's own, the shoulder a little way off.
struct Solved {
    armik::Result r;
    barehold::Arm arm;
};
void Solve(const float* hand, bool right, Solved& s) {
    const float out[3] = {right ? 1.0f : -1.0f, 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
    const float W[3] = {right ? -armik::kWrist[0] : armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]};
    float w[3];
    Point(hand, W, w);
    const float shoulder[3] = {w[0] + (right ? 0.10f : -0.10f), w[1] + 0.20f, w[2] + 0.35f};
    armik::Params p;
    p.flex = 100.0f, p.ext = 95.0f, p.radial = 45.0f, p.ulnar = 60.0f, p.share[0] = 0.5f, p.share[1] = 0.8f, p.share[2] = 1.0f, p.elbowReach = 0.05f;
    armik::State st;
    armik::Solve(hand, shoulder, out, up, back, right, p, st, s.r);
    s.arm.have = true;
    s.arm.frame = s.r.frame;
    s.arm.cap = 0.0f;
}

void FromRigChecks() {
    std::mt19937 g(9);
    float game[kBareBones * 12], gloves[kGloveBones * 12];
    for (int k = 0; k < kBareBones; ++k)
        Rigid(g, game + k * 12, 0.3f);
    for (int k = 0; k < kGloveBones; ++k)
        Rigid(g, gloves + k * 12, 0.3f);
    const bool both[2] = {true, true};
    const float fold[2][3] = {{0.1f, -0.2f, -0.3f}, {-0.1f, -0.25f, -0.35f}};
    Solved solved[2];
    Solve(gloves + 1 * 12, false, solved[0]);
    Solve(gloves + 32 * 12, true, solved[1]);
    // h4: solved from the hands drawn.
    {
        float worstHand = 0, worstArm = 0, rimNoCap = 0, rimCapped = 0;
        bool used = true;
        for (int cap = 0; cap < 2; ++cap) {
            float bare[kBareBones * 12];
            std::memcpy(bare, game, sizeof(bare));
            barehold::Arm arms[2] = {solved[0].arm, solved[1].arm};
            arms[0].cap = arms[1].cap = static_cast<float>(cap);
            const barehold::Placed placed = barehold::FromRig(gloves, both, fold, arms, bare);
            used = used && placed.solved[0] && placed.solved[1] && placed.wristOff[0] < 1e-4f && placed.wristOff[1] < 1e-4f;
            for (int k = 0; k < kBareBones; ++k)
                if (kBareGlove[k] >= 0 && k != 46 && k != 20)
                    worstHand = std::max(worstHand, Apart(bare + k * 12, gloves + kBareGlove[k] * 12));
            for (int side = 0; side < 2; ++side) {
                const float E[3] = {side == 0 ? armik::kElbow[0] : -armik::kElbow[0], armik::kElbow[1], armik::kElbow[2]};
                for (int f = 0; f < barehold::kForearmBones; ++f) {
                    const int bone = barehold::kForearm[side][f][0];
                    const float* frame = solved[side].r.frame[barehold::kForearm[side][f][1]];
                    if (f < barehold::kRimFrom) {
                        worstArm = std::max(worstArm, Apart(bare + bone * 12, frame));
                    } else if (!cap) {
                        rimNoCap = std::max(rimNoCap, Apart(bare + bone * 12, frame));
                    } else {
                        for (const float off : {0.0f, 0.04f, 0.09f}) {  // points up to 9 cm from the joint, as the mesh's rim
                            const float p[3] = {E[0] + off, E[1] + off * 0.5f, E[2] - off * 0.3f};
                            float at[3];
                            Point(bare + bone * 12, p, at);
                            rimCapped = std::max(rimCapped, Dist(at, solved[side].r.elbow));
                        }
                    }
                }
            }
        }
        Check(used && worstHand == 0.0f && worstArm == 0.0f && rimNoCap == 0.0f && rimCapped <= 0.0035f, "h4",
              "both hands there, each forearm solved from the hand drawn: the hands' bones %.1e from their gloves' (0 wanted), the forearms' %.1e from the solved frames (0), the "
              "rim uncapped %.1e from the elbow end's frame (0), capped within %.1f mm of the elbow joint (<= 3.5)%s",
              worstHand, worstArm, rimNoCap, rimCapped * 1000, used ? "" : "; A SOLVED FOREARM NOT USED");
    }
    // h5: solved from another hand.
    {
        float worstKeep = 0, worstHand = 0;
        bool refused = true, kept = true;
        for (const float off : {0.05f, 0.01f}) {
            float bare[kBareBones * 12], moved[2][3][12];
            std::memcpy(bare, game, sizeof(bare));
            barehold::Arm arms[2] = {solved[0].arm, solved[1].arm};
            for (int side = 0; side < 2; ++side) {
                std::memcpy(moved[side], solved[side].r.frame, sizeof(moved[side]));
                for (auto& frame : moved[side])
                    frame[7] += off;  // the whole solved forearm this far up from the hand
                arms[side].frame = moved[side];
            }
            const barehold::Placed placed = barehold::FromRig(gloves, both, fold, arms, bare);
            if (off > barehold::kWristTogether) {
                refused = refused && !placed.solved[0] && !placed.solved[1];
                for (int side = 0; side < 2; ++side) {
                    // Each of the forearm's own bones keeps its place on the palm bone: palm^-1 * bone as the game had it.
                    float gameInverse[12], nowInverse[12];
                    Invert(game + barehold::kBarePalm[side] * 12, gameInverse);
                    Invert(bare + barehold::kBarePalm[side] * 12, nowInverse);
                    for (int k = barehold::kBareSide[side][0]; k <= barehold::kBareSide[side][1]; ++k) {
                        if (kBareGlove[k] >= 0) {
                            worstHand = std::max(worstHand, Apart(bare + k * 12, gloves + kBareGlove[k] * 12));
                            continue;
                        }
                        float was[12], is[12];
                        Compose(gameInverse, game + k * 12, was);
                        Compose(nowInverse, bare + k * 12, is);
                        worstKeep = std::max(worstKeep, Apart(was, is));
                    }
                }
            } else {
                kept = kept && placed.solved[0] && placed.solved[1];
            }
        }
        Check(refused && kept && worstKeep <= 1e-4f && worstHand == 0.0f, "h5",
              "a forearm solved from another hand (its wrist 5 cm off the drawn hand's): %s; the game's forearm whole with the hand, each bone %.1e from its place on the palm "
              "bone (<= 1e-4), the hands' bones %.1e from their gloves'; one 1 cm off: %s",
              refused ? "not used" : "USED", worstKeep, worstHand, kept ? "used" : "NOT USED");
    }
    // h6: a hand folded away.
    {
        float worst = 0, other = 0;
        for (int gone = 0; gone < 2; ++gone) {
            float bare[kBareBones * 12], whole[kBareBones * 12];
            std::memcpy(bare, game, sizeof(bare));
            std::memcpy(whole, game, sizeof(whole));
            const bool here[2] = {gone != 0, gone != 1};
            const barehold::Arm arms[2] = {solved[0].arm, solved[1].arm};
            barehold::FromRig(gloves, here, fold, arms, bare);
            barehold::FromRig(gloves, both, fold, arms, whole);
            for (int k = 0; k < kBareBones; ++k) {
                if (SideOfBare(k) != gone) {
                    other = std::max(other, Apart(bare + k * 12, whole + k * 12));
                    continue;
                }
                for (const float* p : {kLeftPalm, kRightPalm, armik::kElbow}) {
                    float at[3];
                    Point(bare + k * 12, p, at);
                    worst = std::max(worst, Dist(at, fold[gone]));
                }
            }
        }
        Check(worst == 0.0f && other == 0.0f, "h6", "a hand folded away: every bone of that side lands every point %.1e from its fold point (0 wanted); the other side %.1e from "
                                                    "what it is with both there (0)",
              worst, other);
    }
}

// How open a left hand is: the furthest of its four fingers' tips (handpose::Joints' tips, each moved by its finger's last bone: the index's 15, the middle's 22, the
// ring's 10, the little finger's 3) from the palm's centre, on the palm bone (1). bones: the left glove's, from the bind space into anything.
float FurthestTip(const float (*fingers)[12]) {
    static constexpr int kLast[4] = {15, 22, 10, 3};
    float joints[5][4][3], palmInverse[12], worst = 0;
    handpose::Joints(joints);
    if (!Invert(fingers[1], palmInverse))
        return 99.0f;
    for (int f = 0; f < 4; ++f) {
        float at[3], onPalm[3];
        Point(fingers[kLast[f]], joints[f + 1][3], at);
        Point(palmInverse, at, onPalm);
        worst = std::max(worst, Dist(onPalm, kLeftPalm));
    }
    return worst;
}

// ---- The recordings ----
struct Record {
    int32_t head[8];  // the draw's index count, the frame, the eye, its start, the rounds, the spare, the ms, the bones' count
    const float* bones;
};
bool Load(const std::string& path, std::vector<uint8_t>& bytes, std::vector<Record>& records) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
        return false;
    _fseeki64(f, 0, SEEK_END);
    const long long size = _ftelli64(f);
    _fseeki64(f, 0, SEEK_SET);
    bytes.resize(static_cast<size_t>(size));
    const bool read = size > 0 && fread(bytes.data(), 1, bytes.size(), f) == bytes.size();
    fclose(f);
    if (!read)
        return false;
    size_t at = 0;
    while (at + 32 <= bytes.size()) {
        Record r;
        std::memcpy(r.head, bytes.data() + at, 32);
        if (r.head[7] <= 0 || r.head[7] > 64 || at + 32 + static_cast<size_t>(r.head[7]) * 48 > bytes.size())
            break;
        r.bones = reinterpret_cast<const float*>(bytes.data() + at + 32);
        records.push_back(r);
        at += 32 + static_cast<size_t>(r.head[7]) * 48;
    }
    return !records.empty();
}
bool IsArms(const Record& r) {
    return r.head[0] == 18108 && (r.head[3] == 16053 || r.head[3] == 13371) && r.head[7] >= kBareBones;
}

struct Gun {
    const char* name;
    const char* file;
    const char* gloves;  // baregloves.ps1's copy, or none
    int body, bodyStart, frame;  // its colour draw and its frame bone (skin.cpp's kHeldKinds)
};
constexpr Gun kGuns[] = {{"the AKS-74u", "AKS74u_partial.bin", "AKS74u_partial_gloves.bin", 17991, 0, 2},
                         {"the UMP-45", "UMP45_partial.bin", "UMP45_partial_gloves.bin", 19314, 234, 2},
                         {"the M1014", "M1014_partial.bin", nullptr, 13377, 0, 3},
                         {"the AKS-74u, the cuffs off", "aks_test_idle.bin", nullptr, 17991, 0, 2}};

float g_gameLeftOpen = -1.0f;  // the game's own handguard hand's furthest fingertip from its palm, from a recording (h7 compares the fitted hands with it)

void Recording(const std::string& dir, const Gun& gun, int index) {
    char id[8];
    std::vector<uint8_t> bytes;
    std::vector<Record> records;
    if (!Load(dir + "\\" + gun.file, bytes, records)) {
        std::snprintf(id, sizeof(id), "h8.%d", index);
        char what[256];
        std::snprintf(what, sizeof(what), "%s: no recording at %s\\%s", gun.name, dir.c_str(), gun.file);
        Skip(id, what);
        return;
    }
    // a: against baregloves.ps1's copy.
    std::snprintf(id, sizeof(id), "h8a%d", index);
    if (gun.gloves) {
        std::vector<uint8_t> copyBytes;
        std::vector<Record> copy;
        if (!Load(dir + "\\" + gun.gloves, copyBytes, copy)) {
            Skip(id, "no baregloves.ps1 copy beside it");
        } else {
            int arms = 0, differ = 0, missing = 0;
            for (size_t i = 0; i + 1 < copy.size(); ++i) {
                if (!IsArms(copy[i]))
                    continue;
                ++arms;
                const Record& made = copy[i + 1];
                if (made.head[0] != 13755 || made.head[3] != 15051 || made.head[7] != kGloveBones || made.head[1] != copy[i].head[1] || made.head[2] != copy[i].head[2]) {
                    ++missing;
                    continue;
                }
                float gloves[kGloveBones * 12];
                barehold::ToGloves(copy[i].bones, gloves);
                bool same = true;
                for (int k = 0; k < kBareBones; ++k)
                    if (kBareGlove[k] >= 0)
                        same = same && !std::memcmp(gloves + kBareGlove[k] * 12, made.bones + kBareGlove[k] * 12, 48);
                differ += !same;
            }
            Check(arms > 0 && differ == 0 && missing == 0, id, "%s: %d bare-arms records: ToGloves differs from baregloves.ps1's gloves' record on the 42 mapped bones in %d (0 wanted), "
                                                              "%d without one",
                  gun.name, arms, differ, missing);
        }
    }
    // The left eye's arms and gun body of each frame, in the recording's order.
    struct Frame {
        int ms;
        const float* arms;
        const float* body;
        const float* armsOther;  // the other eye's
        const float* bodyOther;
    };
    std::vector<Frame> frames;
    {
        int frameNo = -1;
        Frame f{};
        auto push = [&] {
            if (f.arms && f.body)
                frames.push_back(f);
        };
        for (const Record& r : records) {
            if (r.head[1] != frameNo) {
                push();
                f = Frame{};
                frameNo = r.head[1];
                f.ms = r.head[6];
            }
            const bool first = r.head[2] == 0 || (r.head[2] != 1 && !f.arms);
            if (IsArms(r))
                (first && !f.arms ? f.arms : f.armsOther) = r.bones;
            else if (r.head[0] == gun.body && r.head[3] == gun.bodyStart && r.head[7] > gun.frame)
                (r.head[2] == 0 || (r.head[2] != 1 && !f.body) ? f.body : f.bodyOther) = r.bones;
        }
        push();
    }
    // The palms on the gun's frame, as LearnRig reads them from the gloves' draw.
    auto palms = [&](const float* arms, const float* body, float* right, float* left, float* inverseOut = nullptr) {
        float gloves[kGloveBones * 12], inverse[12], at[3];
        barehold::ToGloves(arms, gloves);
        if (!Invert(body + gun.frame * 12, inverse))
            return false;
        Point(gloves + 37 * 12, kRightPalm, at);
        Point(inverse, at, right);
        Point(gloves + 1 * 12, kLeftPalm, at);
        Point(inverse, at, left);
        if (inverseOut)
            std::memcpy(inverseOut, inverse, sizeof(inverse));
        return true;
    };
    // b: the longest rest: both palms within a centimetre of where the stretch began (NoteHeldPalms' try: kHeldTryWithin 0.01, taken after kHeldFirstRest 1000 ms).
    std::snprintf(id, sizeof(id), "h8b%d", index);
    int bestFrom = -1, bestTo = -1, from = -1;
    float tryRight[3] = {}, tryLeft[3] = {};
    for (int i = 0; i < static_cast<int>(frames.size()); ++i) {
        float right[3], left[3];
        if (!palms(frames[i].arms, frames[i].body, right, left)) {
            from = -1;
            continue;
        }
        if (from < 0 || std::max(Dist(right, tryRight), Dist(left, tryLeft)) > 0.01f) {
            from = i;
            std::memcpy(tryRight, right, sizeof(right));
            std::memcpy(tryLeft, left, sizeof(left));
        }
        if (bestFrom < 0 || frames[i].ms - frames[from].ms > frames[bestTo].ms - frames[bestFrom].ms)
            bestFrom = from, bestTo = i;
    }
    if (bestFrom < 0) {
        Check(false, id, "%s: no frame with the arms and the gun's body (%d from %d) in %s", gun.name, gun.body, gun.bodyStart, gun.file);
        return;
    }
    const Frame& rest = frames[(bestFrom + bestTo) / 2];
    float restRight[3], restLeft[3], frameInverse[12];
    palms(rest.arms, rest.body, restRight, restLeft, frameInverse);
    const int restMs = frames[bestTo].ms - frames[bestFrom].ms;
    Check(restMs >= 1000 && restRight[2] < restLeft[2] && restRight[1] < restLeft[1] + 0.02f && Dist(restRight, restLeft) > 0.10f && Dist(restRight, restLeft) < 0.50f, id,
          "%s: %zu frames: the palms rest on its frame for %d ms (>= 1000: the rig takes them) at right (%.3f %.3f %.3f), left (%.3f %.3f %.3f): %.0f cm apart, the right "
          "behind and not above the left",
          gun.name, frames.size(), restMs, restRight[0], restRight[1], restRight[2], restLeft[0], restLeft[1], restLeft[2], Dist(restRight, restLeft) * 100);
    // c: placed as the rig places the gloves, the gun somewhere else.
    std::snprintf(id, sizeof(id), "h8c%d", index);
    {
        std::mt19937 g(17 + index);
        float gunPose[12], gloves[kGloveBones * 12], placedGloves[kGloveBones * 12], bare[kBareBones * 12];
        Rigid(g, gunPose, 0.5f);
        barehold::ToGloves(rest.arms, gloves);
        for (int k = 0; k < kGloveBones; ++k) {
            float rel[12];
            Compose(frameInverse, gloves + k * 12, rel);  // (skin.cpp's Learn: the bone on the frame)
            Compose(gunPose, rel, placedGloves + k * 12);  // (its placed: the gun's pose times that)
        }
        std::memcpy(bare, rest.arms, sizeof(bare));
        const bool both[2] = {true, true};
        const float fold[2][3] = {};
        const barehold::Arm none[2];
        barehold::FromRig(placedGloves, both, fold, none, bare);
        // Every bare bone, hands and forearms: on the held gun as the game has it on its own.
        float worstHand = 0, worstArm = 0, move[12];
        Compose(gunPose, frameInverse, move);
        for (int k = 0; k < kBareBones; ++k) {
            float want[12];
            Compose(move, rest.arms + k * 12, want);
            (kBareGlove[k] >= 0 ? worstHand : worstArm) = std::max(kBareGlove[k] >= 0 ? worstHand : worstArm, Apart(bare + k * 12, want));
        }
        // The right palm on the held gun's frame, back from the palette as drawn.
        float gunInverse[12], at[3], drawnRight[3], drawnLeft[3];
        Invert(gunPose, gunInverse);
        Point(bare + 20 * 12, kRightPalm, at);
        Point(gunInverse, at, drawnRight);
        Point(bare + 41 * 12, kLeftPalm, at);
        Point(gunInverse, at, drawnLeft);
        // The two eyes' records of the rest frame: one hold.
        float eyes = 0;
        if (rest.armsOther && rest.bodyOther) {
            float right[3], left[3];
            if (palms(rest.armsOther, rest.bodyOther, right, left))
                eyes = std::max(Dist(right, restRight), Dist(left, restLeft));
        }
        Check(worstHand <= 2e-4f && worstArm <= 2e-4f && Dist(drawnRight, restRight) <= 0.0005f && Dist(drawnLeft, restLeft) <= 0.0005f && eyes <= 0.001f, id,
              "%s: placed as the rig places the gloves, the gun elsewhere: the bare hands' bones %.1e from the game's own hold carried onto it (<= 2e-4), the forearms whole "
              "with them %.1e; the right palm %.2f mm and the left %.2f mm from where the game rests them on the frame (<= 0.5); the two eyes' holds %.2f mm apart (<= 1)",
              gun.name, worstHand, worstArm, Dist(drawnRight, restRight) * 1000, Dist(drawnLeft, restLeft) * 1000, eyes * 1000);
        // The game's own left hand there (on the handguard), in the gloves' numbers: how open it is (for h7).
        if (index == 0) {
            float left[handpose::kBones][12];
            std::memcpy(left, gloves, sizeof(left));
            g_gameLeftOpen = FurthestTip(left);
        }
    }
}

void Fitted() {
    struct Table {
        const char* name;
        const float (*fingers)[12];
    } const tables[] = {{"the AKS-74u's magazine", magposes::kAks74uFingers},         {"the UMP-45's magazine", magposes::kUmp45Fingers},
                        {"the AKS-74u's lever, over", magposes::kAks74uLeverFingers}, {"the AKS-74u's lever, under", magposes::kAks74uLeverUnderFingers},
                        {"the UMP-45's lever", magposes::kUmp45LeverFingers}};
    std::mt19937 g(23);
    float game[kBareBones * 12];
    for (int k = 0; k < kBareBones; ++k)
        Rigid(g, game + k * 12, 0.3f);
    float worst = 0, mostOpen = 0;
    int same = 0;
    std::string tips;
    for (const Table& t : tables) {
        float leftPose[12], gloves[kGloveBones * 12], bare[kBareBones * 12];
        Rigid(g, leftPose, 0.4f);
        barehold::ToGloves(game, gloves);
        for (int b = 0; b < handpose::kBones; ++b)
            Compose(leftPose, t.fingers[b], gloves + b * 12);  // (skin.cpp: the left hand free, leftPose times leftFingers)
        std::memcpy(bare, game, sizeof(bare));
        const bool both[2] = {true, true};
        const float fold[2][3] = {};
        const barehold::Arm none[2];
        barehold::FromRig(gloves, both, fold, none, bare);
        for (int k = barehold::kBareSide[0][0]; k <= barehold::kBareSide[0][1]; ++k) {
            if (kBareGlove[k] < 0)
                continue;
            float want[12];
            Compose(leftPose, t.fingers[kBareGlove[k]], want);
            worst = std::max(worst, Apart(bare + k * 12, want));
            same += Apart(bare + k * 12, game + k * 12) == 0.0f;
        }
        const float open = FurthestTip(t.fingers);
        mostOpen = std::max(mostOpen, open);
        char one[96];
        std::snprintf(one, sizeof(one), " %s %.0f mm;", t.name, open * 1000);
        tips += one;
    }
    const bool haveGame = g_gameLeftOpen >= 0.0f;
    Check(worst == 0.0f && same == 0 && mostOpen < 0.2f, "h7",
          "the fitted hands on the bare left hand: each bone %.1e from the left pose times the table's (0 wanted), %d left as the game's own (0); the furthest fingertip from "
          "the palm:%s the game's own handguard hand on the train %s%.0f mm",
          worst, same, tips.c_str(), haveGame ? "" : "(no recording) ", haveGame ? g_gameLeftOpen * 1000 : 0.0f);
}

// h9: the tables on bare skin.
void OnSkin() {
    struct Table {
        const char* name;
        const float (*fingers)[12];
    } const tables[] = {
        {"an94", magposes::kAn94Fingers}, {"rpk", magposes::kRpkFingers}, {"aek971", magposes::kAek971Fingers}, {"ak74", magposes::kAk74Fingers},
        {"g3a3", magposes::kG3a3Fingers}, {"kh2002", magposes::kKh2002Fingers}, {"m16a4", magposes::kM16a4Fingers}, {"m4a1", magposes::kM4a1Fingers},
        {"m240", magposes::kM240Fingers}, {"aks74u", magposes::kAks74uFingers}, {"ump45", magposes::kUmp45Fingers},
        {"aek971 lever", magposes::kAek971LeverFingers}, {"ak74 lever", magposes::kAk74LeverFingers}, {"an94 lever", magposes::kAn94LeverFingers},
        {"g3a3 lever", magposes::kG3a3LeverFingers}, {"aks74u lever", magposes::kAks74uLeverFingers}, {"ump45 lever", magposes::kUmp45LeverFingers},
        {"m16a4 lever", magposes::kM16a4LeverFingers}, {"m4a1 lever", magposes::kM4a1LeverFingers}, {"kh2002 lever", magposes::kKh2002LeverFingers},
        {"an94 lever under", magposes::kAn94LeverUnderFingers}, {"aek971 lever under", magposes::kAek971LeverUnderFingers},
        {"ak74 lever under", magposes::kAk74LeverUnderFingers}, {"aks74u lever under", magposes::kAks74uLeverUnderFingers},
        {"rpk lever under", magposes::kRpkLeverUnderFingers}, {"rpk lever over", magposes::kRpkLeverOverFingers},
        {"m240 lever under", magposes::kM240LeverUnderFingers}, {"m240 lever over", magposes::kM240LeverOverFingers}, {"m240 lid", magposes::kM240LidFingers},
        {"m240 belt pinch", magposes::kBeltPinchFingers}, {"shell pinch", shellposes::kFingers}, {"launcher case", rocketposes::kFingers},
    };
    float worstBefore = 0, worstAfter = 0;
    int widened = 0, rigidChanged = 0, palmMoved = 0;
    std::string names;
    for (const Table& t : tables) {
        float before = 0, after = 0, skin[handpose::kBones][12];
        handpose::SkinTable(t.fingers, skin);
        for (int b = 0; b < handpose::kBones; ++b) {
            before = std::max(before, handpose::OffRigid(t.fingers[b]));
            after = std::max(after, handpose::OffRigid(skin[b]));
        }
        worstBefore = std::max(worstBefore, before), worstAfter = std::max(worstAfter, after);
        if (before > 1e-3f) {
            ++widened;
            names += names.empty() ? "" : ", ";
            names += t.name;
            // (The palm of a hand made by Pose is where the bind pose has it: on skin, nothing moves it.)
            const float same[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            palmMoved += Apart(skin[1], same) > 1e-4f;
        } else {
            rigidChanged += std::memcmp(skin, t.fingers, sizeof(skin)) != 0;
        }
    }
    Check(worstAfter <= 1e-3f && rigidChanged == 0 && palmMoved == 0 && widened > 0 && worstBefore > 0.1f, "h9a",
          "%d fitted hands' tables on bare skin: every bone's determinant within %.1e of 1 (<= 1e-3; as made, %.3f off at most); %d made on the widened glove (%s), their "
          "palms on skin %d off the bind pose's (0 wanted); of the %d rigid as made, %d changed (0 wanted)",
          static_cast<int>(std::size(tables)), worstAfter, worstBefore, widened, names.c_str(), palmMoved, static_cast<int>(std::size(tables)) - widened, rigidChanged);
    // The same hand made on skin. Points over the hand: each bone's own pivot and 3 cm about it.
    std::mt19937 g(57);
    std::uniform_real_distribution<float> u(0, 1);
    float worst = 0, worstMoved = 0, worstRigid = 0;
    for (int n = 0; n < 2000; ++n) {
        handpose::Fingers f;
        for (float& c : f.curl)
            c = u(g);
        f.thumb = u(g), f.spread = u(g);
        if (n % 3 == 0)
            f.thumbOut = 40 * u(g), f.thumbRoll = 40 * u(g), f.apart[0] = 10 * u(g), f.apart[1] = -10 * u(g);
        const float width = n % 5 == 0 ? 0.9f + 0.5f * u(g) : 1.15f;
        float glove[handpose::kBones][12], want[handpose::kBones][12], skin[handpose::kBones][12], moved[handpose::kBones][12], whole[12];
        handpose::Pose(f, width, glove);
        handpose::Fingers bare = f;
        bare.slim = 1.0f;
        handpose::Pose(bare, 1.0f, want);
        handpose::SkinTable(glove, skin);
        Rigid(g, whole, 0.4f);
        for (int b = 0; b < handpose::kBones; ++b)
            Compose(whole, glove[b], moved[b]);
        handpose::SkinTable(moved, moved);
        for (int b = 0; b < handpose::kBones; ++b) {
            float pivot[3], wantMoved[12];
            handpose::Pivot(b, pivot);
            Compose(whole, want[b], wantMoved);
            worstRigid = std::max({worstRigid, handpose::OffRigid(skin[b]), handpose::OffRigid(moved[b])});
            for (int k = 0; k < 4; ++k) {
                const float p[3] = {pivot[0] + (k == 1 ? 0.03f : 0.0f), pivot[1] + (k == 2 ? 0.03f : 0.0f), pivot[2] + (k == 3 ? 0.03f : 0.0f)};
                float a[3], c[3];
                Point(skin[b], p, a), Point(want[b], p, c);
                worst = std::max(worst, Dist(a, c));
                Point(moved[b], p, a), Point(wantMoved, p, c);
                worstMoved = std::max(worstMoved, Dist(a, c));
            }
        }
    }
    Check(worst <= 1e-4f && worstMoved <= 1e-4f && worstRigid <= 1e-3f, "h9b",
          "2000 hands posed on the glove (width 1.15, some 0.9 to 1.4; the fingers slimmed) with the widening taken out: %.3f mm from the same hand posed on skin (width 1, "
          "the fingers whole) at most, %.3f mm moved whole anywhere first (<= 0.1 each); determinants within %.1e of 1",
          worst * 1000, worstMoved * 1000, worstRigid);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build\\survey";
    std::printf("bareholdtest: src/barehold.h, the bare arms with a gun in hand as the gloves under another numbering. Recordings: %s\n", dir.c_str());
    Table();
    RoundTrip();
    ForearmsDraw();
    FromRigChecks();
    for (int i = 0; i < static_cast<int>(std::size(kGuns)); ++i)
        Recording(dir, kGuns[i], i);
    Fitted();
    OnSkin();
    std::printf("%d of %d checks failed%s\n", g_failed, g_checks, g_skipped ? " (some skipped: no recordings)" : "");
    return g_failed;
}

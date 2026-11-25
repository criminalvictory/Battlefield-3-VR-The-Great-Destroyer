// Free hands. Found 2026-10-30: BF3 draws the first-person arms and gun in a space of their own
// with the world's axes, whose origin sits on whole-metre world coordinates near the player (the
// eye was at (0.69, 1.68, 0.18) in it, and (1457.69, 70.68, -1564.82) in the world). The view-
// projection uploaded for those draws gives the eye's position and the view's axes in that space,
// so a controller's pose relative to the head lands there directly: this works out the poses
// relative to the head, and skin.cpp places them with each draw's own view-projection.
//
// The gun model's axes match the aim pose's (x right, y up, z back towards the shooter), so the
// gun takes the right controller's aim orientation, with its grip at the right controller's grip.
// The left glove's bind space has x to the left, y up, z forward, the palm down: it is turned onto
// the left grip pose (fingers along -Z, the back of the hand along +X) and placed there.
// build\grip.txt tunes the grip points and the left hand's turn while the game runs.

#include "hands.h"

#include <windows.h>

#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ammo.h"
#include "armik.h"
#include "barehold.h"
#include "bodyfit.h"
#include "collide.h"
#include "config.h"
#include "freeplaces.h"
#include "freetest.h"
#include "freetune.h"
#include "grasp.h"
#include "gunbody.h"
#include "gunbodies.h"
#include "gunprofile.h"
#include "gunshape.h"
#include "shapemake.h"
#include "handpose.h"
#include "holster.h"
#include "ground.h"
#include "input.h"
#include "loose.h"
#include "magsound.h"
#include "m320.h"
#include "m320carry.h"
#include "log.h"
#include "mountseat.h"
#include "pickrules.h"
#include "pickups.h"
#include "pistolmag.h"
#include "pistolposes.h"
#include "pistolslide.h"
#include "pouch.h"
#include "ammozone.h"
#include "recoilrun.h"
#include "recoilguns.h"
#include "reload.h"
#include "rocket.h"
#include "rocketposes.h"
#include "shellload.h"
#include "shellposes.h"
#include "skin.h"
#include "sounds.h"
#include "steer.h"
#include "usepoint.h"
#include "views.h"
#include "workhand.h"

namespace {

bool g_on = true;

// The gun's grip in its model space (behind the trigger, below the receiver: the mesh dump puts
// the trigger at y -0.07, z 0.165), and the left glove's palm in its bind space (the centre of
// its root bone's vertices).
float g_gunGrip[3] = {0.0f, -0.11f, 0.21f};
bool g_gunGripTuned = false;  // grip.txt set it; otherwise skin::GunGrip, the game's own hold
float g_leftAnchor[3] = {0.502f, 1.146f, 0.144f};
float g_leftTurn[3] = {0.0f, 0.0f, 0.0f};  // degrees about the grip's x, y, z, after the default
// The left glove's placement on the left grip pose: the gloves' bind space -> the grip's frame
// (rows: rotation | translation). The View button's line-up sets it (saved as grip.txt
// "leftpose"); built in is the user's own on the Steam Frame (2026-10-30, the most reliable one),
// so a clean build keeps it. grip.txt "leftturn" without a "leftpose" makes it from the default
// turn and palm instead.
float g_leftOffset[12] = {-0.65114f, -0.68371f, -0.32949f, 1.15898f,  -0.54118f, 0.72265f,
                          -0.43004f, -0.39106f, 0.53211f,  -0.10169f, -0.84054f, 0.02741f};
bool g_leftOffsetSet = true;
// The line-up: the first View press holds the glove still, the hand is put into it, the second
// press keeps where the hand is relative to the glove.
bool g_lining = false;
float g_heldLeft[12] = {};
// The left glove's fingers, smoothed: the index finger (the trigger), the other three (the grip)
// and the thumb (on the stick). By hand ([0] the left, under its old names; [1] the free right glove's, free hands).
float g_indexBy[2] = {}, g_gripBy[2] = {}, g_thumbBy[2] = {};
float& g_index = g_indexBy[0];
float& g_grip = g_gripBy[0];
float& g_thumb = g_thumbBy[0];
// How far apart the relaxed fingers are (0-1: the user wanted space between them) and how much
// wider the palm is drawn (it looked skinny; the whole glove 10% bigger looked huge, the whole hand
// widened alike thick): grip.txt "spread 0.7", "leftwidth 1.15".
float g_spread = 0.7f;
float g_leftWidth = 1.15f;
// grip.txt "fingers i g t": the index finger, the other three and the thumb held so (0-1 each)
// whatever the controller says, to check the poses in the game.
bool g_fingersFixed = false;
float g_fixedFingers[3] = {};
// Magazines from over the left shoulder (pouch.cpp, ammozone.h), tuned in build\pouch.txt (not grip.txt: that's rewritten whole
// when the glove is lined up): "test 1" (pictures with nobody in the headset: see Update), "anygun 0"
// (other guns don't reload from it), "xinsert 1" (X with the magazine out puts one in, off by the user's word 2026-11-05).
// "lineup 1": the left controller's View button lines the glove up with the hand. Off (the user, 2026-11-04: pressed by accident some ten
// times, a beep, and the hand had to be lined up all over again).
bool g_lineUpOn = false;
int g_pouchTest = 0;
float g_pouchTestAlong = -0.05f;
bool g_anyGunReload = true;
ULONGLONG g_seatedAt = 0;  // when a magazine from the pouch last seated: the push isn't a bolt slap
// The glove's fingers round a magazine: 0-1. By hand ([0] the left, under its old name; [1] the free right glove's, free hands).
float g_magFingersBy[2] = {};
float& g_magFingers = g_magFingersBy[0];
// A magazine its hand couldn't keep (pouch.cpp's dropHeld: the rifle left the hands) let go of with that hand still squeezed: the hand's own fingers drawn open,
// as a let-go's, from that frame (the hold still has it at 0.65 then: no jump), while the squeeze that held it stays held and, once let go of, till the
// controller's own fingers are open too or it squeezes again; then eased back to the controller's. Eased from the hold straight into the squeezed hand they
// closed through it (film F9, 4.56); closed again after a set time, through one a stopping hand had sent on ahead. By hand: 1 that squeeze still held, 2 let
// go of (0 the controller's own); how far open (0-1).
int g_dropOpenState[2] = {};
float g_dropOpenBy[2] = {};
// Round the magazine, option A (pouch.cpp g_hold: fitted at the palm width 1.15, Update): the back edge in the palm, the thumb flat along the far face. The
// Fingers are either glove's (the right's posed as the left's, then flipped), so the right's hold is the left's mirrored.
constexpr float kOnMag[4] = {0.306f, 0.281f, 0.219f, 0.156f}, kOnMagThumb = 0.905f, kOnMagSpread = 0.314f;
constexpr float kOnMagThumbOut = 45.63f, kOnMagThumbRoll = 52.72f;  // degrees

// A glove's fingers blended to that hold by share (0-1).
void FingersOnMagazine(handpose::Fingers& fingers, float share) {
    for (int i = 0; i < 4; ++i)
        fingers.curl[i] += (kOnMag[i] - fingers.curl[i]) * share;
    fingers.thumb += (kOnMagThumb - fingers.thumb) * share;
    fingers.spread += (kOnMagSpread - fingers.spread) * share;
    fingers.thumbOut += (kOnMagThumbOut - fingers.thumbOut) * share;
    fingers.thumbRoll += (kOnMagThumbRoll - fingers.thumbRoll) * share;
}

// A glove's own fingers drawn open by share (0-1): the curls and the folded thumb towards relaxed (a magazine dropped by force, above).
void FingersOpened(handpose::Fingers& fingers, float share) {
    for (int i = 0; i < 4; ++i)
        fingers.curl[i] *= 1.0f - share;
    fingers.thumb *= 1.0f - share;
}

// Row-major 3x3 from a quaternion (x, y, z, w).
void QuatToMatrix(const XrQuaternionf& q, float* m) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    m[0] = 1 - 2 * (y * y + z * z), m[1] = 2 * (x * y - z * w), m[2] = 2 * (x * z + y * w);
    m[3] = 2 * (x * y + z * w), m[4] = 1 - 2 * (x * x + z * z), m[5] = 2 * (y * z - x * w);
    m[6] = 2 * (x * z - y * w), m[7] = 2 * (y * z + x * w), m[8] = 1 - 2 * (x * x + y * y);
}

void Mul3(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
}

using gunprofile::SightsTurn;  // (a gun's raise by its sights' up, and the AT4's turn by their right)

void Transpose3(const float* a, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = a[j * 3 + i];
}

// A turn about x, then y, then z, in degrees.
void Euler(const float* degrees, float* m) {
    constexpr float kRad = 3.14159265f / 180.0f;
    float cx = std::cos(degrees[0] * kRad), sx = std::sin(degrees[0] * kRad);
    float cy = std::cos(degrees[1] * kRad), sy = std::sin(degrees[1] * kRad);
    float cz = std::cos(degrees[2] * kRad), sz = std::sin(degrees[2] * kRad);
    float rx[9] = {1, 0, 0, 0, cx, -sx, 0, sx, cx}, ry[9] = {cy, 0, sy, 0, 1, 0, -sy, 0, cy},
          rz[9] = {cz, -sz, 0, sz, cz, 0, 0, 0, 1}, t[9];
    Mul3(ry, rx, t);
    Mul3(rz, t, m);
}

// Affine 3x4 matrices (three rows: rotation | translation).
void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

// The inverse of a rotation | translation (no scale).
void InvertRigid(const float* m, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = -(out[i * 4] * m[3] + out[i * 4 + 1] * m[7] + out[i * 4 + 2] * m[11]);
}

// Any 3x4's inverse (a glove's palm is widened across: not rigid). False if it has none.
bool Invert(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    const float det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
    if (!(std::fabs(det) > 1e-8f))
        return false;
    const float r[9] = {(e * k - f * h) / det, (c * h - b * k) / det, (b * f - c * e) / det, (f * g - d * k) / det, (a * k - c * g) / det,
                        (c * d - a * f) / det, (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int i = 0; i < 3; ++i) {
        out[i * 4] = r[i * 3], out[i * 4 + 1] = r[i * 3 + 1], out[i * 4 + 2] = r[i * 3 + 2];
        out[i * 4 + 3] = -(r[i * 3] * m[3] + r[i * 3 + 1] * m[7] + r[i * 3 + 2] * m[11]);
    }
    return true;
}

void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

// The first step this frame that left the left glove with a number not finite, and what (-1 its place, 0-22 that finger bone): the guard before the arms logs it
// (the user's test, 2026-11-05 22:20: "came out with a number not finite" while holding the launcher's round, the step not known).
const char* g_badStage = nullptr;
int g_badBone = -2;
float g_goodLeft[12] = {};  // the left glove as last drawn with every number finite
bool g_haveGoodLeft = false;
void NoteBad(const skin::HandTargets& t, const char* stage) {
    if (g_badStage || !t.left)
        return;
    for (int i = 0; i < 12; ++i)
        if (!std::isfinite(t.leftPose[i])) {
            g_badStage = stage, g_badBone = -1;
            return;
        }
    for (int b = 0; b < handpose::kBones; ++b)
        for (int i = 0; i < 12; ++i)
            if (!std::isfinite(t.leftFingers[b][i])) {
                g_badStage = stage, g_badBone = b;
                return;
            }
}

// A pose matrix (three rows: rotation | translation) putting the point anchor at position.
void PlaceAt(const float* rotation, const float* anchor, const float* position, float* out) {
    for (int i = 0; i < 3; ++i) {
        const float* r = rotation + i * 3;
        out[i * 4] = r[0], out[i * 4 + 1] = r[1], out[i * 4 + 2] = r[2];
        out[i * 4 + 3] = position[i] - (r[0] * anchor[0] + r[1] * anchor[1] + r[2] * anchor[2]);
    }
}

// build\grip.txt: "gun x y z" (the gun's grip in its model space), "left x y z" (the left palm in
// the gloves' bind space), "leftturn x y z" (degrees), "on" / "off".
void ReadTuning() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadTuning), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"grip.txt");
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info) ||
        CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    lastWrite = info.ftLastWriteTime;
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"r") != 0 || !f)
        return;
    char line[512];
    g_fingersFixed = false;
    bool pose = false, turn = false;
    while (fgets(line, sizeof(line), f)) {
        float v[12];
        if (std::strncmp(line, "fingers", 7) == 0 && sscanf_s(line + 7, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
            std::memcpy(g_fixedFingers, v, sizeof(g_fixedFingers)), g_fingersFixed = true;
        else if (std::strncmp(line, "gun", 3) == 0 && sscanf_s(line + 3, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
            std::memcpy(g_gunGrip, v, 3 * sizeof(float)), g_gunGripTuned = true;
        else if (std::strncmp(line, "leftpose", 8) == 0 &&
                 sscanf_s(line + 8, "%f %f %f %f %f %f %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5],
                          &v[6], &v[7], &v[8], &v[9], &v[10], &v[11]) == 12)
            std::memcpy(g_leftOffset, v, sizeof(g_leftOffset)), g_leftOffsetSet = pose = true;
        else if (std::strncmp(line, "spread", 6) == 0 && sscanf_s(line + 6, "%f", &v[0]) == 1)
            g_spread = std::fmin(std::fmax(v[0], 0.0f), 1.0f);
        else if (std::strncmp(line, "leftwidth", 9) == 0 && sscanf_s(line + 9, "%f", &v[0]) == 1)
            g_leftWidth = std::fmin(std::fmax(v[0], 0.8f), 1.5f);
        else if (std::strncmp(line, "leftturn", 8) == 0 && sscanf_s(line + 8, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
            std::memcpy(g_leftTurn, v, 3 * sizeof(float)), turn = true;
        else if (std::strncmp(line, "left", 4) == 0 && sscanf_s(line + 4, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
            std::memcpy(g_leftAnchor, v, 3 * sizeof(float));
    }
    fclose(f);
    if (turn && !pose)
        g_leftOffsetSet = false;  // made from the turn and palm (Update)
    Log("Hands: gun grip (%.3f %.3f %.3f), left palm (%.3f %.3f %.3f), left turn (%.0f %.0f %.0f), fingers apart "
        "%.2f, palm %.2f wide",
        g_gunGrip[0], g_gunGrip[1], g_gunGrip[2], g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2], g_leftTurn[0],
        g_leftTurn[1], g_leftTurn[2], g_spread, g_leftWidth);
}

// build\pouch.txt, re-read when it changes (pouch.cpp): "zone below forward reach" (metres: where a
// magazine is taken, from the neck: ammozone.h's unless given), "pouch off", "hold" and 12
// numbers (the magazine in the glove), "test 1" / "test 2 -0.05" (pictures with nobody in the
// headset), "anygun 0" (other guns don't reload from it), "xinsert 1" (X puts a magazine in),
// "lineup 1" (the left View button lines the glove up).
void ReadPouchTuning() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadPouchTuning), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"pouch.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"r") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next check
    lastWrite = info.ftLastWriteTime;
    int test = 0, anyGun = 1, tapInsert = 0, lineUp = 0;
    float along = -0.05f;
    bool on = true;
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            float v[12];
            if (std::strncmp(line, "zone", 4) == 0 && sscanf_s(line + 4, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
                pouch::SetZone(v, true);
            else if (std::strncmp(line, "lineup", 6) == 0)
                sscanf_s(line + 6, "%d", &lineUp);
            else if (std::strncmp(line, "pouch off", 9) == 0)
                on = false;
            else if (std::strncmp(line, "hold", 4) == 0 &&
                     sscanf_s(line + 4, "%f %f %f %f %f %f %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4],
                              &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11]) == 12)
                pouch::SetHold(v);
            else if (std::strncmp(line, "test", 4) == 0)
                sscanf_s(line + 4, "%d %f", &test, &along);
            else if (std::strncmp(line, "anygun", 6) == 0)
                sscanf_s(line + 6, "%d", &anyGun);
            else if (std::strncmp(line, "xinsert", 7) == 0)
                sscanf_s(line + 7, "%d", &tapInsert);
        }
        fclose(f);
    }
    pouch::SetZone(nullptr, on);
    g_lineUpOn = lineUp != 0;
    if (!g_lineUpOn)
        g_lining = false;
    g_pouchTest = test;
    g_pouchTestAlong = along;
    g_anyGunReload = anyGun != 0;
    input::SetTapInsert(tapInsert != 0);
    Log("Pouch: %s; other guns %s from it; X %s a magazine with it out%s", on ? "on" : "off",
        g_anyGunReload ? "reload" : "don't reload", tapInsert ? "puts" : "doesn't put",
        test ? " (a test's hand)" : "");
}

// Holsters (holster.h, build\holsters.txt): the settings, and the state kept outside the grasp's Clear (each controller's raw record, a draw waiting
// for the game's switch or for a hand not seen, the launchers' order). For input.cpp (one frame old, as every flag here): the gun put away, and its
// words ("the M416 at the right shoulder").
holster::Settings g_holsterSet;
holster::State g_holster;
std::atomic<bool> g_stowedNow{false};
char g_stowedWhere[96] = "";
// The slots a take off the ground changed since holster.h last heard (NoteSwaps: a hand's take or the game's own R): a gun so taken goes on a shoulder
// only by a hand (the user, 2026-11-18). Told to Arrive and Update (Frame::taken), then cleared.
unsigned g_holsterTaken = 0;

// build\holsters.txt, read whole when it changes (as pouch.txt is): holster.h's words, "holsters 0" today's in everything; deleted, every default
// (holsters on). Said each time it's read, with every line refused and why (that word keeps the value it had).
void ReadHolsterTuning() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadHolsterTuning), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"holsters.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"rb") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next check
    lastWrite = info.ftLastWriteTime;
    static char text[16384];
    size_t length = 0;
    const bool have = f != nullptr;
    if (f) {
        length = fread(text, 1, sizeof(text), f);
        fclose(f);
    }
    holster::ParseReport report;
    g_holsterSet = holster::Parse(have ? text : nullptr, length, &report);
    const holster::Settings& s = g_holsterSet;
    const char* launcher = s.launcher == holster::Launcher::kShoulder ? "the secondary's shoulder (behind the neck when a gun is at that shoulder)"
                           : s.launcher == holster::Launcher::kNeck  ? "behind the neck"
                           : s.launcher == holster::Launcher::kRib   ? "the rib cage"
                                                                     : "none";
    if (have) {
        Log("Holster: holsters.txt read (%d lines, %d words, %d refused): holsters %s, launchers' place %s, a hand not seen %s, it drops %s, buzz %.2f, "
            "log %d",
            report.lines, report.read, report.refused, s.on ? "on" : "off (today's in everything)", launcher, s.unseen ? "judged by where it went" : "never judged",
            s.unseenDrop ? "within the 0.5 s hold, as before" : "nothing", s.buzz, s.log ? 1 : 0);
        if (s.otherGunStow || s.lyingRecall)
            Log("Holster: holsters.txt's %s%s%s answers nothing now: a gun goes into a holster only by a hand (a draw while the other hand holds the gun, "
                "or while it lies, is refused)", s.otherGunStow ? "\"othergun stow\"" : "", s.otherGunStow && s.lyingRecall ? " and " : "",
                s.lyingRecall ? "\"lying recall\"" : "");
    } else {
        Log("Holster: no holsters.txt: every default (holsters on, launchers' place the secondary's shoulder (behind the neck when a gun is at that "
            "shoulder), a hand not seen drops nothing; a gun goes into a holster only by a hand)");
    }
    for (int k = 0; k < report.refused && k < 4; ++k)
        Log("Holster: holsters.txt %s (refused: that word as it was)", report.why[k]);
    // The places in effect (2026-11-19: the shoulders a lot bigger, a let-go at one never a throw), so a log says what a let-go and a squeeze were judged by.
    const auto cm = [](float m) { return m * 100.0f; };
    Log("Holster: holsters.txt's places (cm from the neck point: right, up, forward; then across, high, deep): the right shoulder %.0f %.0f %.0f, %.0f x %.0f x %.0f "
        "(the left its mirror; none in the head's %.0f x %.0f x %.0f round %.0f %.0f %.0f), the back of the neck %.0f %.0f %.0f, %.0f x %.0f x %.0f; a "
        "put-away %.0f cm more, %.0f more for %.2f s after it could, or swung in from %.0f short at %.1f m/s; a throw only outside them, going away at "
        "%.1f m/s; a draw at any speed",
        cm(s.shoulder.mid[0]), cm(s.shoulder.mid[1]), cm(s.shoulder.mid[2]), cm(2 * s.shoulder.half[0]), cm(2 * s.shoulder.half[1]),
        cm(2 * s.shoulder.half[2]), cm(2 * s.head.half[0]), cm(2 * s.head.half[1]), cm(2 * s.head.half[2]), cm(s.head.mid[0]), cm(s.head.mid[1]),
        cm(s.head.mid[2]), cm(s.neck.mid[0]), cm(s.neck.mid[1]), cm(s.neck.mid[2]), cm(2 * s.neck.half[0]), cm(2 * s.neck.half[1]), cm(2 * s.neck.half[2]),
        cm(s.stowMore), cm(s.graceMore), s.stowGrace, cm(s.swingReach), s.swingSpeed, s.stowSpeed);
}

// Wrist movement, on every level and every weapon (the user, 2026-11-04: "that's the foundation"; judged on The Great Destroyer's bare arms: "this is like proper arm
// tracking behavior"). Each forearm solved from its hand and a shoulder estimated from the headset (armik.h), so the hand bends at the wrist while the forearm keeps
// pointing at the elbow. Always on: build\arms.txt "off" draws each forearm whole with its hand, for comparing.
bool g_armsOn = true;
bool g_armsLog = false;  // "log 1": an Arms line once a second
float g_shoulder[3] = {0.164f, 0.16f, 0.02f};  // the shoulder out, down and back from the neck point (metres)
armik::Params ArmDefaults(bool right) {
    armik::Params p;  // the left's
    if (right)
        p.poleOut = 0.9f;  // fitted to the game's right hold
    // As the bare arms have them, the user's verdict: the wrist's range wide (past it the solver turns the forearm to make up the rest: "when I'm bending my wrist, my whole
    // forearm is moving", 2026-10-31), the forearm taking nearly all of the hand's roll (its wrist pinched thin at a third less).
    p.flex = 100.0f, p.ext = 95.0f, p.radial = 45.0f, p.ulnar = 60.0f;
    p.share[0] = 0.5f, p.share[1] = 0.8f, p.share[2] = 1.0f;
    return p;
}
armik::Params g_armParams[2] = {ArmDefaults(false), ArmDefaults(true)};  // left, right// The stand-in's tests (its left hand only, nobody in the headset): moved (metres, in the view's
// space), turned so its straight wrist lies along the solved forearm, bent about its wrist joint
// (degrees: flexion, ulnar deviation, twist; after neutral), or no left hand at all.
struct ArmTests {
    bool move = false, neutral = false, bend = false, drop = false;
    bool guard = false;  // "test guard": with nobody in the headset, the left hand holding the handguard or the launcher's front grip; the stand-in's left hand is the real hand
    bool m320 = false;   // "test m320": that hold on the grenade launcher's grip instead (and the launcher put in hand)
    float moveBy[3] = {}, bendBy[3] = {};
};
ArmTests g_armTests;

// A line's first word (after any spaces) is word, ending there or at a space; rest: what follows.
bool Word(const char* line, const char* word, const char** rest = nullptr) {
    while (*line == ' ' || *line == '\t')
        ++line;
    const size_t n = std::strlen(word);
    if (std::strncmp(line, word, n) != 0 || (line[n] && !std::isspace(static_cast<unsigned char>(line[n]))))
        return false;
    if (rest)
        *rest = line + n;
    return true;
}

// build\arms.txt, re-read when it changes: "off" (the forearms whole with the hands); "shoulder 0.164 0.16 0.02" (out,
// down and back from the neck point); "pole L 0.4 1 0.3" / "pole R 0.9 1 0.3" (the elbow's way: out,
// down, back); "rise 0.6 0.8" (a rising hand takes it out and up); "limits L 70 65 25 40" / "limits R
// 70 65 25 55" (the wrist's flexion, extension, radial and ulnar range, degrees); "twist 0.333 0.667"
// (twist bones A's and B's shares of the hand's twist); "log 1" (an Arms line once a second); with
// the stand-in's left hand only, "test move dx dy dz", "test neutral", "test bend flex dev twist",
// "test drop", "test guard".
void ReadArmsTuning() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadArmsTuning), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"arms.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"r") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next check
    lastWrite = info.ftLastWriteTime;
    bool off = false, log = false;
    float shoulder[3] = {0.164f, 0.16f, 0.02f};
    armik::Params params[2] = {ArmDefaults(false), ArmDefaults(true)};
    ArmTests tests;
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            float v[4];
            char sideName = 0;
            const char* rest = nullptr;
            auto side = [&] { return sideName == 'L' || sideName == 'l' ? 0 : sideName == 'R' || sideName == 'r' ? 1 : -1; };
            if (Word(line, "off")) {
                off = true;
            } else if (Word(line, "shoulder", &rest) && sscanf_s(rest, "%f %f %f", &v[0], &v[1], &v[2]) == 3) {
                std::memcpy(shoulder, v, sizeof(shoulder));
            } else if (Word(line, "pole", &rest) &&
                       sscanf_s(rest, " %c %f %f %f", &sideName, 1u, &v[0], &v[1], &v[2]) == 4 && side() >= 0) {
                armik::Params& p = params[side()];
                p.poleOut = v[0], p.poleDown = v[1], p.poleBack = v[2];
            } else if (Word(line, "rise", &rest) && sscanf_s(rest, "%f %f", &v[0], &v[1]) == 2) {
                for (armik::Params& p : params)
                    p.riseOut = v[0], p.riseDown = v[1];
            } else if (Word(line, "limits", &rest) &&
                       sscanf_s(rest, " %c %f %f %f %f", &sideName, 1u, &v[0], &v[1], &v[2], &v[3]) == 5 && side() >= 0) {
                for (float& d : v)
                    d = std::fmin(std::fmax(d, 1.0f), 179.0f);  // (none at 0: the range's ellipse divides by them)
                armik::Params& p = params[side()];
                p.flex = v[0], p.ext = v[1], p.radial = v[2], p.ulnar = v[3];
            } else if (Word(line, "twist", &rest) && sscanf_s(rest, "%f %f", &v[0], &v[1]) == 2) {
                for (armik::Params& p : params)
                    p.share[1] = std::fmin(std::fmax(v[0], 0.0f), 1.0f), p.share[2] = std::fmin(std::fmax(v[1], 0.0f), 1.0f);
            } else if (Word(line, "log", &rest)) {
                int n = 1;
                sscanf_s(rest, "%d", &n);
                log = n != 0;
            } else if (Word(line, "test", &rest)) {
                const char* what = nullptr;
                if (Word(rest, "move", &what) && sscanf_s(what, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
                    tests.move = true, std::memcpy(tests.moveBy, v, sizeof(tests.moveBy));
                else if (Word(rest, "neutral"))
                    tests.neutral = true;
                else if (Word(rest, "bend", &what) && sscanf_s(what, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
                    tests.bend = true, std::memcpy(tests.bendBy, v, sizeof(tests.bendBy));
                else if (Word(rest, "drop"))
                    tests.drop = true;
                else if (Word(rest, "guard"))
                    tests.guard = true;
                else if (Word(rest, "m320"))
                    tests.guard = tests.m320 = true;
            }
        }
        fclose(f);
    }
    g_armsOn = !off;
    g_armsLog = g_armsOn && log;
    g_armTests = g_armsOn ? tests : ArmTests{};
    std::memcpy(g_shoulder, shoulder, sizeof(g_shoulder));
    g_armParams[0] = params[0], g_armParams[1] = params[1];
    if (!g_armsOn) {
        Log("Arms: off, the forearms move whole with the hands%s", exists ? " (build\\arms.txt)" : "");
        return;
    }
    const armik::Params& p = g_armParams[0];
    const ArmTests& t = g_armTests;
    char testing[160] = "";
    if (t.move || t.neutral || t.bend || t.drop)
        std::snprintf(testing, sizeof(testing), "; the stand-in's left hand%s%s%s%s",
                      t.drop ? " dropped" : "", t.move ? " moved" : "", t.neutral ? " neutral" : "",
                      t.bend ? " bent" : "");
    Log("Arms: both forearms solved from their hands (build\\arms.txt): the shoulder %.3f out, %.3f down, %.3f back "
        "from the neck; the elbow's way %.2f out, %.2f down, %.2f back (rising %.2f %.2f); the wrist's range %.0f "
        "%.0f %.0f %.0f; twist shares %.3f %.3f%s%s",
        g_shoulder[0], g_shoulder[1], g_shoulder[2], p.poleOut, p.poleDown, p.poleBack, p.riseOut, p.riseDown, p.flex,
        p.ext, p.radial, p.ulnar, p.share[1], p.share[2], g_armsLog ? "; logged once a second" : "", testing);
}

// The free left glove can't go into the gun (the user: the hand went through the gun when it
// slapped the bolt release): collide.cpp keeps it out, from frame to frame. The glove is drawn
// there; the reloads still go by the hand. By hand ([0] the left under its old name; [1] the free
// right glove's, free hands: KeepOutOfGunRight).
collide::State g_collideBy[2];
collide::State& g_collide = g_collideBy[0];

// Keeps the glove out of the gun (collide::KeepOut) and moves it there, its wrist with it. touch
// (the gun's model space) gets where it touches the gun's surface, if it does. True when it touches.
bool KeepOutOfGun(skin::HandTargets& t, const float* gunPose, const handpose::Fingers& fingers, float* touch) {
    float bones[handpose::kBones][12], inverse[12], toGun[12], shift[3];
    handpose::Pose(fingers, g_leftWidth, bones);
    InvertRigid(gunPose, inverse);
    Compose(inverse, t.leftPose, toGun);
    const bool touched = collide::KeepOut(bones, toGun, g_collide, shift, touch);
    if (shift[0] || shift[1] || shift[2]) {
        // Into the view's space with the gun's turn, onto the glove and its wrist.
        for (int r = 0; r < 3; ++r) {
            const float v = gunPose[r * 4] * shift[0] + gunPose[r * 4 + 1] * shift[1] + gunPose[r * 4 + 2] * shift[2];
            t.leftPose[r * 4 + 3] += v;
            t.leftWrist[r] += v;
        }
    }
    return touched;
}

// The free right glove kept out alike (free hands: the right at work while the left holds the rifle). collide.cpp's spheres sit on the left
// glove's bones, so the right glove is given as its mirror image: its fingers posed in the left's layout (the right's own exactly, mirrored,
// for every bone a sphere is on) and its place times S = diag(-1, 1, 1) (the right glove seen in a mirror). KeepOut only moves points by that
// place, so its spheres, its shift and its touch are in the rifle's own space. glove: the free right glove (the gloves' bind space into the
// view's); shiftView gets the move to draw it with (the view's space); touch where it touches the gun (its model space). True when it touches.
bool KeepOutOfGunRight(const float* gunPose, const float* glove, const handpose::Fingers& fingers, float width, float* shiftView, float* touch) {
    float bones[handpose::kBones][12], inverse[12], mirrored[12], toGun[12], shift[3];
    handpose::PoseSafe(fingers, width, bones);
    std::memcpy(mirrored, glove, sizeof(mirrored));
    mirrored[0] = -mirrored[0], mirrored[4] = -mirrored[4], mirrored[8] = -mirrored[8];
    InvertRigid(gunPose, inverse);
    Compose(inverse, mirrored, toGun);
    const bool touched = collide::KeepOut(bones, toGun, g_collideBy[1], shift, touch);
    for (int r = 0; r < 3; ++r) {
        const float v = gunPose[r * 4] * shift[0] + gunPose[r * 4 + 1] * shift[1] + gunPose[r * 4 + 2] * shift[2];
        shiftView[r] = std::isfinite(v) ? v : 0.0f;
    }
    return touched;
}

// The left hand on the handguard (the user: time the left hand works as a grip for the gun): the
// left grip squeezed with the palm near the handguard takes hold. The gun then points from the
// right hand towards the left, by the smallest turn from where the right hand alone points it
// (the right hand keeps the roll), and the glove stays on the handguard where it took hold. Only
// while the hands' line is near where the right hand points the gun, eased in and out (steer.h: the
// rifle had followed a left hand beside, below or behind the right, the user's video 2026-10-31). The
// game aims and fires along it too (AimOrientation). It holds until the grip is let go: it had
// let go whenever the hand strayed from where the right hand alone points the gun, which any
// sway did, and it took hold only within 13 cm, where the user's wrist sits on the handguard
// (100 to 128 mm in the log, 2026-10-30). Holding, the glove is the game's own left hand as its
// animation holds this gun (skin::GameLeftGrip: made for it, launcher and all, so it doesn't clip),
// on the free gun, its fingers moving a little with the controller's (BendWithController). A grip
// fitted to the gun's collision shape went through the grenade launcher, which isn't in the shape;
// the user wanted only the game's (2026-10-30).
// The handguard's middle line in the gun's model space: round the bore, its cross-section 57 mm
// across and 73 mm high from z 0.46 to 0.68 (tools/gripfit.cpp, from the gun's mesh dump).
constexpr float kGuardFrom[3] = {0.0f, 0.045f, 0.46f}, kGuardTo[3] = {0.0f, 0.045f, 0.68f};
constexpr float kGuardReach = 0.25f;   // the palm (the wrist's fold) within this of it takes hold
constexpr float kHandsTogether = 0.10f;  // the left hand back by the right: off to the receiver
// Steering by the body, the left hand keeps pointing the gun with the hands this close (fully from 3 + 5
// cm): swung to the right the hands came 9-12 cm apart, and below 10 cm the steer faded to the right
// controller's own aim, 55-68 degrees off the hands' line and up (the user, 2026-10-31 12:01).
constexpr float kHandsTogetherBody = 0.03f;
// Each of these by hand, [0] the left under its old name, [1] the right (free hands: the right on the handguard while the left
// holds the rifle's pistol grip).
bool g_supportingBy[2] = {};
bool& g_supporting = g_supportingBy[0];
// The hold lasts until the grip's let go, but an arm has a length: with the hold's place further from the shoulder than an arm goes and the real hands far apart (the right hand far
// out to one side, the left hand left behind: 1.08 m of a 0.56 m arm, the sleeve stretched across the view, the user's "Mr. Elastic", 2026-11-04; 83-88 cm apart), the glove comes off to where
// the hand really is, the gun to the right hand's aim, and goes back on by itself once it's in reach again. The grip's still held all along. In ordinary aiming the hands are 7-30 cm apart, up
// to 55 pulling the left hand right back (the user's video): that never comes off. The real hand against the hold's place instead, ordinary swings came off (19 times in a run: with the steer
// faded out, that place is far from any hand). (metres, from the square chest's shoulder)
bool g_holdOutOfReachBy[2] = {};
bool& g_holdOutOfReach = g_holdOutOfReachBy[0];
// The hold is the grenade launcher's own grip, not the handguard (M320Held; the second hold, Support). By hand ([0] the left under its old name; [1] the
// right, free hands: on the launcher's grip while the left holds the rifle's pistol grip).
bool g_onM320By[2] = {};
bool& g_onM320 = g_onM320By[0];
constexpr float kHoldOutOfReach = 0.80f, kHoldInReach = 0.74f;
constexpr float kHoldHandAway = 0.62f, kHoldHandBack = 0.52f;  // the real hands this far apart to come off; this near comes back on whatever the reach
bool g_freshBy[2] = {true, true};  // the grip squeezed since the hand last let go (or never let go)
bool& g_freshSqueeze = g_freshBy[0];
float g_supportPointBy[2][3] = {};   // the palm when it took hold, in the gun's model space
float (&g_supportPoint)[3] = g_supportPointBy[0];
float g_gripBlend = 0.0f;  // 0 the glove where the hand is, 1 on the handguard
constexpr float kGripBlendStep = 0.2f;  // a frame: on or off the handguard in 5 frames (70 ms)
float g_fingerBendBy[2][5] = {};  // the controller's extra bend of the held fingers, degrees (smoothed)
float (&g_fingerBend)[5] = g_fingerBendBy[0];
bool g_aimTurned = false;
XrQuaternionf g_aimTurn{0, 0, 0, 1};  // the right controller's aim, turned to point the gun
// How much the left hand points the gun, eased (steer.h), and this frame's lines for it (Support);
// the cone it steers in (weapon.txt "steer 35 65", radians).
steer::State g_steer;
steer::Input g_steerIn;
std::atomic<float> g_steerFull{steer::kFullAngle}, g_steerNone{steer::kNoneAngle};
std::atomic<bool> g_steerByBody{true};  // the left hand on the gun judged by the body (Support); else the cone
// When the hold was taken and the nearest the palm has come to the handguard's line since (the hold
// point follows it in for kHoldSettle), and when the left controller was last seen holding (the steer
// coasts through a loss of up to kCoast).
ULONGLONG g_tookHoldAtBy[2] = {}, g_seenAtBy[2] = {};
ULONGLONG& g_tookHoldAt = g_tookHoldAtBy[0];
ULONGLONG& g_leftSeenAt = g_seenAtBy[0];
float g_nearestHoldBy[2] = {};
float& g_nearestHold = g_nearestHoldBy[0];
constexpr ULONGLONG kHoldSettle = 250, kCoast = 200;  // ms

float Length(const float* v) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

XrQuaternionf MatrixToQuat(const float* m) {
    XrQuaternionf q;
    const float trace = m[0] + m[4] + m[8];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {(m[7] - m[5]) / s, (m[2] - m[6]) / s, (m[3] - m[1]) / s, 0.25f * s};
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const float s = std::sqrt(1.0f + m[0] - m[4] - m[8]) * 2.0f;
        q = {0.25f * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s, (m[7] - m[5]) / s};
    } else if (m[4] > m[8]) {
        const float s = std::sqrt(1.0f + m[4] - m[0] - m[8]) * 2.0f;
        q = {(m[1] + m[3]) / s, 0.25f * s, (m[5] + m[7]) / s, (m[2] - m[6]) / s};
    } else {
        const float s = std::sqrt(1.0f + m[8] - m[0] - m[4]) * 2.0f;
        q = {(m[2] + m[6]) / s, (m[5] + m[7]) / s, 0.25f * s, (m[3] - m[1]) / s};
    }
    return q;
}

XrQuaternionf QuatMul(const XrQuaternionf& a, const XrQuaternionf& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

// The palm's offset from the nearest point of the handguard's line (r) and its length, in the
// gun's model space.
float FromGuard(const float* p, float* r = nullptr) {
    const float d[3] = {kGuardTo[0] - kGuardFrom[0], kGuardTo[1] - kGuardFrom[1], kGuardTo[2] - kGuardFrom[2]};
    const float e[3] = {p[0] - kGuardFrom[0], p[1] - kGuardFrom[1], p[2] - kGuardFrom[2]};
    float s = (d[0] * e[0] + d[1] * e[1] + d[2] * e[2]) / (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    s = std::fmin(std::fmax(s, 0.0f), 1.0f);
    float offset[3] = {e[0] - d[0] * s, e[1] - d[1] * s, e[2] - d[2] * s};
    if (r)
        std::memcpy(r, offset, sizeof(offset));
    return Length(offset);
}

// The left hand on the charging handle (the user: the index and middle fingers wrapped round the
// handle's T bar, touching it, not near it; its shaft between them with room, through neither,
// 2026-10-31). Fitted on the mesh dumps' surfaces (0.5 mm samples of both, the glove posed as below
// at grip.txt's palm width 1.15): the index and middle fingers open in a V (kLeverApart, handpose's
// apart: the index turned towards the thumb, the middle, ring and little away), each over its wing
// of the T bar and down its front, its pad on the face that pulls (the middle's on the latch); the
// body and latch between them 1.5 and 1.9 mm clear, the fingers 22.8 mm apart there. Neither the
// handle into the glove nor the glove into it more than 0.2 mm, nor the receiver at any pull (checked
// again independently). The wings sit 5-7 mm over the receiver: too little for the fingertips to curl
// under them, so it's a shallow hook. While the handle is held the glove is there, sliding back with
// it; it blends in from the tracked hand and back out. The pull itself is measured from the hand
// (reload.cpp). The glove's place on the handle is reload.h's kOnLever (the same numbers, beside the
// handle's grip points); the right glove's is its mirror image, the T sitting on the rifle's middle.
// Index, middle (to the touch on their wings: 0.01 more or less is 0.9 mm in or off), ring, little.
constexpr float kLeverCurl[4] = {0.292f, 0.288f, 0.90f, 0.90f};
constexpr float kLeverApart[4] = {16.4f, -15.6f, -14.4f, -16.8f};  // degrees towards the thumb (the V)
constexpr float kLeverThumb = 0.30f, kLeverSpread = 0.4f;
// 0 the tracked hand, 1 on the handle. By hand ([0] the left under its old name; [1] the free right glove's: RightGlove).
float g_leverBy[2] = {};
float& g_lever = g_leverBy[0];

// A glove's fingers blended into the V by share (0-1): the Fingers are either glove's, so the right's V is the left's mirrored.
void FingersOnLever(handpose::Fingers& fingers, float share) {
    for (int i = 0; i < 4; ++i) {
        fingers.curl[i] += (kLeverCurl[i] - fingers.curl[i]) * share;
        fingers.apart[i] += (kLeverApart[i] - fingers.apart[i]) * share;
    }
    fingers.thumb += (kLeverThumb - fingers.thumb) * share;
    fingers.spread += (kLeverSpread - fingers.spread) * share;
}

// a and b (rotation | translation rows of the gloves' bind space into another) blended about a point p
// of the bind space: p goes straight from where a puts it to where b does, and the turn between them
// blends, the rest (the palm's widening) as it is. Blending the rows themselves moved everything as if
// hung from the bind space's origin, 1.2 m from the hand: half-way through a big turn the fingers were
// tens of centimetres out (long black spikes from the glove as it went onto the handguard and off it,
// the user's video, 2026-10-31 07:57).
void BlendAbout(const float* a, const float* b, float s, const float* p, float* out) {
    float ma[9], mb[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            ma[i * 3 + j] = a[i * 4 + j], mb[i * 3 + j] = b[i * 4 + j];
    auto unit = [](XrQuaternionf q) {
        const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return n > 0.0f ? XrQuaternionf{q.x / n, q.y / n, q.z / n, q.w / n} : XrQuaternionf{0, 0, 0, 1};
    };
    XrQuaternionf qa = unit(MatrixToQuat(ma)), qb = unit(MatrixToQuat(mb));
    if (qa.x * qb.x + qa.y * qb.y + qa.z * qb.z + qa.w * qb.w < 0)
        qb = {-qb.x, -qb.y, -qb.z, -qb.w};
    const XrQuaternionf q = unit({qa.x + (qb.x - qa.x) * s, qa.y + (qb.y - qa.y) * s, qa.z + (qb.z - qa.z) * s,
                                  qa.w + (qb.w - qa.w) * s});
    // Each as its turn times the rest (Q^T M), the rests blended straight.
    float turnA[9], turnB[9], turn[9], back[9], restA[9], restB[9], rest[9], m[9];
    QuatToMatrix(qa, turnA), QuatToMatrix(qb, turnB), QuatToMatrix(q, turn);
    Transpose3(turnA, back), Mul3(back, ma, restA);
    Transpose3(turnB, back), Mul3(back, mb, restB);
    for (int i = 0; i < 9; ++i)
        rest[i] = restA[i] + (restB[i] - restA[i]) * s;
    Mul3(turn, rest, m);
    float pa[3], pb[3];
    Transform(a, p, pa);
    Transform(b, p, pb);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[i * 3 + j];
        out[i * 4 + 3] = pa[i] + (pb[i] - pa[i]) * s - (m[i * 3] * p[0] + m[i * 3 + 1] * p[1] + m[i * 3 + 2] * p[2]);
    }
}

void OnTheLever(skin::HandTargets& t, handpose::Fingers& fingers) {
    const bool held = t.gun && reload::HandleSide() == 0;
    g_lever += ((held ? 1.0f : 0.0f) - g_lever) * 0.35f;
    if (g_lever < 0.001f || !t.gun)
        return;
    // On the handle, slid back with it (along the gun's -z).
    const float back[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -reload::HandleBack()};
    float slid[12], onHandle[12], blended[12];
    Compose(t.gunPose, back, slid);
    Compose(slid, reload::kOnLever, onHandle);
    BlendAbout(t.leftPose, onHandle, g_lever, g_leftAnchor, blended);
    std::memcpy(t.leftPose, blended, sizeof(blended));
    Transform(t.leftPose, g_leftAnchor, t.leftWrist);
    FingersOnLever(fingers, g_lever);
}

// The game's grip a little alive: each held finger bent or opened a few degrees with the
// controller's (the index with the trigger, the others with the grip, the thumb with the thumb on
// the stick), about the game's own pose (the user found its fingers stiff, 2026-10-30).
// On the grenade launcher's grip the index finger rests on its trigger (the user, 2026-11-05: "it's really just the index finger. It's not really touching the trigger. I want it
// to really touch that trigger"): the game's own hand there holds it a little off, and the bend above opened it 12 degrees more. Fitted once for each hold taken: the bend
// that brings the pad of its last bone (bone 15, 60% from its last joint to its tip) to the trigger's face, the finger's middle line 8 mm in front of it (the grip study: face
// at z 0.565-0.569, the pad's spot (0, -0.100, 0.566) in the M416's model space). The left trigger then pulls it on (about the right hand's on its own trigger).
constexpr float kM320TriggerTouch[3] = {0.0f, -0.098f, 0.574f};
constexpr float kM320TriggerPull = 14.0f;  // degrees more with the left trigger all the way
float g_m320IndexFit = 0.0f, g_m320IndexSwing = 0.0f;
bool g_m320IndexFitted = false;
float g_m320FitOn[3][12] = {};  // the index's bones of the hold last fitted on
bool g_m320HaveFitOn = false;

// Bent alone it came no nearer than 20 mm (26 as the game holds it: 2026-11-05): curled and swung sideways at its knuckle, together. side: the hand taking
// the grip (free hands: the right's is this same table flipped, the trigger's face on the rifle's middle, so the one fit serves both); only the log names it.
void FitIndexToM320Trigger(const float (*game)[12], int side) {
    float joints[5][4][3];
    handpose::Joints(joints);
    float pad[3];
    for (int i = 0; i < 3; ++i)
        pad[i] = joints[1][2][i] + 0.6f * (joints[1][3][i] - joints[1][2][i]);
    // Touching: the last bone's middle line (its last joint to its tip) 9 mm from the middle of the trigger's face (the glove's finger about that thick), its nearest point in
    // front of the face (never through it). (A point of the pad at one spot was asked too much: the nearest any bend and swing came was 20 mm, 2026-11-05.)
    constexpr float kFace[3] = {0.0f, -0.098f, 0.566f}, kFingerRadius = 0.009f;
    (void)pad;
    float bestBend = 0.0f, bestSwing = 0.0f, bestDistance = 1e9f, before = 0.0f, bestPad[3] = {};
    const auto tryPose = [&](float degrees, float swing) {
        float bent[handpose::kBones][12], a[3], b[3];
        std::memcpy(bent, game, sizeof(bent));
        const float bend[5] = {0.0f, degrees, 0.0f, 0.0f, 0.0f};
        handpose::BendFurther(bent, bend);
        handpose::SwingFinger(bent, 1, swing);
        Transform(bent[15], joints[1][2], a);
        Transform(bent[15], joints[1][3], b);
        const float ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, af[3] = {kFace[0] - a[0], kFace[1] - a[1], kFace[2] - a[2]};
        const float abab = ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];
        const float t = abab > 1e-9f ? std::fmin(std::fmax((af[0] * ab[0] + af[1] * ab[1] + af[2] * ab[2]) / abab, 0.0f), 1.0f) : 0.0f;
        const float nearest[3] = {a[0] + t * ab[0], a[1] + t * ab[1], a[2] + t * ab[2]};
        const float off[3] = {kFace[0] - nearest[0], kFace[1] - nearest[1], kFace[2] - nearest[2]};
        float score = std::fabs(Length(off) - kFingerRadius);
        if (nearest[2] < kFace[2] + 0.004f)
            score += 0.05f;  // (behind the face: through the trigger)
        if (degrees == 0.0f && swing == 0.0f)
            before = score;
        if (score < bestDistance) {
            bestDistance = score, bestBend = degrees, bestSwing = swing;
            std::memcpy(bestPad, nearest, sizeof(bestPad));
        }
    };
    // Every 2 degrees, then every half degree round the best (a few milliseconds, once for each hold taken).
    for (float swing = -40.0f; swing <= 40.0f; swing += 2.0f)
        for (float degrees = -40.0f; degrees <= 90.0f; degrees += 2.0f)
            tryPose(degrees, swing);
    const float coarseBend = bestBend, coarseSwing = bestSwing;
    for (float swing = coarseSwing - 2.0f; swing <= coarseSwing + 2.0f; swing += 0.5f)
        for (float degrees = coarseBend - 2.0f; degrees <= coarseBend + 2.0f; degrees += 0.5f)
            tryPose(degrees, swing);
    g_m320IndexFit = bestBend;
    g_m320IndexSwing = bestSwing;
    g_m320IndexFitted = true;
    // The index's own bones (9, 19, 15: all the fit reads) of the hold it was fitted on: taken again on the same hold, the same answer (FitIndexIfNew).
    std::memcpy(g_m320FitOn[0], game[9], sizeof(g_m320FitOn[0]));
    std::memcpy(g_m320FitOn[1], game[19], sizeof(g_m320FitOn[1]));
    std::memcpy(g_m320FitOn[2], game[15], sizeof(g_m320FitOn[2]));
    g_m320HaveFitOn = true;
    Log("Hands: the %s index finger on the grenade launcher's trigger: bent %.0f and swung %.0f degrees, its last bone's line nearest the trigger's face at (%.3f %.3f %.3f), %.1f mm off touching "
        "(%.1f mm as the game holds it)",
        side == 1 ? "right" : "left", bestBend, bestSwing, bestPad[0], bestPad[1], bestPad[2], bestDistance * 1000.0f, before * 1000.0f);
}

// onRelease (0-1): the index off the grenade launcher's trigger onto its barrel release just in front of it (m320.h: a pull while it isn't ready to fire pushes the release),
// where the game's own index rests, lifting it lift metres (its tip, some 7 cm out, turned up that far).
// mode: the hold it's on. kPistolGrip, free hands: the game's own right hand on the rifle's pistol grip mirrored onto the left glove; its index on the trigger curls in as
// the right one does (skin.cpp's 22 degrees with the trigger all the way), with no opening. The hand's grip, trigger and thumb are given (each hand's own, or a test's), and
// bendState is that hand's smoothed bend (degrees, five): a table in the left glove's layout, bent with the left glove's own axes (the right's is mirrored after).
enum class Bend { kHandguard, kM320Trigger, kPistolGrip };
constexpr float kPistolTriggerBend = 22.0f;  // degrees: the index with the trigger all the way (skin.cpp kTriggerBend, the right's)
void BendWithController(float (*game)[12], Bend mode, float onRelease, float lift, float squeezeIn, float trigger, bool thumb, float* bendState) {
    const bool onM320Trigger = mode == Bend::kM320Trigger;
    const float squeeze = std::fmin(std::fmax((squeezeIn - 0.4f) / 0.6f, 0.0f), 1.0f);
    float index = onM320Trigger ? g_m320IndexFit + kM320TriggerPull * trigger
                  : mode == Bend::kPistolGrip ? kPistolTriggerBend * trigger
                                              : -12.0f + 18.0f * trigger;
    if (onM320Trigger && onRelease > 0.0f)
        index += (-lift / 0.07f * 57.29578f - index) * std::fmin(onRelease, 1.0f);
    const float target[5] = {thumb ? 5.0f : -6.0f, index,
                             -8.0f + 14.0f * squeeze, -8.0f + 14.0f * squeeze, -8.0f + 14.0f * squeeze};
    for (int f = 0; f < 5; ++f)
        bendState[f] += (target[f] - bendState[f]) * 0.3f;
    handpose::BendFurther(game, bendState);
    if (onM320Trigger)
        handpose::SwingFinger(game, 1, g_m320IndexSwing * (1.0f - std::fmin(onRelease, 1.0f)));  // (on the release: straight on, as the game's own)
}

// The left hand as the aim's workhorse, the right hand the base that keeps it level (the user,
// 2026-10-31: "I'm fighting between my left hand and right hand"). Their hands sit 13-22 cm apart on the
// controllers, so ordinary aiming put the hands' line 40-90 degrees off where the right controller
// points, past the 35-65 cone, and the right hand took over. Whether the left hand is on the gun is
// judged by the body instead: the hands' line pointing out ahead of it (within 70 degrees of its
// heading, none past 100) and not down towards the belt (none past 75 degrees down). A hand beside,
// below or behind (the video's wild poses) is out; one far round the right controller is still in.
// That judged it with a fade, which swung the gun back towards the right hand's aim as the left hand moved on out: the hand going back, the gun pushing forward (the user, 2026-11-04,
// "resistance in my aiming"). The hands' line is limited instead (steer::LimitLine), the gun stays at the limit. turn: the gun's turn as the right hand alone points it, as Support's gunTurn.
steer::BodyLine g_bodyLine;

void SteerGateByBody(const float* apart, const float* headTurn, const float* turn) {
    float neck[3], yaw = 0.0f;
    if (g_steerByBody && pouch::BodyFrame(neck, yaw)) {
        // The hands' line out ahead of the right hand (steer::KeepAhead), in the gun's own space where from is; then in the view's, the room's.
        float inGun[3], ahead3[3], line[3];
        for (int i = 0; i < 3; ++i)
            inGun[i] = turn[i] * apart[0] + turn[3 + i] * apart[1] + turn[6 + i] * apart[2];
        steer::KeepAhead(g_steerIn.from, inGun);
        for (int i = 0; i < 3; ++i)
            ahead3[i] = turn[i * 3] * inGun[0] + turn[i * 3 + 1] * inGun[1] + turn[i * 3 + 2] * inGun[2];
        for (int i = 0; i < 3; ++i)
            line[i] = headTurn[i * 3] * ahead3[0] + headTurn[i * 3 + 1] * ahead3[1] + headTurn[i * 3 + 2] * ahead3[2];
        const float length = Length(line);
        if (!(length > 1e-4f))
            return;
        for (float& v : line)
            v /= length;
        const float sine = std::sin(yaw), cosine = std::cos(yaw);
        const float ahead = -sine * line[0] - cosine * line[2];
        const float across = cosine * line[0] - sine * line[2];
        g_steerIn.limited = true;
        g_steerIn.fullAngle = 120.0f * steer::kDegree;  // and a gun turned back past the wrist stays there too
        g_steerIn.noneAngle = 150.0f * steer::kDegree;
        float limited[3];
        if (!steer::LimitLine(ahead, across, line[1], g_bodyLine, limited)) {
            g_steerIn.freeze = true;
            return;
        }
        // Back: into the room, the view's space, the gun's own.
        const float room[3] = {-sine * limited[0] + cosine * limited[1], limited[2], -cosine * limited[0] - sine * limited[1]};
        float view[3];
        for (int i = 0; i < 3; ++i)
            view[i] = headTurn[i] * room[0] + headTurn[3 + i] * room[1] + headTurn[6 + i] * room[2];
        for (int i = 0; i < 3; ++i)
            g_steerIn.to[i] = turn[i] * view[0] + turn[3 + i] * view[1] + turn[6 + i] * view[2];
    }
}

// This frame's lines for the steer: the gun's line to where the left took hold (model, in the gun's model space as the right hand alone holds it), and the line between the hands (apart, in the
// view's space), turned back by the right hand's own turn of the gun (turn^T). Support's, the launcher's, and arms.txt "test guard"'s.
void GiveSteer(const float* model, const float* apart, float between, const float* turn, const float* headTurn) {
    const float lm = Length(model);
    g_steerIn.holding = true;
    g_steerIn.apart = between;
    g_steerIn.together = g_steerByBody ? kHandsTogetherBody : kHandsTogether;
    g_steerIn.lineOk = lm > 0.05f && between > 0.001f;
    if (!g_steerIn.lineOk)
        return;
    for (int i = 0; i < 3; ++i) {
        g_steerIn.from[i] = model[i] / lm;
        g_steerIn.to[i] = (turn[i] * apart[0] + turn[3 + i] * apart[1] + turn[6 + i] * apart[2]) / between;
    }
    SteerGateByBody(apart, headTurn, turn);
}

// ---- Free hands on the rifle (grasp.h): either hand holds it by its pistol grip with its grip button, lets go of it (it falls, or stays in the other
// hand on its handguard alone), and takes it up again by its pistol grip. The right is given it at a spawn: it drops only once that grip has been
// squeezed and let go. Dormant (another gun, seated, the train pistol, loose off, build\freehands.txt "off"), and while the right holds its pistol
// grip, everything here is as before. freehands.txt (freetune.h) also runs its tests (freetest.h). ----
constexpr int kLooseGun = 5;  // skin.cpp's LooseKind: the rifle lying
grasp::State g_grasp;
grasp::Result g_graspNow;  // this frame's once Update has run the grasp (last frame's before)
bool g_freeOn = false;     // the grasp enabled this frame (not dormant)
// Each hand's own place for the rifle, every frame it's seen (the right's as before, the left's its mirror): the gun's model space into the view's, its
// turn (raised as the right's), its place on the gun (the game's right palm; the left's mirrored) and where that goes.
float g_handGun[2][12] = {};
bool g_haveHandGun[2] = {};
float g_anchorBy[2][3] = {};
float g_atBy[2][3] = {};
float g_rifleTurnBy[2][9] = {};
// The free right glove (the right controller with the left's line-up mirrored, as The Great Destroyer's bare right arm has it) in the view's space,
// its palm (the left's palm point mirrored), and whether the right hand is seen this frame.
float g_rightFree[12] = {};
float g_rightPalm[3] = {};
bool g_rightSeen = false;
// The rifle held by its handguard alone: its place on that hand's grip (the grip's frame -> the gun's model space); flown into a hand from here (the
// room); as last drawn (the room, for the next frame), and each hand's grip then.
float g_onBarrel[12] = {}, g_glideFrom[12] = {}, g_lastGunRoom[12] = {}, g_lastGripRoom[2][12] = {};
bool g_haveLastGunRoom = false, g_haveLastGrip[2] = {};
// Recoil (design 6, recoilrun.h): the gun in hand kicked by its own recoil in Update, after the steer and before the gloves take their holds. The poses as
// the hands hold them this frame, from before the kick: what judges what the hands do reads these (AsHeld), what's drawn rides the kicked ones. The gun as
// last drawn, kicked (the room's), beside g_lastGunRoom as the hands held it: a let-go starts its fall from where it was drawn (the same with no kick).
float g_heldGun[12] = {}, g_heldLauncher[12] = {}, g_heldPistol[12] = {};
int g_kicked = 0;  // which pose the kick moved this frame: 0 none, 1 gunPose, 2 launcherPose, 3 pistolPose
float g_lastGunDrawn[12] = {};
constexpr float kHalfIpd = 0.032f;  // metres from the head's middle to each eye while the views haven't said (views::EyeOffset)

// The pose as the hands hold it: the kicked one's copy from before the kick, any other as it is.
const float* AsHeld(const skin::HandTargets& t, const float* pose) {
    return g_kicked == 1 && pose == t.gunPose ? g_heldGun : g_kicked == 2 && pose == t.launcherPose ? g_heldLauncher : g_kicked == 3 && pose == t.pistolPose ? g_heldPistol : pose;
}
// A let-go to start this frame (StepLoose, with this frame's Frame), and the rifle lying (loose's, after the last Step). Each with the gun it is (free hands
// on every gun: the profile's pose, words and knocks; the M416's by default, so a reset reads as before).
struct GunRelease {
    bool want = false;
    int side = -1;
    bool fromGuard = false;  // let go of its handguard (else of its pistol grip)
    float room[12] = {};
    int rounds = 0;
    gunprofile::Pose pose = gunprofile::Pose::kRifle;
    uint32_t id = gunprofile::kRifleId;  // which gun (the launcher's body is picked again as it's let go of: its case on or not)
    const loose::Spec* body = nullptr;    // its loose body (none: it stays in the hand)
    const char* name = "rifle";
    const char* front = "handguard";
    gunprofile::Sound sound = gunprofile::Sound::kMetal;
    int slot = 0;  // the player's slot it's in, as the game has it (SlotOfProfile): a swap that changes it took this gun
    // Laid down still where it lay before instead (no hand: loose::Place), its pose then (loose's, the arms' space with that origin): the owner's flow
    // (b), the gun a scene's gift took while it lay given back (LyingGone).
    bool still = false;
    float stillBone[12] = {};
    int stillOrigin[3] = {};
} g_gunRelease;
struct GunLoose {
    int id = 0;
    bool have = false, resting = false;
    float bone[12] = {}, room[12] = {};
    int origin[3] = {};
    gunprofile::Pose pose = gunprofile::Pose::kRifle;
    const char* name = "rifle";
    gunprofile::Sound sound = gunprofile::Sound::kMetal;
    int slot = 0;  // the player's slot it's in, as the game has it (SlotOfProfile): a swap that changes it took this gun
    ULONGLONG releasedAt = 0;  // when it was let go of: a swap from before isn't this gun's
} g_gunLoose;
bool g_gunLost = false;  // loose told it gone: the grasp gives it back to the right hand next frame
// The magnet (grasp.h): frames each hand has been seen with its own reading (not held through a gap, nor eased back from a jump; a test's hand always),
// for HandIn::reachDown; when the gun lying last rested (and which lying gun that was); the glide under way is the game's dropped gun's (its sound).
int g_magnetSeenBy[2] = {};
ULONGLONG g_lyingRestAt = 0, g_lyingRestOf = 0;
bool g_glideFromGame = false;
ULONGLONG g_missSaidAt = 0;  // the last "out of the magnet's reach" line (one a second at most)
// For input.cpp (one frame old) and xr_runtime: whose trigger fires it, whether a hand has it, a cutscene or scripted moment, a new life; when Update
// last ran (a second without it: as before).
std::atomic<int> g_fireHandNow{1};
std::atomic<bool> g_holdsGunNow{true}, g_scripted{false}, g_newLife{false};
// The gun in two hands as recoil counts them (held by its pistol grip, the other hand's glove on its front at least half way): a film round's real rounds
// only so (input.cpp's FireAlone, TwoHands).
std::atomic<bool> g_twoHandsNow{false};
// The hand on the gun's pistol grip (0 the left, 1 the right, -1 nobody: lying, gliding into a hand, or held by a handguard alone), for the D-pad's
// right (input.cpp); and a test round's left D-pad right held (freetest's dpadRight), with when a round last set it.
std::atomic<int> g_gripHandNow{1};
std::atomic<bool> g_scriptEjectNow{false};
std::atomic<ULONGLONG> g_scriptEjectAt{0};
std::atomic<ULONGLONG> g_freeHandsAt{0};
// The left on the rifle's pistol grip this frame, its other jobs closed (no magazine, charging handle or grenade launcher), or the frame its squeeze
// took the rifle; and last frame, a magazine, a round or the launcher's barrel in the left hand. By hand ([0] the left under the old names, [1] the right).
bool g_onGripBy[2] = {}, g_spentBy[2] = {}, g_busyWasBy[2] = {};
bool& g_leftOnGrip = g_onGripBy[0];
bool& g_leftSpent = g_spentBy[0];
bool& g_leftBusyWas = g_busyWasBy[0];
// The hand at work (FreeHands): the one that may do the rifle's mechanics (its handle, bolt release, magazine, the launcher's grip and reload) with its own
// controller, one at a time: the hand not on the pistol grip, or the free one while the rifle's held by its handguard (or the launcher's grip) alone. 0 the
// left (also while dormant, as before), 1 the right, -1 nobody (the rifle lying). The right works only while its own pose is drawn (freehands.txt
// "rightpose 0": its glove is the game's grip carried at its controller, its mechanics off too).
int g_workHand = 0;
bool g_rightWorks = false;  // g_freeOn && g_rightSeen && freetune::RightPose()
const char* const kHandName[2] = {"left", "right"};
// The hand holding the rifle as the grenade launcher's module has it (m320::Frame::gunHand: the other is its hand at work), kept while the launcher has
// something in the hand (a round in one hand never jumps to the other as the rifle changes hands), and while its glove or fingers still ease back to that
// hand (g_m320Easing: last frame's gloveBlend or fingersShare; the ease is never handed to the other glove, whose hold it isn't); 1 while dormant, as before.
int g_m320GunHand = 1;
bool g_m320Easing = false;
// The pouch's own arming as its Update counts it (pouch.cpp): the side it was last fed, and whether that hand's squeeze (its grip or its trigger, the more
// pressed) has been under 0.4 since it last went past 0.6 with the hand seen. FreeHands' claim goes by it: the frame the pouch can really take with it.
int g_pouchFedSide = 0;
bool g_pouchArmed = true;
extern float g_rightBlend;  // the right glove on its hold, 0 free (RightGlove, below)
extern int g_rightHold;     // ... the hold it's on or easing off, 1 the pistol grip
// The game's aim from free hands, in the recentered space: as last published, whether it's the hands' own (else the old way), and where a release, a
// glide and an eased change started from.
XrQuaternionf g_aimNow{0, 0, 0, 1}, g_releaseAim{0, 0, 0, 1}, g_glideAimFrom{0, 0, 0, 1}, g_easeFrom{0, 0, 0, 1};
bool g_aimOwn = false;
constexpr float kAimEase = 0.15f;  // s: a change of the aim's source eased over this
float g_easeFor = kAimEase;        // s into the latest
// The game's own right hand on the pistol grip (skin::GameRightGrip: 23-44, [bone - 23]), and mirrored onto the left glove (0-22): filled in place,
// again only when it changes.
float g_gameRightGrip[handpose::kRightBones][12] = {};
float g_leftOnPistol[handpose::kBones][12] = {};
bool g_haveRightGrip = false;
// A gap in Update (the headset off and on: SteamVR asks for no frames meanwhile and comes back focused, so no frame here ever saw a grip unread): the
// first frame after it counts as one with every real hand's grip unread, so grasp disarms the pistol-grip hand and g_barrelArmed the barrel hand, and
// a controller left lying open on the desk never drops the rifle. (A test's hand is scripted, never unread.)
constexpr ULONGLONG kResumeGap = 500;  // ms without an Update
bool g_freeResumed = false;            // this frame is the first after such a gap
// The barrel hand's own arming, as grasp's armed[] for the pistol-grip hand: its grip unread, the hand unseen, a scripted moment or a resume disarm
// it, a squeeze past grasp::kTake while it's read and seen arms it again. Holding the rifle by its handguard alone, only an armed grip's let-go drops
// the rifle (a controller woken open, a cutscene's end: it drops only once that grip has been squeezed and let go, the user's told default).
bool g_barrelArmed[2] = {};
// Loose with no room for the rifle let go of (KeepTheRifle): said once a streak; a handguard let go of (grasp asks again every frame) tried again only
// every kKeptRetry ms, the rifle frozen on that hand meanwhile.
constexpr ULONGLONG kKeptRetry = 500;
ULONGLONG g_keptAt = 0;  // when loose last had no room in this streak (0: none)
bool g_keptSaid = false;
// The left glove on the rifle's pistol grip or easing off it (Update): the left's own place for the rifle is worked out for that ease too, free
// hands gone dormant (it opens where it was, as the handguard's ease does).
bool g_glovesOnPistol = false;
// Each trigger hand's glove on the pistol grip: the rifle it rode (as steered; where a glide takes it) relative to that hand's own place for the
// rifle, so let go it opens where it was drawn and goes with its hand (as g_rightHoldOn does on the handguard): it turned by the whole steer at once.
float g_gripRel[2][12] = {};
bool g_haveGripRel[2] = {};
// A regrip's glide (from a hold on the handguard alone): where the rifle goes, the trigger hand's place turned by the steer onto the barrel hand's
// hold as it'll be held on arrival (the steer runs on through the glide: no swing then), and where it's drawn on the way, from the barrel hand's
// frozen hold as that hand moves; both in the view's space, this frame. The barrel hand's glove rides the drawn one (it never leaves the handguard).
float g_glideTo[12] = {}, g_glideDrawn[12] = {};
bool g_regripDrawn = false;
// The game's aim's source last frame (PublishAim): a change of it is eased from the aim as last published; reset with the grasp.
grasp::Holder g_aimHolderWas = grasp::Holder::kHeld;
int g_aimTriggerWas = 1, g_aimBarrelWas = -1;
bool g_aimGunWas = true;
// Free hands on every gun (gunprofile.h): whatever's in hand, made once a frame in Update. Through a switch's frames with no gun drawn, the last one.
gunprofile::Profile g_profile;          // this frame's (Update, after the anchor)
gunprofile::Profile g_profileLast;      // the last that wasn't none (the switch's bridge, the logs' name while dormant)
gunprofile::Profile g_leftGripProfile;  // the gun the left last held by its pistol grip (its ease off it goes by it)
uint32_t g_profileWas = 0;              // last frame's id (none's 0), set every frame: another is a change of gun
const char* g_profileWasName = "gun";   // ... its gun's name (the lines of a change of gun: the gun before)
uint32_t g_rightGripFor = 0;            // the gun g_gameRightGrip / g_leftOnPistol were fetched for
bool g_smawCaseOnNow = true;            // the launcher's case on its back as rocket::Update last had it (the launcher's body: with it or bare)
int g_lastHeldSide = -1;                // the pistol-grip hand in the last frame free hands were on (-1 not held so): a switch keeps it
ULONGLONG g_lastHeldAt = 0;
ULONGLONG g_noneSince = 0;              // when this run of frames with no gun drawn began (0 none)
bool g_bridgeOk = false;                // ... it began right after the gun it keeps (the frame before had it): only then is it bridged
bool g_bridging = false;                // this frame's profile is the last one, through a switch (nothing changes hands)
// Each pose's grip and up this frame ([0] the M416's, [1] the launcher's or a held gun's), and the left's place for the pose that isn't the profile's: in
// the left hand the other pose is placed there too, so a switch's new gun, drawn in its first frame from the last frame's places, is in that hand.
float g_poseGrip[2][3] = {}, g_poseUp[2] = {}, g_poseRight[2] = {};
float g_otherLeft[12] = {};
bool g_haveOtherLeft = false;
char g_profileWhy[96] = "";             // why the profile is none, for the asleep line
// A held gun's body with its attachments' points in its hull: its fitted body copied, remade only when its attachments or its body change. loose copies
// a body and its hull at each start, so these need only outlive the frame.
loose::Spec g_attachedBody{};
float g_attachedHull[bodyfit::kMostHull][3] = {};
const loose::Spec* g_attachedFrom = nullptr;
uint32_t g_attachedSet = 0;
float g_attachedMin[3] = {}, g_attachedMax[3] = {};
int g_attachedPoints = 0;  // of its hull's points, its attachments'
extern bool g_onLauncherBy[2];  // the launcher's front grip by side (LauncherSupport, below)
// ---- The game's own dropped guns taken by hand (pickups.h; design 4): what a hand may take this frame, the take asked of the game, the gun that lay
// retired when the game swaps it away, and the gun the game gave on its way into the hand that asked. ----
struct PickState {
    uint32_t request = 0;             // the take asked of the game, waiting (0 none) ...
    int side = -1;                    // ... by that hand
    float centreWorld[3] = {};        // ... the middle of that gun's box where it lay (the world): where its glide starts
    char name[pickups::kName] = {};   // ... its blueprint's name
    uint32_t swapSeen = 0;            // the last swap looked at (kept through every reset: no swap is read twice)
    bool lyingRetired = false;        // the game swapped the gun that lies into its pickup: its copy stays drawn, nobody can take it, till the arrival,
    ULONGLONG retiredAt = 0;          // the return or the net
    int landedSide = -1;              // a hand's take landed and its gun is due in that hand once it's the gun in hand ...
    uint32_t landedId = 0;            // ... as that profile (free hands know the gun),
    bool landedAny = false;           // ... or as any profile but wasId (the guns' table has no row for its name)
    uint32_t wasId = 0;               // ... the profile's id before its swap was seen (last frame's: its gun may be drawn that frame)
    bool sameKind = false;            // ... the same gun as the one that lay: no change of gun to wait for
    bool freed = false;               // ... the hands were freed at once (another slot, not forced): its arrival is still honoured
    int landedSlot = -1;              // ... the slot it went into (for the log)
    ULONGLONG landedAt = 0;
    int landedFrames = 0;
    float landedWorld[12] = {};       // ... where it lay as the game drew it (the world)
    bool magnet = false;              // the take asked was won by the magnet (grasp's Result::magnet): its arrival flies in as a magnet take (Frame::arriveMagnet)
    bool giveBack = false;            // the take asked is a draw from a shoulder (holster.h's Away: the game's own take of the pickup a holstered gun went
                                      // into): its arrival starts in the hand that drew it, nothing flies from where the pickup lies
    bool refusedNow = false, swappedAwayNow = false;  // for this frame's grasp (Frame::refused, Frame::swappedAway)
    uint32_t kickOutId = 0;           // a gun the game swapped out of its slot while recoil had it: no gun for recoil while it's still drawn ...
    ULONGLONG kickOutAt = 0;          // ... since then (2 s at most)
    bool byUse = false;               // the landed take is a scripted prompt's gift (g_useMade): nothing of it was drawn lying (no pre-arm draw)
};
PickState g_pick;
ULONGLONG g_pickTookAt = 0;  // when a hand's take last landed (NoteSwaps): the M416's seated tell says nothing for a while after (pickrules::SeatedSleeps)
// A scripted prompt used by hand (usepoint.h, pickups.h's UsePoint): the decision's state; the use asked of the game (pickups::RequestUse), and a use
// made (the game's, or its key's), so the gun the scene gives next comes into that hand as a take's (OnGunChanged: the AT4 on Swordbreaker's roof).
usepoint::State g_usePoint;
struct UseAsked {
    uint32_t serial = 0, entity = 0;  // the use asked and its point (0 none waiting)
    int side = -1, action = -1;
    ULONGLONG at = 0;
    char sid[pickups::kName] = {};
    float hand[3] = {};               // the asking hand's grip as it asked (the world): anywhere in the prompt's reach
    bool haveHand = false;
};
UseAsked g_useAsked;
struct UseMade {
    int side = -1;                    // the hand that used a prompt (-1 none) ...
    ULONGLONG at = 0;                 // ... then
    uint32_t wasId = 0;               // ... with this gun's profile in the hands (0 none): a gun the scene gives is any other
    bool gift = false;                // ... a prompt that gives a gun (on the game's pickup key, the reload's: the AT4's); a door gives none
    float world[3] = {};              // ... the point (the world): where a gift flies in from (its glide's start) when the hand's isn't known
    bool haveWorld = false;
    float lay[3] = {};                // ... where the hand was as it asked (the world: its grip then, anywhere in the reach): where a gift flies in from
    bool haveLay = false;
    char sid[pickups::kName] = {};
};
UseMade g_useMade;
ULONGLONG g_gunChangedAt = 0;  // the last change of gun (OnGunChanged): a gift X held's R at a prompt asked for has come since (GiftOnItsWay)
// How long after a use a change of gun is the scene's gift into the hand that used it. Operation Swordbreaker's AT4, measured on the roof
// (build\designs\at4_measured.md, section 3): the kit's counts 0.1 s after the press, the tube on the floor gone by then or soon after (between 0.38
// and 0.75 s in the take's film), the M4A1's last draw at 0.41 s and the AT4's first at 0.69 s: no AT4 drawn anywhere for 0.3 to 0.6 s. Its gift flies in
// from where the hand that took it was as it asked (UseMade::lay: anywhere in the reach, not where the tube lay, which isn't measured), so from the
// first frame it's drawn it comes from that hand's reach into that hand, the gap between bridged by the take; the "RPG" save reaches the disk at the
// first death, not then. 3 s holds that with room; only the first change of gun after it, and never one a pick or a draw from a place made.
constexpr ULONGLONG kUseGiftMs = 3000;
// The owner's flow (b) on the roof (pickrules::LyingTaken, LyingBack): the gun that lay on the ground when a scene's gift took its slot (the PlayerAT4
// kit with the M4A1 let go of on the floor: its game gun goes with the kit, and nothing of it is drawn while the game draws the AT4: no hosting),
// where it lay, so the game's gun of its kind back in that slot lies there again, still, never into a hand by itself. One at a time; kept through a
// death on its level (the "RPG" checkpoint after a miss: the scene gives it back after the hit as in the first try), its 120 s afresh each new life,
// a new life's own kit no give-back; another level's kit ends it.
struct LyingGone {
    bool on = false;
    uint32_t id = 0;                // its profile (its kind)
    int slot = -1;                  // the slot it lay from, and comes back into
    float bone[12] = {};            // where it lay: loose's pose, the arms' space with origin
    int origin[3] = {};
    const char* name = "rifle";
    const char* gift = "gun";       // the gun the scene gave in its place (for the lines)
    ULONGLONG at = 0;               // when the scene took it, or the newest new life since began
    ULONGLONG lifeAt = 0;           // the newest new life's start since the record (0 none)
};
LyingGone g_lyingGone;
// X held's own pickup (the game's R) put the gun in a slot the game didn't raise: its key once R is let go (the review of 2026-11-18: it stayed in its
// slot unsaid and unseen), so it comes into the right hand as X held's pick does; -1 none.
int g_raiseAfterR = -1;
ULONGLONG g_raiseAfterRAt = 0;
char g_raiseAfterRName[pickups::kName] = {};
constexpr unsigned long long kPickupRAfterMs = 500;  // X held's R for a pickup let go at most this long before its take's swap was told: still X held's
pickups::Reach g_reach[grasp::kMostCandidates];        // this Update's candidates as pickups gave them (the world) ...
grasp::Candidate g_reachRoom[grasp::kMostCandidates];  // ... and in the room, for the grasp and a test's hand
int g_reachCount = 0;
uint32_t g_reachSerial = ~0u, g_updateSerial = 0;      // the Update they were made in; Update's count
uint32_t g_kickId = 0;                                 // the gun the recoil block last gave recoilrun (0 none) ...
int g_kickSlot = -1;                                   // ... and its slot
void ResetPick();
int PickupCandidates(grasp::Candidate* out);
void HolsterSlots(holster::SlotIn* out);  // (below, with holsters' own)

// Where a glide takes the rifle into hand side: a regrip's steered place this frame, else that hand's own place for it.
const float* GlideTarget(int side) {
    return g_regripDrawn && g_graspNow.trigger == side ? g_glideTo : g_handGun[side];
}

bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

// A pose as a matrix (three rows: rotation | translation), and back.
void PoseMatrix(const XrPosef& p, float* out) {
    float m[9];
    QuatToMatrix(p.orientation, m);
    const float at[3] = {p.position.x, p.position.y, p.position.z};
    for (int i = 0; i < 3; ++i)
        out[i * 4] = m[i * 3], out[i * 4 + 1] = m[i * 3 + 1], out[i * 4 + 2] = m[i * 3 + 2], out[i * 4 + 3] = at[i];
}

XrPosef MatrixPose(const float* m) {
    const float r[9] = {m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]};
    XrQuaternionf q = MatrixToQuat(r);
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    q = n > 0.0f && std::isfinite(n) ? XrQuaternionf{q.x / n, q.y / n, q.z / n, q.w / n} : XrQuaternionf{0, 0, 0, 1};
    XrPosef p{};
    p.orientation = q;
    p.position = {m[3], m[7], m[11]};
    return p;
}

// build\freehands.txt "test N" (freetest.h) with weapon.txt "standin 1" and nobody in the headset: scripted hands, the right only while the stand-in holds
// the rifle, the left only while the real left controller isn't tracked. Each gives its grip pose (the room), its grip, trigger and thumb, and for a throw
// its speed and turn (the room's axes).
struct TestHand {
    bool on = false;
    freetest::Step step;
    XrPosef grip{};
    float velocity[3] = {}, spin[3] = {};
    freetest::Where heldWhere = freetest::Where::kNone;  // kGuard and kGrip: worked out at their first frame, then held relative to the head
    float held[12] = {};
};
TestHand g_test[2];
// The round in each hand as the launcher module last had it in that glove (its bind space: the right's in the right glove's), by side ([0] under the old
// names): weapon.txt "m320test 7"'s script holds it where it's drawn, and free hands' film rounds put a scripted hand by it.
float g_lastRoundInGloveBy[2][12] = {};
float (&g_lastRoundInGlove)[12] = g_lastRoundInGloveBy[0];
bool g_haveLastRoundInGloveBy[2] = {};
bool& g_haveLastRoundInGlove = g_haveLastRoundInGloveBy[0];

// Each hand's own grip, whether it was read, its trigger, its thumb on the stick, its grip pose and its sensed speed: its controller's, or the test's
// while one runs for that hand.
float SqueezeOf(int side) {
    if (g_test[side].on)
        return g_test[side].step.squeeze;
    return side == 0 ? input::LeftSqueeze() : input::Squeeze(1);
}

bool ReadOf(int side) {
    return g_test[side].on || input::SqueezeRead(side);
}

float TriggerOf(int side) {
    if (g_test[side].on)
        return g_test[side].step.trigger;
    return side == 0 ? input::LeftTrigger() : input::Trigger(1);
}

bool ThumbOf(int side) {
    if (g_test[side].on)
        return g_test[side].step.thumb > 0.5f;
    return side == 0 ? input::LeftThumbOnStick() : input::ThumbOnStick(1);
}

bool HandPoseOf(int side, XrPosef& out) {
    if (g_test[side].on) {
        if (g_test[side].step.lost)
            return false;  // (a film round's hand giving no place, from its first frame: no 0.5 s hold)
        out = g_test[side].grip;
        return true;
    }
    return input::HandPose(side, out);
}

bool HandVelocityOf(int side, float* v, float* w) {
    if (g_test[side].on) {
        if (!g_test[side].step.sensed || g_test[side].step.lost)
            return false;
        std::memcpy(v, g_test[side].velocity, sizeof(g_test[side].velocity));
        std::memcpy(w, g_test[side].spin, sizeof(g_test[side].spin));
        return true;
    }
    return input::HandVelocity(side, v, w);
}

// What rounds 4-9's places go by (freeplaces.h), this frame's: the head, the rifle as drawn last frame, the stand-in's grip (the head's space), the left glove's
// line-up and palm point, the body frame, the game's left hold on the launcher's grip, and each hand's round in its glove.
void MakeScene(freeplaces::Scene& scene, const float* headPose, const float* standInHead) {
    std::memcpy(scene.headPose, headPose, sizeof(scene.headPose));
    scene.haveGun = g_haveLastGunRoom;
    std::memcpy(scene.gunRoom, g_lastGunRoom, sizeof(scene.gunRoom));
    std::memcpy(scene.standInHead, standInHead, sizeof(scene.standInHead));
    std::memcpy(scene.lineUp, g_leftOffset, sizeof(scene.lineUp));
    std::memcpy(scene.leftAnchor, g_leftAnchor, sizeof(scene.leftAnchor));
    scene.haveBody = pouch::BodyFrame(scene.neck, scene.yaw);
    float bones[handpose::kBones][12];
    scene.haveM320Grip = skin::GameLeftGripM320(bones);
    if (scene.haveM320Grip)
        std::memcpy(scene.m320Grip, bones[0], sizeof(scene.m320Grip));
    for (int side = 0; side < 2; ++side) {
        scene.haveRound[side] = g_haveLastRoundInGloveBy[side];
        std::memcpy(scene.roundInGlove[side], g_lastRoundInGloveBy[side], sizeof(scene.roundInGlove[side]));
    }
    // (Round 13's launcher steps go where holsters.txt puts launchers: behind the neck, or the rib cage.)
    const holster::Place launcher = holster::LauncherPlace(g_holsterSet);
    scene.haveLauncher = launcher != holster::Place::kNone;
    if (scene.haveLauncher)
        std::memcpy(scene.launcher, holster::EggOf(launcher, g_holsterSet).mid, sizeof(scene.launcher));
    // (Round 14: the held gun's lever, its knob at rest on the gun's frame, where the gun as drawn last frame is a held gun's: launcherPose's frame.)
    const handmag::Gun* held = skin::HeldMagGun();
    scene.haveLever = held != nullptr && g_profile.pose == gunprofile::Pose::kLauncher;
    if (scene.haveLever) {
        std::memcpy(scene.lever, held->lever, sizeof(scene.lever));
        scene.leverTravel = held->leverTravel;
        handmag::FistPoint(0, scene.fistInGlove);
    }
}

// The left hand's middle in the gloves' bind space (the palm widens about it: handpose.cpp), where skin.cpp measures the game's palms on a gun through the hand's
// own bone, 1 (its kLeftPalm: gunprofile's anchor and support are that point on the gun). The right's: x negated.
constexpr float kPalmMiddle[3] = {0.537f, 1.098f, 0.207f};

// The game's own left hand on the front of the gun in hand: the M416's handguard hold (skin::GameLeftGrip), a launcher-pose gun's rest on its front grip
// (skin::RigRestLeft, only when it's this gun's rest).
bool SupportTable(float (*bones)[12]) {
    if (g_profile.pose != gunprofile::Pose::kLauncher)
        return skin::GameLeftGrip(bones);
    uint32_t kind = 0;
    return skin::RigRestLeft(bones, kind) && kind == (g_profile.id == gunprofile::kSmawId ? 0u : g_profile.id);
}

// This frame's test hands, before the hands are placed. headPose: the head in the room; fromHead: the room into the head; headTurn: the head's turn;
// anchor: the right hand's place on the gun in hand (its model space). kGuard and kGrip go by that gun as drawn last frame. Rounds 4-9's places, and a step
// moved from one place to another, are freeplaces.cpp's; their events (the bolt locked back, the launcher opened empty or with a spent case in it, X
// tapped) come at a step's first frame, once each, logged. kPickup (test 10) is the nearest dropped gun of the game's (PickupCandidates), decided
// here before freeplaces is asked; test 10's trigger pulls reach the game only with pickups.txt "fire 1" (they'd be real rounds from a gun just taken).
void RunFreeTest(const float* headPose, const float* fromHead, const float* headTurn, const float* anchor, float dt) {
    using freetest::Where;
    static int s_test = 0;
    static float s_clock = 0.0f;
    static bool s_events[2][4] = {};  // each hand's step events as last frame's step had them
    static int s_seenWas[2] = {};     // each hand's step as last said: 0 seen, 1 not seen (its place kept), 2 no place (round 13)
    g_scriptEjectNow = false;  // (a level, set again below each frame its step holds it: a round switched off or changed lets go of it)
    const int test = freetune::Test();
    const bool on = test >= 1 && test < static_cast<int>(std::size(freetest::kRound)) && freetest::kRound[test] > 0.0f && input::StandInOn();
    if (!on || test != s_test) {
        s_clock = 0.0f;
        g_test[0] = g_test[1] = TestHand{};
        std::memset(s_events, 0, sizeof(s_events));
        s_seenWas[0] = s_seenWas[1] = 0;
    }
    s_test = on ? test : 0;
    if (!on)
        return;
    s_clock += dt;
    freetest::Step steps[2];
    freetest::At(test, std::fmod(s_clock, freetest::kRound[test]), steps);
    // The stand-in's right grip (it stands in while the right controller sleeps), in the room's space and the head's.
    XrPosef standIn{};
    if (!input::HandPose(1, standIn)) {
        g_test[0].on = g_test[1].on = false;
        return;
    }
    float standInRoom[12], standInHead[12];
    PoseMatrix(standIn, standInRoom);
    Compose(fromHead, standInRoom, standInHead);
    freeplaces::Scene scene;  // (made once, when a step needs it)
    bool haveScene = false;
    for (int side = 0; side < 2; ++side) {
        TestHand& h = g_test[side];
        const freetest::Step& s = steps[side];
        XrPosef real{};
        const bool allowed = side == 1 ? input::StandingIn() : !input::HandPose(0, real);
        // The left's D-pad right: held as input.cpp's own would be, any gun; whether it counts as X is input.cpp's (ejectkey.h), never the round's.
        if (side == 0 && allowed && s.dpadRight) {
            g_scriptEjectNow = true;
            g_scriptEjectAt = GetTickCount64();
        }
        // The step's events, at its first frame (each once a step): the modules' own test hooks. The M416's alone (another gun in hand: said, not done).
        const bool events[4] = {s.lockBack, s.m320Open, s.m320Case, s.magTap};
        for (int k = 0; k < 4; ++k) {
            if (events[k] && !s_events[side][k] && allowed && s.where != Where::kNone) {
                if (g_profile.pose != gunprofile::Pose::kRifle) {
                    static constexpr const char* kEvent[4] = {"the bolt locked back", "the launcher's barrel emptied and opened", "a spent case put in the launcher's barrel",
                                                              "X tapped"};
                    Log("Free hands: test %d: %s skipped (the %s in hand)", test, kEvent[k], g_profile.name);
                } else if (k == 0) {
                    const bool wasBack = reload::BoltBack();
                    reload::TestLockBack();
                    Log("Free hands: test %d: %s (the %s hand's step)", test, wasBack ? "the bolt was back already" : "the bolt locked back", kHandName[side]);
                } else if (k == 1) {
                    m320::TestOpenEmpty();
                    Log("Free hands: test %d: the launcher's barrel emptied and opened (the %s hand's step)", test, kHandName[side]);
                } else if (k == 2) {
                    m320::TestOpenWithCase();
                    Log("Free hands: test %d: a spent case put in the launcher's open barrel (the %s hand's step)", test, kHandName[side]);
                } else {
                    input::MagazineTap(true);
                    Log("Free hands: test %d: X tapped (the %s hand's step)", test, kHandName[side]);
                }
            }
            s_events[side][k] = events[k];
        }
        if (!allowed || s.where == Where::kNone) {
            h.on = false;
            h.heldWhere = Where::kNone;
            continue;
        }
        // Its own side's stand-in grip: the right's as it is, the left's mirrored about the head (in its space p (-x, y, z), q (x, -y, -z, w)).
        float base[12], inHead[12], grip[12];
        std::memcpy(inHead, standInHead, sizeof(inHead));
        if (side == 0)
            handpose::Mirror(inHead, inHead);
        Compose(headPose, inHead, base);
        bool have = true;
        if (s.where == Where::kPickup) {
            // The grip point at the middle of the nearest dropped gun's box (this Update's first candidate: the nearest the eye), turned as the hand last was;
            // worked out at the step's first frame, then held relative to the head. None near: where it was.
            if (h.heldWhere != Where::kPickup) {
                grasp::Candidate c[grasp::kMostCandidates];
                if (PickupCandidates(c) > 0 && Finite(c[0].room, 12) && Finite(c[0].boxMin, 3) && Finite(c[0].boxMax, 3)) {
                    const float middle[3] = {(c[0].boxMin[0] + c[0].boxMax[0]) / 2, (c[0].boxMin[1] + c[0].boxMax[1]) / 2, (c[0].boxMin[2] + c[0].boxMax[2]) / 2};
                    float at[3], last[12];
                    Transform(c[0].room, middle, at);
                    if (h.on)
                        PoseMatrix(h.grip, last);
                    else
                        std::memcpy(last, base, sizeof(last));
                    last[3] = at[0], last[7] = at[1], last[11] = at[2];
                    Compose(fromHead, last, h.held);
                    h.heldWhere = Where::kPickup;
                }
            }
            if (h.heldWhere == Where::kPickup)
                Compose(headPose, h.held, grip);
            else if (h.on)
                PoseMatrix(h.grip, grip);
            else
                std::memcpy(grip, base, sizeof(grip));
        } else if (freeplaces::Handles(s.where) || s.from != Where::kNone) {
            // Rounds 4-9's places (either side: the left's tables mirrored for the right). Not now (no rifle drawn, no body frame, the launcher's grip
            // not learned, no round in that glove): the hand stays where it was.
            if (!haveScene) {
                MakeScene(scene, headPose, standInHead);
                haveScene = true;
            }
            if (!freeplaces::Grip(scene, s, side, grip)) {
                if (h.on)
                    PoseMatrix(h.grip, grip);
                else
                    std::memcpy(grip, base, sizeof(grip));
            }
        } else {
            switch (s.where) {
            case Where::kStandIn:
            case Where::kMirror:
                std::memcpy(grip, base, sizeof(grip));
                break;
            case Where::kFront:
                // 30 cm out, 12 cm to its side, 15 cm down, turned as that grip.
                inHead[3] = side == 0 ? -0.12f : 0.12f, inHead[7] = -0.15f, inHead[11] = -0.30f;
                Compose(headPose, inHead, grip);
                break;
            case Where::kGuard:
            case Where::kGrip:
                if (h.heldWhere != s.where && g_haveLastGunRoom) {
                    if (s.where == Where::kGuard) {
                        // The glove at the game's own hold on the handguard of the rifle as drawn (the right's that hold mirrored), the controller behind it
                        // through the line-up (the right's mirrored).
                        // A launcher-pose gun's front: the hand itself where the game's rests there (its own bone, 1, not the wrist's: that hold bends the
                        // wrist, and a glove put on its wrist bone with its own wrist straight had its hand swing round at the take, 4 cm and more, the arm
                        // with it), the palm's middle on the profile's support for that side (where the game's palm was measured to rest). The take then
                        // finds the hand where the hold has it: only the fingers close.
                        const bool front = g_profile.pose == gunprofile::Pose::kLauncher;
                        float bones[handpose::kBones][12], root[12], offset[12], glove[12], back[12], controller[12];
                        if (SupportTable(bones)) {
                            std::memcpy(root, bones[front ? 1 : 0], sizeof(root));
                            std::memcpy(offset, g_leftOffset, sizeof(offset));
                            if (side == 1) {
                                handpose::Mirror(root, root);
                                handpose::Mirror(offset, offset);
                            }
                            Compose(g_lastGunRoom, root, glove);
                            if (front) {
                                const float middle[3] = {side == 1 ? -kPalmMiddle[0] : kPalmMiddle[0], kPalmMiddle[1], kPalmMiddle[2]};
                                float at[3], now[3];
                                Transform(g_lastGunRoom, g_profile.support[side], at);
                                Transform(glove, middle, now);
                                for (int i = 0; i < 3; ++i)
                                    glove[i * 4 + 3] += at[i] - now[i];
                            }
                            if (Invert(offset, back)) {
                                Compose(glove, back, controller);
                                Compose(fromHead, controller, h.held);
                                h.heldWhere = Where::kGuard;
                            }
                        }
                    } else {
                        // Its grip point at its side's place on the pistol grip of the rifle as drawn, turned as it last was.
                        const float point[3] = {side == 0 ? -anchor[0] : anchor[0], anchor[1], anchor[2]};
                        float at[3], last[12];
                        Transform(g_lastGunRoom, point, at);
                        if (h.on)
                            PoseMatrix(h.grip, last);
                        else
                            std::memcpy(last, base, sizeof(last));
                        last[3] = at[0], last[7] = at[1], last[11] = at[2];
                        Compose(fromHead, last, h.held);
                        h.heldWhere = Where::kGrip;
                    }
                }
                if (h.heldWhere == s.where)
                    Compose(headPose, h.held, grip);
                else if (h.on)
                    PoseMatrix(h.grip, grip);  // (not worked out yet: where it was)
                else
                    std::memcpy(grip, base, sizeof(grip));
                break;
            default:
                have = false;
                break;
            }
        }
        if (s.where != Where::kGuard && s.where != Where::kGrip && s.where != Where::kPickup)
            h.heldWhere = Where::kNone;
        if (!have || !Finite(grip, 12)) {
            h.on = false;
            continue;
        }
        h.step = s;
        h.grip = MatrixPose(grip);
        for (int i = 0; i < 3; ++i) {
            h.velocity[i] = headTurn[i * 3] * s.velocity[0] + headTurn[i * 3 + 1] * s.velocity[1] + headTurn[i * 3 + 2] * s.velocity[2];
            h.spin[i] = headTurn[i * 3] * s.spin[0] + headTurn[i * 3 + 1] * s.spin[1] + headTurn[i * 3 + 2] * s.spin[2];
        }
        h.on = true;
        // (A film round's hand not seen, or giving no place, said as it changes: the films' sheet reads the holsters' lines against it.)
        if (const int seen = s.lost ? 2 : s.unseen ? 1 : 0; seen != s_seenWas[side]) {
            s_seenWas[side] = seen;
            Log("Free hands: test %d: the %s hand %s", test, kHandName[side],
                seen == 2 ? "gives no place now (lost)" : seen == 1 ? "not seen now (its place kept, as the runtime's guess)" : "seen again");
        }
        input::SetFilmTrigger(side, test == 10 && !pickups::Tuning().testFire ? 0.0f : s.trigger);  // the round's own trigger works the game's fire button (input.cpp), so a film shows the left trigger firing
    }
}

// The game's own right hand on the gun's pistol grip (the M416's: skin::GameRightGrip; a launcher's or a held gun's: its rig's rest, skin::RigRestRight), fetched
// (a copy under skin's lock) and mirrored onto the left glove again only when it's changed, or the gun has. Not while the left glove eases off another gun's
// grip: its table stays that gun's till it's off. The first for each gun is logged with its palm against where its palms were measured to rest.
void FetchTriggerGrip(const gunprofile::Profile& p) {
    if (g_glovesOnPistol && !g_onGripBy[0] && g_gripBlend > 0.0f && g_rightGripFor != p.id)
        return;
    float bones[handpose::kRightBones][12], palm[3] = {};
    bool have = false;
    if (p.pose == gunprofile::Pose::kRifle) {
        have = skin::GameRightGrip(bones);
    } else if (p.pose == gunprofile::Pose::kLauncher) {
        uint32_t kind = 0;  // (the rest of this gun's arms, not the gun before's)
        have = skin::RigRestRight(bones, kind, palm) && kind == (p.id == gunprofile::kSmawId ? 0u : p.id);
    }
    if (!have || !Finite(&bones[0][0], handpose::kRightBones * 12))
        return;
    if (g_haveRightGrip && g_rightGripFor == p.id && std::memcmp(bones, g_gameRightGrip, sizeof(bones)) == 0)
        return;
    bool first = !g_haveRightGrip || g_rightGripFor != p.id;
    if (first) {  // (said once a gun, not at every switch back to it)
        static uint32_t s_said[16] = {};
        static int s_saidCount = 0;
        for (int i = 0; i < s_saidCount && first; ++i)
            first = s_said[i] != p.id;
        if (first && s_saidCount < static_cast<int>(std::size(s_said)))
            s_said[s_saidCount++] = p.id;
    }
    std::memcpy(g_gameRightGrip, bones, sizeof(g_gameRightGrip));
    handpose::RightToLeft(g_gameRightGrip, g_leftOnPistol);
    g_haveRightGrip = true;
    g_rightGripFor = p.id;
    if (first && p.pose == gunprofile::Pose::kLauncher && Finite(palm, 3)) {
        const float off[3] = {palm[0] - p.anchor[1][0], palm[1] - p.anchor[1][1], palm[2] - p.anchor[1][2]};
        Log("Free hands: the game's right hand on the %s's grip learned: its palm (%.3f %.3f %.3f), %.0f mm from where it rests", p.name, palm[0], palm[1], palm[2],
            Length(off) * 1000.0f);
    }
}

// The left glove's table on the gun's pistol grip (the game's own right hand there, mirrored), copied into game (bent there, never in the cache); false until it's
// been seen for this gun (the glove stays free till then; easing off the gun before's grip, that one's still), or without the left's own place for it this frame.
bool LeftOnPistol(float (*game)[12]) {
    if (!g_haveRightGrip || (g_rightGripFor != g_profile.id && !(g_glovesOnPistol && !g_leftOnGrip))) {
        static uint32_t s_said[16] = {};  // (said once a gun; not for none: dormant, the glove only eases off a grip its gun's line was said for)
        static int s_saidCount = 0;
        bool said = g_profile.id == 0;
        for (int i = 0; i < s_saidCount; ++i)
            said = said || s_said[i] == g_profile.id;
        if (!said && s_saidCount < static_cast<int>(std::size(s_said))) {
            s_said[s_saidCount++] = g_profile.id;
            if (g_profile.pose == gunprofile::Pose::kLauncher)
                Log("Free hands: the left glove stays free on the %s's pistol grip until the game's own right hand there has been seen", g_profile.name);
            else
                Log("Free hands: the left glove stays free on the pistol grip until the game's own right hand there has been seen");
        }
        return false;
    }
    if (!g_haveHandGun[0])
        return false;
    std::memcpy(game, g_leftOnPistol, sizeof(g_leftOnPistol));
    return true;
}

// The rifle can't fall (no room in loose, loose not stepping, or nowhere it was drawn): it stays in the hand that let go of it, drawn where it was (no
// frame without the rifle; let go of by its handguard alone, still frozen on that hand's grip as the hold had it), that hand unarmed. Said once a streak.
void KeepTheRifle(skin::HandTargets& t, const float* fromHead) {
    const int side = g_gunRelease.side;
    const bool fromGuard = g_gunRelease.fromGuard;
    const bool lp = g_gunRelease.pose == gunprofile::Pose::kLauncher;  // (the gun's own pose and flags: launcherPose for any but the M416)
    const char* const name = g_gunRelease.name;
    g_gunRelease = GunRelease{};
    grasp::NotLetGo(g_grasp);
    g_graspNow.holder = g_grasp.holder;
    g_graspNow.trigger = g_grasp.trigger;
    g_graspNow.barrel = g_grasp.barrel;
    g_graspNow.released = -1;
    g_graspNow.fireHand = -1;
    g_graspNow.glide = 1.0f;
    g_graspNow.glideFromWorld = false;
    g_fireHandNow = -1;
    g_holdsGunNow = true;
    g_gripHandNow = g_grasp.holder == grasp::Holder::kHeld && (g_grasp.trigger == 0 || g_grasp.trigger == 1) ? g_grasp.trigger : -1;
    auto held = [&t, lp]() {
        if (lp) {
            t.gun = t.launcherDrawn = true;
            t.launcherLying = false;
            t.gunDrawn = t.gun;
        } else {
            t.gun = t.gunDrawn = true;
            t.gunLying = false;
        }
    };
    if (fromGuard && side >= 0 && side < 2 && g_haveLastGrip[side]) {
        float onHand[12];
        Compose(g_lastGripRoom[side], g_onBarrel, onHand);
        Compose(fromHead, onHand, lp ? t.launcherPose : t.gunPose);
        held();
        g_kicked = 0;  // (its pose is the one kept now: nothing kicked is left in it)
    } else if (g_haveLastGunRoom) {
        Compose(fromHead, g_lastGunRoom, lp ? t.launcherPose : t.gunPose);
        held();
        g_kicked = 0;
    }
    if (!g_keptSaid)
        Log("Free hands: the %s stays in the %s hand (loose had no room for it)", name, side == 0 ? "left" : "right");
    g_keptSaid = true;
}

// Back in the right hand with nothing armed (Toggle): a rifle lying is taken out of loose. True if it wasn't there already.
bool ResetGrasp() {
    const bool away = !(g_graspNow.holder == grasp::Holder::kHeld && g_graspNow.trigger == 1);
    if (g_gunLoose.id)
        loose::Remove(g_gunLoose.id);
    g_gunLoose = GunLoose{};
    g_gunRelease = GunRelease{};
    g_gunLost = false;
    grasp::Clear(g_grasp);
    g_graspNow = grasp::Result{};
    g_fireHandNow = 1;
    g_holdsGunNow = true;
    g_gripHandNow = 1;
    g_aimOwn = false;
    g_easeFor = kAimEase;
    g_aimHolderWas = grasp::Holder::kHeld, g_aimTriggerWas = 1, g_aimBarrelWas = -1, g_aimGunWas = true;  // (no ease from an aim of before)
    g_regripDrawn = false;
    g_keptAt = 0, g_keptSaid = false;
    g_supportingBy[1] = false;
    // The right's own work with it gone too: off the launcher's grip and the charging handle, nothing round a magazine, its collision afresh; the launcher's
    // hand holding the rifle the right again. Off a launcher-pose gun's front grip; a switch after this never brings a gun back to the left.
    g_onM320By[1] = false;
    g_onLauncherBy[1] = false;
    g_lastHeldSide = -1;
    g_leverBy[1] = 0.0f;
    g_magFingersBy[1] = 0.0f;
    g_dropOpenState[1] = 0;
    g_dropOpenBy[1] = 0.0f;
    collide::Reset(g_collideBy[1]);
    g_m320GunHand = 1;
    ResetPick();
    return away;
}

// Update has run within the last second, or (the stereo mode) it's only paused: the headset not rendering, its views not located. Then the flags hold
// as last published till it runs again: input.cpp reads them before the first Update back, so a rifle lying stays unfired and untaken that frame too.
// Otherwise (the theater mode: Update stopped) as before: the right trigger, and the rifle in hand.
bool FreshFreeHands() {
    const ULONGLONG at = g_freeHandsAt.load(std::memory_order_relaxed);
    return at && (GetTickCount64() - at < 1000 || config::Get().mode == config::Mode::Stereo);
}

// With both hands tracked: takes hold, holds or lets go, and when holding gives the steer (steer.h,
// applied in Update) the gun's line to the hold point and the line between the hands, both in the
// gun's model space as the right hand alone holds it, so a swing of both hands, a recentre or the head
// turning leave the steer as it is (the glove goes onto the handguard in Update). gunTurn: the gun's
// turn as the right hand alone points it (view space, model -> view); anchor: the right hand's place on
// the gun (model space).
// ---- Loose things let go of (loose.h): each frame's Frame for them (the room, the gun, the left hand, the ground), kept for their starts (skin.cpp), and their step. ----
loose::Frame g_looseFrame;
bool g_haveLooseFrame = false;

// The id free hands' profile will have for a gun, by its blueprint's name (a pickup's content), before it's in hand: the M416's, the launcher's, a held
// gun's colour draw. 0 for one they don't know: no draw of the mod's, no body for it, "guns m416", or launcherhands 0. noRow: the guns' table has no
// row at all for that name, so what it is can't be told until it's drawn. From recoilguns.h's rows (written from the game's own files) and
// gunbodies.h: nothing typed here.
// (By the gun's row in the guns' table itself: holsters ask it of each slot's row every frame, ammo::RowInSlot, with no walk of the table.)
uint32_t ProfileIdOfRow(const recoilguns::Row* row) {
    if (!row || !row->body || row->body == recoilrun::kTrainPistol)
        return 0;
    if (row->body == gunprofile::kRifleId)
        return row->body;
    if (!freetune::EveryGun() || !skin::LauncherHands())
        return 0;
    const gunbodies::Body* body = row->body == gunprofile::kSmawId ? gunbodies::For("smaw") : gunbodies::For(row->shape);
    return body && body->spec ? row->body : 0;
}

uint32_t ProfileIdFor(const char* blueprint, bool* noRow = nullptr) {
    const recoilguns::Row* row = recoilguns::ByName(blueprint);
    if (noRow)
        *noRow = row == nullptr;
    return ProfileIdOfRow(row);
}

// The player's slot the gun in hand is in, as the game has it. The M416 and a held gun (the profile's slot 0): ammo's slot of the gun in the hands
// (ammo::HandSlot), the one its counts went to. The SMAW, the Stinger and the AT4 (every launcher by its row's kind): the slot whose blueprint is this
// gun's own (recoilguns.h's row for the profile's id), by name; else the profile's own slot (the tick not fresh).
int SlotOfProfile(const gunprofile::Profile& p) {
    const recoilguns::Row* row = recoilguns::ByBody(p.id);
    if (p.slot == 0 && p.id != gunprofile::kSmawId && !(row && recoil::Launcher(row->kind)))
        return ammo::HandSlot();
    const int slot = row ? pickups::SlotOf(row->blueprint) : -1;
    return slot >= 0 ? slot : p.slot;
}

void ResetPick() {
    g_pick = PickState{};
    g_pick.swapSeen = pickups::NewestSwap();  // (old swaps are never read again)
    g_reachCount = 0;
}

// A hand's take landed (NoteSwaps) and this profile is its gun: the one its blueprint names (landedId), or, the guns' table having no row for that
// name, any gun but the one in hand when its swap was seen (landedAny).
bool PickArrives(uint32_t id) {
    return g_pick.landedSide >= 0 && (g_pick.landedAny ? id != 0 && id != g_pick.wasId : id != 0 && id == g_pick.landedId);
}

// This Update's dropped guns a hand may take (pickups::Reachable), in the room, made once an Update (a test's hand and FreeHands share them): only while
// the player's gun lies or is put away at its place (holsters: both hands empty), free hands on, no take waiting, no arrival due and the lying gun not
// retired. How many.
int PickupCandidates(grasp::Candidate* out) {
    if (g_reachSerial != g_updateSerial) {
        g_reachSerial = g_updateSerial;
        g_reachCount = 0;
        float armsToRoom[12];
        const bool handsEmpty = g_graspNow.holder == grasp::Holder::kLoose || g_graspNow.holder == grasp::Holder::kStowed;
        if (g_freeOn && handsEmpty && pickups::Hooked() && !g_pick.request && g_pick.landedSide < 0 && !g_pick.lyingRetired &&
            g_haveLooseFrame && Invert(g_looseFrame.roomToArms, armsToRoom)) {
            const float eye[3] = {g_looseFrame.eye[0] + static_cast<float>(g_looseFrame.origin[0]), g_looseFrame.eye[1] + static_cast<float>(g_looseFrame.origin[1]),
                                  g_looseFrame.eye[2] + static_cast<float>(g_looseFrame.origin[2])};
            int n = pickups::Reachable(eye, 3.0f, g_reach, grasp::kMostCandidates);
            if (!pickups::Tuning().takeUnknown) {  // ("unknown 0": a gun free hands don't know isn't a hand's to take)
                int kept = 0;
                for (int i = 0; i < n; ++i)
                    if (ProfileIdFor(g_reach[i].name))
                        g_reach[kept++] = g_reach[i];
                n = kept;
            }
            g_reachCount = n;
            for (int i = 0; i < g_reachCount; ++i) {  // (one not finite stays in its place: the grasp skips it, the indexes stay the same)
                pickrules::WorldToRoom(armsToRoom, g_looseFrame.origin, g_reach[i].drawn, g_reachRoom[i].room);
                std::memcpy(g_reachRoom[i].boxMin, g_reach[i].boxMin, sizeof(g_reachRoom[i].boxMin));
                std::memcpy(g_reachRoom[i].boxMax, g_reach[i].boxMax, sizeof(g_reachRoom[i].boxMax));
            }
        }
    }
    for (int i = 0; i < g_reachCount; ++i)
        out[i] = g_reachRoom[i];
    return g_reachCount;
}

// Where an arrival starts (model -> room). pickups.txt "glidefrom 1": the pickup's drawn pose itself. Else, and always for a scene's gift (a prompt's:
// nothing of it was drawn lying, its middle the prompt's point): turned as that hand will hold it, its box's middle on the middle of the pickup's box
// where it lay. False while the room isn't known or a number isn't finite.
bool PickStart(int side, const gunprofile::Profile& p, const float* fromHead, float* out12) {
    if (g_pick.giveBack) {
        // (A draw from a shoulder: in the hand that drew it from the first frame, as that hand will hold it; nothing flies from the pickup on the ground.)
        float headPose[12];
        InvertRigid(fromHead, headPose);
        Compose(headPose, g_handGun[side], out12);
        return Finite(out12, 12);
    }
    float armsToRoom[12];
    if (!g_haveLooseFrame || !Invert(g_looseFrame.roomToArms, armsToRoom))
        return false;
    if (pickups::Tuning().glideFromDrawn && !g_pick.byUse) {
        pickrules::WorldToRoom(armsToRoom, g_looseFrame.origin, g_pick.landedWorld, out12);
        return Finite(out12, 12);
    }
    float headPose[12], handRoom[12], centre[3];
    InvertRigid(fromHead, headPose);
    Compose(headPose, g_handGun[side], handRoom);  // (that hand's own place for the gun: made before any kick)
    pickrules::WorldPointToRoom(armsToRoom, g_looseFrame.origin, g_pick.centreWorld, centre);
    return pickrules::StartAtCentre(handRoom, p.boxMin, p.boxMax, centre, out12);
}

// The swaps since last frame and the answer to the take asked (FreeHands, before the change of gun is looked at). What a swap means is
// pickrules::OnSwap's to say, and the net is pickrules::Net's (both pure: picktest runs them). This gives them what they need, applies their
// verdicts and says the lines.
void NoteSwaps() {
    const ULONGLONG now = GetTickCount64();
    // (Put away at its place, nothing of the gun lies: a swap that takes its slot retires no copy, and the net never sends it back to a hand; a take
    // lands, is given up, or comes into the hand as with a gun in the hands: picktest T22.)
    const bool lies = g_gunLoose.id != 0 && g_grasp.holder == grasp::Holder::kLoose;
    pickups::Swap swap;
    while (pickups::NextSwap(g_pick.swapSeen, swap)) {
        g_pick.swapSeen = swap.serial;
        hands::HeldChambered(false);  // (a gun the game gave is chambered. Never true: that clears loose and the grasp)
        const int changed = swap.changed < pickups::kMostSlots ? swap.changed : pickups::kMostSlots;
        // (Holsters: every slot a take off the ground changed, a hand's or the game's own R: its new gun goes on a shoulder only by a hand.)
        for (int k = 0; k < changed; ++k)
            if (swap.slot[k] >= 0 && swap.slot[k] < holster::kSlots)
                g_holsterTaken |= 1u << swap.slot[k];
        // (Flow (b)'s record of a lying gun a scene's gift took, its slot's change a take's after all, told a moment late: the lying gun went into that
        // pickup, where the game keeps it and draws it; nothing of it to lay down again.)
        if (g_lyingGone.on && now - g_lyingGone.at <= 1500)
            for (int k = 0; k < changed; ++k)
                if (swap.slot[k] == g_lyingGone.slot) {
                    Log("Free hands: the %s's slot changed by a take of a pickup just after: it lies in that pickup, the game's way (not laid down again)",
                        g_lyingGone.name);
                    g_lyingGone = LyingGone{};
                    break;
                }
        // (Holsters: what it changed, before Arrive and Update hear its marks: a holstered gun that went into the pickup stays on its shoulder, in that
        // pickup; an away gun out of this very pickup is back: holster.h's NoteSwap.)
        {
            holster::SlotIn slots[holster::kSlots];
            HolsterSlots(slots);
            holster::SwapIn in;
            in.changed = changed < holster::kSlots ? changed : holster::kSlots;
            for (int k = 0; k < in.changed; ++k) {
                in.slot[k] = swap.slot[k];
                in.came[k] = holster::NameKey(swap.came[k]);
                in.went[k] = holster::NameKey(swap.went[k]);
                in.wentBlueprint[k] = swap.wentBlueprint[k];
                // (The same weapon kept, its firing state and blueprint as before: a take of its rounds alone. A gun of the same kind for it is a new
                // weapon, a swap like any other: review finding 7.)
                in.same[k] = swap.cameFiring[k] && swap.cameFiring[k] == swap.wentFiring[k] && swap.cameBlueprint[k] == swap.wentBlueprint[k];
            }
            in.entity = swap.entity;
            in.removed = swap.removed;
            in.noPlace = swap.keepsNoPlace;
            in.all = swap.replaceAll;
            holster::CopyName(in.taken, pickups::Nice(swap.taken.name));
            holster::NoteSwap(g_holsterSet, slots, in, g_holster);
        }
        // X held's own pickup (the game's R held for it at the swap, or let go a moment before it was told: review finding 10, never another take the
        // game made) of a gun into a slot the game doesn't raise: said, and its key once R is let go.
        if (!swap.byHand && g_holsterSet.on && !swap.forceSlot && input::PickupRSince() <= kPickupRAfterMs) {
            int landed = -1;
            for (int k = 0; k < changed && landed < 0; ++k)
                if (swap.came[k][0] && recoilguns::SameName(swap.came[k], swap.taken.name))
                    landed = swap.slot[k];
            if (landed >= 0 && holster::KeyOf(landed) != 0 && ammo::GameSlot() != landed) {
                g_raiseAfterR = landed, g_raiseAfterRAt = now;
                strncpy_s(g_raiseAfterRName, pickups::Nice(swap.taken.name), _TRUNCATE);
                Log("Pickups: X held's pickup put the %s in slot %d and the game didn't raise it: on no shoulder; its key once R is let go, so it comes "
                    "into the right hand", g_raiseAfterRName, landed);
            }
        }
        // (Recoil: the gun it last had, if this swap put another gun in its slot, is no gun for it while it's still drawn: the recoil block.)
        for (int k = 0; k < changed; ++k)
            if (g_kickId && swap.slot[k] == g_kickSlot && swap.cameBlueprint[k] != swap.wentBlueprint[k])
                g_pick.kickOutId = g_kickId, g_pick.kickOutAt = now;
        pickrules::SwapSeen s;
        s.lies = lies, s.lyingSlot = g_gunLoose.slot, s.releasedAt = g_gunLoose.releasedAt, s.retired = g_pick.lyingRetired;
        s.at = swap.at, s.changed = changed, s.slot = swap.slot, s.wentBlueprint = swap.wentBlueprint;
        s.takenBlueprint = swap.taken.blueprint, s.takenSlot = swap.taken.slot, s.forceSlot = swap.forceSlot;
        s.byHand = swap.byHand, s.request = swap.request, s.side = swap.side, s.waiting = g_pick.request;
        s.knownId = freetune::On() ? ProfileIdFor(swap.taken.name, &s.noRow) : 0;
        const pickrules::SwapVerdict v = pickrules::OnSwap(s);
        if (v.retire) {
            g_pick.lyingRetired = true;
            g_pick.retiredAt = now;
            Log("Free hands: the %s that lay on the ground went into the pickup the game swapped it into (where the %s lay)", g_gunLoose.name,
                pickups::Nice(swap.taken.name));
        }
        if (v.answered)
            g_pick.request = 0;
        if (v.landedSide >= 0) {
            g_pick.landedSide = v.landedSide, g_pick.landedId = v.landedId, g_pick.landedAny = v.landedAny, g_pick.sameKind = v.sameKind;
            g_pick.wasId = g_profileWas, g_pick.freed = v.freedAtOnce, g_pick.landedSlot = swap.taken.slot, g_pick.byUse = false;
            for (int k = 0; k < changed; ++k)  // (where it went: the slot that came to hold its name, not the one its entry names)
                if (swap.came[k][0] && recoilguns::SameName(swap.came[k], swap.taken.name)) {
                    g_pick.landedSlot = swap.slot[k];
                    break;
                }
            g_pick.landedAt = now, g_pick.landedFrames = 0;
            g_pickTookAt = now;
            pickups::SwapDrawn(swap, g_pick.landedWorld);
            // A gun taken off the ground stays in the hand that took it, never on a shoulder by itself (the user, 2026-11-18): one the game put in another
            // slot and doesn't raise (not forced) is raised by its key, so its arrival (still due) brings it into that hand; till then, and if the game
            // never raises it, it's in its slot on no shoulder (the D-pad or Y brings it).
            if (v.freedAtOnce && g_holsterSet.on && g_pick.landedSlot >= 0 && holster::KeyOf(g_pick.landedSlot) != 0 &&
                ammo::GameSlot() != g_pick.landedSlot) {
                const bool sent = input::SelectSlot(g_pick.landedSlot);
                Log("Pickups: the game put the %s in slot %d without raising it: %s, so it comes into the %s hand", pickups::Nice(swap.taken.name),
                    g_pick.landedSlot, sent ? "its key pressed" : "its key couldn't be sent (it stays in the slot on no shoulder)",
                    kHandName[v.landedSide]);
            }
        }
        if (v.refused)
            g_pick.refusedNow = true;  // (the take waiting is over. A retired copy stays till the old gun's draws have stopped: today's paths, or the net)
        if (v.unknown)
            Log("Pickups: the %s came as the game holds it: free hands don't know it yet (the right trigger fires it)", pickups::Nice(swap.taken.name));
        if (v.swappedAway)
            g_pick.swappedAwayNow = true;  // (the game's own pickup of the same gun: no change of gun will say so)
    }
    if (g_pick.request) {
        const char* why = "";
        if (pickups::Outcome(g_pick.request, &why) < 0) {  // (its line is pickups.cpp's)
            if (g_pick.giveBack)
                holster::GiveBackFailed(g_holster, g_pick.side, why);  // (a draw from a shoulder: the gun stays there, in its pickup; said, felt)
            g_pick.request = 0;
            g_pick.giveBack = false;
            g_pick.refusedNow = true;
        }
    }
    // X held's pickup into a slot the game didn't raise: its key once R is let go (a moment on), unless the game raised it meanwhile or it's too late.
    if (g_raiseAfterR >= 0) {
        const unsigned long long rSince = input::PickupRSince();
        if (ammo::GameSlot() == g_raiseAfterR) {
            g_raiseAfterR = -1;
        } else if (now - g_raiseAfterRAt > 3000) {
            Log("Pickups: the %s stays in slot %d (R held past 3 s): on no shoulder, the D-pad or Y brings it", g_raiseAfterRName, g_raiseAfterR);
            g_raiseAfterR = -1;
        } else if (rSince != 0 && rSince != ~0ULL && rSince >= 250) {
            const bool sent = input::SelectSlot(g_raiseAfterR);
            Log("Pickups: the %s raised from slot %d (X held's pickup): %s", g_raiseAfterRName, g_raiseAfterR,
                sent ? "its key pressed" : "its key couldn't be sent (on no shoulder: the D-pad or Y brings it)");
            g_raiseAfterR = -1;
        }
    }
    // The net: a landed take whose gun isn't in the hand 1.5 s on; a gun on the ground retired 1.5 s ago with nothing come of it.
    const pickrules::NetVerdict n = pickrules::Net(lies, g_pick.lyingRetired, g_pick.retiredAt, g_pick.landedSide, g_pick.landedAt, now);
    if (n.landedOver) {
        if (g_pick.byUse)
            Log("Use: the %s the scene gave isn't in the %s hand 1.5 s on: it's as the game holds it", g_pick.name, kHandName[g_pick.landedSide]);
        else if (g_pick.freed)
            Log("Pickups: the %s went into slot %d; the game left your gun in hand: pick it with Y once a gun is in your hand", pickups::Nice(g_pick.name),
                g_pick.landedSlot);
        else
            Log("Pickups: the %s isn't in the %s hand 1.5 s on: it's where the game has it (in its slot: pick it with Y once a gun is in your hand)",
                pickups::Nice(g_pick.name), kHandName[g_pick.landedSide]);
        g_pick.landedSide = -1;
        g_pick.giveBack = false;
    }
    if (n.refused)
        g_pick.refusedNow = true;
    if (n.swappedAway)
        g_pick.swappedAwayNow = true;
    if (n.retiredOver)
        g_pick.lyingRetired = false;  // (its copy went some other way: nothing to give back)
}
// skin.cpp's LooseKind: which draw each is drawn with.
constexpr int kLooseM320Case = 3, kLooseSmawCase = 4;
constexpr int kLooseShell = 6;  // a shotgun's live shell let go of or thrown out by its bolt (shellload.h), drawn with the gun's own shell draw
// Starts asked for this frame, made in StepLoose with this frame's Frame: where it is (its bone into the room), its speed there, and whether a hand let go of it (loose's
// own reckoning of that hand's speed and turn then) or it slid off (its speed as given, a slow tumble).
struct PendingLoose {
    int kind = 0;
    const loose::Spec* spec = nullptr;
    float room[12] = {};
    float velocityRoom[3] = {};
    bool fromHand = false;
    int rounds = 0;
    int side = 0;  // the hand that let go of it (fromHand): 0 the left, 1 the right (free hands)
};
PendingLoose g_pendingLoose[4];
int g_pendingLooseCount = 0;

void QueueLoose(int kind, const loose::Spec* spec, const float* room, const float* velocityRoom, bool fromHand, int rounds = 0, int side = 0) {
    if (!spec || g_pendingLooseCount >= static_cast<int>(std::size(g_pendingLoose)))
        return;
    PendingLoose& p = g_pendingLoose[g_pendingLooseCount++];
    p.kind = kind;
    p.spec = spec;
    std::memcpy(p.room, room, sizeof(p.room));
    std::memcpy(p.velocityRoom, velocityRoom, sizeof(p.velocityRoom));
    p.fromHand = fromHand;
    p.rounds = rounds;
    p.side = side == 1 ? 1 : 0;
}

// The ground under a place: the game's own, from its ray cast against its own world (ground.cpp: the terrain, buildings, crates, cars), firm; till that's known there, the
// soldier's feet, the game's eye less the stance's eye height (1.6 m standing, 1.0 crouched, 0.3 prone): a guess, not firm, it lands things but never moves one lying.
int SoldierFloor(void*, const float* at, loose::Plane* planes, int most) {
    float eye[3];
    if (most < 1)
        return 0;
    {
        float origin[3];
        for (int i = 0; i < 3; ++i)
            origin[i] = static_cast<float>(g_looseFrame.origin[i]);
        ground::Plane g[1];
        if (ground::Planes(at, origin, g, 1) == 1 && g[0].source != ground::kFeet && g[0].source != ground::kNone && g[0].source != ground::kNothing) {
            loose::Plane& p = planes[0];
            std::memcpy(p.point, g[0].point, sizeof(p.point));
            std::memcpy(p.normal, g[0].normal, sizeof(p.normal));
            p.surface = loose::kDirt;  // (the game's material index by surface: to be mapped)
            p.firm = g[0].source == ground::kRay;
            return 1;
        }
    }
    if (!skin::GameEyeWorld(eye))
        return 0;
    loose::Plane& p = planes[0];
    p.point[0] = at[0];
    p.point[1] = eye[1] - input::StanceEyeHeight() - static_cast<float>(g_looseFrame.origin[1]);
    p.point[2] = at[2];
    p.normal[0] = 0.0f, p.normal[1] = 1.0f, p.normal[2] = 0.0f;
    p.surface = loose::kDirt;
    p.firm = false;
    return 1;
}

// The game's own reload of the grenade launcher (X held on its grip; the user, 2026-11-05: "I want the original reload that the game has to work", and the spent case "to hit the
// floor ... stay there for a minute"): its animation pulls the old round out along the open barrel by the rim and takes it down to the hip, where the same mesh comes back up as
// the new one, never hidden (build\survey\M320_vanilla.bin: pulled 5.23-5.67 s, at the hip 5.72-6.19 s, the new one in from 6.28 s). Here the old one is let go of as the hand
// turns down to the hip: it falls to the ground and lies there (loose, as the case by hand), and the round is drawn nowhere until the hand comes back up with the new one.
// barrelEmpty: the reload by hand had emptied the barrel already (its case tipped out, pulled out, or a round called off on its way home): the game's animation then pulls out a
// round that isn't there, so nothing falls, and the round stays hidden from the start until the new one comes up (two cases for one shot, the review 2026-11-05).
void GameReloadCase(const skin::HandTargets& t, const float* headPose, float dt, bool barrelEmpty) {
    static int s_stage = 0;  // 0 not reloading, 1 in the barrel, 2 pulled out of it, 3 let go of (hidden), 4 the new one shown
    static float s_seat[3] = {}, s_last[3] = {}, s_lowest = 0.0f, s_hiddenFor = 0.0f, s_since = 0.0f;
    static bool s_haveLast = false, s_empty = false;
    const int state = ammo::M320State();
    // (Its firing state's word keeps the state a put-away came from, so a reload put away part way could read on: the hidden round's time limit in stage 3 below.)
    if (!(state == 10 || state == 11 || input::M320GameReloading())) {
        if (s_stage != 0)
            skin::SetM320CaseDropped(false);
        s_stage = 0;
        s_empty = false;
        s_haveLast = false;
        s_since = 0.0f;
        return;
    }
    if (s_stage <= 1 && barrelEmpty && !s_empty) {  // (latched in its first frames: a round called off on its way home shows a frame late)
        s_empty = true;
        skin::SetM320CaseDropped(true);
    }
    // Longer than any reload (the game's is 2.9 s): the round shown again whatever (a stale reload state must never hide it for good).
    static float s_watchFor = 0.0f;
    s_watchFor = s_stage == 0 ? 0.0f : s_watchFor + dt;
    if (s_stage != 4 && s_watchFor > 6.0f) {
        skin::SetM320CaseDropped(false);
        s_stage = 4;
        Log("M320: the game's own reload read on for 6 s: the round shown again");
    }
    s_since += dt;  // (skipped frames counted: a gap's whole move isn't one frame's speed)
    float rel[12];
    if (!t.gun || !skin::M320RoundOnGun(rel))
        return;
    constexpr float kRoundMiddle[3] = {0.0003f, -0.0584f, 0.339f};  // on its bone: kM320RoundToCase's base, half its 11.9 cm along it
    float mid[3], velocity[3] = {};  // on the gun; m/s
    Transform(rel, kRoundMiddle, mid);
    if (s_haveLast && s_since > 1e-4f)
        for (int i = 0; i < 3; ++i)
            velocity[i] = (mid[i] - s_last[i]) / s_since;
    std::memcpy(s_last, mid, sizeof(s_last));
    s_haveLast = true;
    s_since = 0.0f;
    switch (s_stage) {
    case 0:  // the reload's first frame: where it sits
        std::memcpy(s_seat, mid, sizeof(s_seat));
        s_stage = 1;
        break;
    case 1:  // out of the barrel once pulled back more than its length
        if (s_seat[2] - mid[2] > 0.13f)
            s_stage = 2;
        break;
    case 2:  // the hand turning down to the hip with it, or well back: let go of
        if ((mid[1] < s_seat[1] - 0.04f && velocity[1] < -0.05f) || s_seat[2] - mid[2] > 0.26f) {
            if (s_empty) {  // (nothing in it to let go of: hidden on till the new one comes up)
                Log("M320: the game's own reload pulled out nothing (the barrel was emptied by hand): no case falls");
            } else {
                if (!skin::LooseOn() || !g_haveLooseFrame) {
                    s_stage = 4;  // (loose off: the game's own animation as it is)
                    break;
                }
                float gunRoom[12], roundRoom[12], caseOnRound[12], caseRoom[12], v[3];
                Compose(headPose, t.gunPose, gunRoom);
                Compose(gunRoom, rel, roundRoom);
                skin::M320CaseOnRound(caseOnRound);
                Compose(roundRoom, caseOnRound, caseRoom);
                for (int i = 0; i < 3; ++i)  // its own speed on the gun, into the room's axes (loose adds the gun's own)
                    v[i] = gunRoom[i * 4] * velocity[0] + gunRoom[i * 4 + 1] * velocity[1] + gunRoom[i * 4 + 2] * velocity[2];
                const float speed = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
                if (speed > 3.0f)  // (a drop's most, as the other drops have it: logged ones were 0.6-0.9 m/s)
                    for (float& c : v)
                        c *= 3.0f / speed;
                QueueLoose(kLooseM320Case, skin::M320CaseSpec(), caseRoom, v, false, 0);
                skin::SetM320CaseDropped(true);
                Log("M320: the game's own reload let go of the old case %.0f cm back and %.0f cm down from where it sat, at %.2f m/s: it falls (loose)",
                    (s_seat[2] - mid[2]) * 100.0f, (s_seat[1] - mid[1]) * 100.0f, std::fmin(speed, 3.0f));
            }
            s_lowest = mid[1];
            s_hiddenFor = 0.0f;
            s_stage = 3;
        }
        break;
    case 3:  // hidden till the hand comes back up from the hip with the new one (or 2 s: a reload put away part way, its state read on)
        s_hiddenFor += dt;
        s_lowest = std::fmin(s_lowest, mid[1]);
        if ((s_hiddenFor > 0.15f && mid[1] > s_lowest + 0.04f) || s_hiddenFor > 2.0f) {
            skin::SetM320CaseDropped(false);
            s_stage = 4;
            Log("M320: the game's own reload brings the new round up from the hip (%.2f s after the old one went)", s_hiddenFor);
        }
        break;
    default:
        break;
    }
}

// headPose: the head in the room; fromHead: the room into the head; gunPose: the gun's frame bone into the head (the steered one, as drawn).
// Free hands: the rifle let go of starts here (or stays in the hand: KeepTheRifle), and the rifle lying is drawn where loose has it (t).
void StepLoose(skin::HandTargets& t, bool gun, const float* gunPose, const float* headPose, const float* fromHead, float dt, bool trackedLeft) {
    // (The ground's eye and its frame: xr_runtime.cpp, every frame, hands or not.)
    float seenFrom[12], seenOrigin[3];
    if (!skin::LooseOn() || !skin::LatestSeenFrom(seenFrom, seenOrigin)) {
        g_pendingLooseCount = 0;
        if (g_gunRelease.want && g_gunRelease.still) {
            Log("Free hands: the %s couldn't be laid where it lay (nothing lies on the ground now): back in the right hand", g_gunRelease.name);
            g_gunRelease = GunRelease{};
            g_gunLost = true;  // (no hand's let-go to undo: as a lying gun the ground lost)
        } else if (g_gunRelease.want) {
            KeepTheRifle(t, fromHead);  // (nothing to fall with)
            g_keptAt = GetTickCount64();
        }
        if (t.gunLying)
            t.gunLying = t.gunDrawn = false;  // (not drawn while loose isn't stepping: it's back in the right hand next frame)
        if (t.launcherLying)
            t.launcherLying = t.launcherDrawn = false;  // (any other gun alike)
        return;
    }
    loose::Frame& f = g_looseFrame;
    f.dt = dt;
    Compose(seenFrom, fromHead, f.roomToArms);  // the room -> the head -> the arms' space, as the world's picture has it
    for (int i = 0; i < 3; ++i)
        f.origin[i] = static_cast<int>(std::lround(seenOrigin[i]));
    // The player's walking: how the room moves in the world at the head since the last frame, the head's own place in the room held still (so neither a lean nor a
    // turn of the stick about the head counts); over 10 m/s it's a respawn or a load, not a walk.
    {
        static float s_lastRoomToArms[12] = {};
        static int s_lastOrigin[3] = {};
        static bool s_haveLast = false;
        const float h[3] = {headPose[3], headPose[7], headPose[11]};
        float now[3], before[3];
        Transform(f.roomToArms, h, now);
        Transform(s_lastRoomToArms, h, before);
        float speed2 = 0.0f;
        for (int i = 0; i < 3; ++i) {
            f.walk[i] = s_haveLast && dt > 1e-4f ? ((now[i] + static_cast<float>(f.origin[i])) - (before[i] + static_cast<float>(s_lastOrigin[i]))) / dt : 0.0f;
            speed2 += f.walk[i] * f.walk[i];
        }
        if (!(speed2 <= 100.0f))
            f.walk[0] = f.walk[1] = f.walk[2] = 0.0f;
        std::memcpy(s_lastRoomToArms, f.roomToArms, sizeof(s_lastRoomToArms));
        std::memcpy(s_lastOrigin, f.origin, sizeof(s_lastOrigin));
        s_haveLast = true;
    }
    f.gun = gun;
    if (gun)
        Compose(headPose, gunPose, f.gunRoom);
    XrPosef grip{};
    f.hand = trackedLeft && input::HandPose(0, grip);
    f.handSensed = f.hand && input::HandVelocity(0, f.handVelocity, f.handSpin);  // (a throw by its controller's own sensors)
    if (f.hand) {
        float m[9];
        QuatToMatrix(grip.orientation, m);
        const float at[3] = {grip.position.x, grip.position.y, grip.position.z};
        for (int i = 0; i < 3; ++i)
            f.handRoom[i * 4] = m[i * 3], f.handRoom[i * 4 + 1] = m[i * 3 + 1], f.handRoom[i * 4 + 2] = m[i * 3 + 2], f.handRoom[i * 4 + 3] = at[i];
    }
    // Both hands alike (free hands: the rifle thrown by either; the left's as above, or the test's hand): its grip in the room, its own sensors.
    for (int side = 0; side < 2; ++side) {
        loose::HandAt& h = f.hands[side];
        XrPosef p{};
        h.tracked = HandPoseOf(side, p);
        if (h.tracked)
            PoseMatrix(p, h.room);
        h.sensed = h.tracked && HandVelocityOf(side, h.velocity, h.spin);
        if (!h.sensed)
            for (int i = 0; i < 3; ++i)
                h.velocity[i] = h.spin[i] = 0.0f;
    }
    for (int i = 0; i < 3; ++i) {
        f.eye[i] = seenFrom[i * 4 + 3];
        f.look[i] = -seenFrom[i * 4 + 2];  // (the head looks down its -z)
    }
    f.floor = &SoldierFloor;
    f.floorUser = nullptr;
    g_haveLooseFrame = true;
    // This frame's starts (the launcher's case or rocket let go of, or slid off its back).
    for (int k = 0; k < g_pendingLooseCount; ++k) {
        const PendingLoose& p = g_pendingLoose[k];
        float bone[12], velocity[3];
        Compose(f.roomToArms, p.room, bone);
        for (int i = 0; i < 3; ++i)
            velocity[i] = f.roomToArms[i * 4] * p.velocityRoom[0] + f.roomToArms[i * 4 + 1] * p.velocityRoom[1] + f.roomToArms[i * 4 + 2] * p.velocityRoom[2];
        float across[3] = {bone[0], bone[4], bone[8]};  // a slow tumble about an axis across it, as its own fall had
        const float l = std::sqrt(across[0] * across[0] + across[1] * across[1] + across[2] * across[2]);
        if (l > 1e-6f)
            across[0] /= l, across[1] /= l, across[2] /= l;
        // (Let go of by the right hand, free hands: that hand's own speed and turn, ReleaseBy; by the left, as before.)
        int id = 0;
        if (p.fromHand && p.side == 1)
            id = f.hands[1].tracked ? loose::ReleaseBy(*p.spec, p.kind, bone, p.rounds, f, 1) : loose::Flick(*p.spec, p.kind, bone, p.rounds, velocity, across, 1.4f, f);
        else
            id = p.fromHand && f.hand ? loose::Release(*p.spec, p.kind, bone, p.rounds, f) : loose::Flick(*p.spec, p.kind, bone, p.rounds, velocity, across, 1.4f, f);
        Log("Loose: %s %s (%d)", p.spec->name, p.fromHand ? "let go of" : "slid off", id);
    }
    g_pendingLooseCount = 0;
    // Free hands: the gun let go of this frame, with that hand's own speed and turn (not through the queue above: its four places and its loose-off
    // discard could lose a gun), its own body (the launcher's with its case on or bare as it is this frame). No room for it, or no body: it stays in the hand.
    if (g_gunRelease.want) {
        float bone[12];
        if (g_gunRelease.still) {
            // (Laid down where it lay before a scene took it: its pose then, in this frame's arms' space.)
            std::memcpy(bone, g_gunRelease.stillBone, sizeof(bone));
            for (int i = 0; i < 3; ++i)
                bone[i * 4 + 3] += static_cast<float>(g_gunRelease.stillOrigin[i] - f.origin[i]);
        } else {
            Compose(f.roomToArms, g_gunRelease.room, bone);
        }
        const loose::Spec* body = g_gunRelease.body;
        if (g_gunRelease.id == gunprofile::kSmawId)
            if (const gunbodies::Body* b = gunbodies::For(g_smawCaseOnNow ? "smaw" : "smaw_bare"); b && b->spec)
                body = b->spec;
        const int id = !body || !Finite(bone, 12)  ? 0
                       : g_gunRelease.still        ? loose::Place(*body, kLooseGun, bone, g_gunRelease.rounds, f)
                                                   : loose::ReleaseBy(*body, kLooseGun, bone, g_gunRelease.rounds, f, g_gunRelease.side);
        if (id) {
            if (g_gunRelease.still) {
                Log("Free hands: the %s lies again where it lay before the scene took it (loose #%d): take it off the floor by hand", g_gunRelease.name, id);
                Log("Loose: %s laid down where it lay (%d)", body->name, id);
            } else {
                const char* side = g_gunRelease.side == 0 ? "left" : "right";
                if (g_gunRelease.fromGuard)
                    Log("Free hands: the %s hand let go of the %s's %s: it falls (loose #%d)", side, g_gunRelease.name, g_gunRelease.front, id);
                else
                    Log("Free hands: the %s hand let go of the %s's pistol grip: it falls (loose #%d)", side, g_gunRelease.name, id);
                Log("Loose: %s let go of (%d)", body->name, id);
            }
            g_gunLoose = GunLoose{};
            g_gunLoose.id = id;
            g_gunLoose.pose = g_gunRelease.pose;
            g_gunLoose.name = g_gunRelease.name;
            g_gunLoose.sound = g_gunRelease.sound;
            g_gunLoose.slot = g_gunRelease.slot;
            g_gunLoose.releasedAt = GetTickCount64();
            g_gunRelease = GunRelease{};
            g_keptAt = 0, g_keptSaid = false;
        } else if (g_gunRelease.still) {
            // (No room among the loose things, or no body: as a lying gun the ground lost, back in the right hand next frame; never nowhere.)
            Log("Free hands: the %s couldn't be laid where it lay (no room or no body for it): back in the right hand", g_gunRelease.name);
            g_gunRelease = GunRelease{};
            g_gunLost = true;
        } else {
            KeepTheRifle(t, fromHead);
            g_keptAt = GetTickCount64();
        }
    }
    const loose::Result& r = loose::Step(f);
    skin::SetLoose(r, f.origin);
    // The rifle lying (or flying): where loose has it now, drawn from the world (skin.cpp's ToArms: no judder as the head turns); gone (no ground for 5 s, its
    // numbers broken), it's back in the right hand next frame, drawn lying where it last was this frame (not the game's own rifle at its hold for it). Any
    // other gun alike, on its own pose (kept here: losing it forgets g_gunLoose).
    const bool lyingLauncher = g_gunLoose.pose == gunprofile::Pose::kLauncher;
    const char* const lyingName = g_gunLoose.name;
    const gunprofile::Sound lyingSound = g_gunLoose.sound;
    bool lostNow = false;
    if (g_gunLoose.id) {
        bool found = false;
        for (int k = 0; k < r.poses && !found; ++k) {
            const loose::Pose& p = r.pose[k];
            if (p.id != g_gunLoose.id || !Finite(p.bone, 12))
                continue;
            float armsToRoom[12];
            if (!Invert(f.roomToArms, armsToRoom))
                break;
            std::memcpy(g_gunLoose.bone, p.bone, sizeof(g_gunLoose.bone));
            Compose(armsToRoom, p.bone, g_gunLoose.room);
            std::memcpy(g_gunLoose.origin, f.origin, sizeof(g_gunLoose.origin));
            g_gunLoose.resting = p.resting;
            found = true;
        }
        for (int k = 0; k < r.gone; ++k)
            if (r.goneId[k] == g_gunLoose.id)
                found = false;
        g_gunLoose.have = found;
        if (!found) {
            g_gunLost = true;
            g_gunLoose = GunLoose{};
            lostNow = true;
        }
    }
    if (g_graspNow.holder == grasp::Holder::kLoose) {
        bool& lying = lyingLauncher ? t.launcherLying : t.gunLying;
        bool& drawn = lyingLauncher ? t.launcherDrawn : t.gunDrawn;
        if (g_gunLoose.have) {
            lying = drawn = true;
            float* world = lyingLauncher ? t.launcherWorld : t.gunWorld;
            float* worldOrigin = lyingLauncher ? t.launcherWorldOrigin : t.gunWorldOrigin;
            std::memcpy(world, g_gunLoose.bone, sizeof(g_gunLoose.bone));
            for (int i = 0; i < 3; ++i)
                worldOrigin[i] = static_cast<float>(g_gunLoose.origin[i]);
            Compose(fromHead, g_gunLoose.room, lyingLauncher ? t.launcherPose : t.gunPose);  // (the view's space, for hands.cpp's own uses)
        } else if (!(lostNow && lying)) {
            lying = drawn = false;  // (lost this frame: FreeHands' last place for it, drawn lying, stays for this one frame)
        }
    }
    // Once a second while anything's loose: how many, the newest one's place in the world and its distance and bearing from the eye, and the copies drawn.
    if (static ULONGLONG s_said = 0; r.poses > 0 && GetTickCount64() - s_said >= 1000) {
        s_said = GetTickCount64();
        const loose::Pose& p = r.pose[r.poses - 1];
        const float to[3] = {p.bone[3] - f.eye[0], p.bone[7] - f.eye[1], p.bone[11] - f.eye[2]};
        const float d = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
        const float c = d > 1e-3f ? (to[0] * f.look[0] + to[1] * f.look[1] + to[2] * f.look[2]) / d : 1.0f;
        Log("Loose: %d (%d at rest); the newest #%d (kind %d) at (%.2f %.2f %.2f) in the world, %.2f m from the eye, %.0f degrees off the look%s%s; %d copies drawn in the second",
            r.poses, loose::Resting(), p.id, p.kind, p.bone[3] + static_cast<float>(f.origin[0]), p.bone[7] + static_cast<float>(f.origin[1]), p.bone[11] + static_cast<float>(f.origin[2]), d,
            std::acos(std::fmax(-1.0f, std::fmin(1.0f, c))) * 57.29578f, p.resting ? ", lying" : "", p.inGun ? ", in the gun" : "", skin::TakeLooseDraws());
    }
    for (int k = 0; k < r.gone; ++k)
        Log("Loose: #%d gone", r.goneId[k]);
    // Their landings: the game's own sounds, a take at random (never the same one twice running), panned from where it hit; the quietest left out (sounds.cpp has no level
    // for a play). The launcher's case: its sound graph's casing landing. A magazine's first landing: the game's own gun dropped on that ground (weapon_on_<surface>, as its
    // dropped weapons' sound graph maps the ground's material: 0 dirt, 1 tarmac, 2 grass, 3 wood, 4 metal); its smaller hits after: the game's rattly metal, or its hollow
    // plastic for the AK family's polymer magazines (magsound.h). The M249's cloth pouch: the game's own ammo bag landing, every hit.
    static int s_lastTake = -1;
    auto take = [](int of) {
        int t = static_cast<int>(GetTickCount64() % static_cast<ULONGLONG>(of));
        if (t == s_lastTake)
            t = (t + 1) % of;
        s_lastTake = t;
        return t;
    };
    for (int h = 0; h < r.hits; ++h) {
        const loose::Hit& hit = r.hit[h];
        if (hit.gainDb < -18.0f)
            continue;
        float to[3] = {hit.at[0] - f.eye[0], hit.at[1] - f.eye[1], hit.at[2] - f.eye[2]};
        const float right[3] = {seenFrom[0], seenFrom[4], seenFrom[8]}, length = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
        const float pan = length > 0.01f ? std::fmax(-0.8f, std::fmin(0.8f, (to[0] * right[0] + to[1] * right[1] + to[2] * right[2]) / length)) : 0.0f;
        char name[40];
        if (hit.kind == kLooseGun) {
            // The gun: the game's own gun dropped on that ground (weapon_on_<surface>) at its first landing and at any hit as loud after (-10 dB); its smaller
            // knocks the rattly metal, or the game's heavy metal knock for a heavy gun once its takes are cut (the plastic only for a polymer body: the AT4's).
            // (Every ground is dirt until the ray's material is mapped: SoldierFloor.)
            static constexpr const char* kRifleGround[loose::kSurfaces] = {"dirt", "tarmac", "grass", "grass", "dirt", "metal", "metal", "grass", "dirt", "wood", "grass", "tarmac", "wood"};
            const char* ground = hit.surface >= 0 && hit.surface < loose::kSurfaces ? kRifleGround[hit.surface] : "dirt";
            int heavyTakes = 0;
            if (lyingSound == gunprofile::Sound::kHeavy)
                for (char heavy[32]; heavyTakes < 10; ++heavyTakes)
                    if (sprintf_s(heavy, "hit_heavy_v%d_loose", heavyTakes); !sounds::Have(heavy))
                        break;
            // The AT4 thrown away: its first landing the hotel roof's own sound of an AT4 let fall (Matkovic's, SP03_SFX_RoofTop_DropAT: drop_at4,
            // made from the player's own game files) when it's there, else as any gun's; a fibreglass tube's smaller knocks the hollow plastic's (the
            // only gun with a polymer body).
            const bool at4 = lyingName == gunprofile::kNames[gunprofile::kAt4Row][1];
            if (at4 && hit.first && sounds::Have("drop_at4"))
                sprintf_s(name, "drop_at4");
            else if (hit.first || hit.gainDb >= -10.0f)
                sprintf_s(name, "land_%s_v%d_loose", ground, take(std::strcmp(ground, "metal") == 0 ? 3 : 4));
            else if (heavyTakes > 0)
                sprintf_s(name, "hit_heavy_v%d_loose", take(heavyTakes));
            else if (lyingSound == gunprofile::Sound::kPolymer)
                sprintf_s(name, "hit_plastic_v%d_loose", take(6));
            else
                sprintf_s(name, "hit_metal_v%d_loose", take(5));
            Log("Loose: the %s hit the ground at %.1f m/s (%.0f dB): %s", lyingName, hit.speed, hit.gainDb, name);
        } else if (hit.kind == 3) {
            sprintf_s(name, "case_land_v%d_m320", take(10));
        } else if (hit.kind == kLooseShell) {
            sprintf_s(name, "shell_land_v%d_m1014", take(8));  // the game's own shotgun shell landing, eight takes
        } else if (hit.kind == 2 && magsound::Of(skin::HeldShape()) == magsound::Stuff::kCloth) {
            // A cloth pouch (the M249's box): the game's own ammo bag landing at every hit, the first too; never the gun dropped on the ground nor the metal
            // knocks. Its clips are made from the player's own game at the next start: until then the soft knock, the hollow plastic. Only the takes there
            // are played, as the heavy knocks (the review of 2026-11-18; all seven decode from the game's files, 48 kHz, as the cutter takes them).
            int bagTakes = 0;
            for (char bag[32]; bagTakes < magsound::kBagTakes; ++bagTakes)
                if (sprintf_s(bag, "bag_v%d_loose", bagTakes); !sounds::Have(bag))
                    break;
            if (bagTakes > 0)
                sprintf_s(name, "bag_v%d_loose", take(bagTakes));
            else
                sprintf_s(name, "hit_plastic_v%d_loose", take(6));
        } else if (hit.first) {
            static constexpr const char* kGround[loose::kSurfaces] = {"dirt", "tarmac", "grass", "grass", "dirt", "metal", "metal", "grass", "dirt", "wood", "grass", "tarmac", "wood"};
            const char* ground = hit.surface >= 0 && hit.surface < loose::kSurfaces ? kGround[hit.surface] : "dirt";
            sprintf_s(name, "land_%s_v%d_loose", ground, take(std::strcmp(ground, "metal") == 0 ? 3 : 4));
        } else {
            // (The launcher's case, a fibreglass tube: the hollow plastic too.)
            const bool polymer = hit.kind == kLooseSmawCase || (hit.kind == 2 && magsound::Of(skin::HeldShape()) == magsound::Stuff::kPlastic);
            sprintf_s(name, polymer ? "hit_plastic_v%d_loose" : "hit_metal_v%d_loose", take(polymer ? 6 : 5));
        }
        sounds::Play(name, pan);
    }
}

// weapon.txt "m320test 7", nobody in the headset (the stand-in holds the rifle): a scripted left hand works the launcher's reload by hand, round after round, 8 s each,
// so it's filmed and its numbers logged without the user (the user, 2026-11-05: "You're not doing good testing. If you didn't see this, this is insane"). The barrel
// emptied and sprung open (m320::TestOpenEmpty); a round from the belly (0.5 s); up in front of the face, the wrist turning 50 degrees each way (1.5-3.5 s); to 4 cm
// behind the barrel's back end in line with it, then pushed in along it at 0.25 m/s (4-5 s); let go of, beside the barrel's left side, a slap at 1 m/s (5.4-5.8 s).
bool g_scriptOn = false;          // the scripted hand is the left one this frame
float g_scriptSqueeze = 0.0f;     // ... its grip
float g_scriptGripRoom[3] = {};   // ... its grip point in the room
float g_scriptClock = 0.0f;
// (The round in the hand as the module last had it in the glove: g_lastRoundInGlove, by side beside the film rounds' hands.)

// The scripted left glove (view space) this frame, its grip and grip point; false when the test's off or the rifle isn't held.
bool ScriptedLeft(const skin::HandTargets& t, const float* headPose, const float* fromHead, float dt, const float* baseGlove, float* glove) {
    if (skin::M320Test() != 7 || !t.gun) {
        g_scriptClock = 0.0f;
        return false;
    }
    const float was = std::fmod(g_scriptClock, 8.0f);
    g_scriptClock += std::fmin(dt, 0.05f);
    const float c = std::fmod(g_scriptClock, 8.0f);
    if (c < was || g_scriptClock == std::fmin(dt, 0.05f))
        m320::TestOpenEmpty();
    // The places the hand goes (view space): the belly (pouch's, as the module measures it), in front of the face, the barrel's back end in line, beside its left side.
    float neck[3], yaw = 0.0f, belly[3] = {-0.05f, -0.45f, -0.15f};
    if (pouch::BodyFrame(neck, yaw)) {
        const float right[3] = {std::cos(yaw), 0.0f, -std::sin(yaw)}, forward[3] = {-std::sin(yaw), 0.0f, -std::cos(yaw)};
        float room[3];
        for (int i = 0; i < 3; ++i)
            room[i] = neck[i] + right[i] * ammozone::kBelly[0] + forward[i] * ammozone::kBelly[2] + (i == 1 ? ammozone::kBelly[1] : 0.0f);
        Transform(fromHead, room, belly);
    }
    const float face[3] = {-0.04f, -0.06f, -0.42f};  // (held up in front of the eyes, in the middle of the picture: nearer, the sleeve filled it)
    auto onGun = [&](const float* p, float* view) { Transform(t.gunPose, p, view); };
    float slapFrom[3], slapTo[3];
    {
        const float a[3] = {0.135f, -0.036f, 0.60f}, b[3] = {-0.065f, -0.036f, 0.60f};  // 7 cm out from the open barrel's left side, mid way along it; 20 cm on to its right
        onGun(a, slapFrom), onGun(b, slapTo);
    }
    // The glove placed with its palm point at p, turned as base (about the view's forward by turn degrees).
    auto placeAt = [&](const float* p, float turn, float* out) {
        float r[12];
        std::memcpy(r, baseGlove, sizeof(r));
        if (turn != 0.0f) {
            const float c0 = std::cos(turn * 0.0174533f), s0 = std::sin(turn * 0.0174533f);
            const float spin[12] = {c0, -s0, 0, 0, s0, c0, 0, 0, 0, 0, 1, 0};
            float spun[12];
            Compose(spin, r, spun);
            std::memcpy(r, spun, sizeof(r));
        }
        float at[3];
        r[3] = r[7] = r[11] = 0.0f;
        Transform(r, g_leftAnchor, at);
        r[3] = p[0] - at[0], r[7] = p[1] - at[1], r[11] = p[2] - at[2];
        std::memcpy(out, r, sizeof(r));
    };
    // The glove holding the round at s back along the open barrel's line (as the module last had it in the glove).
    auto holdingAt = [&](float s, float* out) {
        float round[12], view[12], inverse[12];
        m320::RoundOnLine(m320::kOpenMost, s, round);
        Compose(t.gunPose, round, view);
        if (g_haveLastRoundInGlove && Invert(g_lastRoundInGlove, inverse)) {
            Compose(view, inverse, out);
        } else {
            const float at[3] = {view[3], view[7], view[11]};
            placeAt(at, 0.0f, out);
        }
    };
    auto lerp = [](const float* a, const float* b, float e, float* out) {
        for (int i = 0; i < 3; ++i)
            out[i] = a[i] + (b[i] - a[i]) * e;
    };
    float p[3];
    g_scriptSqueeze = c >= 0.5f && c < 5.0f ? 1.0f : 0.0f;
    if (c < 1.0f) {
        placeAt(belly, 0.0f, glove);
    } else if (c < 1.5f) {
        lerp(belly, face, steer::Smooth((c - 1.0f) / 0.5f), p);
        placeAt(p, 0.0f, glove);
    } else if (c < 3.5f) {
        placeAt(face, 50.0f * std::sin(6.2831853f * (c - 1.5f) / 2.0f), glove);
    } else if (c < 4.0f) {
        float from[12], to[12];
        placeAt(face, 0.0f, from);
        holdingAt(m320::kClear + 0.04f, to);
        BlendAbout(from, to, steer::Smooth((c - 3.5f) / 0.5f), g_leftAnchor, glove);
    } else if (c < 5.0f) {
        holdingAt(m320::kClear + 0.04f - 0.25f * (c - 4.0f), glove);
    } else if (c < 5.4f) {
        // (Turned and moved together from where it pushed the round to the slap's start: turned at once, it read in the log as a 77-degree jump of its own.)
        float end[12], to[12];
        holdingAt(m320::kClear + 0.04f - 0.25f, end);
        placeAt(slapFrom, 0.0f, to);
        BlendAbout(end, to, steer::Smooth((c - 5.0f) / 0.4f), g_leftAnchor, glove);
    } else if (c < 5.6f) {
        lerp(slapFrom, slapTo, (c - 5.4f) / 0.2f, p);  // 20 cm in 0.2 s: 1 m/s
        placeAt(p, 0.0f, glove);
    } else {
        placeAt(slapTo, 0.0f, glove);
    }
    float palm[3];
    Transform(glove, g_leftAnchor, palm);
    Transform(headPose, palm, g_scriptGripRoom);
    if (c < 1.0f)  // (at the belly: its grip point exactly where pouch measures the belly)
        Transform(headPose, belly, g_scriptGripRoom);
    return true;
}

// The second hold (the user, 2026-11-05: a grip of its own on the grenade launcher, "switchable", not overlapping the first, both working "just the same"): the launcher's own pistol
// grip under the rifle, the only place on it with a trigger under a finger. A fresh squeeze takes it with the real wrist within 10 cm of its wrist point and 2 cm nearer it than the
// handguard hold's (the two are 18 cm apart): anything else goes to the handguard as before. Held, the game's launcher is put in hand (input.cpp) and the trigger of the hand on
// its grip fires it. The wrist points: the game's own gloves there once learned, till then where the study put them (the M416's model space). (Which hand holds it:
// g_onM320By, beside the handguard's hold.) Free hands: the right's palm is measured as the left's mirrored (the grip and the handguard on the rifle's middle).
constexpr float kM320WristGuess[3] = {0.077f, -0.184f, 0.447f}, kGuardWristGuess[3] = {0.035f, -0.164f, 0.609f};
constexpr float kM320Reach = 0.10f, kM320Nearer = 0.02f;

bool InM320Zone(const float* palm, float* distanceOut) {
    if (!ammo::M320Carried() || !skin::RifleInHand())
        return false;
    float m320[3], guard[3], bones[handpose::kBones][12];
    std::memcpy(m320, kM320WristGuess, sizeof(m320));
    std::memcpy(guard, kGuardWristGuess, sizeof(guard));
    if (skin::GameLeftGripM320(bones))
        Transform(bones[0], g_leftAnchor, m320);
    if (skin::GameLeftGrip(bones))
        Transform(bones[0], g_leftAnchor, guard);
    const float toM320[3] = {palm[0] - m320[0], palm[1] - m320[1], palm[2] - m320[2]};
    const float toGuard[3] = {palm[0] - guard[0], palm[1] - guard[1], palm[2] - guard[2]};
    const float dM320 = Length(toM320), dGuard = Length(toGuard);
    *distanceOut = dM320;
    return dM320 < kM320Reach && dM320 + kM320Nearer < dGuard;
}

// The grenade launcher reloaded by hand (m320.h), once a frame before the pouch and the holds: its frame from the hands, its events into the count, the sounds (the game's own,
// cut by tools\gamesounds.cpp), the log, and what leaves it (a spent case or a round) handed to loose. g_m320GunRoom: the gun as drawn last frame, in the room (after the steer).
float g_m320GunRoom[12] = {};
bool g_haveM320GunRoom = false;
bool g_m320InUse = false;  // the reload by hand in use this frame

// The free right glove's palm width (freehands.txt "rightwidth": the game's own 1 unless set; its poses are the left's flipped, posed at this width).
float RightGloveWidth() {
    float width = 1.0f, slim = 1.0f;
    freetune::RightWidth(width, slim);
    return width;
}

m320::Result M320ByHand(const skin::HandTargets& t, const float* headPose, const float* tracked, bool trackedLeft, bool pistolHeld, float dt) {
    // A round carried in the hand held by its base end in a fingertip pinch, the thumb underneath, the index and middle on top, the round straight out along the index
    // (the user, 2026-11-05: "at most three fingers ... at the end of the shell ... on that circle base"; their pick of four, "A"), fitted on the meshes
    // (tools\basegrip.cpp, m320carry.h): given to the module once, before its first frame. (A fist round its body before: "you don't need a whole fist".)
    static bool s_carryGiven = false;
    if (!s_carryGiven) {
        m320::SetHold(m320::Hold::kCarry, m320carry::kRoundInGlove, m320carry::kFingers);
        s_carryGiven = true;
        Log("M320: a round carried in the hand held by its base, thumb under, index and middle on top (m320carry.h, grip A)");
    }
    m320::Frame mf;
    mf.dt = dt;
    mf.active = skin::RifleInHand() && ammo::M320Carried() && !pistolHeld && !skin::RifleSeated();
    mf.gunTracked = t.gun;
    std::memcpy(mf.gunPose, t.gunPose, sizeof(mf.gunPose));  // (as the right hand alone holds it: the steer comes later)
    mf.haveGunRoom = g_haveM320GunRoom;
    std::memcpy(mf.gunRoom, g_m320GunRoom, sizeof(mf.gunRoom));
    std::memcpy(mf.headPose, headPose, sizeof(mf.headPose));
    m320::HandIn& left = mf.hand[0];
    XrPosef leftGrip{};
    left.tracked = trackedLeft && !g_lining && (g_scriptOn || HandPoseOf(0, leftGrip));
    std::memcpy(left.glove, tracked, sizeof(left.glove));
    left.gripRoom[0] = leftGrip.position.x, left.gripRoom[1] = leftGrip.position.y, left.gripRoom[2] = leftGrip.position.z;
    left.squeeze = SqueezeOf(0);
    left.trigger = TriggerOf(0);
    if (g_scriptOn) {  // (weapon.txt "m320test 7": the scripted hand's own)
        std::memcpy(left.gripRoom, g_scriptGripRoom, sizeof(left.gripRoom));
        left.squeeze = g_scriptSqueeze;
        left.trigger = 0.0f;
    }
    left.onGripB = g_supporting && g_onM320 && !g_holdOutOfReach;
    left.busy = pouch::HoldingSide() == 0 || reload::HandleSide() == 0 || (g_supporting && !left.onGripB) || g_leftOnGrip || g_leftSpent;  // (free hands: on the rifle's pistol grip)
    // Free hands: the hand holding the rifle (by its pistol grip, or by its handguard or the launcher's grip alone) is never the one at work. Changed only while
    // the launcher has nothing in the hand and its glove and fingers have eased back off it; dormant, the right, as before.
    if (!m320::Holding() && !g_m320Easing) {
        const grasp::Result& held = g_graspNow;
        if (!g_freeOn)
            g_m320GunHand = 1;
        else if ((held.holder == grasp::Holder::kHeld || held.holder == grasp::Holder::kGlide) && (held.trigger == 0 || held.trigger == 1))
            g_m320GunHand = held.trigger;
        else if (held.holder == grasp::Holder::kBarrel && (held.barrel == 0 || held.barrel == 1))
            g_m320GunHand = held.barrel;
    }
    // The right at work (free hands, the left holding the rifle): its own controller, its free glove (the left's line-up mirrored). On the launcher's grip
    // even while not at work or unseen (the release's press goes by the hand on its grip); busy with anything else.
    m320::HandIn& right = mf.hand[1];
    XrPosef rightGrip{};
    right.tracked = g_rightWorks && HandPoseOf(1, rightGrip) && g_m320GunHand == 0;
    std::memcpy(right.glove, g_rightFree, sizeof(right.glove));
    right.gripRoom[0] = rightGrip.position.x, right.gripRoom[1] = rightGrip.position.y, right.gripRoom[2] = rightGrip.position.z;
    right.squeeze = SqueezeOf(1);
    right.trigger = TriggerOf(1);
    right.onGripB = g_supportingBy[1] && g_onM320By[1] && !g_holdOutOfReachBy[1];
    right.busy = pouch::HoldingSide() == 1 || reload::HandleSide() == 1 || (g_supportingBy[1] && !right.onGripB) || g_onGripBy[1] || g_spentBy[1];
    mf.gunHand = g_m320GunHand;
    mf.flickWay = g_m320GunHand == 0 ? -1 : 1;  // (the rifle in the left hand: the wrist's roll read mirrored)
    mf.haveBody = pouch::BodyFrame(mf.neck, mf.yaw);
    mf.loaded = ammo::M320Loaded() >= 1;
    mf.spare = ammo::M320Spare() >= 1;
    mf.inGameHand = ammo::M320Up();
    mf.shotUnderway = input::M320ShotUnderway();
    // (Its firing state's word keeps the state a put-away came from, so 10 or 11 can read on after a reload put away part way: the module stays ready to fire while it stands
    // aside, so a pull is never swallowed. Reading the word that holds the current state instead (+0xA4) wants a run in the game first: the review, 2026-11-05.)
    const int state = ammo::M320State();
    mf.gameReload = input::M320GameReloading() || state == 10 || state == 11;
    mf.releasePress = m320::TakeReleasePress();
    mf.rifleOpen = skin::MagazineOut() || reload::BoltBack();
    mf.test = skin::M320Test();
    // The palm as the glove draws it: the module takes the round in the hand and its holds through it, as the glove's tables are re-anchored on it below (its round
    // was 6 cm and 40 degrees off the one drawn: caught where it wasn't seen, and the hand riding it home turned 40 degrees at once, both arms gone: the user's video,
    // 2026-11-05).
    {
        static float s_width = -1.0f, s_palm[12];
        if (g_leftWidth != s_width) {
            float palm[handpose::kBones][12];
            handpose::Pose(handpose::Fingers{}, g_leftWidth, palm);
            std::memcpy(s_palm, palm[1], sizeof(s_palm));
            s_width = g_leftWidth;
        }
        std::memcpy(mf.palm[0], s_palm, sizeof(mf.palm[0]));
        mf.havePalm[0] = true;
        // The free right glove's alike, in the left's layout at its own width (the module mirrors the right's holds onto it).
        static float s_rightWidth = -1.0f, s_rightPalm[12];
        const float rightWidth = RightGloveWidth();
        if (rightWidth != s_rightWidth) {
            float palm[handpose::kBones][12];
            handpose::PoseSafe(handpose::Fingers{}, rightWidth, palm);
            std::memcpy(s_rightPalm, palm[1], sizeof(s_rightPalm));
            s_rightWidth = rightWidth;
        }
        std::memcpy(mf.palm[1], s_rightPalm, sizeof(mf.palm[1]));
        mf.havePalm[1] = true;
    }
    const m320::Result r = m320::Update(mf);
    g_m320Easing = r.gloveBlend > 0.0f || r.fingersShare > 0.0f;  // (its glove or fingers still on the hand at work: the rifle's hand kept till they're off)
    g_m320InUse = mf.active && m320::ByHand();  // (its content is only fresh while it's in use: off, or in the car, it keeps an old one)
    const int side = r.side == 1 ? 1 : 0;  // the hand at work
    if (r.spentSqueeze)
        g_freshBy[side] = false;
    // The game's own sounds for each part of it.
    if (r.released)
        sounds::Play("open_latch_m320");
    if (r.openStop)
        sounds::Play("open_stop_m320");
    if (r.contentMoving)
        sounds::Play("case_out_m320");
    if (r.caught)
        sounds::Play("round_in_m320");
    if (r.shut)
        sounds::Play("close_m320");  // (at the hit: its own hit is 10 ms in, its latch 66 ms)
    if (r.dryPull)
        sounds::Play("dry_m320");
    // The count: a live round home, one from the spare ones; a live one out again, back with them.
    if (r.seated)
        ammo::M320LoadByHand();
    if (r.unload)
        ammo::M320Unload();
    // What leaves it falls to the ground and lies there (loose): slid out of the barrel with its slide's speed (loose adds the gun's own), or let go of from the hand.
    if (r.drop) {
        const char* what = r.dropLive ? "live round" : "spent case";
        const char* how = r.dropFromBarrel ? "out of the open barrel" : "let go of";
        if (skin::LooseOn() && g_haveLooseFrame) {
            float view[12], caseOnRound[12], caseRoom[12];
            skin::M320CaseOnRound(caseOnRound);
            Compose(r.dropView, caseOnRound, view);
            Compose(headPose, view, caseRoom);
            // (Let go of from the hand: by that hand's own motion, the right's too.)
            QueueLoose(kLooseM320Case, skin::M320CaseSpec(), caseRoom, r.dropFromBarrel ? r.dropSlideRoom : r.dropVelocityRoom, !r.dropFromBarrel, r.dropLive ? 1 : 0, side);
            Log("M320: the %s %s: it falls", what, how);
        } else {
            Log("M320: the %s %s: gone (%s: nothing to fall with)", what, how, skin::LooseOn() ? "loose not stepping yet" : "weapon.txt \"loose 0\"");
        }
    }
    // The log: each event with its state and why.
    const bool told = r.released || r.openStop || r.contentMoving || r.unload || r.took || r.putBack || r.entered || r.caught || r.seated || r.caseSeated || r.shut || r.slapped || r.latchedNow ||
                      r.dryPull || r.flickOpen || r.flickShut || r.barrelTaken || r.barrelLet || r.sprungBack || r.blocked || r.pinched || r.freed || r.jabbed || r.letGo ||
                      r.cancelled || r.fired || r.reloaded;
    if (told || (skin::M320Log() && r.drawn)) {
        char what[256] = "";
        auto add = [&what](bool on, const char* word) {
            if (on)
                strcat_s(what, *what ? ", " : ""), strcat_s(what, word);
        };
        add(r.released, "unlatched"), add(r.flickOpen, "flicked open"), add(r.openStop, "at its stop"), add(r.contentMoving, "the round in it moving"),
            add(r.unload, "a live round out (back with the spare ones)"), add(r.took, "a new round from the belly"), add(r.putBack, "the round put back"),
            add(r.entered, "its nose into the barrel: pushed in by the hand"), add(r.caught, "pushed past half way: home by itself"), add(r.seated, "a live round home"),
            add(r.caseSeated, "a spent case pushed back in"), add(r.slapped, "slapped shut"), add(r.shut, "shut"),
            add(r.latchedNow, "latched"), add(r.flickShut, "flicked shut"), add(r.dryPull, "a dry pull"), add(r.barrelTaken, "the hand on the barrel"),
            add(r.barrelLet, "the hand off the barrel"), add(r.sprungBack, "sprung back open"), add(r.blocked, "blocked by the round half out"), add(r.pinched, "the rim pinched"),
            add(r.freed, "pulled free"), add(r.jabbed, "jabbed out"), add(r.letGo, "let go of"), add(r.cancelled, "gone back"), add(r.fired, "fired"),
            add(r.reloaded, "reloaded without the hand");
        Log("M320: %s; %s%s%s%s", *what ? what : "state", r.state, *r.note ? " (" : "", r.note, *r.note ? ")" : "");
    }
    if (r.squeezed)
        Log("M320: the %s grip squeezed %.0f cm right, %.0f cm up, %.0f cm forward of the neck: %s", kHandName[side], r.fromNeck[0] * 100.0f, r.fromNeck[1] * 100.0f,
            r.fromNeck[2] * 100.0f, *r.note ? r.note : "nothing for the launcher");
    return r;
}

// ---- A tube shotgun's shells by hand (shellload.h; the M1014; docs\research\shotgun-shells.md) ----
// Once a frame with such a gun drawn (TubeByHand): its frame from the hands, its events into the count (ammo.cpp's tube), the sounds (the game's own, cut
// by tools\gamesounds.cpp), the buzz, the log, and a shell that leaves the hand or the bolt handed to loose. For input.cpp: nothing to fire, and the
// handle held back.
std::atomic<bool> g_tubeBoltHeldNow{false}, g_tubeDryPull{false}, g_tubeInHand{false};
int g_tubeGunHand = 1;        // the hand holding the shotgun (never the one at work), changed only while the hand at work has nothing of it
float g_shellTestClock = 0.0f;

// The game's own sound of the shotgun, if its clip is there (made from the game's files at the first start).
void PlayTube(const char* name) {
    if (sounds::Have(name))
        sounds::Play(name);
}

// A point of a glove's bind space (the left's layout; side 1: mirrored) in the view.
void GlovePoint(int side, const float* glove, const float* bind, float* out) {
    const float at[3] = {side == 1 ? -bind[0] : bind[0], bind[1], bind[2]};
    Transform(glove, at, out);
}

// tracked: the left glove as its controller holds it; gunLeftHands: the gun has left the hands (lying, flying into a hand, put away). scripted (weapon.txt
// "shelltest N", nobody in the headset): the stand-in left hand's glove for this frame, written to scriptGlove.
shellload::Result TubeByHand(const skin::HandTargets& t, const float* headPose, const float* fromHead, const float* tracked, bool trackedLeft, bool gunLeftHands, float dt,
                             bool* scripted, float* scriptGlove) {
    static bool s_holdGiven = false;
    if (!s_holdGiven) {
        shellload::SetHold(shellposes::kShellInGlove, shellposes::kFingers);
        s_holdGiven = true;
        Log("Shotgun: a shell carried in the hand held by its base, thumb under, index and middle on top (shellposes.h)");
    }
    *scripted = false;
    const shellload::Gun* gun = skin::HeldTubeGun();
    const bool live = gun && ammo::TubeLive() && skin::TubeByHand();
    shellload::Frame sf;
    sf.dt = dt;
    sf.gun = gun;
    sf.active = live && t.gun && !gunLeftHands;
    sf.gunTracked = t.gun;
    std::memcpy(sf.gunPose, t.launcherPose, sizeof(sf.gunPose));  // (as the hand holding it alone has it: the steer comes later)
    std::memcpy(sf.headPose, headPose, sizeof(sf.headPose));
    // The hand holding the gun (by its pistol grip, or by its front alone) is never the one at work; changed only while the hand at work has nothing of it.
    if (!shellload::Holding()) {
        const grasp::Result& held = g_graspNow;
        if (!g_freeOn)
            g_tubeGunHand = 1;
        else if ((held.holder == grasp::Holder::kHeld || held.holder == grasp::Holder::kGlide) && (held.trigger == 0 || held.trigger == 1))
            g_tubeGunHand = held.trigger;
        else if (held.holder == grasp::Holder::kBarrel && (held.barrel == 0 || held.barrel == 1))
            g_tubeGunHand = held.barrel;
    }
    sf.gunHand = g_tubeGunHand;
    static float s_tips[2][3];  // the open hand's index and middle fingertips in the left glove's bind space (handpose::Joints)
    static bool s_haveTips = false;
    if (!s_haveTips) {
        float joints[5][4][3];
        handpose::Joints(joints);
        std::memcpy(s_tips[0], joints[1][3], sizeof(s_tips[0]));
        std::memcpy(s_tips[1], joints[2][3], sizeof(s_tips[1]));
        s_haveTips = true;
    }
    auto touches = [&](int side, shellload::HandIn& h) {
        // What presses the release: the hand's middle and the open hand's index and middle fingertips, where the controller has them.
        GlovePoint(side, h.glove, shellload::kHandInGlove, h.touch[0]);
        GlovePoint(side, h.glove, s_tips[0], h.touch[1]);
        GlovePoint(side, h.glove, s_tips[1], h.touch[2]);
        h.touches = 3;
    };
    shellload::HandIn& left = sf.hand[0];
    XrPosef leftGrip{};
    left.tracked = trackedLeft && !g_lining && HandPoseOf(0, leftGrip);
    std::memcpy(left.glove, tracked, sizeof(left.glove));
    left.gripRoom[0] = leftGrip.position.x, left.gripRoom[1] = leftGrip.position.y, left.gripRoom[2] = leftGrip.position.z;
    left.squeeze = SqueezeOf(0);
    left.trigger = TriggerOf(0);
    left.busy = pouch::HoldingSide() == 0 || g_onLauncherBy[0] || g_leftOnGrip || g_leftSpent;
    touches(0, left);
    shellload::HandIn& right = sf.hand[1];
    XrPosef rightGrip{};
    right.tracked = g_rightWorks && HandPoseOf(1, rightGrip) && g_tubeGunHand == 0;
    std::memcpy(right.glove, g_rightFree, sizeof(right.glove));
    right.gripRoom[0] = rightGrip.position.x, right.gripRoom[1] = rightGrip.position.y, right.gripRoom[2] = rightGrip.position.z;
    right.squeeze = SqueezeOf(1);
    right.trigger = TriggerOf(1);
    right.busy = pouch::HoldingSide() == 1 || g_onLauncherBy[1] || g_onGripBy[1] || g_spentBy[1];
    touches(1, right);
    sf.haveBody = pouch::BodyFrame(sf.neck, sf.yaw);
    // The gun as the count has it.
    const int belly = ammo::TubeBelly();
    const bool gameReload = ammo::TubeGameReload();
    sf.take = static_cast<shellload::Take>(static_cast<int>(ammo::TubeCanTake(0)));
    sf.tubeFull = ammo::TubeShells() >= ammo::TubeCapacity();
    sf.canLoad = !sf.tubeFull && belly >= 1 && !gameReload;
    sf.locked = ammo::TubeLocked();
    sf.portOpen = sf.locked && !ammo::TubeChambered() && !ammo::TubePort() && belly >= 1 && !gameReload;
    sf.gameReload = gameReload;
    // weapon.txt "shelltest N" with nobody in the headset (the stand-in holds the gun): a scripted left hand, 7 s a round. 1: a shell from the belly, up to the
    // loading port's mouth, pushed home along the way in. 2: a shell from the belly laid into the ejection port (the bolt must be held open: fire it empty
    // first), then the release poked. 3: the handle taken at its knob, drawn back all its way and let go.
    const int test = sf.active && !trackedLeft ? skin::ShellTest() : 0;
    if (XrPosef standIn; test >= 1 && test <= 3 && input::StandInLeftPose(standIn)) {
        g_shellTestClock = std::fmod(g_shellTestClock + dt, 7.0f);
        const float c = g_shellTestClock;
        float toGun[12], shellOn[12], shellView[12], glove[12];
        InvertRigid(sf.gunPose, toGun);
        auto withShell = [&](const float* onGun) {
            Compose(sf.gunPose, onGun, shellView);
            shellload::GloveFor(0, shellView, glove);
        };
        auto ease = [](float from, float to, float now) {
            const float s = std::fmin(std::fmax((now - from) / (to - from), 0.0f), 1.0f);
            return s * s * (3.0f - 2.0f * s);
        };
        auto shifted = [&](const float* on, float x, float y, float z) {
            std::memcpy(shellOn, on, sizeof(shellOn));
            shellOn[3] += x, shellOn[7] += y, shellOn[11] += z;
        };
        left.forceBelly = false;
        left.busy = false;
        float squeeze = 0.0f;
        // (The hand's turn for the release and the handle: as it holds a shell lying along the gun; the untracked glove itself is all zeros.)
        float reach[12], reachOn[12];
        shellload::ShellInPort(*gun, reachOn);
        Compose(sf.gunPose, reachOn, shellView);
        shellload::GloveFor(0, shellView, reach);
        if (test == 1) {
            float start[12], at[12];
            shellload::ShellOnPath(*gun, 0.0f, start);
            const float length = shellload::PathLength(*gun);
            if (c < 1.0f) {  // at the belly: squeezed from 0.5 s
                shifted(start, 0.10f, -0.25f, -0.25f);
                left.forceBelly = true;
                squeeze = c > 0.5f ? 1.0f : 0.0f;
            } else if (c < 2.5f) {  // carried to 10 cm under the mouth, then up to it
                const float s = ease(1.0f, 2.5f, c);
                shifted(start, 0.10f * (1.0f - s), -0.25f + 0.25f * s, -0.25f * (1.0f - s));
                squeeze = 1.0f;
            } else if (c < 3.0f) {
                shifted(start, 0.0f, 0.0f, 0.0f);
                squeeze = 1.0f;
            } else if (c < 4.5f) {  // pushed home
                shellload::ShellOnPath(*gun, length * ease(3.0f, 4.3f, c), at);
                shifted(at, 0.0f, 0.0f, 0.0f);
                squeeze = 1.0f;
            } else {  // away
                shifted(start, 0.12f, -0.20f, -0.10f);
            }
            withShell(shellOn);
        } else if (test == 2) {
            float port[12];
            shellload::ShellInPort(*gun, port);
            if (c < 1.0f) {
                shifted(port, -0.15f, -0.30f, -0.25f);
                left.forceBelly = true;
                squeeze = c > 0.5f ? 1.0f : 0.0f;
                withShell(shellOn);
            } else if (c < 3.0f) {  // over to the port from the gun's right
                const float s = ease(1.0f, 2.6f, c);
                shifted(port, -0.15f + 0.13f * s, -0.30f + 0.32f * s, -0.25f * (1.0f - s));
                squeeze = 1.0f;
                withShell(shellOn);
            } else {  // the hand's middle onto the release at 0.3 m/s from 6 cm out, held there, then away
                const float out = c < 3.6f ? 0.06f : c < 3.8f ? 0.06f - 0.3f * (c - 3.6f) : c < 5.0f ? 0.0f : 0.10f;
                const float onGun[3] = {gun->release[0] - 0.005f - out, gun->release[1], gun->release[2]};
                float view[3];
                Transform(sf.gunPose, onGun, view);
                std::memcpy(glove, reach, 12 * sizeof(float));
                float middle[3];
                GlovePoint(0, glove, shellload::kHandInGlove, middle);
                for (int i = 0; i < 3; ++i)
                    glove[i * 4 + 3] += view[i] - middle[i];
            }
        } else {
            // (From home: drawn all its way back. Held open as the round begins: a tug of 1 cm further.)
            static bool s_fromOpen = false;
            if (c < 0.1f)
                s_fromOpen = ammo::TubeLocked();
            const float drawn = c < 1.5f ? 0.0f : c < 2.7f ? ease(1.5f, 2.3f, c) : 0.0f;
            const float back = s_fromOpen ? gun->travel + 0.010f * drawn : (gun->travel + 0.005f) * drawn;
            const float out = c < 1.0f ? 0.10f * (1.0f - ease(0.0f, 0.9f, c)) : c < 2.7f ? 0.0f : 0.10f;
            const float onGun[3] = {gun->handle[0] - 0.015f - out, gun->handle[1], gun->handle[2] - back};
            float view[3], middle[3];
            Transform(sf.gunPose, onGun, view);
            std::memcpy(glove, reach, 12 * sizeof(float));
            GlovePoint(0, glove, shellload::kHandInGlove, middle);
            for (int i = 0; i < 3; ++i)
                glove[i * 4 + 3] += view[i] - middle[i];
            squeeze = c > 1.2f && c < 2.6f ? 1.0f : 0.0f;
        }
        left.tracked = true;
        std::memcpy(left.glove, glove, sizeof(left.glove));
        float at[3], room[3];
        GlovePoint(0, glove, shellload::kHandInGlove, at);
        Transform(headPose, at, room);
        std::memcpy(left.gripRoom, room, sizeof(left.gripRoom));
        left.squeeze = squeeze;
        left.trigger = 0.0f;
        touches(0, left);
        std::memcpy(scriptGlove, glove, 12 * sizeof(float));
        *scripted = true;
        (void)toGun;
    } else {
        g_shellTestClock = 0.0f;
    }
    const shellload::Result r = shellload::Update(sf);
    const int side = r.side == 1 ? 1 : 0;
    const int gunSide = g_tubeGunHand == 0 ? 0 : 1;
    if (r.spentSqueeze)
        g_freshBy[side] = false;  // (not the front grip's as well)
    // ---- Its events: the count, the sounds, the buzz, the log ----
    if (r.took) {
        input::Buzz(side, 0.15f, 0.020f);
        Log("Shotgun: a shell from the belly (the %s hand; %d left there)", kHandName[side], ammo::TubeBelly());
    }
    if (r.squeezed)
        Log("Shotgun: squeezed at the belly %.0f cm right, %.0f cm up, %.0f cm forward of the neck: nothing (%s)", r.fromNeck[0] * 100.0f, r.fromNeck[1] * 100.0f,
            r.fromNeck[2] * 100.0f, *r.note ? r.note : "nothing to take");
    if (r.putBack)
        Log("Shotgun: the shell put back at the belly");
    if (r.entered)
        Log("Shotgun: its nose into the loading port: pushed in by the hand");
    if (r.pulledOut)
        Log("Shotgun: the shell pulled back out of the loading port%s%s%s", *r.note ? " (" : "", r.note, *r.note ? ")" : "");
    if (r.fullKnock) {
        input::Buzz(side, 0.3f, 0.040f);
        Log("Shotgun: the tube is full: the shell stops at the lifter");
    }
    if (r.caught) {
        PlayTube("shell_in_m1014");
        input::Buzz(side, 0.5f, 0.030f);
        input::Buzz(gunSide, 0.2f, 0.020f);
    }
    if (r.loaded)
        ammo::TubeLoadShell();
    if (r.portDrop) {
        if (ammo::TubePortLoad() == shotgunammo::Load::kIn) {
            PlayTube("port_in_m1014");
            input::Buzz(side, 0.3f, 0.025f);
        }
    }
    auto boltHome = [&](const shotgunammo::Bolt& b) {
        // The bolt going home: its own slam, in both hands.
        if (b.moved && !b.locked) {
            PlayTube("bolt_m1014");
            input::Buzz(side, 0.4f, 0.050f);
            input::Buzz(gunSide, 0.8f, 0.050f);
        }
    };
    if (r.released) {
        Log("Shotgun: the release pressed (the %s hand)", kHandName[side]);
        boltHome(ammo::TubeBoltHome());
    }
    if (r.handleTaken)
        Log("Shotgun: the %s hand on the bolt handle", kHandName[side]);
    if (r.handleLet) {
        if (r.racked) {
            Log("Shotgun: the handle drawn %.0f mm and let go", r.handlePulled * 1000.0f);
            const shotgunammo::Bolt b = ammo::TubeRack();
            boltHome(b);
            // A live shell thrown out: it flies out of the port to the gun's right and up a little, and falls (loose).
            if (b.threw && skin::LooseOn() && g_haveLooseFrame) {
                float onGun[12], view[12], room[12];
                shellload::ShellInPort(*gun, onGun);
                Compose(t.launcherPose, onGun, view);
                Compose(headPose, view, room);
                const float* base = gun->baseOnBone;
                const float toBone[12] = {1, 0, 0, base[0], 0, 1, 0, base[1], 0, 0, 1, base[2]};  // loose's shell (its base's middle at 0) on the shell's bone
                float caseRoom[12], gunRoom[12];
                Compose(room, toBone, caseRoom);
                Compose(headPose, t.launcherPose, gunRoom);
                const float out[3] = {-1.6f, 0.6f, -0.2f};  // on the gun: to its right, up, a little back (m/s)
                float velocity[3];
                for (int i = 0; i < 3; ++i)
                    velocity[i] = gunRoom[i * 4] * out[0] + gunRoom[i * 4 + 1] * out[1] + gunRoom[i * 4 + 2] * out[2];
                QueueLoose(kLooseShell, loose::SpecFor("12g"), caseRoom, velocity, false, 1, side);
            }
        } else if (r.tugged) {
            Log("Shotgun: the handle tugged off its hold and let go");
            boltHome(ammo::TubeBoltHome());
        } else {
            Log("Shotgun: the handle drawn %.0f mm and let go: %s", r.handlePulled * 1000.0f, ammo::TubeLocked() ? "no tug: the bolt stays held open" : "not far enough");
        }
    }
    static const shellload::Gun* s_lastGun = nullptr;  // (a shell let go of as the gun leaves the hands for another: its own gun's shell still)
    if (gun)
        s_lastGun = gun;
    if (r.letGo) {
        if (skin::LooseOn() && g_haveLooseFrame && s_lastGun) {
            const float* base = s_lastGun->baseOnBone;
            const float toBone[12] = {1, 0, 0, base[0], 0, 1, 0, base[1], 0, 0, 1, base[2]};
            float room[12], caseRoom[12];
            Compose(headPose, r.dropView, room);
            Compose(room, toBone, caseRoom);
            QueueLoose(kLooseShell, loose::SpecFor("12g"), caseRoom, r.dropVelocityRoom, r.dropFromHand, 1, side);
            Log("Shotgun: the shell %s: it falls", r.dropFromHand ? "let go of" : "let go of part way up the loading port");
        } else {
            Log("Shotgun: the shell let go of: gone (%s: nothing to fall with)", skin::LooseOn() ? "loose not stepping yet" : "weapon.txt \"loose 0\"");
        }
    }
    // The last shot left the bolt held open (ammo.cpp said so): felt in the hand on the gun, after recoil's.
    static unsigned s_lockedShots = 0;
    if (const unsigned shots = ammo::TubeLockedShots(); shots != s_lockedShots) {
        s_lockedShots = shots;
        if (live)
            input::Buzz(gunSide, 0.6f, 0.040f);
    }
    // The trigger pulled with nothing to fire (input.cpp held it off): a click, once, with the bolt home on an empty chamber.
    if (g_tubeDryPull.exchange(false, std::memory_order_relaxed) && live) {
        if (ammo::TubePull(r.handleHeld) == shotgunammo::Pull::kClick) {
            PlayTube("dry_m1014");
            input::Buzz(gunSide, 0.15f, 0.015f);
        }
    }
    g_tubeInHand.store(live && t.gun, std::memory_order_relaxed);
    g_tubeBoltHeldNow.store(live && r.handleHeld, std::memory_order_relaxed);
    if (skin::ShellLog() && r.active) {
        static ULONGLONG s_at = 0;
        if (GetTickCount64() - s_at >= 250) {
            s_at = GetTickCount64();
            Log("Shotgun: %s (the %s hand at work; %d in the tube of %d, %s, %s; %d at the belly; the bolt drawn %.0f mm back; the glove on the gun %.2f)", r.state,
                kHandName[side], ammo::TubeShells(), ammo::TubeCapacity(), ammo::TubeChambered() ? "one chambered" : "the chamber empty",
                ammo::TubeLocked() ? "the bolt held open" : "the bolt home", ammo::TubeBelly(), r.boltBack * 1000.0f, r.gloveBlend);
        }
    }
    (void)fromHead;
    return r;
}

// side: the hand on the handguard (0 the left, as before; 1 the right, free hands, with the left on the pistol grip). The trigger hand's turn of the gun as it
// alone holds it (view space), its place on the gun (model space) and where that is (its grip point, view space). steer: the hold points the gun (holding by
// its pistol grip with the other hand); barrelHolder: this hand holds the rifle by its handguard alone, so its grip lets go only while it's read, not in a
// scripted moment and armed (g_barrelArmed: else the rifle would drop with an unread grip, or at once once read again with the grip open).
void Support(skin::HandTargets& t, int side, const float* triggerTurn, const float* triggerAnchor, const float* triggerAt, bool steer, bool barrelHolder,
             const float* headTurn) {
    const float squeeze = SqueezeOf(side);
    bool& supporting = g_supportingBy[side];
    bool& fresh = g_freshBy[side];
    if (squeeze < 0.4f)
        fresh = true;
    // The palm: the left glove's as drawn free, the free right glove's.
    const float* hand = side == 0 ? t.leftWrist : g_rightPalm;
    float inverse[12], palm[3];
    InvertRigid(t.gunPose, inverse);
    Transform(inverse, hand, palm);
    const float apart[3] = {hand[0] - triggerAt[0], hand[1] - triggerAt[1],
                            hand[2] - triggerAt[2]};
    const float between = Length(apart);
    const float distance = FromGuard(palm);
    const ULONGLONG now = GetTickCount64();
    float fromM320 = 0.0f;
    // The launcher's grip by either hand (the right's palm measured as the left's mirrored: InM320Zone's points are the left's); the right's only while its own
    // pose is drawn (freehands.txt "rightpose 0": its mechanics off).
    const float zonePalm[3] = {side == 1 ? -palm[0] : palm[0], palm[1], palm[2]};
    const bool m320Zone = (side == 0 || g_rightWorks) && !supporting && squeeze > 0.6f && fresh && InM320Zone(zonePalm, &fromM320);
    // Squeezed near the handguard, or squeezed and then brought to it; or on the launcher's grip. (Not a hand holding the charging handle.)
    if (!supporting && squeeze > 0.6f && fresh && reload::HandleSide() != side && (distance < kGuardReach || m320Zone) &&
        between > kHandsTogether) {
        supporting = true;
        g_onM320By[side] = m320Zone;
        std::memcpy(g_supportPointBy[side], palm, sizeof(palm));
        g_tookHoldAtBy[side] = now;
        g_nearestHoldBy[side] = distance;
        if (m320Zone) {
            g_m320IndexFitted = false;  // (fitted again to the hold as it's learned now)
            Log("Hands: the %s hand took hold of the grenade launcher's grip (%.0f mm from its wrist point, the grip at %.2f)", kHandName[side], fromM320 * 1000.0f,
                squeeze);
        } else if (side == 1) {
            Log("Hands: the right hand took hold of the handguard (%.0f mm from its line, the grip at %.2f)", distance * 1000.0f, squeeze);
        } else {
            Log("Hands: the left hand took hold of the handguard (%.0f mm from its line, the grip at %.2f)",
                distance * 1000.0f, squeeze);
        }
    } else if (supporting && squeeze < 0.4f &&
               (!barrelHolder || (ReadOf(side) && !g_scripted.load(std::memory_order_relaxed) && g_barrelArmed[side]))) {
        // Only the grip let go lets go (the user: it dropped the gun when swung far right or left,
        // the hands coming within 10 cm; "locked no matter what until I let go", 2026-10-30).
        supporting = false;
        fresh = false;
        Log("Hands: the %s hand let go of the %s (the grip at %.2f)", kHandName[side], g_onM320By[side] ? "grenade launcher's grip" : "handguard", squeeze);
        g_onM320By[side] = false;
    } else if (supporting && !g_onM320By[side] && now - g_tookHoldAtBy[side] < kHoldSettle && distance < g_nearestHoldBy[side]) {
        // Taken on the way in (the grip squeezed before the hand got there: 14 of 23 holds on 2026-10-31
        // were taken 20-25 cm out, where the wrist rests 10-13 cm out): the hold point follows the palm
        // while it still comes nearer, so the gun doesn't swing round as the hand settles (25 degrees).
        g_nearestHoldBy[side] = distance;
        std::memcpy(g_supportPointBy[side], palm, sizeof(palm));
    }
    if (!supporting)
        return;
    g_seenAtBy[side] = now;
    if (g_holdOutOfReachBy[side] || !steer)
        return;  // the glove's off the gun for now: the right hand alone points it (or the rifle's held by this hand alone: nothing to steer)
    // The gun's line from the right hand to where the left took hold, as the right hand points it,
    // and the line between the hands: the steer turns the first onto the second as far as they agree
    // (steer.h; with the hands too close for that line to be steady, the right hand alone points it).
    const float* point = g_supportPointBy[side];
    const float model[3] = {point[0] - triggerAnchor[0], point[1] - triggerAnchor[1],
                            point[2] - triggerAnchor[2]};
    // In the model space: the hold point's line stays the same through a hold.
    GiveSteer(model, apart, between, triggerTurn, headTurn);
}

// The launcher loaded by the left hand (rocket.cpp): X held with it empty, the game's own reload until its count comes up; when it
// was last seen empty (the trigger's held off: input.cpp); when a case went home; the fingers round a case (0-1), and how.
bool g_launcherGameReload = false;
std::atomic<bool> g_launcherEmptyNow{false};
ULONGLONG g_gameReloadSince = 0, g_gameReloadCountedAt = 0;  // the game's own reload: when it started, when its count came up
bool g_gameReloadGaveUp = false;  // it didn't come: the hand can load it, until the launcher's put away
constexpr ULONGLONG kGameReloadTail = 1900;  // its animation goes on this long after its count comes up (smaw.md: 4.6 s and 6.5 s)
constexpr ULONGLONG kGameReloadMost = 8000;  // no count up by then: it isn't reloading
ULONGLONG g_rocketSeatedAt = 0;
float g_rocketFingers = 0.0f;

// The launcher's front grip (the user, 2026-11-02: a magnetic snap on a grip button, on the game's own grip points, free hands,
// "don't compromise any animations"). The left grip squeezed with the left palm within kLauncherReach of where the game's own
// left palm sits takes hold; only the grip let go lets go, as the handguard. The glove eases onto the game's own left glove there, as
// its animation has it just now (Update), and points the launcher as it does the M416 (steer.h; weapon.txt "launchersteer 0" not).
bool g_heldDry = false;     // a gun with no button (handmag.cpp): run dry, its chamber empty until the lever's pulled
int g_heldLoadedWas = -1;   // its count last frame
uint32_t g_heldFiringWas = 0;  // ... of the hands' slot's firing state then (pickrules::SeeDry: another one there is another gun, never rounds fired)

bool g_beltBlocked = false; // a belt fed gun with its lid up or its belt not in the feed
bool g_notchBlocked = false; // a handle resting in its notch (the UMP-45's): the bolt held open
std::atomic<bool> g_stingerEmptyNow{false};  // the Stinger with no missile ready (flap.cpp)
std::atomic<bool> g_at4SpentNow{false};      // the AT4 in hand with its rocket gone (its own count at none) ...
holster::TubeLatch g_at4Tube;                // ... kept while it's drawn (holster.h's TubeSpent; free hands asleep: forgotten)
float g_heldFist = 0.0f;    // 0 the hand as it is, 1 a fist on its lever
float g_heldFistBy1 = 0.0f; // ... the right's, a held gun's lever by the right (RightGlove)
constexpr float kLauncherFrontGrip[3] = {0.039f, -0.073f, 0.315f};
constexpr float kLauncherReach = 0.25f;
// By side ([0] the left under the old names; [1] the right, free hands, while the left holds the pistol grip): on its front grip, the palm when it took
// hold (on the gun's frame), and the nearest it's come since.
bool g_onLauncherBy[2] = {};
bool& g_onLauncher = g_onLauncherBy[0];
float g_launcherHoldPointBy[2][3] = {};
float (&g_launcherHoldPoint)[3] = g_launcherHoldPointBy[0];
float g_nearestLauncherBy[2] = {};
float& g_nearestLauncher = g_nearestLauncherBy[0];

// The launcher's or a held gun's front grip, by either hand (side 0 the left, as before; 1 the right, while the left holds its pistol grip). The trigger hand's
// turn of the gun, its place on it and where that is (as Support's); front: this side's place (the game's left palm, the right's mirrored); steer: the hold
// points it; barrelHolder: this hand holds it by the front grip alone (its let-go only while read, not scripted and armed, as Support's).
void LauncherSupport(skin::HandTargets& t, int side, const float* triggerTurn, const float* triggerAnchor, const float* triggerAt, const float* front,
                     bool steer, bool barrelHolder, const float* headTurn) {
    const float squeeze = SqueezeOf(side);
    bool& on = g_onLauncherBy[side];
    bool& fresh = g_freshBy[side];
    if (squeeze < 0.4f)
        fresh = true;
    const float* hand = side == 0 ? t.leftWrist : g_rightPalm;  // the palm: the left glove's as drawn free, the free right glove's
    float inverse[12], palm[3];
    InvertRigid(t.launcherPose, inverse);
    Transform(inverse, hand, palm);
    const float apart[3] = {hand[0] - triggerAt[0], hand[1] - triggerAt[1], hand[2] - triggerAt[2]};
    const float between = Length(apart);
    const float off[3] = {palm[0] - front[0], palm[1] - front[1], palm[2] - front[2]};
    const float distance = Length(off);
    const ULONGLONG now = GetTickCount64();
    if (!on && squeeze > 0.6f && fresh && distance < kLauncherReach) {
        on = true;
        fresh = false;
        std::memcpy(g_launcherHoldPointBy[side], palm, sizeof(palm));
        g_tookHoldAtBy[side] = now;
        g_nearestLauncherBy[side] = distance;
        Log("Hands: the %s hand took hold of the launcher's front grip (%.0f mm from it, the grip at %.2f)", kHandName[side], distance * 1000.0f, squeeze);
    } else if (on && squeeze < 0.4f && (!barrelHolder || (ReadOf(side) && !g_scripted.load(std::memory_order_relaxed) && g_barrelArmed[side]))) {
        on = false;
        fresh = false;
        Log("Hands: the %s hand let go of the launcher's front grip (the grip at %.2f)", kHandName[side], squeeze);
    } else if (on && now - g_tookHoldAtBy[side] < kHoldSettle && distance < g_nearestLauncherBy[side]) {
        g_nearestLauncherBy[side] = distance;
        std::memcpy(g_launcherHoldPointBy[side], palm, sizeof(palm));
    }
    if (!on || !skin::LauncherSteer() || g_holdOutOfReachBy[side] || !steer)
        return;
    g_seenAtBy[side] = now;
    const float* point = g_launcherHoldPointBy[side];
    const float model[3] = {point[0] - triggerAnchor[0], point[1] - triggerAnchor[1], point[2] - triggerAnchor[2]};
    GiveSteer(model, apart, between, triggerTurn, headTurn);
}

// The train pistol of The Great Destroyer in both hands. The user, 2026-10-31: the game's own M9 hold,
// its left hand round the right, is "the perfect hand model for a two-handed mode ... only for two-handed
// mode". The left grip squeezed with the left palm within kPistolReach of where the game's left palm sits
// on the pistol takes hold; only the grip let go lets go, as the handguard (Support). The left hand eases
// onto the game's hold and back over 5 frames, its fingers with it (Update: t.bareLeftFingers); the right
// hand alone points the pistol. The controllers can't overlap as the game's hands do: 22 cm takes hold
// with them touching.
constexpr float kPistolReach = 0.22f;
bool g_pistolSupport = false;
float g_pistolBlend = 0.0f;  // 0 the left hand where its controller is, 1 on the pistol
float g_pistolLeftHold[12] = {};  // the game's left palm from the pistol's frame (skin::BarePistolLeft)
float g_pistolLeftFingers[handpose::kBones][12] = {};
bool g_havePistolLeft = false;
bool SlideHeld();  // the left hand has the pistol's slide (PistolSlide, below)

// pistol: the pistol in the right hand; tracked: the left hand as its controller holds it.
void PistolSupport(skin::HandTargets& t, bool pistol, bool trackedLeft, const float* tracked) {
    g_havePistolLeft = pistol && skin::BarePistolLeft(g_pistolLeftHold, g_pistolLeftFingers);
    const float squeeze = input::LeftSqueeze();
    if (squeeze < 0.4f)
        g_freshSqueeze = true;
    float hold[12] = {};
    if (g_havePistolLeft)
        Compose(t.pistolPose, g_pistolLeftHold, hold);
    // weapon.txt "pistoltest 1": the stand-in left hand (no left controller) holds it, with no squeeze.
    static bool s_byTest = false;
    const bool test = !trackedLeft && skin::PistolTest();
    if (!g_pistolSupport && g_havePistolLeft && test) {
        g_pistolSupport = s_byTest = true;
        Log("Hands: the stand-in left hand took hold of the pistol (pistoltest)");
    } else if (!g_pistolSupport && squeeze > 0.6f && g_freshSqueeze && SlideHeld()) {
        // The left hand on the slide (PistolSlide, pistol.txt "slide 1"): a squeeze then is the hand closing on it, not
        // a hold, as the handguard's isn't with the charging handle held. Nor once the slide's let go with the grip
        // still squeezed: 22 cm reaches anywhere on the pistol, so the hand would go off its controller onto the
        // game's hold. Let go and squeezed again, it takes hold.
        g_freshSqueeze = false;
        Log("Hands: the left grip squeezed with the slide in the left hand: not a hold of the pistol");
    } else if (!g_pistolSupport && squeeze > 0.6f && g_freshSqueeze && pistolmag::Holding()) {
        // The pistol's magazine in the left hand or part way into the grip (PistolMagazine, pistol.txt "mag 1"): a
        // squeeze then is the hand closing on it, or on its way to push it home, not a hold (22 cm reaches the grip's
        // bottom too). Once it's home, dropped or fallen, let go and squeezed again, it takes hold.
        g_freshSqueeze = false;
        Log("Hands: the left grip squeezed with the pistol's magazine in the left hand or part way into the grip: not "
            "a hold of the pistol");
    } else if (!g_pistolSupport && g_havePistolLeft && trackedLeft && squeeze > 0.6f && g_freshSqueeze) {
        float at[3];
        Transform(hold, g_leftAnchor, at);
        const float d[3] = {at[0] - t.leftWrist[0], at[1] - t.leftWrist[1], at[2] - t.leftWrist[2]};
        if (Length(d) < kPistolReach) {
            g_pistolSupport = true;
            s_byTest = false;
            Log("Hands: the left hand took hold of the pistol (%.0f mm from the game's left hand on it, the grip at %.2f)",
                Length(d) * 1000.0f, squeeze);
        }
    } else if (g_pistolSupport && (!g_havePistolLeft || (s_byTest ? !test : squeeze < 0.4f))) {
        g_pistolSupport = false;
        g_freshSqueeze = false;
        Log("Hands: the left hand let go of the pistol (%s)",
            !g_havePistolLeft ? "no pistol" : s_byTest ? "pistoltest off" : "the grip let go");
    }
    g_pistolBlend = g_pistolSupport ? std::fmin(g_pistolBlend + kGripBlendStep, 1.0f)
                                    : std::fmax(g_pistolBlend - kGripBlendStep, 0.0f);
    if (g_pistolBlend > 0.0f && g_havePistolLeft) {
        float blended[12];
        if (trackedLeft)
            BlendAbout(tracked, hold, g_pistolBlend, g_leftAnchor, blended);
        else
            std::memcpy(blended, hold, sizeof(blended));  // the left controller not tracked: held as it is
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        t.left = true;
    }
}

// The train pistol's manual reload (docs\research\pistol-manual-reload.md), built a step at a time behind
// build\pistol.txt, re-read when it changes: "reload on" turns it on ("reload off" wins; no file, or no "reload on":
// off, and nothing changes). Its own, nothing the M416's. Drawn by skin.cpp: the slide and the magazine by the left
// hand, and tests with nobody in the headset (the pistol in the stand-in's right hand or the controller's). With "mag 1"
// its rounds are the mod's too (ammo.cpp's pistol, pistolammo.h; "rounds 0": the rehearsal as it was, no rounds change).
// Each is logged as the file is read and does nothing without "reload on" (lengths in cm):
//   "slide 1"       the slide worked by the left hand (PistolSlide): the left trigger pressed near its rear grabs it in
//                   one of three grips, by how the hand is turned (overhand, the fingers over its right side; the
//                   fingers over its left side; a pinch from behind), the hand drawn gliding onto it; it rides back
//                   with the hand and springs home when let go; while it's held the right trigger does nothing. With
//                   the rounds by hand a rack feeds a round (one in the chamber is thrown out, lost), and a tug lets
//                   it off its stop ("slide 0", or no "slide": not worked; the trigger then cycles it when it must)
//   "grab 13 0.3 0.12"  the slide's grab: the palm within this of where a grip holds its rear (13 cm by default; the
//                   first headset test's presses missed at 7.1-9.9 cm with 7); a press made before the hand gets there
//                   waits this many seconds for it, the trigger held (0.3; 0: not); the hand drawn glides on, and eases
//                   back off after the let go, over this many seconds (0.12; longer for a hand turned more than 90
//                   degrees from where it goes)
//   "mag 1"         the magazine by hand (PistolMagazine), and its rounds with it ("rounds", below). X drops it out of the grip
//                   (one part way in: that one); the left grip squeezed over the left shoulder takes one, carried in the hand;
//                   brought near the grip it's caught (magnetic, from any direction) and glides in, the hand drawn on
//                   it going over to the flat base under it; the hand pushes it on and it snaps home. X held is still
//                   the game's own reload ("mag 0", or no "mag": as before)
//   "rounds 0"      with "mag 1": the rehearsal as it was, no rounds change, the game's own reload runs the ammunition
//                   and the right trigger does nothing while the magazine is out. Without it ("rounds 1", the default)
//                   the rounds are real: the game never reloads the pistol by itself, firing counts the magazine down,
//                   the last round leaves the slide open on its stop and the trigger fires nothing, the magazine out
//                   takes its rounds back to the spare ones and leaves the chambered one (which still fires), a fresh
//                   one seated brings its rounds from the spare ones, and the slide (the trigger's next pull off its
//                   stop, a tug or a rack by hand) chambers one
//   "carry g2"      the carry: G2, every finger on it, the index along its front (the default, the user's choice);
//                   "carry c2", the hand closed round its base, or "carry game", the game's own pinch (to compare)
//   "catch 7.5 55"  the magnetic catch: the magazine's top within this of the grip's mouth (or already in the grip),
//                   its length within that many degrees of the grip's line (7.5 cm, 55 degrees by default)
//   "glide 0.15"    how long each eased move takes, in seconds: the glide in, the snap home, the hand drawn going over
//                   to the flat base and back to the controller (0.15 by default)
//   "seat 6.5"      the snap: once the hand's push has held the magazine's top within this of home for two frames
//                   running, it goes home by itself (the hands can't close up under the grip as the game's do: the
//                   review); each try at the grip is logged with how near the push brought it, to set this from (6.5 by
//                   default: the first headset test's came 2.1-3.7 cm short)
//   "dwell 0.4"     held on the grip's line this long with the grip squeezed, it goes home by itself too ("dwell 0":
//                   only when pushed within the snap; 0.4 by default)
//   "loadtest"      with "mag 1" and no left controller tracked: a scripted left hand reloads the pistol every 6 s (X,
//                   a magazine taken 40 cm down the grip's line, up towards the grip, caught, pushed home); "loadtest park": it lets go
//                   once it's caught, leaving it part way in, drops away and comes back to push it home
//   "racktest"      with "slide 1" and no left controller tracked: a scripted left hand racks the slide every 3 s
//   "slidetest"     the slide going back and forth over its whole travel, 2 s a cycle
//   "locktest 4.1"  the slide held open that far back, where the slide stop will hold it (4.1 by default; over
//                   slidetest; with "slide 1" the hand grabs it there, and let go it goes back onto the stop)
//   "magtest 15"    a magazine held on the grip's line that far below home (15 by default, its top 4 cm under the
//                   grip; 0 seated where the game's own sits, to see the seat; 5 part way in), the grip empty
//   "droptest"      once a save: the magazine drops as X will drop it, out of the grip along its line (the grip then
//                   empty for 2 s), or with magtest, the one held there from where it's drawn (then not drawn again
//                   until the next save); "droptest hand": magtest's let go of, falling from rest
bool g_pistolReload = false;
bool g_pistolSlideOn = false;  // "slide 1", with "reload on"
bool g_pistolMagOn = false;    // "mag 1", with "reload on"
bool g_pistolRoundsOn = false; // ... and not "rounds 0": its rounds by hand (ammo.cpp's pistol), not only drawn
struct PistolTests {
    bool slide = false;
    float lock = -1.0f;       // m back (below 0: off)
    bool mag = false;
    float magAlong = -0.15f;  // its top's place along the grip's line from home (m)
    int drop = 0;             // a drop waiting: 1 as X, 2 let go of
    bool rack = false;
    int load = 0;             // loadtest: 1 home in one go, 2 left part way and pushed on
};
PistolTests g_pistolTests;
bool g_magTestGone = false;      // magtest's magazine dropped: not drawn again until the next save
ULONGLONG g_gripEmptyUntil = 0;  // droptest's drop out of the grip leaves it empty until then
// The slide's whole travel (the game's own shots, recorded: 4.57 cm) and where the slide stop holds it open (the
// design's 4.1 cm: the game never shows it, so it's to be set by eye with locktest).
constexpr float kPistolSlideTravel = 0.0457f, kPistolSlideLock = 0.041f;
// The slide by the left hand (PistolSlide, below; pistol.txt "grab" tunes its grab).
pistolslide::Slide g_pistolSlide;

// The carry, the magazine in the hand away from the grip (pistol.txt "carry"): the fingers round it (pistolposes.h's
// bones, going over to the flat base's once it's caught), where it sits on the palm (nullptr: the flat base's own
// place; otherwise the hand drawn on it goes over to the flat base about its floorplate once it's caught:
// pistolmag::SetCarry) and how far past the grip's mouth its fingers let it go before that's done (0: never in the
// way). The user chose G2, every finger and the thumb on it, the index along its front (2026-10-31: the default); C2,
// closed round its base ("great"), and the game's pinch (C1: "Holding it with one finger is not doable") are kept.
struct PistolCarry {
    const char* name;
    const float (*fingers)[12];
    const float* hold;
    float guide;
};
const PistolCarry kCarryIndex{"G2, the index along its front", pistolposes::kCarryG2, pistolposes::kOnPalmG2,
                              pistolposes::kGuideG2};
const PistolCarry kCarryClosed{"C2, closed round its base", pistolposes::kCarry, nullptr, 0.0f};
const PistolCarry kCarryGame{"the game's own pinch (C1)", pistolposes::kGamePinch, pistolposes::kOnPalmGamePinch, 0.0f};
const PistolCarry* g_pistolCarry = &kCarryIndex;
// The pistol's own magazine out of the grip, as the mod draws it (PistolMagazine): X drops it, one pushed home by hand
// or the game's own reload (X held) puts it back.
bool g_pistolMagOut = false;

void ReadPistolTuning() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadPistolTuning), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"pistol.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"r") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next check
    lastWrite = info.ftLastWriteTime;
    bool on = false, off = false, slide = false, mag = false, rounds = true;
    const PistolCarry* carry = &kCarryIndex;
    char unknownCarry[64] = "";
    float snap = 0.065f, reach = 0.075f, angle = 55.0f, glide = 0.15f, dwell = 0.4f;
    const pistolslide::Profile grab;  // the slide's grab as it is unless "grab" says
    float slideReach = grab.reach, slideWait = grab.wait, slideGlide = grab.glide;
    PistolTests tests;
    if (f) {
        char line[512];
        // A line longer than the buffer comes in pieces: only its first is read (the rest of a long comment would
        // otherwise be read as a switch of its own: the final review found "loadtest" there).
        bool piece = false;
        while (fgets(line, sizeof(line), f)) {
            const bool more = piece;
            piece = !std::strchr(line, '\n') && !feof(f);
            if (more)
                continue;
            const char* rest = nullptr;
            float cm = 0.0f;
            if (Word(line, "reload", &rest)) {
                if (Word(rest, "on"))
                    on = true;
                else if (Word(rest, "off"))
                    off = true;
            } else if (Word(line, "slide", &rest)) {  // "slide 1" (or "on", or alone); "slide 0" / "slide off"
                slide = !Word(rest, "0") && !Word(rest, "off");
            } else if (Word(line, "mag", &rest)) {  // "mag 1" (or "on", or alone); "mag 0" / "mag off"
                mag = !Word(rest, "0") && !Word(rest, "off");
            } else if (Word(line, "rounds", &rest)) {  // "rounds 0" / "rounds off": the rehearsal, no rounds change
                rounds = !Word(rest, "0") && !Word(rest, "off");
            } else if (Word(line, "carry", &rest)) {
                // "carry g2" (or "index"), "carry c2" (or "closed"), "carry game"; anything else: the default, said so.
                if (Word(rest, "game")) {
                    carry = &kCarryGame;
                } else if (Word(rest, "c2") || Word(rest, "closed")) {
                    carry = &kCarryClosed;
                } else if (Word(rest, "g2") || Word(rest, "index")) {
                    carry = &kCarryIndex;
                } else {
                    carry = &kCarryIndex;
                    sscanf_s(rest, "%63s", unknownCarry, static_cast<unsigned>(sizeof(unknownCarry)));
                }
            } else if (Word(line, "seat", &rest)) {
                if (sscanf_s(rest, "%f", &cm) == 1 && cm >= 0.0f)
                    snap = cm / 100.0f;  // (pistolmag::SetSnap keeps it to 0.5-10 cm)
            } else if (Word(line, "catch", &rest)) {
                // "catch 7.5 55": the reach in cm, then (if given) the angle in degrees.
                float degrees = 0.0f;
                const int read = sscanf_s(rest, "%f %f", &cm, &degrees);
                if (read >= 1 && cm >= 0.0f)
                    reach = cm / 100.0f;
                if (read == 2 && degrees >= 0.0f)
                    angle = degrees;
            } else if (Word(line, "glide", &rest)) {
                float seconds = 0.0f;
                if (sscanf_s(rest, "%f", &seconds) == 1 && seconds >= 0.0f)
                    glide = seconds;
            } else if (Word(line, "dwell", &rest)) {
                float seconds = 0.0f;
                if (sscanf_s(rest, "%f", &seconds) == 1 && seconds >= 0.0f)
                    dwell = seconds;
            } else if (Word(line, "grab", &rest)) {
                // "grab 13 0.3 0.12": the slide's reach in cm, then (if given) the wait and the glide in seconds.
                float wait = 0.0f, seconds = 0.0f;
                const int read = sscanf_s(rest, "%f %f %f", &cm, &wait, &seconds);
                if (read >= 1 && cm >= 0.0f)
                    slideReach = cm / 100.0f;
                if (read >= 2 && wait >= 0.0f)
                    slideWait = wait;
                if (read == 3 && seconds >= 0.0f)
                    slideGlide = seconds;
            } else if (Word(line, "loadtest", &rest)) {
                tests.load = Word(rest, "park") ? 2 : (Word(rest, "0") || Word(rest, "off")) ? 0 : 1;
            } else if (Word(line, "racktest", &rest)) {
                tests.rack = !Word(rest, "0") && !Word(rest, "off");
            } else if (Word(line, "slidetest")) {
                tests.slide = true;
            } else if (Word(line, "locktest", &rest)) {
                tests.lock = kPistolSlideLock;
                if (sscanf_s(rest, "%f", &cm) == 1)
                    tests.lock = std::fmin(std::fmax(cm / 100.0f, 0.0f), kPistolSlideTravel);
            } else if (Word(line, "magtest", &rest)) {
                tests.mag = true;
                if (sscanf_s(rest, "%f", &cm) == 1)
                    tests.magAlong = -std::fmin(std::fmax(cm / 100.0f, 0.0f), 0.5f);
            } else if (Word(line, "droptest", &rest)) {
                tests.drop = Word(rest, "hand") ? 2 : 1;
            }
        }
        fclose(f);
    }
    g_pistolReload = on && !off;
    g_pistolSlideOn = g_pistolReload && slide;
    g_pistolTests = g_pistolReload ? tests : PistolTests{};
    g_magTestGone = false;
    g_gripEmptyUntil = 0;
    // The magazine by hand off: nothing of it is left (one in the hand is gone, the grip drawn as the game has it).
    const bool wasMag = g_pistolMagOn;
    g_pistolMagOn = g_pistolReload && mag;
    g_pistolRoundsOn = g_pistolMagOn && rounds;
    if (!g_pistolMagOn) {
        if (wasMag && (pistolmag::Holding() || g_pistolMagOut))
            Log("Pistol reload: the magazine by hand off: the one in the hand is gone, the grip drawn full again");
        pistolmag::Clear();
        g_pistolMagOut = false;
    }
    g_pistolCarry = carry;
    pistolmag::SetCarry(carry->hold, carry->guide);
    pistolmag::SetCatch(reach, angle);
    pistolmag::SetGlide(glide);
    pistolmag::SetSnap(snap);
    pistolmag::SetDwell(dwell);
    g_pistolSlide.Tune(slideReach, slideWait, slideGlide);
    if (!g_pistolReload) {
        Log("Pistol reload: off, the train pistol as before%s", exists ? " (build\\pistol.txt)" : "");
        return;
    }
    const PistolTests& p = g_pistolTests;
    Log("Pistol reload: on with the train pistol in hand (build\\pistol.txt); %s: %s%s%s",
        g_pistolRoundsOn ? "its rounds by hand (the game never reloads it by itself)" : "only drawn, no rounds change",
        g_pistolSlideOn ? "the slide by the left hand (the left trigger at its rear)" : "",
        g_pistolSlideOn && g_pistolMagOn ? ", " : "",
        g_pistolMagOn      ? "the magazine by hand (X, the left shoulder, the grip)"
        : !g_pistolSlideOn ? "by its tests"
                           : "");
    if (g_pistolSlideOn) {
        const pistolslide::Profile& s = g_pistolSlide.Settings();
        char waits[96];
        if (s.wait > 0.0f)
            std::snprintf(waits, sizeof(waits), "a press made before the hand gets there waits %.2f s for it", s.wait);
        else
            std::snprintf(waits, sizeof(waits), "a press doesn't wait for the hand (wait 0)");
        Log("Pistol reload: slide 1: the left trigger grabs the slide with the palm within %.1f cm of where one of its "
            "grips holds its rear (S1 overhand, S2 the fingers to the left, S3 the pinch from behind: the one the hand "
            "is turned for); %s; the hand drawn glides on and eases back off over %.2f s (grab; longer for a big turn)",
            s.reach * 100.0f, waits, s.glide);
    }
    if (g_pistolMagOn) {
        const pistolmag::Tuning m = pistolmag::Settings();
        char dwelt[64];
        if (m.dwell > 0.0f)
            std::snprintf(dwelt, sizeof(dwelt), "or held there gripped %.1f s (dwell)", m.dwell);
        else
            std::snprintf(dwelt, sizeof(dwelt), "and never only held there (dwell 0)");
        Log("Pistol reload: mag 1: X drops the magazine; the left grip at the belly takes one, carried %s; near the "
            "grip it's caught and glides in, the hand drawn going over to the flat base under it; the hand pushes it "
            "on; %s, X held is the game's own reload",
            g_pistolCarry->name,
            g_pistolRoundsOn ? "its rounds go and come with it, the chambered round stays and fires, empty the slide stays open and "
                               "the trigger fires nothing (\"rounds 0\": the rehearsal)"
                             : "the right trigger held off while it's out; no rounds change (rounds 0: a rehearsal)");
        Log("Pistol reload: the magazine caught within %.1f cm of the grip's mouth, its length within %.0f degrees of "
            "the grip's line (catch); each eased move %.2f s (glide); home by itself once pushed within %.1f cm (seat) "
            "%s",
            m.reach * 100.0f, m.angle, m.glide, m.snap * 100.0f, dwelt);
    }
    if (*unknownCarry)
        Log("Pistol reload: carry \"%s\" isn't known: %s (\"carry g2\", \"carry c2\" or \"carry game\")", unknownCarry,
            g_pistolCarry->name);
    if (p.load)
        Log("Pistol reload: loadtest%s: %s", p.load == 2 ? " park" : "",
            !g_pistolMagOn ? "does nothing without \"mag 1\""
            : p.load == 2  ? "a scripted left hand reloads every 6 s while no left controller is tracked, letting go "
                             "once the magazine's caught (it's left part way in), then pushing it home"
                           : "a scripted left hand reloads every 6 s while no left controller is tracked");
    if (p.rack)
        Log("Pistol reload: racktest: %s",
            !g_pistolSlideOn          ? "does nothing without \"slide 1\""
            : p.load && g_pistolMagOn ? "left out: loadtest has the scripted left hand"
                                      : "a scripted left hand racks the slide every 3 s while no left controller is "
                                        "tracked");
    if (p.slide && p.lock < 0.0f)
        Log("Pistol reload: slidetest: the slide drawn going back and forth over its %.2f cm travel",
            kPistolSlideTravel * 100.0f);
    if (p.lock >= 0.0f)
        Log("Pistol reload: locktest: the slide drawn held open %.1f cm back%s", p.lock * 100.0f,
            p.slide ? " (over slidetest)" : "");
    if (p.mag)
        Log("Pistol reload: magtest: a magazine drawn held %.1f cm down the grip's line from home, the grip empty",
            -p.magAlong * 100.0f);
    if (p.drop)
        Log("Pistol reload: droptest: the magazine drops once, %s",
            !p.mag        ? "out of the grip as X will"
            : p.drop == 2 ? "magtest's let go of"
                          : "magtest's along its line as X will");
}

// The pistol's rounds by hand (pistol.txt "mag 1" without "rounds 0") and counted by the mod just now (ammo.cpp's pistol:
// found once it's in the hand). False: the rehearsal's rules, as before.
bool PistolRoundsLive() {
    return g_pistolRoundsOn && ammo::PistolLive();
}
// The right trigger pulled with nothing in the chamber (input.cpp's thread: PistolDryPull), for PistolRounds.
std::atomic<bool> g_pistolDryPull{false};

// Each frame with the rounds by hand, before the magazine and the slide: the grip's magazine as the mod counts it (a new
// life or the game's own reload, X held, puts one in), and the trigger's pull on an empty chamber: with the slide on its
// stop and a fresh magazine in it only sends the slide home, which chambers a round (the user's answer: the next pull
// fires); with the slide not worked by hand ("slide 0") it also cycles it from home when it must; otherwise nothing.
void PistolRounds(bool pistolHeld) {
    const bool pulled = g_pistolDryPull.exchange(false, std::memory_order_relaxed);
    if (!PistolRoundsLive())
        return;
    const bool out = !ammo::PistolMagIn();
    if (out != g_pistolMagOut) {
        g_pistolMagOut = out;
        Log("Pistol reload: the grip's magazine %s, as its rounds are counted", out ? "out" : "in");
    }
    if (!pulled || !pistolHeld)
        return;
    const pistolammo::Pull pull = ammo::PistolPull(g_pistolSlideOn);
    if (pull == pistolammo::Pull::kSendsHome || pull == pistolammo::Pull::kRacks) {
        if (g_pistolSlideOn)
            g_pistolSlide.Release();  // off its stop it springs home; from home it cycles as a shot does
        sounds::Play("pistol_slide");
    }
}

// The manual reload's targets for skin.cpp, only with the train pistol in the right hand and pistol.txt "reload on":
// otherwise HandTargets keeps none of them, and the pistol is drawn as before. Here its tests' (the slide by hand
// is PistolSlide's, after this).
void PistolReload(skin::HandTargets& t, bool pistolHeld) {
    if (!g_pistolReload || !pistolHeld)
        return;
    t.pistolReload = true;
    const PistolTests& test = g_pistolTests;
    const ULONGLONG now = GetTickCount64();
    if (test.lock >= 0.0f) {
        t.pistolSlideBack = test.lock;
    } else if (test.slide) {
        const float phase = static_cast<float>(now % 2000) / 2000.0f;
        t.pistolSlideBack = kPistolSlideTravel * 0.5f * (1.0f - std::cos(6.2831853f * phase));
    } else if (PistolRoundsLive() && ammo::PistolLocked()) {
        // Empty, the slide on its stop (the game's own always goes home: it still has its hidden round). With the slide by
        // hand PistolSlide draws it, there or springing; this is for "slide 0".
        t.pistolSlideBack = kPistolSlideLock;
    }
    // magtest: straight down the grip's line (the seat's own y, in the frame's space) from home.
    if (test.mag && !g_magTestGone) {
        float seat[12];
        skin::PistolMagazineSeat(seat);
        const float s = test.magAlong;
        const float along[12] = {1, 0, 0, seat[1] * s, 0, 1, 0, seat[5] * s, 0, 0, 1, seat[9] * s};
        Compose(t.pistolPose, along, t.pistolMagPose);
        t.pistolMag = true;
    }
    t.pistolMagOut = test.mag || now < g_gripEmptyUntil;
    // droptest: as X will (a later step) or as the hand lets go, from where it was drawn the frame before; it waits
    // until it's been drawn there.
    if (test.drop) {
        using skin::PistolDrop;
        const PistolDrop from = !t.pistolMag     ? PistolDrop::kGrip
                                : test.drop == 2 ? PistolDrop::kHand
                                                 : PistolDrop::kWell;
        if (skin::DropPistolMagazine(from)) {
            g_pistolTests.drop = 0;
            if (from == PistolDrop::kGrip) {
                g_gripEmptyUntil = now + 2000;
                t.pistolMagOut = true;
            } else {
                g_magTestGone = true;  // falling, not held
                t.pistolMag = false;
            }
        }
    }
}

// The slide worked by the left hand (pistolslide.cpp; pistol.txt "slide 1"): a fresh press of the left trigger with
// the left palm within 13 cm of where one of its three grips holds its rear grabs it, or one made up to 0.3 s before
// the hand gets there with the trigger held (the user: the left trigger, as the M416's charging handle; after the
// first headset test, where presses 7.1-9.9 cm off missed, "make that as magnetic as possible"), in the grip the hand
// is turned for: overhand from the left, the fingers over its left side from the right, or a pinch from behind ("One
// from the left side, one aiming on the right side ... and one maybe in the middle"). The hand drawn glides onto it
// and rides it back (OnTheSlide), and let go it springs home while the hand eases back to the controller (pistol.txt
// "grab"). While the slide's held the right trigger does nothing (input.cpp, hands::PistolSlideHeld). With the rounds
// by hand a rack feeds a round and a tug lets it off its stop (ammo::PistolRack); otherwise it's only drawn. The two-handed hold (PistolSupport, the left grip by the game's left hand) stays
// as it is: the left trigger at the slide takes the slide, drawn over that hold.
pistolslide::Output g_slideOut;  // this frame's: the hand drawn on it and its grip (OnTheSlide; the fingers in Update)
float g_slideShare = 0.0f;       // how far the hand is drawn on it this frame (0-1; 0 without the pistol in hand)
std::atomic<ULONGLONG> g_slideHeldAt{0};  // when the hand last had the slide (0: let go), for PistolSlideHeld
// The bare left palm bone's (41) vertex middle in the arms' bind space: the grab goes by it, as pistolslide's grips'
// holds do, and the hand drawn gliding onto the slide and back off turns about it, so the palm goes straight (about the
// glove's anchor, 5 cm off, a hand turned the other way round from its grip swung the palm 3.5 cm a frame:
// tools\pistolslidetest.cpp).
constexpr float kLeftPalmCentre[3] = {0.5182f, 1.1075f, 0.1756f};
// The grips' names, in pistolposes.h kSlideGrips' order (the log).
const char* const kSlideGripNames[pistolslide::kGrips] = {"S1 overhand", "S2 the fingers to the left",
                                                           "S3 the pinch from behind"};

bool SlideHeld() {
    return g_pistolSlide.Held();
}

// pistolHeld: the train pistol in the right hand; tracked: the left hand as its controller holds it (before any hold);
// busy: why the hand can't take the slide now (nullptr: it can).
void PistolSlide(skin::HandTargets& t, bool pistolHeld, bool trackedLeft, const float* tracked, const char* busy,
                 float dt) {
    const bool on = g_pistolSlideOn && pistolHeld;
    pistolslide::Input in;
    in.dt = dt;
    in.pistolPose = on ? t.pistolPose : nullptr;
    in.trigger = input::LeftTrigger();
    // locktest stands in for the slide stop: held open there, the hand grabs it there, and let go it goes back on it.
    // With the rounds by hand the stop is the pistol's own (empty with a magazine in: ammo.cpp).
    const bool live = PistolRoundsLive();
    in.locked = on && (g_pistolTests.lock >= 0.0f || (live && ammo::PistolLocked()));
    g_pistolSlide.Settings().lockAt = on && g_pistolTests.lock >= 0.0f ? g_pistolTests.lock : kPistolSlideLock;
    const char* noHand = !trackedLeft ? "the left controller isn't tracked" : busy;
    if (on && g_pistolTests.rack && !(g_pistolTests.load && g_pistolMagOn) && !trackedLeft) {
        // racktest, 3 s a cycle: a scripted left hand overhand on the slide's rear (S1) with the trigger let go
        // (0.6 s), the trigger pressed and the hand drawn 5 cm back (0.6 s: the slide stops at its 4.57), held there
        // (0.3 s), let go (the slide springs home out of the hand), and back onto the slide's rear (1.5 s). It's drawn
        // there, and the slide goes by it and the script's trigger, never the left controller's last buttons.
        const float s = static_cast<float>(GetTickCount64() % 3000) / 1000.0f;
        auto ease = [](float u) { return u * u * (3.0f - 2.0f * u); };
        float pull = 0.0f;
        if (s >= 1.5f)
            pull = 0.05f * (1.0f - ease((s - 1.5f) / 1.5f));
        else if (s >= 1.2f)
            pull = 0.05f;
        else if (s >= 0.6f)
            pull = 0.05f * ease((s - 0.6f) / 0.6f);
        in.trigger = s >= 0.6f && s < 1.5f ? 1.0f : 0.0f;
        const float back[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -pull};
        float slid[12];
        Compose(t.pistolPose, back, slid);
        Compose(slid, pistolposes::kSlidePalm, t.leftPose);
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        t.left = true;
        in.hand = t.leftPose;
        noHand = nullptr;
    } else if (!noHand) {
        in.hand = tracked;
    }
    const pistolslide::Output o = g_pistolSlide.Update(in);
    const pistolslide::Profile& p = g_pistolSlide.Settings();
    const char* lost = noHand ? noHand : "the left hand's place isn't known";
    // One line a press: grabbed at once, grabbed as the palm came within reach while it waited, or why not (at once
    // with no palm, otherwise once it's done waiting), with the left grip and the two-handed hold (the slide can be
    // grabbed in both hands, as the M416's handle with the handguard held: a fist taking that hold near the slide's
    // rear grabs it too, and the right trigger then waits for the left trigger's let go).
    const float squeeze = input::LeftSqueeze();
    const char* both = g_pistolSupport ? ", the pistol in both hands" : "";
    const char* stop = in.locked ? " on the slide stop (locktest)" : "";
    // Grabbed in a grip the hand isn't turned for (the one it is out of reach: a press with the hand still there, or
    // stopped short of that one's hold while the press waited for it): which.
    char turned[96] = "";
    if (o.grabbed && o.turnedFor >= 0 && o.turnedFor != o.grip)
        std::snprintf(turned, sizeof(turned), " (turned for %s, its hold out of reach)", kSlideGripNames[o.turnedFor]);
    if (o.grabbed && o.pressed)
        Log("Pistol reload: the left trigger pressed %.0f mm from the slide's hold for %s (the left grip at %.2f%s): "
            "the slide grabbed%s, the hand's turn %.0f degrees off the grip's%s; it glides on over %.2f s",
            o.palmFrom * 1000.0f, kSlideGripNames[o.grip], squeeze, both, stop, o.turnedOff, turned, o.glideFor);
    else if (o.grabbed)
        Log("Pistol reload: the slide grabbed %.2f s after the left trigger's press (%.0f mm from its nearest hold "
            "then), the palm %.0f mm from the hold for %s (the left grip at %.2f%s)%s, the hand's turn %.0f degrees "
            "off the grip's%s; it glides on over %.2f s",
            o.waited, o.pressedFrom * 1000.0f, o.palmFrom * 1000.0f, kSlideGripNames[o.grip], squeeze, both, stop,
            o.turnedOff, turned, o.glideFor);
    else if (o.pressed && o.palmFrom < 0.0f)
        Log("Pistol reload: the left trigger pressed with the pistol in hand: the slide not grabbed (%s)", lost);
    if (o.missed) {
        // Why: pistolslide's, or with no pistol to work which it was (pistol.txt turned off mid-wait isn't the pistol
        // leaving the hand); too far at the press, unless it was in reach of a grip the hand wasn't turned for and
        // waited for the one it was; and how long of the wait the palm wasn't known (the hand busy or not tracked: why,
        // if it still is), so a miss with the palm unseen isn't told as one with the palm out of reach.
        const char* why = on           ? o.missWhy
                          : pistolHeld ? "pistol.txt no longer works the slide"
                                       : "the pistol left the hand";
        char tooFar[64] = "", unseen[192] = "";
        if (o.pressedFrom >= p.reach)
            std::snprintf(tooFar, sizeof(tooFar), "too far to grab it (within %.0f mm), and ", p.reach * 1000.0f);
        if (o.unseen > 0.0f)
            std::snprintf(unseen, sizeof(unseen), "; the palm not known for %.0f ms of the wait: %s",
                          o.unseen * 1000.0f, noHand ? noHand : "busy or not tracked then");
        Log("Pistol reload: the left trigger pressed %.0f mm from the slide's nearest hold (the left grip at %.2f%s): "
            "%s%s (the nearest it came: %.0f mm%s)",
            o.pressedFrom * 1000.0f, squeeze, both, tooFar, why, o.nearest * 1000.0f, unseen);
    }
    if (o.letGo)
        Log("Pistol reload: the slide let go (%s), pulled %.2f cm of its %.2f: %s",
            !on                  ? "the pistol out of hand, or pistol.txt changed"
            : in.trigger < 0.4f ? "the left trigger let go"
                                 : lost,
            o.pulled * 100.0f, p.travel * 100.0f,
            !o.racked ? "too short to rack, it springs back"
            : live    ? "racked, it springs forward"
                      : "racked, it springs home (no round moves: a rehearsal)");
    // Racked, it slams home with the M9's own slide release (the game's reload from empty, build\sounds); with the
    // rounds by hand it feeds (a chambered round thrown out), or the stop takes it again on an empty magazine.
    if (o.letGo && o.racked && on) {
        sounds::Play("pistol_slide");
        if (live)
            ammo::PistolRack(false);
    }
    g_slideOut = o;
    g_slideHeldAt = o.held ? GetTickCount64() : 0;
    if (on && o.back >= 0.0f)
        t.pistolSlideBack = o.back;  // over slidetest's and locktest's
}

// The left hand on the slide while it has it (PistolSlide), as OnTheLever on the M416's charging handle: in the grip
// it took (pistolposes.h kSlidePalms), slid back with it, glided on from where the hand is drawn (over the two-handed
// hold, if that's on too) and eased back off to it after the let go (pistolslide.cpp works out both), blended in about
// the palm centre. Its fingers go onto the slide with the bare fingers in Update.
void OnTheSlide(skin::HandTargets& t, bool pistolHeld) {
    g_slideShare = pistolHeld ? g_slideOut.share : 0.0f;
    if (g_slideShare <= 0.0f || !t.left)
        return;
    float blended[12];
    BlendAbout(t.leftPose, g_slideOut.hand, g_slideShare, kLeftPalmCentre, blended);
    std::memcpy(t.leftPose, blended, sizeof(blended));
    Transform(t.leftPose, g_leftAnchor, t.leftWrist);
}

// The magazine by hand (pistolmag.cpp; pistol.txt "mag 1"): what's drawn and what the hands and X do, and with the
// rounds by hand (PistolRoundsLive) its rounds too: out, they go back to the spare ones (ammo::PistolEject); a fresh one
// seated brings its own (ammo::PistolInsert). pouch.cpp's zone and squeeze, the pistol's own well, hands and numbers.
// The rehearsal ("rounds 0", or the pistol not counted): no rounds change, and while its own magazine is out
// (g_pistolMagOut) the right trigger does nothing (input.cpp, PistolMagazineOut), as if the pistol were empty.
std::atomic<ULONGLONG> g_pistolMagOutAt{0};  // when Update last saw it out with the pistol in hand (0: it's in)
ULONGLONG g_pistolSeatedAt = 0;  // when one was last pushed home: that push isn't a grab of the slide
// The left fingers on the magazine, as pistolmag has them this frame: how far they're on it (1 at the take, easing
// out with the hand drawn on it after the seat, a park or a fall), and the flat base's and the push home's shares.
float g_pistolMagFingers = 0.0f, g_pistolMagFlat = 0.0f, g_pistolMagPush = 0.0f;

// X (input.cpp MagazineTap) with "mag 1" and the train pistol in the right hand: its magazine release (the M9's button
// behind the trigger guard). One part way into the grip drops out of it from where it's drawn (the review: from the
// held draw, not a pose kept from before); otherwise the pistol's own drops out of the grip; with the grip empty
// nothing (fromScript, F16: one goes in, so a scripted run can always put it back). False: not the pistol's, X as
// before.
bool PistolMagazineRelease(bool fromScript) {
    float hold[12];
    if (!g_on || !g_pistolMagOn || !skin::BarePistolHand(hold))
        return false;
    if (pistolmag::Parked()) {
        if (!skin::DropPistolMagazine(skin::PistolDrop::kWell))
            Log("Pistol reload: X: the magazine part way in is gone (not drawn lately, so not seen falling)");
        pistolmag::Clear();
        sounds::Play("pistol_mag_out");
    } else if (pistolmag::Holding()) {
        Log("Pistol reload: X with a magazine in the left hand: nothing in the grip to drop");
    } else if (!g_pistolMagOut) {
        if (!skin::DropPistolMagazine(skin::PistolDrop::kGrip))
            Log("Pistol reload: X: the magazine out of the grip (not drawn lately, so not seen falling)");
        g_pistolMagOut = true;
        sounds::Play("pistol_mag_out");
        if (PistolRoundsLive())
            ammo::PistolEject();
    } else if (fromScript) {
        g_pistolMagOut = false;
        sounds::Play("pistol_mag_in");
        Log("Pistol reload: F16 with the magazine out: one put in (a test)");
        if (PistolRoundsLive())
            ammo::PistolInsert();
    } else {
        Log("Pistol reload: X with the magazine out: take one at the belly with the left grip (X held is the game's "
            "own reload)");
    }
    return true;
}

// pistol.txt "loadtest" (with "mag 1"): a scripted left hand stands in for the left controller (none tracked) while the
// pistol is in the right hand. Every 6 s: X; 40 cm down the grip's line, turned 35 degrees (where a test's hand takes one) the squeeze
// takes a magazine (0.6 s); carried up towards the grip, straightening, to 6 cm under it (to 2.0 s): it's caught on the
// way (with the catch's reach at 6 cm or more) and glides in; pushed on 4 cm (to 2.6 s), it snaps home; held there,
// then the hand lets go and drops away (3.0-4.0 s). "loadtest park": the hand lets go under the grip and drops 10 cm
// (2.0-2.6 s), leaving the magazine part way in (parked); it comes back up under it and pushes it on (to 4.2 s): home;
// then drops away. The hand holds the magazine as the carry does (pistolmag::GloveFor) all the way: the module reads
// its push on the line from where it was caught, and draws the hand going over to the flat base itself.
void LoadTest(skin::HandTargets& t, pistolmag::Frame& f, bool standIn) {
    static ULONGLONG s_start = 0, s_cycle = 0;
    static bool s_took = false, s_on = false;
    if (!g_pistolTests.load || !standIn) {
        s_on = false;
        return;
    }
    const ULONGLONG now = GetTickCount64();
    if (!s_on) {  // its cycles from now, the first starting at once
        s_on = true;
        s_start = now;
        s_cycle = ~0ull;
        Log("Pistol reload: loadtest: the scripted left hand reloads the pistol (no left controller tracked)");
    }
    const ULONGLONG since = now - s_start;
    if (since / 6000 != s_cycle) {  // a new cycle: X (a magazine left in the hand from before is gone first)
        s_cycle = since / 6000;
        s_took = false;
        if (pistolmag::Holding() && !pistolmag::Parked())
            pistolmag::Clear();
        if (pistolmag::Parked() || !g_pistolMagOut)
            PistolMagazineRelease(false);
    }
    const float u = static_cast<float>(since % 6000) / 1000.0f;
    auto ease = [](float x) {
        x = std::fmin(std::fmax(x, 0.0f), 1.0f);
        return x * x * (3.0f - 2.0f * x);
    };
    auto between = [&](float from, float to, float a, float b) { return a + (b - a) * ease((u - from) / (to - from)); };
    const bool park = g_pistolTests.load == 2;
    float s = -0.40f, turn = 0.0f, squeeze = u >= 0.6f ? 1.0f : 0.0f;
    if (u < 0.8f) {
        turn = 35.0f;
    } else if (u < 2.0f) {
        s = between(0.8f, 2.0f, -0.40f, -0.17f);
        turn = between(0.8f, 2.0f, 35.0f, 0.0f);
    } else if (!park) {
        s = u < 2.6f ? between(2.0f, 2.6f, -0.17f, -0.13f) : u < 3.0f ? -0.13f : between(3.0f, 4.0f, -0.13f, -0.28f);
        squeeze = u < 3.0f ? 1.0f : 0.0f;
    } else {
        s = u < 2.6f   ? between(2.0f, 2.6f, -0.17f, -0.27f)
            : u < 4.2f ? between(2.6f, 4.2f, -0.27f, -0.05f)
                       : between(4.2f, 5.2f, -0.05f, -0.25f);
        squeeze = 0.0f;
    }
    float inGun[12], magPose[12];
    pistolmag::PlaceOnLine(s, turn, inGun);
    Compose(t.pistolPose, inGun, magPose);
    pistolmag::GloveFor(magPose, false, f.glove);
    f.left = true;
    f.leftGrip = {};
    f.squeeze = squeeze;
    f.reached = u >= 0.6f && !s_took;
    s_took = s_took || f.reached;
    f.atPouch = false;
    f.apart = 0.0f;
    f.handFree = true;
    f.wellEmpty = g_pistolMagOut;
    std::memcpy(t.leftPose, f.glove, sizeof(t.leftPose));
    Transform(t.leftPose, g_leftAnchor, t.leftWrist);
    t.left = true;
}

// After the pouch (its zone and squeeze, the M416's too): the magazine taken over the left shoulder, carried, caught under the
// grip and pushed home, each logged; drawn (HandTargets::pistolMag, pistolMagOut) and its fingers kept for Update. Only
// with "mag 1" and the train pistol in the right hand, or a magazine still in hand through a moment without it (its "in
// hand" drops now and then), or the hand drawn on one still easing back to the controller after it's gone (the review:
// stopped there, it came back off the controller once the pistol was back in hand); otherwise nothing. tracked: the
// left hand as its controller holds it, palm its grip point (the view's space); toView: the room's turn into the view's
// (a fall's speed).
pistolmag::Result PistolMagazine(skin::HandTargets& t, const pouch::Result& pouchEvents, bool pistolHeld,
                                 bool trackedLeft, const float* tracked, const float* palm, const float* toView,
                                 float dt) {
    pistolmag::Result r;
    if (g_pistolMagOn && (pistolHeld || pistolmag::Holding() || pistolmag::EasingBack())) {
        float seat[12];
        skin::PistolMagazineSeat(seat);  // the game's own once it's drawn it home, the recordings' until then
        pistolmag::SetSeat(seat);
        pistolmag::Frame f;
        f.dt = dt;
        f.pistol = pistolHeld;
        f.pistolTracked = t.gun;
        std::memcpy(f.pistolPose, t.pistolPose, sizeof(f.pistolPose));
        XrPosef leftGrip{};
        f.left = trackedLeft && input::HandPose(0, leftGrip);
        f.leftGrip = leftGrip.position;
        std::memcpy(f.glove, tracked, sizeof(f.glove));
        f.squeeze = input::LeftSqueeze();
        if (t.gun && f.left) {
            const float apart[3] = {palm[0] - t.rightWrist[0], palm[1] - t.rightWrist[1], palm[2] - t.rightWrist[2]};
            f.apart = Length(apart);
        }
        f.reached = pouchEvents.reached;
        f.atPouch = pouchEvents.atPouch;
        f.handFree = !g_pistolSupport && !g_lining && !g_pistolSlide.Held() && g_pistolBlend == 0.0f;
        f.wellEmpty = g_pistolMagOut;
        // The rehearsal: no slide stop, and always one over the shoulder. With the rounds by hand: the pistol's own.
        const bool live = PistolRoundsLive();
        f.slideLocked = live && ammo::PistolLocked();
        f.spare = !live || ammo::PistolSpare() > 0;
        LoadTest(t, f, pistolHeld && !trackedLeft);
        r = pistolmag::Update(f);
        const pistolmag::Tuning tuning = pistolmag::Settings();
        if (r.took) {
            g_freshSqueeze = false;  // not the two-handed hold's as well
            Log("Pistol reload: a magazine taken at the belly, carried %s", g_pistolCarry->name);
        } else if (f.reached && *r.note) {
            Log("Pistol reload: the squeeze at the belly took nothing: %s", r.note);
        }
        // One line each: what the magnetic catch did (to set its reach and angle from), and a near miss once it's out
        // of reach again.
        if (r.missed)
            Log("Pistol reload: the magazine came within %.1f cm of the grip's mouth, its length %.0f degrees off the "
                "grip's line, and wasn't caught: %s (catch %.1f cm, %.0f degrees)",
                r.missFrom * 100.0f, r.missTurned, r.missWhy, tuning.reach * 100.0f, tuning.angle);
        if (r.caught)
            Log("Pistol reload: the magazine caught %.1f cm from the grip's mouth, its length %.0f degrees off the "
                "grip's line (catch %.1f cm, %.0f degrees): it glides in, %.1f cm short of home, the hand onto its "
                "flat base",
                r.caughtFrom * 100.0f, r.caughtTurned, tuning.reach * 100.0f, tuning.angle, -r.s * 100.0f);
        if (r.pushed)
            Log("Pistol reload: the palm pushes the magazine on from %.1f cm short", -r.s * 100.0f);
        if (r.offLine)
            Log("Pistol reload: the magazine pulled back out of the grip as it glided in: back in the hand");
        if (r.parked)
            Log("Pistol reload: the magazine left %.1f cm short in the grip (the palm pushes it on, X drops it)",
                -r.s * 100.0f);
        if (r.letGo) {
            float v[3];  // the hand's speed as it let go, from the room into the view's space
            for (int i = 0; i < 3; ++i)
                v[i] = toView[i * 3] * r.dropVelocity[0] + toView[i * 3 + 1] * r.dropVelocity[1] +
                       toView[i * 3 + 2] * r.dropVelocity[2];
            if (!skin::DropPistolMagazine(skin::PistolDrop::kHand, v))
                Log("Pistol reload: the magazine let go of (not drawn lately, so not seen falling)");
        }
        if (r.putBack)
            Log("Pistol reload: the magazine put back at the belly");
        if (r.cancelled)
            Log("Pistol reload: the magazine in the left hand is gone (the game's own reload put one in, or the pistol "
                "or the hand was lost a second)");
        if (r.seated) {
            g_pistolMagOut = false;
            g_pistolSeatedAt = GetTickCount64();
            sounds::Play("pistol_mag_in");
            Log("Pistol reload: the magazine %s%s", r.dwelt ? "held at the grip went home by itself" : "pushed home",
                live ? "" : " (a rehearsal: no rounds change)");
            if (live)
                ammo::PistolInsert();
        }
        if (r.tried) {
            // Each try at the grip: how near the hand's push brought it, to set the snap from (the headset tests).
            char nearest[48], hands[64] = "";
            if (r.closest >= 0.0f)
                std::snprintf(nearest, sizeof(nearest), "home");
            else
                std::snprintf(nearest, sizeof(nearest), "%.1f cm short of home", -r.closest * 100.0f);
            if (r.apartThen > 0.0f)
                std::snprintf(hands, sizeof(hands), ", the hands' grip points %.0f cm apart then",
                              r.apartThen * 100.0f);
            Log("Pistol reload: a try at the grip %s: the push held the magazine's top %s at the nearest%s (it snaps "
                "home within %.1f cm: pistol.txt seat)",
                r.seated ? "seated it" : r.parked ? "left it part way in" : r.offLine ? "came off the line" : "ended",
                nearest, hands, tuning.snap * 100.0f);
        }
        if (r.mag) {
            t.pistolMag = true;  // over magtest's
            std::memcpy(t.pistolMagPose, r.magPose, sizeof(t.pistolMagPose));
        }
    }
    if (g_pistolMagOut)
        t.pistolMagOut = true;
    // Out with the pistol in hand: the right trigger held off (PistolMagazineOut; through a moment without it too).
    if (!g_pistolMagOn || !g_pistolMagOut)
        g_pistolMagOutAt = 0;
    else if (pistolHeld)
        g_pistolMagOutAt = GetTickCount64();
    // The fingers on it as the module has them: on at the take, going over to the flat base with the hand drawn on it,
    // easing off it with the hand (it eases the hand back to the controller over the glide's time: fingers easing off
    // faster swept the index through the right hand, screenshots\wf_reload\g2\C-index-then-palm).
    g_pistolMagFingers = r.fingers, g_pistolMagFlat = r.flat, g_pistolMagPush = r.push;
    return r;
}

// Once a second while the left hand holds on: how far the hands' line strayed from where the right
// hand points the gun, how far apart the hands were, the left grip's reading and how much the left hand
// steered (steer.h's cone is set from these: 35/65 degrees until the user's own aiming is seen; and a
// hold that never lets go would show a grip that reads 0.4 or more with the hand relaxed: 4.5 minutes
// with the controller lying untracked, 2026-10-31 08:15); and any frame the gun turned or moved more than
// a hand can (25 degrees, 15 cm), with what the right controller itself did then, to tell a tracking jump
// from the mod's own (the user's "gun jumps", 2026-10-31).
void NoteSteer(const steer::Output& s, bool holding, float apart, float squeeze) {
    static ULONGLONG s_since = 0;
    static float s_angleMost = 0.0f, s_apart[2] = {10.0f, 0.0f}, s_squeeze[2] = {1.0f, 0.0f}, s_weightLeast = 1.0f;
    static bool s_held = false;
    if (holding && s.angle >= 0.0f) {
        s_angleMost = std::fmax(s_angleMost, s.angle);
        s_apart[0] = std::fmin(s_apart[0], apart), s_apart[1] = std::fmax(s_apart[1], apart);
        s_squeeze[0] = std::fmin(s_squeeze[0], squeeze), s_squeeze[1] = std::fmax(s_squeeze[1], squeeze);
        s_weightLeast = std::fmin(s_weightLeast, s.weight);
        s_held = true;
    }
    const ULONGLONG now = GetTickCount64();
    if (now - s_since < 1000)
        return;
    s_since = now;
    if (s_held)
        Log("Hands: holding the handguard: the hands' line up to %.0f degrees from where the right hand points the "
            "gun, the hands %.0f-%.0f cm apart, the left grip %.2f-%.2f; the left hand steered it %.0f%% at the least",
            s_angleMost / steer::kDegree, s_apart[0] * 100.0f, s_apart[1] * 100.0f, s_squeeze[0], s_squeeze[1],
            s_weightLeast * 100.0f);
    s_angleMost = 0.0f, s_apart[0] = 10.0f, s_apart[1] = 0.0f, s_squeeze[0] = 1.0f, s_squeeze[1] = 0.0f;
    s_weightLeast = 1.0f;
    s_held = false;
}

// Once a second: how steady the gun's line is in the room: how far it turned a frame on average, and how far off a straight line (the hand's noise and whatever else shakes it), with what the
// left hand was doing then.
void NoteGunRough(const float* gunRoom, float steered, float apart) {
    static float s_before[3] = {}, s_last[3] = {};
    static int s_have = 0, s_frames = 0;
    static float s_turned = 0.0f, s_rough = 0.0f, s_worst = 0.0f, s_steered = 0.0f, s_apart = 0.0f;
    static ULONGLONG s_since = 0;
    const float line[3] = {gunRoom[2], gunRoom[6], gunRoom[10]};  // its z, the way it points
    if (s_have >= 2) {
        const float step[3] = {line[0] - s_last[0], line[1] - s_last[1], line[2] - s_last[2]};
        const float bend[3] = {step[0] - (s_last[0] - s_before[0]), step[1] - (s_last[1] - s_before[1]), step[2] - (s_last[2] - s_before[2])};
        const float rough = Length(bend) / steer::kDegree;
        s_turned += Length(step) / steer::kDegree;
        s_rough += rough;
        s_worst = std::fmax(s_worst, rough);
        s_steered += steered;
        s_apart += apart;
        ++s_frames;
    }
    std::memcpy(s_before, s_last, sizeof(s_before));
    std::memcpy(s_last, line, sizeof(s_last));
    s_have = std::min(s_have + 1, 2);
    const ULONGLONG now = GetTickCount64();
    if (now - s_since < 1000)
        return;
    s_since = now;
    if (s_frames > 10) {
        const float n = static_cast<float>(s_frames);
        Log("Gun rough: its line turned %.2f degrees a frame, %.3f off a straight line (worst %.2f); the left hand steering it %.0f%%, the hands %.0f cm apart", s_turned / n, s_rough / n,
            s_worst, s_steered / n * 100.0f, s_apart / n * 100.0f);
    }
    s_frames = 0;
    s_turned = s_rough = s_worst = s_steered = s_apart = 0.0f;
}

// gunRoom nullptr: no gun this frame; the next one starts afresh (it isn't a jump from seconds before).
void NoteJump(const float* gunRoom, const XrPosef& aimRoom, const XrPosef& gripRoom, float steered) {
    static bool s_have = false;
    static float s_gun[12] = {};
    static XrPosef s_aim{}, s_grip{};
    static ULONGLONG s_window = 0;
    static int s_logged = 0;
    if (!gunRoom) {
        s_have = false;
        return;
    }
    if (s_have) {
        float trace = 0.0f;  // the turn between the two rotations: trace(A^T B) = 1 + 2 cos
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                trace += s_gun[i * 4 + j] * gunRoom[i * 4 + j];
        const float turned = std::acos(std::fmin(std::fmax((trace - 1.0f) * 0.5f, -1.0f), 1.0f)) / steer::kDegree;
        const float moved[3] = {gunRoom[3] - s_gun[3], gunRoom[7] - s_gun[7], gunRoom[11] - s_gun[11]};
        if (turned > 25.0f || Length(moved) > 0.15f) {
            const XrQuaternionf& a = s_aim.orientation;
            const XrQuaternionf& b = aimRoom.orientation;
            const float dot = std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
            const float own = 2.0f * std::acos(std::fmin(dot, 1.0f)) / steer::kDegree;
            const float ownMoved[3] = {gripRoom.position.x - s_grip.position.x, gripRoom.position.y - s_grip.position.y,
                                       gripRoom.position.z - s_grip.position.z};
            const ULONGLONG now = GetTickCount64();
            if (now - s_window > 1000)
                s_window = now, s_logged = 0;
            if (s_logged++ < 5)
                Log("Hands: the gun jumped %.0f degrees and %.0f cm in a frame; the right controller itself turned %.0f "
                    "degrees and moved %.0f cm; the left hand steering it %.0f%%",
                    turned, Length(moved) * 100.0f, own, Length(ownMoved) * 100.0f, steered * 100.0f);
        }
    }
    std::memcpy(s_gun, gunRoom, sizeof(s_gun));
    s_aim = aimRoom, s_grip = gripRoom;
    s_have = true;
}

// The left glove's line-up, kept in grip.txt (rewritten whole).
void SaveLeftOffset() {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&SaveLeftOffset), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"grip.txt");
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"w") != 0 || !f)
        return;
    if (g_gunGripTuned)
        fprintf(f, "gun %.4f %.4f %.4f\n", g_gunGrip[0], g_gunGrip[1], g_gunGrip[2]);
    fprintf(f, "left %.4f %.4f %.4f\n", g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2]);
    fprintf(f, "spread %.2f\n", g_spread);
    fprintf(f, "leftwidth %.2f\n", g_leftWidth);
    const float* o = g_leftOffset;
    fprintf(f, "leftpose %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n", o[0], o[1], o[2], o[3], o[4],
            o[5], o[6], o[7], o[8], o[9], o[10], o[11]);
    fclose(f);
}

// ---- Wrist movement: each forearm solved each frame (armik.h) ----
armik::State g_armState[2];  // left, right, from frame to frame
// The last frame with a hand to solve from: its body heading and wrist, so a recentre (the body's heading turning more than 30 degrees in a frame) or a tracking jump (the wrist
// 30 cm) starts afresh.
bool g_armSeen[2] = {};
float g_armYaw[2] = {}, g_armWrist[2][3] = {};
constexpr float kArmYawJump = 30.0f * 3.14159265f / 180.0f, kArmWristJump = 0.3f;
// The chest's turn from how the hands are held (PlaceForearms), radians; the collar drawn in to the shoulder from this far brought along to this far (metres).
float g_chestBlade = 0.0f;
constexpr float kChestBladePerMetre = 100.0f * 3.14159265f / 180.0f, kChestBladeMost = 45.0f * 3.14159265f / 180.0f, kChestBladeEase = 0.2f;
constexpr float kCollarDrawsIn = 0.12f, kCollarDrawnIn = 0.26f;

// The hand the arm is solved from: the glove's palm bone (1) as it's drawn, turned as the glove is
// times that bone's own turn (its quaternion: the widened palm's is none, the game's grip's rigid),
// placed so its wrist joint is where the drawn bone's is.
void ArmHand(const skin::HandTargets& t, float* hand) {
    float glove[9], bone[9], turn[9], drawn[12], wrist[3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            glove[i * 3 + j] = t.leftPose[i * 4 + j], bone[i * 3 + j] = t.leftFingers[1][i * 4 + j];
    XrQuaternionf q = MatrixToQuat(bone);
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    q = n > 0.0f ? XrQuaternionf{q.x / n, q.y / n, q.z / n, q.w / n} : XrQuaternionf{0, 0, 0, 1};
    QuatToMatrix(q, bone);
    Mul3(glove, bone, turn);
    Compose(t.leftPose, t.leftFingers[1], drawn);
    Transform(drawn, armik::kWrist, wrist);
    PlaceAt(turn, armik::kWrist, wrist, hand);
}

// The same for the right glove drawn as its own (free hands: HandTargets::rightGlove kRightOwn): its palm bone (32, the left's 1 mirrored) as drawn, the wrist joint
// the left's mirrored across x (as armik::Solve takes the right's).
void ArmHandRight(const skin::HandTargets& t, float* hand) {
    const float* palm = t.rightFingers[32 - handpose::kRightFirst];
    float glove[9], bone[9], turn[9], drawn[12], wrist[3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            glove[i * 3 + j] = t.rightGlovePose[i * 4 + j], bone[i * 3 + j] = palm[i * 4 + j];
    XrQuaternionf q = MatrixToQuat(bone);
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    q = n > 0.0f ? XrQuaternionf{q.x / n, q.y / n, q.z / n, q.w / n} : XrQuaternionf{0, 0, 0, 1};
    QuatToMatrix(q, bone);
    Mul3(glove, bone, turn);
    Compose(t.rightGlovePose, palm, drawn);
    const float wristBind[3] = {-armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]};
    Transform(drawn, wristBind, wrist);
    PlaceAt(turn, wristBind, wrist, hand);
}

// A shoulder (side: 0 left, 1 right) as the arms estimate it with the chest square (PlaceForearms), in the view's space. False without a body frame.
bool SquareShoulder(int side, const float* fromHead, const float* toView, float* shoulder) {
    float neckRoom[3], yaw = 0.0f, neck[3];
    if (!pouch::BodyFrame(neckRoom, yaw))
        return false;
    const float rightRoom[3] = {std::cos(yaw), 0.0f, -std::sin(yaw)}, forwardRoom[3] = {-std::sin(yaw), 0.0f, -std::cos(yaw)};
    Transform(fromHead, neckRoom, neck);
    for (int i = 0; i < 3; ++i) {
        const float* r = toView + i * 3;
        const float toLeft = -(r[0] * rightRoom[0] + r[1] * rightRoom[1] + r[2] * rightRoom[2]);
        const float back = -(r[0] * forwardRoom[0] + r[1] * forwardRoom[1] + r[2] * forwardRoom[2]);
        shoulder[i] = neck[i] + (side == 0 ? toLeft : -toLeft) * g_shoulder[0] - r[1] * g_shoulder[1] + back * g_shoulder[2];
    }
    return true;
}

// Each forearm's bones and fold point solved from its hand (HandTargets::arm), in the view's space; the shoulders from the pouch's neck point and body heading (pouch::BodyFrame,
// the recentred space: fromHead and toView take it into the view's). The left's hand is the glove as it's drawn, the right's where the game's hold puts it on the gun in hand
// (skin::RightHandOnGun). standIn: the left glove is the stand-in's (arms.txt's tests move and turn it first). Nothing while arms.txt says "off", without a body frame, for a hand
// that isn't drawn or (the left) is being lined up: then that forearm goes whole with its hand. A gap or a jump starts its arm afresh, solved all the same: skipping the frame
// drew the forearm whole for it, a flicker of the arm.
// rigLeft: the game's own left hand on a launcher-pose gun as it's drawn there (its frame, in the view's space), while the left isn't the player's (skin.cpp's DrawLauncherRig):
// the left arm is solved from that hand instead, as the right's is on the grip, into HandTargets' own fields for it (rigLeftArm). Only drawn: the player's own left, a
// stand-in's too, isn't drawn then, and nothing else reads it. Beyond the arm's reach its shoulder comes along, as for any hand: it never stretches.
// Each hand the arms were last solved from, as drawn (PlaceForearms: the player's own glove, the game's left hand on the gun, the right on the grip as the game holds
// it or as its own glove), for the bare forearms with a gun in hand (PlaceBareForearms): the same hands under the bare arms' numbers.
// With each, the shoulder it was solved from: the chest turned as the hands hold the gun (a square chest leaves the handguard 12 cm further from the left shoulder).
// And that chest's own axes (its left, its back), for the bare arms' collar bones there (their sleeves).
float g_armHandAs[2][12] = {}, g_armShoulderAs[2][3] = {}, g_armChestAs[2][3] = {};
bool g_armHandHave[2] = {};

void PlaceForearms(skin::HandTargets& t, const float* fromHead, const float* toView, bool standIn, const float* rigLeft = nullptr) {
    g_armHandHave[0] = g_armHandHave[1] = false;
    auto reset = [](int side) {
        g_armState[side] = armik::State{};
        g_armSeen[side] = false;
    };
    // (Whose hand the left arm is solved from, the player's or the game's on the gun: a change starts it afresh.)
    static bool s_rigLeftWas = false;
    if ((rigLeft != nullptr) != s_rigLeftWas)
        reset(0);
    s_rigLeftWas = rigLeft != nullptr;
    if (!g_armsOn) {
        reset(0), reset(1);
        return;
    }
    if (standIn && g_armTests.drop) {  // "test drop": the game's own left arm
        t.left = false;
        if (!rigLeft)
            reset(0);
    }
    float neckRoom[3], yaw = 0.0f;
    if (!pouch::BodyFrame(neckRoom, yaw)) {
        reset(0), reset(1);
        return;
    }
    // A gap in the solves (free hands off, F8, the headset not rendering or located: hands::Update not run) starts afresh: the hand may have rolled either way meanwhile (the
    // review, 2026-10-31).
    static ULONGLONG s_solvedAt = 0;
    const ULONGLONG now = GetTickCount64();
    const bool gap = now - s_solvedAt > 100;
    s_solvedAt = now;
    // The body's axes (right, forward, up in the room), in the view's space.
    const float rightRoom[3] = {std::cos(yaw), 0.0f, -std::sin(yaw)}, forwardRoom[3] = {-std::sin(yaw), 0.0f, -std::cos(yaw)};
    float neck[3], toLeft[3], up[3], back[3];
    Transform(fromHead, neckRoom, neck);
    for (int i = 0; i < 3; ++i) {
        const float* r = toView + i * 3;
        toLeft[i] = -(r[0] * rightRoom[0] + r[1] * rightRoom[1] + r[2] * rightRoom[2]);
        up[i] = r[1];
        back[i] = -(r[0] * forwardRoom[0] + r[1] * forwardRoom[1] + r[2] * forwardRoom[2]);
    }
    // Both hands first: the chest turns by how they're held.
    float hands[2][12];
    bool have[2] = {};
    // (arms.txt's tests are of the stand-in's hand: they rest while the game's hand is the one solved. The self-test reads them with the M416 in hand, where the
    // stand-in's is solved.)
    const bool handTests = standIn && !rigLeft && (g_armTests.move || g_armTests.neutral || g_armTests.bend);
    if (rigLeft) {
        // The game's own left hand on the gun: its frame as it's drawn there (the game's bones are rigid: the hand as the solve takes it). The chest turns by it as
        // by the player's own on the front (below): the body drawn holds the gun in both hands. Through the game's own reload too, as it did while the player's glove
        // rode that hand there.
        std::memcpy(hands[0], rigLeft, sizeof(hands[0]));
        have[0] = true;
    } else if (t.left && !g_lining) {
        // The stand-in's tests, on its glove: moved, then turned about its wrist joint (neutral, bent).
        if (handTests) {
            if (g_armTests.move)
                for (int i = 0; i < 3; ++i)
                    t.leftPose[i * 4 + 3] += g_armTests.moveBy[i];
            if (g_armTests.neutral || g_armTests.bend) {
                float shoulder[3], before[12], turned[12];
                for (int i = 0; i < 3; ++i)
                    shoulder[i] = neck[i] + toLeft[i] * g_shoulder[0] - up[i] * g_shoulder[1] + back[i] * g_shoulder[2];
                ArmHand(t, before);
                std::memcpy(turned, before, sizeof(turned));
                if (g_armTests.neutral)
                    armik::Neutral(turned, shoulder, toLeft, up, back, false, g_armParams[0]);
                if (g_armTests.bend)
                    armik::BendWrist(turned, false, g_armTests.bendBy[0], g_armTests.bendBy[1], g_armTests.bendBy[2]);
                float inverse[12], turn[12], glove[12];  // the hand's turn, onto the glove
                InvertRigid(before, inverse);
                Compose(turned, inverse, turn);
                Compose(turn, t.leftPose, glove);
                std::memcpy(t.leftPose, glove, sizeof(glove));
            }
            Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        }
        ArmHand(t, hands[0]);
        have[0] = true;
    } else {
        reset(0);
    }
    {
        // The right hand on the gun as the game holds it (rightGunPose, rightLauncherPose: the gun as drawn, as before); free hands, the right glove drawn as its
        // own (free, on the handguard, easing onto or off the pistol grip): from its palm as drawn; hidden or the game's, none.
        float onGun[12];
        bool launcher = false;
        if (t.rightGlove == skin::kRightOnGrip && skin::RightHandOnGun(onGun, launcher)) {
            Compose(launcher ? t.rightLauncherPose : t.rightGunPose, onGun, hands[1]);
            have[1] = true;
        } else if (t.rightGlove == skin::kRightOwn) {
            ArmHandRight(t, hands[1]);
            have[1] = true;
        } else {
            reset(1);
        }
    }
    // The chest bladed as a body holding a rifle is: the hand further out brings its shoulder forward and the other's back (a rifle's two hands 39 cm apart along it: 39 degrees, the left
    // shoulder 11 cm forward, as the game's own hold has it). Without this a square chest left the handguard 12 cm further from the left shoulder than a body has it. Eased, or a glove going
    // onto the gun swings the shoulders.
    float bladeWanted = 0.0f;
    if (have[0] && have[1] && !handTests) {
        const float wristLeft[3] = {armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]}, wristRight[3] = {-armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]};
        float wl[3], wr[3];
        Transform(hands[0], wristLeft, wl);
        Transform(hands[1], wristRight, wr);
        const float along = (wr[0] - wl[0]) * back[0] + (wr[1] - wl[1]) * back[1] + (wr[2] - wl[2]) * back[2];
        bladeWanted = std::fmin(std::fmax(along * kChestBladePerMetre, -kChestBladeMost), kChestBladeMost);
    }
    g_chestBlade += (bladeWanted - g_chestBlade) * (gap || handTests ? 1.0f : kChestBladeEase);
    const float bladeC = std::cos(g_chestBlade), bladeS = std::sin(g_chestBlade);
    float chestLeft[3], chestBack[3];
    for (int i = 0; i < 3; ++i) {
        chestLeft[i] = toLeft[i] * bladeC - back[i] * bladeS;
        chestBack[i] = toLeft[i] * bladeS + back[i] * bladeC;
    }
    std::memcpy(g_armChestAs[0], chestLeft, sizeof(chestLeft));
    std::memcpy(g_armChestAs[1], chestBack, sizeof(chestBack));
    float shoulders[2][3] = {};
    for (int side = 0; side < 2; ++side) {
        float out[3], shoulder[3];
        for (int i = 0; i < 3; ++i) {
            out[i] = side == 0 ? toLeft[i] : -toLeft[i];  // (the elbow's way stays in the body's axes, as fitted)
            shoulder[i] = neck[i] + (side == 0 ? chestLeft[i] : -chestLeft[i]) * g_shoulder[0] - up[i] * g_shoulder[1] + chestBack[i] * g_shoulder[2];
            shoulders[side][i] = shoulder[i];
        }
        if (!have[side])
            continue;
        std::memcpy(g_armHandAs[side], hands[side], sizeof(g_armHandAs[side]));
        std::memcpy(g_armShoulderAs[side], shoulder, sizeof(g_armShoulderAs[side]));
        g_armHandHave[side] = true;
        // (The game's own left hand's arm goes into its own fields, HandTargets::rigLeftArm: the M416's draws never take it for the player's.)
        const bool rig = side == 0 && rigLeft;
        float (*const forearm)[12] = rig ? t.rigLeftForearm : t.forearm[side];
        float* const fold = rig ? t.rigLeftFold : t.fold[side];
        const float* hand = hands[side];
        const float sx = side == 0 ? 1.0f : -1.0f;
        const float wristBind[3] = {armik::kWrist[0] * sx, armik::kWrist[1], armik::kWrist[2]};
        float wrist[3];
        Transform(hand, wristBind, wrist);
        const float moved[3] = {wrist[0] - g_armWrist[side][0], wrist[1] - g_armWrist[side][1], wrist[2] - g_armWrist[side][2]};
        if (gap || (g_armSeen[side] && (std::fabs(std::remainder(yaw - g_armYaw[side], 6.2831853f)) > kArmYawJump || Length(moved) > kArmWristJump)))
            g_armState[side] = armik::State{};
        g_armSeen[side] = true;
        g_armYaw[side] = yaw;
        std::memcpy(g_armWrist[side], wrist, sizeof(wrist));
        armik::Result r;
        if (const armik::State& s = g_armState[side];
            !(std::isfinite(s.twist) && std::isfinite(s.azimuth) && std::isfinite(s.pole[0]) && std::isfinite(s.pole[1]) && std::isfinite(s.pole[2])))
            g_armState[side] = armik::State{};
        armik::Solve(hand, shoulder, out, up, back, side == 1, g_armParams[side], g_armState[side], r);
        if (side == 0) {
            // The cuff on the forearm as wide as the glove's on the hand: none on the handguard (the game's grip), nor on the game's own hand.
            float cuff[12];
            handpose::CuffWiden(rig ? 1.0f : 1.0f + (g_leftWidth - 1.0f) * (1.0f - g_gripBlend), cuff);
            for (int k = 0; k < 3; ++k)
                Compose(r.frame[k], cuff, forearm[k]);
        } else {
            std::memcpy(forearm, r.frame, sizeof(r.frame));
        }
        // The rest of the arm, so the forearm isn't an open tube looked into: the upper arm, and the collar bone with the body's axes (the bind pose's x its left, z its front),
        // the shoulder's joint where the upper arm's far end is, so nothing between them stretches (a hand beyond the arm's reach brings the shoulder along), the chest between
        // the two.
        std::memcpy(forearm[3], r.upper, sizeof(r.upper));
        std::memcpy(shoulders[side], r.shoulder, sizeof(shoulders[side]));
        const float movedBy[3] = {r.shoulder[0] - shoulder[0], r.shoulder[1] - shoulder[1], r.shoulder[2] - shoulder[2]};
        const float broughtAlong = Length(movedBy);
        {
            // Brought far along, the collar draws in to the shoulder's joint: what ties it to the chest, folded between the shoulders, would be a flap trailing across the view from
            // the arm's end (film, 2026-11-04), and the sleeve's end closes.
            // (Not to nothing: a zero matrix has no normals, and the end showed black.)
            const float keep = std::fmax(1.0f - steer::Smooth((broughtAlong - kCollarDrawsIn) / (kCollarDrawnIn - kCollarDrawsIn)), 0.03f);
            const float sb[3] = {armik::kShoulder[0] * sx, armik::kShoulder[1], armik::kShoulder[2]};
            float* c = forearm[4];
            for (int i = 0; i < 3; ++i) {
                c[i * 4] = chestLeft[i] * keep, c[i * 4 + 1] = up[i] * keep, c[i * 4 + 2] = -chestBack[i] * keep;
                c[i * 4 + 3] = r.shoulder[i] - (c[i * 4] * sb[0] + c[i * 4 + 1] * sb[1] + c[i * 4 + 2] * sb[2]);
            }
        }
        std::memcpy(fold, r.fold, sizeof(r.fold));
        (rig ? t.rigLeftArm : t.arm[side]) = true;
        // (The game's own hand on a gun held far out, once a second with arms.txt's "log 1": its shoulder comes along there, and a film can look at that moment for a
        // sleeve stretched. Only when asked for: a gun aimed in one hand often has its front past the left arm's reach, and this was a line a second all through play.)
        if (static ULONGLONG s_farAt = 0; rig && g_armsLog && broughtAlong > 0.02f && now - s_farAt >= 1000) {
            s_farAt = now;
            const float apart[3] = {r.wrist[0] - shoulder[0], r.wrist[1] - shoulder[1], r.wrist[2] - shoulder[2]};
            Log("Arms: the game's left hand on the gun: %.2f m from the left shoulder (an arm reaches %.2f), its shoulder brought along %.2f m: nothing stretches",
                Length(apart), armik::kUpper + armik::kFore, broughtAlong);
        }
        static ULONGLONG s_logged[2] = {};
        if (g_armsLog && now - s_logged[side] >= 1000) {
            s_logged[side] = now;
            const float apart[3] = {r.wrist[0] - shoulder[0], r.wrist[1] - shoulder[1], r.wrist[2] - shoulder[2]};
            constexpr float kDeg = 180.0f / 3.14159265f;
            Log("Arms: %s reach %.2f/%.2f bend %.1f (flexion %+.1f ulnar %+.1f) twist %+.0f%s%s%s; chest turned %+.0f; from the eye: shoulder (%.2f %.2f %.2f), brought along %.2f m, elbow (%.2f %.2f %.2f), %.2f m off, wrist (%.2f %.2f %.2f)", side == 0 ? "left" : "right", Length(apart), armik::kUpper + armik::kFore,
                r.bend * kDeg, r.flexion * kDeg, r.deviation * kDeg, r.twist * kDeg, r.clamped ? " [clamped]" : "", r.twistHeld ? " [twist held]" : "",
                rig ? " (the game's hand on the gun)" : standIn && side == 0 ? " (the stand-in's hand)" : "", g_chestBlade * kDeg, shoulder[0], shoulder[1], shoulder[2], broughtAlong, r.elbow[0], r.elbow[1], r.elbow[2], Length(r.elbow), r.wrist[0], r.wrist[1], r.wrist[2]);
        }
    }
    for (int i = 0; i < 3; ++i)
        t.chest[i] = (shoulders[0][i] + shoulders[1][i]) / 2;
    t.haveChest = true;
}

// ---- Holsters (holster.h): what the hands and the grasp have, as holster.h reads it; the gun put away through free hands' sleep. ----

// The gun put away now (its profile's id, 0 none), and as free hands went to sleep with it put away (the game drawing a grenade, the knife, a gun the
// hands don't know; seated; free hands off) with input's count of the user's picks then: it comes back put away when they wake with that gun, no pick
// and no new life since (the holster's word outlives the grasp's Clear, which a sleep makes).
uint32_t g_stowedId = 0;
uint32_t g_sleptStowedId = 0;
unsigned g_sleptPicks = 0;
// When the last scripted moment ended (a cutscene, a QTE): the game's new weapon then, or during one, comes into the right hand as at a new life (the
// user's default of 2026-11-06), not to its place.
ULONGLONG g_scriptedEndAt = 0;
constexpr ULONGLONG kScriptedEndKeeps = 2000;
// Last frame's, by hand: a rifle magazine (the pouch's, or handmag's in the left), the grenade launcher's round or case pulled out, a rocket or the
// launcher's case (rocket.cpp's hand is the left). This frame's Result::hold: that hand holds the gun and isn't seen, so its grips are disarmed and it
// drops nothing (the user's rule: a tracking loss never drops a gun).
bool g_holsterMagBy[2] = {}, g_holsterRoundBy[2] = {}, g_holsterRocketBy[2] = {};
bool g_holsterHold[2] = {};

holster::Holder HolsterHolder(grasp::Holder h) {
    switch (h) {
    case grasp::Holder::kHeld: return holster::Holder::kHeld;
    case grasp::Holder::kBarrel: return holster::Holder::kBarrel;
    case grasp::Holder::kGlide: return holster::Holder::kGlide;
    case grasp::Holder::kLoose: return holster::Holder::kLoose;
    case grasp::Holder::kStowed: return holster::Holder::kStowed;
    }
    return holster::Holder::kHeld;
}

// That hand has a place as the runtime gives it now (seen, or its own guess; never input's 0.5 s held place); a film round's hand unless it's lost.
bool HolsterPlaced(int side) {
    if (g_test[side].on)
        return !g_test[side].step.lost;
    tracking::Record rec;
    return input::TrackingOf(side, rec) && (rec.state == tracking::State::kSeen || rec.state == tracking::State::kGuessed);
}

// The soldier's ten slots as holster.h reads them: a weapon in it, a launcher by its kind, a gun the hands know (the profile's id from its row), and
// which gun it is (its weapon object and blueprint: holster.h tells a new gun in a slot by them, a take's or a gift's).
void HolsterNameOf(int slot, char* out, size_t size);
void HolsterSlots(holster::SlotIn* out) {
    for (int k = 0; k < holster::kSlots; ++k) {
        out[k] = holster::SlotIn{};
        out[k].carried = ammo::SlotCarried(k);
        out[k].launcher = out[k].carried && ammo::LauncherInSlot(k);
        out[k].known = out[k].carried && ProfileIdOfRow(ammo::RowInSlot(k)) != 0;
        uint32_t weapon = 0, blueprint = 0;
        if (out[k].carried && ammo::SlotIdentity(k, weapon, blueprint)) {
            // (Its kind by its blueprint's name: the same through a new life, where the blueprint's address may be another; none read, the address.)
            const uint32_t byName = holster::NameKey(ammo::NameInSlot(k));
            out[k].id = weapon, out[k].kind = byName ? byName : blueprint;
            HolsterNameOf(k, out[k].name, sizeof(out[k].name));
        }
    }
}

// The slots and the takes for holster.h's Arrive (it notes a new gun in a slot before deciding where the game's new weapon goes).
void HolsterArriveSlots(holster::ArriveIn& in) {
    in.haveSlots = true;
    HolsterSlots(in.slots);
    in.taken = g_holsterTaken;
    in.swapsSeen = pickups::Hooked();
}

// A take of a pickup with these content entries (entriesAll: every entry it has), before the game is asked: holster.h's TakeCheck with the guns the game
// would take out of the soldier's slots (pickrules::TakeVictims). kAway: a holstered one goes into that pickup and stays on its shoulder (drawn back
// from there by the game's take of it); kRefuse: one couldn't be given back (the pickup destroys it, takes every gun, or has entries not read), so it
// isn't asked (the user, 2026-11-18: a holstered gun stays till a hand takes it out and drops it). Refused only for a plain floor gun (a soldier's
// drop): the level's own pickups (kind other than kDrop: Rock's Stinger, a script's gun) always go ahead (lost: a holstered gun it destroys is gone, as
// the game does it; review finding 6). Holsters off: kGo.
holster::TakeOut HolsterTakeCheck(int entries, int entriesAll, const int* entrySlot, const bool* entryGun, bool replaceAll, bool unspawnOrStatic,
                                  pickups::Kind kind) {
    if (!g_holsterSet.on || !entrySlot || !entryGun)
        return holster::TakeOut{};
    holster::SlotIn slots[holster::kSlots];
    HolsterSlots(slots);
    pickrules::TakeIn in;
    in.entries = entries < pickrules::kMostEntries ? (entries > 0 ? entries : 0) : pickrules::kMostEntries;
    in.entriesAll = entriesAll > in.entries ? entriesAll : in.entries;
    for (int e = 0; e < in.entries; ++e)
        in.entry[e].gun = entryGun[e], in.entry[e].slot = entrySlot[e];
    in.replaceAll = replaceAll, in.unspawnOrStatic = unspawnOrStatic;
    for (int k = 0; k < holster::kSlots; ++k)
        if (slots[k].carried)
            in.carried |= 1u << k;
    return holster::TakeCheck(g_holsterSet, slots, g_holster, HolsterHolder(g_grasp.holder), pickrules::TakeVictims(in), unspawnOrStatic,
                              replaceAll && !unspawnOrStatic, pickrules::TakeUnread(in), kind != pickups::kDrop);
}

// A take that goes ahead though it destroys a holstered gun (the level's own pickup: TakeOut::lost): said before the game is asked.
void HolsterGun(int slot, char* out, size_t size);
void SayTakeLost(const char* taking, const holster::TakeOut& check) {
    if (!check.lost || check.slot < 0)
        return;
    char gun[48];
    HolsterGun(check.slot, gun, sizeof(gun));
    if (check.why == holster::Why::kUnread)
        Log("Holster: the %s's take may put %s (%s, slot %d) out of its slot for good (the pickup has more guns in it than are read): it's the level's "
            "own pickup, so it goes ahead, as the game takes it", taking, gun, holster::AtName(check.at), check.slot);
    else
        Log("Holster: the %s's take puts %s (%s, slot %d) out of its slot for good (%s): it's the level's own pickup, so it goes ahead, as the game "
            "takes it: that gun is gone from its shoulder", taking, gun, holster::AtName(check.at), check.slot,
            check.why == holster::Why::kGone ? "the pickup keeps no place for the gun it replaces" : "the pickup takes every gun, and can't give one back alone");
}

// A refused take's words for the line: why the holstered gun couldn't be given back.
const char* TakeRefusedWords(holster::Why why) {
    switch (why) {
    case holster::Why::kGone: return "the pickup keeps no place for the gun it replaces: it would be gone for good";
    case holster::Why::kAll: return "the pickup takes every gun you carry, and taking it back would give them all back as they were";
    case holster::Why::kUnread: return "the pickup has more guns in it than are read: what it would take isn't known";
    case holster::Why::kPart: return "it would take the grenade launcher off it";
    default: return "it couldn't be given back";
    }
}

// A slot's gun by its own name for the lines ("M416", "SMAW", "AN-94"; one the guns' table doesn't know, its blueprint's last part), not by free hands'
// profile name ("rifle"), so a gun not drawn yet has its name too; "" for none. Copied out of pickups::Nice's ring of 8.
void HolsterNameOf(int slot, char* out, size_t size) {
    out[0] = 0;
    if (const recoilguns::Row* row = ammo::RowInSlot(slot))
        strncpy_s(out, size, row->name, _TRUNCATE);
    else if (const char* blueprint = ammo::NameInSlot(slot); blueprint && *blueprint)
        strncpy_s(out, size, pickups::Nice(blueprint), _TRUNCATE);
}

// Every slot's, for holster::Say.
struct HolsterNames {
    char name[holster::kSlots][32] = {};
    const char* list[holster::kSlots] = {};
    HolsterNames() {
        for (int k = 0; k < holster::kSlots; ++k) {
            HolsterNameOf(k, name[k], sizeof(name[k]));
            list[k] = name[k];
        }
    }
};

// "the M416", for a line of hands.cpp's own about the gun in that slot; "the gun" when it isn't known.
void HolsterGun(int slot, char* out, size_t size) {
    char name[32] = "";
    if (slot >= 0 && slot < holster::kSlots)
        HolsterNameOf(slot, name, sizeof(name));
    if (name[0])
        snprintf(out, size, "the %s", name);
    else
        snprintf(out, size, "the gun");
}

// holster.h's lines, said: the quiet ones only with holsters.txt "log 1".
void SayHolster(const holster::Event* events, int count) {
    if (count <= 0)
        return;
    const HolsterNames names;
    for (int k = 0; k < count; ++k) {
        if (events[k].quiet && !g_holsterSet.log)
            continue;
        char line[320];
        if (holster::Say(events[k], names.list, line, sizeof(line)) > 0)
            Log("%s", line);
    }
}

// One hand as holster.h reads it: the runtime's own word on it this frame (seen, its guess, no place: raw from the loss's first frame, never input's
// 0.5 s held place), or a film round's step's (lost: no place; unseen: its place kept as the runtime's guess); its grip as the grasp reads it, unread
// while the controllers aren't watched (the headset off or out of focus: nothing is counted then); what it holds as last frame left it; and, for the
// tick alone, the trigger hand with its gun at the eye (the head within a few centimetres of the gun's line, behind its middle, as last drawn).
void HolsterHand(holster::HandIn& h, const grasp::HandIn& g, int side, const float* headRoom) {
    bool watched = true;
    if (g_test[side].on) {
        const freetest::Step& step = g_test[side].step;
        h.seen = step.lost ? holster::Seen::kNone : step.unseen ? holster::Seen::kGuessed : holster::Seen::kSeen;
        if (!step.lost) {
            const XrVector3f& at = g_test[side].grip.position;
            h.at[0] = at.x, h.at[1] = at.y, h.at[2] = at.z;
        }
        float v[3], w[3];
        h.velocityKnown = h.seen == holster::Seen::kSeen && HandVelocityOf(side, v, w);  // (a throw's sensed speed, the room's)
        if (h.velocityKnown)
            std::memcpy(h.velocity, v, sizeof(h.velocity));
    } else if (tracking::Record rec; input::TrackingOf(side, rec)) {
        watched = rec.state != tracking::State::kNotWatched;
        // (A controller the runtime hasn't once called tracked since the controllers were watched again is taken as seen where the runtime puts it, as
        // input takes every controller: a runtime that never set the tracked bit would otherwise keep every gun in the hand that holds it and put none
        // away. The Frame calls them tracked within a second of each watch: the reach test's "Tracking:" lines.)
        const bool neverTracked = rec.state == tracking::State::kGuessed && !rec.haveSeen;
        if (rec.state == tracking::State::kSeen || neverTracked) {
            h.seen = holster::Seen::kSeen;
            std::memcpy(h.at, neverTracked ? rec.guessAt : rec.seenAt, sizeof(h.at));
            h.velocityKnown = !neverTracked && rec.speedKnown;
            if (h.velocityKnown)
                std::memcpy(h.velocity, rec.seenSpeed, sizeof(h.velocity));
        } else if (rec.state == tracking::State::kGuessed) {
            h.seen = holster::Seen::kGuessed;
            std::memcpy(h.at, rec.guessAt, sizeof(h.at));
        }
    } else {
        watched = false;
    }
    // (The left lost past input's hold is still the player's own controller while its grip is read: the grasp's own for it is "tracked now", and the
    // stand-in's left, with nobody in the headset, is never read. The right: the grasp's, never the stand-in's.)
    h.own = g.own || (side == 0 && g.read);
    h.read = g.read && watched;
    h.squeeze = g.squeeze;
    h.armed = g_graspNow.armed[side];
    h.busy = g.busy;
    h.magazine = g_holsterMagBy[side];
    h.round40 = g_holsterRoundBy[side];
    h.rocket = g_holsterRocketBy[side];
    h.onGun = g.onSupport;
    if (g_haveLastGunRoom && g_grasp.holder == grasp::Holder::kHeld && g_grasp.trigger == side) {
        float inverse[12], eye[3];
        const float head[3] = {headRoom[3], headRoom[7], headRoom[11]};
        InvertRigid(g_lastGunRoom, inverse);
        Transform(inverse, head, eye);  // (the gun's model space: x left, y up, z to its muzzle)
        h.aims = std::fabs(eye[0]) < 0.08f && eye[1] > -0.03f && eye[1] < 0.25f && eye[2] > -0.30f && eye[2] < 0.30f;
    }
}

// The gun in hand changed (the profile's id), before the grasp runs. The new one stays in the hand that held the one before by its pistol grip: the left,
// within 2 s, seen, not a new life; else it's in the right, unarmed as at a spawn, the glove on its grip from the first frame (today's after a
// switch). One lying, flying into a hand from the ground or held by its front alone goes back to the right: true, the grasp is given this one frame off
// so its return and FreeHands' cleanup run. In those two cases the gun before's steer and the gloves' ride on it are forgotten, and so are its holds
// on its front if it came straight before (another gun's places). Into the right hand from its pistol grip all is as at today's switch: the steer eases
// out by itself and the front grips go as they did (the same gun back after a knife or a grenade steers on). Dormant (enabled false): the line alone.
// Holsters first: with the gun before put away at its place (or put away as free hands went to sleep, and this gun the same), the new one goes into
// the hand that drew it at a place, into the right after a pick, or stays at its place (holster.h's Arrive); else today's.
bool OnGunChanged(const gunprofile::Profile& p, bool enabled) {
    g_gunChangedAt = GetTickCount64();  // (GiftOnItsWay: a gift X held's R asked for came, or another gun did)
    {
        // The M416's front is its handguard's line (Support's), a launcher-pose gun's the game's left palm on it.
        const bool rifle = p.pose == gunprofile::Pose::kRifle;
        const float front[3] = {rifle ? (kGuardFrom[0] + kGuardTo[0]) / 2 : p.support[0][0], rifle ? (kGuardFrom[1] + kGuardTo[1]) / 2 : p.support[0][1],
                                rifle ? (kGuardFrom[2] + kGuardTo[2]) / 2 : p.support[0][2]};
        char attached[48] = "";
        if (p.attachments > 0)
            sprintf_s(attached, ", %d of them its attachments'", p.attachments);
        const loose::Spec* body = p.body;
        // (Its weight as loose weighs it let go of, empty and with its rounds: loose::WeightWords. The M249's had said "8.8 kg" alone, its full box's
        // rounds in it, against the 7.5 its notes give empty: 2026-11-18.)
        char weight[64] = "no weight";
        if (body)
            loose::WeightWords(*body, weight, sizeof(weight));
        Log("Free hands: the %s in hand: by its pistol grip at (%.3f %.3f %.3f), its %s at (%.3f %.3f %.3f), its sights %.2f degrees under its z, its body %s "
            "(%s, %d hull points%s)",
            p.name, p.anchor[1][0], p.anchor[1][1], p.anchor[1][2], p.front, front[0], front[1], front[2], p.up, body ? body->name : "none", weight,
            body ? body->hullPoints : 0, attached);
    }
    g_gunRelease = GunRelease{};
    if (!enabled)
        return false;
    const auto forget = []() {
        g_haveGripRel[0] = g_haveGripRel[1] = false;
        g_regripDrawn = false;
        g_steer = steer::State{};
        g_bodyLine = steer::BodyLine{};
        if (g_profileWas != 0) {  // (straight from it: after frames with none, its holds went as they always have)
            // A hand taken off its hold so keeps its squeeze as a fresh one: still held and within reach of this gun's front, it takes that at once (Support,
            // LauncherSupport), and a switch leaves the hands in place: whether the gun stayed in the left hand or went back to the right from a hold by its
            // front alone, as a hand off the M416's handguard always could. (A launcher-pose gun's hold had spent the squeeze at its take: the hand floated off
            // the new gun till it let go and squeezed again, where one off the M416's handguard went straight on: the films' review, 2026-11-11.)
            for (int s = 0; s < 2; ++s)
                if (g_onLauncherBy[s]) {
                    Log("Hands: the %s hand let go of the launcher's front grip (another gun)", kHandName[s]);
                    g_onLauncherBy[s] = false;
                    g_freshBy[s] = true;
                }
            if (g_supportingBy[1]) {  // (the right's hold on the gun before's front is another gun's)
                Log("Hands: the right hand let go of the %s (another gun)", g_onM320By[1] ? "grenade launcher's grip" : "handguard");
                g_supportingBy[1] = g_onM320By[1] = false;
                g_freshBy[1] = true;
            }
        }
    };
    // The owner's flow (b) (pickrules::LyingTaken): a scene's gift (a prompt used by a hand, or X held's R at one, within its window) taking the slot of
    // the gun that lies on the ground (the roof's AT4 by its kit into the slot of the M4A1 let go of on the floor; never a pickup's take, which puts
    // the lying gun into that pickup): its game gun goes with the kit and nothing of it can be drawn meanwhile (no hosting), so where it lay is kept,
    // for the game's gun of its kind back in that slot to lie there again (LyingBack, below).
    {
        const ULONGLONG nowMs = GetTickCount64();
        pickrules::LyingTakenIn lt;
        lt.gift = (g_useMade.side >= 0 && g_useMade.gift && nowMs - g_useMade.at <= kUseGiftMs) || input::PromptRSince() <= kUseGiftMs;
        lt.lies = g_grasp.holder == grasp::Holder::kLoose && g_gunLoose.id != 0 && g_gunLoose.have;
        lt.retired = g_pick.lyingRetired;
        lt.lyingId = g_profileWas, lt.newId = p.id;
        lt.lyingSlot = g_gunLoose.slot, lt.newSlot = ammo::GameSlot();
        if (pickrules::LyingTaken(lt)) {
            g_lyingGone = LyingGone{};
            g_lyingGone.on = true;
            g_lyingGone.id = g_profileWas;
            g_lyingGone.slot = g_gunLoose.slot;
            std::memcpy(g_lyingGone.bone, g_gunLoose.bone, sizeof(g_lyingGone.bone));
            std::memcpy(g_lyingGone.origin, g_gunLoose.origin, sizeof(g_lyingGone.origin));
            g_lyingGone.name = g_gunLoose.name;
            g_lyingGone.gift = p.name;
            g_lyingGone.at = nowMs;
            Log("Free hands: the %s that lay on the ground went with the scene's %s (its kit took slot %d): it lies there again once the game gives it back, "
                "nothing of it drawn meanwhile (no hosting)", g_gunLoose.name, p.name, g_gunLoose.slot);
        }
    }
    // A scripted prompt that gives a gun used by a hand (g_useMade: the game's own use, or its key) and the gun the scene gives within kUseGiftMs
    // (Operation Swordbreaker's AT4: its kit puts it in slot 0 0.1 s after the use): a take by that hand, as a pickup's, below; it glides in from
    // where that hand was as it asked, else the prompt's point, else that hand's own place now. (The gun before goes as for a take.) Only the first
    // change of gun after the use, and never one the player's own pick or a draw from a place made (a door opened by hand, then the other gun drawn:
    // the draw's, into the drawing hand).
    if (g_useMade.side >= 0 && p.id && p.id != g_useMade.wasId && g_pick.landedSide < 0) {
        const ULONGLONG now = GetTickCount64();
        const int side = g_useMade.side;
        const ULONGLONG pickAt = input::LastPickAt();
        const bool picked = pickAt && pickAt >= g_useMade.at, drawing = g_holster.wait.on;
        if (g_useMade.gift && (picked || drawing || now - g_useMade.at > kUseGiftMs))
            Log("Use: the %s %.1f s after the prompt %s was used isn't the scene's gift (%s): as any change of gun", p.name,
                static_cast<double>(now - g_useMade.at) / 1000.0, g_useMade.sid, picked ? "a pick of yours" : drawing ? "a draw from its place" : "too late");
        if (g_useMade.gift && !picked && !drawing && now - g_useMade.at <= kUseGiftMs) {
            g_pick.landedSide = side, g_pick.landedId = p.id, g_pick.landedAny = false, g_pick.wasId = g_useMade.wasId, g_pick.sameKind = false;
            g_pick.freed = false, g_pick.landedSlot = ammo::GameSlot(), g_pick.landedAt = now, g_pick.landedFrames = 0, g_pick.byUse = true;
            g_pick.magnet = false;
            strncpy_s(g_pick.name, p.name, _TRUNCATE);
            // (Holsters: the gift's slot marked a take's, heard by the take's Arrive below and this frame's Update: the gun the scene gave has no holster,
            // in the hand that took it and on a shoulder only by a hand, never the kit's even in a life's first seconds; the gun before keeps its own
            // shoulder with the game, held or hanging, as a take's mark with no swap told for its slot is no pickup's: holster.h's NoteSlots.)
            if (g_pick.landedSlot >= 0 && g_pick.landedSlot < holster::kSlots)
                g_holsterTaken |= 1u << g_pick.landedSlot;
            // From where the hand that took it was as it asked (its grip then, kept in the world: anywhere in the prompt's reach, a seated player's
            // hand a metre over the roof; the roof's AT4 lies about 0.65 m off its prompt's point, at4_measured.md section 2, where exactly isn't
            // measured), else the prompt's point, else that hand's own place now.
            bool from = g_useMade.haveLay || g_useMade.haveWorld;
            if (g_useMade.haveLay) {
                std::memcpy(g_pick.centreWorld, g_useMade.lay, sizeof(g_pick.centreWorld));
            } else if (from) {
                std::memcpy(g_pick.centreWorld, g_useMade.world, sizeof(g_pick.centreWorld));
            } else if (XrPosef hand{}; g_haveLooseFrame && HandPoseOf(side, hand)) {
                const float room[3] = {hand.position.x, hand.position.y, hand.position.z};
                const float* m = g_looseFrame.roomToArms;
                for (int r = 0; r < 3; ++r)
                    g_pick.centreWorld[r] = m[4 * r] * room[0] + m[4 * r + 1] * room[1] + m[4 * r + 2] * room[2] + m[4 * r + 3] +
                                            static_cast<float>(g_looseFrame.origin[r]);
                from = true;
            }
            Log("Use: the %s the scene gave %.1f s after the prompt %s was used comes into the %s hand (from %s)", p.name,
                static_cast<double>(now - g_useMade.at) / 1000.0, g_useMade.sid, side == 0 ? "left" : "right",
                g_useMade.haveLay ? "where that hand was as it took it" : g_useMade.haveWorld ? "the prompt's point" : from ? "that hand's own place" : "where it can");
        }
        g_useMade.side = -1;
    }
    // The owner's flow (b) (pickrules::LyingBack): the game's gun of the kind that lay on the ground when a scene's gift took its slot, back in that slot
    // by the scene with no take and no pick of the player's (the level's kit after the roof's AT4, 1.3-3.3 s after the shot): it lies again where it
    // lay, still, as it lay (StepLoose: loose::Place), and a hand takes it off the floor as before, on no shoulder till a hand puts it on one. The gun
    // the scene gave meanwhile, in a hand or thrown, went with the game's own switch (nothing of it is drawn once the game draws another): one gun, never
    // two. Too far or not known where it lay: as any change of gun (today's: into the right hand).
    if (g_lyingGone.on) {
        pickrules::LyingBackIn lb;
        lb.on = true, lb.id = g_lyingGone.id, lb.slot = g_lyingGone.slot, lb.at = g_lyingGone.at;
        lb.newId = p.id, lb.newSlot = ammo::GameSlot();
        lb.take = g_pick.landedSide >= 0 && PickArrives(p.id);
        lb.now = GetTickCount64();
        int pickSlot = -1;
        const ULONGLONG pickAt = input::LastPickAt(&pickSlot);
        // (A pick of its own slot just now, the D-pad's or Y's: their way, into the right hand; an older one, or another slot's (a key the "RPG" state
        // refused), brought nothing. X held's R for a pickup, slot -1, is a take's: lb.take.)
        lb.picked = pickAt && pickAt > g_lyingGone.at && pickSlot == g_lyingGone.slot && lb.now - pickAt <= pickrules::kLyingPickMs;
        lb.newLife = g_newLife.load(std::memory_order_relaxed);
        lb.lifeAt = g_lyingGone.lifeAt;
        lb.otherLevel = g_lyingGone.lifeAt && g_holster.memoForgot;  // (that life's kit another level's: holsters' KitCheck)
        if (g_haveLooseFrame) {
            float d2 = 0.0f;
            for (int i = 0; i < 3; ++i) {
                const float d = g_lyingGone.bone[i * 4 + 3] + static_cast<float>(g_lyingGone.origin[i] - g_looseFrame.origin[i]) - g_looseFrame.eye[i];
                d2 += d * d;
            }
            lb.distance = std::isfinite(d2) ? std::sqrt(d2) : -1.0f;
        }
        const pickrules::LyingBackOut back = pickrules::LyingBack(lb);
        if (back.verdict == pickrules::LyingBackVerdict::kDrop) {
            Log("Free hands: the %s that lay on the ground before the scene took it isn't laid there again (%s)", g_lyingGone.name, back.why);
            g_lyingGone = LyingGone{};
        } else if (back.verdict == pickrules::LyingBackVerdict::kLie) {
            const bool held = g_grasp.holder == grasp::Holder::kHeld || g_grasp.holder == grasp::Holder::kBarrel || g_grasp.holder == grasp::Holder::kGlide;
            if (g_gunLoose.id) {
                Log("Free hands: the %s on the ground went with the game's own switch", g_gunLoose.name);
                loose::Remove(g_gunLoose.id);
            } else if (held) {
                Log("Free hands: the %s in the hands went with the game's own switch", g_lyingGone.gift);
            }
            g_gunLoose = GunLoose{};
            g_pick.lyingRetired = false;
            forget();
            grasp::Clear(g_grasp);
            g_grasp.holder = grasp::Holder::kLoose;
            g_grasp.trigger = -1;
            g_grasp.headShare = 1.0f;
            // (Holsters read the gun that lies afresh at this frame's Update: this one's identity and kind, never the spent AT4's thrown before it, so a
            // draw of another gun leaves the player's own lying there: holster.h's LyingGoes, the reviews of e403947.)
            g_holster.lyingSlot = -1;
            g_gunRelease = GunRelease{};
            g_gunRelease.want = g_gunRelease.still = true;
            g_gunRelease.pose = p.pose;
            g_gunRelease.id = p.id;
            g_gunRelease.body = p.body;
            g_gunRelease.name = p.name;
            g_gunRelease.front = p.front;
            g_gunRelease.sound = p.sound;
            g_gunRelease.slot = g_lyingGone.slot;
            std::memcpy(g_gunRelease.stillBone, g_lyingGone.bone, sizeof(g_gunRelease.stillBone));
            std::memcpy(g_gunRelease.stillOrigin, g_lyingGone.origin, sizeof(g_gunRelease.stillOrigin));
            int loaded = 0, spare = 0;
            g_gunRelease.rounds = ammo::Get(loaded, spare) ? loaded : 0;
            g_releaseAim = g_aimNow;
            Log("Free hands: the %s back in slot %d (%s, %.1f s after it went, %.1f m from the eye): it lies where it lay, never into a hand by itself",
                p.name, g_lyingGone.slot, back.why, static_cast<double>(lb.now - g_lyingGone.at) / 1000.0, lb.distance);
            g_lyingGone = LyingGone{};
            return false;
        }
    }
    // A gun taken off the ground by hand (pickups.h) is the gun in hand now: it goes into the hand that took it by the grasp's arrival (FreeHands, before
    // the grasp runs), never back to the right. What was the gun before's is forgotten as for one that lay; the grasp gets no frame off (the arrival is
    // itself its clear).
    if (g_pick.landedSide >= 0 && !g_pick.sameKind && PickArrives(p.id)) {
        holster::ArriveIn take;  // (holsters: a take's arrival, today's: no draw waits for another gun any more; its gun on no shoulder)
        take.take = true;
        take.slot = ammo::GameSlot();
        HolsterArriveSlots(take);
        const holster::ArriveOut out = holster::Arrive(g_holsterSet, take, g_holster);
        if (g_pick.giveBack && out.arrival == holster::Arrival::kAtPlace) {
            // A draw from a shoulder whose hand opened while its gun came (review finding 9): at its place, put away, never into that open hand (its
            // arrival would arm it and drop it three frames on). The take is over; what lay on the ground went into the pickup.
            if (g_gunLoose.id)
                loose::Remove(g_gunLoose.id);
            g_gunLoose = GunLoose{};
            g_gunRelease = GunRelease{};
            const PickState was = g_pick;
            g_pick = PickState{};
            g_pick.swapSeen = was.swapSeen;
            g_pick.kickOutId = was.kickOutId, g_pick.kickOutAt = was.kickOutAt;
            forget();
            grasp::Clear(g_grasp);
            g_grasp.holder = grasp::Holder::kStowed;
            g_grasp.trigger = -1;
            g_grasp.headShare = 1.0f;
            SayHolster(&out.event, out.event.what != holster::What::kNone ? 1 : 0);
            return false;
        }
        if (g_pick.giveBack)
            Log("Holster: the %s back from its shoulder into the %s hand (the game's own take of the pickup it was in)", p.name, kHandName[g_pick.landedSide]);
        forget();
        return false;
    }
    // Holsters: the gun before put away (read before any Clear here). Before OnChange, whose leftSeen would send a draw by a hand not seen to the right.
    {
        unsigned picks = 0;
        input::LastPickAt(nullptr, &picks);
        const bool slept = g_sleptStowedId != 0 && g_sleptStowedId == p.id && picks == g_sleptPicks && g_grasp.holder != grasp::Holder::kStowed;
        const holster::Wait wait = g_holster.wait;  // (the draw waiting for this switch, if one does: Arrive ends it)
        holster::ArriveIn in;
        in.stowed = g_grasp.holder == grasp::Holder::kStowed || slept;
        in.lying = g_grasp.holder == grasp::Holder::kLoose && !slept;
        in.newLife = g_newLife.load(std::memory_order_relaxed) || g_scripted.load(std::memory_order_relaxed) ||
                     (g_scriptedEndAt && GetTickCount64() - g_scriptedEndAt < kScriptedEndKeeps);  // (a cutscene's end too)
        in.slot = ammo::GameSlot();
        for (int s = 0; s < 2; ++s)
            in.placed[s] = HolsterPlaced(s);
        HolsterArriveSlots(in);
        const holster::ArriveOut out = holster::Arrive(g_holsterSet, in, g_holster);
        char gun[48];
        HolsterGun(in.slot >= 0 ? in.slot : g_holster.gameSlot, gun, sizeof(gun));
        if (out.arrival == holster::Arrival::kHand && (out.side == 0 || out.side == 1)) {
            // Into that hand by its pistol grip, unarmed till its grip is past 0.6 (a hand still squeezing is armed at once), its trigger firing once let go
            // as a take's. Drawn by that hand (case B), it eases onto the grip as after a take; a pick's, into the right, on it at once as at today's switch.
            const int side = out.side;
            const bool drew = wait.on && !wait.gameWay && wait.side == side;
            forget();
            grasp::Clear(g_grasp);
            g_grasp.trigger = side;
            g_grasp.fireReset[side] = false;
            if (side == 0) {
                g_leftGripProfile = p;
            } else if (!drew) {
                g_rightBlend = 1.0f;
                g_rightHold = 1;
                g_holdOutOfReachBy[1] = false;
            }
            if (out.event.what == holster::What::kDiscard) {
                // (The gun before lay on the ground, a picked-up one let go of: it went into its slot with the game's switch, its copy off the ground.)
                if (g_gunLoose.id)
                    loose::Remove(g_gunLoose.id);
                g_gunLoose = GunLoose{};
                g_gunRelease = GunRelease{};
                g_pick.lyingRetired = false;
                SayHolster(&out.event, 1);
            } else if (out.event.what == holster::What::kToHand) {
                SayHolster(&out.event, 1);  // (the game's own switch to a gun on no shoulder: into the right hand, never to a place)
            } else {
                Log("Holster: %s in the %s hand (%s)", gun, kHandName[side],
                    drew ? "the game's switch to the gun drawn from its place" : "the game's switch after a pick: into the right hand");
            }
            return false;
        }
        if (out.arrival == holster::Arrival::kAtPlace) {
            // Kept at its place: the grasp put away with no hand (no click, no sound: no hand's put-away), the game's aim at the head. The gun before lay on
            // the ground (a holstered gun the game gave back meanwhile): its copy goes with the game's switch. Held (the spent AT4 not thrown yet as the
            // level's kit gives the M4A1 back): it went with the game's switch, the hands empty.
            if (!in.lying && !in.stowed)
                Log("Free hands: the %s in the hands went with the game's own switch (%s back at its place): the hands are empty", g_profileWasName, gun);
            if (in.lying) {
                const char* lay = g_gunLoose.id ? g_gunLoose.name : "gun";
                Log("Free hands: the %s on the ground went with the game's own switch (%s back at its place)", lay, gun);
                if (g_gunLoose.id)
                    loose::Remove(g_gunLoose.id);
                g_gunLoose = GunLoose{};
                g_gunRelease = GunRelease{};
                g_pick.lyingRetired = false;
            }
            forget();
            grasp::Clear(g_grasp);
            g_grasp.holder = grasp::Holder::kStowed;
            g_grasp.trigger = -1;
            g_grasp.headShare = 1.0f;
            if (slept && out.event.what == holster::What::kCameAway && out.event.why == holster::Why::kNone)
                Log("Holster: %s still put away at its place (it was before free hands slept)", gun);
            else
                SayHolster(&out.event, out.event.what != holster::What::kNone ? 1 : 0);
            return false;
        }
    }
    const bool byPistolGrip = !(g_grasp.holder == grasp::Holder::kLoose || g_grasp.holder == grasp::Holder::kGlide || g_grasp.holder == grasp::Holder::kBarrel);
    XrPosef seen{};
    const gunprofile::Change change = gunprofile::OnChange(byPistolGrip, g_lastHeldSide, GetTickCount64() - g_lastHeldAt, HandPoseOf(0, seen),
                                                           g_newLife.load(std::memory_order_relaxed));  // (read, not taken: the grasp's frame takes it)
    if (change != gunprofile::Change::kRight)
        forget();
    if (change != gunprofile::Change::kLeft) {
        // (Into the right hand: its glove on the grip at once, as dormant, even if it was free: the gun before lay, flew, or was held by the left.)
        g_rightBlend = 1.0f;
        g_rightHold = 1;
        g_holdOutOfReachBy[1] = false;
    }
    if (change == gunprofile::Change::kReturn)
        return true;
    grasp::Clear(g_grasp);
    if (change == gunprofile::Change::kLeft) {
        g_grasp.trigger = 0;
        g_grasp.fireReset[0] = false;  // (its trigger fires once let go, as a take's)
        g_leftGripProfile = p;         // (the left holds this gun's grip now: its place goes by it)
        Log("Free hands: the switch kept the %s in the left hand", p.name);
    }
    return false;
}

// ---- Free hands on the gun in hand (gunprofile.h: the M416 on gunPose, any other on launcherPose), each frame (Update), after the hands are placed and before the
// modules: who has it (grasp.h), what follows from that (its events), and where it is for the modules after (t.gun and the profile's pose). ----
// Where the thing a prompt gives lies, once measured, by the prompt's id (pickups::UsePoint::sid): its two ends in the world and its radius. A hand's reach
// is then the capsule round it (usepoint.h, kCapsuleGrow wider) instead of the disc round the prompt's point. The AT4 on Operation Swordbreaker's roof:
// its use point was measured on the roof (at4_measured.md section 1), where it lies was not (no census at the prompt: its stills and an estimate only,
// section 2), so its disc below stands; its row goes here once a census or drawscan at the prompt gives its two ends. The empty row only ends the table.
struct UseCapsule {
    const char* sid;
    float a[3], b[3];
    float radius;
};
constexpr UseCapsule kUseCapsules[] = {
    // {"ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG", {x0, y0, z0}, {x1, y1, z1}, 0.055f},  // the AT4 lying on the hotel roof (R4: its tube's two ends, the world)
    {nullptr, {}, {}, 0.0f},
};
// A prompt's own reach across, by its id, where usepoint.h's kDisc wouldn't reach what it gives. The AT4 on the hotel roof lies about 0.65 m east of
// its prompt's point (build\designs\at4_measured.md, section 2: its stills, the 17:00 estimate (-455.5, -2013.5) against the point (-456.14, -2013.60);
// the take spot, prone, 1.72 m from the point): 1.0 m round the point, at floor height, till its capsule is measured. The empty row only ends the table.
struct UseDisc {
    const char* sid;
    float disc;
};
constexpr UseDisc kUseDiscs[] = {
    {"ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG", 1.0f},
    {nullptr, 0.0f},
};
// A squeeze this near (m) to a gun in a hand's hull box is the gun's, never a prompt's: its handguard or front grip, its magazine, its handle, a regrip
// (the support's own reach, kGuardReach and kLauncherReach).
constexpr float kUseNearGun = 0.25f;

// A point of the room (the grasp's: a hand's grip) in the world (absolute: the arms' space plus its origin), as loose's frame has them now. False
// while that frame isn't known or a number isn't finite.
bool RoomPointToWorld(const float room[3], float world[3]) {
    if (!g_haveLooseFrame || !gunprofile::FiniteAll(room, 3))
        return false;
    const float* m = g_looseFrame.roomToArms;
    for (int r = 0; r < 3; ++r)
        world[r] = m[4 * r] * room[0] + m[4 * r + 1] * room[1] + m[4 * r + 2] * room[2] + m[4 * r + 3] + static_cast<float>(g_looseFrame.origin[r]);
    return gunprofile::FiniteAll(world, 3);
}

// A use made of a scripted prompt by that hand (the game's own, or its key). A prompt on the game's pickup key (the reload's: the AT4's) gives a gun: the
// gun the scene gives next comes into that hand (OnGunChanged), from where that hand found it (handWorld: its grip as it asked, the world; null: not
// known); any other (a door) gives none.
void MadeUse(int side, const pickups::UsePoint& up, int action, const float* handWorld) {
    g_useMade = UseMade{};
    g_useMade.side = side;
    g_useMade.at = GetTickCount64();
    g_useMade.wasId = g_profile.id;
    g_useMade.gift = action == pickups::kUseReload;
    g_useMade.haveWorld = up.haveWorld;
    if (up.haveWorld)
        g_useMade.world[0] = up.world[12], g_useMade.world[1] = up.world[13], g_useMade.world[2] = up.world[14];
    if (handWorld && gunprofile::FiniteAll(handWorld, 3)) {
        std::memcpy(g_useMade.lay, handWorld, sizeof(g_useMade.lay));
        g_useMade.haveLay = true;
    }
    strncpy_s(g_useMade.sid, up.sid, _TRUNCATE);
}

// A gun in a hand, for a prompt that gives one (usepoint::Frame::gunHeld, hands::GunBarsGift): held by its pistol grip or by its front alone, flying
// into a hand, a take of one off the ground asked or landing, a draw from a shoulder waiting for its gun; its words for the line ("the right hand
// holds the M4A1"). The owner, 2026-11-19, asked whether the AT4 is taken with the rifle in the other hand: "Rifle dropped on the floor first", then
// "Better to physically drop the rifle and pick up the AT": never one the scene's kit takes from a hand.
bool GunInAHand(char* words, size_t size) {
    using grasp::Holder;
    const bool say = words && size;
    if (say)
        words[0] = 0;
    const Holder holder = g_grasp.holder;
    const char* name = g_profile.pose != gunprofile::Pose::kNone ? g_profile.name : "gun";
    const char* front = g_profile.front ? g_profile.front : "front";
    if (holder == Holder::kHeld && (g_grasp.trigger == 0 || g_grasp.trigger == 1)) {
        if (say)
            snprintf(words, size, "the %s hand holds the %s", kHandName[g_grasp.trigger], name);
        return true;
    }
    if (holder == Holder::kGlide && (g_grasp.trigger == 0 || g_grasp.trigger == 1)) {
        if (say)
            snprintf(words, size, "the %s is coming into the %s hand", name, kHandName[g_grasp.trigger]);
        return true;
    }
    if (holder == Holder::kBarrel && (g_grasp.barrel == 0 || g_grasp.barrel == 1)) {
        if (say)
            snprintf(words, size, "the %s hand holds the %s by its %s", kHandName[g_grasp.barrel], name, front);
        return true;
    }
    if (g_grasp.pendingSide == 0 || g_grasp.pendingSide == 1 || g_pick.request) {
        if (say)
            snprintf(words, size, "a take of a gun off the ground is under way");
        return true;
    }
    if ((g_pick.landedSide == 0 || g_pick.landedSide == 1) && !g_pick.byUse) {
        if (say)
            snprintf(words, size, "a gun taken off the ground is coming into the %s hand", kHandName[g_pick.landedSide]);
        return true;
    }
    if (g_holster.wait.on && (g_holster.wait.side == 0 || g_holster.wait.side == 1)) {
        if (say)
            snprintf(words, size, "a gun drawn from its place is coming into the %s hand", kHandName[g_holster.wait.side]);
        return true;
    }
    return false;
}

// A scene's gift on its way: a use of a prompt that gives a gun asked of the game or made (its gun not in the hand yet, kUseGiftMs at most), or X held's
// R at one with no change of gun since. Meanwhile no gun comes into a hand (holsters draw nothing: Frame::pendingTake; the grasp takes nothing: its
// hands busy), so the scene's kit never takes one from a hand: the use is made with both hands free of guns (usepoint.h), and they stay so till the
// gift is in the hand (the review of e403947: a rifle in a hand between the use and the AT4's draw, 0.7 s, was with the game and lay on the floor).
bool GiftOnItsWay(ULONGLONG now) {
    if (g_useAsked.serial && g_useAsked.action == pickups::kUseReload)
        return true;
    if (g_useMade.side >= 0 && g_useMade.gift && now - g_useMade.at <= kUseGiftMs)
        return true;
    const unsigned long long since = input::PromptRSince();
    return since <= kUseGiftMs && !(g_gunChangedAt && now >= g_gunChangedAt && now - g_gunChangedAt <= since);
}

// That hand's squeeze is something else's of the hands' (usepoint::Hand::yield), as this frame's holsters and last frame's poses have it: the hand at
// one of the body's places (the shoulders, the neck, the ribs, the chests, the belly: a draw, a grenade, a magazine), a draw or a put-away of the
// holsters'; within kUseNearGun of a gun in a hand (its front, its magazine, its handle, a regrip); within the grasp's own reach of a gun lying (the
// player's, a dropped one of the game's): the thing under the hand goes first. (A gun in the other hand bars only a prompt that gives one: UseFrame.)
bool UseYields(int side, const grasp::Frame& f, const holster::Result& hr) {
    using grasp::Holder;
    if (hr.place[side] != holster::Place::kNone || hr.busy[side] || hr.drawSide == side || hr.stowSide == side)
        return true;
    const float at[3] = {f.hand[side].pose[3], f.hand[side].pose[7], f.hand[side].pose[11]};
    if (!Finite(at, 3))
        return false;
    float m[3];
    const Holder holder = g_grasp.holder;
    if ((holder == Holder::kHeld || holder == Holder::kBarrel || holder == Holder::kGlide) && f.haveGun && Finite(f.gunRoom, 12)) {
        grasp::ToModel(f.gunRoom, at, m);
        if (grasp::BoxDistance(m, f.boxMin, f.boxMax) < kUseNearGun)
            return true;
    }
    const float reach = std::isfinite(f.pickReach) && f.pickReach >= 0.0f ? f.pickReach : 0.25f;
    if (f.lying && holder == Holder::kLoose && Finite(f.lyingRoom, 12)) {
        grasp::ToModel(f.lyingRoom, at, m);
        if (grasp::BoxDistance(m, f.boxMin, f.boxMax) < reach)
            return true;
    }
    for (int i = 0; i < f.candidateCount && i < grasp::kMostCandidates; ++i) {
        const grasp::Candidate& c = f.candidates[i];
        if (!Finite(c.room, 12))
            continue;
        grasp::ToModel(c.room, at, m);
        if (grasp::BoxDistance(m, c.boxMin, c.boxMax) < reach)
            return true;
    }
    return false;
}

// A scripted prompt by hand (usepoint.h): the point the game targets now (pickups::UseTarget), if it's one a hand uses (usepoint::ByHand), put into the
// room, each hand's grip point, squeeze, whether it holds anything (the grasp's frame, before the grasp runs) and whether its squeeze is something
// else's (UseYields: hr is this frame's holsters). Its pulses sent, its lines said, a use asked of the game (the prompt's key only where the game's use
// isn't as read); out.spent: the hands whose squeezes are the use's (FreeHands tells the grasp they're busy).
void UseFrame(const grasp::Frame& f, const holster::Result& hr, bool enabled, usepoint::Out& out) {
    using grasp::Holder;
    static const char* const kHand[2] = {"left", "right"};
    const ULONGLONG now = GetTickCount64();
    usepoint::Frame uf;
    uf.dt = f.dt;
    uf.asleep = !enabled || f.frozen || f.newLife;
    pickups::UsePoint up;
    float armsToRoom[12];
    const bool targeted = pickups::UseTarget(up);
    const bool byHand = targeted && usepoint::ByHand(up.action, up.kind, up.hold, up.radius);
    const bool way = pickups::UseCodeReady() || pickups::UseByKey();  // (neither: the game's use faulted this session: no hand's use at all)
    if (byHand && way && pickups::UseByHandOn() && up.haveWorld && up.enabled && (!up.maxUses || up.uses < up.maxUses) && g_haveLooseFrame &&
        Invert(g_looseFrame.roomToArms, armsToRoom)) {
        const float world[3] = {up.world[12], up.world[13], up.world[14]};
        pickrules::WorldPointToRoom(armsToRoom, g_looseFrame.origin, world, uf.point.at);
        uf.point.on = gunprofile::FiniteAll(uf.point.at, 3);
        for (const UseDisc& d : kUseDiscs)
            if (d.sid && std::strcmp(d.sid, up.sid) == 0)
                uf.point.disc = d.disc;
        for (const UseCapsule& c : kUseCapsules)
            if (c.sid && std::strcmp(c.sid, up.sid) == 0) {
                pickrules::WorldPointToRoom(armsToRoom, g_looseFrame.origin, c.a, uf.point.a);
                pickrules::WorldPointToRoom(armsToRoom, g_looseFrame.origin, c.b, uf.point.b);
                uf.point.radius = c.radius;
                uf.point.capsule = gunprofile::FiniteAll(uf.point.a, 3) && gunprofile::FiniteAll(uf.point.b, 3);
            }
    }
    // A prompt on the game's pickup key gives a gun (the AT4's): used only with no gun in either hand (the owner, 2026-11-19); a door with one too.
    uf.point.gives = uf.point.on && up.action == pickups::kUseReload;
    uf.gunHeld = GunInAHand(nullptr, 0);
    const Holder holder = g_grasp.holder;
    for (int s = 0; s < 2; ++s) {
        const grasp::HandIn& in = f.hand[s];
        usepoint::Hand& u = uf.hand[s];
        u.tracked = in.tracked && in.own && in.read;
        u.at[0] = in.pose[3], u.at[1] = in.pose[7], u.at[2] = in.pose[11];
        u.squeeze = in.squeeze;
        const bool holdsGun = ((holder == Holder::kHeld || holder == Holder::kGlide) && g_grasp.trigger == s) || (holder == Holder::kBarrel && g_grasp.barrel == s);
        u.empty = !holdsGun && !in.busy && !in.onSupport;
        u.yield = uf.point.on && UseYields(s, f, hr);
    }
    usepoint::Update(uf, g_usePoint, out);
    // (Said once a target, the same one again not within 10 s, so a target that comes and goes, or goes between the roof's three prompts, says little:
    // the prompt the game shows now, and whether a hand can use it.)
    static uint32_t s_saidEntity[4] = {};
    static ULONGLONG s_saidAt[4] = {};
    static uint32_t s_saidLast = 0;
    bool say = false;
    if (targeted && up.entity != s_saidLast) {
        int slot = -1, oldest = 0;
        for (int i = 0; i < 4; ++i) {
            if (s_saidEntity[i] == up.entity)
                slot = i;
            if (s_saidAt[i] < s_saidAt[oldest])
                oldest = i;
        }
        say = slot < 0 || now - s_saidAt[slot] > 10000;
        if (slot < 0)
            slot = oldest;
        if (say)
            s_saidEntity[slot] = up.entity, s_saidAt[slot] = now;
    }
    s_saidLast = targeted ? up.entity : 0;
    if (say) {
        const char* how = !byHand                       ? "not a hand's (only a press on the interact or the reload key, the game's reach 3 m at most)"
                          : !pickups::UseByHandOn()     ? "not a hand's: pickups.txt \"use 0\" or \"level 1\""
                          : pickups::UseCodeReady()     ? "reach for it and hold the grip: the game's own use, called on the game's thread"
                          : pickups::UseByKey()         ? "reach for it and hold the grip: its key (the game's use isn't as read)"
                                                        : "not a hand's: the game's use faulted earlier";
        const bool gives = byHand && pickups::UseByHandOn() && way && up.action == pickups::kUseReload;
        Log("Use: the game targets a scripted prompt %s (its key's action %d%s, hold %.2f s, %.1f m, %s, %u of %u uses; at (%.2f %.2f %.2f)): %s%s",
            up.sid[0] ? up.sid : "(no name)", up.action, up.action == pickups::kUseReload ? ": R, X held" : up.action == pickups::kUseInteract ? ": E, Y" : "",
            up.hold, up.radius, up.enabled ? "on" : "off", up.uses, up.maxUses, up.world[12], up.world[13], up.world[14], how,
            gives ? " (it gives a gun: with no gun in either hand, the rifle dropped or put on a shoulder first)" : "");
    }
    static ULONGLONG s_enteredAt[2] = {};
    for (int s = 0; s < 2; ++s) {
        if (out.pulse[s].seconds > 0.0f)
            input::Buzz(s, out.pulse[s].amplitude, out.pulse[s].seconds);
        if (out.entered[s] && now - s_enteredAt[s] > 2000) {
            s_enteredAt[s] = now;
            Log("Use: the %s hand at the prompt %s: hold the grip", kHand[s], up.sid);
        }
        if (out.given[s])
            Log("Use: the %s hand's squeeze at the prompt %s took something else first (a magazine, a grenade, a gun's front): not used", kHand[s], up.sid);
        if (out.refused[s]) {  // (its double click: usepoint's pulses, sent above, the second 60 ms on)
            char held[128];
            GunInAHand(held, sizeof(held));
            Log("Use: the %s hand holds the grip at the prompt %s: not used: %s: drop it or put it on a shoulder first (a prompt that gives a gun is "
                "used with no gun in either hand)", kHand[s], up.sid, held[0] ? held : "a gun is in a hand");
        }
    }
    if (out.use >= 0) {
        const char* why = "";
        if (g_useAsked.serial) {  // (one use at a time: the other hand's waits)
            Log("Use: the %s hand holds the grip at the prompt %s: not asked (the %s hand's use, #%u, waits)", kHand[out.use], up.sid,
                kHand[g_useAsked.side == 0 ? 0 : 1], g_useAsked.serial);
        } else if (const uint32_t serial = pickups::RequestUse(up.entity, out.use, &why)) {
            g_useAsked = UseAsked{serial, up.entity, out.use, up.action, now};
            strncpy_s(g_useAsked.sid, up.sid, _TRUNCATE);
            g_useAsked.haveHand = RoomPointToWorld(uf.hand[out.use].at, g_useAsked.hand);  // (where the hand is as it asks)
            Log("Use: the %s hand holds the grip at the prompt %s: the game's own use asked (#%u)", kHand[out.use], up.sid, serial);
        } else if (pickups::UseByKey() && input::TapPromptKey(up.action)) {
            float handWorld[3];
            MadeUse(out.use, up, up.action, RoomPointToWorld(uf.hand[out.use].at, handWorld) ? handWorld : nullptr);
            Log("Use: the %s hand holds the grip at the prompt %s: its key tapped (the game's own use not asked: %s)", kHand[out.use], up.sid, why);
        } else {
            Log("Use: the %s hand holds the grip at the prompt %s: not used (%s%s)", kHand[out.use], up.sid, why,
                pickups::UseByKey() ? up.action == pickups::kUseReload ? "; its key, R, not tapped now: a gun in a hand, R already down, or bf3 not in front"
                                                                       : "; its key isn't one the mod presses: E is Y's own"
                                    : "");
        }
    }
}

// What came of a use asked (each frame): made, the scene's gift (a prompt that gives a gun) to come into that hand; not made, said, nothing pressed
// instead (the game's own checks refused it, it was used up or gone, or the game's use faulted: its key would meet the same, or worse).
void NoteUse() {
    if (!g_useAsked.serial)
        return;
    const char* why = "";
    const int outcome = pickups::UseOutcome(g_useAsked.serial, &why);
    if (outcome == 0)
        return;
    static const char* const kHand[2] = {"left", "right"};
    const int side = g_useAsked.side == 0 ? 0 : 1;
    pickups::UsePoint up;
    const bool targeted = pickups::UseTarget(up) && up.entity == g_useAsked.entity;
    if (outcome > 0) {
        if (!targeted)  // (the point gone already: its place as last asked is not kept; the gift flies in from where the hand found it)
            up = pickups::UsePoint{}, strncpy_s(up.sid, g_useAsked.sid, _TRUNCATE);
        MadeUse(side, up, g_useAsked.action, g_useAsked.haveHand ? g_useAsked.hand : nullptr);
        if (g_useMade.gift)
            Log("Use: the game used the prompt %s (#%u, the %s hand's): the scene goes on; the gun it gives within %llu s comes into that hand",
                g_useAsked.sid, g_useAsked.serial, kHand[side], kUseGiftMs / 1000);
        else
            Log("Use: the game used the prompt %s (#%u, the %s hand's): the scene goes on", g_useAsked.sid, g_useAsked.serial, kHand[side]);
    } else if (!pickups::UseCodeReady() && !pickups::UseByKey()) {
        Log("Use: the game's own use of the prompt %s not made (#%u: %s): no hand's use of a prompt again this session; X held (R) still uses one on "
            "the reload key the game's way", g_useAsked.sid, g_useAsked.serial, why);
    } else {
        Log("Use: the game's own use of the prompt %s not made (#%u: %s)", g_useAsked.sid, g_useAsked.serial, why);
    }
    g_useAsked = UseAsked{};
}

void FreeHands(skin::HandTargets& t, const float* fromHead, const float* toView, float dt) {
    using grasp::Holder;
    static const char* const kSide[2] = {"left", "right"};
    for (int s = 0; s < 2; ++s)
        g_spentBy[s] = g_onGripBy[s] = false;
    // The gun in hand as Update's profile has it, and its pose and flags in HandTargets.
    const gunprofile::Profile& p = g_profile;
    const bool lp = p.pose == gunprofile::Pose::kLauncher;  // its pose is launcherPose
    float* pose = lp ? t.launcherPose : t.gunPose;
    bool& drawn = lp ? t.launcherDrawn : t.gunDrawn;
    bool& lyingNow = lp ? t.launcherLying : t.gunLying;
    float* world = lp ? t.launcherWorld : t.gunWorld;
    float* worldOrigin = lp ? t.launcherWorldOrigin : t.gunWorldOrigin;
    float seenFrom[12], seenOrigin[3];
    // The swaps first: a hand's take landing this frame quiets the seated tell this frame too (the game's switch to it moves the M416's parts).
    NoteSwaps();
    const bool seatedNow = pickrules::SeatedSleeps(skin::RifleSeated(), g_pickTookAt, GetTickCount64());
    const bool enabled = freetune::On() && p.pose != gunprofile::Pose::kNone && !skin::TrainPistolInHand() && !seatedNow && skin::LooseOn() &&
                         skin::LatestSeenFrom(seenFrom, seenOrigin);
    // Said once each way, with what stops it (a film once found the hands asleep for minutes with nothing in the log to say why).
    {
        static int s_said = -1;
        if (s_said != (enabled ? 1 : 0)) {
            s_said = enabled ? 1 : 0;
            if (enabled) {
                Log("Free hands: awake (the %s in hand)", p.name);
            } else {
                float from[12], origin[3];
                char seated[96] = "";
                if (seatedNow) {
                    if (lp)
                        sprintf_s(seated, " the %s stays in the right hand (seated);", p.name);
                    else
                        strcpy_s(seated, " the rifle's seated pose;");
                }
                Log("Free hands: asleep:%s%s%s%s%s%s", freetune::On() ? "" : " off in freehands.txt;", p.pose == gunprofile::Pose::kNone ? g_profileWhy : "",
                    skin::TrainPistolInHand() ? " the train pistol in hand;" : "", seated, skin::LooseOn() ? "" : " loose things off;",
                    skin::LatestSeenFrom(from, origin) ? "" : " the rifle's drawn place not known;");
            }
        }
    }
    g_freeOn = enabled;
    // The right does the rifle's mechanics only while its own pose is drawn (freehands.txt "rightpose 0": the game's grip carried at its controller).
    g_rightWorks = enabled && g_rightSeen && freetune::RightPose();
    // The first frame after a gap in Update (kResumeGap): every real hand's grip taken as unread this once (nothing is let go of, its arming goes).
    {
        const ULONGLONG at = g_freeHandsAt.load(std::memory_order_relaxed), nowAt = GetTickCount64();
        g_freeResumed = at && nowAt - at > kResumeGap;
        if (g_freeResumed && enabled)
            Log("Free hands: %.1f s without a frame here (the headset off?): the grips taken as unread this frame, so nothing is let go of",
                static_cast<double>(nowAt - at) / 1000.0);
    }
    // (Holsters: when a scripted moment last ended, for a change of gun just after it.)
    {
        static bool s_scriptedWas = false;
        const bool scripted = g_scripted.load(std::memory_order_relaxed);
        if (s_scriptedWas && !scripted)
            g_scriptedEndAt = GetTickCount64();
        s_scriptedWas = scripted;
    }
    // A change of gun: settled before anything below uses the gun before's state. (Counted on any frame, so "off" and back is no change of gun.)
    const bool changedGun = p.pose != gunprofile::Pose::kNone && p.id != g_profileWas;
    const bool returnNow = changedGun && OnGunChanged(p, enabled);
    // Holsters: awake again with the gun that was put away as free hands went to sleep, no pick and no new life since: back at its place (a change of
    // gun's frame had it in OnGunChanged). Asleep with it put away, it's noted below.
    if (enabled && g_sleptStowedId) {
        unsigned picks = 0;
        input::LastPickAt(nullptr, &picks);
        if (!changedGun && g_sleptStowedId == p.id && picks == g_sleptPicks && g_holsterSet.on && !g_newLife.load(std::memory_order_relaxed) &&
            g_grasp.holder != Holder::kStowed) {
            grasp::Clear(g_grasp);
            g_grasp.holder = Holder::kStowed;
            g_grasp.trigger = -1;
            g_grasp.headShare = 1.0f;
            char gun[48];
            HolsterGun(g_holster.gameSlot, gun, sizeof(gun));
            Log("Holster: %s still put away at its place (it was before free hands slept)", gun);
        }
        g_sleptStowedId = 0;
    }
    // The left's own place for the gun, the mirror of the right's: its grip pose with the right controller's learned aim-to-grip mirrored, the game's right palm on
    // the gun mirrored at its grip point (exact for mirror-image controllers). freehands.txt "lefthold": moved in the left grip's axes, or at the lined-up palm.
    // (Also while the left glove eases off the pistol grip with free hands just gone dormant or the gun changed: it opens where it was, going with the left hand,
    // on the gun it was on.)
    const bool easingOff = g_glovesOnPistol && g_gripBlend > 0.0f && (!enabled || g_leftGripProfile.id != p.id);
    const gunprofile::Profile& onLeft = easingOff ? g_leftGripProfile : p;
    g_haveHandGun[0] = false;
    g_haveOtherLeft = false;
    bool leftShifted = false;  // "lefthold": the left grip's origin isn't at the left's place on the rifle
    XrPosef leftGrip{}, leftAim{};
    if ((enabled || (g_glovesOnPistol && g_gripBlend > 0.0f)) && HandPoseOf(0, leftGrip) && input::AimFromGrip(0, leftGrip, leftAim)) {
        const float flip[9] = {-1, 0, 0, 0, 1, 0, 0, 0, -1};
        float aimTurn[9], turn[9], gunTurn[9];
        QuatToMatrix(leftAim.orientation, aimTurn);
        Mul3(toView, aimTurn, turn);
        Mul3(turn, flip, gunTurn);
        float raise[9];
        SightsTurn(onLeft.up, onLeft.right, raise);
        Mul3(gunTurn, raise, g_rifleTurnBy[0]);
        g_anchorBy[0][0] = onLeft.anchor[0][0], g_anchorBy[0][1] = onLeft.anchor[0][1], g_anchorBy[0][2] = onLeft.anchor[0][2];
        float shift[3] = {};
        if (freetune::LeftHold(shift) && t.left) {
            std::memcpy(g_atBy[0], t.leftWrist, sizeof(g_atBy[0]));  // ("lefthold lineup": the glove stays where it's lined up, the rifle at its palm)
            leftShifted = true;
        } else {
            leftShifted = shift[0] != 0.0f || shift[1] != 0.0f || shift[2] != 0.0f;
            float m[9];
            QuatToMatrix(leftGrip.orientation, m);
            const float room[3] = {leftGrip.position.x + m[0] * shift[0] + m[1] * shift[1] + m[2] * shift[2],
                                   leftGrip.position.y + m[3] * shift[0] + m[4] * shift[1] + m[5] * shift[2],
                                   leftGrip.position.z + m[6] * shift[0] + m[7] * shift[1] + m[8] * shift[2]};
            Transform(fromHead, room, g_atBy[0]);
        }
        PlaceAt(g_rifleTurnBy[0], g_anchorBy[0], g_atBy[0], g_handGun[0]);
        g_haveHandGun[0] = Finite(g_handGun[0], 12);
        // The other pose at the same grip point by its own grip and up (the M416's under any other gun; the launcher's under the M416), unsteered: a switch's
        // new gun is drawn in its first frame from these places, before its profile is known, and kept in the left it's unsteered there.
        if (p.pose != gunprofile::Pose::kNone) {
            const int other = p.pose == gunprofile::Pose::kRifle ? 1 : 0;
            float otherRaise[9];
            SightsTurn(g_poseUp[other], g_poseRight[other], otherRaise);
            float otherTurn[9], otherAnchor[3];
            Mul3(gunTurn, otherRaise, otherTurn);
            gunprofile::MirrorPoint(g_poseGrip[other], otherAnchor);
            PlaceAt(otherTurn, otherAnchor, g_atBy[0], g_otherLeft);
            g_haveOtherLeft = Finite(g_otherLeft, 12);
        }
    }
    // The grasp's frame: each hand as seen and read now (busy: last frame's), the gun as drawn last frame and as it lies, the reaches (freehands.txt). A change
    // of gun from one lying, flying or held by its front alone: one frame off (its return). Through a switch's frames with no gun drawn: frozen.
    grasp::Frame f;
    f.dt = dt;
    f.enabled = enabled && !returnNow;
    f.frozen = g_scripted.load(std::memory_order_relaxed) || g_bridging;
    f.newLife = g_newLife.exchange(false);
    f.gameReload = input::GameReloadSince() < 2500;
    // The magnet (freehands.txt "magnet", "snap"): its numbers, and the body as the pouch placed it (its gate is measured from the neck point).
    float magnetNeck[3] = {}, magnetYaw = 0.0f, magnetGate = 0.5f;
    const bool magnetBody = pouch::BodyFrame(magnetNeck, magnetYaw) && Finite(magnetNeck, 3) && std::isfinite(magnetYaw);
    freetune::Magnet(f.magnet, f.magnetSphere, f.magnetDown, f.magnetUp, f.magnetWiden, magnetGate);
    freetune::Snap(f.snapPerMetre, f.snapMost);
    f.magnetLowFirst = freetune::LowSqueeze();
    for (int s = 0; s < 2; ++s) {
        grasp::HandIn& h = f.hand[s];
        XrPosef p{}, real{};
        h.tracked = HandPoseOf(s, p);
        if (h.tracked)
            PoseMatrix(p, h.pose);
        // Reaching down: seen with its own reading these kSeenForColumn frames (a hand held through a moment out of sight, or eased back from a jump,
        // starts its count again), lowered past the gate under the neck and outside the belly (grasp::ReachesDown).
        const bool ownReading = h.tracked && (g_test[s].on || input::HandSettled(s));
        g_magnetSeenBy[s] = ownReading ? std::min(g_magnetSeenBy[s] + 1, 1000) : 0;
        h.settled = g_magnetSeenBy[s] >= grasp::kSeenForColumn;  // (the magnet's reach past "reach" asks it too)
        if (f.magnet && magnetBody && g_magnetSeenBy[s] >= grasp::kSeenForColumn) {
            const float at[3] = {h.pose[3], h.pose[7], h.pose[11]};
            h.reachDown = grasp::ReachesDown(at, magnetNeck, magnetYaw, magnetGate);
        }
        // A real controller, or a test's hand: the stand-in's right (and weapon.txt "forcehand") never arms, drops or takes.
        h.own = g_test[s].on || (s == 1 ? !input::StandingIn() : input::HandPose(0, real));
        h.read = ReadOf(s) && !(g_freeResumed && !g_test[s].on);  // (back after a gap: unread this once, as the frames it missed were)
        h.squeeze = SqueezeOf(s);
        h.trigger = TriggerOf(s);
        // (Busy: last frame's magazine or round in that hand, the charging handle in it, the left glove held still to line it up.)
        h.busy = s == 0 ? (g_busyWasBy[0] || reload::HandleSide() == 0 || g_lining) : (g_busyWasBy[1] || reload::HandleSide() == 1);
        h.onSupport = (lp ? g_onLauncherBy[s] : g_supportingBy[s]) && !g_holdOutOfReachBy[s];  // (a launcher-pose gun's front grip)
        h.forceTake = g_test[s].on && g_test[s].step.forceTake;
    }
    // Held by its handguard alone: the free hand's squeeze is a module's when that hand, seen, is where the pouch takes the seated magazine or at the charging
    // handle, on the rifle as drawn last frame (where the regrip is measured): it doesn't regrip with it (grasp.h; the grip origin sits 15 cm from the pistol
    // grip's point holding the seated magazine, the regrip's reach). Only a squeeze a module can really take this frame: the rifle tracked (the hand holding it
    // seen), and for the seated magazine the pouch's own squeeze (armed and past 0.6 now: the frame it takes) with the hand free as the pouch has it. Any
    // other squeeze there regrips, as before. Those are the M416's; any other gun's are its own (handmag.h: its seated magazine, lever, lid or belt, by
    // either hand, the one at work: the right's free glove as its controller holds it; the owner, 2026-11-19, the M249 held by its front with the left
    // and the right on its lever), and the nearer wins: a hand nearer its own pistol grip than the part claiming it regrips (grasp.h's claimFrom; the
    // review of fc010e2: a loose regrip by the right had opened the M249's feed cover or taken a magazine).
    if (enabled && g_grasp.holder == Holder::kBarrel && (g_grasp.barrel == 0 || g_grasp.barrel == 1) && g_haveLastGunRoom && f.hand[g_grasp.barrel].tracked) {
        const int g = 1 - g_grasp.barrel;
        if (g == 0 ? t.left && !g_lining : g_rightWorks) {
            float gunView[12];
            Compose(fromHead, g_lastGunRoom, gunView);
            if (p.pose == gunprofile::Pose::kRifle) {
                const bool seated = ammo::RifleFiringInHand() && skin::MagazineSeatKnown() && !skin::MagazineOut() && pouch::HoldingSide() < 0;
                const bool pouchTakes = g_pouchFedSide == g && g_pouchArmed && f.hand[g].tracked && std::fmax(SqueezeOf(g), TriggerOf(g)) > 0.6f &&
                                        (g == 0 ? !g_supporting && g_gripBlend == 0.0f && g_pistolBlend == 0.0f : !g_supportingBy[1] && g_rightBlend == 0.0f);
                f.hand[g].claimed = (seated && pouchTakes && pouch::OnSeated(gunView, g == 0 ? t.leftPose : g_rightFree, g)) ||
                                    reload::NearHandle(gunView, g == 0 ? t.leftWrist : g_rightPalm, true, g);
            } else if (lp && !g_onLauncherBy[g]) {
                if (const handmag::Gun* gun = skin::HeldMagGun()) {
                    float apart = 0.0f;
                    f.hand[g].claimed = handmag::Claims(*gun, gunView, g == 0 ? t.leftPose : g_rightFree, g == 0 ? t.leftWrist : g_rightPalm, g, &apart);
                    f.hand[g].claimFrom = apart;
                }
            }
        }
    }
    f.haveGun = g_haveLastGunRoom;
    std::memcpy(f.gunRoom, g_lastGunRoom, sizeof(f.gunRoom));
    f.lying = g_gunLoose.have && !g_pick.lyingRetired;  // (in the game's pickup now: nobody takes its copy)
    std::memcpy(f.lyingRoom, g_gunLoose.room, sizeof(f.lyingRoom));
    // At rest for the magnet's column: at rest now, or within the last second (loose puts a resting gun back to free for a moment when its ground is
    // asked again and not answered, and settles it again 0.3-1 s later), or let go of 2.5 s ago or more (still sliding down a slope, slowly).
    {
        const ULONGLONG now = GetTickCount64();
        if (f.lying && g_gunLoose.resting)
            g_lyingRestAt = now, g_lyingRestOf = g_gunLoose.releasedAt;
        f.lyingResting = f.lying && (g_gunLoose.resting || (g_lyingRestOf == g_gunLoose.releasedAt && now - g_lyingRestAt < 1000) ||
                                     (g_gunLoose.releasedAt && now - g_gunLoose.releasedAt >= 2500));
    }
    f.lost = g_gunLost;
    g_gunLost = false;
    for (int i = 0; i < 3; ++i) {
        f.gripPoint[1][i] = p.anchor[1][i];
        f.gripPoint[0][i] = p.anchor[0][i];
    }
    if (leftShifted && g_haveHandGun[0]) {
        // freehands.txt "lefthold": where the left grip's origin sits on the rifle as the left holds it (its place for it shifted), so a regrip by the left
        // is measured from where that hand would really hold it.
        const float origin[3] = {leftGrip.position.x, leftGrip.position.y, leftGrip.position.z};
        float inView[3], inverse[12];
        Transform(fromHead, origin, inView);
        InvertRigid(g_handGun[0], inverse);
        Transform(inverse, inView, f.gripPoint[0]);
    }
    std::memcpy(f.boxMin, p.boxMin, sizeof(f.boxMin));
    std::memcpy(f.boxMax, p.boxMax, sizeof(f.boxMax));
    f.glide = freetune::Glide();
    freetune::Reach(f.pickReach, f.regripReach);
    // The game's own dropped guns (pickups.h): what a hand may take this frame, the answer to a take waiting, and a gun the game gave: into the hand
    // that asked once it's the gun in hand (another gun: this frame's profile is its own; the same gun: the frame after the swap was seen), that hand
    // seen, nothing scripted. Asked again every frame till the grasp takes it, or the net ends it (NoteSwaps).
    if (f.newLife)
        ResetPick();
    f.candidateCount = PickupCandidates(f.candidates);
    f.refused = g_pick.refusedNow, g_pick.refusedNow = false;
    f.swappedAway = g_pick.swappedAwayNow, g_pick.swappedAwayNow = false;
    if (g_pick.landedSide >= 0) {
        const int a = g_pick.landedSide;
        ++g_pick.landedFrames;
        const bool inHand = g_pick.sameKind ? g_pick.landedFrames >= 2 : PickArrives(p.id) && !g_bridging;
        // (A scene's gift comes 0.7 s after its use was asked at the end of a 0.25 s squeeze: into an open hand unarmed, kept there as at a spawn,
        // never dropped three frames on; a hand still squeezing is armed as a take's: grasp.h's Frame::arriveUnarmed.)
        if (inHand && f.enabled && !f.frozen && !f.newLife && f.hand[a].tracked && g_haveHandGun[a] && PickStart(a, p, fromHead, f.arriveFrom))
            f.arriveSide = a, f.arriveMagnet = g_pick.magnet, f.arriveUnarmed = g_pick.byUse;
    }
    // Holsters (holster.h): its frame from what the hands and the grasp have now, and its word before the grasp runs: a let-go at a place puts the gun
    // away, a put-away or a draw of its own (a hand not seen opening at its place, a draw from a place), a hand not seen holding the gun keeps it (told
    // the grasp as not tracked: nothing drops), a squeeze that is a waiting draw's takes nothing else. After the grasp, Settle: the keys, the clicks,
    // the sound, the lines. Every frame, whatever holsters.txt says: "holsters 0" answers nothing, but brings a gun left put away back into the right
    // hand. (Its state outlives the grasp's Clear; a new life, a checkpoint or another level starts it afresh.)
    if (f.newLife) {
        // (Every gun's home goes, and every gun away: the world they were in is gone. The memo stays: each kind keeps the holster it last had, so the
        // new life's kit has its holsters from the start, and a gun picked up and carried into the checkpoint stays on no shoulder: NoteSlots.)
        holster::NewLife(g_holster);
        g_holsterTaken = 0;         // (no take of the life before marks the new kit)
        g_sleptStowedId = 0;
        g_raiseAfterR = -1;
        if (g_lyingGone.on) {
            // (Flow (b) through a death on its level: the "RPG" checkpoint after a miss gives the AT4 again and the scene the M4A1 back after the hit,
            // as in the first try, so it lies where it lay then too (flow (a)'s holstered M4A1 is back at its shoulder alike). Its 120 s afresh; this
            // life's own kit is no give-back, and another level's kit ends it (pickrules::LyingBack; holsters' KitCheck, below Update).)
            g_lyingGone.at = g_lyingGone.lifeAt = GetTickCount64();
            Log("Free hands: the %s that lay on the ground before the scene took it is laid there again once the scene gives it back, after this new "
                "life too (its level's checkpoint: within %.0f m of the eye then)", g_lyingGone.name, static_cast<double>(pickrules::kLyingNear));
        }
    }
    if (!f.enabled && g_grasp.holder == Holder::kStowed && g_holsterSet.on && g_stowedId) {
        g_sleptStowedId = g_stowedId;
        input::LastPickAt(nullptr, &g_sleptPicks);
        char gun[48];
        HolsterGun(g_holster.gameSlot, gun, sizeof(gun));
        Log("Holster: free hands asleep with %s put away: back at its place when they wake with it (no pick, no new life meanwhile)", gun);
    }
    holster::Frame hf;
    hf.dt = f.dt;
    hf.on = f.enabled;
    hf.frozen = f.frozen;
    hf.gameReload = f.gameReload;
    hf.m320 = input::M320ShotUnderway() || input::M320GameReloading();
    hf.spentTube = hands::At4Spent();  // (a fired AT4: thrown away wherever it's let go of, a shoulder too, never put away: holster.h)
    hf.body = magnetBody;
    std::memcpy(hf.neck, magnetNeck, sizeof(hf.neck));
    hf.yaw = magnetYaw;
    {
        float headRoom[12];
        InvertRigid(fromHead, headRoom);
        for (int s = 0; s < 2; ++s)
            HolsterHand(hf.hand[s], f.hand[s], s, headRoom);
    }
    // The holsters' own heading: a look towards a shoulder doesn't move the places (holster.h FollowHeading).
    static holster::Heading s_heading;
    if (hf.body)
        hf.yaw = holster::FollowHeading(s_heading, magnetYaw, pouch::HeadYaw(), hf, f.dt, g_holsterSet);
    hf.holder = HolsterHolder(g_grasp.holder);
    hf.trigger = g_grasp.trigger;
    hf.barrel = g_grasp.barrel;
    // (A scene's gift on its way, its use made with no gun in either hand: as a take waiting, no draw till it's in the hand, so the kit never takes a
    // gun from a hand: GiftOnItsWay.)
    const bool giftWay = GiftOnItsWay(GetTickCount64());
    hf.pendingTake = g_grasp.pendingSide >= 0 || giftWay;
    hf.anyTake = f.lying || f.candidateCount > 0;
    hf.gameSlot = ammo::GameSlot();
    HolsterSlots(hf.slots);
    {
        // A change of gun under way (holster.h's Frame::drawnOther): the game's own current slot holds another gun by its name than the one in the hands
        // (both known), 2 s at most (a gun the hands know by one id and its row by another would otherwise block holsters for good): the roof's kit, its
        // AT4 in slot 0 0.6 s before its first draw, its M4A1 back 0.9-1.3 s before its. Said once a change.
        static ULONGLONG s_sameAt = 0;
        static bool s_said = false;
        static int s_sameSlot = -1;
        const ULONGLONG now = GetTickCount64();
        const int g = hf.gameSlot;
        const uint32_t slotGun = g >= 0 && g < holster::kSlots && hf.slots[g].carried ? ProfileIdOfRow(ammo::RowInSlot(g)) : 0;
        const bool other = p.id != 0 && slotGun != 0 && slotGun != p.id;
        if (!other)
            s_sameAt = now, s_said = false, s_sameSlot = g;
        hf.drawnOther = other && now - s_sameAt <= 2000;
        if (hf.drawnOther && !s_said) {
            s_said = true;
            // (Said for another gun put into the very slot drawn, a kit's swap or a take's; a switch to another slot (the D-pad, Y, a draw's key) is the
            // same moment for holsters, unsaid: the review of e403947, a line every switch in the players' reports.)
            if (g == s_sameSlot) {
                char gun[48];
                HolsterNameOf(g, gun, sizeof(gun));
                Log("Holster: the game put the %s into slot %d with the %s still drawn (a change of gun under way): nothing decided on it till it's "
                    "drawn (2 s at most)", gun[0] ? gun : "gun", g, p.name);
            }
        }
    }
    hf.taken = g_holsterTaken;  // (the takes since the last frame: their guns go on a shoulder only by a hand)
    hf.swapsSeen = pickups::Hooked();
    // Each gun away in a pickup as the census has that pickup: still holding it, within the game's reach, how far.
    for (int a = 0; a < holster::kMostAway; ++a) {
        const holster::Away& w = g_holster.away[a];
        hf.awayFound[a] = w.on && w.entity ? pickups::HoldsGun(w.entity, w.blueprint, &hf.awayDistance[a], &hf.awayReach[a]) : -2;
    }
    {
        // The user's pick this frame (input.cpp's record: the D-pad, Touch's menu button, Y held, X held's R for a pickup every frame it's held), never
        // holsters' own keys; one made while free hands slept isn't taken seconds later.
        static unsigned s_picksWas = 0;
        int slot = -1;
        unsigned picks = 0;
        const ULONGLONG at = input::LastPickAt(&slot, &picks);
        hf.picked = picks != s_picksWas && at && GetTickCount64() - at < 250;
        hf.pickSlot = slot;
        s_picksWas = picks;
    }
    holster::Result hr;
    holster::Update(g_holsterSet, hf, g_holster, hr);
    g_holsterTaken = 0;  // (heard: Arrive this frame, if the gun changed, and Update)
    if (g_lyingGone.on && g_lyingGone.lifeAt && g_holster.memoForgot) {  // (flow (b)'s record kept through a new life whose kit is another level's)
        Log("Free hands: the %s that lay on the ground before the scene took it isn't laid there again (another level's kit: the world it lay in is gone)",
            g_lyingGone.name);
        g_lyingGone = LyingGone{};
    }
    // A draw from a shoulder of a holstered gun the game put into a pickup: the game's own take of that pickup into this hand (its swap gives the gun
    // back into its slot with its rounds, the slot's gun into the pickup; the take's arrival brings it into the hand). Not asked: said, felt.
    if (hr.giveBack.side >= 0) {
        const int side = hr.giveBack.side;
        const char* why = "";
        float distance = -1.0f;
        const uint32_t request = g_pick.request || g_pick.landedSide >= 0 ? 0 : pickups::RequestGiveBack(hr.giveBack.entity, hr.giveBack.blueprint, side, &why, &distance);
        if (!request && (g_pick.request || g_pick.landedSide >= 0))
            why = "a take is under way";
        char gun[holster::kNameLen + 4] = "the gun";
        if (hr.giveBack.away >= 0 && hr.giveBack.away < holster::kMostAway && g_holster.away[hr.giveBack.away].name[0])
            snprintf(gun, sizeof(gun), "the %s", g_holster.away[hr.giveBack.away].name);
        if (request) {
            g_pick.request = request;
            g_pick.side = side;
            g_pick.magnet = false;
            g_pick.giveBack = true;
            strncpy_s(g_pick.name, gun + 4, _TRUNCATE);
            Log("Pickups: the %s hand draws %s from its shoulder: the game is asked to take the pickup it's in (#%u, %.1f m away)", kSide[side], gun,
                request, distance);
        } else {
            holster::GiveBackFailed(g_holster, side, why, distance);
        }
    }
    // A scripted prompt by hand (usepoint.h): a squeeze begun in its reach that nothing else of the hands' wants (this frame's holsters, a gun near the
    // hand) is the use's till let go (the grasp is told that hand is busy: no lying gun's take, no magnet, no regrip comes of it); the use asked of the
    // game, and what came of the last one.
    if (f.newLife)
        usepoint::Clear(g_usePoint);
    NoteUse();
    usepoint::Out uo;
    UseFrame(f, hr, enabled, uo);
    bool trackedWas[2], busyWas[2], supportWas[2];
    for (int s = 0; s < 2; ++s) {
        grasp::HandIn& h = f.hand[s];
        trackedWas[s] = h.tracked, busyWas[s] = h.busy, supportWas[s] = h.onSupport;
        h.stows = hr.stows[s];
        // (For the grasp alone: holster.h's own frame keeps the modules' busy. A scene's gift on its way: no take by either hand till it's in the hand.)
        h.busy = h.busy || hr.busy[s] || uo.spent[s] || giftWay;
        g_holsterHold[s] = hr.hold[s];
        if (hr.hold[s]) {
            // (Not seen: never let go of. By its front, held by it alone or the support of a regrip's glide: that hold kept, the hand as the grasp counts
            // it, so a pistol grip let go of mid-glide still leaves the gun on it. By its pistol grip: told the grasp it isn't tracked, its grip disarmed,
            // then or once seen again.)
            if ((g_grasp.holder == Holder::kBarrel || g_grasp.holder == Holder::kGlide) && g_grasp.barrel == s)
                h.onSupport = true;
            else
                h.tracked = false;
        }
    }
    f.stowSide = hr.stowSide;
    f.drawSide = hr.drawSide;
    // A draw into an open hand (a pick of the put-away gun's own slot, or holsters turned off with the gun put away): never into a right hand that holds
    // something; a right hand that isn't its own controller seen now (the stand-in, the controller asleep) has it as at a spawn instead, as today.
    if (f.drawSide == 1 && !hr.drawArmed && g_grasp.holder == Holder::kStowed) {
        const holster::HandIn& right = hf.hand[1];
        char gun[48];
        HolsterGun(g_holster.gameSlot, gun, sizeof(gun));
        if (g_holsterSet.on && (right.busy || right.magazine || right.round40 || right.rocket)) {
            f.drawSide = -1;
            g_holster.drawAsk = holster::Ask{};
            Log("Holster: %s not drawn into the right hand for the pick: that hand holds something (a magazine, a round or a case): it stays at its place", gun);
        } else if (!(f.hand[1].own && f.hand[1].tracked)) {
            f.drawSide = -1;
            g_holster.drawAsk = holster::Ask{};
            grasp::Clear(g_grasp);
            Log("Holster: %s back in the right hand as at a spawn (%s; the right hand isn't its own controller seen now)", gun,
                g_holsterSet.on ? "the pick" : "holsters off");
        }
    }
    const int triggerWas = g_graspNow.trigger;
    const grasp::Result r = grasp::Update(f, g_grasp);
    g_graspNow = r;
    for (int s = 0; s < 2; ++s)
        f.hand[s].tracked = trackedWas[s], f.hand[s].busy = busyWas[s], f.hand[s].onSupport = supportWas[s];
    {
        // Holsters: what the grasp did (a let-go by its pistol grip or a front held alone; put away; drawn; a squeeze spent on a take), and what follows.
        holster::After ha;
        ha.stowed = r.stowed;
        ha.drawn = r.drawn;
        ha.letGo = r.letGo >= 0 ? r.letGo : r.released >= 0 ? r.released : r.stowed;
        ha.spent[0] = r.spent[0], ha.spent[1] = r.spent[1];
        holster::Settle(g_holsterSet, hf, ha, g_holster, hr);
        if (hr.key >= 0)
            input::SelectSlot(hr.key);  // (false: no key went, bf3 not in front: its line is input's; the draw's wait gives up by itself)
        for (int s = 0; s < 2; ++s)
            if (hr.buzz[s].seconds > 0.0f)
                input::Buzz(s, hr.buzz[s].amplitude, hr.buzz[s].seconds);
        // The game's own sounds: the vest's rustle at a put-away (in the middle by default, as the reloads' are); at a draw the gun's own deploy where
        // the game has one (a held gun's: the Stinger's draw_fim92), else the magnet's take.
        if (hr.sound == holster::Sound::kStow && g_holsterSet.stowSound != holster::StowSound::kNone)
            sounds::Play(g_holsterSet.stowSound == holster::StowSound::kSettle ? "stow_settle" : "stow", hr.pan);
        else if (hr.sound == holster::Sound::kDraw && p.pose == gunprofile::Pose::kLauncher && p.id != gunprofile::kSmawId)
            hands::PlayGunsOwn("draw", "take");
        else if (hr.sound == holster::Sound::kDraw)
            sounds::Play("take");
        SayHolster(hr.event, hr.events);
        // "lying recall": the lying gun off the ground and put away at its place (as a return takes it off), the draw that waited for it next frame.
        if (hr.recall && g_grasp.holder == Holder::kLoose) {
            if (g_gunLoose.id)
                loose::Remove(g_gunLoose.id);
            g_gunLoose = GunLoose{};
            g_gunRelease = GunRelease{};
            grasp::Clear(g_grasp);
            g_grasp.holder = Holder::kStowed;
            g_grasp.trigger = -1;
            g_grasp.headShare = 1.0f;
            g_graspNow.holder = Holder::kStowed;
            g_graspNow.trigger = g_graspNow.barrel = g_graspNow.fireHand = -1;
            g_graspNow.released = -1;
        }
        if (r.stowed >= 0)
            g_releaseAim = g_aimNow;  // (the game's aim eases from where it was to the head, as at a let-go)
    }
    // What follows from each of its events, this frame.
    for (int s = 0; s < 2; ++s)
        if (r.spent[s]) {
            g_freshBy[s] = false;  // (the squeeze that took the rifle isn't the handguard's too)
            g_spentBy[s] = true;
        }
    if (r.returned) {
        // (A gun lying when the gun changed: the game's own switch took it. The gun in the right hand now is this frame's: the new one after a change
        // of gun, never the one that lay (the review of e403947: flow (b) by X held said the M4A1 back in the hand as the AT4 came).)
        const char* lay = g_gunLoose.id ? g_gunLoose.name : "gun";
        const char* name = p.pose != gunprofile::Pose::kNone ? p.name : g_gunLoose.id ? g_gunLoose.name : g_profileLast.name;
        const bool layThere = returnNow && g_gunLoose.id != 0 && !g_pick.lyingRetired;
        if (g_gunLoose.id)
            loose::Remove(g_gunLoose.id);
        g_gunLoose = GunLoose{};
        g_gunRelease = GunRelease{};
        Log("Free hands: the %s back in the right hand (%s)", name, r.why && *r.why ? r.why : "its start");
        if (layThere)
            Log("Free hands: the %s on the ground went with the game's own switch", lay);
        g_pick.lyingRetired = false;  // (its copy went with the return. A take that waits or is due goes on: the grasp's arrival runs from any
                                      // state, and the net ends it if its gun never comes)
    }
    if (r.released >= 0) {
        // Started in StepLoose with this frame's Frame, from where it was drawn last frame, with what's in its magazine, as the gun it is.
        g_gunRelease = GunRelease{};
        g_gunRelease.want = true;
        g_gunRelease.side = r.released;
        g_gunRelease.fromGuard = g_grasp.beforeHolder == Holder::kBarrel;
        g_gunRelease.pose = p.pose;
        g_gunRelease.id = p.id;
        g_gunRelease.body = p.body;
        g_gunRelease.name = p.name;
        g_gunRelease.front = p.front;
        g_gunRelease.sound = p.sound;
        g_gunRelease.slot = SlotOfProfile(p);
        std::memcpy(g_gunRelease.room, g_lastGunDrawn, sizeof(g_gunRelease.room));  // (as drawn, kicked: let go mid-kick, it falls from where it was seen)
        int loaded = 0, spare = 0;
        g_gunRelease.rounds = !skin::MagazineOut() && ammo::Get(loaded, spare) ? loaded : 0;
        g_releaseAim = g_aimNow;
        if (!g_haveLastGunRoom)
            KeepTheRifle(t, fromHead);  // (nowhere it was drawn to fall from)
        else if (g_gunRelease.fromGuard && g_keptAt && GetTickCount64() - g_keptAt < kKeptRetry)
            KeepTheRifle(t, fromHead);  // (loose had no room a moment ago: a handguard let go of is tried again every kKeptRetry, not every frame)
    } else {
        g_keptAt = 0, g_keptSaid = false;  // (nothing let go of this frame: a streak of loose having no room is over)
    }
    if (r.toBarrel >= 0) {
        // Held by that hand's handguard alone: frozen on its grip where it was last frame (both last frame's: nothing jumps).
        const int o = r.toBarrel;
        float inverse[12];
        InvertRigid(g_haveLastGrip[o] ? g_lastGripRoom[o] : f.hand[o].pose, inverse);
        Compose(inverse, g_lastGunRoom, g_onBarrel);
        Log("Free hands: the %s hand let go of the %s's pistol grip: held by the %s hand on its %s alone", kSide[1 - o], p.name, kSide[o], p.front);
    }
    if (r.pickedUp >= 0) {
        if (g_gunLoose.id)
            loose::Remove(g_gunLoose.id);
        std::memcpy(g_glideFrom, g_gunLoose.room, sizeof(g_glideFrom));
        g_gunLoose = GunLoose{};
        g_glideAimFrom = g_aimNow;
        Log("Free hands: the %s hand picked the %s up (%.0f cm from it): by its pistol grip, its trigger fires it", kSide[r.pickedUp], p.name, r.reach * 100.0f);
        if (r.magnet)
            Log("Free hands: drawn to the hand from %.0f cm %s (%.0f cm off its line), in %.2f s", std::fabs(r.reachDepth) * 100.0f,
                r.reachDepth < 0.0f ? "above" : "below", r.reachOff * 100.0f, r.glideFor);
        g_glideFromGame = false;
    }
    if (r.missed >= 0 && GetTickCount64() - g_missSaidAt >= 1000) {
        g_missSaidAt = GetTickCount64();
        Log("Free hands: the %s hand squeezed low, the %s lying %.0f cm off its line %.0f cm %s: out of the magnet's reach", kSide[r.missed],
            g_gunLoose.id ? g_gunLoose.name : p.name, r.missOff * 100.0f, std::fabs(r.missDepth) * 100.0f, r.missDepth < 0.0f ? "above" : "below");
    }
    if (r.regrip >= 0) {
        g_glideFromGame = false;
        std::memcpy(g_glideFrom, g_lastGunRoom, sizeof(g_glideFrom));
        g_glideAimFrom = g_aimNow;
        Log("Free hands: the %s hand took the %s's pistol grip from the %s hand's hold on its %s", kSide[r.regrip], p.name, kSide[1 - r.regrip], p.front);
    }
    if (r.arrived >= 0) {
        Log("Free hands: the %s in the %s hand", p.name, kSide[r.arrived]);
        // The magnet's take closing in the hand: the game's own handling sound (take_<shape>.wav, else take.wav; none there: nothing plays). Only a take
        // the magnet made (from "reach" or further, or by the column): every take within "reach" stays as it was, silent.
        const int sound = freetune::SnapSound();
        if (f.magnet && r.arrivedByMagnet && (sound == 2 || (sound == 1 && !g_glideFromGame)))
            hands::PlayGunsOwn("take", "take");
    }
    if (r.reachedFor >= 0 && r.candidate >= 0 && r.candidate < g_reachCount) {
        const pickups::Reach& c = g_reach[r.candidate];
        const char* why = "";
        // A holstered gun stays till a hand takes it out and drops it (the user, 2026-11-18): a take that puts one into the pickup goes ahead, and the
        // gun stays on its shoulder, in that pickup (holster.h's NoteSwap: drawn back from there by the game's take of it). Only a take that couldn't
        // give it back isn't asked; the double click, and the line says why.
        const holster::TakeOut check = HolsterTakeCheck(c.entries, c.entriesAll, c.entrySlot, c.entryGun, c.replaceAll, c.unspawnOrStatic, c.kind);
        const int offSlot = check.verdict == holster::TakeVerdict::kRefuse ? check.slot : -1;
        if (offSlot >= 0) {
            char gun[48];
            HolsterGun(offSlot, gun, sizeof(gun));
            Log("Pickups: the %s hand reaches for the %s (%.0f cm from it): not asked: it would take %s off %s (slot %d), and %s: take that gun out "
                "and drop it first", kSide[r.reachedFor], pickups::Nice(c.name), r.reach * 100.0f, gun, holster::Name(check.at), offSlot,
                TakeRefusedWords(check.why));
            const holster::Pulse pulse = holster::RefusePulse(g_holsterSet, g_holster, r.reachedFor);
            if (pulse.seconds > 0.0f)
                input::Buzz(r.reachedFor, pulse.amplitude, pulse.seconds);  // (the second pulse 60 ms on: holster.h's Settle)
            g_pick.request = 0;
            g_pick.refusedNow = true;  // (the take waiting goes next frame)
        } else {
            g_pick.request = pickups::RequestTake(c.entity, c.blueprint, r.reachedFor, &why);
            if (g_pick.request && check.verdict == holster::TakeVerdict::kAway && check.lost) {
                SayTakeLost(pickups::Nice(c.name), check);
            } else if (g_pick.request && check.verdict == holster::TakeVerdict::kAway) {
                char gun[48];
                HolsterGun(check.slot, gun, sizeof(gun));
                Log("Pickups: the %s's take puts %s (slot %d) into its pickup: it stays %s, drawn back there by the game's take of that pickup",
                    pickups::Nice(c.name), gun, check.slot, holster::AtName(check.at));
            }
        }
        g_pick.side = r.reachedFor;
        g_pick.magnet = r.magnet;
        g_pick.giveBack = false;  // (a hand's take off the ground: it flies in from where it lies)
        strncpy_s(g_pick.name, c.name, _TRUNCATE);
        const float middle[3] = {(c.boxMin[0] + c.boxMax[0]) / 2, (c.boxMin[1] + c.boxMax[1]) / 2, (c.boxMin[2] + c.boxMax[2]) / 2};
        Transform(c.drawn, middle, g_pick.centreWorld);
        if (g_pick.request) {
            Log("Pickups: the %s hand reaches for the %s (%.0f cm from it): the game is asked to take it (#%u)", kSide[r.reachedFor], pickups::Nice(c.name),
                r.reach * 100.0f, g_pick.request);
            if (r.magnet)  // (only for a take asked: a refused one is drawn nowhere; worded apart from the lying gun's "drawn to the hand from")
                Log("Free hands: the magnet reached it %.0f cm %s (%.0f cm off its line), to fly in once the game gives it", std::fabs(r.reachDepth) * 100.0f,
                    r.reachDepth < 0.0f ? "above" : "below", r.reachOff * 100.0f);
        } else if (offSlot < 0) {
            Log("Pickups: the %s hand reaches for the %s (%.0f cm from it): not asked (%s)", kSide[r.reachedFor], pickups::Nice(c.name), r.reach * 100.0f, why);
            g_pick.refusedNow = true;  // (the take waiting goes next frame)
        }
    }
    if (r.gameGave >= 0) {
        std::memcpy(g_glideFrom, f.arriveFrom, sizeof(g_glideFrom));  // flies in from where it lay, as a pickup of the lying gun does from loose
        g_glideAimFrom = g_aimNow;
        if (g_gunLoose.id) {
            if (g_pick.byUse && g_gunLoose.slot == g_pick.landedSlot) {  // (a scene's gift: its kit took the lying gun's slot, the game's gun is gone)
                Log("Free hands: the %s on the ground is gone with the scene's kit (the %s took its place in the game's slots)", g_gunLoose.name, p.name);
            } else if (!g_pick.lyingRetired) {  // (a gun of another slot: the game has it in its slot still, on no shoulder: it lay, and holsters are by hand
                                                // only; a gift's too, a dropped Mk11 while the kit's AT4 takes slot 0: the review of e403947)
                Log("Free hands: the %s on the ground went back into the game's slot %d (the game draws one gun at a time): on no shoulder, the D-pad "
                    "or Y brings it", g_gunLoose.name, g_gunLoose.slot);
            }
            loose::Remove(g_gunLoose.id);
        }
        g_gunLoose = GunLoose{};
        g_gunRelease = GunRelease{};
        if (g_pick.byUse)
            Log("Free hands: the %s hand took the %s the scene gave: by its pistol grip, its trigger fires it", kSide[r.gameGave], p.name);
        else if (g_pick.giveBack)
            Log("Free hands: the %s in the %s hand from its shoulder (the game's own take of its pickup): by its pistol grip, its trigger fires it", p.name,
                kSide[r.gameGave]);
        else
            Log("Free hands: the %s hand took the %s off the ground (the game's own dropped gun): by its pistol grip, its trigger fires it", kSide[r.gameGave],
                p.name);
        g_glideFromGame = true;
        if (f.magnet && f.arriveMagnet)
            Log("Free hands: the game's dropped gun drawn to the hand in %.2f s", r.glideFor);
        const PickState was = g_pick;
        g_pick = PickState{};
        g_pick.swapSeen = was.swapSeen;
        g_pick.kickOutId = was.kickOutId, g_pick.kickOutAt = was.kickOutAt;  // (recoil's wait for the gun swapped out runs on: the recoil block)
    }
    if (r.pendingLapsed) {
        Log("Pickups: a take (#%u) not answered in %.0f s: the hand is free again", g_pick.request, grasp::kPendingMost);
        g_pick.request = 0;  // (free for the candidates too: the grasp gave it up)
    }
    if (r.trigger != triggerWas) {  // another trigger hand: the steer starts afresh (its lines were the other hand's)
        g_steer = steer::State{};
        g_bodyLine = steer::BodyLine{};
    }
    // Where the gun is for the modules after: the right hand's as before; the left's own place; frozen on the hand holding its handguard alone (also while the other
    // takes its pistol grip from there); none while it flies into a hand from the world or lies, drawn all the same. On its own pose (launcherPose for any gun but
    // the M416, whose flags then stay as when dormant: none of its draws come).
    const grasp::Result& now = g_graspNow;  // (KeepTheRifle may have put it back)
    for (int s = 0; s < 2; ++s)
        g_onGripBy[s] = (now.holder == Holder::kHeld || now.holder == Holder::kGlide) && now.trigger == s;
    const bool fromBarrel = now.holder == Holder::kGlide && !now.glideFromWorld;
    if (now.holder == Holder::kHeld && now.trigger == 0) {
        t.gun = g_haveHandGun[0];
        if (t.gun)
            std::memcpy(pose, g_handGun[0], sizeof(g_handGun[0]));
        if (t.gun && g_haveOtherLeft)  // (the other pose in the left hand too: a switch's new gun there from its first frame)
            std::memcpy(lp ? t.gunPose : t.launcherPose, g_otherLeft, sizeof(g_otherLeft));
    } else if (now.holder == Holder::kBarrel || fromBarrel) {
        const int b = now.barrel;
        t.gun = b >= 0 && f.hand[b].tracked;
        if (t.gun) {
            float onHand[12];
            Compose(f.hand[b].pose, g_onBarrel, onHand);
            Compose(fromHead, onHand, pose);
        }
    } else if (now.holder == Holder::kGlide || now.holder == Holder::kLoose || now.holder == Holder::kStowed) {
        t.gun = false;  // (put away at its place: in no hand, lying nowhere, drawn nowhere)
        if (now.holder == Holder::kGlide && now.trigger >= 0)
            std::memcpy(pose, g_handGun[now.trigger], sizeof(g_handGun[now.trigger]));  // (where it goes: the gloves go by it)
        else if (now.holder != Holder::kStowed && g_gunLoose.have)
            Compose(fromHead, g_gunLoose.room, pose);
    }
    drawn = t.gun || now.holder == Holder::kGlide || (now.holder == Holder::kLoose && (g_gunLoose.have || g_gunRelease.want));
    lyingNow = now.holder == Holder::kLoose && g_gunLoose.have;
    // Holsters: the gun drawn nowhere (skin.cpp: every draw of the game's weapon left out, what lies and what a hand holds drawn as before): put away
    // at its place, a draw waiting (the key sent and its gun not in the hand yet; a take by a hand not seen), or the hand holding it not seen and
    // taken to be at a place (it may be putting it away there: the game would draw its own at the camera meanwhile).
    t.stowed = now.holder == Holder::kStowed || hr.unseenAtPlace || hr.drawWaits;
    if (lyingNow) {
        std::memcpy(world, g_gunLoose.bone, sizeof(g_gunLoose.bone));
        for (int i = 0; i < 3; ++i)
            worldOrigin[i] = static_cast<float>(g_gunLoose.origin[i]);
    }
    if (lp)
        t.gunDrawn = t.gun;
    // Each hand's grip now, kept for the next frame (a hold on the handguard alone starts from it).
    for (int s = 0; s < 2; ++s) {
        g_haveLastGrip[s] = f.hand[s].tracked;
        if (f.hand[s].tracked)
            std::memcpy(g_lastGripRoom[s], f.hand[s].pose, sizeof(g_lastGripRoom[s]));
    }
    g_fireHandNow = now.fireHand;
    g_holdsGunNow = now.holder == Holder::kHeld || now.holder == Holder::kBarrel;
    g_gripHandNow = now.holder == Holder::kHeld && (now.trigger == 0 || now.trigger == 1) ? now.trigger : -1;
    // Holsters: the gun put away, and its words for input.cpp ("the M416 at the right shoulder").
    g_stowedNow = now.holder == Holder::kStowed;
    g_stowedId = now.holder == Holder::kStowed && p.pose != gunprofile::Pose::kNone ? p.id : 0;
    g_stowedWhere[0] = 0;
    if (now.holder == Holder::kStowed) {
        holster::Place places[holster::kSlots];
        holster::PlacesNow(hf.slots, g_holster, g_holsterSet, places);
        const int slot = g_holster.gameSlot;
        const holster::Place at = slot >= 0 && slot < holster::kSlots ? places[slot] : holster::Place::kNone;
        char gun[48];
        HolsterGun(slot, gun, sizeof(gun));
        if (at != holster::Place::kNone)
            snprintf(g_stowedWhere, sizeof(g_stowedWhere), "%s %s", gun, holster::AtName(at));
        else
            snprintf(g_stowedWhere, sizeof(g_stowedWhere), "%s put away", gun);
    }
    g_freeHandsAt = GetTickCount64();
    // The hand at work: the one not on the pistol grip (a regrip's glide: the hand on the handguard), or the free one while the rifle's held by its handguard
    // alone; nobody while it lies; the left while dormant, as before (workhand.h: every gun, either hand in either role).
    const int workWas = g_workHand;
    g_workHand = workhand::WorkHand(enabled, now.holder, now.trigger, now.barrel);
    if (g_workHand != workWas) {
        const char* name = p.pose != gunprofile::Pose::kNone ? p.name : g_profileLast.name;
        if (g_workHand >= 0)
            Log("Free hands: the %s hand works the %s now", kSide[g_workHand], name);
        else
            Log("Free hands: no hand works the %s (nobody holds it)", name);
    }
    // The free right glove's fingers, from the right controller alone (as the left's from the left's): its pose and its collision with the gun go by them.
    if (enabled) {
        g_indexBy[1] += (TriggerOf(1) - g_indexBy[1]) * 0.35f;
        g_gripBy[1] += (SqueezeOf(1) - g_gripBy[1]) * 0.35f;
        g_thumbBy[1] += ((ThumbOf(1) ? 1.0f : 0.0f) - g_thumbBy[1]) * 0.35f;
    }
    // The game's own right hand on this gun's pistol grip (here, not before the grasp: its wait reads this frame's hold, g_onGripBy); the gun the left holds
    // by its pistol grip; the pistol-grip hand, for a switch (the switch's memory); this frame's gun, for the next frame's change ("off" and back is none).
    if (enabled)
        FetchTriggerGrip(p);
    if (g_onGripBy[0])
        g_leftGripProfile = p;
    if (enabled)
        g_lastHeldSide = now.holder == Holder::kHeld ? now.trigger : -1, g_lastHeldAt = GetTickCount64();
    pickups::SetReaching(enabled && (now.holder == Holder::kLoose || now.holder == Holder::kStowed || g_pick.request != 0 || g_pick.landedSide >= 0));
    g_profileWas = p.id;
    if (p.pose != gunprofile::Pose::kNone && p.name)
        g_profileWasName = p.name;
}

// ---- The right glove (skin.h RightGlove), before PlaceForearms (the right arm is solved from it). Dormant, or with the right on the pistol grip: on the gun as the game
// holds it, as before. Free hands otherwise: its own pose (free: the left's relaxed, pointing and fist poses flipped, moved by the right controller alone; on the
// handguard: the game's left hold flipped), easing onto and off its holds over 5 frames; hidden while it isn't seen and the rifle's drawn elsewhere. ----
float g_rightBlend = 1.0f;  // 0 free, 1 on its hold (a launch starts on the grip)
int g_rightHold = 1;        // the hold it's on or easing off: 1 the pistol grip, 2 the handguard, 3 the grenade launcher's grip
float g_rightHoldOn[12] = {};  // ... its place on the free glove (the free glove's bind space -> the hold's), kept for the ease off it
float g_rightHoldFingers[handpose::kRightBones][12] = {};  // ... its bones relative to its root (37)
bool g_haveRightHold = false;
float g_goodRightPose[12] = {}, g_goodRightFingers[handpose::kRightBones][12] = {};  // as last drawn with every number finite
bool g_haveGoodRight = false;

// The free right glove's own fingers, from its controller alone (smoothed in FreeHands), as RightGlove poses the free glove: its collision goes by them.
handpose::Fingers RightFreeFingers(float slim) {
    handpose::Fingers fingers;
    fingers.curl[0] = g_indexBy[1];
    fingers.curl[1] = fingers.curl[2] = fingers.curl[3] = g_gripBy[1];
    fingers.thumb = g_thumbBy[1] * (0.6f + 0.4f * g_gripBy[1]);
    fingers.spread = g_spread;
    fingers.slim = slim;
    return fingers;
}

// What the right hand works this frame besides its holds (free hands: the right at work while the left holds the rifle), from Update: the pouch's magazine
// and the launcher's reload (side 1's: its glove and fingers), the launcher's table re-anchored on the right's palm (the left glove's layout, bones 0-22) and
// that rebase, and the move that keeps the free glove out of the gun (KeepOutOfGunRight, the view's space).
struct RightWork {
    const pouch::Result* magazine = nullptr;
    const m320::Result* m320 = nullptr;
    const float (*m320Table)[12] = nullptr;
    float m320Rebase[12] = {};
    bool haveM320Rebase = false;
    bool collided = false;
    float shift[3] = {};
    const shellload::Result* shell = nullptr;  // a tube shotgun's shells in this hand (side 1's: its pinch, its glove riding the shell or on the handle)
    // A held gun's magazine, box, lever, lid or belt in this hand (handmag.h, side 1's: its glove on it), and its fingers as Update eased them: the fitted
    // table (the left's layout: flipped onto the right) by its share, the fist on a lever with no table for it by its own.
    const handmag::Result* held = nullptr;
    const float (*heldFingers)[12] = nullptr;
    float heldShare = 0.0f, heldFist = 0.0f;
};

void RightGlove(skin::HandTargets& t, const float* fromHead, const float* toView, const RightWork& work) {
    using grasp::Holder;
    const grasp::Result& now = g_graspNow;
    // On the charging handle (the right hand has it: reload.cpp), eased in and out as the left's (OnTheLever).
    g_leverBy[1] += ((t.gun && reload::HandleSide() == 1 ? 1.0f : 0.0f) - g_leverBy[1]) * 0.35f;
    if (!g_freeOn) {
        // Dormant: the right hand on the gun as the game holds it while its controller's seen, the game's own otherwise (as before).
        g_rightBlend = 1.0f;
        g_rightHold = 1;
        g_holdOutOfReachBy[1] = false;
        t.rightGlove = t.gun ? skin::kRightOnGrip : skin::kRightGame;
        std::memcpy(t.rightGunPose, t.gunPose, sizeof(t.rightGunPose));
        std::memcpy(t.rightLauncherPose, t.launcherPose, sizeof(t.rightLauncherPose));
        t.rightOnPistol = true;
        return;
    }
    const bool onGrip = (now.holder == Holder::kHeld || now.holder == Holder::kGlide) && now.trigger == 1;
    t.rightOnPistol = onGrip;  // (only then the game's own left hand on a launcher-pose gun whose left isn't the player's)
    const bool lp = g_profile.pose == gunprofile::Pose::kLauncher;  // (the gun in hand is on launcherPose)
    const float* gunPose = lp ? t.launcherPose : t.gunPose;
    // (Its own fingers, from the right controller alone, are smoothed in FreeHands, kept up to date for the ease off a hold.)
    // On the handguard: the game's own left hold flipped onto the right glove, its fingers bent a little with the right controller's (in the left's layout, with the left's
    // axes, then flipped). On the grenade launcher's grip, the game's own left hold there flipped alike, its index on the trigger as the left's (the left's own fit: the
    // trigger's face is on the rifle's middle, so the same table flipped is the right's), lifted onto the release while this hand presses it.
    // A launcher-pose gun's front grip: its rig's rest left hand there flipped alike, this gun's own rest only.
    float gameLeft[handpose::kBones][12], onGuard[handpose::kRightBones][12];
    const bool onM320 = g_supportingBy[1] && g_onM320By[1];
    uint32_t restKind = 0;
    const bool haveGuard = lp ? g_onLauncherBy[1] && t.gun && skin::RigRestLeft(gameLeft, restKind) && restKind == (g_profile.id == gunprofile::kSmawId ? 0u : g_profile.id)
                              : g_supportingBy[1] && t.gun && (onM320 ? skin::GameLeftGripM320(gameLeft) : skin::GameLeftGrip(gameLeft));
    if (haveGuard) {
        if (onM320) {
            if (!g_m320IndexFitted) {  // (fitted again only on a hold learned anew since, as the left's)
                if (g_m320HaveFitOn && !std::memcmp(gameLeft[9], g_m320FitOn[0], sizeof(g_m320FitOn[0])) &&
                    !std::memcmp(gameLeft[19], g_m320FitOn[1], sizeof(g_m320FitOn[1])) && !std::memcmp(gameLeft[15], g_m320FitOn[2], sizeof(g_m320FitOn[2])))
                    g_m320IndexFitted = true;
                else
                    FitIndexToM320Trigger(gameLeft, 1);
            }
            const bool pressing = work.m320 && work.m320->gripSide == 1;
            BendWithController(gameLeft, Bend::kM320Trigger, pressing ? work.m320->indexOnRelease : 0.0f, pressing ? work.m320->releaseLift : 0.0f, SqueezeOf(1),
                               TriggerOf(1), ThumbOf(1), g_fingerBendBy[1]);
        } else {
            BendWithController(gameLeft, Bend::kHandguard, 0.0f, 0.0f, SqueezeOf(1), TriggerOf(1), ThumbOf(1), g_fingerBendBy[1]);
        }
        handpose::LeftToRight(gameLeft, onGuard);
    }
    // Off the handguard while the right arm can't reach it, as the left's (not while the rifle's held by this hand alone: that hold moves with the hand).
    float shoulder[3];
    if (haveGuard && g_rightSeen && now.barrel != 1 && SquareShoulder(1, fromHead, toView, shoulder)) {
        const float wristBind[3] = {-armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]};
        float palmBone[12], holdWrist[3], handWrist[3];
        Compose(AsHeld(t, gunPose), onGuard[32 - handpose::kRightFirst], palmBone);  // the glove's palm bone on the gun as the hands hold it (never off it for a kick), as ArmHandRight has it
        Transform(palmBone, wristBind, holdWrist);
        Transform(g_rightFree, wristBind, handWrist);
        const float* at = g_atBy[now.trigger == 1 ? 1 : 0];  // the trigger hand's grip point
        const float toHold[3] = {holdWrist[0] - shoulder[0], holdWrist[1] - shoulder[1], holdWrist[2] - shoulder[2]};
        const float handsApart[3] = {at[0] - handWrist[0], at[1] - handWrist[1], at[2] - handWrist[2]};
        const float reach = Length(toHold), away = Length(handsApart);
        if (!g_holdOutOfReachBy[1] && reach > kHoldOutOfReach && away > kHoldHandAway) {
            g_holdOutOfReachBy[1] = true;
            Log("Hands: the hold %.2f m from the right shoulder, out of the arm's reach, the real hands %.2f m apart: the right glove off the gun until it's back in reach", reach, away);
        } else if (g_holdOutOfReachBy[1] && (reach < kHoldInReach || away < kHoldHandBack)) {
            g_holdOutOfReachBy[1] = false;
            Log("Hands: the hold %.2f m from the right shoulder, the real hands %.2f m apart: the right glove back on the gun", reach, away);
        }
    } else {
        g_holdOutOfReachBy[1] = false;
    }
    const bool wantGuard = haveGuard && !g_holdOutOfReachBy[1];
    if (onGrip)
        g_rightHold = 1;
    else if (wantGuard)
        g_rightHold = onM320 ? 3 : 2;
    g_rightBlend = onGrip || wantGuard ? std::fmin(g_rightBlend + kGripBlendStep, 1.0f) : std::fmax(g_rightBlend - kGripBlendStep, 0.0f);
    if (!g_rightSeen) {
        // (Not the game's right hand at its hold while the gun is drawn nowhere: put away, or its hand not seen at a place.)
        t.rightGlove = t.gun || t.gunDrawn || t.launcherDrawn || t.stowed ? skin::kRightHidden : skin::kRightGame;
        return;
    }
    // Fully on the pistol grip: the game's own right hand there, each pass its own (today's numbers while it holds the rifle; at the hand while the rifle flies into it).
    if (onGrip && g_rightBlend >= 1.0f) {
        t.rightGlove = skin::kRightOnGrip;
        std::memcpy(t.rightGunPose, now.holder == Holder::kHeld ? t.gunPose : GlideTarget(1), sizeof(t.rightGunPose));
        std::memcpy(t.rightLauncherPose, now.holder == Holder::kHeld ? t.launcherPose : GlideTarget(1), sizeof(t.rightLauncherPose));
        return;
    }
    // freehands.txt "rightpose 0": the game's grip carried at the right controller instead of a pose of its own.
    if (!freetune::RightPose()) {
        t.rightGlove = skin::kRightOnGrip;
        std::memcpy(t.rightGunPose, g_handGun[1], sizeof(t.rightGunPose));
        std::memcpy(t.rightLauncherPose, g_handGun[1], sizeof(t.rightLauncherPose));
        return;
    }
    // Its own pose: the free glove (the left's pose flipped, by the right controller alone), eased onto its hold.
    handpose::Fingers fingers;
    fingers.curl[0] = g_indexBy[1];
    fingers.curl[1] = fingers.curl[2] = fingers.curl[3] = g_gripBy[1];
    fingers.thumb = g_thumbBy[1] * (0.6f + 0.4f * g_gripBy[1]);
    fingers.spread = g_spread;
    float width = 1.0f, slim = 1.0f;
    freetune::RightWidth(width, slim);
    fingers.slim = slim;
    // Round a magazine, and the V on the charging handle: the left's own numbers (the Fingers are either glove's: posed as the left's, then flipped), eased as
    // the left's.
    if (g_dropOpenBy[1] > 0.001f)
        FingersOpened(fingers, g_dropOpenBy[1]);  // (a magazine dropped by force from the squeezed hand: open, above)
    if (g_magFingersBy[1] > 0.001f)
        FingersOnMagazine(fingers, g_magFingersBy[1]);
    if (g_leverBy[1] >= 0.001f && t.gun)
        FingersOnLever(fingers, g_leverBy[1]);
    // A fist on a held gun's lever with no table for this hand (a lever off the gun's middle), as the left's: every finger closed round the knob, the thumb over.
    if (work.heldFist > 0.001f) {
        for (int i = 0; i < 4; ++i)
            fingers.curl[i] += (0.88f - fingers.curl[i]) * work.heldFist;
        fingers.thumb += (0.8f - fingers.thumb) * work.heldFist;
    }
    handpose::PoseRight(fingers, width, t.rightFingers);
    // A held gun's own fitted hand in this hand (its magazine, a handle on the gun's middle, the lid, the belt's pinch): the left's table flipped onto the
    // right glove, in by its share about each bone's own joint (on bare skin without the glove's widening, as the left's).
    if (work.heldFingers && work.heldShare > 0.001f) {
        float flipped[handpose::kRightBones][12], skinTable[handpose::kBones][12];
        const bool bareSkin = skin::BareHold() && skin::BareArmsLately();
        if (bareSkin)
            handpose::SkinTable(work.heldFingers, skinTable);
        handpose::LeftToRight(bareSkin ? skinTable : work.heldFingers, flipped);
        for (int r = 0; r < handpose::kRightBones; ++r) {
            float pivot[3], blended[12];
            handpose::PivotRight(handpose::kRightFirst + r, pivot);
            BlendAbout(t.rightFingers[r], flipped[r], work.heldShare, pivot, blended);
            std::memcpy(t.rightFingers[r], blended, sizeof(blended));
        }
    }
    // The grenade launcher's reload in the right hand (side 1): the left's table re-anchored on the right's palm (Update), flipped onto the right glove, in by the
    // module's own share about each bone's own joint.
    if (work.m320 && work.m320->side == 1 && work.m320Table && work.m320->fingersShare > 0.001f) {
        float flipped[handpose::kRightBones][12];
        handpose::LeftToRight(work.m320Table, flipped);
        for (int r = 0; r < handpose::kRightBones; ++r) {
            float pivot[3], blended[12];
            handpose::PivotRight(handpose::kRightFirst + r, pivot);
            BlendAbout(t.rightFingers[r], flipped[r], work.m320->fingersShare, pivot, blended);
            std::memcpy(t.rightFingers[r], blended, sizeof(blended));
        }
    }
    // A shotgun's shell in the right hand (side 1): the fitted pinch (the left's table) flipped onto the right glove, in by the module's own share; on its
    // bolt handle, a fist.
    // (On bare skin, Semper Fidelis's and The Great Destroyer's hands, without the glove's widening it was made with, as the left's: handpose::SkinTable.)
    if (work.shell && work.shell->side == 1 && work.shell->fingers && work.shell->fingersShare > 0.001f) {
        float flipped[handpose::kRightBones][12], skinTable[handpose::kBones][12];
        const bool bareSkin = skin::BareHold() && skin::BareArmsLately();
        if (bareSkin)
            handpose::SkinTable(work.shell->fingers, skinTable);
        handpose::LeftToRight(bareSkin ? skinTable : work.shell->fingers, flipped);
        for (int r = 0; r < handpose::kRightBones; ++r) {
            float pivot[3], blended[12];
            handpose::PivotRight(handpose::kRightFirst + r, pivot);
            BlendAbout(t.rightFingers[r], flipped[r], work.shell->fingersShare, pivot, blended);
            std::memcpy(t.rightFingers[r], blended, sizeof(blended));
        }
    }
    // The opposed thumb laid flat on the magazine's far side, as the left's: bent in the left's layout, then flipped back (exact).
    if (g_magFingersBy[1] > 0.001f) {
        float left[handpose::kBones][12];
        handpose::RightToLeft(t.rightFingers, left);
        const float press[5] = {-15.49f * g_magFingersBy[1], 0.0f, 0.0f, 0.0f, 0.0f};
        handpose::BendFurther(left, press);
        handpose::LeftToRight(left, t.rightFingers);
    }
    std::memcpy(t.rightGlovePose, g_rightFree, sizeof(t.rightGlovePose));
    if (g_rightBlend > 0.0f) {
        float hold[12], letGo[12];
        bool have = false;
        const float* source = nullptr;  // the hold's bones on the gun (23-44, [bone - 23])
        const float* on = nullptr;      // the gun it's on
        if (g_rightHold == 1 && g_haveRightGrip && g_rightGripFor == g_profile.id) {
            // The pistol grip: the game's own right hand there (this gun's: another gun's hand is never eased onto it), on the gun as the right holds it; flying
            // into it, where the glide takes it; let go of, where the right hand puts it with the turn it last rode (so the glove opens where it was drawn, steered
            // or not, and goes with the hand).
            source = &g_gameRightGrip[0][0];
            if (onGrip) {
                on = now.holder == Holder::kHeld ? gunPose : GlideTarget(1);
            } else if (g_haveGripRel[1]) {
                Compose(g_handGun[1], g_gripRel[1], letGo);
                on = letGo;
            } else {
                on = g_handGun[1];
            }
        } else if ((g_rightHold == 2 || g_rightHold == 3) && wantGuard) {
            source = &onGuard[0][0];
            on = g_regripDrawn ? g_glideDrawn : gunPose;  // (a regrip's glide: on the gun as it's drawn, never left behind)
        }
        if (source) {
            const float* root = source + (37 - handpose::kRightFirst) * 12;
            float inverse[12];
            if (Invert(root, inverse)) {
                Compose(on, root, hold);
                for (int r = 0; r < handpose::kRightBones; ++r)
                    Compose(inverse, source + r * 12, g_rightHoldFingers[r]);
                // Its place on the free glove, kept for the ease off it once let go (it goes with the right hand: the glove opens where it was).
                float freeInverse[12];
                if (Invert(g_rightFree, freeInverse)) {
                    Compose(freeInverse, hold, g_rightHoldOn);
                    g_haveRightHold = true;
                }
                have = true;
            }
        } else if ((g_rightHold == 2 || g_rightHold == 3) && g_haveRightHold) {
            Compose(g_rightFree, g_rightHoldOn, hold);  // (let go of the handguard or the launcher's grip: easing off where it was, with the hand)
            have = true;
        }
        if (have) {
            const float rightAnchor[3] = {-g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2]};  // the left's palm point mirrored
            float blended[12];
            BlendAbout(g_rightFree, hold, g_rightBlend, rightAnchor, blended);
            std::memcpy(t.rightGlovePose, blended, sizeof(blended));
            for (int r = 0; r < handpose::kRightBones; ++r) {
                float pivot[3];
                handpose::PivotRight(handpose::kRightFirst + r, pivot);  // each bone blended about its own joint
                BlendAbout(t.rightFingers[r], g_rightHoldFingers[r], g_rightBlend, pivot, blended);
                std::memcpy(t.rightFingers[r], blended, sizeof(blended));
            }
        }
    }
    // What it works, each blended in about its palm point as the left's (Update): the left's computation flipped.
    {
        const float rightAnchor[3] = {-g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2]};  // the left's palm point mirrored
        float blended[12];
        // A magazine caught on the well's line in this hand (the pouch's side 1): the glove goes with it, easing in and back out to the hand.
        if (work.magazine && work.magazine->side == 1 && work.magazine->gloveBlend > 0.0f) {
            BlendAbout(t.rightGlovePose, work.magazine->glove, work.magazine->gloveBlend, rightAnchor, blended);
            std::memcpy(t.rightGlovePose, blended, sizeof(blended));
        }
        // A held gun's magazine or box on its way in or out, the fist or the fitted hand on its lever, the hand on its lid (handmag's side 1: that glove's own
        // place), likewise.
        if (work.held && work.held->side == 1 && work.held->gloveBlend > 0.0f) {
            BlendAbout(t.rightGlovePose, work.held->glove, work.held->gloveBlend, rightAnchor, blended);
            std::memcpy(t.rightGlovePose, blended, sizeof(blended));
        }
        // The grenade launcher's reload by this hand: its glove on the round's rim, on the round going home, as the game's own hand there flipped, re-anchored on
        // its palm as its fingers are (the rebase is in the left's layout: mirrored into the right glove's).
        if (work.m320 && work.m320->side == 1 && work.m320->gloveBlend > 0.0f) {
            float hold[12], onGun[12], rebaseInverse[12], mirrored[12];
            if (work.haveM320Rebase && Invert(work.m320Rebase, rebaseInverse)) {
                handpose::Mirror(rebaseInverse, mirrored);
                Compose(work.m320->gloveOnGun, mirrored, hold);
            } else {
                std::memcpy(hold, work.m320->gloveOnGun, sizeof(hold));
            }
            Compose(t.gunPose, hold, onGun);
            BlendAbout(t.rightGlovePose, onGun, work.m320->gloveBlend, rightAnchor, blended);
            std::memcpy(t.rightGlovePose, blended, sizeof(blended));
        }
        // A shotgun's shell riding up its loading port in this hand, or this hand on its bolt handle: the glove on the gun with it (shellload.h).
        if (work.shell && work.shell->side == 1 && work.shell->gloveBlend > 0.0f) {
            float onGun[12];
            Compose(t.launcherPose, work.shell->gloveOnGun, onGun);
            BlendAbout(t.rightGlovePose, onGun, work.shell->gloveBlend, rightAnchor, blended);
            std::memcpy(t.rightGlovePose, blended, sizeof(blended));
        }
        // Free, kept out of the gun (KeepOutOfGunRight): drawn moved out (its reloads still go by the hand).
        if (work.collided)
            for (int r = 0; r < 3; ++r)
                t.rightGlovePose[r * 4 + 3] += work.shift[r];
        // On the charging handle, slid back with it: the left's place there mirrored (the T sits on the rifle's middle).
        if (g_leverBy[1] >= 0.001f && t.gun) {
            const float back[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -reload::HandleBack()};
            float onLever[12], slid[12], onHandle[12];
            handpose::Mirror(reload::kOnLever, onLever);
            Compose(t.gunPose, back, slid);
            Compose(slid, onLever, onHandle);
            BlendAbout(t.rightGlovePose, onHandle, g_leverBy[1], rightAnchor, blended);
            std::memcpy(t.rightGlovePose, blended, sizeof(blended));
        }
    }
    t.rightGlove = skin::kRightOwn;
    // Never a number that isn't finite (its whole draw would go): the last good one, else the game's grip at the right controller.
    if (Finite(t.rightGlovePose, 12) && Finite(&t.rightFingers[0][0], handpose::kRightBones * 12)) {
        std::memcpy(g_goodRightPose, t.rightGlovePose, sizeof(g_goodRightPose));
        std::memcpy(g_goodRightFingers, t.rightFingers, sizeof(g_goodRightFingers));
        g_haveGoodRight = true;
    } else {
        static ULONGLONG s_at = 0;
        if (GetTickCount64() - s_at > 1000) {
            s_at = GetTickCount64();
            Log("Hands: the right glove came out with a number not finite (the free glove %s, on its hold %.2f, the hold %d): %s", Finite(g_rightFree, 12) ? "finite" : "NOT finite",
                g_rightBlend, g_rightHold, g_haveGoodRight ? "drawn as it last was" : "drawn on the grip at the right controller");
        }
        if (g_haveGoodRight) {
            std::memcpy(t.rightGlovePose, g_goodRightPose, sizeof(t.rightGlovePose));
            std::memcpy(t.rightFingers, g_goodRightFingers, sizeof(t.rightFingers));
        } else {
            t.rightGlove = skin::kRightOnGrip;
            std::memcpy(t.rightGunPose, g_handGun[1], sizeof(t.rightGunPose));
            std::memcpy(t.rightLauncherPose, g_handGun[1], sizeof(t.rightLauncherPose));
        }
    }
}

// ---- The game's aim from free hands (hands::Aim, read by xr_runtime), once the rifle's place is final: along the rifle as drawn wherever a hand holds it; while it
// flies into a hand, from where the aim was to along that hand's place for it; while it lies, easing to the head. Dormant, or held by the right with its own
// controller: the old way (Aim false). Any other change of its source eased over 0.15 s. ----
XrQuaternionf Slerp(XrQuaternionf a, XrQuaternionf b, float s) {
    s = std::fmin(std::fmax(std::isfinite(s) ? s : 1.0f, 0.0f), 1.0f);
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (!std::isfinite(dot))
        return std::isfinite(b.x + b.y + b.z + b.w) ? b : a;
    if (dot < 0.0f)
        b = {-b.x, -b.y, -b.z, -b.w}, dot = -dot;
    float wa = 1.0f - s, wb = s;
    if (dot < 0.9995f) {
        const float angle = std::acos(std::fmin(dot, 1.0f)), sine = std::sin(angle);
        wa = std::sin((1.0f - s) * angle) / sine, wb = std::sin(s * angle) / sine;
    }
    XrQuaternionf q{a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb};
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return n > 1e-6f && std::isfinite(n) ? XrQuaternionf{q.x / n, q.y / n, q.z / n, q.w / n} : b;
}

// The drawn gun's line in the aim pose's axes, in the recentered space: the right hand's placing (flip, then the raise by its sights' up, degrees) taken back
// off, so the right's unsteered gun gives its own aim pose back exactly.
XrQuaternionf RifleLine(const float* gunPose, const float* headTurn, float upDegrees, float rightDegrees = 0.0f) {
    const float up = upDegrees * 3.14159265f / 180.0f;
    const float lower[9] = {1, 0, 0, 0, std::cos(up), -std::sin(up), 0, std::sin(up), std::cos(up)};  // (the raise turned back)
    const float flip[9] = {-1, 0, 0, 0, 1, 0, 0, 0, -1};
    const float turn[9] = {gunPose[0], gunPose[1], gunPose[2], gunPose[4], gunPose[5], gunPose[6], gunPose[8], gunPose[9], gunPose[10]};
    float a[9], b[9], m[9];
    Mul3(headTurn, turn, a);
    if (rightDegrees != 0.0f && std::isfinite(rightDegrees)) {  // (SightsTurn's turn about y undone first: its inverse is raise^-1 after it)
        const float r = rightDegrees * 3.14159265f / 180.0f;
        const float back[9] = {std::cos(r), 0, -std::sin(r), 0, 1, 0, std::sin(r), 0, std::cos(r)};
        float c[9];
        Mul3(a, back, c);
        std::memcpy(a, c, sizeof(a));
    }
    Mul3(a, lower, b);
    Mul3(b, flip, m);
    XrQuaternionf q = MatrixToQuat(m);
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return n > 1e-6f && std::isfinite(n) ? XrQuaternionf{q.x / n, q.y / n, q.z / n, q.w / n} : XrQuaternionf{0, 0, 0, 1};
}

void PublishAim(const skin::HandTargets& t, const XrPosef& head, const XrPosef& aim, const float* headTurn, float dt) {
    using grasp::Holder;
    const grasp::Result& now = g_graspNow;
    const bool rightByTest = g_test[1].on && g_test[1].step.where != freetest::Where::kStandIn;  // (a test's right hand: its aim isn't the controller's)
    // Along the gun in hand as drawn (its own pose), raised by its own sights' up.
    const float* gunPose = g_profile.pose == gunprofile::Pose::kLauncher ? t.launcherPose : t.gunPose;
    const float upDegrees = g_profile.pose == gunprofile::Pose::kNone ? skin::RifleUp() : g_profile.up;
    const float rightDegrees = g_profile.pose == gunprofile::Pose::kNone ? 0.0f : g_profile.right;  // (the AT4's sights' line right of its tube)
    XrQuaternionf want = head.orientation;
    bool own = true;
    switch (now.holder) {
    case Holder::kHeld:
        if (now.trigger == 1 && !rightByTest) {
            own = false;  // the old way: the right controller's aim (steered: AimOrientation), the head without it
            want = !g_rightSeen ? head.orientation : g_aimTurned ? g_aimTurn : aim.orientation;
        } else if (t.gun) {
            want = RifleLine(gunPose, headTurn, upDegrees, rightDegrees);
        }
        break;
    case Holder::kBarrel:
        if (t.gun)
            want = RifleLine(gunPose, headTurn, upDegrees, rightDegrees);
        break;
    case Holder::kGlide:
        want = Slerp(g_glideAimFrom, RifleLine(GlideTarget(now.trigger == 0 ? 0 : 1), headTurn, upDegrees, rightDegrees), now.glide);
        break;
    case Holder::kLoose:
    case Holder::kStowed:  // (put away at its place: to the head as while it lies)
        want = Slerp(g_releaseAim, head.orientation, grasp::Smooth(now.headShare));
        break;
    }
    // A change the blends above don't cover (the rifle back in the right hand, let go of onto the other hand's handguard, its holding hand lost or back): eased.
    // (Its source last frame is reset with the grasp; and back after a gap in Update, nothing eases from an aim of before it.)
    const bool covered = now.holder == Holder::kGlide || now.holder == Holder::kLoose || now.holder == Holder::kStowed;
    const bool changed = now.holder != g_aimHolderWas || now.trigger != g_aimTriggerWas || now.barrel != g_aimBarrelWas;
    if (g_freeResumed) {
        g_easeFor = kAimEase;
    } else if (!covered && (changed || (own && t.gun != g_aimGunWas))) {
        g_easeFrom = g_aimNow;
        g_easeFor = 0.0f;
    }
    if (g_easeFor < kAimEase) {
        want = Slerp(g_easeFrom, want, grasp::Smooth(g_easeFor / kAimEase));
        g_easeFor += std::fmax(dt, 0.001f);
        own = true;
    }
    g_aimHolderWas = now.holder, g_aimTriggerWas = now.trigger, g_aimBarrelWas = now.barrel, g_aimGunWas = t.gun;
    if (!(std::isfinite(want.x) && std::isfinite(want.y) && std::isfinite(want.z) && std::isfinite(want.w)))
        want = head.orientation;
    g_aimNow = want;
    g_aimOwn = own;
}

}
// The bare arms' forearms (The Great Destroyer) solved from their hands, both sides, as PlaceForearms
// solves the gloved left (armik.h; the bare arms share the gloves' rig and bind pose, each palm drawn
// exactly at its pose, leftPose / rightPose). The user, 2026-10-31: "track my forearms ... but when
// I'm bending my wrist, I need the forearms to stay locked". The shoulder from the pouch's neck point
// and body heading, the right's mirrored. Nothing with weapon.txt "barewrist 0" or without a body
// frame. A gap in the solves (a hitch) or a tracking jump starts that arm afresh, solved all the same,
// its sleeve's memory kept (barehold::Afresh: a stall never puts a sleeve on or takes it off).
// With a gun in hand (onRig: skin.cpp draws the bare hands as gloves, on the gun as the game holds it or by their own fitted poses) each is solved from the hand as
// it's drawn there, the very hand PlaceForearms solved the gloves' arm from (g_armHandAs); a side with none (folded away, the game's own) has no solved forearm.
// The elbow end is kept by the arm's own elbow (weapon.txt "bareelbowreach": a wrist bent past its range turned the forearm by the hand alone, its open elbow end
// far off the arm and to the eye: the left forearm a thin torn flap, the train, 2026-11-18; within that of the triangle's elbow, out from the shoulder or in towards
// it), and capped only with the hand beyond the arm's reach ("barecap", HandTargets::bareCap: never for a hand in reach, The Great Destroyer's arms as they were).
void PlaceBareForearms(skin::HandTargets& t, const float* fromHead, const float* toView, bool onRig) {
    static armik::State s_state[2];
    // Each arm's sleeve memory (barehold::SleeveStays: back on only 2 cm inside the limit) and its last wrist, kept across a hitch (barehold::Afresh).
    static barehold::ArmMemory s_arm[2];
    static ULONGLONG s_solvedAt = 0;
    static bool s_onRig = false;
    float neckRoom[3], yaw = 0.0f;
    if (!skin::BareWrist() || !pouch::BodyFrame(neckRoom, yaw)) {
        s_state[0] = s_state[1] = armik::State{};
        barehold::Forget(s_arm[0]), barehold::Forget(s_arm[1]);
        return;
    }
    if (onRig != s_onRig) {  // (another hand's from here on: afresh)
        s_state[0] = s_state[1] = armik::State{};
        barehold::Forget(s_arm[0]), barehold::Forget(s_arm[1]);
    }
    s_onRig = onRig;
    const ULONGLONG now = GetTickCount64();
    const bool gap = barehold::Gap(now, s_solvedAt);
    const float rightRoom[3] = {std::cos(yaw), 0.0f, -std::sin(yaw)}, forwardRoom[3] = {-std::sin(yaw), 0.0f, -std::cos(yaw)};
    float neck[3], left[3], up[3], back[3];
    Transform(fromHead, neckRoom, neck);
    for (int i = 0; i < 3; ++i) {
        const float* r = toView + i * 3;
        left[i] = -(r[0] * rightRoom[0] + r[1] * rightRoom[1] + r[2] * rightRoom[2]);
        up[i] = r[1];
        back[i] = -(r[0] * forwardRoom[0] + r[1] * forwardRoom[1] + r[2] * forwardRoom[2]);
    }
    // The chest the collar bones go by (the sleeves): square to the body's heading; with a gun in hand the gloves' arms' own, turned as the two hands hold it.
    float chestLeft[3], chestBack[3], shoulders[2][3];
    std::memcpy(chestLeft, onRig ? g_armChestAs[0] : left, sizeof(chestLeft));
    std::memcpy(chestBack, onRig ? g_armChestAs[1] : back, sizeof(chestBack));
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < 3; ++i)
            shoulders[side][i] = neck[i] + (side == 0 ? chestLeft[i] : -chestLeft[i]) * g_shoulder[0] - up[i] * g_shoulder[1] + chestBack[i] * g_shoulder[2];
    for (int side = 0; side < 2; ++side) {
        const bool tracked = onRig ? g_armHandHave[side] : side == 0 ? t.left : t.right;
        if (!tracked) {
            s_state[side] = armik::State{};
            barehold::Forget(s_arm[side]);
            continue;
        }
        // The hand turned back about the forearm's line by weapon.txt "baretwist" before it's solved
        // (only for the solve; the hand is drawn where it is): the forearms rolled 70-90 degrees with the
        // hands relaxed, a candy-wrapper wrist (the user, 15:59), so the solver's no-roll may be off by a
        // fixed amount per arm.
        float turned[12];
        std::memcpy(turned, onRig ? g_armHandAs[side] : side == 0 ? t.leftPose : t.rightPose, sizeof(turned));
        if (const float off = skin::BareTwist(side); off != 0.0f) {
            const float sx = side == 0 ? 1.0f : -1.0f;
            const float e[3] = {armik::kElbow[0] * sx, armik::kElbow[1], armik::kElbow[2]};
            const float w[3] = {armik::kWrist[0] * sx, armik::kWrist[1], armik::kWrist[2]};
            float ew[3], ww[3];
            Transform(turned, e, ew);
            Transform(turned, w, ww);
            float axis[3] = {ww[0] - ew[0], ww[1] - ew[1], ww[2] - ew[2]};
            armik::detail::Normalize(axis);
            armik::detail::TurnPlacement(turned, axis, -off * armik::kDeg, ww);
        }
        const float* hand = turned;
        float out[3], shoulder[3], wrist[3];
        for (int i = 0; i < 3; ++i) {
            out[i] = side == 0 ? left[i] : -left[i];
            shoulder[i] = neck[i] + out[i] * g_shoulder[0] - up[i] * g_shoulder[1] + back[i] * g_shoulder[2];
        }
        if (onRig)  // (the gloves' arm's own shoulder, the chest turned by the two hands on the gun)
            std::memcpy(shoulder, g_armShoulderAs[side], sizeof(shoulder));
        const float kneeWrist[3] = {side == 0 ? armik::kWrist[0] : -armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]};
        Transform(hand, kneeWrist, wrist);
        // A hitch (the gap) or a tracking jump: this frame's solve afresh, solved all the same (skipping it drew the forearm unsolved and no sleeve for
        // the frame), the sleeve's memory as it was.
        if (barehold::Afresh(s_arm[side], gap, wrist, kArmWristJump))
            s_state[side] = armik::State{};
        // The wrist's range wider than the gloved arm's: with the controller strapped to the palm the
        // user bends further, and past the range the solver turns the forearm to make up the rest (the
        // user: "when I'm bending my wrist, my whole forearm is moving", 15:27).
        armik::Params params = g_armParams[side];
        params.flex = 100.0f, params.ext = 95.0f, params.radial = 45.0f, params.ulnar = 60.0f;
        // The forearm takes nearly all of the hand's roll, its wrist end 95% and its middle half (the
        // gloves' 0, 1/3, 2/3 left a third to the wrist joint itself: the right wrist, rolled some 90
        // degrees, pinched thin between the cuff and the hand, the user's picture 15:52). weapon.txt
        // "bareshare 0.5 0.8 1" to try the arm study's (less thinning along the forearm at 90-120 degrees).
        skin::BareShares(params.share);
        params.elbowReach = skin::BareElbowReach();
        armik::Result r;
        armik::Solve(hand, shoulder, out, up, back, side == 1, params, s_state[side], r);
        if (!Finite(&r.frame[0][0], 36) || !Finite(r.elbow, 3)) {  // (never a forearm of numbers not finite: whole with its hand for the frame, afresh after)
            s_state[side] = armik::State{};
            continue;
        }
        std::memcpy(t.bareForearm[side], r.frame, sizeof(r.frame));
        t.bareArm[side] = true;
        const int capMode = skin::BareCap();
        t.bareCap[side] = capMode == 0 ? 0.0f : capMode == 2 ? 1.0f : armik::EndCap(r, shoulder);
        // The rest of the arm, for its sleeve (the torso's draw, the strip over the shoulders: skin.cpp DrawBareSleeves), as PlaceForearms has the gloved arms': the
        // upper arm, and the collar bone on the chest's axes with its shoulder joint where the upper arm's far end is, so nothing between them stretches (a hand
        // beyond the arm's reach brings the shoulder along). With it the arm is whole and its elbow end is inside the sleeve: no cap.
        // The sleeve never leaves the body: only while the shoulder is brought along no further than its cloth takes (barehold::SleeveStays, 12 cm; the collar
        // whole, never drawn in as the gloved levels' is). Further (the gun held 20 cm and more out ahead, an arm raised high: film, 2026-11-18, the sleeves off the
        // body, the shoulder's cloth a pointed tail between a collar drawn in to nothing and the chest's point) that arm's sleeve is folded away and the arm is as
        // it was without one, the forearm alone with its open end capped, until the hand is back in the sleeve's reach.
        const float movedBy[3] = {r.shoulder[0] - shoulder[0], r.shoulder[1] - shoulder[1], r.shoulder[2] - shoulder[2]};
        const float broughtAlong = Length(movedBy);
        std::memcpy(t.bareForearm[side][barehold::kFrameUpper], r.upper, sizeof(r.upper));
        armik::Collar(chestLeft, up, chestBack, r.shoulder, side == 1, 1.0f, t.bareForearm[side][barehold::kFrameCollar]);
        const bool sleeveStays = barehold::SleeveAfter(s_arm[side], broughtAlong);
        t.bareSleeve[side] = skin::BareSleeves() && sleeveStays && Finite(t.bareForearm[side][barehold::kFrameUpper], 24) && Finite(r.shoulder, 3);
        if (t.bareSleeve[side])
            std::memcpy(shoulders[side], r.shoulder, sizeof(shoulders[side]));
        // Once a second each side: the wrist's bend, whether the range was met, and how far its point
        // moved in the second (it shouldn't, bending only the wrist: then the elbow guess moves too).
        static ULONGLONG s_logged[2] = {};
        static float s_from[2][3] = {};
        if (now - s_logged[side] >= 1000) {
            s_logged[side] = now;
            const float drift[3] = {wrist[0] - s_from[side][0], wrist[1] - s_from[side][1], wrist[2] - s_from[side][2]};
            std::memcpy(s_from[side], wrist, sizeof(wrist));
            const float off[3] = {r.elbow[0] - shoulder[0], r.elbow[1] - shoulder[1], r.elbow[2] - shoulder[2]};
            Log("Bare arms: %s wrist bent %+.0f (flexion +) and %+.0f (ulnar +) degrees, twist %+.0f%s; the wrist "
                "point moved %.0f cm in the second; the elbow end %.2f m from the shoulder (the upper arm %.2f), its cap %.2f%s; its sleeve %s, the shoulder %s %.2f m%s",
                side == 0 ? "left" : "right", r.flexion / armik::kDeg, r.deviation / armik::kDeg, r.twist / armik::kDeg,
                r.reachHeld ? " [range met: the elbow kept by the arm's own, the wrist bent on]" : r.clamped ? " [range met: the forearm turned]" : "", Length(drift) * 100.0f,
                Length(off), armik::kUpper, t.bareCap[side], onRig ? "; from the hand as drawn on the gun" : "", t.bareSleeve[side] ? "on" : "off",
                sleeveStays ? "brought along" : "not brought along: the hand", broughtAlong, sleeveStays ? "" : " beyond the arm's reach from the body (the cloth takes 0.12)");
        }
    }
    // The chest folds onto the point between the two shoulders (an arm without a sleeve: its shoulder as estimated).
    if (t.bareSleeve[0] || t.bareSleeve[1]) {
        for (int i = 0; i < 3; ++i)
            t.bareChest[i] = (shoulders[0][i] + shoulders[1][i]) / 2;
        t.haveBareChest = Finite(t.bareChest, 3);
        if (!t.haveBareChest)
            t.bareSleeve[0] = t.bareSleeve[1] = false;
    }
}

namespace hands {

void PlayGunsOwn(const char* kind, const char* otherwise) {
    char name[48];
    const char* shape = skin::HeldShape();
    std::snprintf(name, sizeof(name), "%s_%s", kind, shape ? shape : "");
    sounds::Play(sounds::Have(name) ? name : otherwise);
}

// The left glove's place on its controller's grip pose, made from grip.txt's turn and palm while
// it has no "leftpose".
void MakeLeftOffset() {
    if (g_leftOffsetSet)
        return;
    // The glove's bind axes onto the grip pose's: x (outward) -> +Y, y (back of the hand) -> +X,
    // z (fingers) -> -Z, then the tuning turn; the palm at the grip.
    const float glove[9] = {0, 1, 0, 1, 0, 0, 0, 0, -1};
    const float origin[3] = {};
    float tuned[9], local[9];
    Euler(g_leftTurn, tuned);
    Mul3(tuned, glove, local);
    PlaceAt(local, g_leftAnchor, origin, g_leftOffset);
}

void Update(const XrPosef& head) {
    g_badStage = nullptr;
    g_badBone = -2;
    g_kicked = 0;  // (nothing kicked yet this frame: AsHeld gives every pose as it is)
    ++g_updateSerial;
    ReadTuning();
    ReadPouchTuning();
    ReadHolsterTuning();
    ReadArmsTuning();
    ReadPistolTuning();
    ammo::SetPistolByHand(g_on && g_pistolRoundsOn);  // (its rounds by hand: ammo.cpp finds the pistol once it's in the hand)
    ammo::SetTubeByHand(g_on && skin::TubeByHand() && skin::LauncherHands());  // (a tube shotgun's shells by hand: found once it's drawn in the hands)
    freetune::Read();  // build\freehands.txt (free hands on the rifle: "off", its tests)
    recoilrun::Read();  // build\recoil.txt (recoil for every gun: "off", each layer's strength, its tests)
    if (!g_on) {
        skin::SetHands(nullptr);
        g_launcherEmptyNow = false;
        g_tubeInHand = false, g_tubeBoltHeldNow = false;
        g_at4Tube = holster::TubeLatch{}, g_at4SpentNow = false;
        ammo::SetLauncherByHand(false);
        return;
    }
    // In a gun's seat (mountseat.h, mountgun::Quiet; never today) the game's own gunner arms are drawn as it draws them, and nothing of the free hands runs:
    // no held gun, no holster (holster.h's frame isn't run: no draw, no put-away, no keys 1, 2 or 4), no grenade at the chest, no magazine or pouch. What
    // the hands had waits as it was, as with free hands off, and goes on after the seat as after a cutscene.
    if (mountseat::Quiet(mountgun::kFreeHands)) {
        skin::SetHands(nullptr);
        g_launcherEmptyNow = false;
        g_tubeInHand = false, g_tubeBoltHeldNow = false;
        ammo::SetLauncherByHand(false);
        return;
    }
    // Seconds since the last frame here (the steer's easing; at most 0.1).
    static LARGE_INTEGER s_frequency{}, s_lastFrame{};
    LARGE_INTEGER frameNow;
    QueryPerformanceCounter(&frameNow);
    if (!s_frequency.QuadPart)
        QueryPerformanceFrequency(&s_frequency);
    const float frameSeconds =
        s_lastFrame.QuadPart ? std::fmin(static_cast<float>(frameNow.QuadPart - s_lastFrame.QuadPart) /
                                             static_cast<float>(s_frequency.QuadPart),
                                         0.1f)
                             : 0.0f;
    // (The same never capped, for recoil: a long gap, a load or the headset off, clears its kick.)
    const float frameGap =
        s_lastFrame.QuadPart ? static_cast<float>(frameNow.QuadPart - s_lastFrame.QuadPart) / static_cast<float>(s_frequency.QuadPart) : 0.0f;
    s_lastFrame = frameNow;
    // Everything is worked out in the recentered space, then given to skin.cpp in the view's own
    // space (relative to the head), which places it with each frame's eye. The gun flickered
    // while walking when this placed it in the arms' space itself: that space's origin jumps a
    // metre at a time as the player moves, and the targets were a frame late.
    float headTurn[9], toView[9];
    QuatToMatrix(head.orientation, headTurn);
    Transpose3(headTurn, toView);
    auto pose = [](const XrPosef& p, float* out) {
        float m[9];
        QuatToMatrix(p.orientation, m);
        const float at[3] = {p.position.x, p.position.y, p.position.z};
        for (int i = 0; i < 3; ++i)
            out[i * 4] = m[i * 3], out[i * 4 + 1] = m[i * 3 + 1], out[i * 4 + 2] = m[i * 3 + 2], out[i * 4 + 3] = at[i];
    };
    float headPose[12], fromHead[12];  // the recentered space -> the view's
    pose(head, headPose);
    InvertRigid(headPose, fromHead);
    auto toViewPoint = [&](const XrVector3f& p, float* out) {
        const float at[3] = {p.x, p.y, p.z};
        Transform(fromHead, at, out);
    };

    skin::HandTargets t;
    handpose::Fingers fingers;  // the left glove's, as the controller says
    const bool bareHands = skin::BareHold() && skin::BareArmsLately();  // a level of bare arms (no gloves): the hands' fingers drawn as skin, all of their width
    bool pistolHeld = false;    // The Great Destroyer's train pistol in the right hand (its hold learned)
    float pistolAnchor[3] = {-0.0001f, -0.0354f, 0.0299f};  // ... where the right hand holds it (its frame's space: the right-hand block's grip), recoil's pivot
    XrPosef aim, grip, rightGrip{};
    float gunTurn[9], anchor[3];  // the gun as the right hand alone holds it (view space), its grip
    float rifleTurn[9] = {};      // the M416 alike, raised so its sights look along the aim
    // The launcher: its rear grip, where the game's own right palm sits on its frame bone (docs\research\smaw.md); its turn as the
    // right hand alone holds it.
    // Any other gun in hand goes the same way (skin.cpp DrawHeldPart), by where the game's palms rest on it.
    float launcherGrip[3] = {-0.026f, -0.141f, 0.061f};
    float launcherFront[3] = {kLauncherFrontGrip[0], kLauncherFrontGrip[1], kLauncherFrontGrip[2]};
    skin::HeldNowOut held;
    skin::HeldNow(held);  // (the held gun's kind, palms, up and shape in one read)
    const bool heldGun = held.held;
    if (heldGun) {  // (as HeldPalms wrote them, whatever its return)
        std::memcpy(launcherGrip, held.rightPalm, sizeof(launcherGrip));
        std::memcpy(launcherFront, held.leftPalm, sizeof(launcherFront));
    }
    float launcherTurn[9] = {};
    // Where the game's own right hand holds the gun, unless grip.txt says otherwise. (Before the right hand: free hands place the rifle in the left hand by it too,
    // with the right controller asleep.)
    if (g_gunGripTuned || !skin::GunGrip(anchor)) {
        std::memcpy(anchor, g_gunGrip, sizeof(anchor));
    } else {
        static bool logged = false;
        if (!logged) {
            logged = true;
            Log("Hands: the game holds the gun at (%.3f %.3f %.3f) in its model space", anchor[0], anchor[1],
                anchor[2]);
        }
    }
    // The profile of whatever's in hand (gunprofile.h): its pose, its places, its up, its body (with its attachments' points), its words.
    gunprofile::In in;
    in.rifle = skin::RifleInHand() && !skin::LauncherInHand() && !heldGun;
    in.launcher = skin::LauncherInHand() && skin::LauncherHands();
    in.held = heldGun && skin::LauncherHands() && held.palms;
    in.heldId = held.id;
    in.heldShape = held.shape;
    std::memcpy(in.rifleAnchor, anchor, sizeof(in.rifleAnchor));
    in.rifleUp = skin::RifleUp();
    std::memcpy(in.launcherGrip, launcherGrip, sizeof(in.launcherGrip));
    std::memcpy(in.launcherFront, launcherFront, sizeof(in.launcherFront));
    in.launcherUp = in.held ? held.up : skin::LauncherUp();
    in.launcherRight = in.held ? held.right : 0.0f;
    if (in.rifle) {
        // The M416: gunbody.h's (with its grenade launcher, as this level gives it; without, once it's not been met).
        in.body = skin::RifleM320Seen() ? &gunbody::kM416 : &gunbodies::kM416Bare;
        std::memcpy(in.bodyMin, gunbody::kM416HullMin, sizeof(in.bodyMin));
        std::memcpy(in.bodyMax, gunbody::kM416HullMax, sizeof(in.bodyMax));
        in.bodySound = gunbodies::kMetal;
    } else if (const gunbodies::Body* b = in.launcher ? gunbodies::For(g_smawCaseOnNow ? "smaw" : "smaw_bare") : in.held ? gunbodies::For(held.shape) : nullptr;
               b && b->spec) {
        in.body = b->spec;
        std::memcpy(in.bodyMin, b->hullMin, sizeof(in.bodyMin));
        std::memcpy(in.bodyMax, b->hullMax, sizeof(in.bodyMax));
        in.bodySound = b->sound;
        // A held gun's attachments drawn with it (its sight, a handle): their points in its hull, before the fit's "the rest", and its box grown by them.
        // Remade only when they or the body change.
        float points[32][3];
        uint32_t set = 0;
        const int n = in.held ? skin::AttachmentsOnGun(points, static_cast<int>(std::size(points)), set) : 0;
        if (n > 0 && n <= static_cast<int>(std::size(points)) && gunprofile::FiniteAll(&points[0][0], n * 3)) {
            if (g_attachedFrom != b->spec || g_attachedSet != set) {
                g_attachedBody = *b->spec;
                const int restFrom = std::min(std::max(b->restFrom, 0), b->spec->hullPoints);
                const int count = b->spec->hull && b->spec->hullPoints > 0
                                      ? bodyfit::WithExtra(b->spec->hull, b->spec->hullPoints, restFrom, points, n, g_attachedHull)
                                      : 0;
                g_attachedPoints = 0;
                for (int i = 0; i < count; ++i)  // (those copied from an attachment's points: the fit's own never equal one, a near one is merged)
                    for (int j = 0; j < n; ++j)
                        if (std::memcmp(g_attachedHull[i], points[j], sizeof(points[j])) == 0) {
                            ++g_attachedPoints;
                            break;
                        }
                g_attachedBody.hull = count > 0 ? g_attachedHull : b->spec->hull;
                g_attachedBody.hullPoints = count > 0 ? count : b->spec->hullPoints;
                std::memcpy(g_attachedMin, b->hullMin, sizeof(g_attachedMin));
                std::memcpy(g_attachedMax, b->hullMax, sizeof(g_attachedMax));
                for (int j = 0; j < n; ++j)
                    for (int i = 0; i < 3; ++i)
                        g_attachedMin[i] = std::fmin(g_attachedMin[i], points[j][i]), g_attachedMax[i] = std::fmax(g_attachedMax[i], points[j][i]);
                g_attachedFrom = b->spec;
                g_attachedSet = set;
            }
            in.body = &g_attachedBody;
            std::memcpy(in.bodyMin, g_attachedMin, sizeof(in.bodyMin));
            std::memcpy(in.bodyMax, g_attachedMax, sizeof(in.bodyMax));
            in.bodyAttachments = g_attachedPoints;
        }
    }
    in.everyGun = freetune::EveryGun();
    g_profile = gunprofile::Make(in);
    // (Each pose's grip and up this frame, for the left's place for the pose that isn't the profile's: FreeHands.)
    std::memcpy(g_poseGrip[0], anchor, sizeof(g_poseGrip[0]));
    std::memcpy(g_poseGrip[1], launcherGrip, sizeof(g_poseGrip[1]));
    g_poseUp[0] = in.rifleUp;
    g_poseUp[1] = in.launcherUp;
    g_poseRight[1] = in.launcherRight;
    // (The left glove easing off the M416's pistol grip goes by this frame's grip and up, as before, whatever's in hand now: the game's grip is a running mean.)
    if (g_leftGripProfile.pose == gunprofile::Pose::kRifle) {
        std::memcpy(g_leftGripProfile.anchor[1], anchor, sizeof(g_leftGripProfile.anchor[1]));
        gunprofile::MirrorPoint(g_leftGripProfile.anchor[1], g_leftGripProfile.anchor[0]);
        g_leftGripProfile.up = in.rifleUp;
    }
    // Why it's none, for the asleep line.
    const int kinds = (in.rifle ? 1 : 0) + (in.launcher ? 1 : 0) + (in.held ? 1 : 0);
    if (g_profile.pose == gunprofile::Pose::kNone) {
        const char* gun = in.launcher || (skin::LauncherInHand() && !heldGun) ? "launcher" : gunprofile::NameOf(held.shape);
        if (kinds > 1)
            strcpy_s(g_profileWhy, " two guns drawn at once;");
        else if (kinds == 0 && (skin::LauncherInHand() || heldGun) && !skin::LauncherHands())
            sprintf_s(g_profileWhy, " the %s as the game holds it (launcherhands 0);", gun);
        else if (kinds == 0 && heldGun)
            sprintf_s(g_profileWhy, " where the game's palms rest on the %s not known yet;", gun);
        else if (kinds == 0)
            strcpy_s(g_profileWhy, " no gun drawn lately;");
        else if (!in.body)
            sprintf_s(g_profileWhy, " no body for the %s yet;", gun);
        else if (!in.rifle && !in.everyGun)
            sprintf_s(g_profileWhy, " the %s in the right hand as before (guns m416);", gun);
        else if (in.held && !in.heldId)
            strcpy_s(g_profileWhy, " a gun of no known kind in hand;");
        else
            sprintf_s(g_profileWhy, " the %s's numbers not finite;", in.rifle ? "rifle" : gun);
    } else {
        g_profileWhy[0] = 0;
    }
    // A switch: frames with no gun drawn, or two at once, right after a gun free hands know, for at most 0.5 s: the last one kept (nothing changes hands).
    // Only straight after it (the frame before had it): not a gun from before a stretch free hands didn't have ("guns m416", no body yet), minutes old.
    {
        const ULONGLONG nowMs = GetTickCount64();
        g_bridging = false;
        if (g_profile.pose == gunprofile::Pose::kNone && kinds != 1 && g_profileLast.pose != gunprofile::Pose::kNone) {
            if (!g_noneSince) {
                g_noneSince = nowMs;
                g_bridgeOk = g_profileWas == g_profileLast.id;
            }
            if (g_bridgeOk && nowMs - g_noneSince <= 500)
                g_profile = g_profileLast, g_bridging = true;
        } else {
            g_noneSince = 0;
        }
        if (g_profile.pose != gunprofile::Pose::kNone && !g_bridging)
            g_profileLast = g_profile;
    }
    // build\freehands.txt "test N", nobody in the headset: this frame's test hands.
    RunFreeTest(headPose, fromHead, headTurn, g_profile.pose == gunprofile::Pose::kNone ? anchor : g_profile.anchor[1], frameSeconds);
    // (A test's right hand away from the stand-in's hold is aimed as its controller would be, from its grip.)
    g_rightSeen = false;
    g_haveHandGun[1] = false;
    const bool rightByTest = g_test[1].on && g_test[1].step.where != freetest::Where::kStandIn;
    if (rightByTest ? HandPoseOf(1, grip) && input::AimFromGrip(1, grip, aim) : input::AimPose(aim) && HandPoseOf(1, grip)) {
        rightGrip = grip;
        // The gun model's axes are x left, y up, z forward (its muzzle; the first try took z for
        // back and the gun pointed at the player), the aim pose's x right, y up, z back: half a
        // turn about y between them.
        const float flip[9] = {-1, 0, 0, 0, 1, 0, 0, 0, -1};
        float aimTurn[9], turn[9];
        QuatToMatrix(aim.orientation, aimTurn);
        Mul3(toView, aimTurn, turn);
        Mul3(turn, flip, gunTurn);
        toViewPoint(grip.position, t.rightWrist);
        {
            const float up = skin::RifleUp() * 3.14159265f / 180.0f;
            const float raise[9] = {1, 0, 0, 0, std::cos(up), std::sin(up), 0, -std::sin(up), std::cos(up)};
            Mul3(gunTurn, raise, rifleTurn);
        }
        PlaceAt(rifleTurn, anchor, t.rightWrist, t.gunPose);
        t.gun = true;
        // Free hands: the right hand's own place for the rifle (where it holds it, as above), kept by side.
        std::memcpy(g_handGun[1], t.gunPose, sizeof(g_handGun[1]));
        std::memcpy(g_rifleTurnBy[1], rifleTurn, sizeof(g_rifleTurnBy[1]));
        std::memcpy(g_anchorBy[1], anchor, sizeof(g_anchorBy[1]));
        std::memcpy(g_atBy[1], t.rightWrist, sizeof(g_atBy[1]));
        g_haveHandGun[1] = true;
        g_rightSeen = true;
        // The right hand placed as the left glove is, mirrored across x (the gloves' right hand is
        // the left's mirror image, and so is its controller): The Great Destroyer's bare right arm.
        MakeLeftOffset();
        float controller[12], mirrored[12], hand[12];
        pose(grip, controller);
        std::memcpy(mirrored, g_leftOffset, sizeof(mirrored));
        mirrored[1] = -mirrored[1], mirrored[2] = -mirrored[2], mirrored[3] = -mirrored[3];
        mirrored[4] = -mirrored[4], mirrored[8] = -mirrored[8];
        Compose(controller, mirrored, hand);
        Compose(fromHead, hand, t.rightPose);
        // Free hands: the free right glove is placed so (the left's line-up mirrored), its palm the left's palm point mirrored.
        std::memcpy(g_rightFree, t.rightPose, sizeof(g_rightFree));
        {
            const float rightAnchor[3] = {-g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2]};
            Transform(g_rightFree, rightAnchor, g_rightPalm);
        }
        // The train pistol of The Great Destroyer, aimed as the M416 is (the game fires along the aim pose),
        // its grip (the seated magazine's middle in its frame bone's space; the magazine's middle where its
        // vertices lie apart, 13.6 cm off below the gun, was taken for it at first) at the hand's grip point:
        // skin.cpp draws it there. Holding it, the right palm sits on it as the game's does (the user: "I'm not
        // holding the gun", 15:59), and the pistol goes where the empty hand's grip point would be at the
        // controller's, so the hand holding it stays where the real hand is.
        float pistolGrip[3] = {-0.0001f, -0.0354f, 0.0299f};
        float hold[12];
        pistolHeld = skin::BarePistolHand(hold);  // the arms' bind space -> the pistol frame's (hand root 15)
        if (pistolHeld) {
            float fromGrip[12];
            InvertRigid(mirrored, fromGrip);  // the controller's grip space -> the arms' bind space
            const float origin[3] = {fromGrip[3], fromGrip[7], fromGrip[11]};
            Transform(hold, origin, pistolGrip);
        }
        PlaceAt(gunTurn, pistolGrip, t.rightWrist, t.pistolPose);
        std::memcpy(pistolAnchor, pistolGrip, sizeof(pistolAnchor));
        float raise[9];  // (a held gun's up and right from the same read as its profile)
        SightsTurn(heldGun ? held.up : skin::LauncherUp(), heldGun ? held.right : 0.0f, raise);
        Mul3(gunTurn, raise, launcherTurn);
        PlaceAt(launcherTurn, launcherGrip, t.rightWrist, t.launcherPose);
        if (g_profile.pose == gunprofile::Pose::kLauncher) {  // (free hands: the right hand's own place for a launcher-pose gun)
            std::memcpy(g_handGun[1], t.launcherPose, sizeof(g_handGun[1]));
            std::memcpy(g_rifleTurnBy[1], launcherTurn, sizeof(g_rifleTurnBy[1]));
            std::memcpy(g_anchorBy[1], launcherGrip, sizeof(g_anchorBy[1]));
        }
        if (pistolHeld)
            Compose(t.pistolPose, hold, t.rightPose);
        t.right = true;
        // The right controller itself moving more than 4 cm in a frame in the room (tracking), and the
        // head (the user: the bare hands jump swaying left and right, 15:06).
        static float s_last[2][3] = {};
        static bool s_have = false;
        const float now[2][3] = {{grip.position.x, grip.position.y, grip.position.z},
                                 {head.position.x, head.position.y, head.position.z}};
        if (s_have) {
            for (int which = 0; which < 2; ++which) {
                const float d[3] = {now[which][0] - s_last[which][0], now[which][1] - s_last[which][1],
                                    now[which][2] - s_last[which][2]};
                const float moved = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                static ULONGLONG s_logged[2] = {};
                if (moved > 0.04f && GetTickCount64() - s_logged[which] > 200) {
                    s_logged[which] = GetTickCount64();
                    Log("Hands: the %s moved %.0f cm in a frame (tracking)", which == 0 ? "right controller" : "head",
                        moved * 100.0f);
                }
            }
        }
        std::memcpy(s_last, now, sizeof(s_last));
        s_have = true;
    }
    if (input::HandPose(0, grip)) {
        MakeLeftOffset();
        // In the recentered space: the glove held still there during the line-up stays put in the
        // room while the head moves.
        float controller[12], glove[12];
        pose(grip, controller);
        Compose(controller, g_leftOffset, glove);
        if (input::TakeCalibratePress()) {
            if (!g_lineUpOn) {
                Log("Hands: the left View button: nothing (it lined the glove up; pouch.txt \"lineup 1\" turns that back on)");
            } else if (!g_lining) {
                std::memcpy(g_heldLeft, glove, sizeof(g_heldLeft));
                g_lining = true;
                Log("Hands: the left glove is held still; put the left hand into it and press View again");
                Beep(880, 150);
            } else {
                float inverse[12];
                InvertRigid(controller, inverse);
                Compose(inverse, g_heldLeft, g_leftOffset);
                g_leftOffsetSet = true;
                g_lining = false;
                SaveLeftOffset();
                Log("Hands: the left glove is lined up with the hand (saved to grip.txt)");
                Beep(1320, 200);
            }
        }
        Compose(fromHead, g_lining ? g_heldLeft : glove, t.leftPose);
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);  // the palm, where the arm folds into
        // The fingers follow the controller: the trigger the index finger, the grip the other three,
        // the thumb on the stick folds the thumb in; all of them, a fist. Relaxed while lining up.
        const float* fixed = g_fingersFixed ? g_fixedFingers : nullptr;
        float index = fixed ? fixed[0] : input::LeftTrigger(), grip = fixed ? fixed[1] : input::LeftSqueeze(),
              thumb = fixed ? fixed[2] : input::LeftThumbOnStick() ? 1.0f : 0.0f;
        g_index += ((g_lining ? 0.0f : index) - g_index) * 0.35f;
        g_grip += ((g_lining ? 0.0f : grip) - g_grip) * 0.35f;
        g_thumb += ((g_lining ? 0.0f : thumb) - g_thumb) * 0.35f;
        fingers.curl[0] = g_index;
        fingers.curl[1] = fingers.curl[2] = fingers.curl[3] = g_grip;
        // Folded part way on its own; with the grip held, down onto the curled middle finger (also
        // when the index finger points: hovering over it looked wrong, user 2026-10-30).
        fingers.thumb = g_thumb * (0.6f + 0.4f * g_grip);
        fingers.spread = g_spread;
        t.left = true;
        g_scriptOn = false;
    } else {
        // weapon.txt "m320test 7": the scripted left hand (ScriptedLeft), turned as the stand-in's.
        g_scriptOn = false;
        XrPosef standIn, testGrip;
        if (g_test[0].on && HandPoseOf(0, testGrip)) {
            // build\freehands.txt "test N": the test's left hand, a glove on its grip as the real one's, its fingers by the test's grip, trigger and thumb.
            MakeLeftOffset();
            float controller[12], glove[12];
            pose(testGrip, controller);
            Compose(controller, g_leftOffset, glove);
            Compose(fromHead, glove, t.leftPose);
            Transform(t.leftPose, g_leftAnchor, t.leftWrist);
            g_index += (TriggerOf(0) - g_index) * 0.35f;
            g_grip += (SqueezeOf(0) - g_grip) * 0.35f;
            g_thumb += ((ThumbOf(0) ? 1.0f : 0.0f) - g_thumb) * 0.35f;
            fingers.curl[0] = g_index;
            fingers.curl[1] = fingers.curl[2] = fingers.curl[3] = g_grip;
            fingers.thumb = g_thumb * (0.6f + 0.4f * g_grip);
            fingers.spread = g_spread;
            t.left = true;
        } else if (skin::M320Test() == 7 && input::StandInLeftPose(standIn)) {
            MakeLeftOffset();
            float controller[12], glove[12], base[12];
            pose(standIn, controller);
            Compose(controller, g_leftOffset, glove);
            Compose(fromHead, glove, base);
            static float s_frame = 0.0f;
            static ULONGLONG s_at = 0;
            const ULONGLONG now = GetTickCount64();
            s_frame = s_at ? std::fmin((now - s_at) / 1000.0f, 0.05f) : 0.0139f;
            s_at = now;
            if (ScriptedLeft(t, headPose, fromHead, s_frame, base, t.leftPose)) {
                Transform(t.leftPose, g_leftAnchor, t.leftWrist);
                g_index += (0.0f - g_index) * 0.35f;
                g_grip += (g_scriptSqueeze - g_grip) * 0.35f;
                g_thumb += (g_scriptSqueeze - g_thumb) * 0.35f;
                fingers.curl[0] = g_index;
                fingers.curl[1] = fingers.curl[2] = fingers.curl[3] = g_grip;
                fingers.thumb = g_thumb * (0.6f + 0.4f * g_grip);
                fingers.spread = g_spread;
                t.left = true;
                g_scriptOn = true;
            }
        }
    }
    // Free hands on the gun in hand: who has it this frame, and where it is for everything below (as before while the right holds its pistol grip).
    FreeHands(t, fromHead, toView, frameSeconds);
    // The gun in hand as free hands placed it: the M416 on gunPose, any other on launcherPose (the profile's pose and its drawn flag).
    const bool lpNow = g_profile.pose == gunprofile::Pose::kLauncher;
    float* const gunNow = lpNow ? t.launcherPose : t.gunPose;
    // Once a second: frames where a controller wasn't tracked (the game's own gun or hand is drawn
    // for those).
    static int frames = 0, noGun = 0, noLeft = 0;
    static ULONGLONG lastLog = 0;
    ++frames, noGun += !t.gun && !t.gunDrawn && !t.launcherDrawn && !t.stowed, noLeft += !t.left;  // (a gun put away isn't a frame without the right controller)
    if (GetTickCount64() - lastLog >= 1000) {
        lastLog = GetTickCount64();
        if (noGun || noLeft)
            Log("Hands: of the last %d frames, %d without the right controller's aim or grip, %d without the left", frames,
                noGun, noLeft);
        frames = noGun = noLeft = 0;
    }
    // The reloads go by where the left hand really is, wherever the glove is drawn.
    float palm[3];
    std::memcpy(palm, t.leftWrist, sizeof(palm));
    // The glove as the controller holds it, to blend from onto the handguard and back.
    float tracked[12];
    std::memcpy(tracked, t.leftPose, sizeof(tracked));
    const bool trackedLeft = t.left;
    // The grenade launcher under the rifle reloaded by hand (m320.h): the release, its barrel by hand, the case out by gravity or by the rim, a 40 mm from the belly, the
    // magnet home and the push shut. Before the pouch: a squeeze it takes isn't the pouch's, and the belly is its own while its barrel's the last thing opened.
    const m320::Result m320Out = M320ByHand(t, headPose, tracked, trackedLeft, pistolHeld, frameSeconds);
    if (float a[12], b[12], mix = 0.0f; m320::TakeBadRound(a, b, mix)) {
        static ULONGLONG s_at = 0;
        if (GetTickCount64() - s_at > 1000) {
            s_at = GetTickCount64();
            Log("M320 numbers: the round's blend to the carry not finite at mix %.9g; the pinch's %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g; the carry's %.9g %.9g "
                "%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g",
                mix, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11]);
        }
    }
    // The round in either hand as the module has it in that glove (each hand's own: the right's in the right glove's).
    if (m320Out.handRound && (m320Out.handSide == 0 || m320Out.handSide == 1)) {
        bool finite = true;
        for (float v : m320Out.roundInGlove)
            finite = finite && std::isfinite(v);
        if (finite) {
            std::memcpy(g_lastRoundInGloveBy[m320Out.handSide], m320Out.roundInGlove, sizeof(g_lastRoundInGloveBy[m320Out.handSide]));
            g_haveLastRoundInGloveBy[m320Out.handSide] = true;
        }
    }
    // Its hand tables (the game's own hand at each hold: every bone relative to the game's bone 0, which sits at the wrist) re-anchored on the palm, bone 1, the one the
    // arm is solved from (ArmHand): rebase = the palm as this glove draws it, times the game's palm's inverse. Taken as they came onto a glove left where the controller
    // has it (a round carried from the belly), the palm turned by the game's wrist bend and the arm solved from it went off from the body or out of sight (the user,
    // 2026-11-05: "my hand and arm just totally disappear or twists away"). The glove's place on a hold, the fingers and a round in the hand all go through the same.
    // The hand at work's (m320Side): free hands' right (side 1) on its own palm at its own width, all in the left's layout (RightGlove flips them onto the right glove).
    const int m320Side = m320Out.side == 1 ? 1 : 0;
    float m320Rebase[12], m320Table[handpose::kBones][12];
    bool haveM320Rebase = false;
    if (m320Out.fingers) {
        float palm[handpose::kBones][12], gamePalmInverse[12];
        if (m320Side == 0)
            handpose::Pose(handpose::Fingers{}, g_leftWidth, palm);  // (its palm as the glove draws it: no finger moves it)
        else
            handpose::PoseSafe(handpose::Fingers{}, RightGloveWidth(), palm);
        for (int b = 0; b < handpose::kBones; ++b) {
            if (m320Out.fingersAlt) {
                float pivot[3];
                handpose::Pivot(b, pivot);
                BlendAbout(m320Out.fingers[b], m320Out.fingersAlt[b], m320Out.fingersMix, pivot, m320Table[b]);
            } else {
                std::memcpy(m320Table[b], m320Out.fingers[b], sizeof(m320Table[b]));
            }
        }
        if (Invert(m320Table[1], gamePalmInverse)) {
            Compose(palm[1], gamePalmInverse, m320Rebase);
            for (int b = 0; b < handpose::kBones; ++b) {
                float rebased[12];
                Compose(m320Rebase, m320Table[b], rebased);
                std::memcpy(m320Table[b], rebased, sizeof(rebased));
            }
            haveM320Rebase = true;
        }
    }
    // The train pistol's magazine by hand (pistol.txt "mag 1"; PistolMagazine, after the pouch): a squeeze over the
    // shoulder with the pistol in hand, or lately (its "in hand" drops for a moment now and then), is for its magazine,
    // not the game's own reload as with other guns.
    static ULONGLONG s_pistolAt = 0;
    if (pistolHeld)
        s_pistolAt = GetTickCount64();
    const bool pistolMag = g_pistolMagOn && s_pistolAt && GetTickCount64() - s_pistolAt < 1000;
    // A magazine from over the left shoulder (pouch.cpp), worked out before the handguard: the
    // squeeze that takes one there doesn't also take the handguard.
    pouch::Result magazine;
    XrPosef standIn;
    const bool testing = g_pouchTest && !trackedLeft && input::StandInLeftPose(standIn);
    {
        static LARGE_INTEGER s_frequency{}, s_last{};
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (!s_frequency.QuadPart)
            QueryPerformanceFrequency(&s_frequency);
        const float dt = s_last.QuadPart ? std::fmin(static_cast<float>(now.QuadPart - s_last.QuadPart) /
                                                         static_cast<float>(s_frequency.QuadPart),
                                                     0.1f)
                                         : 0.0f;
        s_last = now;
        pouch::Frame f;
        f.dt = dt;
        f.head = head;
        // Whose hand it's fed (free hands, one magazine at a time): the hand with one (or drawing the seated one down), else the hand at work; the left while
        // dormant, as before, and for pouch.txt's test. Each side its own controller, glove and palm.
        const int heldSide = pouch::HoldingSide();
        const int side = testing ? 0 : heldSide >= 0 ? heldSide : g_workHand == 1 ? 1 : 0;
        f.side = side;
        XrPosef leftGrip{};
        if (side == 0) {
            f.left = trackedLeft && HandPoseOf(0, leftGrip);
            f.leftGrip = leftGrip.position;
            f.squeeze = std::fmax(SqueezeOf(0), TriggerOf(0));  // a magazine by either (the user, 2026-11-05)
        } else {
            f.left = g_rightWorks && HandPoseOf(1, leftGrip);
            f.leftGrip = leftGrip.position;
            f.squeeze = std::fmax(SqueezeOf(1), TriggerOf(1));
        }
        // The rifle the pouch knows, its own firing state (not the launcher under it), its
        // magazine's seat in the gun learned.
        const bool knownGun = skin::RifleInHand();
        f.gun = knownGun && ammo::RifleFiringInHand() && skin::MagazineSeatKnown();
        // Free hands, the frame the rifle's let go of (it starts to fall from where it was drawn last frame, StepLoose): that place, so a magazine in the hand is
        // drawn every frame till it's let go of in turn (dropHeld, below). Only with one in a hand: with none, a squeeze that frame takes nothing, as before.
        const bool releaseFrame = g_graspNow.holder == grasp::Holder::kLoose && g_gunRelease.want && g_haveLastGunRoom && heldSide >= 0;
        if (releaseFrame) {
            f.gunTracked = true;
            Compose(fromHead, g_lastGunDrawn, f.gunPose);  // (as drawn: where the let-go falls from)
        } else {
            f.gunTracked = t.gun;  // the right controller this frame (without it the magazine is kept a second)
            std::memcpy(f.gunPose, t.gunPose, sizeof(f.gunPose));
        }
        std::memcpy(f.glove, side == 0 ? tracked : g_rightFree, sizeof(f.glove));
        if (side == 0 ? t.gun && trackedLeft : t.gun && f.left) {
            float inverse[12], at[3];
            InvertRigid(t.gunPose, inverse);
            Transform(inverse, side == 0 ? palm : g_rightPalm, at);
            f.guardDistance = FromGuard(at);
        }
        if (side == 0)
            f.handFree = !g_supporting && !g_lining && reload::HandleSide() != 0 && g_gripBlend == 0.0f && g_pistolBlend == 0.0f &&
                         !(m320Out.side == 0 && (m320Out.holding || m320Out.spentSqueeze)) && !g_leftOnGrip && !g_leftSpent;  // (free hands: not on the rifle's pistol grip)
        else
            f.handFree = !g_supportingBy[1] && reload::HandleSide() != 1 && g_rightBlend == 0.0f && !(m320Out.side == 1 && (m320Out.holding || m320Out.spentSqueeze)) &&
                         !g_onGripBy[1] && !g_spentBy[1];
        // (The grenade launcher's open, opened last, and able to take a round: rounds left, none live in it, the hand free. Otherwise the rifle's magazine as before: told
        // the belly was the launcher's with no 40 mm left, it gave nothing at all, the review 2026-11-05.)
        f.bellyTaken = m320Out.wantsRound;
        f.wellEmpty = skin::MagazineOut();
        f.dry = reload::BoltBack();
        // The charging handle's before the shoulder's: with the gun at the eye it's right there.
        if (side == 0)
            f.atHandle = t.gun && trackedLeft && reload::NearHandle(t.gunPose, palm, TriggerOf(0) <= SqueezeOf(0));
        else
            f.atHandle = t.gun && f.left && reload::NearHandle(t.gunPose, g_rightPalm, TriggerOf(1) <= SqueezeOf(1), 1);
        // A magazine in a hand that can't keep it is let go of (it falls, or goes back at the belly), never cancelled: the rifle has left the hands (once loose
        // really has it, a frame after the let-go: till then it could still stay in the hand), another hand works the rifle now, or free hands are off for the
        // right's. On the seated one it stays in.
        const bool rifleLying = g_graspNow.holder == grasp::Holder::kLoose;
        const bool rifleGone = g_freeOn && rifleLying && g_gunLoose.id != 0;
        const bool rifleAway = g_freeOn && g_graspNow.holder == grasp::Holder::kStowed;  // (holsters: put away at its place, the gun tracked by no hand)
        const bool freeOff = !g_freeOn && heldSide == 1;
        f.dropHeld = !testing && heldSide >= 0 && (freeOff || rifleGone || rifleAway || (!rifleLying && g_workHand != heldSide));
        int loaded = 0, spare = 0;
        f.spare = ammo::Get(loaded, spare) && spare > 0;
        // pouch.txt "test": a stand-in left hand (with nobody in the headset) takes one as if
        // squeezed in the pouch once the magazine is out and holds it upright 20 cm below the well;
        // "test 2" pushes it up the well's line from below the mouth at 10 cm/s ("test 2 -0.05" stops
        // 5 cm from home).
        static int s_testFrames = 0;
        static float s_testAlong = 0.0f;
        if (testing) {
            f.left = true;
            f.leftGrip = standIn.position;
            s_testFrames = f.wellEmpty ? s_testFrames + 1 : 0;
            f.squeeze = s_testFrames > 1 ? 1.0f : 0.0f;
            f.forceInside = true;
            f.handFree = true;
            f.guardDistance = 1.0f;
            const bool up = g_pouchTest == 2 && pouch::Holding();
            s_testAlong = up ? std::fmin(s_testAlong + 0.1f * dt, g_pouchTestAlong) : -0.12f;
            float place[12] = {1, 0, 0, 0, 0, 1, 0, up ? s_testAlong : -0.20f, 0, 0, 1, up ? 0.0f : 0.06f};
            // "test 3": the seated one taken by the hand and drawn down out of the well, as a real hand does it.
            static float s_testDown = 0.0f;
            if (g_pouchTest == 3) {
                f.forceInside = false;
                static int s_seatedFrames = 0;
                s_seatedFrames = f.wellEmpty ? 0 : s_seatedFrames + 1;
                f.squeeze = s_seatedFrames > 30 || f.wellEmpty ? 1.0f : 0.0f;
                s_testDown = s_seatedFrames > 45 ? std::fmin(s_testDown + 0.25f * dt, 0.25f) : f.wellEmpty ? std::fmin(s_testDown + 0.25f * dt, 0.25f) : 0.0f;
                place[7] = -s_testDown, place[11] = 0.0f;
            }
            float at[12];
            Compose(t.gunPose, place, at);
            pouch::GloveFor(at, f.glove);
        } else {
            s_testFrames = 0;
        }
        magazine = pouch::Update(f);
        // (Its arming, kept as its Update counts it for FreeHands' claim: a side newly fed is armed only under 0.4, under 0.4 arms, and past 0.6 with the hand
        // seen takes the arming.)
        if (f.side != g_pouchFedSide)
            g_pouchArmed = f.squeeze < 0.4f, g_pouchFedSide = f.side;
        if (f.squeeze < 0.4f)
            g_pouchArmed = true;
        if (f.left && g_pouchArmed && f.squeeze > 0.6f)
            g_pouchArmed = false;
        // A hand that couldn't keep its magazine, said as the pouch had it go: fallen or back at the belly, or the seated one left in (gone without a fall is the
        // pouch's own line below).
        if (f.dropHeld && !magazine.cancelled) {
            const char* why = rifleGone   ? "the rifle left the hands"
                              : rifleAway ? "the rifle was put away"
                              : freeOff   ? "free hands are off for it"
                                          : "another hand works the rifle now";
            if (magazine.letGo || magazine.putBack)
                Log("Pouch: the magazine in the %s hand let go of: %s", kHandName[heldSide], why);
            else
                Log("Pouch: the %s hand let go of the seated magazine: it stays in (%s)", kHandName[heldSide], why);
        }
        const int magSide = magazine.side == 1 ? 1 : 0;  // whose hand its events are
        if (magazine.squeezed)
            Log("Pouch: the %s grip squeezed %.0f cm right, %.0f cm up, %.0f cm forward of the neck "
                "(%.2f of the zone's size; %s): %s",
                kHandName[magSide], magazine.fromMiddle[0] * 100.0f, magazine.fromMiddle[1] * 100.0f, magazine.fromMiddle[2] * 100.0f,
                magazine.size, testing ? "a test's hand" : "tracked",
                !magazine.reached ? "outside it"
                : pistolMag       ? "for the train pistol's magazine (Pistol reload)"
                : *magazine.note  ? magazine.note
                                  : "nothing taken");
        if (magazine.took)
            g_freshBy[magSide] = false;  // not the handguard's as well
        if (magazine.grabbed) {
            g_freshBy[magSide] = false;
            Log("Pouch: the %s hand has the seated magazine", kHandName[magSide]);
        }
        if (magazine.toGrip) {
            g_freshBy[magSide] = true;
            Log("Pouch: the hand went on to the gun: the magazine stays in, the squeeze is the handguard's");
        }
        if (magazine.pulledOut) {
            Log("Pouch: the magazine drawn out by hand");
            ammo::Eject();
            skin::PullMagazine();
            sounds::Play("mag_out");
        }
        // Any other gun (the launchers too) reloads over the shoulder as the game does: a fresh squeeze
        // there is its reload (the user: ammo from the left shoulder for whatever gun is in hand). The real
        // hand only, at most once every 1.5 s. Not the train pistol with its magazine by hand.
        static ULONGLONG s_otherReload = 0;
        // A squeeze the handguard would take (Support: the palm within kGuardReach of its line, the
        // hands more than kHandsTogether apart) stays the handguard's, as before the pouch.
        bool guardWouldTake = false;
        if (t.gun && trackedLeft) {
            const float apart[3] = {palm[0] - t.rightWrist[0], palm[1] - t.rightWrist[1], palm[2] - t.rightWrist[2]};
            guardWouldTake = f.guardDistance < kGuardReach && Length(apart) > kHandsTogether;
        }
        if (magazine.reached && !knownGun && !pistolMag && f.handFree && !testing && g_anyGunReload && !skin::HeldByHand() &&
            !(skin::LauncherInHand() && skin::LauncherHands()) &&  // the launcher's come from over the shoulder (rocket.cpp)
            (!guardWouldTake || heldGun) && GetTickCount64() - s_otherReload > 1500 && g_holdsGunNow.load(std::memory_order_relaxed) && ammo::Get(loaded, spare)) {
            s_otherReload = GetTickCount64();
            g_freshBy[magSide] = false;  // (the hand that squeezed: free hands, the right with a gun in the left)
            Log("Pouch: the game's own reload for the gun in hand");
            input::MagazineTap();
        }
        if (magazine.caught)
            Log("Pouch: the magazine caught under the well");
        if (magazine.offLine)
            Log("Pouch: the magazine pulled off the well's line");
        if (magazine.putBack)
            Log("Pouch: the magazine put back");
        if (magazine.cancelled)
            Log("Pouch: the magazine in the hand is gone (another went in, the gun went, the hand or the gun was "
                "lost a second, or it was let go of while lost)");
        if (magazine.letGo) {
            // (Let go of because the hand can't keep it, its grip still squeezed: that hand drawn open, g_dropOpenState.)
            if (f.dropHeld && f.squeeze >= 0.4f)
                g_dropOpenState[magSide] = 1;
            float v[3];  // the hand's speed as it let go, from the room into the view's space
            for (int i = 0; i < 3; ++i)
                v[i] = toView[i * 3] * magazine.dropVelocity[0] + toView[i * 3 + 1] * magazine.dropVelocity[1] +
                       toView[i * 3 + 2] * magazine.dropVelocity[2];
            if (magSide == 1) {
                // (Free hands: let go of by the right, it falls by that hand's own motion: its grip and its controller's own speed and turn now.)
                loose::HandAt hand;
                XrPosef p{};
                hand.tracked = HandPoseOf(1, p);
                if (hand.tracked)
                    PoseMatrix(p, hand.room);
                hand.sensed = hand.tracked && HandVelocityOf(1, hand.velocity, hand.spin);
                if (!hand.sensed)
                    for (int i = 0; i < 3; ++i)
                        hand.velocity[i] = hand.spin[i] = 0.0f;
                skin::DropHeldMagazine(v, 1, &hand);
            } else {
                skin::DropHeldMagazine(v);
            }
        }
        if (magazine.seated) {
            Log("Pouch: the magazine pushed home");
            ammo::Insert();
            skin::InsertMagazine();
            sounds::Play("mag_in");
            g_seatedAt = GetTickCount64();
            g_freshBy[magSide] = false;  // the squeeze that pushed it home isn't the handguard's
        }
        t.mag = magazine.mag;
        std::memcpy(t.magPose, magazine.magPose, sizeof(t.magPose));
        if (testing && magazine.holding) {  // the test's glove, holding it
            std::memcpy(t.leftPose, f.glove, sizeof(t.leftPose));
            Transform(t.leftPose, g_leftAnchor, t.leftWrist);
            t.left = true;
        }
    }
    // Free hands: the gun has left the hands (loose has it, a frame after the let-go as the pouch's, or it flies into a hand from the ground; or holsters
    // put it away at its place): what the left carries for it is let go of, a rocket or a case kept in the hand, nothing taken off it (one rule for every
    // gun). (A put-away is refused while the other hand holds a rocket, a case or the grenade launcher's round: holster.h.)
    const bool gunStowed = g_graspNow.holder == grasp::Holder::kStowed;
    const bool gunLeftHands = (g_graspNow.holder == grasp::Holder::kLoose && g_gunLoose.id != 0) ||
                              (g_graspNow.holder == grasp::Holder::kGlide && g_graspNow.glideFromWorld) || gunStowed;
    // Any other gun with no magazine button (handmag.cpp): its magazine out by the hand, or knocked out by a fresh one, and its side lever.
    const handmag::Gun* magGun = skin::HeldMagGun();
    handmag::Result heldMag;
    const int heldTest = magGun && !trackedLeft ? skin::HeldTest() : 0;
    bool heldTesting = false;
    float heldTestGlove[12];
    // Whose hand it serves (the owner, 2026-11-19: "hold a gun by any barrel instead of the trigger grip ... and still pull whatever lever with whichever
    // hand"): the hand it has something in until it's done, else the hand at work (FreeHands: the one not holding the gun, by its pistol grip or its front
    // alone; the right only while its own pose is drawn); the left while dormant, while nobody holds the gun, and for weapon.txt "heldtest"'s scripted left,
    // as before (workhand.h).
    const int heldBusy = handmag::BusySide();
    const int heldSide = workhand::FedSide(heldBusy, g_workHand, g_rightWorks, heldTest != 0);
    {
        handmag::Frame hf;
        hf.dt = frameSeconds;
        hf.gun = magGun;
        hf.side = heldSide;
        // (The frame the gun's let go of it falls from where it was drawn last frame, as the pouch's: that place, with a magazine in the hand, so it isn't
        // taken for one whose gun or hand was lost, and is let go of next frame at the hand's own speed, one rule for every gun.)
        if (g_graspNow.holder == grasp::Holder::kLoose && g_gunRelease.want && g_haveLastGunRoom && handmag::Holding()) {
            hf.gunTracked = true;
            Compose(fromHead, g_lastGunDrawn, hf.gunPose);  // (as drawn: where the let-go falls from)
        } else {
            hf.gunTracked = t.gun;
            std::memcpy(hf.gunPose, t.launcherPose, sizeof(hf.gunPose));
        }
        // That hand's own controller, glove and front place (each its own: a film round's scripted hand too, as SqueezeOf and HandPoseOf give it), its reach
        // to the belly (the pouch is fed the hand at work too), free of the gun's grips.
        XrPosef handGrip{};
        if (heldSide == 1) {
            hf.left = g_rightWorks && HandPoseOf(1, handGrip);
            std::memcpy(hf.glove, g_rightFree, sizeof(hf.glove));  // (the free right glove: its bind space the left's mirror image)
            hf.squeeze = SqueezeOf(1);
            hf.trigger = TriggerOf(1);
            std::memcpy(hf.frontGrip, g_profile.support[1], sizeof(hf.frontGrip));  // (the game's left palm on its front, mirrored: the right's place there)
        } else {
            hf.left = trackedLeft && !g_lining && HandPoseOf(0, handGrip);
            std::memcpy(hf.glove, tracked, sizeof(hf.glove));
            hf.squeeze = SqueezeOf(0);
            hf.trigger = TriggerOf(0);
            std::memcpy(hf.frontGrip, launcherFront, sizeof(hf.frontGrip));
        }
        // Its palm as the front grip's own take measures it (LauncherSupport: the glove's palm point, the right's mirrored): a grip's squeeze that's the
        // front's by it or by the hand's middle isn't a lever's or the lid's (the review of fc010e2).
        {
            const float palmPoint[3] = {heldSide == 1 ? -g_leftAnchor[0] : g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2]};
            Transform(hf.glove, palmPoint, hf.palm);
            hf.havePalm = true;
        }
        hf.gripRoom[0] = handGrip.position.x, hf.gripRoom[1] = handGrip.position.y, hf.gripRoom[2] = handGrip.position.z;
        hf.shoulder = magazine.reached && magazine.side == heldSide;  // (that hand's reach to the belly)
        hf.atShoulder = magazine.atPouch && magazine.side == heldSide;
        hf.wellEmpty = skin::MagazineOut();
        int loadedNow = 0, spareNow = 0;
        const bool counted = ammo::Get(loadedNow, spareNow);
        hf.spare = counted && spareNow > 0;
        if (heldSide == 1)
            hf.handFree = !g_onLauncherBy[1] && !g_onGripBy[1] && !g_spentBy[1];  // (not on the gun's front or its pistol grip)
        else
            hf.handFree = !g_onLauncher && !g_lining && !g_leftOnGrip && !g_leftSpent;  // (free hands: not on the gun's pistol grip)
        // The gun has left the hands; or the hand with something of it in is no longer the hand at work (the gun back in that hand as at a new life):
        // what it has is let go of.
        hf.dropHeld = gunLeftHands || workhand::NoLongerAtWork(heldBusy, g_workHand, g_freeOn, g_graspNow.holder == grasp::Holder::kLoose);
        // weapon.txt "heldtest": a scripted left hand, with nobody in the headset. Its keys are the hand's offset on the gun from where it holds the seated magazine, the magazine's tilt,
        // what the hand does; 1 pulls it out and rocks a new one in, 2 the Afghan reload, 3 and 4 the lever over the top and from below, 9 a handle with a notch
        // (the UMP-45's) drawn back and lifted into it, then knocked down.
        static int s_testWas = 0;
        static float s_testTime = 0.0f;
        s_testTime = heldTest != s_testWas ? 0.0f : s_testTime + frameSeconds;
        s_testWas = heldTest;
        XrPosef standInHand;
        if (heldTest && input::StandInLeftPose(standInHand)) {
            struct Key {
                float time;
                float off[3];
                float tilt;
            };
            float toLever[3], toShoulder[3], handOnMag[3], inverse[12], shoulderOnGun[3];
            constexpr float kMiddle[3] = {0.5363f, 1.0976f, 0.2092f};
            float holdInverse[12], seatedGlove[12];
            handmag::GloveFor(*magGun, t.launcherPose, seatedGlove);
            (void)holdInverse;
            InvertRigid(t.launcherPose, inverse);
            {
                float at[3];
                Transform(seatedGlove, kMiddle, at);
                Transform(inverse, at, handOnMag);
            }
            const float shoulderView[3] = {-0.18f, -0.02f, 0.06f};
            Transform(inverse, shoulderView, shoulderOnGun);
            for (int i = 0; i < 3; ++i) {
                toLever[i] = magGun->lever[i] - handOnMag[i];
                toShoulder[i] = shoulderOnGun[i] - handOnMag[i];
            }
            const float rest[3] = {0.14f, -0.22f, -0.10f};
            float under[3] = {0.0f, magGun->mouth - 0.04f, 0.0f}, boxOff[3] = {0.0f, -0.16f, 0.03f};
            if (magGun->way) {
                // A box with a way (the M249's): drawn off along it to its last row, well past where it comes free; a fresh one brought 3 cm to the left
                // of where its way on starts and 4 cm under it, where the magnet takes it.
                float at[3];
                Transform(magGun->way->off[magGun->way->offCount - 1] + 1, magGun->grab, at);
                for (int i = 0; i < 3; ++i)
                    boxOff[i] = at[i] - magGun->grab[i];
                boxOff[2] += 0.03f;
                Transform(magGun->way->on[0] + 1, magGun->grab, at);
                for (int i = 0; i < 3; ++i)
                    under[i] = at[i] - magGun->grab[i];
                under[0] += 0.03f, under[1] -= 0.04f;
            }
            const float t2 = s_testTime;
            std::vector<Key> keys;
            float squeezeNow = 0.0f, triggerNow = 0.0f;
            bool shoulderNow = false;
            static bool s_shouldered = false;
            if (t2 < 0.05f)
                s_shouldered = false;
            auto window = [&](float from, float to) { return t2 >= from && t2 < to; };
            auto shoulderAt = [&](float when) {
                if (t2 >= when && !s_shouldered) {
                    s_shouldered = true;
                    shoulderNow = true;
                }
            };
            if (heldTest == 1 || heldTest == 6) {
                keys = {{0.0f, {rest[0], rest[1], rest[2]}, 0.0f}, {1.0f, {0, 0, 0}, 0.0f}, {1.3f, {0, 0, 0}, 0.0f}, {2.0f, {0.0f, -0.16f, 0.03f}, 0.2f}, {2.4f, {0.0f, -0.16f, 0.03f}, 0.2f},
                        {3.2f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f}, {3.5f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f}, {4.4f, {under[0], under[1], under[2]}, 0.4f},
                        {4.7f, {under[0], under[1], under[2]}, 0.4f}, {5.7f, {0, 0, 0}, 0.0f}, {6.0f, {0, 0, 0}, 0.0f}, {6.7f, {toLever[0], toLever[1], toLever[2]}, 0.0f}, {7.0f, {toLever[0], toLever[1], toLever[2]}, 0.0f},
                        {7.6f, {toLever[0], toLever[1], toLever[2] - 0.10f}, 0.0f}, {7.9f, {toLever[0], toLever[1], toLever[2] - 0.10f}, 0.0f}, {8.6f, {rest[0], rest[1], rest[2]}, 0.0f}};
                squeezeNow = (heldTest == 1 && window(1.1f, 2.2f)) || window(3.4f, 5.9f) ? 1.0f : 0.0f;
                triggerNow = window(6.9f, 7.8f) ? 1.0f : 0.0f;
                shoulderAt(3.4f);
                // "heldtest 6", a gun with a button: X, instead of the hand pulling it out.
                static bool s_tapped = false;
                if (t2 < 0.05f)
                    s_tapped = false;
                if (heldTest == 6 && magGun->button && t2 >= 1.2f && !s_tapped) {
                    s_tapped = true;
                    input::MagazineTap(true);
                }
            } else if (heldTest == 7 && magGun->belt) {
                // A belt fed gun's whole reload: lid up, box out, a new one in, the belt into the feed, lid shut, the lever.
                const handmag::BeltGun& b = *magGun->belt;
                float pinchOnMag[3];
                {
                    float at[3];
                    Transform(seatedGlove, b.pinch, at);
                    Transform(inverse, at, pinchOnMag);
                }
                float lid[3] = {b.lidGrab[0] - handOnMag[0], b.lidGrab[1] - handOnMag[1], b.lidGrab[2] - handOnMag[2]};
                auto lidAt = [&](float angle, float* out) {
                    const float c = std::cos(angle), sn = std::sin(angle), y = b.lidGrab[1] - b.lidHinge[0], z = b.lidGrab[2] - b.lidHinge[1];
                    out[0] = lid[0];
                    out[1] = b.lidHinge[0] + c * y - sn * z - handOnMag[1];
                    out[2] = b.lidHinge[1] + sn * y + c * z - handOnMag[2];
                };
                float lidHalf[3], lidUp[3];
                lidAt(b.lidOpen * 0.5f, lidHalf);
                lidAt(b.lidOpen, lidUp);
                // The pinch at the belt's end, then up to the feed. A little short of it: the feed takes it from there.
                float tipHanging[3] = {b.testTip[0], b.testTip[1], b.testTip[2]};
                if (tipHanging[0] == 0.0f && tipHanging[1] == 0.0f && tipHanging[2] == 0.0f)
                    handmag::HangingTip(b, tipHanging);
                const float belt[3] = {tipHanging[0] - pinchOnMag[0], tipHanging[1] - pinchOnMag[1], tipHanging[2] - pinchOnMag[2]};
                const float feed[3] = {b.feed[0] + 0.06f - pinchOnMag[0], b.feed[1] + 0.02f - pinchOnMag[1], b.feed[2] - pinchOnMag[2]};
                keys = {{0.0f, {rest[0], rest[1], rest[2]}, 0.0f}, {1.0f, {lid[0], lid[1], lid[2]}, 0.0f}, {1.3f, {lid[0], lid[1], lid[2]}, 0.0f}, {1.7f, {lidHalf[0], lidHalf[1], lidHalf[2]}, 0.0f}, {2.1f, {lidUp[0], lidUp[1], lidUp[2]}, 0.0f},
                        {2.4f, {lidUp[0], lidUp[1], lidUp[2]}, 0.0f}, {3.2f, {0, 0, 0}, 0.0f}, {3.5f, {0, 0, 0}, 0.0f}, {4.2f, {boxOff[0], boxOff[1], boxOff[2]}, 0.0f}, {4.6f, {boxOff[0], boxOff[1], boxOff[2]}, 0.0f},
                        {5.4f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f}, {5.7f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f}, {6.6f, {under[0], under[1], under[2]}, 0.0f}, {7.0f, {under[0], under[1], under[2]}, 0.0f}, {7.5f, {0, 0, 0}, 0.0f},
                        {7.8f, {0, 0, 0}, 0.0f}, {8.5f, {belt[0], belt[1], belt[2]}, 0.0f}, {8.8f, {belt[0], belt[1], belt[2]}, 0.0f}, {9.6f, {feed[0], feed[1], feed[2]}, 0.0f}, {10.0f, {feed[0], feed[1], feed[2]}, 0.0f},
                        {10.8f, {lidUp[0], lidUp[1], lidUp[2]}, 0.0f}, {11.1f, {lidUp[0], lidUp[1], lidUp[2]}, 0.0f}, {11.5f, {lidHalf[0], lidHalf[1], lidHalf[2]}, 0.0f}, {11.9f, {lid[0], lid[1], lid[2]}, 0.0f}, {12.2f, {lid[0], lid[1], lid[2]}, 0.0f},
                        {13.0f, {toLever[0], toLever[1], toLever[2]}, 0.0f}, {13.3f, {toLever[0], toLever[1], toLever[2]}, 0.0f}, {14.0f, {toLever[0], toLever[1], toLever[2] - 0.11f}, 0.0f}, {14.3f, {toLever[0], toLever[1], toLever[2] - 0.11f}, 0.0f}, {15.1f, {rest[0], rest[1], rest[2]}, 0.0f}};
                squeezeNow = window(1.2f, 2.3f) || window(3.4f, 4.4f) || window(5.6f, 7.7f) || window(8.7f, 9.9f) || window(11.0f, 12.1f) ? 1.0f : 0.0f;
                triggerNow = window(13.2f, 14.2f) ? 1.0f : 0.0f;
                shoulderAt(5.6f);
            } else if (heldTest == 2 || heldTest == 5) {
                // 2: the Afghan reload, with force. 5: the same sweep gently: it stops against the old one, then comes back and knocks it out with force.
                const float sweepFrom[3] = {-0.015f, -0.07f, -0.16f}, sweepTo[3] = {-0.015f, -0.03f, 0.02f};
                if (heldTest == 2)
                    keys = {{0.0f, {rest[0], rest[1], rest[2]}, 0.0f}, {0.8f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f}, {1.1f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f},
                            {2.0f, {sweepFrom[0], sweepFrom[1], sweepFrom[2]}, -0.1f}, {2.3f, {sweepFrom[0], sweepFrom[1], sweepFrom[2]}, -0.1f}, {2.42f, {sweepTo[0], sweepTo[1], sweepTo[2]}, -0.3f}, {3.3f, {under[0], under[1], under[2]}, 0.4f},
                            {3.6f, {under[0], under[1], under[2]}, 0.4f}, {4.6f, {0, 0, 0}, 0.0f}, {4.9f, {0, 0, 0}, 0.0f}, {5.6f, {rest[0], rest[1], rest[2]}, 0.0f}};
                else
                    keys = {{0.0f, {rest[0], rest[1], rest[2]}, 0.0f}, {0.8f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f}, {1.1f, {toShoulder[0], toShoulder[1], toShoulder[2]}, 0.0f},
                            {2.0f, {sweepFrom[0], sweepFrom[1], sweepFrom[2]}, -0.1f}, {2.3f, {sweepFrom[0], sweepFrom[1], sweepFrom[2]}, -0.1f}, {3.5f, {sweepTo[0], sweepTo[1], sweepTo[2]}, -0.3f}, {4.0f, {sweepTo[0], sweepTo[1], sweepTo[2]}, -0.3f},
                            {4.7f, {sweepFrom[0], sweepFrom[1], sweepFrom[2]}, -0.1f}, {5.0f, {sweepFrom[0], sweepFrom[1], sweepFrom[2]}, -0.1f}, {5.12f, {sweepTo[0], sweepTo[1], sweepTo[2]}, -0.3f}, {6.0f, {rest[0], rest[1], rest[2]}, 0.0f}};
                squeezeNow = window(0.9f, heldTest == 2 ? 4.8f : 5.5f) ? 1.0f : 0.0f;
                shoulderAt(0.9f);
            } else if (heldTest == 9 && magGun->leverNotch > 0.0f) {
                // "heldtest 9", a handle with a notch (the UMP-45's): drawn back and lifted into its notch, the hand away above it for a second, then down on
                // it: knocked out, the bolt home.
                const float c9 = std::cos(magGun->leverNotch), s9 = std::sin(magGun->leverNotch);
                const float dx = magGun->lever[0] - magGun->leverNotchAxis[0], dy = magGun->lever[1] - magGun->leverNotchAxis[1];
                const float knob[3] = {magGun->leverNotchAxis[0] + c9 * dx - s9 * dy - handOnMag[0], magGun->leverNotchAxis[1] + s9 * dx + c9 * dy - handOnMag[1],
                                       magGun->lever[2] - magGun->leverNotchBack - handOnMag[2]};
                const float back = magGun->leverTravel + 0.005f;
                keys = {{0.0f, {rest[0], rest[1], rest[2]}, 0.0f}, {0.5f, {toLever[0], toLever[1] + 0.2f, toLever[2]}, 0.0f}, {1.0f, {toLever[0], toLever[1], toLever[2]}, 0.0f},
                        {1.5f, {toLever[0], toLever[1], toLever[2]}, 0.0f}, {2.3f, {toLever[0], toLever[1], toLever[2] - back}, 0.0f},
                        {2.6f, {toLever[0], toLever[1] + 0.035f, toLever[2] - back}, 0.0f}, {3.0f, {knob[0], knob[1] + 0.2f, knob[2]}, 0.0f},
                        {4.0f, {knob[0], knob[1] + 0.2f, knob[2]}, 0.0f}, {4.15f, {knob[0], knob[1] + 0.01f, knob[2]}, 0.0f}, {4.6f, {knob[0], knob[1] + 0.01f, knob[2]}, 0.0f},
                        {5.2f, {rest[0], rest[1], rest[2]}, 0.0f}};
                triggerNow = window(1.3f, 2.7f) ? 1.0f : 0.0f;
            } else {
                // Over the top it comes down from above, not up through the gun.
                const float from[3] = {heldTest == 3 ? toLever[0] : rest[0], heldTest == 3 ? toLever[1] + 0.2f : rest[1], heldTest == 3 ? toLever[2] : rest[2]};
                keys = {{0.0f, {rest[0], rest[1], rest[2]}, 0.0f}, {0.5f, {from[0], from[1], from[2]}, 0.0f}, {1.0f, {toLever[0], toLever[1], toLever[2]}, 0.0f}, {1.5f, {toLever[0], toLever[1], toLever[2]}, 0.0f},
                        {2.3f, {toLever[0], toLever[1], toLever[2] - 0.10f}, 0.0f}, {2.8f, {toLever[0], toLever[1], toLever[2] - 0.10f}, 0.0f}, {3.2f, {from[0], from[1], from[2]}, 0.0f}, {3.6f, {rest[0], rest[1], rest[2]}, 0.0f}};
                triggerNow = window(1.3f, 2.6f) ? 1.0f : 0.0f;
            }
            float off[3] = {keys.back().off[0], keys.back().off[1], keys.back().off[2]}, tilt = keys.back().tilt;
            for (size_t k = 0; k + 1 < keys.size(); ++k)
                if (t2 >= keys[k].time && t2 < keys[k + 1].time) {
                    float s = (t2 - keys[k].time) / (keys[k + 1].time - keys[k].time);
                    s = s * s * (3.0f - 2.0f * s);
                    for (int i = 0; i < 3; ++i)
                        off[i] = keys[k].off[i] + (keys[k + 1].off[i] - keys[k].off[i]) * s;
                    tilt = keys[k].tilt + (keys[k + 1].tilt - keys[k].tilt) * s;
                    break;
                }
            // The magazine tilted about its top, bottom forward; the hand wherever the keys have it. "heldtest 4": the wrist rolled over, from below.
            const float c = std::cos(tilt), sn = std::sin(tilt);
            const float tilted[12] = {1, 0, 0, 0, 0, c, sn, 0, 0, -sn, c, 0};
            float place[12];
            Compose(t.launcherPose, tilted, place);
            handmag::GloveFor(*magGun, place, heldTestGlove);
            if (heldTest == 3 || heldTest == 4 || (heldTest == 9 && magGun->leverNotch > 0.0f)) {
                // The wrist as a real hand has it on a lever, in the gun's axes (x left, y up, z forward): "heldtest 3" reaching over the top, the palm down, the fingers over to the gun's right, the thumb towards the
                // shooter; "heldtest 4" from under it, the palm up. A gun with its handle on its left (the G3's) is reached from its own side: the palm towards the gun, the fingers forward.
                constexpr float kBindAlong[3] = {0.437f, -0.544f, 0.716f}, kBindPalm[3] = {-0.639f, -0.748f, -0.179f}, kBindAcross[3] = {0.633f, -0.380f, -0.675f};
                const bool onItsLeft = magGun->lever[0] > 0.01f;
                const float along[3] = {onItsLeft ? 0.2f : heldTest == 3 ? -0.954f : -0.976f, onItsLeft ? 0.0f : heldTest == 3 ? 0.0f : 0.217f, onItsLeft ? 0.98f : heldTest == 3 ? 0.3f : 0.0f};
                const float palmFaces[3] = {onItsLeft ? -0.98f : heldTest == 3 ? 0.0f : -0.217f, onItsLeft ? 0.0f : heldTest == 3 ? -1.0f : 0.976f, onItsLeft ? 0.2f : 0.0f};
                const float across[3] = {along[1] * palmFaces[2] - along[2] * palmFaces[1], along[2] * palmFaces[0] - along[0] * palmFaces[2], along[0] * palmFaces[1] - along[1] * palmFaces[0]};
                float onGunTurn[12] = {};
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        onGunTurn[i * 4 + j] = along[i] * kBindAlong[j] + palmFaces[i] * kBindPalm[j] + across[i] * kBindAcross[j];
                Compose(t.launcherPose, onGunTurn, heldTestGlove);
            }
            // Tilted about the magazine's top, not about the hand: the keys say where its top is.
            float want[3], middle[3], onGun[3];
            const float fromTop[3] = {handOnMag[0] - magGun->top[0], handOnMag[1] - magGun->top[1], handOnMag[2] - magGun->top[2]};
            const float swung[3] = {fromTop[0], c * fromTop[1] + sn * fromTop[2], -sn * fromTop[1] + c * fromTop[2]};
            for (int i = 0; i < 3; ++i)
                onGun[i] = magGun->top[i] + swung[i] + off[i];
            Transform(t.launcherPose, onGun, want);
            Transform(heldTestGlove, kMiddle, middle);
            for (int i = 0; i < 3; ++i)
                heldTestGlove[i * 4 + 3] += want[i] - middle[i];
            hf.left = true;
            std::memcpy(hf.glove, heldTestGlove, sizeof(hf.glove));
            hf.havePalm = false;  // (the front by the hand's middle, as the scripted left always had it)
            // The room's speed from the view's: near enough for a test's drop.
            hf.gripRoom[0] = want[0], hf.gripRoom[1] = want[1], hf.gripRoom[2] = want[2];
            hf.squeeze = squeezeNow;
            hf.trigger = triggerNow;
            hf.shoulder = shoulderNow;
            hf.atShoulder = false;
            hf.handFree = true;
            heldTesting = true;
        }
        heldMag = handmag::Update(hf);
        // (Whose hand its events are: the hand fed, or the last one on the frame it lets go of what it had. The left's lines as before; the right's say so.)
        const int heldBy = heldMag.side == 1 ? 1 : 0;
        const char* const byHand = heldBy == 1 ? " (the right hand)" : "";
        {
            static int s_heldSideWas = 0;
            if (magGun && heldSide != s_heldSideWas)
                Log("Held magazine: the %s hand works the %s's magazine, lever and the rest now", kHandName[heldSide], g_profile.name);
            s_heldSideWas = heldSide;
        }
        if (heldMag.note && *heldMag.note)
            Log("Held magazine: a squeeze at the belly took nothing%s: %s", byHand, heldMag.note);
        if (heldMag.spentSqueeze)
            g_freshBy[heldBy] = false;  // (the left's g_freshSqueeze)
        if (heldMag.took)
            Log("Held magazine: a fresh one taken at the belly%s", byHand);
        if (heldMag.grabbed)
            Log("Held magazine: the %s hand has the seated one", kHandName[heldBy]);
        if (heldMag.toGrip) {
            Log("Held magazine: the hand went on to the gun: it stays in, the squeeze is the front grip's%s", byHand);
            g_freshBy[heldBy] = true;
        }
        if (heldMag.leftForGrip)
            Log("Held magazine: a squeeze by it left for the front grip: the hand (%.3f %.3f %.3f) from its middle, the palm %.2f up", heldMag.squeezeFrom[0], heldMag.squeezeFrom[1], heldMag.squeezeFrom[2], heldMag.palmUp);
        if (heldMag.touched) {
            Log("Held magazine: touched the old one at %.2f m/s: too gently to knock it out", heldMag.hitSpeed);
            sounds::Play("rocket_catch");
        }
        if (heldMag.pulledOut) {
            Log("Held magazine: pulled out by hand");
            ammo::Eject();
            skin::PullMagazine();
            PlayGunsOwn("mag_out", "mag_out");
        }
        if (heldMag.flicked) {
            Log("Held magazine: the fresh one knocked the old one out at %.2f m/s", heldMag.hitSpeed);
            ammo::Eject();
            skin::FlickMagazine(heldMag.flickVelocity);
            PlayGunsOwn("mag_out", "mag_out");
        }
        if (heldMag.caught)
            Log("Held magazine: near the well: going home by itself");
        if (heldMag.putBack)
            Log("Held magazine: put back at the belly");
        if (heldMag.cancelled)
            Log("Held magazine: the one in the hand is gone (the gun went, another went in, or the hand was lost a second)");
        if (heldMag.letGo) {
            // (Let go of because the gun left the hands, its grip still squeezed: the hand drawn open, as the pouch's. The hand that had it: the one fed, or the
            // last one as the other is fed.)
            const float squeezed = heldBy == heldSide ? std::fmax(hf.squeeze, hf.trigger) : std::fmax(SqueezeOf(heldBy), TriggerOf(heldBy));
            if ((hf.dropHeld || heldBy != heldSide) && squeezed >= 0.4f)
                g_dropOpenState[heldBy] = 1;
            float v[3];
            for (int i = 0; i < 3; ++i)
                v[i] = toView[i * 3] * heldMag.dropVelocity[0] + toView[i * 3 + 1] * heldMag.dropVelocity[1] + toView[i * 3 + 2] * heldMag.dropVelocity[2];
            if (heldBy == 1) {
                // (Let go of by the right, it falls by that hand's own motion, as the pouch's magazine in the right: its grip and its controller's own speed and turn.)
                loose::HandAt hand;
                XrPosef p{};
                hand.tracked = HandPoseOf(1, p);
                if (hand.tracked)
                    PoseMatrix(p, hand.room);
                hand.sensed = hand.tracked && HandVelocityOf(1, hand.velocity, hand.spin);
                if (!hand.sensed)
                    for (int i = 0; i < 3; ++i)
                        hand.velocity[i] = hand.spin[i] = 0.0f;
                skin::DropHeldMagazine(v, 1, &hand);
            } else {
                skin::DropHeldMagazine(v);
            }
            Log("Held magazine: let go of%s", byHand);
        }
        if (heldMag.seated) {
            Log("Held magazine: rocked home%s", byHand);
            ammo::Insert();
            skin::InsertMagazine();
            PlayGunsOwn("mag_in", "mag_in");
            g_seatedAt = GetTickCount64();
            g_freshBy[heldBy] = false;  // the squeeze that pushed it home isn't the front grip's
        }
        if (heldMag.leverTaken)
            Log("Held magazine: the fist closed on the lever%s", byHand);
        if (heldMag.leverMoving)
            PlayGunsOwn("lever", "lever");
        if (heldMag.leverLet) {
            Log("Held magazine: the lever let go of, drawn %.0f mm%s%s", heldMag.leverPulled * 1000.0f, heldMag.charged ? "" : ": not far enough to chamber", byHand);
            if (heldMag.charged) {
                int loaded = 0, spare = 0;
                const bool counted = ammo::Get(loaded, spare);
                if (workhand::LeverChambers(heldMag.charged, g_heldDry, skin::MagazineOut(), heldMag.beltReady, counted, loaded)) {
                    g_heldDry = false;
                    Log("Held magazine: a round chambered");
                }
                // (A bolt that locks back stays there with nothing to chamber: not the sound of it going home.)
                if (!magGun || !magGun->boltCatch || !g_heldDry)
                    PlayGunsOwn("bolt", "bolt");
            }
        }
        // A handle with a notch (the UMP-45's): turned up into it, the bolt is held open and nothing fires; knocked down, the bolt goes home.
        if (heldMag.notched) {
            Log("Held magazine: the handle turned up into its notch: the bolt held open");
            PlayGunsOwn("notch", "lever");
        }
        if (heldMag.knocked) {
            int loaded = 0, spare = 0;
            const bool chambers = g_heldDry && !skin::MagazineOut() && ammo::Get(loaded, spare) && loaded > 1;
            if (chambers)
                g_heldDry = false;
            Log("Held magazine: the handle knocked down out of its notch%s", chambers ? ": a round chambered" : "");
            PlayGunsOwn("bolt", "bolt");
        }
        g_notchBlocked = magGun && heldMag.leverHeld;
        // Another gun: its chamber as it is, its lid and belt as they come. (Not a frame or two with no gun known.)
        static const handmag::Gun* s_dryGun = nullptr;
        const int dryHandSlot = ammo::HandSlot();
        const uint32_t dryFiring = magGun ? static_cast<uint32_t>(ammo::FiringOf(dryHandSlot)) : 0u;
        if (magGun && magGun != s_dryGun) {
            s_dryGun = magGun;
            g_heldLoadedWas = -1;
            g_heldFiringWas = dryFiring;
            g_heldDry = ammo::Get(loadedNow, spareNow) && loadedNow <= 1 && !skin::MagazineOut();
            handmag::BeltReset();
        }
        // Run dry: no bolt held back on these, the chamber's simply empty until the lever's pulled with a magazine in. Its count coming up by itself is the game's own reload.
        // A drop is this gun's own rounds only while the hands' slot holds the very gun counted the frame before and, by its name, the gun drawn
        // (pickrules::SeeDry): a change of the slot's gun under the drawn one is no round fired (Operation Swordbreaker's roof, at4_measured.md section
        // 11: the kit's AT4 and its one rocket in slot 0 0.1 s after the prompt, the M4A1 drawn 0.3 s more, read as the M4A1 run dry; the kit's M4A1, 31
        // rounds, then passed for a reload of the game's, but a gun given back with fewer than 5 more than that would have stayed "empty" till a
        // magazine and its lever); a new gun there that is the one drawn has its chamber as its count is. The slot's name is ammo's, read at the top
        // of this Present with the slot's firing state; the count and the firing state here are read after the headset's waits, so a swap between the
        // two (the kit's, about a third of the frame) leaves the name a frame behind: nothing is decided by it till ammo's read holds the firing state
        // counted (kWaiting: the count and the firing state before kept), a second at most (a take's window holds ammo's read as it was).
        if (magGun && ammo::Get(loadedNow, spareNow)) {
            const recoilguns::Row* drawnRow = recoilguns::ByShape(skin::HeldShape());
            const recoilguns::Row* slotRow = ammo::RowInSlot(dryHandSlot);
            static int s_nameWaits = 0;  // frames in a row the slot's name wasn't read with the firing state counted
            pickrules::DryIn d;
            d.was = g_heldLoadedWas, d.firingWas = g_heldFiringWas, d.now = loadedNow, d.firing = dryFiring;
            d.otherGun = pickrules::OtherGun(drawnRow, slotRow);
            d.nameWithIt = ammo::SlotFiringRead(dryHandSlot) == dryFiring || s_nameWaits >= 72;
            d.magOut = skin::MagazineOut(), d.seated = heldMag.seated;
            const pickrules::DryOut dry = pickrules::SeeDry(d);
            s_nameWaits = dry.what == pickrules::DrySeen::kWaiting ? s_nameWaits + 1 : 0;
            static bool s_saidOther = false;
            switch (dry.what) {
            case pickrules::DrySeen::kRunDry:
                g_heldDry = true;
                Log("Held magazine: it's run dry: a new magazine, then the lever");
                break;
            case pickrules::DrySeen::kReloaded:
                g_heldDry = false;
                handmag::BeltReset();
                break;
            case pickrules::DrySeen::kNewGun:
                g_heldDry = dry.dry;
                handmag::BeltReset();
                Log("Held magazine: another %s in slot %d (a new gun there, no round fired): %d rounds loaded, its chamber %s", drawnRow ? drawnRow->name : "gun",
                    dryHandSlot, loadedNow, dry.dry ? "empty: a new magazine, then the lever" : "as its count is");
                break;
            case pickrules::DrySeen::kNotDrawn:
                if (!s_saidOther)
                    Log("Held magazine: slot %d holds the %s now, not the %s drawn (a change of gun under way): its count isn't the %s's, no round fired",
                        dryHandSlot, slotRow->name, drawnRow->name, drawnRow->name);
                break;
            case pickrules::DrySeen::kWaiting:
            case pickrules::DrySeen::kNone:
                break;
            }
            if (dry.what != pickrules::DrySeen::kWaiting)
                s_saidOther = dry.what == pickrules::DrySeen::kNotDrawn;
            g_heldLoadedWas = dry.was;
            if (!dry.keepFiring)
                g_heldFiringWas = dryFiring;
        }
        if (magGun) {
            t.mag = heldMag.mag;
            if (heldMag.mag)
                std::memcpy(t.magPose, heldMag.magPose, sizeof(t.magPose));
        }
        t.heldMagHidden = heldMag.hideSeated || magazine.hideSeated;
        t.heldLeverBack = heldMag.leverBack;
        // The port's cover, as the M416's (reload.cpp); its bolt locked back on an empty magazine holds it open.
        // A belt fed gun: its lid, its belt; it won't fire until they're as they should be and the lever's been pulled.
        t.heldLid = heldMag.lid;
        t.heldBeltDrawn = heldMag.beltDrawn;
        if (heldMag.beltDrawn)
            std::memcpy(t.heldBeltBones, heldMag.beltBones, sizeof(t.heldBeltBones));
        g_beltBlocked = magGun && magGun->belt && !heldMag.beltReady;
        // By the true count, as the game's own doesn't follow a reload by hand: about 2.6 rounds to a piece of the belt, the one kept in the gun not one of them.
        if (magGun && magGun->belt && counted)
            t.heldBeltPieces = std::fmin(static_cast<int>(std::ceil(std::fmax(loadedNow - 1, 0) / 2.6f)), 5);
        if (heldMag.lidTaken) {
            Log("Held magazine: the hand on the lid");
            PlayGunsOwn("lid_latch", "rocket_catch");
        }
        if (heldMag.lidUp) {
            Log("Held magazine: the lid's up");
            PlayGunsOwn("lid_up", "rocket_catch");
        }
        if (heldMag.lidShut) {
            Log("Held magazine: the lid's shut");
            PlayGunsOwn("lid_shut", "mag_in");
        }
        if (heldMag.beltTaken) {
            Log("Held magazine: the hand on the belt");
            PlayGunsOwn("belt", "rocket_catch");
        }
        if (heldMag.beltLaid) {
            Log("Held magazine: the belt's in the feed");
            PlayGunsOwn("belt_lay", "rocket_catch");
        }
        if (heldMag.beltLet)
            Log("Held magazine: the belt let go of before it's in");
        if (magGun && magGun->belt && heldMag.pulledOut)
            g_heldDry = true;  // the lever again, once it's all back
        static reload::Cover s_heldCover, s_heldKick;
        t.heldCover = reload::StepCover(s_heldCover, frameSeconds, magGun && magGun->boltCatch && g_heldDry, heldMag.leverBack, true);
        t.heldLeverSwing = heldMag.leverSwing;
        t.heldLeverNotch = heldMag.leverNotch;
        // A lever that doesn't move by itself as the gun fires goes back and home with each shot, as the cover opens and shuts.
        const float kick = reload::StepCover(s_heldKick, frameSeconds, false, 0.0f);
        if (magGun && magGun->kickLever > 0.0f)
            t.heldLeverBack = std::fmax(t.heldLeverBack, magGun->kickLever * kick);
        t.heldOldRocked = heldMag.oldRocked;
        if (heldMag.oldRocked)
            std::memcpy(t.heldOldRock, heldMag.oldRock, sizeof(t.heldOldRock));
        if (heldTesting) {
            std::memcpy(t.leftPose, heldTestGlove, sizeof(t.leftPose));
            Transform(t.leftPose, g_leftAnchor, t.leftWrist);
            t.left = true;
        }
    }
    // The Stinger's reload by hand (flap.cpp): fired, its aerial folds; lifted by the hand, it springs open with the next missile ready.
    bool stingerShot = false;  // (a missile really gone this frame: its kick, below)
    flap::Result heldFlap;
    {
        flap::Frame ff;
        ff.dt = frameSeconds;
        ff.aerial = skin::HeldAerial();
        ff.gunTracked = t.gun;
        std::memcpy(ff.gunPose, t.launcherPose, sizeof(ff.gunPose));
        ff.left = trackedLeft && !g_lining;
        std::memcpy(ff.glove, tracked, sizeof(ff.glove));
        ff.squeeze = input::LeftSqueeze();
        ff.trigger = input::LeftTrigger();
        ff.handFree = !g_onLauncher && !g_lining && !g_leftOnGrip && !g_leftSpent;  // (free hands: not on the gun's pistol grip)
        ff.held = !gunLeftHands;  // (let go of mid-lift, the lift lets go)
        static int s_missilesWas = -1;
        static unsigned s_whichWas = 0;
        int loaded = 0, spare = 0;
        unsigned which = 0;
        if (ff.aerial && ammo::StingerGet(loaded, spare, which)) {
            if (which != s_whichWas)  // a new life, or another one taken up: ready
                flap::Reset(), s_missilesWas = -1;
            s_whichWas = which;
            ff.shot = s_missilesWas >= 0 && loaded < s_missilesWas;
            stingerShot = ff.shot;
            ff.spare = spare > 0;
            if (ff.shot)
                Log("Stinger: a missile gone: %d left", spare);
            if (ff.shot && spare < 1)
                ammo::StingerOut();
            s_missilesWas = loaded;
        }
        // weapon.txt "heldtest 8": a scripted left hand with nobody in the headset: a shot as if fired, then the folded aerial lifted.
        const int flapTest = ff.aerial && !trackedLeft ? skin::HeldTest() : 0;
        static float s_flapTime = 0.0f;
        static bool s_flapShot = false;
        s_flapTime = flapTest == 8 ? s_flapTime + frameSeconds : 0.0f;
        if (flapTest != 8)
            s_flapShot = false;
        float flapGlove[12];
        if (flapTest == 8) {
            if (s_flapTime > 0.3f && !s_flapShot) {
                s_flapShot = true;
                ff.shot = true;
                ff.spare = true;
            }
            const float along = std::fmin(std::fmax((s_flapTime - 2.4f) / 1.0f, 0.0f), 1.0f) * (ff.aerial->liftKeys - 1);
            const int k = std::fmin(static_cast<int>(along), ff.aerial->liftKeys - 2);
            const float u = along - k;
            std::memcpy(flapGlove, ff.aerial->liftGlove[k], sizeof(flapGlove));
            for (int j = 0; j < 12; ++j)
                flapGlove[j] += (ff.aerial->liftGlove[k + 1][j] - ff.aerial->liftGlove[k][j]) * u;
            Compose(t.launcherPose, flapGlove, ff.glove);
            ff.left = true;
            ff.handFree = true;
            ff.squeeze = s_flapTime > 2.3f && s_flapTime < 3.6f ? 1.0f : 0.0f;
        }
        heldFlap = flap::Update(ff);
        if (heldFlap.spentSqueeze)
            g_freshSqueeze = false;
        if (heldFlap.folded) {
            Log("Stinger: its aerial's folded: lift it for the next missile");
            PlayGunsOwn("aerial_fold", "rocket_catch");
        }
        if (heldFlap.taken) {
            Log("Stinger: the hand on the aerial");
            PlayGunsOwn("aerial_take", "rocket_catch");
        }
        if (heldFlap.opened) {
            Log("Stinger: its aerial's open: the next missile's ready");
            ammo::StingerLoad();
            PlayGunsOwn("aerial_open", "mag_in");
        }
        g_stingerEmptyNow = heldFlap.empty;
        t.heldPanels = heldFlap.panels;
        if (heldFlap.panels)
            std::memcpy(t.heldPanelBones, heldFlap.bones, sizeof(t.heldPanelBones));
        if (flapTest == 8) {
            std::memcpy(t.leftPose, ff.glove, sizeof(t.leftPose));
            Transform(t.leftPose, g_leftAnchor, t.leftWrist);
            t.left = true;
        }
    }
    // The AT4 (Operation Swordbreaker's): one rocket, then a spent tube to throw away (the user, 2026-11-18: "once it's shot, it's MEANT to be thrown away
    // and discarded"). Its count is its own (ammo.cpp's At4Get, found by its name: the scene puts it in slot 0, the Mk11 stays in 1; its hidden round keeps
    // the game from reloading it by itself): a rocket gone when the count drops, its kick and buzz then (recoil's extra shot, below). At none the trigger
    // stops (input.cpp's FireBlocked) and letting go of it throws it away as any gun let go of falls (free hands, loose.cpp: its body, gunbodies.h's at4).
    // Spent for as long as it's drawn (holster.h's TubeSpent): the roof's kit gives the M4A1 back into its slot with the spent tube still drawn 0.56-0.89 s
    // more, its count unread then (no slot holds an AT4).
    bool at4Shot = false;
    {
        static int s_rocketsWas = -1;
        static unsigned s_whichWas = 0;
        int loaded = 0, spare = 0;
        unsigned which = 0;
        const bool drawn = skin::At4Held();
        const bool at4 = drawn && ammo::At4Get(loaded, spare, which);
        if (at4) {
            if (which != s_whichWas)  // another AT4 (a new life, its checkpoint loaded again): counted from what it holds now
                s_rocketsWas = -1;
            s_whichWas = which;
            at4Shot = s_rocketsWas >= 0 && loaded < s_rocketsWas;
            if (at4Shot)
                Log("AT4: the rocket gone: the tube is spent; let go of it to throw it away (%d more in the game's count, none to be had by hand)", spare);
            else if (s_rocketsWas < 0)
                Log("AT4: in hand, %d rocket in it (%d more in the game's count)", loaded, spare);
            s_rocketsWas = loaded;
        }
        g_at4SpentNow = holster::TubeSpent(g_at4Tube, drawn, at4, loaded);
    }
    // The train pistol's manual reload (build\pistol.txt "reload on"; so far only drawn): its tests, its magazine by
    // hand ("mag 1": pouch.cpp's events, after the pouch) and its slide by the left hand ("slide 1"), by where the hand
    // really is. The slide not while the glove is lined up (held still, not where the hand is), the hand carries a
    // magazine (the M416's, or the pistol's in the hand or on the grip's line) or has just pushed the pistol's home;
    // nor grabbed while the right trigger is pulled (a left trigger squeezed along with a shot: the grab's 13 cm takes
    // in the two-handed hold, and a slide held holds the trigger off), though one held stays held.
    // A tube shotgun's shells by hand (shellload.h): a shell from the belly, up the loading port or into the ejection port, the release, the handle.
    bool shellTesting = false;
    float shellTestGlove[12];
    const shellload::Result shellOut = TubeByHand(t, headPose, fromHead, tracked, trackedLeft, gunLeftHands, frameSeconds, &shellTesting, shellTestGlove);
    if (shellTesting) {
        std::memcpy(t.leftPose, shellTestGlove, sizeof(t.leftPose));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        t.left = true;
    }
    PistolRounds(pistolHeld);
    PistolReload(t, pistolHeld);
    const pistolmag::Result pistolMagazine =
        PistolMagazine(t, magazine, pistolHeld, trackedLeft, tracked, palm, toView, frameSeconds);
    PistolSlide(t, pistolHeld, trackedLeft, tracked,
                g_lining                                       ? "the left glove is being lined up"
                : magazine.holding                             ? "the left hand carries a magazine"
                : pistolMagazine.holding                       ? "the left hand has the pistol's magazine"
                : GetTickCount64() - g_pistolSeatedAt < 300    ? "the pistol's magazine has just been pushed home"
                : !SlideHeld() && input::RightTrigger() > 0.6f ? "the right trigger is pulled (a squeeze with the shot)"
                                                               : nullptr,
                frameSeconds);
    const bool holding = (magazine.holding && magazine.side == 0) || (heldMag.holding && heldMag.side == 0);
    // Either magazine in the left hand keeps it busy: no handguard, no pushing off the gun, no bolt (all the M416's,
    // left out with the train pistol in hand anyway, but its "in hand" drops for a moment now and then).
    // By hand (free hands: the right's own magazine, a held gun's magazine or box in it, or the launcher's round or barrel in the right hand at work).
    const bool busyBy[2] = {holding || pistolMagazine.holding || (m320Out.holding && m320Out.side == 0) ||  // (a round or the launcher's barrel in the hand alike)
                                (shellOut.holding && shellOut.side == 0),                                    // (a shotgun's shell, or its bolt handle)
                            (magazine.holding && magazine.side == 1) || (heldMag.holding && heldMag.side == 1) || (m320Out.holding && m320Out.side == 1) ||
                                (shellOut.holding && shellOut.side == 1)};
    const bool busyWithMag = busyBy[0];
    // (Free hands, next frame: a busy hand doesn't take the rifle up.)
    g_busyWasBy[0] = busyBy[0];
    g_busyWasBy[1] = busyBy[1];
    // Both hands on the gun: the hold (Support) and the lines the steer turns the gun by, the glove on
    // the handguard. Not with the train pistol in hand (The Great Destroyer: PistolSupport below): the
    // M416's handguard, unseen, would take the left hand and steer the aim.
    // Nor with the launcher: the same handguard is what sent its aim haywire.
    const bool smawHeld = skin::LauncherInHand();
    const bool launcherHeld = smawHeld || heldGun;
    // The launcher loaded by the left hand and its front grip (rocket.cpp, LauncherSupport). Its case goes first: the squeeze that
    // takes one over the shoulder doesn't also take the grip. Any other gun is held the same way, front grip and all, and has no case.
    const bool launcherHands = launcherHeld && skin::LauncherHands();
    const bool smawHands = smawHeld && skin::LauncherHands();
    // (Through a switch's frames with no gun drawn: a launcher-pose gun's front grips stay held as they are, none let go of for its not being drawn;
    // nor is the free right glove kept out of a gun not known here, nor the M416's charging handle worked on one.)
    const bool lpBridge = g_bridging && lpNow;
    const bool launcherKept = launcherHands || lpBridge;
    ammo::SetLauncherByHand(smawHands);
    if (t.left && !launcherKept && g_onLauncher) {
        Log("Hands: the left hand let go of the launcher's front grip (no launcher)");
        g_onLauncher = false;
    }
    if (skin::TakeRocketGameTest())
        LauncherGameReload();
    const int rocketTest = smawHands && !trackedLeft ? skin::RocketTest() : 0;
    bool rocketTesting = false;
    float rocketTestGlove[12];
    rocket::Result rocketOut;
    {
        static float s_launcherPose[12] = {};
        if (t.gun)
            std::memcpy(s_launcherPose, t.launcherPose, sizeof(s_launcherPose));
        rocket::Frame rf;
        rf.dt = frameSeconds;
        rf.launcher = smawHands;
        std::memcpy(rf.launcherPose, s_launcherPose, sizeof(rf.launcherPose));
        std::memcpy(rf.headPose, headPose, sizeof(rf.headPose));
        rf.left = trackedLeft && !g_lining;
        std::memcpy(rf.glove, tracked, sizeof(rf.glove));
        rf.squeeze = std::fmax(input::LeftSqueeze(), input::LeftTrigger());
        rf.haveBody = pouch::BodyFrame(rf.neck, rf.yaw);
        // By the game's count, also while it's put away: it comes back as it was. A rocket in it when that can't be read.
        int rockets = 1, spareRockets = 0;
        const bool counted = skin::LauncherHands() && ammo::LauncherGet(rockets, spareRockets);
        rf.loaded = !counted || rockets > 0;
        rf.spare = spareRockets > 0;
        // Its count at none with no hidden round in it: the game reloads it by itself (X held and put away before it was done, then raised again).
        if (!smawHands)
            g_gameReloadGaveUp = false;
        if (smawHands && !g_launcherGameReload && !g_gameReloadGaveUp && counted && spareRockets > 0 && ammo::LauncherAtNone()) {
            g_launcherGameReload = true;
            g_gameReloadSince = GetTickCount64();
            g_gameReloadCountedAt = 0;
            Log("Rocket: the launcher's count is at none: the game's own reload");
        }
        // Any other gun: the game's own moves on it, its reload above all, are followed for as long as its hands are off their rest.
        static bool s_heldBefore = false;
        if (heldGun) {
            g_launcherGameReload = skin::HeldGameMoving();
        } else if (s_heldBefore) {
            g_launcherGameReload = false;
        } else if (g_launcherGameReload) {
            // Over when its animation is, not when its count comes up: the hands and the case jumped home with 1.9 s of it to go.
            const ULONGLONG now = GetTickCount64();
            if (counted && rockets > 0 && !g_gameReloadCountedAt)
                g_gameReloadCountedAt = now;
            const bool done = g_gameReloadCountedAt && now - g_gameReloadCountedAt > kGameReloadTail;
            const bool neverCame = !g_gameReloadCountedAt && now - g_gameReloadSince > kGameReloadMost;
            if (!counted || !smawHands || done || neverCame) {
                g_launcherGameReload = false;
                g_gameReloadGaveUp = neverCame;
                Log("Rocket: the game's own reload is %s", neverCame ? "not coming: the hand can load it" : "over");
            }
        }
        s_heldBefore = heldGun;
        rf.gameReload = g_launcherGameReload;
        // Free hands: the left on its pistol grip takes nothing; let go of (it lies, or flies into a hand), nothing is caught at its back or taken off it.
        // Put away at its place (holsters): no new one at the belly either, for a launcher drawn nowhere.
        rf.handFree = !g_leftOnGrip && !g_leftSpent;
        rf.held = !gunLeftHands;
        rf.stowed = gunStowed;
        // weapon.txt "rockettest": a scripted left hand, with nobody in the headset.
        static float s_testTime = 0.0f;
        XrPosef standInHand;
        if (rocketTest && input::StandInLeftPose(standInHand)) {
            MakeLeftOffset();
            float controller[12], glove[12], middle[3];
            pose(standInHand, controller);
            Compose(controller, g_leftOffset, glove);
            Compose(fromHead, glove, rocketTestGlove);
            s_testTime = rf.loaded ? 0.0f : s_testTime + frameSeconds;
            const float t2 = s_testTime;
            auto lerp = [](const float* a, const float* b, float s, float* out) {
                s = s * s * (3.0f - 2.0f * s);
                for (int i = 0; i < 3; ++i)
                    out[i] = a[i] + (b[i] - a[i]) * s;
            };
            auto part = [&](float from, float to) { return std::fmin(std::fmax((t2 - from) / (to - from), 0.0f), 1.0f); };
            // The script, the hand's middle in the view's space. "rockettest 4": from the chest onto the spent case, a roll of the wrist, pulled off
            // and let go of; to the shoulder for a new one, to the launcher's back; "rockettest 5" the same but no roll, a pull alone, a push alone.
            const float rest[3] = {-0.20f, -0.22f, -0.42f}, shoulder[3] = {-0.18f, -0.02f, 0.06f};
            const float onCaseFrame[3] = {0.02f, -0.02f, -0.46f}, offCaseFrame[3] = {0.02f, -0.02f, -0.86f}, nearBackFrame[3] = {0.10f, 0.10f, -0.32f};
            float onCase[3], offCase[3], nearBack[3], at[3], next[3];
            Transform(s_launcherPose, onCaseFrame, onCase);
            Transform(s_launcherPose, offCaseFrame, offCase);
            Transform(s_launcherPose, nearBackFrame, nearBack);
            std::memcpy(at, rest, sizeof(at));
            float squeezeNow = 0.0f, roll = 0.0f, spin = 0.0f;
            if (rocketTest >= 4) {
                // "rockettest 6": the hand on the spent case, staying there. "7": a new one brought in front of the view, away from the launcher and turned about, to
                // look at the grip from every side; "8": the same palm up, to see the thumb.
                const bool byWrist = rocketTest == 4 || rocketTest >= 7;
                lerp(rest, onCase, part(1.0f, 2.0f), at);
                squeezeNow = t2 > 2.1f && (t2 < 4.4f || rocketTest == 6) ? 1.0f : 0.0f;
                if (byWrist)
                    roll = 0.30f * part(2.4f, 2.9f);
                if (rocketTest != 6) {
                    constexpr float kInView[3] = {-0.14f, -0.04f, -0.42f};
                    lerp(at, offCase, part(3.1f, 4.2f), next);
                    std::memcpy(at, next, sizeof(at));
                    lerp(at, shoulder, part(4.5f, 5.4f), next);
                    std::memcpy(at, next, sizeof(at));
                    if (t2 > 5.6f)
                        squeezeNow = t2 < 11.0f || rocketTest >= 7 ? 1.0f : 0.0f;
                    lerp(at, rocketTest >= 7 ? kInView : nearBack, part(5.9f, 7.0f), next);
                    std::memcpy(at, next, sizeof(at));
                    if (rocketTest == 4)
                        roll += 0.30f * part(8.2f, 8.6f);
                    if (rocketTest >= 7)
                        spin = std::fmax(t2 - 7.0f, 0.0f);
                }
                rf.forceTake = true;
            } else if (rocketTest == 1) {  // a new one held still in front of the chest, as soon as the launcher's empty
                squeezeNow = t2 > 2.0f ? 1.0f : 0.0f;
                rf.forceTake = true;
            }
            const float hand[3] = {0.537f, 1.098f, 0.207f};
            auto turnGlove = [&](const float* axis, float angle) {
                const float c = std::cos(angle), s = std::sin(angle), u = 1.0f - c;
                const float r[9] = {u * axis[0] * axis[0] + c,           u * axis[0] * axis[1] - s * axis[2], u * axis[0] * axis[2] + s * axis[1],
                                    u * axis[0] * axis[1] + s * axis[2], u * axis[1] * axis[1] + c,           u * axis[1] * axis[2] - s * axis[0],
                                    u * axis[0] * axis[2] - s * axis[1], u * axis[1] * axis[2] + s * axis[0], u * axis[2] * axis[2] + c};
                float turned[12];
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j)
                        turned[i * 4 + j] = r[i * 3] * rocketTestGlove[j] + r[i * 3 + 1] * rocketTestGlove[4 + j] + r[i * 3 + 2] * rocketTestGlove[8 + j];
                    turned[i * 4 + 3] = rocketTestGlove[i * 4 + 3];
                }
                std::memcpy(rocketTestGlove, turned, sizeof(turned));
            };
            if (roll != 0.0f) {  // the wrist rolled about the launcher's line
                const float axis[3] = {s_launcherPose[2], s_launcherPose[6], s_launcherPose[10]};
                turnGlove(axis, roll);
            }
            if (spin != 0.0f) {  // turned about in front of the view
                const float up[3] = {0.0f, 1.0f, 0.0f}, across[3] = {1.0f, 0.0f, 0.0f}, forward[3] = {0.0f, 0.0f, 1.0f};
                if (rocketTest == 8)
                    turnGlove(forward, 2.6f);
                turnGlove(up, spin * 0.75f);
                turnGlove(across, 0.55f * std::sin(spin * 0.45f));
            }
            Transform(rocketTestGlove, hand, middle);
            for (int i = 0; i < 3; ++i)
                rocketTestGlove[i * 4 + 3] += at[i] - middle[i];
            rf.left = true;
            std::memcpy(rf.glove, rocketTestGlove, sizeof(rf.glove));
            rf.squeeze = squeezeNow;
            rocketTesting = true;
            // "rockettest 3": the hand on the launcher's front grip, by the same way a squeeze takes it.
            if (rocketTest == 3) {
                g_onLauncher = true;
                rf.left = false;
                rf.forceTake = false;
                rf.squeeze = 0.0f;
            }
        } else {
            s_testTime = 0.0f;
        }
        // Its case's fall is loose's (the real ground, a minute there) whenever loose is stepping; else its own as before.
        rocket::SetFallOwn(!(skin::LooseOn() && g_haveLooseFrame));
        rocketOut = rocket::Update(rf);
        g_smawCaseOnNow = rocketOut.caseOn;  // (the launcher let go of this frame lies with its case or bare, as it is now)
        if (rocketOut.fell && skin::LooseOn() && g_haveLooseFrame) {
            float room[12];
            Compose(rf.headPose, rocketOut.fellPose, room);
            QueueLoose(kLooseSmawCase, loose::SpecFor("smaw"), room, rocketOut.fellVelocity, rocketOut.fellFromHand);
        }
        if (rocketOut.spent)
            Log("Rocket: fired; the spent case stays on for the hand (%d spare)", spareRockets);
        if (rocketOut.squeezed)
            Log("Rocket: the left grip squeezed %.0f cm right, %.0f cm up, %.0f cm forward of the neck: %s", rocketOut.fromNeck[0] * 100.0f,
                rocketOut.fromNeck[1] * 100.0f, rocketOut.fromNeck[2] * 100.0f,
                rocketOut.grabbed ? "the spent case taken" : rocketOut.took ? "a rocket taken" : rocketOut.note);
        if (rocketOut.grabbed || rocketOut.took)
            g_freshSqueeze = false;
        if (rocketOut.took)
            sounds::Play("rocket_take");
        if (rocketOut.unlocked) {
            Log("Rocket: the spent case turned open");
            sounds::Play("rocket_unlock");
        }
        if (rocketOut.freed) {
            Log("Rocket: the spent case off the launcher, in the hand");
            sounds::Play("rocket_out");
        }
        if (rocketOut.caught) {
            Log("Rocket: caught at the launcher's back");
            sounds::Play("rocket_catch");
        }
        if (rocketOut.offLine)
            Log("Rocket: pulled off the launcher's line");
        if (rocketOut.letGo)
            Log("Rocket: let go of: it falls");
        if (rocketOut.putBack)
            Log("Rocket: put back at the belly");
        if (rocketOut.cancelled)
            Log("Rocket: let go of by itself (the launcher went, the hand was lost a second, or the game reloaded it)");
        if (rocketOut.homing)
            sounds::Play("rocket_in");
        if (rocketOut.seated) {
            ammo::LauncherInsert();
            sounds::Play("rocket_lock");
            g_freshSqueeze = false;
            g_rocketSeatedAt = GetTickCount64();
        }
        g_launcherEmptyNow = rocketOut.empty && smawHands;
    }
    if (rocketTesting) {
        std::memcpy(t.leftPose, rocketTestGlove, sizeof(t.leftPose));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        t.left = true;
    }
    // A case in the hand lets go of the front grip: reaching back past it with the grip squeezed took the grip, then the case, and the glove stayed on the grip with the case
    // floating where the hand really was (the review, 2026-11-04).
    // (A held gun's magazine, box, lever, lid or belt: in the hand the module serves, either.)
    const bool heldWorkBy[2] = {heldMag.side == 0 && (heldMag.holding || heldMag.fist || heldMag.took || heldMag.grabbed),
                                heldMag.side == 1 && (heldMag.holding || heldMag.fist || heldMag.took || heldMag.grabbed)};
    const bool caseInHand = rocketOut.holding || rocketOut.took || rocketOut.grabbed || heldWorkBy[0];
    // (Free hands, next frame: the left with a case, a rocket, a magazine, the lever fist or the aerial in it doesn't take a gun up; the right with a held
    // gun's magazine or lever in it neither.)
    g_busyWasBy[0] = g_busyWasBy[0] || caseInHand || heldFlap.holding;
    g_busyWasBy[1] = g_busyWasBy[1] || heldWorkBy[1];
    // (Holsters, next frame: what each hand holds, for a draw by an empty hand only and a put-away refused while the other holds a rocket, a case or a
    // 40 mm round, which ride the gun's own draws.)
    for (int s = 0; s < 2; ++s) {
        g_holsterMagBy[s] = (magazine.holding && magazine.side == s) || (s == heldMag.side && heldMag.holding);
        g_holsterRoundBy[s] = m320Out.handRound && m320Out.handSide == s;
        g_holsterRocketBy[s] = s == 0 && rocketOut.holding;
    }
    // Free hands: the hand that may hold the handguard (the one not on the pistol grip; the one holding the rifle by its handguard alone, or still on it while the
    // other takes the pistol grip), whether that hold points the rifle, whether it's what holds the rifle up, and the trigger hand it steers from. Dormant, or held by
    // the right: the left, from the right hand, as before.
    const grasp::Result& grasped = g_graspNow;
    const bool heldByGrip = grasped.holder == grasp::Holder::kHeld;
    // (A regrip's glide: the hand on the handguard steers from the hand taking the pistol grip already, as it will once the rifle's there: no swing at the arrival.)
    const bool regripGlide = grasped.holder == grasp::Holder::kGlide && !grasped.glideFromWorld && grasped.trigger >= 0 && grasped.barrel >= 0;
    const int supportSide = heldByGrip ? 1 - grasped.trigger : grasped.barrel;
    const bool barrelHolder = grasped.holder == grasp::Holder::kBarrel;
    const bool rightTrigger = heldByGrip || regripGlide ? grasped.trigger == 1 : true;
    const float* triggerTurn = rightTrigger ? (launcherHands ? launcherTurn : rifleTurn) : g_rifleTurnBy[0];
    const float* triggerAnchor = rightTrigger ? (launcherHands ? launcherGrip : anchor) : g_anchorBy[0];
    const float* triggerAt = rightTrigger ? t.rightWrist : g_atBy[0];
    // The barrel hand's arming (g_barrelArmed): its grip unread, the hand unseen, a scripted moment or the first frame after a gap disarm it; a squeeze
    // past grasp::kTake while read and seen arms it again (a hand on the handguard took hold so: armed). Holsters: a hand holding the gun that the runtime
    // doesn't see now (raw, input's 0.5 s held place included) is disarmed too, so its front is never let go of then (the user's rule: a tracking loss
    // never drops a gun), nor at once once it's seen again with its grip open.
    {
        const bool scripted = g_scripted.load(std::memory_order_relaxed);
        for (int s = 0; s < 2; ++s) {
            const bool seen = s == 0 ? t.left : g_rightSeen;
            if (!ReadOf(s) || !seen || scripted || (g_freeResumed && !g_test[s].on) || g_holsterHold[s])
                g_barrelArmed[s] = false;
            else if (SqueezeOf(s) >= grasp::kTake)
                g_barrelArmed[s] = true;
        }
    }
    // The launcher's or a held gun's front grip (LauncherSupport), by the hand that may hold it (supportSide): the left, as before, while the right holds the
    // pistol grip; the right while the left does. Held by its front grip alone and the hand not seen, the hold stays till it's back. (arms.txt "test
    // guard": the stand-in's left has no grip button, so its squeeze doesn't let go, as the handguard's below doesn't. It did: always none, it let go each
    // frame before recoil read the hold and the guard took it again after, "hold 0.00" on every held gun and the SMAW.)
    if (launcherHands && supportSide == 0 && t.gun && t.left && !g_lining && !caseInHand && !rocketTesting && !heldTesting) {
        LauncherSupport(t, 0, triggerTurn, triggerAnchor, triggerAt, launcherFront, heldByGrip || regripGlide, barrelHolder, headTurn);
    } else if (g_onLauncher && rocketTest != 3) {
        if (barrelHolder && supportSide == 0 && launcherKept && !t.gun && !g_lining) {
            // (held by the left's front grip alone, the hand not seen: the hold stays till it's back)
        } else if (!launcherKept || !t.gun || g_lining || caseInHand || supportSide != 0 ||
                   (SqueezeOf(0) < 0.4f && !g_armTests.guard &&
                    (!barrelHolder || (ReadOf(0) && t.left && !g_scripted.load(std::memory_order_relaxed) && g_barrelArmed[0])))) {
            g_onLauncher = false;
            g_freshSqueeze = SqueezeOf(0) < 0.4f;
        }
    }
    if (launcherHands && supportSide == 1 && t.gun && g_rightSeen && !heldWorkBy[1]) {  // (not with a held gun's magazine or lever in it, as the left's)
        LauncherSupport(t, 1, triggerTurn, triggerAnchor, triggerAt, g_profile.support[1], heldByGrip || regripGlide, barrelHolder, headTurn);
    } else if (g_onLauncherBy[1]) {
        if (barrelHolder && supportSide == 1 && launcherKept && !t.gun) {
            // (held by it alone, the hand not seen: the hold stays)
        } else if (!launcherKept || !t.gun) {
            Log("Hands: the right hand let go of the launcher's front grip (no launcher)");
            g_onLauncherBy[1] = false;
        } else if (supportSide != 1) {
            Log("Hands: the right hand let go of the launcher's front grip (it has the pistol grip)");
            g_onLauncherBy[1] = false;
        } else if (SqueezeOf(1) < 0.4f && (!barrelHolder || (ReadOf(1) && g_rightSeen && !g_scripted.load(std::memory_order_relaxed) && g_barrelArmed[1]))) {
            Log("Hands: the right hand let go of the launcher's front grip (the grip at %.2f)", SqueezeOf(1));
            g_onLauncherBy[1] = false;
            g_freshBy[1] = false;
        }
    }
    if (supportSide == 0 && t.gun && t.left && !g_lining && !busyWithMag && !pistolHeld && !launcherHeld) {
        Support(t, 0, triggerTurn, triggerAnchor, triggerAt, heldByGrip || regripGlide, barrelHolder, headTurn);
    } else {
        // Without the left controller's place the hold stays (the gun stays as the hands last had it
        // for a moment, then eases back to the right hand's aim; the glove on the handguard); its grip
        // button still lets go. Without the gun, nothing to hold.
        if (g_supporting && (!t.gun || g_lining || pistolHeld || launcherHeld)) {
            if (barrelHolder && supportSide == 0 && !t.gun && !g_lining && !pistolHeld && !launcherHeld) {
                // (free hands: the rifle held by this hand's handguard alone and the hand not seen: the hold stays till it's back, nothing changes hands)
            } else {
                Log("Hands: the left hand let go of the handguard (no gun)");
                g_supporting = false;
            }
        } else if (g_supporting && SqueezeOf(0) < 0.4f && !g_armTests.guard &&
                   (!(barrelHolder && supportSide == 0) || (ReadOf(0) && t.left && !g_scripted.load(std::memory_order_relaxed) && g_barrelArmed[0]))) {
            Log("Hands: the left hand let go of the handguard");
            g_supporting = false;
            g_freshSqueeze = false;
        } else if (g_supporting && supportSide != 0) {
            Log("Hands: the left hand let go of the handguard (free hands: it isn't the hand that can hold it now)");
            g_supporting = false;
        }
    }
    // Free hands: the right on the handguard (or the launcher's grip) while the left holds the pistol grip (or the right holding the rifle up by it), as the left's
    // above; not while it has a magazine or the launcher's round or barrel.
    if (supportSide == 1 && t.gun && g_rightSeen && !pistolHeld && !launcherHeld && !busyBy[1]) {
        Support(t, 1, triggerTurn, triggerAnchor, triggerAt, heldByGrip || regripGlide, barrelHolder, headTurn);
    } else if (g_supportingBy[1]) {
        const float squeeze = SqueezeOf(1);
        const char* const what = g_onM320By[1] ? "grenade launcher's grip" : "handguard";
        if (barrelHolder && supportSide == 1 && !t.gun && !pistolHeld && !launcherHeld) {
            // (the rifle held by it alone and the hand not seen: the hold stays)
        } else if (!t.gun || pistolHeld || launcherHeld) {
            Log("Hands: the right hand let go of the %s (no gun)", what);
            g_supportingBy[1] = false;
            g_onM320By[1] = false;
        } else if (supportSide != 1) {
            Log("Hands: the right hand let go of the %s (it has the rifle's pistol grip)", what);
            g_supportingBy[1] = false;
            g_onM320By[1] = false;
        } else if (squeeze < 0.4f && (!barrelHolder || (ReadOf(1) && g_rightSeen && !g_scripted.load(std::memory_order_relaxed) && g_barrelArmed[1]))) {
            Log("Hands: the right hand let go of the %s (the grip at %.2f)", what, squeeze);
            g_supportingBy[1] = false;
            g_onM320By[1] = false;
            g_freshBy[1] = false;
        }
    }
    // arms.txt "test guard", nobody in the headset: the stand-in's left hand takes hold where it is and points the gun, as Support has it ("test move": the real hand moved, not the glove).
    // A launcher-pose gun's (a held gun's front, the SMAW's front grip) alike, as LauncherSupport has it, while the guard holds it and nothing else is in that hand; its place on that
    // gun its own (taken again for another such gun), the M416's as before.
    static bool s_guardHeld = false;
    static float s_guardFront[3] = {};
    static uint32_t s_guardFrontFor = 0;  // the launcher-pose gun the guard's place is on (0 none)
    const bool guardFront = launcherHands && g_onLauncher && skin::LauncherSteer() && !caseInHand && !rocketTesting && !heldTesting;
    if (XrPosef standIn; heldByGrip && grasped.trigger == 1 && g_armTests.guard && t.gun && !trackedLeft && !pistolHeld && (!launcherHands || guardFront) &&
                         !g_holdOutOfReach && input::StandInLeftPose(standIn)) {
        MakeLeftOffset();
        float controller[12], glove[12], hand[12], wrist[3], inverse[12], inGun[3];
        pose(standIn, controller);
        Compose(controller, g_leftOffset, glove);
        Compose(fromHead, glove, hand);
        if (g_armTests.move)
            for (int i = 0; i < 3; ++i)
                hand[i * 4 + 3] += g_armTests.moveBy[i];
        Transform(hand, g_leftAnchor, wrist);
        InvertRigid(launcherHands ? t.launcherPose : t.gunPose, inverse);
        Transform(inverse, wrist, inGun);
        float* const point = launcherHands ? s_guardFront : g_supportPoint;
        if (launcherHands ? s_guardFrontFor != g_profile.id : !s_guardHeld)
            std::memcpy(point, inGun, sizeof(inGun));
        if (launcherHands)
            s_guardFrontFor = g_profile.id;
        else
            s_guardHeld = true;
        const float apart[3] = {wrist[0] - t.rightWrist[0], wrist[1] - t.rightWrist[1], wrist[2] - t.rightWrist[2]};
        // (The trigger hand's turn and place: the right's, as it holds this gun: the M416's rifleTurn and anchor, a launcher-pose gun's its own.)
        const float model[3] = {point[0] - triggerAnchor[0], point[1] - triggerAnchor[1], point[2] - triggerAnchor[2]};
        GiveSteer(model, apart, Length(apart), triggerTurn, headTurn);
    } else if (!g_armTests.guard) {
        s_guardHeld = false;
        s_guardFrontFor = 0;
    }
    // The gun as the hands point it (steer.h), a turn in its own model space: onto the hands' line only
    // while it's near where the right hand points the gun, eased in and out; the game aims and fires
    // along it (AimOrientation).
    g_aimTurned = false;
    g_regripDrawn = false;
    // Free hands: only while a hand holds the pistol grip (none while the rifle's held by its handguard alone, or flies into a hand from the world); by the left's,
    // the gun turned about its own place on the gun (the right's own aim and the launcher only by the right's, as before). A regrip's glide steers on as it will
    // once the rifle's arrived: where it goes (g_glideTo) and where it's drawn on the way (g_glideDrawn), the modules keeping the barrel hand's hold.
    if (t.gun && (heldByGrip || regripGlide)) {
        const int gunHand = grasped.trigger, other = 1 - gunHand;
        g_steerIn.dt = frameSeconds;
        if (!g_steerByBody) {  // weapon.txt "steer 35 65": the cone round the right hand's line instead
            g_steerIn.fullAngle = g_steerFull.load(std::memory_order_relaxed);
            g_steerIn.noneAngle = g_steerNone.load(std::memory_order_relaxed);
        }
        // The left controller lost for a moment while holding: the gun stays as the hands last had it.
        if ((g_supportingBy[other] || (g_onLauncherBy[other] && skin::LauncherSteer())) && !(other == 0 ? trackedLeft : g_rightSeen) &&
            GetTickCount64() - g_seenAtBy[other] < kCoast)
            g_steerIn.holding = g_steerIn.coast = true;
        // Likewise a launcher-pose gun's front grip held through a switch's frames with no gun drawn (nothing steers it then): it's back on the hands' line.
        if (lpBridge && g_onLauncherBy[other] && skin::LauncherSteer())
            g_steerIn.holding = g_steerIn.coast = true;
        const steer::Output s = steer::Update(g_steerIn, g_steer);
        if (regripGlide) {
            // Where it goes: the taking hand's own place turned by the steer (as kHeld will place it), from the barrel hand's frozen hold (t.gunPose here)
            // as that hand moves now; kept as the glide's start should that hand be lost.
            if (s.weight > 0.0f) {
                const XrQuaternionf q{s.turn.x, s.turn.y, s.turn.z, s.turn.w};
                float local[9], turned[9];
                QuatToMatrix(q, local);
                Mul3(g_rifleTurnBy[gunHand], local, turned);
                PlaceAt(turned, g_anchorBy[gunHand], g_atBy[gunHand], g_glideTo);
            } else {
                std::memcpy(g_glideTo, g_handGun[gunHand], sizeof(g_glideTo));
            }
            Compose(headPose, gunNow, g_glideFrom);
            BlendAbout(gunNow, g_glideTo, grasped.glide, g_anchorBy[gunHand], g_glideDrawn);
            g_regripDrawn = Finite(g_glideTo, 12) && Finite(g_glideDrawn, 12);
            NoteJump(nullptr, aim, rightGrip, 0.0f);
            g_haveM320GunRoom = false;  // (nothing while it flies into a hand, or the launcher would read the glide as a flick)
        } else if (s.weight > 0.0f && gunHand == 0) {
            const XrQuaternionf q{s.turn.x, s.turn.y, s.turn.z, s.turn.w};
            float local[9], turned[9];
            QuatToMatrix(q, local);
            Mul3(g_rifleTurnBy[0], local, turned);
            PlaceAt(turned, g_anchorBy[0], g_atBy[0], gunNow);
        } else if (s.weight > 0.0f) {
            const XrQuaternionf q{s.turn.x, s.turn.y, s.turn.z, s.turn.w};
            float local[9], turned[9];
            QuatToMatrix(q, local);
            Mul3(rifleTurn, local, turned);
            PlaceAt(turned, anchor, t.rightWrist, t.gunPose);
            Mul3(launcherTurn, local, turned);
            PlaceAt(turned, launcherGrip, t.rightWrist, t.launcherPose);
            // The aim pose's axes are the gun model's turned half a turn about y (flip above), so the
            // same turn of the aim has its x and z the other way.
            // The launcher's tube is raised from the gun's turn (launcherTurn) and turned after that, so its aim is turned by the same turn as seen from the raise; or the iron
            // sights go off the aim as the left hand steers, by all of the 0.7 degrees with the hands' line 90 degrees off the right hand's (the review, 2026-11-04).
            XrQuaternionf inAim = q;
            {
                const float halfUp = (launcherHands ? skin::LauncherUp() : skin::RifleUp()) * 3.14159265f / 360.0f;
                XrQuaternionf raised{std::sin(halfUp), 0.0f, 0.0f, std::cos(halfUp)}, lowered{-std::sin(halfUp), 0.0f, 0.0f, std::cos(halfUp)};
                // (A held gun's sights' line right of its tube, the AT4's: its turn about y with the raise, as SightsTurn has it; none for any other.)
                if (const float right = launcherHands && heldGun ? held.right : 0.0f; right != 0.0f && std::isfinite(right)) {
                    const float halfRight = right * 3.14159265f / 360.0f;
                    raised = QuatMul(XrQuaternionf{0.0f, -std::sin(halfRight), 0.0f, std::cos(halfRight)}, raised);
                    lowered = QuatMul(lowered, XrQuaternionf{0.0f, std::sin(halfRight), 0.0f, std::cos(halfRight)});
                }
                inAim = QuatMul(QuatMul(raised, q), lowered);
            }
            g_aimTurn = QuatMul(aim.orientation, XrQuaternionf{-inAim.x, inAim.y, -inAim.z, inAim.w});
            g_aimTurned = true;
        }
        if (!regripGlide) {
            float gunRoom[12];
            Compose(headPose, t.gunPose, gunRoom);
            if (gunHand == 1) {
                NoteSteer(s, g_steerIn.holding && !g_steerIn.coast, g_steerIn.apart, input::LeftSqueeze());
                NoteJump(gunRoom, aim, rightGrip, s.weight);
                NoteGunRough(gunRoom, s.weight, g_steerIn.apart);
            } else {
                NoteJump(nullptr, aim, rightGrip, 0.0f);  // (the right controller's jumps: not the left-held gun's)
            }
            std::memcpy(g_m320GunRoom, gunRoom, sizeof(gunRoom));  // (the grenade launcher's next frame: its turn and speed tip and flick it)
            g_haveM320GunRoom = true;
        }
    } else {
        g_steer = steer::State{};  // no gun to ease back
        NoteJump(nullptr, aim, rightGrip, 0.0f);
        // (Free hands: held by its handguard alone, as it's drawn; nothing while it flies into a hand, or the launcher would read the glide as a flick.)
        g_haveM320GunRoom = t.gun && grasped.holder == grasp::Holder::kBarrel;
        if (g_haveM320GunRoom)
            Compose(headPose, t.gunPose, g_m320GunRoom);
    }
    g_steerIn = steer::Input{};
    // Free hands: the trigger hand's glove on the pistol grip rides the rifle as steered (kHeld) or where a glide takes it: that, relative to the hand's own place for
    // the rifle, kept for its let-go (the glove opens where it was drawn, then goes with the hand; it had turned by the whole steer in one frame).
    if (g_freeOn && (heldByGrip || grasped.holder == grasp::Holder::kGlide) && grasped.trigger >= 0 && g_haveHandGun[grasped.trigger]) {
        const int gripHand = grasped.trigger;
        const float* rides = heldByGrip ? (t.gun ? gunNow : nullptr) : GlideTarget(gripHand);
        if (rides) {
            float inverse[12];
            InvertRigid(g_handGun[gripHand], inverse);
            Compose(inverse, rides, g_gripRel[gripHand]);
            g_haveGripRel[gripHand] = Finite(g_gripRel[gripHand], 12);
        }
    }
    // ---- The kick (recoil, design 6: recoilrun.h): the gun in hand as the hands hold it, turned and slid by this frame's recoil, from its own numbers.
    // Everything below draws, aims and fires from the kicked gun: the gloves ride it, the arms are solved from them, the look is written from its line (the
    // right hand's own aim turned with it), the bullets start on its sights. What judges what the hands do (a glove's reach, the gloves kept out of the gun,
    // the slap, the handle, the loose things, the grasp's memory) reads AsHeld, the pose from before it. With recoil off, nothing held, or before the first
    // shot the move is none and nothing below changes.
    std::memcpy(g_heldGun, t.gunPose, sizeof(g_heldGun));
    std::memcpy(g_heldLauncher, t.launcherPose, sizeof(g_heldLauncher));
    std::memcpy(g_heldPistol, t.pistolPose, sizeof(g_heldPistol));
    g_kicked = 0;
    {
        recoilrun::In kin;
        kin.dt = frameSeconds;
        kin.gap = frameGap;
        // Which gun, by what's drawn (not by the profile: a gun free hands don't take is drawn all the same, and kicks): the train pistol, a launcher-pose gun
        // (the SMAW, a held gun, the Stinger), the M416. Its slot: the SMAW's 5, the Stinger's 1, the train pistol's 0; the M416 and a held gun in ammo's
        // slot of the gun in the hands (ammo::HandSlot: a gun taken up from the ground is wherever the game put it), whatever free hands make of the gun.
        const bool smawDrawn = launcherHands && smawHeld && !heldGun;
        const bool stinger = launcherHands && heldGun && gunprofile::RowOf(held.shape) == gunprofile::kStingerRow;
        const bool at4 = launcherHands && heldGun && gunprofile::RowOf(held.shape) == gunprofile::kAt4Row;
        kin.pose = pistolHeld ? 3 : launcherHands ? 2 : in.rifle ? 1 : 0;
        kin.id = kin.pose == 3 ? recoilrun::kTrainPistol : kin.pose == 2 ? (heldGun ? held.id : gunprofile::kSmawId) : kin.pose == 1 ? gunprofile::kRifleId : 0;
        kin.shape = kin.pose == 2 && heldGun ? held.shape : nullptr;
        // (The AT4's slot by its name: the scene's kit puts it in slot 0, whatever the hands' slot says; -1 not found this frame.)
        kin.slot = kin.pose == 0 ? -1 : smawDrawn ? 5 : stinger ? 1 : at4 ? ammo::At4Slot() : kin.pose == 3 ? 0 : ammo::HandSlot();
        // (A gun the game swapped out of its slot (pickups.h, NoteSwaps) while it's still drawn: that slot holds the new gun's data already, so it's no gun
        // for recoilrun till what's drawn changes, 2 s at most.)
        if (g_pick.kickOutId && (kin.id != g_pick.kickOutId || GetTickCount64() - g_pick.kickOutAt > 2000))
            g_pick.kickOutId = 0;
        if (g_pick.kickOutId)
            kin.pose = 0, kin.id = 0, kin.shape = nullptr, kin.slot = -1;
        g_kickId = kin.id, g_kickSlot = kin.slot;
        float* drawn = kin.pose == 3 ? t.pistolPose : kin.pose == 2 ? t.launcherPose : t.gunPose;
        const float* heldPose = kin.pose == 3 ? g_heldPistol : kin.pose == 2 ? g_heldLauncher : g_heldGun;
        const bool heldFinite = Finite(heldPose, 12);
        // Held by its pistol grip (by either hand; free hands dormant: the right), where a hand has it, no scripted moment: else the kick clears.
        kin.held = kin.pose != 0 && t.gun && heldFinite && !g_scripted.load(std::memory_order_relaxed) && (pistolHeld || !FreshFreeHands() || heldByGrip);
        kin.leftTrigger = !pistolHeld && heldByGrip && grasped.trigger == 0;
        // Two hands: the support hand's glove on the gun's front as far as it's eased onto it (last frame's blend: this frame's is worked out further down), none
        // while it's off the gun out of reach. The left while the right has the pistol grip (or free hands are dormant), the right while the left has it, the
        // left in the train pistol's two-handed hold. Only a front grip (the grenade launcher's, the SMAW's, the Stinger's, the AT4's) buzzes as one, not a handguard.
        if (pistolHeld) {
            kin.hands = g_pistolBlend;
            kin.support = g_pistolBlend > 0.0f ? 0 : -1;
        } else if (kin.leftTrigger) {
            const bool on = (launcherHands ? g_onLauncherBy[1] : g_supportingBy[1]) && !g_holdOutOfReachBy[1];
            kin.hands = on ? g_rightBlend : 0.0f;
            kin.frontGrip = on && (launcherHands ? smawDrawn || stinger || at4 : g_onM320By[1]);
            kin.support = on ? 1 : -1;
        } else {
            const bool on = (launcherHands ? g_onLauncher : g_supporting) && !g_holdOutOfReach;
            kin.hands = on ? g_gripBlend : 0.0f;
            kin.frontGrip = on && (launcherHands ? smawDrawn || stinger || at4 : g_onM320);
            kin.support = on ? 0 : -1;
        }
        g_twoHandsNow.store(kin.held && kin.hands >= 0.5f, std::memory_order_relaxed);
        // The pivot: the trigger hand's own place on the gun, either hand's (the train pistol's grip). Both eyes in the gun's model space as the hands hold it,
        // whatever the hold (one hand or two, either hand: the glass is kept off the eye in every one), each at its own place along the head's right.
        std::memcpy(kin.pivot, pistolHeld ? pistolAnchor : triggerAnchor, sizeof(kin.pivot));
        float inverse[12];
        InvertRigid(heldPose, inverse);
        if (heldFinite) {
            for (int e = 0; e < 2; ++e) {
                float offset = views::EyeOffset(e);
                if (!(std::fabs(offset) > 0.005f && std::fabs(offset) < 0.1f))
                    offset = e == 0 ? -kHalfIpd : kHalfIpd;  // (not set: the views off)
                const float eye[3] = {offset, 0.0f, 0.0f};  // the view's own space: x right, the head's middle at its origin
                Transform(inverse, eye, kin.eyes[e]);
            }
        }
        // The rounds seen leaving here: the launchers' counts are the mod's own (their hidden rounds), so recoilrun doesn't count them.
        kin.extraShots = (smawDrawn && rocketOut.spent ? 1 : 0) + (stinger && stingerShot ? 1 : 0) + (at4 && at4Shot ? 1 : 0);
        kin.stance = input::StanceIndex();
        kin.heldPose = kin.pose != 0 && heldFinite ? heldPose : nullptr;  // (this frame's copy: good through Step)
        const recoil::Output& k = recoilrun::Step(kin);
        // (The scatter for the hold, the game's own unzoomed one with one hand on a long gun, is Step's to ask skin for with its pulses and the scope's slack:
        // asked in that one place only, so nothing goes round the wait that settles a change of hold.)
        // Drawn kicked while it's in a hand (by its pistol grip, or still by its front alone while a clear eases out), never lying or flying into a hand.
        const bool inHand = kin.pose != 0 && t.gun && heldFinite && grasped.holder != grasp::Holder::kGlide && grasped.holder != grasp::Holder::kLoose &&
                            grasped.holder != grasp::Holder::kStowed;
        if (k.active && inHand) {
            float kicked[12];
            Compose(heldPose, k.k, kicked);
            if (Finite(kicked, 12)) {
                if (kin.pose == 3) {  // the bare right hand rides the pistol: the same move, in the view's space
                    float move[12], hand[12];
                    Compose(kicked, inverse, move);
                    Compose(move, t.rightPose, hand);
                    if (Finite(hand, 12))
                        std::memcpy(t.rightPose, hand, sizeof(hand));
                }
                std::memcpy(drawn, kicked, sizeof(kicked));
                g_kicked = kin.pose;
                // Free hands: the trigger glove's let-go (g_gripRel, above: only what it's drawn with) from the gun as drawn, kicked: let go mid-kick, the glove
                // opens where it was drawn as the gun falls from there.
                if (g_freeOn && heldByGrip && (grasped.trigger == 0 || grasped.trigger == 1) && g_haveHandGun[grasped.trigger] && drawn == gunNow) {
                    float handInverse[12];
                    InvertRigid(g_handGun[grasped.trigger], handInverse);
                    Compose(handInverse, gunNow, g_gripRel[grasped.trigger]);
                    g_haveGripRel[grasped.trigger] = Finite(g_gripRel[grasped.trigger], 12);
                }
                // The right hand's own aim (PublishAim leaves the right's to xr_runtime: AimOrientation) turned as the drawn gun is, through the raise the
                // right-hand block placed it with (its sights' up), so the look is the kicked line. Free hands' own aim takes the kicked pose itself.
                if (heldByGrip && grasped.trigger == 1 && g_rightSeen) {
                    const float up = kin.pose == 3 ? 0.0f : kin.pose == 2 ? (heldGun ? held.up : skin::LauncherUp()) : skin::RifleUp();
                    const float right = kin.pose == 2 && heldGun ? held.right : 0.0f;  // (the AT4's sights' line right of its tube: SightsTurn's)
                    const XrQuaternionf base = g_aimTurned ? g_aimTurn : aim.orientation;
                    const float from[4] = {base.x, base.y, base.z, base.w};
                    float turned[4];
                    recoil::Fold(from, k.q, up, right, turned);
                    g_aimTurn = XrQuaternionf{turned[0], turned[1], turned[2], turned[3]};
                    g_aimTurned = true;
                }
            } else if (static ULONGLONG s_at = 0; GetTickCount64() - s_at > 1000) {
                s_at = GetTickCount64();
                Log("Recoil: the kicked gun came out with a number not finite: drawn as the hands hold it");
            }
        }
    }
    // Onto the handguard and off it over a few frames, not in one jump: the glove between where the
    // hand is and the game's own grip on the gun (its bones learned against the game's gun: game[0]
    // places the glove, the rest bend relative to it in BendWithController below).
    // The launcher's front grip alike, the game's own left glove on it as its animation has it just now.
    float game[handpose::kBones][12];
    // On the grenade launcher's grip, the game's own glove there (none till it's been learned: the hand stays where it is till then). Let go, the glove eases off the hold it was
    // on: off the launcher's grip it had gone to the handguard's for its few frames of easing off.
    static bool s_glovesOnM320 = false;
    if (g_supporting)
        s_glovesOnM320 = g_onM320;
    // Free hands: the left glove on the gun's pistol grip, and easing off it once let go: the game's own right hand there mirrored (FetchTriggerGrip), on the gun as
    // the left holds it (flying into it: where the glide takes it; let go, free hands gone dormant too: where the left would hold it with the turn it last rode, so
    // it opens where it was instead of snapping). On the handguard through a regrip's glide: on the rifle as it's drawn (g_glideDrawn), never left behind.
    if (g_leftOnGrip || g_supporting || (launcherHands && g_onLauncher))
        g_glovesOnPistol = g_leftOnGrip;
    const bool pistolGrip = g_glovesOnPistol;
    const bool haveGame = pistolGrip ? LeftOnPistol(game)
                                     : t.gun && (launcherHands ? skin::LauncherLeftGrip(game) : s_glovesOnM320 ? skin::GameLeftGripM320(game) : skin::GameLeftGrip(game));
    float leftLetGo[12];
    const float* leftOnPistolPose = g_handGun[0];
    if (g_leftOnGrip) {
        leftOnPistolPose = heldByGrip ? (t.gun ? gunNow : g_handGun[0]) : GlideTarget(0);
    } else if (g_haveGripRel[0]) {
        Compose(g_handGun[0], g_gripRel[0], leftLetGo);
        leftOnPistolPose = leftLetGo;
    }
    const float* holdOn = !pistolGrip ? (launcherHands ? (g_regripDrawn ? g_glideDrawn : t.launcherPose) : g_regripDrawn ? g_glideDrawn : t.gunPose) : leftOnPistolPose;
    // arms.txt "test guard", nobody in the headset: held, the stand-in's left hand as the real hand.
    float realHand[12];
    bool haveRealHand = trackedLeft;
    if (trackedLeft) {
        std::memcpy(realHand, tracked, sizeof(realHand));
    } else if (XrPosef standIn; g_armTests.guard && t.gun && !pistolHeld && input::StandInLeftPose(standIn)) {
        (launcherHands ? g_onLauncher : g_supporting) = true;
        if (!launcherHands)
            g_onM320 = g_armTests.m320;  // ("test m320": its hold on the grenade launcher's grip)
        MakeLeftOffset();
        float controller[12], glove[12];
        pose(standIn, controller);
        Compose(controller, g_leftOffset, glove);
        Compose(fromHead, glove, realHand);
        if (g_armTests.move)  // ("test move": where the stand-in's hand is taken, below)
            for (int i = 0; i < 3; ++i)
                realHand[i * 4 + 3] += g_armTests.moveBy[i];
        haveRealHand = true;
    }
    // Off the gun while the arm can't reach it (g_holdOutOfReach), back on by itself as soon as it can. (Free hands: never off the pistol grip, nor the handguard of the
    // rifle held by this hand alone: either moves with the hand, it can't drift out of reach.)
    const bool holdWanted = g_leftOnGrip || (launcherHands ? g_onLauncher : g_supporting);
    const bool holdWithHand = pistolGrip || grasped.barrel == 0;
    float shoulderNow[3];
    if (holdWanted && haveGame && haveRealHand && !holdWithHand && SquareShoulder(0, fromHead, toView, shoulderNow)) {
        float palmBone[12], holdWrist[3], handWrist[3];
        Compose(AsHeld(t, launcherHands ? t.launcherPose : t.gunPose), game[1], palmBone);  // the glove's palm bone on the gun as the hands hold it (never off it for a kick), as ArmHand has it
        Transform(palmBone, armik::kWrist, holdWrist);
        Transform(realHand, armik::kWrist, handWrist);
        const float toHold[3] = {holdWrist[0] - shoulderNow[0], holdWrist[1] - shoulderNow[1], holdWrist[2] - shoulderNow[2]};
        const float handsApart[3] = {t.rightWrist[0] - handWrist[0], t.rightWrist[1] - handWrist[1], t.rightWrist[2] - handWrist[2]};
        const float reach = Length(toHold), away = Length(handsApart);
        if (static ULONGLONG s_logged = 0; g_armsLog && GetTickCount64() - s_logged >= 1000) {
            s_logged = GetTickCount64();
            Log("Arms: the hold %.2f m from the square left shoulder, the real hands %.2f m apart%s", reach, away, g_holdOutOfReach ? " (off the gun)" : "");
        }
        if (!g_holdOutOfReach && reach > kHoldOutOfReach && away > kHoldHandAway) {
            g_holdOutOfReach = true;
            Log("Hands: the hold %.2f m from the left shoulder, out of the arm's reach, the real hands %.2f m apart: the glove off the gun until it's back in reach", reach, away);
        } else if (g_holdOutOfReach && (reach < kHoldInReach || away < kHoldHandBack)) {
            g_holdOutOfReach = false;
            Log("Hands: the hold %.2f m from the left shoulder, the real hands %.2f m apart: back on the gun", reach, away);
        }
    } else {
        g_holdOutOfReach = false;
    }
    g_gripBlend = holdWanted && haveGame && !g_holdOutOfReach ? std::fmin(g_gripBlend + kGripBlendStep, 1.0f)
                                                              : std::fmax(g_gripBlend - kGripBlendStep, 0.0f);
    if (g_gripBlend == 0.0f && !g_leftOnGrip)
        g_glovesOnPistol = false;  // (eased off the pistol grip: done with it)
    if (g_gripBlend > 0.0f && haveGame) {
        float hold[12], blended[12];
        Compose(holdOn, game[0], hold);
        if (trackedLeft)
            BlendAbout(tracked, hold, g_gripBlend, g_leftAnchor, blended);
        else
            std::memcpy(blended, hold, sizeof(blended));  // the left controller not tracked: held as it is
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        t.left = true;
    }
    // The train pistol in both hands (The Great Destroyer): the left hand onto the game's own left hand on
    // it and off it, as onto the handguard.
    PistolSupport(t, pistolHeld, trackedLeft, tracked);
    // The left hand on its slide (pistol.txt "slide 1"), over the two-handed hold if that's on too.
    OnTheSlide(t, pistolHeld);
    // The right fingers' opening on the grip (skin.cpp): both hands' while the left holds the pistol, not its slide.
    t.pistolTwoHanded = pistolHeld ? g_pistolBlend * (1.0f - g_slideShare) : 0.0f;
    // A magazine caught on the well's line: the glove goes with it (it stops at home), easing in and
    // back out to the hand. (The left's; the right's: RightGlove.)
    if (magazine.side == 0 && magazine.gloveBlend > 0.0f && t.left) {
        float blended[12];
        BlendAbout(t.leftPose, magazine.glove, magazine.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
    }
    // A gun with no button's magazine on the well's line or being rocked out, or the fist on its lever, likewise. (The left's; the right's: RightGlove.)
    if (heldMag.side == 0 && heldMag.gloveBlend > 0.0f && t.left) {
        float blended[12];
        BlendAbout(t.leftPose, heldMag.glove, heldMag.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
    }
    // The train pistol's on the grip's line (pistol.txt "mag 1"), likewise: the hand drawn under it, on the flat base
    // and then the push home, easing back out to the hand after the seat or a park.
    if (pistolMagazine.gloveBlend > 0.0f && t.left) {
        float blended[12];
        BlendAbout(t.leftPose, pistolMagazine.glove, pistolMagazine.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        NoteBad(t, "the pistol magazine's hold");
    }
    // The case on the launcher's line, likewise: the hand drawn on it on its way home, easing back out to the hand after.
    if (rocketOut.gloveBlend > 0.0f && t.left) {
        float blended[12];
        BlendAbout(t.leftPose, rocketOut.glove, rocketOut.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        NoteBad(t, "the launcher case's hold");
    }
    // The grenade launcher's reload by hand (m320.h): the glove on its round's rim, on the round going home, or on its barrel, as the game's own hand holds them there.
    // (The left at work's; the right's: RightGlove.)
    if (m320Out.side == 0 && m320Out.gloveBlend > 0.0f && t.left) {
        float onGun[12], blended[12], hold[12], rebaseInverse[12];
        // (Re-anchored on the palm as the fingers are: the hold's glove times the rebase's inverse, so the drawn hand is the game's there exactly.)
        if (haveM320Rebase && Invert(m320Rebase, rebaseInverse))
            Compose(m320Out.gloveOnGun, rebaseInverse, hold);
        else
            std::memcpy(hold, m320Out.gloveOnGun, sizeof(hold));
        Compose(t.gunPose, hold, onGun);
        BlendAbout(t.leftPose, onGun, m320Out.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        NoteBad(t, "the grenade launcher's hold (the glove)");
    }
    // A shotgun's shell riding up its loading port in the left hand, or the left on its bolt handle: the glove on the gun with it (shellload.h; the
    // right's: RightGlove).
    if (shellOut.side == 0 && shellOut.gloveBlend > 0.0f && t.left) {
        float onGun[12], blended[12];
        Compose(t.launcherPose, shellOut.gloveOnGun, onGun);
        BlendAbout(t.leftPose, onGun, shellOut.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        NoteBad(t, "the shotgun's shell or handle (the glove)");
    }
    // The hand on the Stinger's aerial, drawn as the game has it there.
    if (heldFlap.gloveBlend > 0.0f && t.left) {
        float blended[12];
        BlendAbout(t.leftPose, heldFlap.glove, heldFlap.gloveBlend, g_leftAnchor, blended);
        std::memcpy(t.leftPose, blended, sizeof(blended));
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        NoteBad(t, "the Stinger's hold");
    }
    // A fist on a gun's side lever (the user's video, 2026-11-05): every finger closed round the knob, the thumb over them. (The right's: RightGlove.)
    g_heldFist += (((heldMag.side == 0 && heldMag.fist && !heldMag.leverFingers) || (shellOut.fist && shellOut.side == 0) ? 1.0f : 0.0f) - g_heldFist) * 0.35f;
    g_heldFistBy1 += ((heldMag.side == 1 && heldMag.fist && !heldMag.leverFingers ? 1.0f : 0.0f) - g_heldFistBy1) * 0.35f;
    if (t.left && g_heldFist > 0.001f) {
        for (int i = 0; i < 4; ++i)
            fingers.curl[i] += (0.88f - fingers.curl[i]) * g_heldFist;
        fingers.thumb += (0.8f - fingers.thumb) * g_heldFist;
    }
    // The free glove stays out of the gun in hand, each by its own shape (the user, 2026-11-05: "Give them all collision"), and where it touches the gun is felt (the bolt release);
    // not while it carries a magazine or a rocket up to the gun. The fist on a lever stays out too: it clipped through the gun.
    float touch[3];
    const char* shape = pistolHeld ? nullptr : heldGun ? held.shape : smawHeld ? "smaw" : "m416";
    static const char* s_shape = nullptr;
    static bool s_shapeLoaded = false;
    if (shape && (shape != s_shape || (!s_shapeLoaded && shapemake::Ready(shape)))) {  // (or its shape has just been made on this PC: shapemake.h)
        s_shapeLoaded = gunshape::Use(shape);
        for (collide::State& s : g_collideBy)
            collide::Reset(s);
        s_shape = shape;
    }
    shapemake::Holding(shape, s_shapeLoaded);  // (no shape: it's made on this PC, asked for now and every few seconds while this gun is held)
    // Whether the gun in hand really has its shape, its file loaded (not just its name known): the release zip carries no shapes
    // (they're made from the game's meshes), and without one no glove ever touches the gun, so the bolt release's slap goes by
    // a quick hand right at the release instead of by the touch (a player's M4A1 never slapped in the download, 2026-11-14).
    const bool shapeLoaded = shape != nullptr && s_shapeLoaded;
    const bool free = t.gun && t.left && !g_lining && !g_supporting && g_gripBlend == 0.0f && reload::HandleSide() != 0 &&
                      !busyWithMag && !rocketOut.holding && !testing && shape && !(heldMag.side == 0 && heldMag.leverFingers) && !heldFlap.holding &&  // (exactly on a lever: it isn't to be pushed off it)
                      !(m320Out.side == 0 && m320Out.gloveBlend > 0.0f) &&  // (nor on the grenade launcher's round or barrel, as the game's hand is there)
                      !(shellOut.side == 0 && (shellOut.gloveBlend > 0.0f || shellOut.holding)) &&  // (nor with a shotgun's shell at its ports, or on its bolt handle)
                      !shellTesting &&
                      !g_leftOnGrip;  // (nor on the pistol grip the gun hangs from, its table not learned yet)
    if (!free)
        collide::Reset(g_collide);
    // (By the gun as the hands hold it: a kick never shoves the free glove, nor is it felt as a touch on the bolt release.)
    const bool touching = free && KeepOutOfGun(t, AsHeld(t, launcherHeld ? t.launcherPose : t.gunPose), fingers, touch);
    // Free hands: the free right glove alike while it's the hand at work and free (not on a hold or the charging handle, no magazine or launcher's round in it),
    // by the left glove's spheres through the mirror (KeepOutOfGunRight); drawn moved out (RightGlove), its touch felt by the bolt release. Its fingers are its
    // controller's own, as the free glove is posed. (Its blend onto a hold and the handle's V: last frame's, RightGlove comes later.)
    float rightTouch[3], rightShift[3] = {};
    // (A fist on a held gun's lever with no table for the right, a lever off the gun's middle: kept out of the gun too, as the left's fist is; the review of
    // fc010e2: it went through the gun, which the user had seen the left's do on 2026-11-05.)
    const bool rightFist = heldMag.side == 1 && heldMag.fist && !heldMag.leverFingers;
    const bool rightFree = g_freeOn && shape && t.gun && g_workHand == 1 && g_rightWorks && !g_supportingBy[1] && g_rightBlend == 0.0f &&
                           reload::HandleSide() != 1 && g_leverBy[1] < 0.001f && !(magazine.side == 1 && (magazine.holding || magazine.gloveBlend > 0.0f)) &&
                           !(m320Out.side == 1 && (m320Out.holding || m320Out.gloveBlend > 0.0f)) && !(shellOut.side == 1 && (shellOut.holding || shellOut.gloveBlend > 0.0f)) &&
                           !(heldMag.side == 1 && (heldWorkBy[1] || heldMag.gloveBlend > 0.0f) && !rightFist) &&  // (a held gun's magazine, a fitted hand on its lever or the lid: drawn on it)
                           !lpBridge;  // (no shape or place for it then: the M416's at the right hand)
    bool rightTouching = false;
    if (rightFree) {
        float width = 1.0f, slim = 1.0f;
        freetune::RightWidth(width, slim);
        handpose::Fingers rightFingers = RightFreeFingers(slim);
        float rightGlove[12];
        std::memcpy(rightGlove, g_rightFree, sizeof(rightGlove));
        if (rightFist) {
            // The fist as RightGlove draws it before the move out: the glove on the knob about its palm point (handmag's side 1), every finger closed
            // round it, the thumb over (g_heldFistBy1, this frame's).
            const float rightAnchor[3] = {-g_leftAnchor[0], g_leftAnchor[1], g_leftAnchor[2]};
            if (heldMag.gloveBlend > 0.0f)
                BlendAbout(g_rightFree, heldMag.glove, heldMag.gloveBlend, rightAnchor, rightGlove);
            for (int i = 0; i < 4; ++i)
                rightFingers.curl[i] += (0.88f - rightFingers.curl[i]) * g_heldFistBy1;
            rightFingers.thumb += (0.8f - rightFingers.thumb) * g_heldFistBy1;
        }
        rightTouching = KeepOutOfGunRight(AsHeld(t, launcherHeld ? t.launcherPose : t.gunPose), rightGlove, rightFingers, width, rightShift, rightTouch);  // (the gun in hand's own pose, as held)
    } else {
        collide::Reset(g_collideBy[1]);
    }
    // The bolt and its manual releases (reload.cpp): the gun and the left palm, both in the view's
    // space. Only a tracked hand works them: without the left controller's place the palm is still
    // HandTargets' zero (the middle of the head), even while the glove is drawn on the handguard (and
    // the pouch test's glove is never tracked). A hand carrying a magazine doesn't grab the handle or
    // slap, nor does the push that just seated one (for 0.3 s: the bolt release is right by the well):
    // no palm then, which blocks both. The trigger stays the real one, so a trigger held through that
    // time isn't a fresh press when it ends.
    const bool busy =
        busyWithMag || GetTickCount64() - g_seatedAt < 300 || GetTickCount64() - g_pistolSeatedAt < 300;
    // An M16's or an M4's bolt, locked back with a magazine in: a slap on its release, as the M416's.
    {
        static float s_palmWas[3] = {};
        static bool s_havePalm = false, s_onRelease = false;
        const bool have = magGun && magGun->boltCatch && t.gun && trackedLeft && !busy && !g_leftOnGrip;  // (not the hand on its pistol grip)
        float onGun[3] = {};
        if (have) {
            float inverse[12];
            InvertRigid(AsHeld(t, t.launcherPose), inverse);  // (the palm on the gun as the hands hold it: a kick isn't a slap)
            Transform(inverse, palm, onGun);
            int loaded = 0, spare = 0;
            const bool slapped = reload::Slapped(onGun, s_havePalm ? s_palmWas : onGun, s_havePalm ? frameSeconds : 0.0f, touching ? touch : nullptr, s_onRelease, magGun->release, shapeLoaded);
            if (slapped && g_heldDry && !heldMag.fist && !skin::MagazineOut() && ammo::Get(loaded, spare) && loaded > 1) {
                g_heldDry = false;
                PlayGunsOwn("bolt", "bolt");
                Log("Held magazine: the bolt release slapped%s: a round chambered", touching ? " (the glove touched it)" : "");
            }
            std::memcpy(s_palmWas, onGun, sizeof(s_palmWas));
        }
        s_havePalm = have;
    }
    // ... and by the right (free hands: the right at work, the left holding the gun by its pistol grip or its front alone), as the M416's right
    // (reload.cpp): by where it really is and its own glove's touch, free of a magazine and not just after a seat, nor on the gun's grips.
    {
        static float s_palmWas[3] = {};
        static bool s_havePalm = false, s_onRelease = false;
        const bool rightBusy = busyBy[1] || GetTickCount64() - g_seatedAt < 300;
        const bool have = magGun && magGun->boltCatch && t.gun && g_workHand == 1 && g_rightWorks && !rightBusy && !g_onGripBy[1] && !g_spentBy[1] &&
                          !g_onLauncherBy[1];
        float onGun[3] = {};
        if (have) {
            float inverse[12];
            InvertRigid(AsHeld(t, t.launcherPose), inverse);
            Transform(inverse, g_rightPalm, onGun);
            int loaded = 0, spare = 0;
            const bool slapped = reload::Slapped(onGun, s_havePalm ? s_palmWas : onGun, s_havePalm ? frameSeconds : 0.0f, rightTouching ? rightTouch : nullptr, s_onRelease,
                                                 magGun->release, shapeLoaded);
            if (slapped && g_heldDry && !heldMag.fist && !skin::MagazineOut() && ammo::Get(loaded, spare) && loaded > 1) {
                g_heldDry = false;
                PlayGunsOwn("bolt", "bolt");
                Log("Held magazine: the bolt release slapped by the right hand%s: a round chambered", rightTouching ? " (the glove touched it)" : "");
            }
            std::memcpy(s_palmWas, onGun, sizeof(s_palmWas));
        }
        s_havePalm = have;
    }
    // Not the M416's charging handle and bolt release with the train pistol in hand, nor the launcher.
    // (Free hands: nor with the left on the rifle's pistol grip, or the frame its squeeze took it up. Either hand, each by its own controller: the right
    // only as the hand at work, free of a magazine or the launcher's round and not just after a seat, by where it really is and its own glove's touch;
    // the left not while the right works the rifle.)
    {
        reload::Hand reloadHands[2];
        reloadHands[0].palm = trackedLeft && !busy && !g_leftOnGrip && !g_leftSpent && g_workHand != 1 ? palm : nullptr;
        reloadHands[0].grip = SqueezeOf(0);
        reloadHands[0].trigger = TriggerOf(0);
        reloadHands[0].touch = touching ? touch : nullptr;
        const bool rightBusy = busyBy[1] || GetTickCount64() - g_seatedAt < 300;
        reloadHands[1].palm = g_workHand == 1 && g_rightWorks && !rightBusy && !g_onGripBy[1] && !g_spentBy[1] ? g_rightPalm : nullptr;
        reloadHands[1].grip = SqueezeOf(1);
        reloadHands[1].trigger = TriggerOf(1);
        reloadHands[1].touch = rightTouching ? rightTouch : nullptr;
        reload::UpdateHands(t.gun && !pistolHeld && !launcherHeld && !lpBridge ? AsHeld(t, t.gunPose) : nullptr, reloadHands, shapeLoaded);  // (as held: a kick isn't a pull)
    }
    // Round the magazine, easing out like the handle's; on the magazine at once the frame it's taken
    // (eased in, the squeezed fist went through it for 4 frames). Refitted again 2026-10-31 on the
    // meshes at the user's palm width 1.15, with pouch.cpp's g_hold (the user: the thumb wrapped round
    // to the other side, the four fingers really gripping): the palm on the magazine's left side, the
    // four fingers curled round its front edge with their tips on its right side, the thumb opposed
    // (handpose's thumbOut and thumbRoll) round its back edge onto the right side, pressing there.
    // (A gun with its own fit has its own table below.)
    // (By hand: the pouch's magazine in the left, side 0, here; in the right, side 1, its own: RightGlove.)
    const bool heldFitted = magGun && magGun->fingers;
    const bool magLeft = magazine.side == 0, magRight = magazine.side == 1;
    const bool heldLeft = heldMag.side == 0, heldRight = heldMag.side == 1;  // (a held gun's magazine: in the hand the module serves)
    if ((magLeft && (magazine.took || magazine.grabbed)) || (heldLeft && !heldFitted && (heldMag.took || heldMag.grabbed)))
        g_magFingers = 1.0f;
    g_magFingers += (((magLeft && magazine.holding) || (heldLeft && !heldFitted && heldMag.holding) ? 1.0f : 0.0f) - g_magFingers) * 0.35f;
    if ((magRight && (magazine.took || magazine.grabbed)) || (heldRight && !heldFitted && (heldMag.took || heldMag.grabbed)))
        g_magFingersBy[1] = 1.0f;
    g_magFingersBy[1] += (((magRight && magazine.holding) || (heldRight && !heldFitted && heldMag.holding) ? 1.0f : 0.0f) - g_magFingersBy[1]) * 0.35f;
    // A magazine dropped by force from the squeezed hand (the pouch block): that hand open at once (the hold still has it: no jump) while the squeeze stays
    // held, and after it's let go of till its own fingers are open too (none closing back as the real hand opens) or it squeezes again (eased in from open).
    for (int s = 0; s < 2; ++s) {
        const float squeeze = std::fmax(SqueezeOf(s), TriggerOf(s));
        if (g_dropOpenState[s] == 1 && squeeze < 0.4f)
            g_dropOpenState[s] = 2;
        if (g_dropOpenState[s] == 2 && (std::fmax(g_gripBy[s], g_indexBy[s]) < 0.05f || squeeze > 0.6f))
            g_dropOpenState[s] = 0;
        g_dropOpenBy[s] = g_dropOpenState[s] ? 1.0f : g_dropOpenBy[s] * 0.65f;
    }
    if (t.left && g_dropOpenBy[0] > 0.001f)
        FingersOpened(fingers, g_dropOpenBy[0]);
    if (t.left && g_magFingers > 0.001f)
        FingersOnMagazine(fingers, g_magFingers);  // option A (pouch.cpp g_hold): the back edge in the palm, the thumb flat along the far face
    // Round the launcher's case (rocket.cpp): at once the frame it's taken (eased in, a hand closing went through it), easing out as it's let go.
    g_rocketFingers = rocketOut.took || rocketOut.grabbed ? 1.0f : g_rocketFingers + ((rocketOut.holding ? 1.0f : 0.0f) - g_rocketFingers) * 0.35f;
    // Round a gun's own magazine, fitted to it (magposes.h): at once as it's taken, as the launcher's case, or the hand closing would go through it.
    static const float (*s_heldFingers)[12] = nullptr;
    static float s_heldFingersShare = 0.0f;
    if (heldFitted)
        s_heldFingers = magGun->fingers;
    if (heldLeft && heldMag.leverFingers)  // on the lever, as the game's own hand
        s_heldFingers = heldMag.leverFingers;
    if (heldFlap.fingers)  // on the Stinger's aerial, as the game's own hand
        s_heldFingers = heldFlap.fingers;
    s_heldFingersShare = heldLeft && heldFitted && (heldMag.took || heldMag.grabbed)
                             ? 1.0f
                             : s_heldFingersShare + (((heldLeft && heldFitted && heldMag.holding) || (heldLeft && heldMag.leverFingers) || heldFlap.fingers ? 1.0f : 0.0f) -
                                                     s_heldFingersShare) * 0.35f;
    // The right's alike (the left's tables flipped onto it: RightGlove), a held gun's alone: its magazine fitted, its lever, lid or the belt's pinch.
    static const float (*s_heldFingersRight)[12] = nullptr;
    static float s_heldFingersRightShare = 0.0f;
    if (heldFitted)
        s_heldFingersRight = magGun->fingers;
    if (heldRight && heldMag.leverFingers)
        s_heldFingersRight = heldMag.leverFingers;
    s_heldFingersRightShare = heldRight && heldFitted && (heldMag.took || heldMag.grabbed)
                                  ? 1.0f
                                  : s_heldFingersRightShare + (((heldRight && heldFitted && heldMag.holding) || (heldRight && heldMag.leverFingers) ? 1.0f : 0.0f) -
                                                               s_heldFingersRightShare) * 0.35f;
    if (t.left) {
        NoteBad(t, "before the fingers (the holds above)");
        OnTheLever(t, fingers);
        // (Bare hands, Semper Fidelis's and The Great Destroyer's, drawn by these with a gun in hand: skin as it is, no glove's widening, the fingers their whole width.
        // The fitted hands' tables too: most were made on the glove at width 1.15 with its fingers slimmed, and blended whole onto skin the palm jumped 15% wider
        // (the M1014's shell pinch, the AKS-74u's under-hand on the bolt handle: the review, 2026-11-18). On skin each goes on without its widening, every bone
        // rigid (handpose::SkinTable; a table made rigid as it is). The grenade launcher's are its own, rebased on the glove's palm: no bare level has one.)
        float skinTable[handpose::kBones][12];
        auto onSkin = [&](const float (*table)[12]) -> const float (*)[12] {
            if (!bareHands)
                return table;
            handpose::SkinTable(table, skinTable);
            return skinTable;
        };
        if (bareHands) {
            handpose::Fingers skinFingers = fingers;
            skinFingers.slim = 1.0f;
            handpose::Pose(skinFingers, 1.0f, t.leftFingers);
        } else {
            handpose::Pose(fingers, g_leftWidth, t.leftFingers);
        }
        NoteBad(t, "the controller's own fingers (or the charging handle)");
        if (s_heldFingers && s_heldFingersShare > 0.001f) {
            const float (*const table)[12] = onSkin(s_heldFingers);
            for (int b = 0; b < handpose::kBones; ++b) {
                float pivot[3], blended[12];
                handpose::Pivot(b, pivot);
                BlendAbout(t.leftFingers[b], table[b], s_heldFingersShare, pivot, blended);
                std::memcpy(t.leftFingers[b], blended, sizeof(blended));
            }
        }
        NoteBad(t, "a gun's own fingers (magazine, lever, aerial)");
        // A shotgun's shell in the left hand: the fitted pinch (shellposes.h), in by the module's own share (at once as it's taken).
        if (shellOut.side == 0 && shellOut.fingers && shellOut.fingersShare > 0.001f) {
            const float (*const table)[12] = onSkin(shellOut.fingers);
            for (int b = 0; b < handpose::kBones; ++b) {
                float pivot[3], blended[12];
                handpose::Pivot(b, pivot);
                BlendAbout(t.leftFingers[b], table[b], shellOut.fingersShare, pivot, blended);
                std::memcpy(t.leftFingers[b], blended, sizeof(blended));
            }
        }
        NoteBad(t, "the shotgun shell's fingers");
        // The game's own hand on the launcher's case, bone by bone (rocketposes.h).
        if (g_rocketFingers > 0.001f) {
            const float (*const table)[12] = onSkin(rocketposes::kFingers);
            for (int b = 0; b < handpose::kBones; ++b) {
                float pivot[3], blended[12];
                handpose::Pivot(b, pivot);  // each bone blended about its own joint
                BlendAbout(t.leftFingers[b], table[b], g_rocketFingers, pivot, blended);
                std::memcpy(t.leftFingers[b], blended, sizeof(blended));
            }
        }
        NoteBad(t, "the launcher case's fingers");
        // The game's own hand on the grenade launcher's round or barrel in its reload (m320.h's tables from the recording): the pinch blending to the push as the round
        // goes home, in by the module's own share. (The left at work's; the right's: RightGlove.)
        float beforeM320[handpose::kBones][12];
        std::memcpy(beforeM320, t.leftFingers, sizeof(beforeM320));
        if (m320Out.side == 0 && haveM320Rebase && m320Out.fingersShare > 0.001f) {  // (the tables re-anchored on the palm above: the palm stays the hand's)
            for (int b = 0; b < handpose::kBones; ++b) {
                float pivot[3], blended[12];
                handpose::Pivot(b, pivot);
                BlendAbout(t.leftFingers[b], m320Table[b], m320Out.fingersShare, pivot, blended);
                std::memcpy(t.leftFingers[b], blended, sizeof(blended));
            }
        }
        NoteBad(t, "the grenade launcher's fingers");
        // (Its numbers when it went bad there, once a second: what went in, to replay offline.)
        if (g_badStage && std::strcmp(g_badStage, "the grenade launcher's fingers") == 0 && g_badBone >= 0) {
            static ULONGLONG s_dumpAt = 0;
            if (GetTickCount64() - s_dumpAt > 1000) {
                s_dumpAt = GetTickCount64();
                const int b = g_badBone;
                const float* a = beforeM320[b];
                const float* m = m320Table[b];
                Log("M320 numbers: bone %d share %.9g mix %.9g; the hand's own %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g; the table's %.9g %.9g %.9g %.9g %.9g %.9g "
                    "%.9g %.9g %.9g %.9g %.9g %.9g; rebase %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g",
                    b, m320Out.fingersShare, m320Out.fingersMix, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], m[0], m[1], m[2], m[3], m[4], m[5], m[6],
                    m[7], m[8], m[9], m[10], m[11], m320Rebase[0], m320Rebase[1], m320Rebase[2], m320Rebase[3], m320Rebase[4], m320Rebase[5], m320Rebase[6], m320Rebase[7],
                    m320Rebase[8], m320Rebase[9], m320Rebase[10], m320Rebase[11]);
            }
        }
        if (g_magFingers > 0.001f) {
            // The opposed thumb laid flat on the magazine's far side (opened 15.5 degrees over its
            // joints, option A); eased in and out with the other fingers.
            const float press[5] = {-15.49f * g_magFingers, 0.0f, 0.0f, 0.0f, 0.0f};
            handpose::BendFurther(t.leftFingers, press);
        }
        NoteBad(t, "the fingers round a magazine");
        if (g_gripBlend > 0.0f && haveGame) {
            // The game's grip, its fingers moving a little with the controller's (less stiff: user,
            // 2026-10-30), relative to its palm bone; blended from the controller's own fingers. On the grenade launcher's grip, the index finger on its trigger.
            const bool onM320Trigger = s_glovesOnM320 && !launcherHands && !pistolGrip;
            // Fitted again only on a hold learned anew since (a few thousand poses tried: a frame's spike at every take of the grip otherwise, the review 2026-11-05).
            if (onM320Trigger && !g_m320IndexFitted) {
                if (g_m320HaveFitOn && !std::memcmp(game[9], g_m320FitOn[0], sizeof(g_m320FitOn[0])) && !std::memcmp(game[19], g_m320FitOn[1], sizeof(g_m320FitOn[1])) &&
                    !std::memcmp(game[15], g_m320FitOn[2], sizeof(g_m320FitOn[2])))
                    g_m320IndexFitted = true;
                else
                    FitIndexToM320Trigger(game, 0);
            }
            // (Free hands, on the rifle's pistol grip: its index on the trigger, curling in with the left trigger as the right's does. Onto the launcher's release only
            // while this hand presses it: the hand on its grip.)
            const bool pressing = m320Out.gripSide == 0;
            BendWithController(game, pistolGrip ? Bend::kPistolGrip : onM320Trigger ? Bend::kM320Trigger : Bend::kHandguard,
                               onM320Trigger && pressing ? m320Out.indexOnRelease : 0.0f, pressing ? m320Out.releaseLift : 0.0f, SqueezeOf(0), TriggerOf(0), ThumbOf(0),
                               g_fingerBend);
            float root[12], held[12], blended[12], pivot[3];
            InvertRigid(game[0], root);
            for (int b = 0; b < handpose::kBones; ++b) {
                Compose(root, game[b], held);
                handpose::Pivot(b, pivot);  // each bone blended about its own joint
                BlendAbout(t.leftFingers[b], held, g_gripBlend, pivot, blended);
                std::memcpy(t.leftFingers[b], blended, sizeof(blended));
            }
            NoteBad(t, "the handguard's grip (fingers)");
        }
    }
    // The stand-in's left hand (nobody in the headset: input.cpp), only drawn, its fingers relaxed:
    // the holds, reloads and touches above never saw it.
    bool standInLeft = false;
    if (XrPosef standIn; !t.left && input::StandInLeftPose(standIn)) {
        MakeLeftOffset();
        float controller[12], glove[12];
        pose(standIn, controller);
        Compose(controller, g_leftOffset, glove);
        Compose(fromHead, glove, t.leftPose);
        Transform(t.leftPose, g_leftAnchor, t.leftWrist);
        handpose::Fingers relaxed;
        relaxed.spread = g_spread;
        if (skin::BareHold() && skin::BareArmsLately())
            relaxed.slim = 1.0f;
        handpose::Pose(relaxed, relaxed.slim == 1.0f ? 1.0f : g_leftWidth, t.leftFingers);
        t.left = true;
        standInLeft = true;
    }
    // The bare left hand while the right holds the train pistol (The Great Destroyer): its fingers as the
    // controller holds them (the glove's pose, all of the fingers' width as the game draws them), going into
    // the game's own two-handed hold as the hand takes the pistol (PistolSupport). skin.cpp puts them on the
    // bare palm. The user: the game's left hand there "only for two-handed mode" (2026-10-31).
    if (t.left && pistolHeld) {
        handpose::Fingers bare = fingers;
        bare.slim = 1.0f;
        float posed[handpose::kBones][12], palmInverse[12];
        handpose::Pose(bare, 1.0f, posed);
        InvertRigid(posed[1], palmInverse);
        for (int b = 0; b < handpose::kBones; ++b) {
            float own[12];
            Compose(palmInverse, posed[b], own);
            if (g_pistolBlend > 0.0f && g_havePistolLeft) {
                float pivot[3];
                handpose::Pivot(b, pivot);  // each bone blended about its own joint
                BlendAbout(own, g_pistolLeftFingers[b], g_pistolBlend, pivot, t.bareLeftFingers[b]);
            } else {
                std::memcpy(t.bareLeftFingers[b], own, sizeof(own));
            }
            // On the pistol's magazine (PistolMagazine): the carry round it, opening into the flat base once it's
            // caught, going over to the push home near the top, eased in and out with the hand.
            if (g_pistolMagFingers > 0.0f) {
                float pivot[3], carried[12], target[12], blended[12];
                handpose::Pivot(b, pivot);
                BlendAbout(g_pistolCarry->fingers[b], pistolposes::kFlat[b], g_pistolMagFlat, pivot, carried);
                BlendAbout(carried, pistolposes::kSeated[b], g_pistolMagPush, pivot, target);
                BlendAbout(t.bareLeftFingers[b], target, g_pistolMagFingers, pivot, blended);
                std::memcpy(t.bareLeftFingers[b], blended, sizeof(blended));
            }
            // On the slide (OnTheSlide): in the grip it took (S1 over its right side, S2 over its left, S3 the pinch),
            // eased in and out with the hand (it keeps that grip until it's eased back off).
            if (g_slideShare > 0.0f) {
                float pivot[3], blended[12];
                handpose::Pivot(b, pivot);
                BlendAbout(t.bareLeftFingers[b], pistolposes::kSlideFingers[g_slideOut.grip][b], g_slideShare, pivot,
                           blended);
                std::memcpy(t.bareLeftFingers[b], blended, sizeof(blended));
            }
        }
        t.bareLeftOn = true;
    }
    // The bare left hand (Semper Fidelis, The Great Destroyer: no gloves) with a shotgun's shell in it: its fingers the fitted pinch too, on the bare palm as
    // the pistol's holds are (relative to the table's own palm), blended in from the controller's own; without a shell, the game's own fingers as before.
    // (Only while the bare hands aren't drawn as gloves, weapon.txt "barehold 0": as gloves the pinch is in leftFingers already, with every other fitted hand.)
    if (t.left && !pistolHeld && !skin::BareOnRig() && shellOut.side == 0 && shellOut.fingers && shellOut.fingersShare > 0.001f) {
        handpose::Fingers bare = fingers;
        bare.slim = 1.0f;
        float posed[handpose::kBones][12], palmInverse[12], tableInverse[12];
        handpose::Pose(bare, 1.0f, posed);
        InvertRigid(posed[1], palmInverse);
        if (Invert(shellOut.fingers[1], tableInverse)) {
            for (int b = 0; b < handpose::kBones; ++b) {
                float own[12], target[12], pivot[3];
                Compose(palmInverse, posed[b], own);
                Compose(tableInverse, shellOut.fingers[b], target);
                handpose::Pivot(b, pivot);
                BlendAbout(own, target, shellOut.fingersShare, pivot, t.bareLeftFingers[b]);
            }
            t.bareLeftOn = Finite(&t.bareLeftFingers[0][0], handpose::kBones * 12);
        }
    }
    // The left glove as finally drawn: never a number that isn't finite (the real hand's own then), and a jump of it the real hand didn't make logged with what the
    // launcher's hand was doing (the user, 2026-11-05: both arms gone for a moment each time the launcher's magnet took a round).
    if (t.left) {
        bool finite = true;
        for (int i = 0; i < 12 && finite; ++i)
            finite = std::isfinite(t.leftPose[i]);
        for (int b = 0; b < handpose::kBones && finite; ++b)
            for (int i = 0; i < 12 && finite; ++i)
                finite = std::isfinite(t.leftFingers[b][i]);
        static ULONGLONG s_badAt = 0;
        if (!finite) {
            if (GetTickCount64() - s_badAt > 1000) {
                s_badAt = GetTickCount64();
                bool trackedFinite = true;
                for (int i = 0; i < 12; ++i)
                    trackedFinite = trackedFinite && std::isfinite(tracked[i]);
                Log("Hands: the left glove came out with a number not finite, first after \"%s\" (%s %d); the controller's own %s; the launcher's hand %d, glove on it %.2f, "
                    "fingers %.2f mixed %.2f (%s, %s), rebase %s; the controller's fingers index %.2f grip %.2f thumb %.2f; magazine %.2f, case %.2f, handguard %.2f: drawn as the "
                    "controller has it",
                    g_badStage ? g_badStage : "none of the steps", g_badBone == -1 ? "its place" : "finger bone", g_badBone, trackedFinite ? "finite" : "NOT finite",
                    static_cast<int>(m320Out.handState), m320Out.gloveBlend, m320Out.fingersShare, m320Out.fingersMix, m320Out.fingers ? "a table" : "none",
                    m320Out.fingersAlt ? "and another" : "alone", haveM320Rebase ? "had" : "none", g_index, g_grip, g_thumb, g_magFingers, g_rocketFingers, g_gripBlend);
            }
            bool trackedFinite = true, fingersFinite = true;
            for (int i = 0; i < 12; ++i)
                trackedFinite = trackedFinite && std::isfinite(tracked[i]);
            handpose::Fingers own = fingers;
            for (float* v : {&own.curl[0], &own.curl[1], &own.curl[2], &own.curl[3], &own.thumb, &own.spread, &own.thumbOut, &own.thumbRoll})
                if (!std::isfinite(*v))
                    *v = 0.0f, fingersFinite = false;
            // (The controller's own pose, or the last good one drawn if even that isn't finite.)
            std::memcpy(t.leftPose, trackedFinite || !g_haveGoodLeft ? tracked : g_goodLeft, sizeof(t.leftPose));
            handpose::Pose(own, g_leftWidth, t.leftFingers);
            Transform(t.leftPose, g_leftAnchor, t.leftWrist);
            (void)fingersFinite;
        } else {
            std::memcpy(g_goodLeft, t.leftPose, sizeof(g_goodLeft));
            g_haveGoodLeft = true;
        }
        static float s_drawnWas[12], s_realWas[3], s_pointWas[3];
        static bool s_haveWas = false;
        static ULONGLONG s_jumpAt = 0;
        float drawnPalm[12], realPalm[3], drawnPoint[3];
        Compose(t.leftPose, t.leftFingers[1], drawnPalm);
        Transform(tracked, g_leftAnchor, realPalm);
        Transform(t.leftPose, g_leftAnchor, drawnPoint);  // (the palm as drawn: the palm bone's own place is the bind space's origin, a metre off the hand)
        if (s_haveWas && trackedLeft) {
            const float moved[3] = {drawnPoint[0] - s_pointWas[0], drawnPoint[1] - s_pointWas[1], drawnPoint[2] - s_pointWas[2]};
            const float real[3] = {realPalm[0] - s_realWas[0], realPalm[1] - s_realWas[1], realPalm[2] - s_realWas[2]};
            float dot = 0.0f, na = 0.0f, nb = 0.0f;
            for (int c = 0; c < 3; ++c)
                for (int i = 0; i < 3; ++i)
                    dot += drawnPalm[i * 4 + c] * s_drawnWas[i * 4 + c], na += drawnPalm[i * 4 + c] * drawnPalm[i * 4 + c], nb += s_drawnWas[i * 4 + c] * s_drawnWas[i * 4 + c];
            const float turned = std::acos(std::fmax(-1.0f, std::fmin(1.0f, (dot / std::sqrt(na * nb) * 3.0f - 1.0f) / 2.0f))) * 57.29578f;
            if ((Length(moved) > 0.08f || turned > 30.0f) && Length(real) < 0.03f && GetTickCount64() - s_jumpAt > 500) {
                s_jumpAt = GetTickCount64();
                Log("Hands: the left glove jumped %.0f cm and %.0f degrees in a frame, the real hand %.1f cm (the launcher's hand %d, glove on it %.2f, fingers %.2f; "
                    "handguard %.2f, magazine %.2f)", Length(moved) * 100.0f, turned, Length(real) * 100.0f, static_cast<int>(m320Out.handState), m320Out.gloveBlend,
                    m320Out.fingersShare, g_gripBlend, g_magFingers);
            }
        }
        std::memcpy(s_drawnWas, drawnPalm, sizeof(s_drawnWas));
        std::memcpy(s_realWas, realPalm, sizeof(s_realWas));
        std::memcpy(s_pointWas, drawnPoint, sizeof(s_pointWas));
        s_haveWas = trackedLeft;
    }
    // The right glove: on the gun as the game holds it (as before), or, free hands, its own pose, hidden, or the game's (RightGlove); with what it works this frame
    // (its magazine, the launcher's reload in it, kept out of the gun).
    {
        RightWork work;
        work.magazine = &magazine;
        work.m320 = &m320Out;
        if (m320Side == 1 && haveM320Rebase) {
            work.m320Table = m320Table;
            std::memcpy(work.m320Rebase, m320Rebase, sizeof(work.m320Rebase));
            work.haveM320Rebase = true;
        }
        work.collided = rightFree;
        std::memcpy(work.shift, rightShift, sizeof(work.shift));
        work.shell = &shellOut;
        work.held = &heldMag;
        work.heldFingers = s_heldFingersRight;
        work.heldShare = s_heldFingersRightShare;
        work.heldFist = g_heldFistBy1;
        RightGlove(t, fromHead, toView, work);
    }
    // A tube shotgun's bolt and shells as drawn (skin.cpp's DrawHeldPart): its bolt back while held open or drawn by the handle; a shell lying in its
    // ejection port, one on its way up the loading port, one in the hand at work, in the glove as it's finally drawn.
    if (const shellload::Gun* tubeGun = skin::HeldTubeGun(); tubeGun && ammo::TubeLive() && skin::TubeByHand()) {
        t.heldTube = true;
        t.heldBoltBack = shellOut.active ? shellOut.boltBack : ammo::TubeLocked() ? tubeGun->travel : 0.0f;
        t.heldShellGame = ammo::TubeGameReload();
        auto add = [&](bool onGun, const float* pose) {
            if (t.heldShells >= static_cast<int>(std::size(t.heldShellPose)) || !Finite(pose, 12))
                return;
            t.heldShellOnGun[t.heldShells] = onGun;
            std::memcpy(t.heldShellPose[t.heldShells++], pose, sizeof(t.heldShellPose[0]));
        };
        if (ammo::TubePort()) {
            float port[12];
            shellload::ShellInPort(*tubeGun, port);
            add(true, port);
        }
        if (shellOut.lineShell)
            add(true, shellOut.shellOnGun);
        if (shellOut.handShell) {
            float inHand[12];
            if (shellOut.side == 0 && t.left)
                Compose(t.leftPose, shellOut.shellInGlove, inHand);
            else if (shellOut.side == 1 && t.rightGlove == skin::kRightOwn)
                Compose(t.rightGlovePose, shellOut.shellInGlove, inHand);
            else
                std::memcpy(inHand, shellOut.shellView, sizeof(inHand));
            add(false, inHand);
        }
    }
    // The launcher's left hand and its case (with nobody in the headset, the game's own hold on it, unless it's a test's hand). In the game's own reload the left is
    // the game's only while the right holds the pistol grip: with the left on it, or the gun lying or flying, both gloves stay the player's, its parts alone move.
    // (Here, before the arms: the game's own left hand on the gun has its arm solved with them.)
    t.launcherLeftFree = launcherHands && (trackedLeft || rocketTesting || heldTesting) &&
                         (!g_launcherGameReload || !(g_graspNow.holder == grasp::Holder::kHeld && g_graspNow.trigger == 1));
    // (Not by whether it's in hand: these are for its draws, and in the first frame of a raise it isn't known to be yet. Its case pulled off stayed drawn for that frame.)
    t.launcherGameHands = !skin::LauncherHands() || g_launcherGameReload;
    // The game's own left hand on a launcher-pose gun while the left isn't the player's (skin.cpp's DrawLauncherRig draws it by this same test: the left controller
    // not tracked, or the game's own reload with the right on the pistol grip): its arm is solved from where that hand is drawn on the gun, as the right's on the
    // grip is, so it's whole up to its shoulder. (Its glove and bare forearm were drawn there with the rest of the arm folded away: a forearm ending in an open cut
    // in mid-air, the films' review, 2026-11-11.) Only drawn: no hold or reload reads it.
    float rigLeft[12];
    bool haveRigLeft = false;
    if (launcherHeld && t.gun && t.rightOnPistol && !(t.launcherLeftFree && t.left) && skin::LeftHandOnRig(t.launcherGameHands, t.rigLeftHand)) {
        Compose(t.launcherPose, t.rigLeftHand, rigLeft);
        haveRigLeft = Finite(rigLeft, 12);
    }
    // Each forearm solved from its hand: the foundation of the arms, on every level.
    PlaceForearms(t, fromHead, toView, standInLeft, haveRigLeft ? rigLeft : nullptr);
    PlaceBareForearms(t, fromHead, toView, skin::BareOnRig() && (t.gun || t.launcherDrawn || t.stowed));
    // An arm solved into a number that isn't finite isn't drawn solved (its whole draw would go): the hand alone, logged.
    for (int side = 0; side < 2; ++side) {
        if (!t.arm[side])
            continue;
        bool finite = std::isfinite(t.fold[side][0]) && std::isfinite(t.fold[side][1]) && std::isfinite(t.fold[side][2]);
        for (int k = 0; k < 5 && finite; ++k)
            for (int i = 0; i < 12 && finite; ++i)
                finite = std::isfinite(t.forearm[side][k][i]);
        if (!finite) {
            t.arm[side] = false;
            g_armState[side] = armik::State{};  // (what it carries from frame to frame, its elbow's way and twist, starts afresh: a bad number there would stay)
            static ULONGLONG s_armAt = 0;
            if (GetTickCount64() - s_armAt > 1000) {
                s_armAt = GetTickCount64();
                Log("Arms: the %s arm solved into a number not finite: the hand alone this frame (the launcher's hand %d)", side == 0 ? "left" : "right",
                    static_cast<int>(m320Out.handState));
            }
        }
    }
    // (The game's own left hand's arm on a launcher-pose gun, alike: folded away for the frame, as it was before it had one.)
    if (t.rigLeftArm && !(Finite(&t.rigLeftForearm[0][0], 5 * 12) && Finite(t.rigLeftFold, 3))) {
        t.rigLeftArm = false;
        g_armState[0] = armik::State{};
        static ULONGLONG s_rigArmAt = 0;
        if (GetTickCount64() - s_rigArmAt > 1000) {
            s_rigArmAt = GetTickCount64();
            Log("Arms: the game's left hand's arm on the gun solved into a number not finite: folded away this frame");
        }
    }
    // (Said as it comes and goes, at most twice a second, for the films: a left controller dropping out for a frame or two isn't a line each time.)
    {
        static bool s_rigArmWas = false;
        static ULONGLONG s_rigArmSaid = 0;
        if (const ULONGLONG now = GetTickCount64(); t.rigLeftArm != s_rigArmWas && now - s_rigArmSaid >= 500) {
            s_rigArmWas = t.rigLeftArm;
            s_rigArmSaid = now;
            const char* gun = g_profile.pose != gunprofile::Pose::kNone ? g_profile.name : "gun";
            if (t.rigLeftArm)
                Log("Arms: the game's own left hand on the %s (the left not the player's): its arm solved from it, whole to the shoulder", gun);
            else
                Log("Arms: the game's own left hand on the %s no more: its arm gone with it", gun);
        }
    }
    if (t.haveChest && !(std::isfinite(t.chest[0]) && std::isfinite(t.chest[1]) && std::isfinite(t.chest[2])))
        t.haveChest = false;
    // Free hands: the rifle flying into a hand by its pistol grip (0.12 s), the drawn rifle only: its grip straight to the hand while it turns about it (the glove and
    // the arm are at the hand already). A regrip's was worked out with the steer above (the barrel hand's glove on it); should that hand be lost, from where it last was.
    if (g_graspNow.holder == grasp::Holder::kGlide && g_graspNow.trigger >= 0) {
        const int to = g_graspNow.trigger;
        g_kicked = 0;  // (nothing kicks while it flies into a hand: its pose is the glide's)
        if (g_regripDrawn) {
            std::memcpy(gunNow, g_glideDrawn, sizeof(g_glideDrawn));
        } else {
            float from[12], drawn[12];
            Compose(fromHead, g_glideFrom, from);
            BlendAbout(from, g_handGun[to], g_graspNow.glide, g_anchorBy[to], drawn);
            if (Finite(drawn, 12))
                std::memcpy(gunNow, drawn, sizeof(drawn));
        }
        (lpNow ? t.launcherDrawn : t.gunDrawn) = true;
    }
    // The grenade launcher's parts as its reload by hand has them (skin.cpp's M320PartNow: as the game has them when it doesn't), and a round in the hand drawn at the glove
    // as finally drawn.
    t.m320Own = m320Out.drawn;
    t.m320Barrel = m320Out.drawn ? m320Out.barrel : 0.0f;
    t.m320Round = !m320Out.drawn ? 0 : m320Out.round == m320::Round::kHidden ? 1 : m320Out.round == m320::Round::kOnGun ? 2 : 0;
    t.m320RoundOut = m320Out.drawn ? m320Out.roundOut : 0.0f;
    std::memcpy(t.m320RoundOnGun, m320Out.roundOnGun, sizeof(t.m320RoundOnGun));
    t.m320HandRound = m320Out.handRound;
    if (m320Out.handRound) {
        // (Free hands: a round in the right hand at the right glove as drawn, its own pose; each hand's last good one its own.)
        const int hs = m320Out.handSide == 1 ? 1 : 0;
        if (hs == 0 ? t.left : t.rightGlove == skin::kRightOwn) {
            // (In the glove as the module has it: re-anchored on the palm drawn, the pinch going over to the carry. The module catches it where it's drawn.)
            Compose(hs == 0 ? t.leftPose : t.rightGlovePose, m320Out.roundInGlove, t.m320HandRoundPose);
            // Never a number not finite (its whole draw would go: the user saw the round "disappear and come back"): the last good one, logged.
            static float s_goodRound[2][12];
            static bool s_haveGoodRound[2] = {};
            bool finite = true;
            for (int i = 0; i < 12; ++i)
                finite = finite && std::isfinite(t.m320HandRoundPose[i]);
            if (finite) {
                std::memcpy(s_goodRound[hs], t.m320HandRoundPose, sizeof(s_goodRound[hs]));
                s_haveGoodRound[hs] = true;
            } else {
                static ULONGLONG s_at = 0;
                if (GetTickCount64() - s_at > 1000) {
                    s_at = GetTickCount64();
                    bool inGlove = true;
                    for (int i = 0; i < 12; ++i)
                        inGlove = inGlove && std::isfinite(m320Out.roundInGlove[i]);
                    Log("M320: the round in the %s came out with a number not finite (its place in the glove %s): drawn where it last was", hs == 1 ? "right hand" : "hand",
                        inGlove ? "finite" : "NOT finite");
                }
                if (s_haveGoodRound[hs])
                    std::memcpy(t.m320HandRoundPose, s_goodRound[hs], sizeof(s_goodRound[hs]));
                else
                    t.m320HandRound = false;
            }
        } else
            std::memcpy(t.m320HandRoundPose, m320Out.handRoundView, sizeof(t.m320HandRoundPose));
    }
    // The loose things let go of, moved once a frame in the world (after the gun's settled: they slide out of it with it).
    // The game's own reload of the grenade launcher: its old case let go of on the way to the hip.
    GameReloadCase(t, headPose, frameSeconds, g_m320InUse && m320Out.content == m320::Content::kNone);
    // (Its gun the frame of what's in the right hand: a magazine out of a held gun's well, or the launcher's case off its back, goes with that gun's own speed and turn.)
    // (By the gun as the hands hold it: a kick's jolt doesn't shove what's in its well, nor send what leaves it with the kick's speed.)
    StepLoose(t, t.gun, AsHeld(t, launcherHeld ? t.launcherPose : pistolHeld ? t.pistolPose : t.gunPose), headPose, fromHead, frameSeconds, trackedLeft);
    // The launcher's case, once the launcher's place is final (let go of this frame, StepLoose has it lying already: its case lies on it from that frame).
    t.launcherCase = rocketOut.show == rocket::Show::kGame ? 0 : rocketOut.show == rocket::Show::kNone ? 1 : 2;
    if (rocketOut.show == rocket::Show::kAt) {
        if (rocketOut.onFrame && t.launcherLying) {  // lying: on its frame, drawn with the launcher as it's drawn there
            std::memcpy(t.launcherCaseRel, rocketOut.rel, sizeof(t.launcherCaseRel));
            t.launcherCase = 3;
        } else if (rocketOut.onFrame && t.stowed) {
            t.launcherCase = 1;  // (on its frame, the launcher drawn nowhere: it goes with the launcher; drawn alone only in a hand)
        } else if (rocketOut.onFrame) {  // as the launcher is now, the left hand's steer and all
            Compose(t.launcherPose, rocketOut.rel, t.launcherCasePose);
        } else {
            std::memcpy(t.launcherCasePose, rocketOut.pose, sizeof(t.launcherCasePose));
        }
    }
    // Free hands: the gun as drawn this frame (the room's; lying, loose's; on its own pose), for the next frame's grasp; and the game's aim from it.
    // (As the hands hold it, for what judges what they do; and as drawn, kicked, for where a let-go falls from: the same with no kick.)
    if (g_graspNow.holder == grasp::Holder::kLoose) {
        g_haveLastGunRoom = g_gunLoose.have;
        if (g_gunLoose.have) {
            std::memcpy(g_lastGunRoom, g_gunLoose.room, sizeof(g_lastGunRoom));
            std::memcpy(g_lastGunDrawn, g_gunLoose.room, sizeof(g_lastGunDrawn));
        }
    } else if (t.gun || (lpNow ? t.launcherDrawn : t.gunDrawn)) {
        Compose(headPose, AsHeld(t, gunNow), g_lastGunRoom);
        Compose(headPose, gunNow, g_lastGunDrawn);
        g_haveLastGunRoom = Finite(g_lastGunRoom, 12);
        if (!Finite(g_lastGunDrawn, 12))
            std::memcpy(g_lastGunDrawn, g_lastGunRoom, sizeof(g_lastGunDrawn));
    } else {
        g_haveLastGunRoom = false;
    }
    PublishAim(t, head, aim, headTurn, frameSeconds);
    // A gun taken off the ground by hand is due (g_pick.landedSide): till its own draws are known it's drawn where it lay (lying, from the world) on its
    // own pose's targets, so its first frame is there, never as the game holds it. Only on the pose the gun that lies doesn't use; not for the same gun.
    t.arriveKind = 0;
    if (g_pick.landedSide >= 0 && !g_pick.sameKind && g_pick.landedId && !g_pick.byUse && g_graspNow.holder == grasp::Holder::kLoose && g_haveLooseFrame &&
        pickups::Tuning().preArm) {
        const bool rifle = g_pick.landedId == gunprofile::kRifleId;                           // its pose: the M416's gunPose, else launcherPose
        const bool lyingRifle = g_gunLoose.id == 0 || g_gunLoose.pose != gunprofile::Pose::kLauncher;
        float world[12];
        std::memcpy(world, g_pick.landedWorld, sizeof(world));
        world[3] -= static_cast<float>(g_looseFrame.origin[0]), world[7] -= static_cast<float>(g_looseFrame.origin[1]), world[11] -= static_cast<float>(g_looseFrame.origin[2]);
        if ((rifle != lyingRifle || g_gunLoose.id == 0) && Finite(world, 12)) {
            (rifle ? t.gunDrawn : t.launcherDrawn) = true;
            (rifle ? t.gunLying : t.launcherLying) = true;
            std::memcpy(rifle ? t.gunWorld : t.launcherWorld, world, sizeof(world));
            for (int i = 0; i < 3; ++i)
                (rifle ? t.gunWorldOrigin : t.launcherWorldOrigin)[i] = static_cast<float>(g_looseFrame.origin[i]);
            t.arriveKind = g_pick.landedId;
        }
    }
    // (Free hands: the rifle in the left hand or held by its handguard alone with that hand lost: the game's own arms and gun until it's seen. Never while
    // the gun is drawn nowhere, put away or its hand taken to be at a place: that would be the game's own arms and gun in front of the eyes.)
    const bool holderLost = !t.gun && ((g_graspNow.holder == grasp::Holder::kHeld && g_graspNow.trigger == 0) || g_graspNow.holder == grasp::Holder::kBarrel);
    t.lyingTwoSided = freetune::TwoSided();
    skin::SetHands(t.stowed || (!holderLost && (t.gun || t.gunDrawn || t.launcherDrawn || t.left || t.rightGlove != skin::kRightGame)) ? &t : nullptr);
}

bool On() {
    return g_on;
}

const loose::Frame* LooseFrame() {
    return g_haveLooseFrame ? &g_looseFrame : nullptr;
}

int M320HeldBy() {
    if (!g_on)
        return -1;
    for (int side = 0; side < 2; ++side)  // (one hand holds it at a time; the left asked first)
        if (g_supportingBy[side] && g_onM320By[side] && !g_holdOutOfReachBy[side])
            return side;
    return -1;
}

bool M320Held() {
    return M320HeldBy() >= 0;
}

bool HeldChamberEmpty() {
    return g_on && !workhand::HeldMayFire(g_heldDry, g_beltBlocked, g_notchBlocked);
}

void HeldChambered(bool newLife) {
    g_heldDry = false;
    if (newLife) {
        g_heldLoadedWas = -1;
        g_heldFiringWas = 0;  // (its guns are new objects: met afresh, never another gun's)
        handmag::BeltReset();
        m320::Clear();  // (the grenade launcher shut and loaded, nothing in the hand)
        loose::Clear();  // (a new life, a checkpoint, another level: what lay on the ground is in another world now)
        recoilrun::Clear();  // (no kick carried into a new life)
        g_newLife = true;  // (free hands: the rifle back in the right hand, given again, at the next Update)
    }
}

void TubeForgotten() {
    // (A shell in a hand or part way up the port, a handle held, the bolt drawn back for a count that is no more: without this they stayed for the
    // gun met next if it was in the hands again within shellload's 0.2 s out of use, a checkpoint loaded with a shell in the hand.)
    shellload::Clear();
    g_tubeBoltHeldNow = false;
}

int FireHand() {
    return g_on && FreshFreeHands() ? g_fireHandNow.load(std::memory_order_relaxed) : 1;
}

bool TwoHands() {
    return g_on && FreshFreeHands() && g_twoHandsNow.load(std::memory_order_relaxed);
}

bool HoldsGun() {
    return !g_on || !FreshFreeHands() || g_holdsGunNow.load(std::memory_order_relaxed);
}

int GripHand() {
    return g_on && FreshFreeHands() ? g_gripHandNow.load(std::memory_order_relaxed) : 1;
}

bool GunStowed() {
    return g_on && FreshFreeHands() && g_stowedNow.load(std::memory_order_relaxed);
}

// (Asked by input.cpp before this frame's Update, on the same thread: holster.h's own answer on the state the last frame left, as its Update gives the
// draw for that pick from the same rule.)
bool PickIsOwnSlot(int slot) {
    return g_on && FreshFreeHands() && g_freeOn && holster::PickIsOwnSlot(slot, g_holster, HolsterHolder(g_grasp.holder), g_holsterSet);
}

const char* StowedWhere() {
    return GunStowed() ? g_stowedWhere : "";
}

bool TakeWouldSwapHolstered(char* words, size_t size) {
    if (words && size)
        words[0] = 0;
    if (!g_on || !FreshFreeHands() || !g_holsterSet.on)  // (free hands asleep too: the guns at the shoulders stay there whatever the game draws)
        return false;
    int entries = 0, entriesAll = 0, entrySlot[pickups::kMostContent] = {-1, -1, -1, -1};
    bool entryGun[pickups::kMostContent] = {}, replaceAll = false, unspawnOrStatic = false;
    const char* name = "";
    pickups::Kind kind = pickups::kNoKind;
    if (!pickups::GameTargetTake(entries, entrySlot, entryGun, replaceAll, unspawnOrStatic, &name, &entriesAll, &kind))
        return false;
    // (A take that puts a holstered gun into its pickup goes ahead: the gun stays on its shoulder, in that pickup. Only a plain floor gun's that couldn't
    // give it back; the level's own goes ahead all the same, said.)
    const holster::TakeOut check = HolsterTakeCheck(entries, entriesAll, entrySlot, entryGun, replaceAll, unspawnOrStatic, kind);
    if (check.verdict == holster::TakeVerdict::kAway && check.lost)
        SayTakeLost(name, check);
    if (check.verdict != holster::TakeVerdict::kRefuse || check.slot < 0)
        return false;
    if (words && size) {
        char gun[48];
        HolsterGun(check.slot, gun, sizeof(gun));
        snprintf(words, size, "the %s would take %s off %s (slot %d), and %s", name, gun, holster::Name(check.at), check.slot,
                 TakeRefusedWords(check.why));
    }
    return true;
}

// (Asked by input.cpp before this frame's Update, on the same thread, as PickIsOwnSlot: the grasp and the take as the last frame left them. Only while
// the hands can drop the gun or put it away: free hands awake, no scripted moment (the hands frozen then); else the game's own key stays the way, so no
// prompt is ever out of reach.)
bool GunBarsGift(char* words, size_t size) {
    if (words && size)
        words[0] = 0;
    return g_on && FreshFreeHands() && g_freeOn && !g_scripted.load(std::memory_order_relaxed) && GunInAHand(words, size);
}

void RefuseGift(const char* by) {
    char held[128];
    if (!GunBarsGift(held, sizeof(held)))
        held[0] = 0;
    Log("Input: %s at the prompt on the reload key (it gives a gun): R not sent: %s: drop it or put it on a shoulder first (a prompt that gives a gun "
        "is used with no gun in either hand)", by && *by ? by : "X", held[0] ? held : "a gun is in a hand");
    // The double click (usepoint's, its second pulse at the next Updates) on the hand that holds the gun; both when none does by its grip or front.
    const int side = g_grasp.holder == grasp::Holder::kBarrel ? g_grasp.barrel
                     : g_grasp.holder == grasp::Holder::kHeld || g_grasp.holder == grasp::Holder::kGlide ? g_grasp.trigger
                                                                                                          : -1;
    for (int s = 0; s < 2; ++s) {
        if ((side == 0 || side == 1) && s != side)
            continue;
        const usepoint::Pulse pulse = usepoint::Refuse(g_usePoint, s);
        input::Buzz(s, pulse.amplitude, pulse.seconds);
    }
}

bool ScriptEject() {
    // (Set within the last 2 s too: a round that stopped running never leaves it held.)
    return g_on && FreshFreeHands() && g_scriptEjectNow.load(std::memory_order_relaxed) &&
           GetTickCount64() - g_scriptEjectAt.load(std::memory_order_relaxed) < 2000;
}

bool Aim(XrQuaternionf& out) {
    if (!g_on || !g_aimOwn || !FreshFreeHands())
        return false;
    out = g_aimNow;
    return true;
}

void SetScripted(bool scripted) {
    g_scripted.store(scripted, std::memory_order_relaxed);
}

bool StingerEmpty() {
    return g_on && g_stingerEmptyNow.load(std::memory_order_relaxed) && skin::HeldAerial();
}

bool At4Spent() {
    return g_on && g_at4SpentNow.load(std::memory_order_relaxed) && skin::At4Held();
}

bool AimOrientation(XrQuaternionf& out) {
    if (!g_on || !g_aimTurned)
        return false;
    out = g_aimTurn;
    return true;
}

bool PistolSlideHeld() {
    const ULONGLONG at = g_slideHeldAt.load(std::memory_order_relaxed);
    return g_on && at && GetTickCount64() - at < 200;
}

bool PistolMagazineTap(bool fromScript) {
    return PistolMagazineRelease(fromScript);
}

void PistolGameReload() {
    float hold[12];
    if (!g_on || !g_pistolMagOn || !skin::BarePistolHand(hold))
        return;
    // The game's own reload: its animation brings its magazine back in, so the mod's is in again. One part way into the
    // grip drops out first; one in the left hand is gone (pistolmag: another went in).
    if (pistolmag::Parked()) {
        skin::DropPistolMagazine(skin::PistolDrop::kWell);
        pistolmag::Clear();
    }
    // (With its rounds by hand the magazine goes back in when the game's count rises, as the rifles': the game may
    // refuse the reload, with no spare rounds.)
    if (PistolRoundsLive()) {
        Log("Pistol reload: X held: the game's own reload asked for; its magazine goes back in the grip once the game's count rises");
        return;
    }
    if (g_pistolMagOut) {
        g_pistolMagOut = false;
        Log("Pistol reload: X held: the game's own reload puts its magazine back in the grip");
    }
}

bool PistolMagazineOut() {
    // (With its rounds by hand the chambered round still fires with the magazine out: PistolDry holds the trigger off.)
    if (g_pistolRoundsOn && ammo::PistolLive())
        return false;
    const ULONGLONG at = g_pistolMagOutAt.load(std::memory_order_relaxed);
    return g_on && at && GetTickCount64() - at < 200;
}

// A state, not a time, as LauncherEmpty.
bool PistolDry() {
    return g_on && g_pistolRoundsOn && ammo::PistolLive() && ammo::PistolDry() && skin::TrainPistolInHand();
}

void PistolDryPull() {
    g_pistolDryPull.store(true, std::memory_order_relaxed);
}

// A tube shotgun with its shells by hand (the M1014), as the last Update had it: in a hand; nothing to fire (no live shell chambered, or its bolt held
// open); its bolt handle held back.
bool TubeInHand() {
    return g_on && g_tubeInHand.load(std::memory_order_relaxed) && skin::HeldTubeGun() != nullptr;
}

bool TubeDry() {
    return TubeInHand() && ammo::TubeLive() && ammo::TubeDry();  // (the count's own word, as its last look had it: not a frame older)
}

bool TubeBoltHeld() {
    return TubeInHand() && g_tubeBoltHeldNow.load(std::memory_order_relaxed);
}

void TubeDryPull() {
    g_tubeDryPull.store(true, std::memory_order_relaxed);
}

// A state, not a time: input.cpp asks before this frame's Update, and a long frame let the trigger through on an empty launcher, firing the hidden round.
bool LauncherEmpty() {
    return g_on && g_launcherEmptyNow.load(std::memory_order_relaxed) && skin::LauncherInHand();
}

void LauncherGameReload() {
    if (!g_on || g_launcherGameReload || !skin::LauncherInHand() || !LauncherEmpty())
        return;
    // With none left there's nothing for it to reload: its state would never end.
    int rockets = 0, spareRockets = 0;
    if (ammo::LauncherGet(rockets, spareRockets) && spareRockets < 1)
        return;
    g_launcherGameReload = true;
    g_gameReloadSince = GetTickCount64();
    g_gameReloadCountedAt = 0;
    ammo::LauncherGameReload();
    Log("Rocket: X held: the game's own reload");
}

void SetSteerCone(float fullDegrees, float noneDegrees) {
    const bool body = fullDegrees < 0.0f;
    if (g_steerByBody.exchange(body) != body)
        Log("Hands: the left hand %s", body ? "aims the gun while it's out ahead of the body (the right hand levels it)"
                                            : "steers within a cone round the right hand's line (weapon.txt steer)");
    if (body)
        return;
    const float full = std::fmin(std::fmax(fullDegrees, 1.0f), 170.0f) * steer::kDegree;
    const float none = std::fmin(std::fmax(noneDegrees, fullDegrees + 1.0f), 179.0f) * steer::kDegree;
    if (g_steerFull.exchange(full) != full || g_steerNone.exchange(none) != none)
        Log("Hands: the left hand steers the gun fully within %.0f degrees of the right hand's line, not at all past "
            "%.0f",
            full / steer::kDegree, none / steer::kDegree);
}

void Toggle() {
    g_on = !g_on;
    if (!g_on) {
        skin::SetHands(nullptr);
        pouch::Clear();
        pistolmag::Clear();
        rocket::Clear();
        m320::Clear();
        shellload::Clear();  // (the hands turned off: no shell in one, no handle held)
        g_onLauncher = false;
    }
    // Free hands on the rifle start afresh either way: back in the right hand, given again (a rifle lying is gone from loose; one put away, from its place:
    // what the holsters waited for goes too).
    if (ResetGrasp())
        Log("Free hands: the rifle back in the right hand (%s)", g_on ? "free hands on again" : "free hands off");
    holster::Reset(g_holster);  // (what waited goes; every gun keeps its holster, and the guns away stay on their shoulders)
    g_stowedNow = false;
    g_stowedId = g_sleptStowedId = 0;
    recoilrun::Clear();  // (and its kick gone)
    Log("Hands: %s", g_on ? "free (the gun in the right hand, the left hand on its own)" : "the game's own");
    Beep(g_on ? 1200 : 440, 200);
}

}

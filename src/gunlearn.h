#pragma once

// Guns never seen before (free hands for every gun, phase 9): a first-person gun with no table row in skin.cpp is watched while the game draws it as
// its own; its body draw is found (the largest colour draw with the player's bones in the first-person pass, drawn in 27 of 30 frames); its mesh is
// read back from the graphics card once (the index format, the draw inside its index buffer and every index inside its vertex buffer checked before a
// vertex is read); its frame bone (the bone most of its vertices go with), its depth pass (the depth-only draw in the same view whose palette holds the
// frame's matrix), its body when it's let go of (bodyfit.h, as tools/gunbodyfit.exe fits the known guns) and where the game's palms rest on it are
// learned; only then do the hands take it. Learned guns are kept in heldguns.txt beside the DLL: at the next launch one is held as soon as its mesh's
// key is checked (its vertex count at once, a hash of its first indices from a 4 KB readback). A mesh refused for what it is (too small for a gun, a
// layout this doesn't know) is kept there too and never read back again; one refused for the palette it was drawn with that once (its frame bone
// not rigid, its body posed out of reach, a body that would stand on a narrow bottom), or with no room left to read it, only till the next launch.
// Off unless weapon.txt says "learnguns 1", or "learnguns test <count>" (a known gun learned as new and checked against its table row); switched
// off, no gun learned is found (the game's own again) until it's on again.
//
// Threads and locks. The game records its draws on several threads: NoteDraw runs on them, OnFrame on the thread that presents (the immediate
// context's), NotePalms and NoteSights where skin.cpp learns the rest and the sights. The rows are one table published whole with release, never
// changed once published and never freed: Find and ByBody read it with no lock, from any thread, and the rows they give stay valid for good.
// Everything else is under this file's one mutex. The lock order is skin.cpp's g_handsMutex first, then this file's:
//   - a caller may hold g_handsMutex when it calls in (NotePalms from LearnRig);
//   - gunlearn.cpp never calls skin.cpp, hands.cpp or anything else that takes g_handsMutex, so its mutex is never held while that one is waited for;
//   - skin.cpp calls NoteDraw outside g_handsMutex, OnFrame outside skin::Update's locked block, Find and ByBody anywhere;
//   - the readback (CreateBuffer, CopyResource, Map, Unmap) runs on OnFrame's thread with this file's mutex not held;
//   - the fit (some 20-30 ms for a rifle) and heldguns.txt's read and write run on short threads of their own, never in a frame's work: they
//     take only this file's mutex, and only to take their result.

#include <cstdint>
#include <string>

struct ID3D11Device;
struct ID3D11DeviceContext;
namespace loose {
struct Spec;
}

namespace gunlearn {

struct Row {
    uint32_t bodyIndices = 0, bodyStart = 0, depthIndices = 0, depthStart = 0;
    int frameBone = -1, depthFrame = -1;
    bool palms = false; float rightPalm[3] = {}, leftPalm[3] = {};
    bool sightsLearned = false; float up = 0.0f, sights[3] = {}, muzzle[3] = {}, port[3] = {};
    const loose::Spec* body = nullptr; float hullMin[3] = {}, hullMax[3] = {};
    bool ready = false;  // the frame, the depth pass, the body and the palms known: the hands hold it
    // Its mesh's key with its body's count and start: its vertex buffer's vertices (ByteWidth / stride) and the FNV-1a of its first 4 KB of indices.
    uint32_t vertices = 0, indexHash = 0;
};
// The test gun's table row ("learnguns test <count>"), to compare with. shape: its body in gunbodies.h ("an94"), compared within 1 mm.
struct Check {
    uint32_t bodyIndices = 0, bodyStart = 0, depthIndices = 0, depthStart = 0;
    int frameBone = -1, depthFrame = -1;
    float rightPalm[3] = {}, leftPalm[3] = {}, up = 0.0f, sights[3] = {};
    const char* shape = nullptr;
};
bool On();                                       // weapon.txt "learnguns" not 0
void Set(int mode, uint32_t testBody);           // 0 off, 1 on; testBody: a known gun learned as new ("learnguns test <count>")
uint32_t TestBody();
void SetCheck(const Check& c);                   // skin.cpp: the test gun's table row, to compare with
// A draw of the first-person pass (its stencil) with the player's own bones, drawn while no gun the mod knows was drawn in the last 2 frames, and no
// arm draw: the candidates, the gun's depth pass and the draws of a gun not ready yet included. palette: its paletteBones bones as uploaded (3x4
// rows a bone, at least that many there). frame: the frames shown so far; view: the first-person views uploaded so far (a view's draws share it).
void NoteDraw(ID3D11DeviceContext* ctx, uint32_t count, uint32_t start, int base, const float* palette, uint32_t paletteBones, uint32_t frame, uint32_t view);
void OnFrame(ID3D11Device* device, ID3D11DeviceContext* immediate);
const Row* Find(uint32_t count, uint32_t start);  // by its colour or depth draw, ready or not; none while it's switched off
const Row* ByBody(uint32_t bodyIndices);           // none while it's switched off
void NotePalms(uint32_t bodyIndices, const float* right, const float* left);
void SetSightLearning(bool on);
void NoteSights(uint32_t bodyIndices, const float* eyeOnFrame, const float* forwardOnFrame);
// skin.cpp's own Map and Unmap of the immediate context, past its hooks (g_origMap[0], g_origUnmap[0]), for the readback. Unset, the context's own are
// used (skin.cpp's hooked Map passes a read straight through).
void SetMap(void* map, void* unmap);
// learntest: its own file instead of heldguns.txt beside the module; and everything forgotten, as at a new launch (the file read again at the next
// OnFrame; called on OnFrame's thread). Rows published before stay valid.
void SetStore(const wchar_t* path);
void Forget();
uint32_t Readbacks();  // learntest: meshes read back so far (a mesh refused is never read again)
bool Loaded();         // learntest: the file read since the last Forget (or found not there)

// ---- The learner's own steps, pure: no game, no graphics card, no logs (learntest runs them on the mesh dumps) ----

constexpr uint32_t kBodyLeast = 2000;          // indices: a smaller draw is a gun's part, never its body or its depth pass
constexpr int kWindow = 30, kWindowSeen = 27;  // frames watched, and in how many of them the body is drawn (90%)
constexpr int kMostBones = 64;                 // a palette's bones read: the frame bone and the depth pass's below this
constexpr int kMostHull = 22;                  // hull points (loose's)
constexpr float kGunLeast = 0.30f;             // m: the kept mesh's length along z at least this (grenades and the knife are 0.1-0.25)

// FNV-1a (32-bit) of at most the first 4 KB of a draw's indices, from its start: with its vertex count, its mesh's key.
uint32_t IndexHash(const uint8_t* indices, uint32_t bytes);
// The first bone of the palette equal to frame, each of its 12 numbers within tolerance, or -1: the depth pass's frame bone.
int MatchBone(const float* frame, const float* palette, uint32_t bones, float tolerance = 1e-5f);

// The watch: the frames each candidate draw is seen in, a window of kWindow frames at a time. A window closes at a frame kWindow or more after it
// opened: Pick then gives its body, and Next opens another.
struct Watch {
    struct Seen {
        uint32_t count = 0, start = 0;
        int base = 0;
        uint32_t frames = 0, lastFrame = 0;
        bool colour = false;  // drawn with render targets (the depth pass has none)
    };
    static constexpr int kMost = 48;
    Seen seen[kMost];
    int seenCount = 0;
    bool open = false;
    uint32_t windowStart = 0;
    bool Closes(uint32_t frame) const;  // this frame closes the window open
    void Note(uint32_t count, uint32_t start, int base, uint32_t frame, bool colour);
    // The window's body: the largest colour draw by its count (at least kBodyLeast) seen in kWindowSeen of its frames, not one of skip (count, start
    // pairs); its index in seen, or -1.
    int Pick(const uint32_t (*skip)[2], int skips) const;
    void Next(uint32_t frame);
};

// A body draw as read back: its buffers' bytes whole (as long as their ByteWidth), where the draw is in them, and the palette it was drawn with.
struct Capture {
    const uint8_t* ib = nullptr;
    uint32_t ibBytes = 0, ibOffset = 0;
    uint32_t ibFormat = 0;  // DXGI_FORMAT: R16_UINT (57) or R32_UINT (42), anything else refused
    const uint8_t* vb = nullptr;
    uint32_t vbBytes = 0, vbOffset = 0, stride = 0;
    uint32_t count = 0, start = 0;
    int base = 0;
    const float* palette = nullptr;  // 3x4 rows a bone (rotation | translation), as uploaded
    uint32_t paletteBones = 0;
};
// What's learned from it, on its frame bone (x left, y up, z forward; metres).
struct Fitted {
    bool ok = false;
    const char* why = "";  // why it was refused, for the log
    bool forGood = false;  // refused for what its mesh is (kept in heldguns.txt), not for the palette drawn with it that once (till the next launch)
    uint32_t used = 0;     // the vertices its triangles use
    int frameBone = -1, frameVertices = 0;
    uint32_t vertices = 0, indexHash = 0;  // its mesh's key
    float keptMin[3] = {}, keptMax[3] = {};  // its kept mesh's box (parked parts left out)
    float boxMin[3] = {}, boxMax[3] = {}, hullMin[3] = {}, hullMax[3] = {}, centre[3] = {}, keelX = 0.0f;  // its body (bodyfit::Result's)
    int hullPoints = 0;
    float hull[kMostHull][3] = {};
    int leftOut[kMostBones] = {}, leftOutCount = 0;
    float muzzle[3] = {}, sights[3] = {}, port[3] = {};
};
// Checks the draw (the index format; the draw inside its index buffer; every index with the base inside its vertex buffer; the layout: half positions
// at 0, four bone bytes at 8, four weight bytes at 12, the weights summing 250-260 and the places within 2 m on 99% of its vertices, a vertex naming a
// bone past the palette left out but no more than 1% of them; the frame bone rigid), poses it into its frame bone's space as tools/gunbodyfit.exe
// does (so its body is the one gunbodyfit fits), fits its body and checks it's gun-sized, and that under a narrow bottom it touches upright on its
// keel alone (tools/bodytest.cpp b6's rule: it never stands). Reads nothing outside the buffers it's given.
bool FitCapture(const Capture& c, Fitted& out);
// A kept mesh's box a gun's: at least kGunLeast long along z, and longer along z than across or up.
bool GunSized(const float* keptMin, const float* keptMax);

// A learned gun as a line of heldguns.txt (its row and its body's numbers; the row's body pointer isn't kept), and a mesh refused for good.
struct Saved {
    Row row;
    float boxMin[3] = {}, boxMax[3] = {}, centre[3] = {}, keelX = 0.0f, mass = 0.0f;
    int hullPoints = 0;
    float hull[kMostHull][3] = {};
};
struct Refused {
    uint32_t count = 0, start = 0, vertices = 0;
    char why[64] = {};
};
std::string FormatRow(const Saved& s);  // one line, no line end
bool ParseRow(const char* line, Saved& s);
std::string FormatRefused(const Refused& r);
bool ParseRefused(const char* line, Refused& r);

}  // namespace gunlearn

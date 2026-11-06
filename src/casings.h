#pragma once

#include <d3d11.h>

#include <atomic>
#include <cstdint>
#include <functional>

namespace loose {
struct Frame;
struct Pose;
struct Result;
struct Spec;
}

// The spent casings of the gun in the hands, the mod's own (the user, 2026-11-05: "When I take out the empty shell, I want all of that, and I want it
// to hit the floor. I want it to stay there for a minute. Same thing as the bullet casings"). The game's own casings are particles with no collision
// that live under a second and a half; these are loose things (loose.h's casings), thrown out of the drawn gun's ejection port with the game's own
// emitter numbers for each round, landing on the game's own ground and lying a minute. They're drawn with the game's own casing draw: its mesh,
// material, shaders and states, kept from one of its casing draws in an eye's picture, all of a kind in one instanced draw at the start of each eye's
// own pass of the world (its G-buffer), so they have that eye's view, its depth against the world and its lighting. While the mod's are thrown, the
// game's own from the player's gun aren't drawn.
//
// weapon.txt "casings 2" (SetMode(2)) turns it on; anything else leaves all of this off: one comparison in each draw hook, and nothing kept, made or
// drawn ("casings 1" and "casings 0" stay as effects.cpp has them).
//
// The world here is the game's own (metres, y up); loose's arms' space plus its origin in whole metres.
namespace casings {

// What's thrown: the game's casings, by their mesh. Each is loose's kind kLooseKind plus this (10 the 5.56, 11 the 5.45, 12 the 7.62, 13 the 9 mm;
// skin.cpp's LooseKind uses 1 to 4).
enum Kind { k556, k545, k762, k9, kKinds };
constexpr int kLooseKind = 10;
constexpr bool IsLooseCasing(int looseKind) { return looseKind >= kLooseKind && looseKind < kLooseKind + kKinds; }

// ---------------------------------------------------------------- the throw (pure: no game calls and no logs; tools/casingtest.cpp)

// A gun's casing as the game throws it: its first-person shell effect's FireEffectData offset and turn on the gun, and that effect's casing emitter
// (the public Venice EBX dump of the game's data; the design study of 2026-11-05).
struct Emitter {
    const char* shape;          // skin::HeldShape()'s name for it, or "m416", "m9"
    Kind kind;
    float port[3];              // where the effect starts on the gun's frame bone (x left, y up, z forward), m
    float turn[3];              // the effect's own turn on the gun, degrees about x, y and z
    float zoomTurn[3];          // ... with the eye at the sights (the game's UseZoomRotation: zoomed)
    float dirMin[3], dirMax[3]; // its SpawnDirection's random box, in the effect's axes (out to the right, up, a little back)
    float speedMin, speedMax;   // m/s, its SpawnSpeed's random
    float spinMin, spinMax;     // rad/s, its SpawnRotationSpeed's random
    float life;                 // s: how long the game's own lives (UpdateAgeData), what its hiding has to cover
};
// By name (skin::HeldShape()'s, or "m416" for the M416, "m9" for the train's pistol). Null: none thrown (the launchers, the Stinger, a knife,
// anything not in the table).
const Emitter* EmitterFor(const char* shape);

// One round's casing: what loose::Flick takes for it.
struct Throw {
    const Emitter* gun = nullptr;
    float gunRoom[12] = {};     // the gun's frame bone as drawn, in the room: loose::Frame::gunRoom
    float roomToArms[12] = {};  // loose::Frame::roomToArms
    // The head in the room (the middle of the eyes): within 15 cm of the port and behind it (the eye at the sights), the effect's zoom turn, as the
    // game turns its casings away from the face zoomed.
    bool haveEye = false;
    float eyeRoom[3] = {};
    const float* port = nullptr;  // another port on the frame bone (null: the emitter's)
    uint32_t seed = 0;            // any number: its own random throw
};
struct Start {
    const loose::Spec* spec = nullptr;  // loose::SpecFor's "556", "545", "762" or "9mm"
    int kind = 0;                       // loose's kind: kLooseKind + the emitter's
    float bone[12] = {};                // the casing's own space (loose's: its base's middle at 0, its mouth along +z) into the arms' space: along the bore, its middle at the port
    float velocity[3] = {};             // m/s, the arms' axes: its own throw (loose::Flick adds the gun's own speed and turn there, and the walking)
    float axis[3] = {};                 // its spin's axis, the arms' axes, unit
    float spin = 0.0f;                  // rad/s
    bool zoomed = false;                // thrown with the zoom turn
};
// False: no emitter, broken numbers, or no spec for its kind.
bool ShotStart(const Throw& t, Start& out);

// Rounds fired between two frames by the count of rounds in the gun (ammo::RoundsInHand) going down: 1 to 3, and only with the trigger held lately
// (a magazine let go of with two rounds left isn't two shots). -1 for either count: not known, none.
int RoundsFired(int before, int now, bool triggerHeldLately);

// A casing's world placement as the game's own casing draw takes it, one instance (stream 1, 48 bytes): three rows, rotation | the place in the world,
// its mesh's vertices into the world. bone: loose's pose (the casing's own space into the arms' space with origin); specFromMesh: the mesh's vertices
// into the casing's own space (MeshFit).
void PackInstance(const float* bone, const int* origin, const float* specFromMesh, float* rows);

// The mod's casings of each kind in a row, each kind's together (loose's poses of any kinds: only the casings are taken, and only of a kind whose fit
// is known). rows: room for most instances of 12 floats; first and count by kind. The number packed.
int PackAll(const loose::Pose* poses, int poseCount, const int* origin, const float (*specFromMesh)[12], const bool* fitKnown, float* rows, int most,
            int* first, int* count);

// How the game's casing mesh lies in its own vertices (read from them, scaled as the game draws it): along which axis, from which end, and its sizes.
struct MeshFit {
    float specFromMesh[12] = {};  // its vertices (in the game's units) into the casing's own space, scale and all
    int axis = 2;                 // 0 x, 1 y, 2 z: the one it's long along
    bool baseAtMax = false;       // its base at that axis's highest end
    bool ambiguous = false;       // its two ends alike (a straight 9 mm): the lowest end taken for its base
    float scale = 1.0f;           // the game's instance's own scale
    float length = 0.0f, baseRadius = 0.0f, mouthRadius = 0.0f;  // m, as drawn
};
// From its vertices' places (count of them, xyz each), drawn at scale. False: not a casing's shape (a wrong guess at where the places are in a vertex).
bool FitMesh(const float* places, int count, float scale, MeshFit& out);
float HalfToFloat(uint16_t half);

// ---------------------------------------------------------------- in the game

// weapon.txt "casings 2": the mod's own casings; any other: none of this (effects.cpp's "casings 1" and "casings 0" as before). Render thread.
void SetMode(int mode);
int Mode();
// weapon.txt "casingsspeed 1", "casingsspin 1": the throw's speed and spin, times these (the game's own law is still to be measured: 2 gives its
// SpawnSpeed times the random, 3 to 6 m/s, instead of the random alone).
void SetThrow(float speedScale, float spinScale);
// weapon.txt "casingsmesh 1 312": the game's casing draw of kind 1 (the 5.45) has 312 indices (weapon.txt "flashsurvey" measures each gun's). The
// M416's 234 and the train pistol's 114 are known; 0: that kind isn't the mod's.
void SetMeshCount(int kind, unsigned indexCount);
// weapon.txt "casingspasses 6": the most eye passes drawn into in a frame (a safety net: two a frame is right).
void SetPasses(int most);
// weapon.txt "casingslog 0": no once-a-second line.
void SetLog(bool on);
// weapon.txt "casingsanycontext 1": an eye's depth cleared arms whichever context draws its pass first, not only the one that cleared it (it turns on
// by itself once the casings have been in view 2 s and no pass was drawn into; 0 turns it back).
void SetAnyContext(bool on);

// Once a frame on the render thread, in OnPresent before the hands' update (xr_runtime.cpp, after skin::Update): the shots, the trigger, a new world,
// what's to be let go of, the mesh's shape read back, and the log.
void Update();

// hands.cpp StepLoose, once a frame: the gun in the right hand that throws casings (null: none, seated, a launcher), and how many to throw for it now.
int TakeShots(const Emitter* gun);
// hands.cpp StepLoose, after loose::Step: where each is, for this frame's drawing.
void SetLoose(const loose::Result& r, const loose::Frame& f);
// The mod can draw that kind's now (the game's draw of it kept, its mesh read, and not off for not having found the eyes' passes lately): only then
// are casings thrown and the game's hidden.
bool Drawable(int kind);
// Everything let go of (a new life, a checkpoint, another level: a new world also does it by itself). Render thread.
void Clear();

// ---------------------------------------------------------------- the draw hooks (any thread: the game records on several)

using DrawIndexedInstancedFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT);

// depth3d.cpp Clear0/Clear1: a depth target cleared. An eye's (depth and stencil, the kept draw's depth target's size, samples and format), with casings
// in view, arms that context for the drawing (or any context: SetAnyContext).
void NoteClear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, UINT flags);
// effects.cpp DrawIndexedInstanced0/1, for each casing draw of the game's (its index count), before anything else is done with it: kept from an
// eye's picture, every 10 s while there are any.
void KeepDraw(ID3D11DeviceContext* ctx, UINT count, UINT instances, UINT start, INT base, UINT first);
// effects.cpp DrawIndexedInstanced0/1, next: the game's casing draw of the kind the gun in hand throws, while it's firing (from the trigger to 1.7 s
// after the last shot), drawn with its instances within 3 m of the game's ejection port folded to a point (nothing drawn of them): draw gets the
// first instance to use. False: not the mod's to hide, draw it as before (effects.cpp's move).
bool HideGameCasings(ID3D11DeviceContext* ctx, UINT count, UINT instances, UINT first, const std::function<void(UINT)>& draw);

// At the top of every draw hook (effects.cpp Draw0/1, DrawInstanced0/1, DrawIndexedInstanced0/1, NoteDrawIndexed): with that context armed, the casings
// are drawn just before the first draw of its eye's pass. original: effects.cpp's g_origDrawIndexedInstanced (the deferred contexts' when it's one of
// theirs). One load of an atomic while nothing is armed.
extern std::atomic<int> g_armed;  // how many armings there are
bool ArmedSlow(ID3D11DeviceContext* ctx);
void Host(ID3D11DeviceContext* ctx, const DrawIndexedInstancedFn* original);
inline void BeforeDraw(ID3D11DeviceContext* ctx, const DrawIndexedInstancedFn (&original)[2]) {
    if (g_armed.load(std::memory_order_relaxed) && ArmedSlow(ctx))
        Host(ctx, original);
}

}  // namespace casings

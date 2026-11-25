#pragma once

// Hosting a lying gun's draws on the graphics card (hostrules.h says what and why; this is its graphics part).
//
// Kept (Begin, Note, End), on the thread that draws the gun lying (skin.cpp DrawHeldPart, a held gun let go of while it's still the game's weapon):
// every draw the mod makes of it then with its own palette is noted (its indices, the rasterizer state it's drawn with: a mirrored side, both faces,
// no near cut; its palette, the bones put on the gun's own frame: hostrules::Rel), and at its first time and then every 2 s the draw's whole state as
// the game set it: its input layout, the mesh (copied on the graphics card into a buffer of the mod's own, CopyResource: never mapped, never read by
// the processor, AMD's write-combined memory never touched; one over 8 MB is held as the game's instead), its shaders, their constant buffers (the
// views' left to each pass, the game's per-draw block to the mod's bones, an immutable one as the game's, every other copied on the card as it was for
// that draw), textures and samplers, its blend and depth states, and the kind of pass it's in (its targets, its depth, colour written or not).
//
// Hosted (Host), right after a draw of another gun the game draws (its colour draw, its depth pass): every kept draw of that pass's kind is drawn with
// its states, that pass's own view (each eye's: both eyes, each its depth), its bones placed on the gun's frame where loose has it now in that draw's
// space; in the G-buffer, so the game lights it as it lights its own. Everything touched is put back as it was (the input stage, the shaders and all
// they had bound, the rasterizer, blend and depth states, the predicate); targets, viewports and scissors are never touched.
//
// Threads. Begin, Note and End: the threads the game records its draws on (one draw at a time on each, a thread's notes its own). Host: likewise.
// Ready, Info, Sweep, Clear, the settings and the stats: any. One mutex of this file's own, never held while another of the mod's is waited for.

#include <d3d11.h>

#include <cstdint>

#include "hostrules.h"

namespace gunhost {

using DrawIndexedFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, INT);
using MapFn = HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);
// The draw, map and unmap of that context past the mod's own hooks (skin.cpp's g_origDrawIndexed, g_origMap, g_origUnmap: the mod's palette is no
// upload of the game's to look at, its draws no game's draw to place), and which buffers the first-person views went into (skin::ViewBufferKind;
// null: by the 352-byte size alone).
struct Fns {
    DrawIndexedFn draw = nullptr;
    MapFn map = nullptr;
    UnmapFn unmap = nullptr;
    int (*viewKind)(ID3D11Resource*) = nullptr;
};

constexpr int kMostSubs = 8;          // the mod's own draws noted for one draw of the game's (a body round a handle drawn apart, a cover's two sides)
constexpr unsigned long long kRefreshMs = 2000;  // a kept draw's state kept again this often while the game draws it lying
constexpr UINT kMeshCopyMost = 8u << 20;  // bytes: a vertex or index buffer up to this is copied; a bigger one held as the game's

// ---- Keeping ----
void Begin();
bool Noting();
void Note(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const void* palette, UINT size);
// Keeps what was noted since Begin for kind's draw (count, start, base): pose is the gun's frame where those draws placed it, in their space (the bones
// are kept on it); paletteBuffer the game's per-draw block bound for the draw (its slots are the palette's). Nothing noted, a pose that can't be
// inverted, or hosting off: nothing kept. now: ms (GetTickCount64).
void End(ID3D11DeviceContext* ctx, const Fns& fns, uint32_t kind, UINT count, UINT start, INT base, const float* pose, ID3D11Resource* paletteBuffer,
         unsigned long long now);
void Cancel();

// ---- Hosting ----
// The kind of pass the draw on ctx is in now, and its hash (0: no depth target: no pass of a kept draw's).
uint32_t PassNow(ID3D11DeviceContext* ctx, hostrules::Pass* out = nullptr);
// The first vertex shader constant buffer bound now that is a view's (fns.viewKind, or 352 bytes), for the once-a-pass key; 0 none.
uintptr_t ViewNow(ID3D11DeviceContext* ctx, const Fns& fns);
struct HostOut {
    int draws = 0;            // drawn (each noted draw of each kept draw of this pass)
    int kept = 0;             // its kept draws of this pass
    const char* why = "";     // nothing drawn: why
};
// Every kept draw of kind of the pass ctx is in, drawn as above, its gun's frame at frameInDraw (hostrules::FrameInDraw: this draw's space).
HostOut Host(ID3D11DeviceContext* ctx, const Fns& fns, uint32_t kind, const float* frameInDraw, unsigned long long now);

// A colour draw of kind kept (with its state), not idle (kept again or hosted within hostrules::kIdleMs).
bool Ready(uint32_t kind, unsigned long long now);
struct KindInfo {
    int colour = 0, depth = 0, subs = 0;   // its kept draws that write colour, that don't; the mod's draws noted for them
    int meshCopied = 0, meshHeld = 0;      // its buffers copied on the card, held as the game's
    unsigned long long keptAt = 0;         // ms, its state last kept
    char draws[160] = "";                  // "22194/312 colour 2 subs, 22506/22506 depth" (for the log)
    char cbs[160] = "";                    // its first colour draw's constant buffers: " vs 0:352 view 2:4096 palette ps 1:64 copy"
};
bool Info(uint32_t kind, KindInfo& out);
// Kinds neither hosted (hosted[n]) nor kept again within hostrules::kIdleMs: let go (their references and copies). Once a second is plenty.
void Sweep(unsigned long long now, const uint32_t* hosted, int n);
// Everything let go of (weapon.txt "lyinghost 0").
void Clear();
// weapon.txt "lyinghostexact 1": a colour draw that tests EQUAL against its own depth draw drawn so (hostrules::Relaxed); 0, the default: LESS_EQUAL.
void SetExact(bool exact);

// What happened since the last ask, for the once-a-second line.
struct Stats {
    uint32_t keptStates = 0;     // draws' states kept (anew or again)
    uint32_t refused = 0;        // draws whose state couldn't be kept
    const char* refusal = "";    // ... the last one's why
    uint32_t passes = 0;         // passes hosted in
    uint32_t draws = 0;          // draws made there
    uint32_t noView = 0;         // kept draws not drawn: no view constants of their size in the pass
    uint32_t noPass = 0;         // hosts with none of the kind's draws of that pass
    uint32_t noPalette = 0;      // the mod's palette couldn't be written
    double worstMs = 0.0;        // the longest host
};
Stats TakeStats();

}  // namespace gunhost

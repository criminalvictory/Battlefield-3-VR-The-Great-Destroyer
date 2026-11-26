#pragma once
// The game's picture-relative post effects held as they are at the base picture (2160 pixels over 100 degrees) when the eyes' pictures are drawn wider at the same
// sharpness ([window] size with [stereo] pixels_per_tangent). Two of the tone map's family reach by the picture's own size: the vignette ((uv - 0.5) x
// vignetteParams.xy in the 24 tone map shaders) and the suppression blur's clear middle (the same form with dofParams in one shader). Drawn wider, the same angle
// from the middle sat at a smaller uv - 0.5, so the wide picture's middle was brighter toward the old corners (measured 2026-11-12: +2 near the middle to +8..10
// at the 2160 picture's corners). Their .xy are scaled by the picture's half-width in tangent over the base's at each of their draws, in a copy of the game's
// constants bound in its place for that one draw: the same darkening at the same angle, exactly. At the base the factor is 1 and nothing is done.
// bf3vr.ini [stereo] pin_effects (on by default only when the picture is wider than 100 degrees); weapon.txt "postpin 0/1" overrides it while the line is there.
//
// The copy is the buffer's whole contents as the graphics card will read them at that draw, followed through every way the game can write a constant buffer:
// Map (skin.cpp's hook calls NoteMap and NoteUpload), UpdateSubresource, CopyResource, CopySubresourceRegion and CopyStructureCount (hooked here), on the immediate context and on
// deferred contexts, and carried from a deferred context to the immediate one by FinishCommandList and ExecuteCommandList (hooked here). A draw whose buffer's
// contents can't be known that way is left as the game drew it. Every 30 s the log says by which ways the buffer was written and what each draw found.

#include <windows.h>

#include <atomic>
#include <cstdint>

struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11PixelShader;
struct ID3D11Resource;

namespace postpin {

namespace detail {
extern std::atomic<bool> g_on;  // wanted, and one of the shaders made
}

// From the pixel shader creation hook (shaders.cpp): each new shader with its bytecode's checksum.
void NoteShader(ID3D11PixelShader* ps, uint64_t checksum);
// From Map (skin.cpp), every successful one, before anything else looks at it: a buffer being refilled on that context, so its copy there is no longer what the
// card will read until NoteUpload says what was written. A refill the mod doesn't follow (the scope's own drawing, a fifth map at once) so leaves no copy.
void NoteMap(ID3D11DeviceContext* ctx, ID3D11Resource* resource);
// From Unmap (skin.cpp), after the mod's own changes to the upload: a constant buffer's new contents as the graphics card will read them.
void NoteUpload(ID3D11DeviceContext* ctx, ID3D11Resource* resource, const void* data, UINT size, bool fresh);
// weapon.txt "postpin": -1 bf3vr.ini's pin_effects, 0 off, 1 on.
void SetTest(int value);
// weapon.txt "grainpin 1": the right eye's film grain offset made the left eye's of the same frame (off by default). Each eye's exposure and grain are said in
// the 30 s lines either way (from the copy the pin draws with: only while the picture is drawn wider than 100 degrees).
void SetGrainPin(bool on);

// The device context's functions this hooks itself (d3d11_hook.cpp finds them; [0] the immediate context's, [1] the deferred contexts'; the same function in
// both is hooked once). Each hook's first test is one atomic load, and none changes the call it passes on.
struct Targets {
    void* updateSubresource[2];
    void* copyResource[2];
    void* copySubresourceRegion[2];
    void* copyStructureCount[2];
    void* executeCommandList[2];
    void* finishCommandList[2];
    // ID3D11DeviceContext1's (null where the runtime has none: then the game can't call them either).
    void* updateSubresource1[2];
    void* copySubresourceRegion1[2];
    void* discardResource[2];
};
// Before MH_EnableHook(MH_ALL_HOOKS). mapFollowed: skin.cpp's Map/Unmap hooks were made (they call NoteMap and NoteUpload). False if any could not
// be made, or Map isn't followed (the pin then never takes a copy it can't follow: see postpin.cpp).
bool InstallHooks(const Targets& targets, bool mapFollowed);

// Around a draw (effects.cpp Draw, skin.cpp DrawIndexed): with one of these shaders bound and the picture wider than the base, its constants are swapped for a
// scaled copy until the end of the scope. Full-screen passes only: draws of more than 6 vertices are not looked at. The scope's own picture (scopeview.h) is left alone.
class Pin {
public:
    Pin(ID3D11DeviceContext* ctx, UINT count) {
        if (count <= 6 && detail::g_on.load(std::memory_order_relaxed))
            Begin(ctx, count);
    }
    ~Pin() {
        if (m_ctx)
            End();
    }
    Pin(const Pin&) = delete;
    Pin& operator=(const Pin&) = delete;

private:
    void Begin(ID3D11DeviceContext* ctx, UINT count);
    void End();
    ID3D11DeviceContext* m_ctx = nullptr;
    ID3D11Buffer* m_game = nullptr;
};

}  // namespace postpin

#pragma once

// The guns' collision shapes made on the player's PC, from the game's own mesh (the owner, 2026-11-18: nothing of the game's in the download, as
// the sounds are; "making sure that the game runs well for everyone who installs it"). The shapes (guns\<name>.bin, gunshape.h) were never in the
// release zip, so for every player the hands went through every gun. Now, the first time a gun with a recipe (shaperecipes.h) is held and its
// shape isn't there:
//   1. its mesh is read back from the graphics card as weapon.txt "dump" does (skin.cpp CaptureMesh / SaveBuffer): the buffers bound for its
//      colour draw are copied to staging buffers (CopyResource) and those are mapped for reading. Never a pointer the game mapped for writing
//      (AMD: 1400 times slower, never again). Unlike "dump" the map never waits for the graphics card (DO_NOT_WAIT, tried again the next
//      frame, given up after 2 s), one buffer a frame at most, and nothing but the recipe's draws is ever looked at;
//   2. a thread of the mod's own writes the dump to guns\make\<name>\ and runs build\gunshape.exe on it as a process of its own at idle
//      priority, with no window, into guns\<name>.bin.tmp; only when that exits 0 and the file reads back as a version-2 field of that gun's
//      size is it renamed to guns\<name>.bin;
//   3. gunshape::Reload loads it (on that thread), and hands.cpp's next look (Ready) switches to it.
// Whatever fails, the mod goes on exactly as before, with no shape for that gun, and one line in the log says why. Nothing that failed is tried
// twice in a launch; what a crash left behind (guns\make, a .tmp) is cleared at the next launch's first try, and gunshape.exe ends with the game
// (a job object). A gun whose draw doesn't come as known in 20 s of holding it (another mesh, a part not at rest, never the player's own bones)
// has nothing read back: one line says what was seen, once a launch, and it's waited for again a minute later for as long as it's held.
//
// Threads. Want and Ready: hands.cpp's (any). NoteDraw: the threads the game records its draws on. OnFrame: the thread that presents (the
// immediate context's). One mutex of this file's own, never held while another of the mod's is waited for, nor while the process runs.

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "shaperecipes.h"

struct ID3D11DeviceContext;

namespace shapemake {

// The gun in hand has no shape (gunshape::Use said so): make it, if it has a recipe and nothing was tried for it this launch.
void Want(const char* name);
// Every frame, from hands.cpp: the shape's name of the gun in hand (null: none), and whether gunshape::Use has its shape. A job waits for its
// draw only while its gun is the one held. While the gun in hand has no shape, Want is called for it: at once when it's taken in hand, then
// every 3 s (a no-op unless its draw didn't come the last time and may be waited for again).
void Holding(const char* name, bool hasShape);
// True once for a name whose shape has been made and loaded since Want: gunshape::Use(name) has it now.
bool Ready(const char* name);

// The draw waited for, for the hooks: one load, no lock.
extern std::atomic<uint64_t> g_wantDraw;
inline bool Wants(uint32_t count, uint32_t start) {
    return g_wantDraw.load(std::memory_order_relaxed) == ((static_cast<uint64_t>(count) << 32) | start) && count;
}
// That draw, about to be drawn on ctx. palette: the player's own bones (skin.cpp's PlayersOwn), the whole upload, the constant buffer bound for
// this very draw; null when it isn't all of that (only counted, for the log). Taken only when its buffers are the gun's as known and every bone
// the shape keeps is where it is at rest (shaperecipes::Apart): then its buffers are taken hold of and its palette copied.
void NoteDraw(ID3D11DeviceContext* ctx, uint32_t count, uint32_t start, int base, const uint8_t* palette, size_t paletteBytes);
// Every frame, on the thread that presents. map, unmap: skin.cpp's own Map and Unmap of the immediate context past its hooks (g_origMap[0],
// g_origUnmap[0]); null, the context's own are used.
extern std::atomic<bool> g_frameWork;
void OnFrameWork(void* map, void* unmap);
inline void OnFrame(void* map, void* unmap) {
    if (g_frameWork.load(std::memory_order_relaxed))
        OnFrameWork(map, unmap);
}

// For tools (tools/shapemaketest.cpp), before anything else is called: the shapes' folder (with its last backslash) and gunshape.exe instead of
// those next to the module, and the waits (ms) before a gun's draw is taken, for it to come, and before it's waited for again.
void ForTools(const wchar_t* gunsFolder, const wchar_t* exe, unsigned long settleMs, unsigned long drawWaitMs, unsigned long retryMs);
// The last line this logged, and how many it has.
std::string LastLine();
int Lines();

// ---- The making itself: no game, no graphics card (tools/shapemaketest.cpp runs it on the mesh dumps) ----

struct CapturedDraw {
    uint32_t count = 0, start = 0;
    int base = 0, topology = 4;
    uint32_t ibFormat = 0, ibOffset = 0, stride = 0, vbOffset = 0;
    std::vector<uint8_t> ib, vb, bones;  // the buffers whole, the palette as uploaded
};
struct Capture {
    std::vector<CapturedDraw> draws;  // the recipe's, in its order
};
struct Result {
    bool ok = false;
    std::string line;  // the one log line: made, or why not
};
// Writes the capture as a dump to <gunsFolder>make\<name>\, runs exe (gunshape.exe) on it at idle priority with no window, checks what it
// wrote and renames it to <gunsFolder><name>.bin; the make folder and the .tmp are gone afterwards either way. gunsFolder with its last
// backslash. Waits for the process: never on a thread of the game's.
Result Make(const shaperecipes::Recipe& recipe, const Capture& capture, const std::wstring& gunsFolder, const std::wstring& exe,
            unsigned long timeoutMs = 300000);
// What a crash or a closed game left in gunsFolder: make\ and every *.bin.tmp.
void Sweep(const std::wstring& gunsFolder);
// The file a version-2 field whole, its grid within shaperecipes::kCellsApart of the recipe's; why not, otherwise (empty: it is).
std::string Refused(const std::wstring& file, const shaperecipes::Recipe& recipe);

}

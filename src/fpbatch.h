#pragma once

#include <atomic>

struct ID3D11DeviceContext;
struct ID3D11Resource;

// The first-person batches probe, weapon.txt "fpbatchlog 1": read-only research, off by default (docs\research\fp_batches.md). While it's
// on, the game's camera transform set (bf3.exe 0x006BBFC0) is hooked for its two calls from the first-person batch camera (0x010181B0):
// the view's own first-person camera and the transform made relative to the batch's origin, each with the batch in EBX. Once a second the
// log has a line for each kind of batch seen (its pointer, flags, FOV, views, origin, camera, meshes and their names where readable) and
// the draws skin.cpp saw made with its camera: whether the held gun, its attachments and the arms are batches of their own the mod could
// move whole. Off (no line, or "fpbatchlog 0"): the hook is never made, or turned off again; nothing of it runs.
namespace fpbatch {

extern std::atomic<bool> g_on;  // (On's)

// weapon.txt's reader (skin.cpp): wanted or not.
void SetLog(bool on);

// Every frame, from the frame's thread (xr_runtime.cpp): the hook made the first time it's wanted (the game's code checked first), turned
// on and off as wanted; the log once a second.
void OnFrame();

// Whether the probe runs: the draw hooks' only cost while it's off.
inline bool On() { return g_on.load(std::memory_order_relaxed); }

// A 352-byte per-view upload, as skin.cpp's Unmap sees it before anything changes it: its camera's rows (right, up, back) and its eye as
// viewturn::ReadPerView reads them, nullptr if it doesn't read as a view. The batch whose camera it is tags this thread's draws after it.
void NoteView(ID3D11Resource* buffer, const float* rows, const double* eye);

// Whether this thread's draws are tagged with a batch (NoteView's).
bool Tagged();

// A draw on this thread (skin.cpp's DrawIndexed, before anything): counted for its batch if the batch's per-view block is bound to the
// vertex shader. kind: what skin.cpp makes of it, one letter (0 nothing).
void NoteDraw(ID3D11DeviceContext* ctx, unsigned count, unsigned start, int base, char kind);

}  // namespace fpbatch

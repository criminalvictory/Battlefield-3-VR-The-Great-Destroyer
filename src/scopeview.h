#pragma once

#include <d3d11.h>

#include <atomic>

// The view through a scope, really drawn (the user, 2026-11-05: "I don't want no shortcuts. I want the real crispiness"): a second copy of the game's world renderer, its own small targets, a narrow camera along
// the sight's line, drawn only while someone's looking through a magnified sight. weapon.txt "scopeview S": its picture's size in pixels (720 by default); 0: nothing of this is hooked or made.
namespace scopeview {

// Set round the second renderer's drawing, on the render thread: everything of the mod's that watches the game's draws and uploads lets them pass.
extern std::atomic<bool> g_drawing;
inline bool Drawing() { return g_drawing.load(std::memory_order_relaxed); }

// Each plain draw of its drawing (effects.cpp).
void NotePlainDraw(ID3D11DeviceContext* ctx);

// weapon.txt "scopeview S": 0 off.
void SetSize(int size);
// weapon.txt "scopeviewfov": its picture's vertical field of view, radians.
void SetFov(float radians);
// weapon.txt "scopeviewforce 1": drawn all the time, not only while a lens is looked through.
void SetForce(bool on);
void SetShare(int mask);
void SetDetail(float ratio);
void SetOwnTerrain(bool own);
// weapon.txt "scopeviewshadows 0": the view through a scope without its own sun shadows (on by default; read when it's made).
void SetShadows(bool on);
// The eyes' own sun shadow maps' size (not the scope's): weapon.txt "shadowres N" (a test, while its line is there) and bf3vr.ini [world] shadowmap_resolution; 0 the game's own,
// else 512 to 8192. Kept once a frame; the game's own is put back when neither asks.
void SetEyesShadowSize(int size);
void SetEyesShadowBase(int size);
// The eyes' motion blur passes: weapon.txt "noblur 1" (off) / "noblur 0" (left alone) while its line is there, -1 for no line; bf3vr.ini [world] motion_blur_passes (1 as the game
// has them, 0 off). Only for a game whose blur amount is 0: the passes then draw nothing that shows.
void SetEyesBlurTest(int off);
void SetEyesBlurBase(int passes);
// weapon.txt "scopeviewparticles 0": no particles (smoke, dust, fire) of its own (read when it's made).
void SetParticles(bool on);
// weapon.txt "scopeviewset": one of its own settings, to see what it costs (bytes 1 or 4, 0 a float).
void PokeSetting(int offset, int bytes, float value);
// Once a frame on the render thread: hooks what it needs once the switch is on, and the once-a-second line. Also the frame's memory's own call (framemem.h), which this view shares with the eyes:
// its hooks and its line, whatever this view's switch says. When a frame would crowd that memory this view's dispatch is left out of it, and the lens shows the eye's own picture for the frame.
void OnFrame();
// weapon.txt has been read once (its "scopeview 0" counts before anything's hooked).
void SettingsRead();
// Its finished picture for this frame (null when there's none): linear light, as the eyes' before their tone map.
ID3D11ShaderResourceView* Picture();
inline bool PictureNow() { return Picture() != nullptr; }
int PictureSize();
// How a way in the arms' space lands in that picture, for this draw's eye: rows to clip x, y, w (w positive in front; x, y over w are -1 to 1). viewToArms: the draw's view (rows: right, up, back, place, in the arms' space).
bool PictureFrom(int eye, const float* viewToArms, float* rows);
// A magnified sight's lens as this frame's eye sees it. viewToArms as above; scope: its axes and the glass's middle into the arms' space; radius and off: the lens's radius and how far its line's far point is from its middle, in the picture's pixels;
// focal: the picture's pixels per unit of slope.
void NoteLens(int eye, const float* viewToArms, const float* scope, float radiusPixels, float offPixels, float power, float focal);

// Once a second (xr_runtime.cpp's pace, the render thread): the frames of that second, how many came a refresh late, and whether they went to the
// headset. The frame-rate guard (scopegov.h): seconds under the floor with this view on hold it back a while (the lens magnifies the eye's own
// picture), then it's tried again. Said in the log as it's held back and let go.
void NotePace(int frames, int late, bool paced);

// How far the sight's line may turn before the next frame beyond what its picture's margin covers (radians; recoil, each frame, render thread; 0 none). The picture is fitted wider by it
// times the power, twice that when the line it's drawn along is two frames old; with 0, or nothing sent for a quarter second, every number is as it was.
void SetKickSlack(float radians);
// Whether the second renderer is on for a magnified sight now: the dispatch's own on/off machine (on after two frames of a lens big enough to look through, off half a second after it's too
// small or 120 ms after it was last drawn), only read, never stepped. That half second lags the eye: use it with the eye's own test. False with weapon.txt "scopeviewforce" (the machine
// isn't run then) or "scopeview 0". Any thread.
bool Showing();

}

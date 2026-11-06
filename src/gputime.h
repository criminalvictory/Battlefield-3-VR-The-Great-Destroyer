#pragma once

#include <d3d11.h>
#include <dxgi.h>

// Where the graphics card's time goes in a frame, logged once a second: timestamps along the frame, read a few frames
// late and never waiting for the card. A stretch is the time from one mark to the next as the card reached them, so
// when the card is ahead of the game a stretch holds its wait for the game's next commands as well as its work: the
// mod's own stretches (the orange key, the copy to the headset, the stream view) are all work, the game's are at most.
namespace gputime {

enum Mark {
    kStart,      // just after the last frame's Present
    kScopeView,  // the view through a scope drawn (scopeview.h), when it's drawn
    kLeftEye,    // the left eye's finished picture going onto the window
    kRightEye,   // the right eye's
    kHudStart,   // the frame's first HUD draw
    kHudEnd,     // at Present: the HUD done
    kKeyDone,    // the orange markers taken off
    kSubmitted,  // copied to the headset
    kEnd,        // the stream view drawn, just before the game's own Present
    kMarks
};

// Around the game's own Present.
void FrameEnd();
void FrameStart(IDXGISwapChain* swap);
// Each mark once a frame, the first time it's reached: on the context drawing there, or by default the immediate one.
void Stamp(Mark mark, ID3D11DeviceContext* context = nullptr);

}

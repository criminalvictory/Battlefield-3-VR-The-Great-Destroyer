#pragma once
// The headset shows more than the game draws: the game's +-50 degrees against a field of -59/+50 sideways and +49/-60 up and down, so the picture's own edge was in view as black: 9-10 degrees of it
// to the outside and below, and at the nose and the top, where the picture only just reaches, black wedges opening and closing as the compositor turned the picture to the head ("I don't want to see
// no black edges at all", the user, 2026-11-04). Drawing that much more for real doesn't hold 60 frames a second at today's sharpness (2026-11-04: 3100 per eye at 120 degrees, 51-61 and down to 30).
// So the eye's image handed to the headset is bigger than the picture: the picture in it as it is, and round it its own edge carried outwards, blurred more the further out and a little darker,
// which is all the corner of an eye asks for. It costs a copy of the picture with its mips and a few million simple pixels. weapon.txt "edgefill 0": as before.

#include <d3d11.h>

namespace edgefill {

struct Layout {
    int eyeWidth = 0, eyeHeight = 0;          // each eye's part of the target, side by side
    int pictureWidth = 0, pictureHeight = 0;  // one eye's picture, as in the back buffer
    int pictureX[2] = {}, pictureY[2] = {};   // where it goes in its eye's part
};

// target: at least 2 * eyeWidth by eyeHeight, its format of back's family, a render target. The picture's copied in
// for both eyes and the rest filled. Runs inside the game's Present: the pipeline state is put back. False: nothing was done.
bool Draw(ID3D11Device* device, ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& backDesc, ID3D11Texture2D* target, const Layout& layout);

// The targets and the copy let go (the swapchains went).
void Reset();

}  // namespace edgefill

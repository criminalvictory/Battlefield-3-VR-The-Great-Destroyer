// The PKP's belt, as the game's own reload has it (build\survey\PKP_empty.bin, "allrec"): its five bones on the frame (bone 7). Numbered from the box: bone 4 lies
// across the feed tray, 3 to 0 go down the receiver's right side into the box (the M240's and the M249's are numbered from the feed; handmag::BeltGun::reversed).
// Hanging out of the fresh box: the empty reload's 4.016 s, the box seated (3.985 s) and the hand not yet on the belt (it takes it from 4.047 s; the five frames
// from the seat on drift 0.4 to 0.5 mm a frame); bones 0 to 3 stand on the right turned 19 degrees about z, the tip (4) turned over, 162 degrees. Lying in the
// feed: the empty reload's 10.0 s, of its still stretch 4.61 to 15.99 s (821 frames within 0.4 mm and 0.2 degrees; the partial recording's 2.61 to 5.20 s and
// 7.50 to 10.34 s the same to 0.2 mm). Between the two it goes as the hand takes it (handmag.cpp).
inline constexpr float kPkpBeltHanging[5][12] = {
    {0.94575f, -0.32491f, -0.00017f, -0.07999f, 0.32491f, 0.94575f, -0.00002f, 0.00939f, 0.00016f, -0.00004f, 1.00000f, 0.24831f},
    {0.94575f, -0.32491f, -0.00017f, -0.07487f, 0.32491f, 0.94575f, -0.00002f, 0.03317f, 0.00016f, -0.00004f, 1.00000f, 0.24831f},
    {0.94575f, -0.32491f, -0.00017f, -0.07257f, 0.32491f, 0.94575f, -0.00002f, 0.05559f, 0.00016f, -0.00004f, 1.00000f, 0.24831f},
    {0.94575f, -0.32491f, -0.00017f, -0.07375f, 0.32491f, 0.94575f, -0.00002f, 0.07894f, 0.00016f, -0.00004f, 1.00000f, 0.24831f},
    {-0.95088f, 0.30955f, -0.00017f, -0.06709f, -0.30955f, -0.95088f, -0.00002f, -0.03445f, -0.00017f, 0.00004f, 1.00000f, 0.25055f},
};
inline constexpr float kPkpBeltLaid[5][12] = {
    {1.00000f, 0.00000f, 0.00000f, -0.07489f, 0.00000f, 1.00000f, 0.00000f, 0.04354f, 0.00000f, 0.00000f, 1.00000f, 0.24709f},
    {1.00000f, 0.00000f, 0.00000f, -0.06227f, 0.00000f, 1.00000f, 0.00000f, 0.06439f, 0.00000f, 0.00000f, 1.00000f, 0.24709f},
    {1.00000f, 0.00000f, 0.00000f, -0.05279f, 0.00000f, 1.00000f, 0.00000f, 0.08490f, 0.00000f, 0.00000f, 1.00000f, 0.24709f},
    {1.00000f, 0.00000f, 0.00000f, -0.04628f, 0.00000f, 1.00000f, 0.00000f, 0.10738f, 0.00000f, 0.00000f, 1.00000f, 0.24709f},
    {1.00000f, 0.00000f, 0.00000f, -0.01951f, 0.00000f, 1.00000f, 0.00000f, 0.10925f, 0.00000f, 0.00000f, 1.00000f, 0.24868f},
};

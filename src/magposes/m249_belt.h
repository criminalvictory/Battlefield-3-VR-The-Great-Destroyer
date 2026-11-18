// The M249's belt, as the game's own reload has it (build\survey\M249_partial.bin and M249_empty.bin, "allrec"): its five bones on the frame (bone 3), hanging out of a
// new box (the empty reload's 4.47-4.60 s, the box seated and the hand not yet on the belt: 9 frames within 0.7 mm) and lying in the feed (the partial
// recording's 9.5-15.9 s, the gun still: 462 frames within 0.4 mm). Its pieces are the M240's belt's to half a millimetre (the same links), and the
// hanging bones turned as the M240's are to a thousandth: the same animation, about another box. Between the two it goes as the hand takes it (handmag.cpp).
inline constexpr float kM249BeltHanging[5][12] = {
    {-0.53062f, -0.83762f, -0.12973f, 0.13776f, 0.84751f, -0.52199f, -0.09620f, -0.05877f, 0.01286f, -0.16099f, 0.98687f, 0.11630f},
    {-0.81283f, -0.57455f, -0.09593f, 0.11866f, 0.58072f, -0.81214f, -0.05646f, -0.07373f, -0.04547f, -0.10160f, 0.99379f, 0.11586f},
    {-0.96763f, 0.23343f, -0.09593f, 0.15090f, -0.22885f, -0.97182f, -0.05646f, -0.07135f, -0.10641f, -0.03268f, 0.99379f, 0.11910f},
    {-0.63292f, 0.76825f, -0.09593f, 0.17072f, -0.76710f, -0.63903f, -0.05646f, -0.04437f, -0.10468f, 0.03786f, 0.99379f, 0.12255f},
    {0.93632f, -0.33778f, -0.09593f, 0.05541f, 0.33369f, 0.94099f, -0.05646f, 0.10099f, 0.10934f, 0.02085f, 0.99379f, 0.11968f},
};
inline constexpr float kM249BeltLaid[5][12] = {
    {1.00000f, 0.00000f, 0.00000f, -0.00609f, 0.00000f, 1.00000f, 0.00000f, 0.09976f, 0.00000f, 0.00000f, 1.00000f, 0.12141f},
    {1.00000f, 0.00000f, 0.00000f, 0.01860f, 0.00000f, 1.00000f, 0.00000f, 0.11340f, 0.00000f, 0.00000f, 1.00000f, 0.12141f},
    {1.00000f, -0.00011f, 0.00000f, 0.04524f, 0.00011f, 1.00000f, 0.00000f, 0.11598f, 0.00000f, 0.00000f, 1.00000f, 0.12141f},
    {1.00000f, -0.00010f, 0.00000f, 0.06674f, 0.00010f, 1.00000f, 0.00000f, 0.11000f, 0.00000f, 0.00000f, 1.00000f, 0.12141f},
    {1.00000f, -0.00010f, 0.00000f, 0.07371f, 0.00010f, 1.00000f, 0.00000f, 0.09308f, 0.00000f, 0.00000f, 1.00000f, 0.12141f},
};

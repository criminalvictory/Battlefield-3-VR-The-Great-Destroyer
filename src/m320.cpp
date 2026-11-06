#include "m320.h"

#include "ammozone.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace m320 {

// ---- The launcher as the game draws it (the M416's body space; skin.cpp kPartRests and kM320Hinge, build\survey\M320_vanilla.bin) ----
const float kOpenMost = 15.0f * 3.14159265f / 180.0f;
// The round's mesh runs z 0.5105 to 0.6294 on the gun at rest and the barrel's back end is at 0.495: slid this far back its nose is out.
const float kClear = 0.134f;
// On the round's own bone: its axis at x 0.0003, y -0.0584 (the gun's 0.0153, -0.0354), its base and its nose.
const float kRoundBaseOnBone[3] = {0.0003f, -0.0584f, 0.2795f};
const float kRoundNoseOnBone[3] = {0.0003f, -0.0584f, 0.3984f};
const float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};
// Where the hand takes the barrel's back end: on its outer left side (its line at x 0.0153, the tube 6.6 cm across), 2.5 cm in front of its back end.
const float kBarrelGrab[3] = {0.0483f, -0.036f, 0.52f};

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;
constexpr float kGravity = 9.81f;

// The barrel turns about the axis (0, -1, 0) through here: a pin at its front, on its right, as the real one's (TM 3-22.31, H&K's patent).
constexpr float kHingeAt[3] = {-0.018f, 0.0f, 0.751f};
// The tube's bone and the round's at rest on the gun's body, unturned.
constexpr float kTubeRest[3] = {-0.019f, 0.012f, 0.454f};
constexpr float kRoundRest[3] = {0.015f, 0.023f, 0.231f};
// The barrel's back end (its mouth) on its line, shut; its line's height.
constexpr float kMouthShut[3] = {0.0153f, -0.0363f, 0.495f};
// A hand behind this (along the tube as shut) is at the round's rim; in front of it, beside the barrel.
constexpr float kBehindFace = 0.485f;
// The barrel's outer left side and a finger's thickness: a push there swings it shut. A push counts this far above or below its line, and from its back end
// to here along it; moving to the gun's right this fast at least to start (the bolt slap's: reload.cpp), and still touching within this.
constexpr float kSurfaceX = 0.0483f + 0.009f;
constexpr float kTouchHigh = 0.035f, kTouchBack = 0.485f, kTouchFront = 0.62f;
constexpr float kPushSpeed = 0.05f, kPushKeep = 0.002f;
// A slap (the user, 2026-11-05: "I also like the slapping to close it ... make the slap a lot more responsive ... Sometimes it doesn't really capture it"): any of
// the glove's points coming at the barrel from its left at kSlapSpeed or more, from more than kSlapFrom out from its side to within kSlapNear of it (or past it)
// in the frame, anywhere from just in front of its back end to near its pin and kSlapHigh above or below its line: shut with the snap, as a push is at its end.
// (The push alone needed a point crossing its side exactly in a band 3.5 cm high and 13.5 cm long: a slap a little high, low or along missed.)
constexpr float kSlapSpeed = 0.35f, kSlapFrom = 0.01f, kSlapNear = 0.025f, kSlapHigh = 0.06f, kSlapBack = 0.51f, kSlapFront = 0.72f, kSlapMost = 0.10f;
// The handguard's hold (hands.cpp's line) on the gun.
constexpr float kHandguard[2][3] = {{0.0f, 0.045f, 0.46f}, {0.0f, 0.045f, 0.68f}};
// The glove's points that push (its palm, 1.5 cm out of the hand's middle, and the index, middle and ring tips of the straight fingers: handpose.cpp).
constexpr float kTouchInGlove[4][3] = {{0.5267f, 1.0864f, 0.2065f}, {0.5855f, 1.0376f, 0.3128f}, {0.6005f, 1.0244f, 0.3070f}, {0.6038f, 1.0165f, 0.2839f}};

// ---- The barrel's springs: the game's own swing open (frames 7312-7327 of the recording), degrees one frame apart, re-timed a sixtieth of a second each
// (the recording's own time stamps repeat: frames 7320 and 7321 both read 4.562 s), so it reaches its stop at 0.25 s as the game's does. ----
constexpr float kSpringDegrees[16] = {0.0f, 0.5f, 2.0f, 4.4f, 5.7f, 7.4f, 8.4f, 10.0f, 11.4f, 12.5f, 13.4f, 14.0f, 14.5f, 14.7f, 14.9f, 15.0f};
constexpr float kSpringStep = 0.25f / 15.0f;
constexpr float kSpringEnd = 0.25f;
// Shutting, the game's own: 15 degrees to shut in 0.093 s as a thing let go of falls (quadratic), past shut by 0.7 degrees and back in 0.09 s (frames
// 7462-7474); from less open, sooner and a smaller bounce.
constexpr float kSnapTime = 0.093f, kOvershoot = 0.7f * kDegree, kSettle = 0.09f;
// Opened by the hand on its back end: past this much of the way open the springs take it. Shutting, past g_snapShut of the way shut it snaps.
constexpr float kGrabSpringAt = 0.30f;
std::atomic<float> g_snapShut{0.60f};
// The round in it moves only with the barrel this far open; a round more than this out of it stops the barrel shutting there.
constexpr float kMoveFrom = 10.0f * kDegree;
constexpr float kBlockS = 0.005f;

// ---- The round loose in the open barrel ----
std::atomic<float> g_tipStart{30.0f * kDegree}, g_friction{0.3f};
constexpr float kMovingOut = 0.002f;    // case_out once it's this far out
std::atomic<float> g_jab{25.0f};        // m/s/s along the gun, 2 frames of 3 ...
constexpr float kJabKick = 0.6f;        // ... throws it back at this (m/s)
constexpr float kJump = 0.04f;          // the gun moving this far in a frame is tracking, not the hand
constexpr float kMaxDropSpeed = 3.0f;
constexpr float kLongestFrame = 0.1f;   // a frame this long (the most dt is taken as): what the gun did across it isn't read

// ---- The hands ----
constexpr float kArm = 0.4f, kTake = 0.6f, kLetGo = 0.35f, kReachWindow = 0.40f;  // the squeeze (rocket.cpp's)
constexpr float kLost = 1.0f;           // a lost hand keeps what it has this long (s)
constexpr float kMaxStep = 0.05f;       // the most a pull moves the round a frame (m): a tracking jump can't throw it
constexpr float kGloveEase = 0.12f;     // the glove onto the launcher and back off (s)
constexpr float kPinchReach = 0.08f;    // the hand's middle this near the game's on the rim (as its glove is drawn there)
constexpr float kGrabReachShut = 0.06f, kGrabReachOpen = 0.07f, kGrabNearer = 0.04f, kGrabLost = 0.25f;
constexpr float kBellyAway = 0.10f;     // a squeeze this near the handguard or the launcher's grip is the gun's, not the belly's
// The real hand this far from the hand drawn on the rim, 3 frames running: it has let go (nothing pulls the round down, up or forward, and a glove never
// stays behind on the gun: "Mr. Elastic never again").
constexpr float kOffRim = 0.12f;
constexpr int kOffRimFrames = 3;
constexpr float kAwayWait = 0.2f;       // not in use this long before what the hand has goes (a frame's dip of the state read keeps everything)
constexpr float kAtStop = 0.1f * kDegree;  // the barrel let go of this near its stop: open, nothing moves
constexpr float kGripPlane = 0.0004f;   // the launcher's grip's middle (x on the gun, the grip study): a right hand's hold there is the left's mirrored about it
// A round put in by hand. There was a magnet (its nose within 10 cm of the open barrel's back end, home by itself in 0.15 s): the user, 2026-11-05, "too snappy
// ... too magnetic. I feel like it did it on its own without me actually putting it in", and a case pulled free (then 3 cm out) was caught again 40 ms later, a
// dozen times running ("as much as I try to pull it, it just stays back in"). Now: a live round's nose within g_enterRadius of the barrel's line, from
// kEnterBehind outside its back end to kEnterIn inside, turned no more than kEnterTurn from going in, and going in (kEnterSpeed) or already kEnterDeep in, goes
// onto the line, the hand on its rim; from then on it moves only as the hand pushes or pulls it, and pushed in to its last kSeatS it goes home in g_seatTime
// with a click (the hand riding it). A round only comes out into the hand with its nose out (kClear), and goes back on only after its nose has been kRearm from
// the back end; a spent case never does.
// The user tried that (2026-11-05 22:20): "now it's like harder to insert it": the window wider (4 cm off the line and 45 degrees were 2.5 cm and 30), its nose
// at the back end going in at all or already in, and pushed past half way it goes home by itself at kSeatSpeed (a finish, not a magnet: it has to be put in and
// pushed first). Just put in, it comes back out into the hand only pulled kOutPast past the back end (twice in that test it went in and straight back out).
std::atomic<float> g_enterRadius{0.04f}, g_seatTime{0.06f};
constexpr float kEnterBehind = 0.03f, kEnterIn = 0.06f, kEnterTurn = 45.0f * kDegree, kEnterSpeed = 0.02f, kEnterDeep = 0.0f, kEnterFirst = 0.005f;
constexpr float kRearm = 0.06f;
constexpr float kSeatS = 0.065f, kSeatOut = 0.02f;  // pushed past half way (its last 6.5 cm), once it's been out 2 cm more than that in this hold, it goes home
constexpr float kSeatSpeed = 0.5f;       // ... by itself at this (m/s), the hand riding it: 0.13 s from half way
constexpr float kOutPast = 0.025f;       // a round put in from the hand comes back out only pulled this far past the barrel's back end
constexpr float kCarryEase = 0.15f;      // the fingers from the rim's pinch round to the carry, once the glove's off the launcher (s)
constexpr float kNoteNear = 0.25f;      // squeezes this near its back end are noted when they take nothing

// ---- The wrist's flick: a quick roll of the gun about its length that stops (the user's word, "I could flick it open") ----
std::atomic<float> g_flickRate{9.0f};   // rad/s at its peak (516 degrees/s)
// The flick OPENING it is off (the user, 2026-11-06: "I would flick my gun and it would open ... get rid of the flick to open"; the release, a pull of its
// trigger, opens it). The flick shutting it stays. On only by SetFlickOpens(true) or in the scripted roll of "m320test 6" (m320drop.ps1 opens it that way).
std::atomic<bool> g_flickOpens{false};
constexpr float kFlickStart = 0.3f, kFlickStop = 0.35f, kFlickStopWithin = 0.15f, kFlickLeast = 15.0f * kDegree, kFlickMost = 100.0f * kDegree,
                kFlickOffAxis = 0.6f, kFlickAgain = 0.6f;

std::atomic<bool> g_byHand{true};
std::atomic<bool> g_fireReady{true}, g_releasePressed{false}, g_holdingNow{false};

// ---- The game's own left hand in its reload (build\survey\M320_vanilla.bin): the glove's bones 0-22
// from the gloves' bind space into the round's bone (frames 7350 and 7433), the tube's (7469), the gun's body (7213, on the launcher's grip). ----
// frame 7350 (5.000 s): pinching the round's rim, the thumb over, the index under.
constexpr float kBones7350[handpose::kBones][12] = {
    {0.966186f, 0.134237f, -0.220143f, -0.530875f, 0.019356f, 0.813620f, 0.581073f, -1.072720f, 0.257115f, -0.565687f, 0.783511f, 0.562227f},
    {0.715837f, 0.657457f, -0.235217f, -0.987313f, -0.607637f, 0.752485f, 0.254053f, -0.632581f, 0.344026f, -0.038934f, 0.938152f, -0.097090f},
    {0.913604f, 0.111099f, -0.391131f, -0.449990f, -0.038494f, 0.981259f, 0.188806f, -1.133659f, 0.404777f, -0.157438f, 0.900759f, 0.007133f},
    {-0.575374f, -0.808604f, -0.122897f, 1.241385f, -0.316299f, 0.081416f, 0.945159f, -0.242678f, -0.754254f, 0.582692f, -0.302606f, 0.133104f},
    {0.925627f, -0.088614f, -0.367914f, -0.236770f, 0.255633f, 0.863270f, 0.435218f, -1.225154f, 0.279044f, -0.496901f, 0.821719f, 0.467849f},
    {0.580226f, -0.621630f, -0.526226f, 0.561700f, 0.708885f, 0.703583f, -0.049512f, -1.198954f, 0.401022f, -0.344306f, 0.848901f, 0.230421f},
    {0.715816f, 0.657493f, -0.235179f, -0.987402f, -0.607698f, 0.752455f, 0.253992f, -0.632524f, 0.343959f, -0.038893f, 0.938178f, -0.097135f},
    {0.085759f, 0.720118f, -0.688529f, -0.603923f, -0.863769f, 0.398150f, 0.308830f, -0.119242f, 0.496533f, 0.568246f, 0.656162f, -0.775709f},
    {0.715768f, 0.657543f, -0.235187f, -0.987423f, -0.607723f, 0.752410f, 0.254068f, -0.632448f, 0.344017f, -0.038925f, 0.938155f, -0.097156f},
    {0.288385f, 0.886712f, -0.361350f, -0.978183f, -0.911808f, 0.369516f, 0.179058f, -0.031308f, 0.292298f, 0.277845f, 0.915075f, -0.410814f},
    {-0.579381f, -0.803430f, -0.137175f, 1.225214f, -0.290277f, 0.046134f, 0.955829f, -0.241485f, -0.761614f, 0.593608f, -0.259947f, 0.142853f},
    {-0.125020f, 0.601523f, -0.789011f, -0.338425f, -0.878203f, 0.302946f, 0.370111f, -0.021439f, 0.461658f, 0.739184f, 0.490385f, -0.905921f},
    {0.715797f, 0.657499f, -0.235222f, -0.987346f, -0.607647f, 0.752446f, 0.254144f, -0.632481f, 0.344091f, -0.038984f, 0.938126f, -0.097073f},
    {-0.111184f, 0.593748f, -0.796932f, -0.344075f, -0.870603f, 0.328523f, 0.366226f, -0.051364f, 0.479256f, 0.734531f, 0.480392f, -0.914117f},
    {0.715800f, 0.657500f, -0.235206f, -0.987386f, -0.607647f, 0.752443f, 0.254151f, -0.632484f, 0.344083f, -0.038999f, 0.938128f, -0.097057f},
    {-0.759237f, 0.223351f, -0.611288f, 0.381398f, -0.650753f, -0.247726f, 0.717740f, 0.325748f, 0.008876f, 0.942733f, 0.333429f, -0.793806f},
    {-0.740143f, -0.335516f, -0.582765f, 0.951654f, -0.618383f, -0.000862f, 0.785876f, 0.057473f, -0.264176f, 0.942033f, -0.206840f, -0.551566f},
    {0.870549f, 0.241015f, -0.429016f, -0.561564f, -0.057810f, 0.915896f, 0.397231f, -1.108188f, 0.488673f, -0.321007f, 0.811266f, 0.169823f},
    {0.631703f, 0.639001f, -0.438894f, -0.890322f, -0.529583f, 0.769177f, 0.357640f, -0.707419f, 0.566120f, 0.006508f, 0.824296f, -0.237557f},
    {-0.439220f, 0.618134f, -0.651916f, -0.205118f, -0.865436f, -0.096421f, 0.491653f, 0.354805f, 0.241049f, 0.780136f, 0.577306f, -0.826135f},
    {-0.745327f, -0.325055f, -0.582087f, 0.945176f, -0.604557f, -0.038552f, 0.795627f, 0.074696f, -0.281064f, 0.944908f, -0.167781f, -0.530144f},
    {-0.603737f, 0.162349f, -0.780475f, 0.405599f, -0.791629f, -0.006739f, 0.610963f, 0.188170f, 0.093930f, 0.986709f, 0.132589f, -0.849733f},
    {-0.747585f, -0.239149f, -0.619614f, 0.862400f, -0.616338f, -0.097808f, 0.781383f, 0.131905f, -0.247470f, 0.966043f, -0.074277f, -0.569395f},
};
// frame 7433 (6.328 s): pushing the round home, the thumb on its base.
constexpr float kBones7433[handpose::kBones][12] = {
    {0.849232f, 0.522030f, -0.079305f, -0.949740f, -0.294917f, 0.593528f, 0.748831f, -0.740104f, 0.437982f, -0.612543f, 0.657999f, 0.529743f},
    {0.777597f, 0.622206f, 0.090568f, -1.052875f, -0.539024f, 0.585504f, 0.605506f, -0.583863f, 0.323722f, -0.519658f, 0.790670f, 0.462472f},
    {0.856643f, -0.161412f, -0.490008f, -0.108108f, 0.233851f, 0.968104f, 0.089924f, -1.257023f, 0.459864f, -0.191621f, 0.867067f, 0.004895f},
    {-0.623375f, 0.303484f, -0.720625f, 0.243790f, -0.607999f, 0.391371f, 0.690771f, -0.359364f, 0.491670f, 0.868749f, -0.059453f, -0.913450f},
    {0.840036f, 0.539380f, -0.058370f, -0.967886f, -0.317866f, 0.576505f, 0.752730f, -0.710006f, 0.439658f, -0.613767f, 0.655736f, 0.530628f},
    {0.905563f, 0.274577f, -0.323361f, -0.661928f, 0.001508f, 0.760177f, 0.649714f, -1.061800f, 0.424208f, -0.588845f, 0.687974f, 0.505385f},
    {0.777554f, 0.622252f, 0.090612f, -1.052972f, -0.539102f, 0.585486f, 0.605453f, -0.583814f, 0.323693f, -0.519622f, 0.790705f, 0.462438f},
    {0.493195f, 0.836167f, -0.239959f, -1.052532f, -0.615176f, 0.530273f, 0.583410f, -0.476847f, 0.615073f, -0.140118f, 0.775919f, -0.106933f},
    {0.777527f, 0.622288f, 0.090598f, -1.052998f, -0.539096f, 0.585426f, 0.605517f, -0.583748f, 0.323768f, -0.519648f, 0.790658f, 0.462396f},
    {0.539013f, 0.822265f, -0.182602f, -1.077146f, -0.601825f, 0.527641f, 0.599500f, -0.485288f, 0.589297f, -0.213244f, 0.779265f, -0.013073f},
    {-0.700990f, 0.226168f, -0.676358f, 0.370782f, -0.594201f, 0.339231f, 0.729278f, -0.324856f, 0.394381f, 0.913110f, -0.103409f, -0.869783f},
    {0.199138f, 0.853079f, -0.482286f, -0.855821f, -0.636580f, 0.486777f, 0.598175f, -0.421261f, 0.745056f, 0.187894f, 0.639989f, -0.503615f},
    {0.777580f, 0.622228f, 0.090557f, -1.052898f, -0.539004f, 0.585444f, 0.605582f, -0.583759f, 0.323794f, -0.519699f, 0.790613f, 0.462458f},
    {0.245647f, 0.842224f, -0.479911f, -0.878582f, -0.614935f, 0.518105f, 0.594492f, -0.466609f, 0.749340f, 0.149079f, 0.645185f, -0.466363f},
    {0.777581f, 0.622224f, 0.090572f, -1.052932f, -0.539007f, 0.585436f, 0.605586f, -0.583752f, 0.323786f, -0.519712f, 0.790608f, 0.462485f},
    {0.343598f, 0.876377f, -0.337495f, -0.981153f, -0.628229f, 0.481620f, 0.611039f, -0.424802f, 0.698045f, 0.002072f, 0.716050f, -0.285153f},
    {-0.354555f, 0.585361f, -0.729138f, -0.205978f, -0.628203f, 0.428460f, 0.649448f, -0.376577f, 0.692568f, 0.688312f, 0.215813f, -0.907200f},
    {0.793228f, 0.062782f, -0.605679f, -0.292504f, 0.198029f, 0.914004f, 0.354091f, -1.250782f, 0.575824f, -0.400817f, 0.712581f, 0.219448f},
    {0.764530f, 0.622694f, -0.166569f, -1.002884f, -0.373633f, 0.638677f, 0.672673f, -0.736405f, 0.525253f, -0.452043f, 0.720947f, 0.299431f},
    {0.422411f, 0.862179f, -0.279667f, -1.028028f, -0.619253f, 0.499821f, 0.605560f, -0.447494f, 0.661886f, -0.082611f, 0.745038f, -0.183789f},
    {-0.436157f, 0.547913f, -0.713833f, -0.109609f, -0.632671f, 0.377394f, 0.676240f, -0.327622f, 0.639917f, 0.746568f, 0.182046f, -0.917956f},
    {0.229544f, 0.871973f, -0.432401f, -0.888597f, -0.646122f, 0.468769f, 0.602312f, -0.399021f, 0.727897f, 0.141127f, 0.671005f, -0.441508f},
    {0.177361f, 0.868120f, -0.463582f, -0.845148f, -0.649789f, 0.457075f, 0.607334f, -0.386118f, 0.739131f, 0.193514f, 0.645163f, -0.495318f},
};
// frame 7469 (6.890 s): the fingers on the barrel's outer left side, slapping it shut.
constexpr float kBones7469[handpose::kBones][12] = {
    {0.613237f, 0.721442f, -0.321653f, -0.935130f, -0.225298f, 0.550044f, 0.804171f, -0.716801f, 0.757086f, -0.420680f, 0.499847f, 0.006556f},
    {0.421517f, 0.864966f, -0.272318f, -1.006336f, -0.409278f, 0.449438f, 0.794038f, -0.508848f, 0.809206f, -0.223247f, 0.543458f, -0.248203f},
    {0.299472f, 0.952131f, -0.061324f, -1.090907f, -0.938516f, 0.305541f, 0.160722f, 0.041515f, 0.171765f, 0.009422f, 0.985092f, -0.291088f},
    {0.423018f, 0.864606f, -0.271127f, -1.007201f, -0.409458f, 0.449321f, 0.794011f, -0.508569f, 0.808331f, -0.224865f, 0.544092f, -0.246237f},
    {0.617714f, 0.712651f, -0.332500f, -0.925830f, -0.213298f, 0.558793f, 0.801407f, -0.732292f, 0.756923f, -0.424119f, 0.497182f, 0.010919f},
    {0.671312f, 0.418802f, -0.611510f, -0.579000f, 0.144622f, 0.735180f, 0.662265f, -1.089729f, 0.726928f, -0.533024f, 0.432967f, 0.158536f},
    {0.421487f, 0.864984f, -0.272306f, -1.006395f, -0.409362f, 0.449432f, 0.793997f, -0.508813f, 0.809178f, -0.223188f, 0.543522f, -0.248295f},
    {0.419323f, 0.866645f, -0.270356f, -1.007462f, -0.412153f, 0.447076f, 0.793883f, -0.504691f, 0.808886f, -0.221465f, 0.544660f, -0.250270f},
    {0.421426f, 0.865020f, -0.272285f, -1.006386f, -0.409338f, 0.449368f, 0.794046f, -0.508761f, 0.809222f, -0.223175f, 0.543462f, -0.248358f},
    {0.418663f, 0.865929f, -0.273653f, -1.005564f, -0.409872f, 0.449069f, 0.793940f, -0.508121f, 0.810385f, -0.220230f, 0.542929f, -0.252078f},
    {0.633863f, 0.772389f, -0.040389f, -1.089063f, -0.450670f, 0.411274f, 0.792306f, -0.444213f, 0.628580f, -0.484012f, 0.608784f, 0.114753f},
    {0.413855f, 0.871858f, -0.261891f, -1.011669f, -0.421872f, 0.438609f, 0.793502f, -0.489961f, 0.806689f, -0.217910f, 0.549333f, -0.253835f},
    {0.421467f, 0.864999f, -0.272289f, -1.006336f, -0.409240f, 0.449370f, 0.794096f, -0.508753f, 0.809251f, -0.223253f, 0.543387f, -0.248252f},
    {0.420315f, 0.865179f, -0.273495f, -1.005680f, -0.409147f, 0.449742f, 0.793933f, -0.509173f, 0.809897f, -0.221803f, 0.543019f, -0.250111f},
    {0.421471f, 0.865002f, -0.272272f, -1.006378f, -0.409246f, 0.449359f, 0.794099f, -0.508739f, 0.809245f, -0.223264f, 0.543391f, -0.248250f},
    {0.645251f, 0.760909f, -0.068313f, -1.076358f, -0.421212f, 0.428931f, 0.799123f, -0.481696f, 0.637362f, -0.486861f, 0.597273f, 0.114491f},
    {0.421178f, 0.864997f, -0.272740f, -1.006154f, -0.409246f, 0.449608f, 0.793958f, -0.508980f, 0.809398f, -0.222780f, 0.543362f, -0.248869f},
    {0.300502f, 0.951560f, -0.065044f, -1.089787f, -0.938500f, 0.307158f, 0.157702f, 0.040539f, 0.170042f, 0.013654f, 0.985342f, -0.294963f},
    {0.470893f, 0.845434f, -0.251991f, -1.011912f, -0.540822f, 0.502322f, 0.674672f, -0.483759f, 0.696972f, -0.181416f, 0.693770f, -0.266374f},
    {0.643086f, 0.762527f, -0.070649f, -1.076146f, -0.421057f, 0.429140f, 0.799092f, -0.481996f, 0.639648f, -0.484138f, 0.597042f, 0.110386f},
    {0.631746f, 0.773984f, -0.042960f, -1.088783f, -0.450315f, 0.411539f, 0.792370f, -0.444715f, 0.630962f, -0.481231f, 0.608525f, 0.110530f},
    {0.644034f, 0.762644f, -0.059937f, -1.081664f, -0.428458f, 0.424509f, 0.797630f, -0.472457f, 0.633752f, -0.488021f, 0.600160f, 0.117496f},
    {0.645386f, 0.761618f, -0.058418f, -1.081820f, -0.428592f, 0.424365f, 0.797634f, -0.472229f, 0.632284f, -0.489745f, 0.600303f, 0.120111f},
};
// frame 7213 (3.000 s): the glove's root on the launcher's own grip (the rest of that hold is skin.cpp's GameLeftGripM320, learned in the game).
constexpr float kGripRoot7213[12] = {0.470867f, 0.845236f, -0.252703f, -1.095694f, -0.243223f, 0.399722f, 0.883778f, -0.592625f, 0.848013f, -0.354679f, 0.393797f, 0.335468f};

// ---- Small helpers (as rocket.cpp's and handmag.cpp's) ----
float Finite(float v) {
    return std::isfinite(v) ? v : 0.0f;
}

bool Finite3(const float* v) {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

bool Finite12(const float* m) {
    for (int i = 0; i < 12; ++i)
        if (!std::isfinite(m[i]))
            return false;
    return true;
}

void Identity(float* m) {
    const float i[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, i, sizeof(i));
}

void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}

void InvertRigid(const float* m, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        r[i * 4 + 3] = -(r[i * 4] * m[3] + r[i * 4 + 1] * m[7] + r[i * 4 + 2] * m[11]);
    std::memcpy(out, r, sizeof(r));
}

void Transform(const float* m, const float* p, float* out) {
    float r[3];
    for (int i = 0; i < 3; ++i)
        r[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
    std::memcpy(out, r, sizeof(r));
}

void Turn(const float* m, const float* v, float* out) {
    float r[3];
    for (int i = 0; i < 3; ++i)
        r[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2];
    std::memcpy(out, r, sizeof(r));
}

float Dot(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float Length(const float* v) {
    return std::sqrt(Dot(v, v));
}

float Apart(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return Length(d);
}

float Smooth(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// How near a point comes to the stretch from a to b.
float FromSegment(const float* p, const float* a, const float* b) {
    const float ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, ap[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    const float length2 = Dot(ab, ab);
    const float t = length2 > 0.0f ? std::clamp(Dot(ap, ab) / length2, 0.0f, 1.0f) : 0.0f;
    const float at[3] = {a[0] + ab[0] * t, a[1] + ab[1] * t, a[2] + ab[2] * t};
    return Apart(p, at);
}

void ToQuat(const float* m, float* q) {  // a 3x4's rotation; q: x y z w
    const float trace = m[0] + m[5] + m[10];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q[0] = (m[9] - m[6]) / s, q[1] = (m[2] - m[8]) / s, q[2] = (m[4] - m[1]) / s, q[3] = 0.25f * s;
    } else if (m[0] > m[5] && m[0] > m[10]) {
        const float s = std::sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
        q[0] = 0.25f * s, q[1] = (m[1] + m[4]) / s, q[2] = (m[2] + m[8]) / s, q[3] = (m[9] - m[6]) / s;
    } else if (m[5] > m[10]) {
        const float s = std::sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
        q[0] = (m[1] + m[4]) / s, q[1] = 0.25f * s, q[2] = (m[6] + m[9]) / s, q[3] = (m[2] - m[8]) / s;
    } else {
        const float s = std::sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
        q[0] = (m[2] + m[8]) / s, q[1] = (m[6] + m[9]) / s, q[2] = 0.25f * s, q[3] = (m[4] - m[1]) / s;
    }
}

void FromQuat(const float* q, float* m) {  // into a 3x4's rotation, its translation untouched
    const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    const float x = q[0] / n, y = q[1] / n, z = q[2] / n, w = q[3] / n;
    const float r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),
                        2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                        2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y)};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i * 4 + j] = r[i * 3 + j];
}

// a and b blended, turn and place: s 0 is a, 1 is b.
void Blend(const float* a, const float* b, float s, float* out) {
    float qa[4], qb[4], r[12];
    ToQuat(a, qa), ToQuat(b, qb);
    if (qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3] < 0.0f)
        for (float& v : qb)
            v = -v;
    float q[4];
    for (int i = 0; i < 4; ++i)
        q[i] = qa[i] + (qb[i] - qa[i]) * s;
    FromQuat(q, r);
    for (int i = 0; i < 3; ++i)
        r[i * 4 + 3] = a[i * 4 + 3] + (b[i * 4 + 3] - a[i * 4 + 3]) * s;
    std::memcpy(out, r, sizeof(r));
}

// A 3x4 seen in a mirror across x (both its spaces): the right glove's from the left's (hands.cpp mirrors the left glove's line-up the same way).
void Mirror(const float* m, float* out) {
    std::memcpy(out, m, 12 * sizeof(float));
    out[1] = -out[1], out[2] = -out[2], out[3] = -out[3], out[4] = -out[4], out[8] = -out[8];
}

// ---- The game's holds, by side (the right glove's mirrored stand-ins) ----
struct HoldData {
    float place[12] = {};    // the glove on its object: the gloves' bind space into the object's (the game's bone 0 there)
    float inGlove[12] = {};  // the object in the glove: its inverse
    float own[handpose::kBones][12] = {};  // the game's fingers, each bone relative to the glove
    const float (*fingers)[12] = nullptr;  // those, or a fitted table (SetHold); none for the right glove yet
};
struct Tables {
    HoldData hold[2][5];
    float pinchTarget[3] = {};  // the hand's middle as the game pinches the rim, on the round's bone
    float gripMiddle[3] = {};   // ... on the launcher's grip, on the gun
};

void FillHold(HoldData& d, const float (*bones)[12]) {
    std::memcpy(d.place, bones[0], sizeof(d.place));
    InvertRigid(d.place, d.inGlove);
    for (int b = 0; b < handpose::kBones; ++b)
        Compose(d.inGlove, bones[b], d.own[b]);
    d.fingers = d.own;
}

void MirrorHold(const HoldData& left, HoldData& right) {
    Mirror(left.place, right.place);
    Mirror(left.inGlove, right.inGlove);
    right.fingers = nullptr;
}

// Filled where they stay: each hold's fingers point at its own table (d.fingers = d.own), so a copy made on the way out of a function left those pointing into
// a stack frame gone (the DLL's build copied it where m320test's didn't: the game's hand drawn from whatever lay there, its numbers not finite or huge: the
// rods and the arms gone of the user's videos, 2026-11-05).
void FillTables(Tables& t) {
    FillHold(t.hold[0][static_cast<int>(Hold::kPinch)], kBones7350);
    FillHold(t.hold[0][static_cast<int>(Hold::kPush)], kBones7433);
    FillHold(t.hold[0][static_cast<int>(Hold::kBarrel)], kBones7469);
    FillHold(t.hold[0][static_cast<int>(Hold::kCarry)], kBones7350);  // (the pinch until SetHold gives the fitted carry)
    for (int h = 1; h < 5; ++h)
        MirrorHold(t.hold[0][h], t.hold[1][h]);
    Transform(kBones7350[0], kHandInGlove, t.pinchTarget);
    Transform(kGripRoot7213, kHandInGlove, t.gripMiddle);
}

Tables& Vanilla() {
    static Tables t;
    static const bool filled = (FillTables(t), true);
    (void)filled;
    return t;
}

const HoldData& HoldFor(int side, Hold hold) {
    return Vanilla().hold[side & 1][static_cast<int>(hold)];
}

// The hand's middle, and a glove point, in each side's bind space.
void InGlove(int side, const float* left, float* out) {
    out[0] = side == 1 ? -left[0] : left[0], out[1] = left[1], out[2] = left[2];
}

// The palm as hands.cpp draws the glove (Frame::palm), this frame; and whether the carry hold was fitted (SetHold).
float g_palm[12] = {};
bool g_havePalm = false, g_carrySet = false;

// A general 3x4's inverse (the game's tables carry the palm's widening: not rigid).
bool Invert(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    const float A = e * i - f * h, B = f * g - d * i, C = d * h - e * g;
    const float det = a * A + b * B + c * C;
    if (!(std::fabs(det) > 1e-9f))
        return false;
    const float r[9] = {A / det, (c * h - b * i) / det, (b * f - c * e) / det, B / det, (a * i - c * g) / det, (c * d - a * f) / det,
                        C / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int k = 0; k < 3; ++k) {
        out[k * 4] = r[k * 3], out[k * 4 + 1] = r[k * 3 + 1], out[k * 4 + 2] = r[k * 3 + 2];
        out[k * 4 + 3] = -(r[k * 3] * m[3] + r[k * 3 + 1] * m[7] + r[k * 3 + 2] * m[11]);
    }
    return true;
}

// The hold's table re-anchored on the palm as drawn: hands.cpp's m320Rebase for it (the drawn palm times the table's palm's inverse). The right glove has no
// table of the game's (its holds are mirrored stand-ins drawn on its own fingers), and without the palm given the tables are as they are: none.
void RebaseFor(int side, Hold hold, float* out) {
    Identity(out);
    const HoldData& d = HoldFor(side, hold);
    float inverse[12];
    if (side == 0 && g_havePalm && d.fingers && Invert(d.fingers[1], inverse))
        Compose(g_palm, inverse, out);
}

// The hand's middle as this side's hold has it on the object (its bone on the gun), on the gun: where that glove is drawn (re-anchored on the palm, as hands.cpp
// draws it: the pinch's 3.7 cm from where its bone 0 alone put it), so where it's reached for.
void HoldMiddle(int side, Hold hold, const float* object, float* onGun) {
    float glove[12], rebase[12], inverse[12], drawn[12], middle[3];
    Compose(object, HoldFor(side, hold).place, glove);
    RebaseFor(side, hold, rebase);
    if (Invert(rebase, inverse))
        Compose(glove, inverse, drawn);
    else
        std::memcpy(drawn, glove, sizeof(drawn));
    InGlove(side, kHandInGlove, middle);
    Transform(drawn, middle, onGun);
}

// a and b blended about the point p of the space they take in: p goes straight from where a puts it to where b does, the turn between them blends (hands.cpp's
// BlendAbout: blended about their own origin, 1.2 m from the hand, things in the hand swung out tens of centimetres half way).
void BlendAboutPoint(const float* a, const float* b, float s, const float* p, float* out) {
    float pa[3], pb[3], turned[3];
    Transform(a, p, pa);
    Transform(b, p, pb);
    Blend(a, b, s, out);
    out[3] = out[7] = out[11] = 0.0f;
    Transform(out, p, turned);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = pa[i] + (pb[i] - pa[i]) * s - turned[i];
}

// The round in the hand at work, in its glove as drawn: the game's pinch on its rim re-anchored on the palm, going over to the carry (the hand wrapped round it)
// by g_carryMix once that's fitted.
float g_carryMix = 0.0f;
float g_badRound[2][12] = {}, g_badRoundMix = 0.0f;  // a blend that came out not finite: its two ends and its mix (BadRound)
bool g_badRoundSeen = false;
void RoundInGlove(int side, float* out) {
    float rebase[12];
    RebaseFor(side, Hold::kPinch, rebase);
    float pinched[12];
    Compose(rebase, HoldFor(side, Hold::kPinch).inGlove, pinched);
    if (side == 0 && g_carrySet && g_carryMix > 0.0f) {
        const float middle[3] = {(kRoundBaseOnBone[0] + kRoundNoseOnBone[0]) / 2, (kRoundBaseOnBone[1] + kRoundNoseOnBone[1]) / 2,
                                 (kRoundBaseOnBone[2] + kRoundNoseOnBone[2]) / 2};
        BlendAboutPoint(pinched, HoldFor(side, Hold::kCarry).inGlove, Smooth(g_carryMix), middle, out);
        if (!Finite12(out)) {  // (never handed on: what went in kept for the log, the pinch's own place instead)
            std::memcpy(g_badRound[0], pinched, sizeof(pinched));
            std::memcpy(g_badRound[1], HoldFor(side, Hold::kCarry).inGlove, sizeof(g_badRound[1]));
            g_badRoundMix = g_carryMix;
            g_badRoundSeen = true;
            std::memcpy(out, pinched, sizeof(pinched));
        }
    } else {
        std::memcpy(out, pinched, sizeof(pinched));
    }
}

// The hand's middle on the launcher's grip, either hand's (a right hand's the left's mirrored about the grip's middle).
void GripMiddleFor(int side, float* onGun) {
    std::memcpy(onGun, Vanilla().gripMiddle, 3 * sizeof(float));
    if (side == 1)
        onGun[0] = 2.0f * kGripPlane - onGun[0];
}

// ---- The launcher's shape ----
void Hinge(float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle);
    const float r[9] = {c, 0.0f, -s, 0.0f, 1.0f, 0.0f, s, 0.0f, c};  // about -y: the back end (behind the pin) goes to +x, the gun's left
    for (int i = 0; i < 3; ++i) {
        out[i * 4] = r[i * 3], out[i * 4 + 1] = r[i * 3 + 1], out[i * 4 + 2] = r[i * 3 + 2];
        out[i * 4 + 3] = kHingeAt[i] - (r[i * 3] * kHingeAt[0] + r[i * 3 + 1] * kHingeAt[1] + r[i * 3 + 2] * kHingeAt[2]);
    }
}

void TubeAt(float angle, float* out) {
    float hinge[12];
    Hinge(angle, hinge);
    const float rest[12] = {1, 0, 0, kTubeRest[0], 0, 1, 0, kTubeRest[1], 0, 0, 1, kTubeRest[2]};
    Compose(hinge, rest, out);
}

// A point on the gun in the tube's own frame (as it lies shut).
void ToTube(float angle, const float* p, float* out) {
    float back[12];
    Hinge(-angle, back);
    Transform(back, p, out);
}

// Out of the barrel's back end, on the gun.
void ExitAt(float angle, float* out) {
    out[0] = std::sin(angle), out[1] = 0.0f, out[2] = -std::cos(angle);
}

// How far round the hinge a point on the gun is: turning the barrel open by a turns a point on it by a.
float AngleAbout(const float* p) {
    return std::atan2(p[0] - kHingeAt[0], -(p[2] - kHingeAt[2]));
}

// The barrel's angle that puts this point on the gun on its outer left side (a finger's thickness out): open less than that, the point is outside it.
bool AngleOnSurface(const float* p, float* angle) {
    const float qx = p[0] - kHingeAt[0], qz = p[2] - kHingeAt[2], x = kSurfaceX - kHingeAt[0];
    const float radius = std::sqrt(qx * qx + qz * qz);
    if (!(radius > x))
        return false;
    *angle = std::atan2(qz, qx) + std::acos(x / radius);
    return true;
}

float SpringAngle(float t) {
    const float x = t / kSpringStep;
    if (!(x > 0.0f))
        return 0.0f;
    if (x >= 15.0f)
        return kOpenMost;
    const int i = static_cast<int>(x);
    const float f = x - static_cast<float>(i);
    return (kSpringDegrees[i] + (kSpringDegrees[i + 1] - kSpringDegrees[i]) * f) * kDegree;
}

// Where along the springs' swing this angle comes (s): one let go of part way open goes on from there.
float SpringTimeAt(float angle) {
    const float degrees = angle / kDegree;
    if (degrees <= 0.0f)
        return 0.0f;
    for (int i = 0; i < 15; ++i)
        if (degrees <= kSpringDegrees[i + 1]) {
            const float span = kSpringDegrees[i + 1] - kSpringDegrees[i];
            const float f = span > 0.0f ? std::clamp((degrees - kSpringDegrees[i]) / span, 0.0f, 1.0f) : 1.0f;
            return (static_cast<float>(i) + f) * kSpringStep;
        }
    return kSpringEnd;
}

float SnapAt() {
    return kOpenMost * (1.0f - std::clamp(g_snapShut.load(std::memory_order_relaxed), 0.0f, 1.0f));
}

// ---- State ----
struct Hand {
    bool armedGrip = true, armedTrigger = true;
    float crossedAt = -1.0f;
    bool crossed = false, fresh = false;
    float grasp = 0.0f;
    float gripAt[4][3] = {}, gripDt[4] = {};
    int gripNext = 0, gripCount = 0;
    float onGunWas[3] = {};
    bool haveOnGun = false;
    float touchWas[4][3] = {};
    bool haveTouch = false;
    bool pressing = false;  // its index finger on the barrel release
};

struct Burst {
    bool on = false;
    float sign = 0.0f, peak = 0.0f, peakAt = 0.0f, swept = 0.0f, off = 0.0f;
    bool jump = false;
};

float g_clock = 0.0f;
bool g_first = true, g_wasActive = false, g_loadedWas = false;
float g_inactiveFor = 0.0f;         // not in use this long (since it last was)
// The head and the gun as last seen with every number finite (a frame with one that isn't takes them, as not seen).
float g_headWas[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}, g_gunPoseWas[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
float g_seatedAt = -10.0f;          // when the hand last seated a round (the count comes up after it)
constexpr float kOwnSeat = 0.5f;
// The barrel.
Barrel g_barrel = Barrel::kShut;
float g_angle = 0.0f, g_angleWas = 0.0f;
bool g_latched = true;
float g_springT = 0.0f;
float g_snapFrom = 0.0f, g_snapT = 0.0f, g_snapTime = 0.0f;
bool g_snapHit = false;
float g_grabOffset = 0.0f;
bool g_grabFromShut = false;
bool g_blockNoted = false;
// The round in it.
Content g_content = Content::kSeated;
bool g_live = true;
float g_s = 0.0f, g_v = 0.0f;
bool g_movedOut = false;
float g_fly = 0.0f, g_flyS = 0.0f;  // its last way home (0-1), from this far out on the line
float g_flyFor = 0.06f;             // ... taking this long (s)
bool g_flyLive = true;
bool g_entered = false;             // the round on the rim was put in from the hand (it comes back out only pulled kOutPast past the back end)
bool g_rideCarry = false;           // the hand riding it home holds it as it carried it (put in from the hand), not as the game's pinch then push
float g_outPast = 0.0f;             // ... how far past it the hand has pulled it
bool g_counted = true;              // the live round in the barrel is in the count (seated by hand or there from the start; one pushed in from the hand isn't, till home)
float g_pinchOut = 0.0f;            // the most it's been out since the hand took its rim (pushed back in from 4 cm more than its last 2 cm, it goes home)
bool g_rearmed = true;              // a round in the hand may go onto the line: its nose has been kRearm from the back end since it came out
float g_noseWas[3] = {};            // the round in the hand's nose on the gun last frame (its speed into the barrel)
bool g_haveNoseWas = false;
// The hand at work.
HandState g_hand = HandState::kFree;
bool g_handLive = true;
float g_lostFor = 0.0f;
int g_offRim = 0;           // frames the real hand has been off the rim it pinches
float g_handRoom[12] = {};  // the round in it as last held, in the room
float g_gloveOnGun[12] = {}, g_gloveBlend = 0.0f;
Hold g_glovesHold = Hold::kNone;
const float (*g_fingers)[12] = nullptr;
const float (*g_fingersAlt)[12] = nullptr;
float g_fingersMix = 0.0f, g_fingersShare = 0.0f;
Hand g_hands[2];
// The belly's two claims: when each was last opened.
float g_m320OpenedAt = -1.0f, g_rifleOpenedAt = -1.0f;
bool g_rifleWas = false;
// The gun's movement in the room. gunRoom is a frame late (hands.cpp takes it after the steer), so the difference from the last one spans the frame before
// this: the last Update's dt (0: none to go by).
float g_gunWas[12] = {};
bool g_haveGunWas = false;
float g_dtWas = 0.0f;
float g_omegaWas[3] = {};
bool g_haveOmega = false;
Burst g_burst;
bool g_burstArmed = true;
float g_lastFlickAt = -10.0f;
float g_breechAt[3][3] = {}, g_breechDt[3] = {};
int g_breechCount = 0;
float g_along[3] = {};
int g_alongCount = 0;
bool g_jabArmed = true;
char g_stateText[160] = "shut";

void ForgetMotion() {
    g_haveGunWas = false;
    g_dtWas = 0.0f;
    g_haveOmega = false;
    g_burst = Burst{};
    g_burstArmed = true;
    g_breechCount = 0;
    g_alongCount = 0;
    g_jabArmed = true;
}

void Unlatch(Result& r) {
    g_latched = false;
    g_m320OpenedAt = g_clock;
    g_blockNoted = false;
    r.released = true;
}

void SpringFrom(float angle) {
    g_barrel = Barrel::kSpringing;
    g_springT = SpringTimeAt(angle);
    g_angle = std::max(angle, 0.0f);
}

// Let go of short of the snap: its springs push it back open. At its stop already it's open, and nothing moves (no stop to hit, nothing sprung back).
void SpringBack(Result& r) {
    if (g_angle >= kOpenMost - kAtStop) {
        g_barrel = Barrel::kOpen;
        g_angle = kOpenMost;
        return;
    }
    SpringFrom(g_angle);
    r.sprungBack = true;
}

void StartSnap() {
    g_barrel = Barrel::kSnapping;
    g_snapFrom = std::max(g_angle, 0.0f);
    g_snapT = 0.0f;
    g_snapTime = kSnapTime * std::sqrt(g_snapFrom / kOpenMost);
    g_snapHit = false;
}

void StepSnap(float dt, Result& r) {
    g_snapT += dt;
    if (!g_snapHit) {
        if (g_snapT < g_snapTime) {
            const float u = g_snapT / g_snapTime;
            g_angle = g_snapFrom * (1.0f - u * u);
            return;
        }
        g_snapHit = true;
        r.shut = true;
    }
    const float t = g_snapT - g_snapTime;
    if (t >= kSettle) {
        g_angle = 0.0f;
        g_barrel = Barrel::kShut;
        g_latched = true;
        r.latchedNow = true;
        return;
    }
    g_angle = -kOvershoot * (g_snapFrom / kOpenMost) * std::sin(kPi * t / kSettle);
}

// A round half out of the barrel or on its way in: the barrel can't shut past kMoveFrom.
bool Blocked() {
    return ((g_content == Content::kSliding || g_content == Content::kPinched) && g_s > kBlockS) || g_content == Content::kFlying;
}

// The round's turn about its own length on the barrel's line (radians, about +z through its axis on its bone): a round put in from the hand keeps the turn the hand
// gave it (it snapped to the game's own, up to half a turn in a frame, and the hand on it swung round with it: "inserting it in is awkward as hell", the user,
// 2026-11-05, and the review). Back to the game's own (0) once it's shut in or gone.
float g_roll = 0.0f;

void RoundAt(float angle, float s, float* out) {
    float hinge[12];
    Hinge(angle, hinge);
    const float back[12] = {1, 0, 0, kRoundRest[0], 0, 1, 0, kRoundRest[1], 0, 0, 1, kRoundRest[2] - s};
    Compose(hinge, back, out);
    if (g_roll != 0.0f) {
        const float c = std::cos(g_roll), sn = std::sin(g_roll), ax = kRoundBaseOnBone[0], ay = kRoundBaseOnBone[1];
        const float turn[12] = {c, -sn, 0, ax - (c * ax - sn * ay), sn, c, 0, ay - (sn * ax + c * ay), 0, 0, 1, 0};  // about its own axis (x ax, y ay on its bone)
        float turned[12];
        Compose(out, turn, turned);
        std::memcpy(out, turned, sizeof(turned));
    }
}

// The turn about its own length a round's pose on the gun has from the line's own at s (radians).
float RollOf(float angle, float s, const float* onGunRound) {
    const float keep = g_roll;
    g_roll = 0.0f;
    float line[12], inverse[12], rel[12];
    RoundAt(angle, s, line);
    g_roll = keep;
    InvertRigid(line, inverse);
    Compose(inverse, onGunRound, rel);
    return std::atan2(rel[4], rel[0]);
}

// The round on its last way home, u 0 to 1: on the barrel's line from g_flyS out to home.
void FlyPose(float angle, float u, float* out) {
    RoundAt(angle, g_flyS * (1.0f - Smooth(u)), out);
}

// From the neck in the body's axes (right, up, forward), exactly as pouch.cpp measures the belly.
void FromNeck(const Frame& f, const float* gripRoom, float* out) {
    const float forward[3] = {-std::sin(f.yaw), 0.0f, -std::cos(f.yaw)};
    const float right[3] = {std::cos(f.yaw), 0.0f, -std::sin(f.yaw)};
    const float d[3] = {gripRoom[0] - f.neck[0], gripRoom[1] - f.neck[1], gripRoom[2] - f.neck[2]};
    out[0] = d[0] * right[0] + d[2] * right[2];
    out[1] = d[1];
    out[2] = d[0] * forward[0] + d[2] * forward[2];
}

void NoteGrip(Hand& h, const float* at, float dt) {
    std::memcpy(h.gripAt[h.gripNext], at, sizeof(h.gripAt[0]));
    h.gripDt[h.gripNext] = dt;
    h.gripNext = (h.gripNext + 1) % 4;
    h.gripCount = std::min(h.gripCount + 1, 4);
}

// The grip's speed over its last frames, in the room; at most kMaxDropSpeed.
void HandSpeed(const Hand& h, float* v) {
    v[0] = v[1] = v[2] = 0.0f;
    if (h.gripCount < 2)
        return;
    const int newest = (h.gripNext + 3) % 4, oldest = (h.gripNext + 4 - h.gripCount) % 4;
    float time = 0.0f;
    for (int k = 1; k < h.gripCount; ++k)
        time += h.gripDt[(oldest + k) % 4];
    if (!(time > 0.0f))
        return;
    for (int i = 0; i < 3; ++i)
        v[i] = (h.gripAt[newest][i] - h.gripAt[oldest][i]) / time;
    const float speed = Length(v);
    if (speed > kMaxDropSpeed)
        for (int i = 0; i < 3; ++i)
            v[i] *= kMaxDropSpeed / speed;
}

void CapSpeed(float* v) {
    const float speed = Length(v);
    if (speed > kMaxDropSpeed)
        for (int i = 0; i < 3; ++i)
            v[i] *= kMaxDropSpeed / speed;
}

// The turn of the gun since last frame as a speed about its own axes (rad/s): x its left, y up, z its length (+ its top to the right).
void Spin(const float* was, const float* now, float dt, float* omega) {
    float d[9];  // now times was's inverse: last frame's turn to this one's, in the room
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            d[i * 3 + j] = now[i * 4] * was[j * 4] + now[i * 4 + 1] * was[j * 4 + 1] + now[i * 4 + 2] * was[j * 4 + 2];
    const float w[3] = {0.5f * (d[7] - d[5]), 0.5f * (d[2] - d[6]), 0.5f * (d[3] - d[1])};
    const float sine = Length(w), cosine = (d[0] + d[4] + d[8] - 1.0f) * 0.5f;
    const float angle = std::atan2(sine, cosine);
    const float k = sine > 1e-7f ? angle / sine / dt : 1.0f / dt;
    const float room[3] = {w[0] * k, w[1] * k, w[2] * k};
    for (int i = 0; i < 3; ++i)
        omega[i] = now[i] * room[0] + now[4 + i] * room[1] + now[8 + i] * room[2];
}

// The flick: +1 a roll to the right (the gun's top going right) that stops, -1 to the left, 0 none. span: the time the reading covers (s).
int Flick(const float* omega, bool jumpNow, float span) {
    const float setting = g_flickRate.load(std::memory_order_relaxed), rate = std::fabs(setting);
    if (!(rate > 0.0f)) {
        g_burst.on = false;
        return 0;
    }
    const float roll = omega[2], off = std::sqrt(omega[0] * omega[0] + omega[1] * omega[1]), size = std::fabs(roll);
    if (!g_burst.on) {
        if (!g_burstArmed && size < kFlickStart * rate)
            g_burstArmed = true;
        if (g_burstArmed && size > kFlickStart * rate) {
            g_burst.on = true;
            g_burst.sign = roll > 0.0f ? 1.0f : -1.0f;
            g_burst.peak = size;
            g_burst.peakAt = g_clock;
            g_burst.swept = size * span;
            g_burst.off = off;
            g_burst.jump = jumpNow;
        }
        return 0;
    }
    Burst& b = g_burst;
    const float along = roll * b.sign;
    b.swept += along * span;
    b.jump = b.jump || jumpNow;
    if (along > b.peak) {
        b.peak = along;
        b.peakAt = g_clock;
    }
    b.off = std::max(b.off, off);
    if (along < kFlickStop * b.peak) {
        // It stopped (or turned back): a snap of the wrist if it was quick, far enough and not too far, a roll and not a swing, and no tracking jump.
        const bool snap = b.peak >= rate && g_clock - b.peakAt <= kFlickStopWithin && b.swept >= kFlickLeast && b.swept <= kFlickMost &&
                          b.off <= kFlickOffAxis * b.peak && !b.jump && g_clock - g_lastFlickAt >= kFlickAgain;
        b.on = false;
        g_burstArmed = size < kFlickStart * rate;
        if (snap) {
            g_lastFlickAt = g_clock;
            const int way = b.sign > 0.0f ? 1 : -1;
            return setting < 0.0f ? -way : way;
        }
        return 0;
    }
    if (g_clock - b.peakAt > kFlickStopWithin) {  // a roll that goes on: not a flick
        b.on = false;
        g_burstArmed = false;
    }
    return 0;
}

// What the hand at work has, let go of: a spent case falls, a live round goes back (it was counted back at its unload, or never counted), a pinch leaves
// the round where it is, the barrel goes on as let go of.
void LetGoOfAll(const Frame& f, Result& r, bool dropCases) {
    switch (g_hand) {
    case HandState::kRound:
        if (!g_handLive && dropCases) {
            float fromRoom[12];
            InvertRigid(f.headPose, fromRoom);
            Compose(fromRoom, g_handRoom, r.dropView);
            r.drop = r.letGo = true;
            r.dropLive = false;
            r.dropFromBarrel = false;
            std::memset(r.dropVelocityRoom, 0, sizeof(r.dropVelocityRoom));
            std::memset(r.dropSlideRoom, 0, sizeof(r.dropSlideRoom));
        } else {
            r.cancelled = true;
        }
        break;
    case HandState::kPinch:
        g_content = Content::kSliding;
        g_v = 0.0f;
        g_entered = false;
        break;
    case HandState::kBarrel:
        r.barrelLet = true;
        if (g_grabFromShut ? g_angle > kGrabSpringAt * kOpenMost : g_angle > SnapAt()) {
            if (g_grabFromShut)
                SpringFrom(g_angle);  // (opened from shut: on to its stop)
            else
                SpringBack(r);
        } else {
            StartSnap();
        }
        break;
    case HandState::kRiding:
    case HandState::kFree:
        break;
    }
    g_hand = HandState::kFree;
}

// The barrel's own swing finished at once (the launcher not in use: nothing's drawn to watch it).
void FinishBarrel() {
    if (g_barrel == Barrel::kSpringing || ((g_barrel == Barrel::kHeld || g_barrel == Barrel::kPushed) && g_angle > SnapAt())) {
        g_barrel = Barrel::kOpen;
        g_angle = kOpenMost;
    } else if (g_barrel != Barrel::kOpen && g_barrel != Barrel::kShut) {
        g_barrel = Barrel::kShut;
        g_angle = 0.0f;
        g_latched = true;
    }
}

// A bone on the gun in the view's space as the gun was drawn (its steer and all: last frame's in the room), or as this frame has it.
void DrawnOnGun(const Frame& f, const float* onGun, float* view) {
    if (!f.haveGunRoom) {
        Compose(f.gunPose, onGun, view);
        return;
    }
    float fromRoom[12], room[12];
    InvertRigid(f.headPose, fromRoom);
    Compose(f.gunRoom, onGun, room);
    Compose(fromRoom, room, view);
}

// A round on its last way home, called off: it stays on the barrel's line where it was, loose (gravity takes it on; shut on, it goes home with the barrel).
void CallOffFlight(const Frame&, Result&) {
    if (g_content != Content::kFlying)
        return;
    g_content = Content::kSliding;
    g_live = g_flyLive;
    g_s = g_flyS * (1.0f - Smooth(std::min(g_fly, 1.0f)));
    g_v = 0.0f;
    g_movedOut = true;
    if (g_hand == HandState::kRiding)
        g_hand = HandState::kFree;
}

const char* BarrelName() {
    switch (g_barrel) {
    case Barrel::kShut: return "shut";
    case Barrel::kSpringing: return "springing open";
    case Barrel::kOpen: return "open";
    case Barrel::kHeld: return "in the hand";
    case Barrel::kPushed: return "pushed shut";
    case Barrel::kSnapping: return "snapping shut";
    }
    return "";
}

const char* ContentName() {
    switch (g_content) {
    case Content::kNone: return "empty";
    case Content::kSeated: return "seated";
    case Content::kSliding: return "sliding";
    case Content::kPinched: return "pinched";
    case Content::kFlying: return "going home";
    }
    return "";
}

const char* HandName() {
    switch (g_hand) {
    case HandState::kFree: return "free";
    case HandState::kPinch: return "on the rim";
    case HandState::kRound: return "a round in it";
    case HandState::kBarrel: return "on the barrel";
    case HandState::kRiding: return "riding it home";
    }
    return "";
}

// The frame with any number that isn't finite taken as not seen, never stored: a hand with such a glove or grip point untracked, the gun with such a pose
// untracked (the last good one standing in for the sums that still take one), the gun as drawn or the body not had, given touch points left for the glove's
// own; a head like that, everything untracked (its last good one standing in).
Frame Checked(const Frame& in) {
    Frame f = in;
    if (Finite12(f.headPose)) {
        std::memcpy(g_headWas, f.headPose, sizeof(g_headWas));
    } else {
        std::memcpy(f.headPose, g_headWas, sizeof(f.headPose));
        f.gunTracked = f.haveGunRoom = f.haveBody = false;
        f.hand[0].tracked = f.hand[1].tracked = false;
    }
    if (Finite12(f.gunPose)) {
        std::memcpy(g_gunPoseWas, f.gunPose, sizeof(g_gunPoseWas));
    } else {
        std::memcpy(f.gunPose, g_gunPoseWas, sizeof(f.gunPose));
        f.gunTracked = false;
    }
    if (f.haveGunRoom && !Finite12(f.gunRoom))
        f.haveGunRoom = false;
    if (f.haveBody && !(Finite3(f.neck) && std::isfinite(f.yaw)))
        f.haveBody = false;
    for (HandIn& hand : f.hand) {
        if (hand.tracked && !(Finite12(hand.glove) && Finite3(hand.gripRoom)))
            hand.tracked = false;
        for (int i = 0; i < std::min(hand.touches, 4); ++i)
            if (!Finite3(hand.touch[i]))
                hand.touches = 0;
    }
    return f;
}

// The round in the hand at work, in its glove (the hand lost: where it was in the room).
void PublishHandRound(const Frame& f, int w, Result& r) {
    if (g_hand != HandState::kRound)
        return;
    r.handRound = true;
    r.handSide = w;
    r.handRoundLive = g_handLive;
    RoundInGlove(w, r.roundInGlove);  // (as the glove draws it: re-anchored on the palm, going over to the carry)
    if (f.hand[w].tracked) {
        Compose(f.hand[w].glove, r.roundInGlove, r.handRoundView);
    } else {
        float fromRoom[12];
        InvertRigid(f.headPose, fromRoom);
        Compose(fromRoom, g_handRoom, r.handRoundView);
    }
}

// The glove eased onto the launcher's hold (onIt) or back off it to the real hand, its fingers onto the game's there (fingersOn) or back to the
// controller's; given out while either is anywhere near. (Riding the round home it's on it all the way: set before.)
void PublishGlove(float dt, bool onIt, bool fingersOn, bool riding, Result& r) {
    if (!riding)
        g_gloveBlend = std::clamp(g_gloveBlend + (onIt ? dt : -dt) / kGloveEase, 0.0f, 1.0f);
    g_fingersShare = fingersOn ? 1.0f : std::max(g_fingersShare - dt / kGloveEase, 0.0f);
    if (g_gloveBlend > 0.0f) {
        std::memcpy(r.gloveOnGun, g_gloveOnGun, sizeof(r.gloveOnGun));
        r.gloveBlend = g_gloveBlend;
        r.hold = g_glovesHold;
    }
    if (g_fingersShare > 0.0f && g_fingers) {
        r.fingers = g_fingers;
        r.fingersAlt = g_fingersAlt;
        r.fingersMix = g_fingersMix;
        r.fingersShare = g_fingersShare;
    }
}

void Publish(Result& r) {
    r.latched = g_latched;
    r.barrelState = g_barrel;
    r.content = g_content;
    r.handState = g_hand;
    r.roundS = g_s;
    r.roundV = g_v;
    r.m320OpenedAt = g_m320OpenedAt;
    g_fireReady.store(r.fireReady, std::memory_order_relaxed);
    g_holdingNow.store(r.holding, std::memory_order_relaxed);
}

}  // namespace

void HingeAt(float angle, float* out) {
    Hinge(angle, out);
}

void RoundOnLine(float angle, float s, float* out) {
    RoundAt(angle, s, out);
}

void MouthAt(float angle, float* out) {
    float hinge[12];
    Hinge(angle, hinge);
    Transform(hinge, kMouthShut, out);
}

void PinchTarget(float* onRound) {
    std::memcpy(onRound, Vanilla().pinchTarget, 3 * sizeof(float));
}

void GripMiddle(float* onGun) {
    std::memcpy(onGun, Vanilla().gripMiddle, 3 * sizeof(float));
}

bool FireReady() {
    return g_fireReady.load(std::memory_order_relaxed);
}

bool ByHand() {
    return g_byHand.load(std::memory_order_relaxed);
}

void SetByHand(bool on) {
    g_byHand = on;
}

void PressRelease() {
    g_releasePressed = true;
}

bool TakeReleasePress() {
    return g_releasePressed.exchange(false);
}

bool Holding() {
    return g_holdingNow.load(std::memory_order_relaxed);
}

void SetTip(float degrees, float friction) {
    g_tipStart = std::clamp(degrees, 0.0f, 90.0f) * kDegree;
    g_friction = std::clamp(friction, 0.0f, 2.0f);
}

void SetFlick(float radiansPerSecond) {
    g_flickRate = radiansPerSecond;
}

void SetFlickOpens(bool opens) {
    g_flickOpens = opens;
}

void SetSnapShut(float fraction) {
    g_snapShut = std::clamp(fraction, 0.0f, 1.0f);
}

void SetJab(float metresPerSecondSquared) {
    g_jab = metresPerSecondSquared;
}

void SetMagnet(float metres, float seconds) {
    if (!std::isfinite(metres) || !std::isfinite(seconds))
        return;
    g_enterRadius = std::clamp(metres, 0.01f, 0.06f);
    g_seatTime = std::clamp(seconds, 0.02f, 0.3f);
}

void SetHold(Hold hold, const float* objectInGlove, const float (*fingers)[12]) {
    if (hold == Hold::kNone || !objectInGlove || !Finite12(objectInGlove))
        return;
    Tables& t = Vanilla();
    HoldData& left = t.hold[0][static_cast<int>(hold)];
    std::memcpy(left.inGlove, objectInGlove, sizeof(left.inGlove));
    InvertRigid(left.inGlove, left.place);
    left.fingers = fingers ? fingers : left.own;
    MirrorHold(left, t.hold[1][static_cast<int>(hold)]);
    if (hold == Hold::kPinch)
        Transform(left.place, kHandInGlove, t.pinchTarget);  // reached for where the glove is drawn
    if (hold == Hold::kCarry)
        g_carrySet = true;
}

std::atomic<bool> g_testOpenEmpty{false};
void TestOpenEmpty() {
    g_testOpenEmpty = true;
}

bool TakeBadRound(float* pinched, float* carry, float& mix) {
    if (!g_badRoundSeen)
        return false;
    g_badRoundSeen = false;
    std::memcpy(pinched, g_badRound[0], sizeof(g_badRound[0]));
    std::memcpy(carry, g_badRound[1], sizeof(g_badRound[1]));
    mix = g_badRoundMix;
    return true;
}

void GloveFor(Hold hold, int side, const float* objectView, float* glove) {
    if (hold == Hold::kNone) {
        std::memcpy(glove, objectView, 12 * sizeof(float));
        return;
    }
    Compose(objectView, HoldFor(side, hold).place, glove);
}

void Clear() {
    g_first = true;
    g_wasActive = false;
    g_inactiveFor = 0.0f;
    g_loadedWas = false;
    g_seatedAt = -10.0f;
    g_barrel = Barrel::kShut;
    g_angle = g_angleWas = 0.0f;
    g_latched = true;
    g_springT = g_snapT = g_snapTime = g_snapFrom = 0.0f;
    g_snapHit = false;
    g_grabFromShut = false;
    g_blockNoted = false;
    g_content = Content::kSeated;
    g_live = true;
    g_s = g_v = 0.0f;
    g_movedOut = false;
    g_fly = g_flyS = 0.0f;
    g_flyFor = 0.06f;
    g_entered = false;
    g_rideCarry = false;
    g_roll = 0.0f;
    g_outPast = 0.0f;
    g_counted = true;
    g_pinchOut = 0.0f;
    g_rearmed = true;
    g_haveNoseWas = false;
    g_carryMix = 0.0f;
    g_hand = HandState::kFree;
    g_lostFor = 0.0f;
    g_offRim = 0;
    g_gloveBlend = 0.0f;
    g_glovesHold = Hold::kNone;
    g_fingers = g_fingersAlt = nullptr;
    g_fingersMix = g_fingersShare = 0.0f;
    for (Hand& h : g_hands)
        h = Hand{};
    g_m320OpenedAt = g_rifleOpenedAt = -1.0f;
    g_rifleWas = false;
    g_lastFlickAt = -10.0f;
    ForgetMotion();
    g_releasePressed = false;
    g_fireReady = true;
    g_holdingNow = false;
}

Result Update(const Frame& frame) {
    const Frame f = Checked(frame);
    Result r;
    const float dt = std::clamp(Finite(f.dt), 0.0f, kLongestFrame);
    g_clock += dt;
    g_havePalm = f.havePalm && Finite12(f.palm);
    if (g_havePalm)
        std::memcpy(g_palm, f.palm, sizeof(g_palm));
    const int w = f.gunHand == 0 ? 1 : 0;  // the hand at work: never the rifle's
    r.side = w;

    // The squeezes, each hand's: the grip or the trigger crossing 0.6 from below 0.4; fresh for 0.4 s after, still held (the belly reached by then counts).
    for (int side = 0; side < 2; ++side) {
        const HandIn& in = f.hand[side];
        Hand& h = g_hands[side];
        const float squeeze = Finite(in.squeeze), trigger = Finite(in.trigger);
        h.grasp = std::max(squeeze, trigger);
        if (squeeze < kArm)
            h.armedGrip = true;
        if (trigger < kArm)
            h.armedTrigger = true;
        const bool byGrip = in.tracked && h.armedGrip && squeeze >= kTake;
        const bool byTrigger = in.tracked && h.armedTrigger && trigger >= kTake;
        if (byGrip)
            h.armedGrip = false;
        if (byTrigger)
            h.armedTrigger = false;
        h.crossed = byGrip || byTrigger;
        if (h.crossed)
            h.crossedAt = g_clock;
        // On the launcher's grip the trigger fires it (input.cpp), and a hand busy elsewhere takes nothing here.
        if (in.onGripB || in.busy || !in.tracked) {
            h.crossed = false;
            h.crossedAt = -1.0f;
        }
        h.fresh = h.crossedAt >= 0.0f && g_clock - h.crossedAt <= kReachWindow && h.grasp >= kTake;
    }
    // The rifle's own claim on the belly: when its magazine last came out (or its bolt locked back).
    if (f.rifleOpen && !g_rifleWas)
        g_rifleOpenedAt = g_clock;
    g_rifleWas = f.rifleOpen;

    // ---- Not in use (another gun, the car, free hands off, m320byhand 0): the firing is the game's. For a moment (kAwayWait) everything is kept, as a state
    // read that fails for a frame or a draw missed would otherwise throw it all away: the launcher as the game has it, the round in the hand still drawn in it,
    // the glove easing back to the real hand. Then whatever the hand has goes, the barrel stays as it was. ----
    if (!(f.active && g_byHand.load(std::memory_order_relaxed))) {
        g_inactiveFor += dt;
        const bool waiting = g_wasActive && g_inactiveFor < kAwayWait;
        if (g_wasActive && !waiting) {
            CallOffFlight(f, r);
            LetGoOfAll(f, r, true);
            FinishBarrel();
            if (g_content == Content::kSliding || g_content == Content::kPinched) {
                g_content = Content::kSliding;
                g_v = 0.0f;
            }
            g_gloveBlend = g_fingersShare = 0.0f;
            g_offRim = 0;
            for (Hand& h : g_hands)
                h.pressing = h.haveTouch = h.haveOnGun = false;
            g_wasActive = false;
        }
        ForgetMotion();
        r.drawn = false;
        r.fireReady = true;
        r.holding = false;
        if (waiting) {
            const HandIn& in = f.hand[w];
            if (g_hand == HandState::kRound && in.tracked) {
                float roundView[12], inGlove[12];
                RoundInGlove(w, inGlove);
                Compose(in.glove, inGlove, roundView);
                Compose(f.headPose, roundView, g_handRoom);  // (it falls from where the hand has it, if it comes to that)
            }
            PublishHandRound(f, w, r);
            PublishGlove(dt, false, g_hand == HandState::kRound, false, r);
            g_hands[w].haveTouch = false;  // (a push starts from two frames in use running)
            r.holding = g_hand != HandState::kFree;
        }
        Publish(r);
        r.state = waiting ? "not in use for a moment: everything kept" : "not in use";
        return r;
    }
    g_inactiveFor = 0.0f;
    g_wasActive = true;
    if (g_first) {
        // First sight (a new life, a launch): shut, with a live round in it if the count says so, else the spent case (the user may come with it fired).
        g_first = false;
        g_live = f.loaded;
        g_counted = true;
        g_content = Content::kSeated;
        g_s = g_v = 0.0f;
        g_barrel = Barrel::kShut;
        g_angle = 0.0f;
        g_latched = true;
        g_loadedWas = f.loaded;
    }

    // ---- The scripted hand's round starting (TestOpenEmpty): emptied and open ----
    if (g_testOpenEmpty.exchange(false)) {
        CallOffFlight(f, r);
        LetGoOfAll(f, r, false);
        if (g_content != Content::kNone && g_live && g_counted)
            r.unload = true;
        g_content = Content::kNone;
        g_s = g_v = 0.0f;
        g_movedOut = false;
        g_entered = false;
        if (g_latched || g_barrel == Barrel::kShut || g_barrel == Barrel::kSnapping) {
            if (g_latched)
                Unlatch(r);
            SpringFrom(0.0f);
        }
        r.note = "the test's round: emptied and opened";
    }

    // ---- The count's own moves ----
    const bool liveIn = g_content == Content::kSeated && g_live;
    const bool liveAnywhere = (g_live && (liveIn || g_content == Content::kSliding || g_content == Content::kPinched)) || (g_content == Content::kFlying && g_flyLive);
    if (g_loadedWas && !f.loaded && liveIn) {
        // Fired: the spent case stays in the barrel (no extractor: the real one's).
        g_live = false;
        r.fired = true;
    } else if (!g_loadedWas && f.loaded && !liveAnywhere && g_clock - g_seatedAt > kOwnSeat) {
        // Its count up without the hand (the game's own reload, a checkpoint): shut and loaded. (Not a round of the hand's just seated: the count comes up a
        // frame or two after it, by when the muzzle up may already have it sliding back out.)
        CallOffFlight(f, r);
        LetGoOfAll(f, r, true);
        g_content = Content::kSeated;
        g_live = true;
        g_counted = true;
        g_s = g_v = 0.0f;
        g_barrel = Barrel::kShut;
        g_angle = 0.0f;
        g_latched = true;
        r.reloaded = true;
    }
    g_loadedWas = f.loaded;

    // ---- The game's own reload (X held on its grip): its animation has the launcher until its round is in. ----
    if (f.gameReload) {
        CallOffFlight(f, r);
        LetGoOfAll(f, r, true);
        if (g_content == Content::kSliding || g_content == Content::kPinched) {
            g_content = Content::kSeated;
            g_s = g_v = 0.0f;
        }
        g_barrel = Barrel::kShut;
        g_angle = 0.0f;
        g_latched = true;
        g_gloveBlend = g_fingersShare = 0.0f;
        for (Hand& h : g_hands)
            h.pressing = false;
        ForgetMotion();
        r.drawn = false;
        // Not in use: ready, so the firing never waits on it. (input.cpp drops pulls while its own reload runs; a reload state read stale would otherwise turn
        // every pull into a release press thrown away here, and the launcher would neither fire nor reload.)
        r.fireReady = true;
        r.holding = false;
        Publish(r);
        r.state = "the game's reload";
        return r;
    }

    // ---- The gun's movement, as drawn last frame: the flick, gravity, the jab. Each difference over the time it spans: the frame before this one. ----
    float omega[3] = {}, breechRoom[3] = {};
    bool spun = false, jumpNow = false;
    const bool motion = f.haveGunRoom && f.gunTracked && dt > 0.0f;
    float span = 0.0f;
    if (motion) {
        span = g_dtWas;
        if (!(span > 0.0f))
            ForgetMotion();  // (nothing to go by: the first, or the frame before took the longest and what the gun did across it isn't read)
        Transform(f.gunRoom, kMouthShut, breechRoom);
        if (g_haveGunWas) {
            Spin(g_gunWas, f.gunRoom, span, omega);
            spun = true;
            // The gun itself (its body's place, as hands.cpp logs the controller's) or its breech this far in a frame: tracking. (Rolled about its length
            // at once, the breech can move back against a jump of the gun: 5 cm across read 3.6 there.)
            const float bodyNow[3] = {f.gunRoom[3], f.gunRoom[7], f.gunRoom[11]}, bodyWas[3] = {g_gunWas[3], g_gunWas[7], g_gunWas[11]};
            jumpNow = Apart(bodyNow, bodyWas) > kJump || (g_breechCount > 0 && Apart(breechRoom, g_breechAt[0]) > kJump);
        }
        std::memcpy(g_gunWas, f.gunRoom, sizeof(g_gunWas));
        g_haveGunWas = true;
        g_dtWas = dt < kLongestFrame ? dt : 0.0f;  // (the time to the gun drawn this frame, which the next frame's gunRoom will be)
    } else {
        ForgetMotion();
    }
    float omegaSeen[3] = {omega[0], omega[1], omega[2]};
    if (spun && g_haveOmega)
        for (int i = 0; i < 3; ++i)
            omegaSeen[i] = 0.5f * (omega[i] + g_omegaWas[i]);  // two frames' worth
    if (spun) {
        std::memcpy(g_omegaWas, omega, sizeof(g_omegaWas));
        g_haveOmega = true;
    }
    if (f.test == 6) {
        // weapon.txt "m320test 6": a scripted roll for the flick, 600 degrees/s for 50 ms: to the right at 0.5 s of every 3, to the left at 1.5 s.
        const float phase = std::fmod(g_clock, 3.0f);
        omegaSeen[0] = omegaSeen[1] = 0.0f;
        omegaSeen[2] = phase >= 0.5f && phase < 0.55f ? 10.47f : phase >= 1.5f && phase < 1.55f ? -10.47f : 0.0f;
        spun = true;
        jumpNow = false;
    }
    const int flick = spun ? Flick(omegaSeen, jumpNow, f.test == 6 ? dt : span) : 0;  // (the scripted roll goes by the module's own clock)
    // The breech's place over the last frames: its speed (for a drop) and its pull along the gun (the jab). A tracking jump starts them again.
    bool jab = false;
    float breechVelocity[3] = {};
    if (motion) {
        if (jumpNow) {
            g_breechCount = 0;
            g_alongCount = 0;
        }
        for (int k = 2; k > 0; --k) {
            std::memcpy(g_breechAt[k], g_breechAt[k - 1], sizeof(g_breechAt[k]));
            g_breechDt[k] = g_breechDt[k - 1];
        }
        std::memcpy(g_breechAt[0], breechRoom, sizeof(g_breechAt[0]));
        g_breechDt[0] = span;  // (from the one before it)
        g_breechCount = std::min(g_breechCount + 1, 3);
        if (g_breechCount >= 2 && g_breechDt[0] > 0.0f)
            for (int i = 0; i < 3; ++i)
                breechVelocity[i] = (g_breechAt[0][i] - g_breechAt[1][i]) / g_breechDt[0];
        if (g_breechCount >= 3 && g_breechDt[0] > 0.0f && g_breechDt[1] > 0.0f) {
            float accel[3];
            for (int i = 0; i < 3; ++i)
                accel[i] = 2.0f * ((g_breechAt[0][i] - g_breechAt[1][i]) / g_breechDt[0] - (g_breechAt[1][i] - g_breechAt[2][i]) / g_breechDt[1]) /
                           (g_breechDt[0] + g_breechDt[1]);
            const float muzzle[3] = {f.gunRoom[2], f.gunRoom[6], f.gunRoom[10]};
            g_along[2] = g_along[1], g_along[1] = g_along[0], g_along[0] = Dot(accel, muzzle);
            g_alongCount = std::min(g_alongCount + 1, 3);
            const float need = g_jab.load(std::memory_order_relaxed);
            int hard = 0;
            for (int k = 0; k < g_alongCount; ++k)
                hard += need > 0.0f && g_along[k] >= need ? 1 : 0;
            if (hard >= 2 && g_jabArmed) {
                jab = true;
                g_jabArmed = false;
            } else if (hard == 0) {
                g_jabArmed = true;
            }
        }
    }

    // ---- The barrel's own swing, and the round on its way home ----
    switch (g_barrel) {
    case Barrel::kSpringing:
        g_springT += dt;
        g_angle = SpringAngle(g_springT);
        if (g_springT >= kSpringEnd - 1e-5f) {
            g_angle = kOpenMost;
            g_barrel = Barrel::kOpen;
            r.openStop = true;
        }
        break;
    case Barrel::kSnapping:
        StepSnap(dt, r);
        break;
    default:
        break;
    }
    if (g_content == Content::kFlying) {
        g_fly += dt / std::max(g_flyFor, 0.01f);
        if (g_fly >= 1.0f) {
            g_content = Content::kSeated;
            g_s = g_v = 0.0f;
            g_live = g_flyLive;
            (g_live ? r.seated : r.caseSeated) = true;
            g_counted = g_counted || g_live;
            g_seatedAt = g_clock;
            g_movedOut = false;
            if (g_hand == HandState::kRiding)
                g_hand = HandState::kFree;
        }
    }

    const bool liveCounted = g_content == Content::kSeated && g_live && f.loaded;

    // ---- The release: a pull of the trigger on its grip, when it isn't ready to fire (input.cpp). input.cpp presses it only from its idle, no shot under way,
    // so f.inGameHand isn't asked (it reads in hand long after a put-away: a pull eaten every time). ----
    Hand& h = g_hands[w];
    const HandIn& in = f.hand[w];
    if (f.releasePress) {
        if (g_latched && g_barrel == Barrel::kShut) {
            if (liveCounted) {
                r.note = g_flickOpens.load(std::memory_order_relaxed) ? "loaded: a pull fires it (a flick of the wrist opens it)" : "loaded: a pull fires it";
            } else {
                Unlatch(r);
                SpringFrom(0.0f);
                h.pressing = true;
            }
        } else {
            r.dryPull = true;  // the real one won't fire unlocked
            r.note = "the barrel isn't locked: the trigger does nothing";
        }
    }
    if (h.pressing && (!in.onGripB || Finite(in.trigger) < kArm))
        h.pressing = false;

    // ---- The wrist's flick: open, fired or still loaded (the user, 2026-11-05: a way to open it "without shooting the launcher": "Flick only"; a live round
    // stays in it, counted, till it's taken out or slides out), never while input.cpp's own shot or reload of it is under way. (Not asked: the game's hand on
    // it, which reads in hand long after a put-away and refused every flick then.) ----
    if (flick > 0) {
        if ((g_flickOpens.load(std::memory_order_relaxed) || f.test == 6) && g_latched && g_barrel == Barrel::kShut && !f.shotUnderway) {
            Unlatch(r);
            SpringFrom(0.0f);
            r.flickOpen = true;
        }
    } else if (flick < 0) {
        if (!g_latched && g_barrel != Barrel::kSnapping && g_hand != HandState::kBarrel) {
            if (!Blocked() && g_content != Content::kPinched) {
                StartSnap();
                r.flickShut = true;
            } else {
                r.blocked = true;
                r.note = "the flick didn't shut it: the round half out is in the way";
            }
        }
    }

    // ---- The hand at work ----
    const bool handOk = in.tracked && f.gunTracked;
    float gunInverse[12], middleView[3] = {}, onGun[3] = {}, middle[3];
    InvertRigid(f.gunPose, gunInverse);
    InGlove(w, kHandInGlove, middle);
    if (in.tracked) {
        Transform(in.glove, middle, middleView);
        Transform(gunInverse, middleView, onGun);
        NoteGrip(h, in.gripRoom, dt);
    } else {
        h.gripCount = 0;
    }
    // Lost: what it has is kept a second. A round in the hand needs only the hand's own controller; the rim and the barrel need the gun's as well.
    const bool keeps = g_hand == HandState::kRound ? in.tracked : handOk;
    if ((g_hand == HandState::kPinch || g_hand == HandState::kRound || g_hand == HandState::kBarrel) && !keeps) {
        g_lostFor += dt;
        if (g_lostFor > kLost)
            LetGoOfAll(f, r, true);
    } else {
        g_lostFor = 0.0f;
    }
    const HoldData& pinch = HoldFor(w, Hold::kPinch);

    // The fingers round a round carried (the fitted carry, once SetHold has it): over from the rim's pinch once the glove's off the launcher, back as it goes onto
    // the rim again.
    const float (*carried)[12] = w == 0 && g_carrySet ? HoldFor(w, Hold::kCarry).fingers : nullptr;
    // A round put in from the hand stays in the carry's grip all the way home (the hand holds it by its base: the user, 2026-11-05), no going over to the rim's pinch.
    const bool keepCarry = g_hand == HandState::kRound || (g_hand == HandState::kPinch && g_entered) || (g_hand == HandState::kRiding && g_rideCarry);
    if (keepCarry) {
        if (g_hand != HandState::kRound || g_gloveBlend <= 0.0f)
            g_carryMix = std::min(1.0f, g_carryMix + dt / kCarryEase);
    } else {
        g_carryMix = std::max(0.0f, g_carryMix - dt / kGloveEase);
    }

    // A round in the hand: let go of, put into the barrel's back end, or carried.
    if (g_hand != HandState::kRound)
        g_haveNoseWas = false;
    if (g_hand == HandState::kRound && in.tracked) {
        float roundView[12], roundInGlove[12];
        RoundInGlove(w, roundInGlove);  // (as drawn)
        Compose(in.glove, roundInGlove, roundView);
        Compose(f.headPose, roundView, g_handRoom);
        if (h.grasp < kLetGo) {
            float fromNeck[3];
            FromNeck(f, in.gripRoom, fromNeck);
            const bool atBelly = f.haveBody && (in.forceBelly || ammozone::Inside(fromNeck, ammozone::kPutBackMore));
            if (g_handLive && atBelly) {
                r.putBack = true;
            } else {
                r.drop = r.letGo = true;
                r.dropLive = g_handLive;
                r.dropFromBarrel = false;
                std::memcpy(r.dropView, roundView, sizeof(r.dropView));
                HandSpeed(h, r.dropVelocityRoom);
            }
            g_hand = HandState::kFree;
        } else if (f.gunTracked) {
            // Put in by hand: its nose into the open barrel's back end, in line with it, going in: onto the barrel's line, the hand on its rim.
            float onGunRound[12], nose[3], base[3], mouth[3], inward[3], axis[3], fromMouth[3];
            Compose(gunInverse, roundView, onGunRound);
            Transform(onGunRound, kRoundNoseOnBone, nose);
            Transform(onGunRound, kRoundBaseOnBone, base);
            MouthAt(g_angle, mouth);
            ExitAt(g_angle, inward);
            for (float& v : inward)
                v = -v;
            for (int i = 0; i < 3; ++i)
                axis[i] = nose[i] - base[i], fromMouth[i] = nose[i] - mouth[i];
            const float deep = Dot(fromMouth, inward);  // how far in its nose is (below 0: still outside)
            const float across[3] = {fromMouth[0] - inward[0] * deep, fromMouth[1] - inward[1] * deep, fromMouth[2] - inward[2] * deep};
            const float off = Length(across), length = Length(axis);
            const float turned = length > 1e-6f ? std::acos(std::clamp(Dot(axis, inward) / length, -1.0f, 1.0f)) : kPi;
            float speedIn = 0.0f;
            if (g_haveNoseWas && dt > 0.0f) {
                const float moved[3] = {nose[0] - g_noseWas[0], nose[1] - g_noseWas[1], nose[2] - g_noseWas[2]};
                if (Length(moved) <= kJump)  // (a tracking jump isn't a push)
                    speedIn = Dot(moved, inward) / dt;
            }
            std::memcpy(g_noseWas, nose, sizeof(g_noseWas));
            g_haveNoseWas = true;
            if (Length(fromMouth) >= kRearm)
                g_rearmed = true;  // (out of the way since it came out: it may go back on)
            const bool open = !g_latched && g_angle >= kMoveFrom && g_content == Content::kNone && g_barrel != Barrel::kSnapping;
            if (open && g_handLive && g_rearmed && deep >= -kEnterBehind && deep <= kEnterIn && off <= g_enterRadius.load(std::memory_order_relaxed) &&
                turned <= kEnterTurn && (speedIn >= kEnterSpeed || deep >= kEnterDeep)) {
                g_content = Content::kPinched;
                g_live = true;
                g_counted = false;  // (counted once it's home)
                g_s = std::clamp(kClear - std::max(deep, kEnterFirst), 0.0f, kClear - kEnterFirst);
                g_roll = RollOf(g_angle, g_s, onGunRound);  // (turned about its length as the hand had it)
                g_v = 0.0f;
                g_movedOut = true;  // (no case_out: it's going in)
                g_hand = HandState::kPinch;
                g_pinchOut = g_s;
                g_entered = true;
                g_outPast = 0.0f;
                g_offRim = 0;
                g_blockNoted = false;
                g_haveNoseWas = false;
                h.haveOnGun = false;  // (this frame's move of the hand is already in where it went on: the rim's pull starts next frame)
                r.entered = true;
            }
        }
    }

    // On the rim: it moves along the barrel with the hand, out or in. Out with its nose clear of the back end, it's in the hand; pushed in to its last kSeatS
    // (once out 4 cm more than that in this hold), home with a click.
    if (g_hand == HandState::kPinch && handOk) {
        if (h.grasp < kLetGo) {
            // Let go of before it's free: it stays where it is (or gravity takes it on).
            g_content = Content::kSliding;
            g_v = 0.0f;
            g_hand = HandState::kFree;
            g_entered = false;
        } else {
            float exit[3];
            ExitAt(g_angle, exit);
            const float moved[3] = {onGun[0] - h.onGunWas[0], onGun[1] - h.onGunWas[1], onGun[2] - h.onGunWas[2]};
            const float step = h.haveOnGun ? std::clamp(Dot(moved, exit), -kMaxStep, kMaxStep) : 0.0f;
            const float wanted = g_s + step;
            g_outPast = wanted > kClear ? g_outPast + (wanted - std::max(g_s, kClear)) : 0.0f;  // (pulled on past its back end, the round staying at it)
            g_s = std::clamp(wanted, 0.0f, kClear);
            g_pinchOut = std::max(g_pinchOut, g_s);
            if (g_s > kMovingOut && !g_movedOut) {
                r.contentMoving = true;
                g_movedOut = true;
            }
            if (g_s >= kClear && (!g_entered || g_outPast >= kOutPast)) {
                r.freed = true;
                r.unload = g_live && g_counted;
                g_handLive = g_live;
                g_hand = HandState::kRound;
                g_content = Content::kNone;
                g_s = g_v = 0.0f;
                g_movedOut = false;
                g_rearmed = false;  // (its nose is at the back end: out of the way first)
                g_haveNoseWas = false;
                g_entered = false;
                g_outPast = 0.0f;
                float roundView[12], roundInGlove[12];
                RoundInGlove(w, roundInGlove);
                Compose(in.glove, roundInGlove, roundView);
                Compose(f.headPose, roundView, g_handRoom);
            } else if (step < 0.0f && g_s <= kSeatS && g_pinchOut >= kSeatS + kSeatOut) {
                // Pushed past half way: home by itself, the hand riding it (the game's thumb on its base), with a click.
                g_content = Content::kFlying;
                g_fly = 0.0f;
                g_flyS = g_s;
                g_flyFor = std::max(g_seatTime.load(std::memory_order_relaxed), g_s / kSeatSpeed);
                g_flyLive = g_live;
                g_hand = HandState::kRiding;
                g_offRim = 0;
                g_rideCarry = g_entered;  // (put in from the hand: its grip goes home with it)
                g_entered = false;
                r.caught = true;
            } else if (step < 0.0f && g_s <= kBlockS && g_pinchOut > kMovingOut) {
                // Drawn out a little and pushed all the way back by the hand: home now.
                g_content = Content::kSeated;
                g_s = g_v = 0.0f;
                g_movedOut = false;
                g_hand = HandState::kFree;
                g_entered = false;
                (g_live ? r.seated : r.caseSeated) = true;
                g_counted = g_counted || g_live;
                g_seatedAt = g_clock;
            } else {
                // The real hand gone off the rim (only its pull along the barrel moves the round: down, up or forward nothing does): kOffRim from the hand as
                // drawn there for kOffRimFrames running and it has let go, as at the grip let go, the round staying where it is.
                float round[12], drawnMiddle[3];
                RoundAt(g_angle, g_s, round);
                HoldMiddle(w, g_entered && carried ? Hold::kCarry : Hold::kPinch, round, drawnMiddle);
                g_offRim = Apart(onGun, drawnMiddle) > kOffRim ? g_offRim + 1 : 0;
                if (g_offRim >= kOffRimFrames) {
                    g_content = Content::kSliding;
                    g_v = 0.0f;
                    g_hand = HandState::kFree;
                    g_offRim = 0;
                    r.note = "the hand left the rim: let go of";
                    g_entered = false;
                }
            }
        }
    }

    // On the barrel's back end: it swings with the hand about its pin.
    if (g_hand == HandState::kBarrel && handOk) {
        float angle = std::clamp(std::remainder(AngleAbout(onGun) - g_grabOffset, 2.0f * kPi), 0.0f, kOpenMost);
        if (Blocked() && angle < kMoveFrom) {
            angle = kMoveFrom;  // "the case is in the way"
            if (!g_blockNoted) {
                r.blocked = true;
                g_blockNoted = true;
            }
        }
        g_angle = angle;
        float hinge[12], grab[3];
        Hinge(g_angle, hinge);
        Transform(hinge, kBarrelGrab, grab);
        // Off the barrel as on the rim: kOffRim from the hand as drawn there for kOffRimFrames running and it lets go (it held on to 25 cm off, the glove drawn
        // 22 cm from the real hand: "Mr. Elastic", the review 2026-11-05).
        float tubeNow[12], drawnMiddle[3];
        TubeAt(g_angle, tubeNow);
        HoldMiddle(w, Hold::kBarrel, tubeNow, drawnMiddle);
        g_offRim = Apart(onGun, drawnMiddle) > kOffRim ? g_offRim + 1 : 0;
        const bool away = Apart(onGun, grab) > kGrabLost || g_offRim >= kOffRimFrames;
        if (g_grabFromShut) {
            if (g_angle > kGrabSpringAt * kOpenMost) {  // past the point of no return: the springs take it the rest of the way
                SpringFrom(g_angle);
                g_hand = HandState::kFree;
                r.barrelLet = true;
            } else if (h.grasp < kLetGo || away) {
                StartSnap();
                g_hand = HandState::kFree;
                r.barrelLet = true;
            }
        } else if (g_angle <= SnapAt() && !Blocked()) {
            StartSnap();
            g_hand = HandState::kFree;
            r.barrelLet = true;
        } else if (h.grasp < kLetGo || away) {
            SpringBack(r);
            g_hand = HandState::kFree;
            r.barrelLet = true;
        }
    }

    // ---- A fresh squeeze with the hand free: the rim, the barrel, or a new round at the belly ----
    const bool free = in.tracked && !in.onGripB && !in.busy;
    const bool liveInBarrel = g_live && g_content != Content::kNone;  // (on its last way home too: called off, it stays on the line)
    const bool bellyForM320 = !g_latched && (!f.rifleOpen || g_m320OpenedAt > g_rifleOpenedAt);
    r.bellyForM320 = bellyForM320;
    bool took = false;
    if (g_hand == HandState::kFree && free && f.gunTracked && (h.crossed || h.fresh)) {
        float local[3], round[12], target[3], hinge[12], grab[3];
        ToTube(g_angle, onGun, local);
        RoundAt(g_angle, g_s, round);
        HoldMiddle(w, Hold::kPinch, round, target);  // (this hand's own hold: where its glove will be drawn)
        Hinge(g_angle, hinge);
        Transform(hinge, kBarrelGrab, grab);
        const float toRim = Apart(onGun, target), toBarrel = Apart(onGun, grab);
        // The rim, behind the barrel's back end, with the barrel open and a round or a case in it (the manual's way: "Grasp the rim ... pull rearward"); just in
        // front of the back end too, if the rim's the nearer of the two.
        if (!g_latched && g_angle >= kMoveFrom && (g_content == Content::kSeated || g_content == Content::kSliding) && toRim <= kPinchReach &&
            (local[2] < kBehindFace || toRim < toBarrel)) {
            g_hand = HandState::kPinch;
            g_content = Content::kPinched;
            g_v = 0.0f;
            g_offRim = 0;
            g_pinchOut = g_s;
            g_entered = false;
            g_outPast = 0.0f;
            took = r.pinched = true;
            g_fingersShare = 1.0f;
        }
        // The barrel's back end, beside it: to swing it shut, once it's unlatched (the release or the flick opens it, as the real one's release does). Only a hand well
        // nearer it than the launcher's grip or the handguard's line: a hand going to either is theirs. (It once opened a spent, shut barrel too, "forgiving": a squeeze
        // for the handguard coming in from the left, 2-6 cm off the tube's side, took the barrel instead of the handguard: the review, 2026-11-05.)
        if (!took && local[2] >= kBehindFace && g_barrel != Barrel::kSnapping && !g_latched) {
            float gripMiddle[3];
            GripMiddleFor(w, gripMiddle);
            const float reach = kGrabReachShut + (kGrabReachOpen - kGrabReachShut) * std::clamp(g_angle / kOpenMost, 0.0f, 1.0f);
            const float d = toBarrel;
            if (d <= reach && d + kGrabNearer <= Apart(onGun, gripMiddle) && d + kGrabNearer <= FromSegment(onGun, kHandguard[0], kHandguard[1])) {
                g_grabFromShut = g_latched;
                if (g_latched)
                    Unlatch(r);
                g_barrel = Barrel::kHeld;
                g_grabOffset = AngleAbout(onGun) - g_angle;
                g_hand = HandState::kBarrel;
                g_offRim = 0;
                took = r.barrelTaken = true;
                g_fingersShare = 1.0f;
            }
        }
        // A new round at the belly: the belly gives what's open (the user agreed it, 2026-11-05).
        if (!took && bellyForM320) {
            float fromNeck[3];
            FromNeck(f, in.gripRoom, fromNeck);
            const bool inside = f.haveBody && (in.forceBelly || ammozone::Inside(fromNeck));
            if (inside) {
                const float fromGuard = FromSegment(onGun, kHandguard[0], kHandguard[1]);
                float gripMiddle[3];
                GripMiddleFor(w, gripMiddle);
                const char* why = !f.spare                    ? "no rounds left"
                                  : liveInBarrel              ? "a live round is in it"
                                  : fromGuard < kBellyAway || Apart(onGun, gripMiddle) < kBellyAway ? "the hand is at the gun, not the belly"
                                                              : "";
                if (!*why) {
                    g_hand = HandState::kRound;
                    g_handLive = true;
                    h.gripCount = 0;
                    took = r.took = true;
                    g_fingersShare = 1.0f;
                    g_carryMix = 1.0f;     // (wrapped round it at once: the glove isn't on the launcher)
                    g_rearmed = true;      // (from the belly, not out of the barrel: it may go in at once)
                    g_haveNoseWas = false;
                    float roundView[12], roundInGlove[12];
                    RoundInGlove(w, roundInGlove);
                    Compose(in.glove, roundInGlove, roundView);
                    Compose(f.headPose, roundView, g_handRoom);
                } else {
                    r.squeezed = true;
                    r.note = why;
                    std::memcpy(r.fromNeck, fromNeck, sizeof(r.fromNeck));
                    h.crossedAt = -1.0f;  // said once
                }
            }
        }
        if (took) {
            r.spentSqueeze = true;
            h.crossedAt = -1.0f;
        } else if (h.crossed && !r.squeezed) {
            float mouth[3];
            MouthAt(g_angle, mouth);
            if (Apart(onGun, mouth) < kNoteNear) {
                r.squeezed = true;
                FromNeck(f, in.gripRoom, r.fromNeck);
                const bool flickOpens = g_flickOpens.load(std::memory_order_relaxed);
                r.note = g_latched && liveCounted ? (flickOpens ? "loaded and shut: a flick of the wrist opens it" : "loaded and shut: a pull fires it, then a pull opens it")
                         : g_latched              ? (flickOpens ? "shut: the release (a pull of its trigger) or the wrist's flick opens it" : "shut: the release (a pull of its trigger) opens it")
                         : g_angle < kMoveFrom    ? "not open enough yet"
                         : local[2] < kBehindFace ? (g_content == Content::kNone ? "nothing in the barrel to take" : "not on the round's rim")
                                                  : "not on the barrel's back end";
            }
        }
    }

    // ---- A push on the barrel's left side swings it shut, the hand's own (the bolt slap the user liked, reload.cpp) ----
    const bool canPush = g_hand == HandState::kFree && free && f.gunTracked && !g_latched &&
                         (g_barrel == Barrel::kOpen || g_barrel == Barrel::kSpringing || g_barrel == Barrel::kPushed);
    if (canPush) {
        float points[4][3];
        const int count = in.touches > 0 ? std::min(in.touches, 4) : 4;
        for (int i = 0; i < count; ++i) {
            float view[3], inBind[3];
            if (in.touches > 0) {
                std::memcpy(view, in.touch[i], sizeof(view));
            } else {
                InGlove(w, kTouchInGlove[i], inBind);
                Transform(in.glove, inBind, view);
            }
            Transform(gunInverse, view, points[i]);
        }
        const bool pushing = g_barrel == Barrel::kPushed;
        bool any = false;
        float most = g_angle;
        for (int i = 0; i < count; ++i) {
            float local[3];
            ToTube(g_angle, points[i], local);
            const bool along = local[2] >= kTouchBack && local[2] <= kTouchFront && std::fabs(local[1] - kMouthShut[1]) <= kTouchHigh;
            const bool inside = local[0] <= kSurfaceX + (pushing ? kPushKeep : 0.0f);
            bool starts = false;
            if (!pushing && along && inside && h.haveTouch && dt > 0.0f) {
                float was[3];
                ToTube(g_angleWas, h.touchWas[i], was);
                starts = was[0] > kSurfaceX && (points[i][0] - h.touchWas[i][0]) / dt <= -kPushSpeed;
            }
            float angle;
            if (along && inside && (pushing || starts) && AngleOnSurface(points[i], &angle)) {
                most = std::min(most, angle);
                any = true;
            }
        }
        // The slap: a point from more than kSlapFrom out from the side to within kSlapNear of it, or past it, this frame, closing at kSlapSpeed, in the zone.
        // (The hand's own move, both places in the barrel's frame as it is now: the barrel springing open into a hand held still beside it is no slap. A point
        // moving more than kSlapMost in a frame is tracking, not a hand.)
        // By the palm (point 0) and an open hand only: the straight fingers' tips reach 10 cm past it, over the barrel while the palm is still behind its back end,
        // where a hand going for the case's rim comes in fast and squeezing.
        bool slap = false;
        if (!pushing && h.haveTouch && dt > 0.0f && h.grasp < kTake) {
            for (int i = 0; i < 1 && !slap; ++i) {
                float now[3], was[3];
                ToTube(g_angle, points[i], now);
                ToTube(g_angle, h.touchWas[i], was);
                const float closing = (was[0] - now[0]) / dt;
                slap = closing >= kSlapSpeed && Apart(points[i], h.touchWas[i]) <= kSlapMost && was[0] > kSurfaceX + kSlapFrom && now[0] <= kSurfaceX + kSlapNear &&
                       now[2] >= kSlapBack && now[2] <= kSlapFront && std::fabs(now[1] - kMouthShut[1]) <= kSlapHigh;
            }
        }
        for (int i = 0; i < count; ++i)
            std::memcpy(h.touchWas[i], points[i], sizeof(h.touchWas[i]));
        h.haveTouch = true;
        if (slap) {
            if (Blocked()) {
                if (!g_blockNoted) {
                    r.blocked = true;
                    g_blockNoted = true;
                }
            } else {
                g_barrel = Barrel::kPushed;  // (StartSnap from where it is)
                StartSnap();
                r.slapped = true;
                any = false;
            }
        }
        if (any) {
            float angle = std::max(most, 0.0f);
            if (Blocked() && angle < kMoveFrom) {
                angle = std::max(kMoveFrom, std::min(g_angle, kMoveFrom));
                if (!g_blockNoted) {
                    r.blocked = true;
                    g_blockNoted = true;
                }
            }
            g_angle = std::min(g_angle, angle);
            g_barrel = Barrel::kPushed;
            if (g_angle <= SnapAt() && !Blocked())
                StartSnap();
        } else if (pushing) {
            SpringBack(r);  // the hand came off it short of the snap: its springs push it back open
        }
    } else {
        h.haveTouch = false;
        if (g_barrel == Barrel::kPushed)
            SpringBack(r);
    }

    // ---- Shut on a round barely out, it goes home with the barrel; more out, the barrel can't shut past kMoveFrom ----
    if (g_angle < kMoveFrom && (g_content == Content::kSliding || g_content == Content::kSeated)) {
        if (g_s <= kBlockS) {
            g_s = g_v = 0.0f;
            g_content = Content::kSeated;
            g_movedOut = false;
            if (g_live && !g_counted) {  // (a round put in by hand and let go of just short: home with the barrel, counted now)
                r.seated = true;
                g_counted = true;
                g_seatedAt = g_clock;
            }
        } else if (g_barrel == Barrel::kSnapping) {
            SpringFrom(kMoveFrom);
            r.blocked = true;
        } else {
            g_angle = kMoveFrom;
        }
    }

    // ---- The round loose in the open barrel: it slides out once the muzzle is up enough, a jab throws it back (the patent: it "falls out ... by
    // centrifugal action, or manually"). ----
    if (motion && g_angle >= kMoveFrom && (g_content == Content::kSeated || g_content == Content::kSliding)) {
        float exitGun[3], exitRoom[3];
        ExitAt(g_angle, exitGun);
        Turn(f.gunRoom, exitGun, exitRoom);
        const float down = std::asin(std::clamp(-exitRoom[1], -1.0f, 1.0f));  // how far its way out points below level
        const float tip = g_tipStart.load(std::memory_order_relaxed), mu = g_friction.load(std::memory_order_relaxed);
        if (jab) {
            g_v += kJabKick;
            r.jabbed = true;
        }
        const bool moving = g_v != 0.0f || down >= tip || (down <= -tip && g_s > 0.0f);
        if (moving) {
            const float pull = kGravity * std::sin(down), grip = mu * kGravity * std::cos(down);
            const float way = g_v > 0.0f ? 1.0f : g_v < 0.0f ? -1.0f : pull > 0.0f ? 1.0f : -1.0f;
            float v = g_v + (pull - way * grip) * dt;
            if (v * way < 0.0f)
                v = 0.0f;  // its walls stopped it
            g_v = v;
            g_s += g_v * dt;
            g_content = Content::kSliding;
            if (g_s <= 0.0f) {
                g_s = g_v = 0.0f;
                g_content = Content::kSeated;
                g_movedOut = false;
                if (g_live && !g_counted) {  // (a round put in by hand and let go of part way, slid home by itself: counted now)
                    r.seated = true;
                    g_counted = true;
                    g_seatedAt = g_clock;
                }
            }
        }
        if (g_s > kMovingOut && !g_movedOut) {
            r.contentMoving = true;
            g_movedOut = true;
        }
        if (g_s >= kClear && r.drop) {
            // Out, but the hand let go of a round this same frame and a frame has one drop: this one waits the frame at the barrel's back end, drawn, and
            // falls the next (nothing vanishes).
            g_s = kClear;
        } else if (g_s >= kClear) {
            // Out: it falls from where it was drawn (the gun as drawn, its steer and all), at the speed it slid, with the gun's own.
            float pose[12];
            RoundAt(g_angle, kClear, pose);
            DrawnOnGun(f, pose, r.dropView);
            for (int i = 0; i < 3; ++i) {
                r.dropSlideRoom[i] = exitRoom[i] * std::max(g_v, 0.0f);
                r.dropVelocityRoom[i] = r.dropSlideRoom[i] + breechVelocity[i];
            }
            CapSpeed(r.dropVelocityRoom);
            r.drop = r.dropFromBarrel = true;
            r.dropLive = g_live;
            r.unload = g_live && g_counted;  // (one put in by hand that never got home was never counted)
            g_content = Content::kNone;
            g_s = g_v = 0.0f;
            g_movedOut = false;
        }
    }  // (without the gun's turn this frame it waits where it is, its speed kept)

    // ---- What's drawn ----
    if (g_latched || g_content == Content::kNone)
        g_roll = 0.0f;  // (shut in, or gone: the game's own turn again)
    r.drawn = true;
    r.barrel = g_angle;
    Hinge(g_angle, r.hinge);
    TubeAt(g_angle, r.tubeOnGun);
    switch (g_content) {
    case Content::kNone:
        r.round = Round::kHidden;
        Identity(r.roundOnGun);
        break;
    case Content::kFlying:
        r.round = Round::kOnGun;
        FlyPose(g_angle, std::min(g_fly, 1.0f), r.roundOnGun);
        r.roundLive = g_flyLive;
        break;
    default:
        r.round = g_roll != 0.0f ? Round::kOnGun : Round::kGame;  // (turned as the hand put it in: drawn at its own pose, the game's draw has none)
        r.roundOut = g_s;
        RoundAt(g_angle, g_s, r.roundOnGun);
        r.roundLive = g_live;
        break;
    }
    PublishHandRound(f, w, r);
    // The glove on the launcher, as the game's own hand is there: on the rim, riding the round home, on the barrel; easing in and back out.
    bool onIt = false;
    switch (g_hand) {
    case HandState::kPinch: {
        float round[12];
        RoundAt(g_angle, g_s, round);
        // Put in from the hand: held as it was carried (by its base), never going over to the rim's pinch: the hand drawn just where it was. A case taken by its
        // rim: the game's pinch.
        const bool carry = g_entered && carried;
        Compose(round, carry ? HoldFor(w, Hold::kCarry).place : pinch.place, g_gloveOnGun);
        g_glovesHold = carry ? Hold::kCarry : Hold::kPinch;
        g_fingers = pinch.fingers, g_fingersAlt = carried, g_fingersMix = carried ? Smooth(g_carryMix) : 0.0f;
        onIt = g_offRim == 0;  // (counting frames off the rim, it doesn't ease on towards a hand gone)
        break;
    }
    case HandState::kRiding: {
        // Its last way home. Put in from the hand: still held as carried. Else from the rim: the game's thumb on its base by home. Eased on as ever, not set: a round
        // pushed home at once after it went in would have thrown the glove the rest of the way in a frame.
        float pose[12], place[12];
        FlyPose(g_angle, std::min(g_fly, 1.0f), pose);
        if (g_rideCarry && carried) {
            // Held by its base, the hand can't follow it into the barrel (its fingers would go in after it): it lets go where it pushed it past half way and the
            // round slides the rest home by itself, the glove easing back to the real hand.
            float start[12];
            FlyPose(g_angle, 0.0f, start);  // (where its last way home began)
            Compose(start, HoldFor(w, Hold::kCarry).place, g_gloveOnGun);
            g_glovesHold = Hold::kCarry;
            g_fingers = pinch.fingers, g_fingersAlt = carried, g_fingersMix = Smooth(g_carryMix);
            onIt = false;  // (easing off below, its fingers kept as they held it till it's home)
            break;
        } else {
            const HoldData& push = HoldFor(w, Hold::kPush);
            const float e = Smooth(g_fly);
            BlendAboutPoint(pinch.place, push.place, e, kHandInGlove, place);
            Compose(pose, place, g_gloveOnGun);
            g_glovesHold = Hold::kPush;
            g_fingers = pinch.fingers, g_fingersAlt = push.fingers, g_fingersMix = e;
        }
        onIt = true;
        break;
    }
    case HandState::kBarrel: {
        const HoldData& barrel = HoldFor(w, Hold::kBarrel);
        float tube[12];
        TubeAt(g_angle, tube);
        Compose(tube, barrel.place, g_gloveOnGun);
        g_glovesHold = Hold::kBarrel;
        g_fingers = barrel.fingers, g_fingersAlt = nullptr, g_fingersMix = 0.0f;
        onIt = true;
        break;
    }
    case HandState::kRound:
        g_fingers = pinch.fingers, g_fingersAlt = carried, g_fingersMix = carried ? Smooth(g_carryMix) : 0.0f;
        break;
    case HandState::kFree:
        break;
    }
    PublishGlove(dt, onIt, g_hand != HandState::kFree, false, r);
    // On its grip: the index finger on its trigger, or up on the barrel release while it's pushed.
    if (in.onGripB) {
        const float trigger = std::clamp(Finite(in.trigger), 0.0f, 1.0f);
        r.indexOnRelease = h.pressing ? 1.0f : 0.0f;
        r.releaseLift = h.pressing ? 0.004f * std::clamp((trigger - kTake) / (1.0f - kTake), 0.0f, 1.0f) : 0.0f;
        r.trigger = h.pressing ? 0.0f : trigger;
    }

    // ---- The launcher's state ----
    r.fireReady = g_latched && g_barrel == Barrel::kShut && g_content == Content::kSeated && g_live && f.loaded;
    r.wantsRound = bellyForM320 && f.spare && !liveInBarrel && g_hand == HandState::kFree;
    r.holding = g_hand != HandState::kFree;
    if (in.tracked) {
        std::memcpy(h.onGunWas, onGun, sizeof(h.onGunWas));
        h.haveOnGun = f.gunTracked;
    } else {
        h.haveOnGun = false;
    }
    g_angleWas = g_angle;
    Publish(r);
    std::snprintf(g_stateText, sizeof(g_stateText), "%s%s %.1f degrees; %s %s, %.3f m out; the hand %s", BarrelName(), g_latched ? " (latched)" : "",
                  g_angle / kDegree, g_live ? "live" : "spent", ContentName(), g_s, HandName());
    r.state = g_stateText;
    return r;
}

}  // namespace m320

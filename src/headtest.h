#pragma once
// A head replay and its meters. With nobody in the headset, a made-up head takes the place of the located one (xr_runtime.cpp, just before its filter), so what a shaken
// head does to the picture can be read from the log and judged by numbers (tools\headfit.ps1): how far the picture slides off the head, how rough the picture's pose
// is from frame to frame, the black at its edge, the game's camera against the head it was drawn for. The motions and the meters' math are here, free of the game and
// of OpenXR, so tools/headtesttest.cpp runs them. So is the turn carried by the head's own rate (weapon.txt "poserate"), which xr_runtime.cpp's filter and the
// offline test share; and so are the rules a picture is named by and the two meters that run with a person in the headset too (the world's turn, the edge), at the
// end of this file.
//
// The switch file, build\headtest.txt, read when it changes (one already there when the game starts is a run's leftover: never acted on). A line each:
//   shake yaw <degrees> <hz>                         a sine each way about the head as it was when the test started
//   shake pitch <degrees> <hz>
//   shake both <yaw degrees> <pitch degrees> <hz>    both at once, in step
//   burst yaw|pitch <degrees> <swing ms> <pause ms>  a smooth swing there, a pause, a swing back, a pause, over and over (the swing half a cosine: at its fastest
//                                                    pi/2 x degrees / swing); the first swing starts once the ease-in is over
//   track <file>                                     rows "ms yaw pitch [roll x y z]" (degrees and metres, from the head as it was), played at their own times,
//                                                    smoothly between rows (a cubic through each row with the slope its two neighbours give: the head's turn rate
//                                                    has no step at a row, as a real head's has none), looped (a quarter second from the last row back to the
//                                                    first); a name beside the DLL, or a full path
//   located <file>                                   with a track: the located head's own rows at the track's times ("ms yaw pitch"). The located head is then the
//                                                    true head (the track) off by this file's row less the track at that row's time, each frame the row nearest in
//                                                    time (a frame's error is its own, never one between two rows). It takes the place of "predict" and of the
//                                                    noise's turn: a head as the headset really located it, played against a smooth true one
//   neck <forward m> <up m>                          the eyes that far ahead of and above a pivot (0.10 0.08), so the head's place swings as a real head's does; a
//                                                    track's own x y z win
//   predict <ms>                                     0 (the default): the located head IS the true head at the time it is located for. Above 0 it is carried on from
//                                                    the true head that many ms earlier by its speed then, as a runtime's predictor does: a predictor's error is in it
//   noise <degrees> [<mm>]                           the located head (never the true one) off by a random turn and place each frame, that much an axis (root mean square)
//   noisefast <degrees> [<from> <to>]                the noise's turn grows with the true head's speed: "noise" up to <from> degrees a second (30), evenly more up to
//                                                    this much at <to> (150) and above. As uneven as the located head was on 2026-11-12 (off its own curve by 0.11 at
//                                                    30 to 100 degrees a second, 0.21 at 100 to 300, 0.24 above): "noise 0.02 0.2" with "noisefast 0.26 60 180"
//   rate <late ms> [<noise deg/s>] [own]             the head's turn rate as "poserate" is given it in a replay: the true head's own of that many ms before (0), off by
//                                                    a random amount each frame, that many degrees a second an axis (0); "own": given about the head's own axes, not
//                                                    the room's (a runtime that has it so)
//   level [<pitch degrees>]                          the made-up head level, at that pitch (0), however the headset lies; without it, from the head's own pitch and roll
//   log <seconds>                                    the per-frame lines for that long from each change of the file
//   seconds <n>                                      the motion stops by itself after n seconds (60 without the line) and the head eases back to where it was
//   off                                              (or no file) nothing replayed
// The motion eases in over a quarter of a second from the head as it was (no jump for the filter) and eases back the same way at its end.
//
// The angles: one convention for the head in the room and for the game's camera in its world. Both have y up; OpenXR's axes are x right, y up, z back, and the
// camera's rows are right, up, back. A turn is yaw, pitch, roll in degrees, the turn Y(yaw) X(pitch) Z(roll): the yaw about the up axis, then the pitch about the
// head's own right, then the roll about its own back.
//   yaw    turning LEFT is positive (the "View:" lines' "head"): atan2 of the back axis's x and z
//   pitch  looking UP is positive (their "pitch head"): -asin of the back axis's y
//   roll   the head tilted to its LEFT shoulder is positive: atan2 of the right axis's y and the up axis's y
// The game's own yaw, as the "View:" lines' "camera" and "shown" have it (growing to the right: atan2 of the back row's x and -z), is 180 less this yaw.
// The slide is the turn from the picture's pose to the true head at the time the picture is shown, as the picture's own frame has it, in the same three angles: its
// yaw positive, the head has turned left of the picture (the picture's left edge comes into view); its pitch positive, the head looks above it (its top edge does).
// Places: the head's in the room in millimetres (x right, y up, z back: the recentred space), the camera's in the world in metres.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace headtest {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEase = 0.25;  // seconds: the motion eased in from the head as it was, and back to it at its end

struct Turn {
    double x = 0.0, y = 0.0, z = 0.0, w = 1.0;
};
struct Pose {
    Turn turn;
    double place[3] = {};  // the middle between the eyes, metres
};
struct Angles {
    double yaw = 0.0, pitch = 0.0, roll = 0.0;  // degrees
};

Turn Mul(const Turn& a, const Turn& b);  // b first, then a
Turn Inverse(const Turn& q);
void Rotate(const Turn& q, const double* v, double* out);
Turn FromAngles(const Angles& a);
Angles ToAngles(const Turn& q);
// A camera's rows (right, up, back; three floats each) in the same convention: as angles, as a turn.
Angles RowsAngles(const float* rows);
Turn RowsTurn(const float* rows);
double Between(const Turn& a, const Turn& b);  // degrees
// The turn from the picture's pose to the head, as the picture's own frame has it (the slide).
Angles Slide(const Turn& picture, const Turn& head);
// A turn as a vector along its axis (as the room has it), as long as its angle in radians; and back.
void ToVector(const Turn& q, double* v);
Turn FromVector(const double* v);

struct Row {
    double ms = 0.0, yaw = 0.0, pitch = 0.0, roll = 0.0, x = 0.0, y = 0.0, z = 0.0;
    bool placed = false;  // its own x y z given
};
enum class Kind { kNone, kShake, kBurst, kTrack };
struct Config {
    Kind kind = Kind::kNone;
    double yaw = 0.0, pitch = 0.0, hz = 0.0;  // shake: degrees each way, swings a second
    bool burstPitch = false;                  // burst: of the pitch, not the yaw
    double burst = 0.0, swingMs = 0.0, pauseMs = 0.0;
    std::string trackFile;  // track: the file named; its rows (ParseTrack), timed from its first, the yaw and the roll without their jumps at +-180; the loop's length
    std::vector<Row> track;
    double trackMs = 0.0;
    std::string locatedFile;  // located: the file named; its rows as how far the located head is off the track at each one's time (ParseLocated: ms, yaw, pitch)
    std::vector<Row> locatedOff;
    double neckForward = 0.10, neckUp = 0.08;
    double predictMs = 0.0;
    double noiseDegrees = 0.0, noiseMm = 0.0;
    double noiseFast = 0.0, noiseFrom = 30.0, noiseTo = 150.0;  // noisefast (0: the noise the same at any speed)
    double rateLateMs = 0.0, rateNoise = 0.0;                    // rate
    bool rateOwn = false;
    bool level = false;
    double levelPitch = 0.0;
    double logSeconds = 0.0;
    double seconds = 60.0;
    bool Moves() const { return kind != Kind::kNone; }
};
// headtest.txt's text (null or empty: nothing replayed). A "track" or a "located" line only names its file: the caller reads it and hands its text to ParseTrack
// and, after that, to ParseLocated.
Config Parse(const char* text);
// A track's rows. False (the motion off) with fewer than two good rows.
bool ParseTrack(const char* text, Config& c);
// The located head's rows, for the track already read. False (none kept: the located head is the true one, with "predict" and "noise" as asked) with fewer than
// two good rows or no track.
bool ParseLocated(const char* text, Config& c);
// What it does, in a line for the log.
std::string Describe(const Config& c);

// The motion alone, t seconds from its start: degrees and metres from the head as it was.
struct Offsets {
    double yaw = 0.0, pitch = 0.0, roll = 0.0, x = 0.0, y = 0.0, z = 0.0;
    bool placed = false;
};
Offsets OffsetsAt(const Config& c, double t);
// How much of the motion is in the head at t: 0 at the start, 1 from kEase on, back to 0 over kEase from "seconds".
double Envelope(const Config& c, double t);
// The motion and its ease back are over.
bool Over(const Config& c, double t);
// The true head at t, from the head as it was (base).
Pose HeadAt(const Config& c, const Pose& base, double t);
// The head as a runtime would give it for a frame shown at t ("located", or "predict" and "noise"; frame: which frame, for the noise).
Pose LocatedAt(const Config& c, const Pose& base, double t, uint32_t frame);
// The head's turn rate as a runtime would give it for that frame ("rate"): radians a second, a vector along the turn's axis as the room has it (with "own", as
// the head itself has it).
void RateAt(const Config& c, const Pose& base, double t, uint32_t frame, double* out);

// ---- The turn carried by the head's own rate (weapon.txt "poserate"; xr_runtime.cpp FilterViews and the offline test alike) ----
// A low-passed turn is late, and a lead made from the located head's own steps hands their unevenness on three times over. Here the turn is carried from frame to
// frame by the head's turn rate as it is given from outside (the runtime's own, from the headset's motion sensors), which holds nothing back however fast the head
// turns, and is only pulled towards the located turn, through two stages, so no error of the rate builds up and the located head's unevenness comes through no more
// than through two low-passes. Then it is carried on by the same rate for as long as the picture will be old when it is shown.
struct RateFilter {
    bool have = false, haveRate = false;
    Turn stage[2];           // the carried turn, pulled once and twice
    double rateWas[3] = {};  // the last frame's rate
    // How many frames old the pictures' poses are when they are shown, for the lead: a whole number, held. It changes only once kAgeAgreed pictures in a row
    // came with another (a smoothed age stands between two, where no picture is).
    int age = 1, ageSeen = 1, ageRun = 0;
    static constexpr int kAgeAgreed = 36;
    void NoteAge(uint32_t drawnAge);
    // One frame. located: the head's turn as located for this frame. rate: its turn rate then, radians a second as the room has it. seconds: since the last frame
    // (0: none, as on the first). hz: the pull's cut-off (0: the located turn taken whole each frame). lead: seconds the turn is carried on after. fresh: started
    // again at the located turn (a recenter, a gap); it also starts again by itself once the carried turn has come 40 degrees from the located one (tracking found
    // again). apart: how far the carried turn is from the located one, before the lead, degrees. Returns the turn for the picture.
    Turn Step(const Turn& located, const double* rate, double seconds, double hz, double lead, bool fresh, double* apart = nullptr);
    // The carried turn taken from outside (the frames the rate could not be used for: the turn the filter beside it gave), so the next frame goes on from there.
    void Follow(const Turn& turn);
};

// Whether a rate given is the head's: held against the located head's own turning from frame to frame, over the frames in which it turns more than 30 degrees a
// second. As it comes and as the head's rate of some ms before (a runtime may give the last it measured), as the room has it and as the head itself has it.
struct RateFit {
    static constexpr int kKept = 8, kLags = 7;
    static constexpr double kLagMs[kLags] = {-7.0, 0.0, 7.0, 14.0, 21.0, 28.0, 35.0};
    struct Sample {
        double t = 0.0;
        Turn turn;
        double rate[3] = {}, turned[3] = {};  // the rate given; the located head's own turning over the frame that ended here (radians a second, as the room has it)
        bool stepped = false;
    };
    Sample kept[kKept];
    int count = 0;
    double total = 0.0, off[2][kLags] = {};  // since the last line: the turning squared; the rate's distance from it squared (0 as the room has it, 1 the head's own)
    int frames = 0;
    bool fits[2] = {true, true};     // as the last verdict said
    double bestMs[2] = {0.0, 0.0};   // and how late the rate is given, by its best fit
    bool told = false;               // there has been a verdict
    // t: the frame's time in seconds (from any start). rate: as given.
    void Add(const Turn& located, const double* rate, double t);
    void Restart() { count = 0; }  // a gap or a jump: no turning is told across it
    // The verdict from the sums as they stand (fits, bestMs, told). False, and nothing changed, with under 24 frames of turning.
    bool Judge();
    // The line ("Head rate fit: ...") with the verdict, said for the way the rate is used (own: as the head itself has it); then the sums start again. With under
    // 24 frames of turning: no line (0), no verdict, the sums kept.
    int Line(char* out, size_t size, bool own);
};

// "poserate" as xr_runtime.cpp FilterViews runs it each frame, whole (and the offline test alike): the rate taken as the room or the head itself has it, held
// against the located head's own turning (RateFit) and used only once it has shown itself the head's (a third of a second of turning tells) and for as long as
// it stays so, all but left out under a few degrees a second (a still head's picture then stands exactly as two low-passes leave it, whatever noise the rate
// has), and, asked to, made up for being given late (carried on by its own change for as long as the fit says it is late); then RateFilter. A frame it cannot
// do falls back to the turn of the filter beside it, and goes on from there.
struct RatePose {
    RateFilter filter;
    RateFit fit;
    static constexpr double kKnee = 8.0;  // degrees a second: half of the rate is used at this speed, 93% at 30, 99% at 100
    // The rate (as the room has it) of the four frames before, the newest last, and when: for its own change over two frames, and for how uneven it is itself.
    static constexpr int kRates = 4;
    double rateWas[kRates][3] = {}, timeWas[kRates] = {};
    int kept = 0;
    double lateMs = 0.0;  // how late the rate is given, as the fit said lately (0 until it has)
    bool lateKnown = false;
    // This frame: fallen back; the size of the rate as used, degrees a second.
    bool back = false;
    double size = 0.0;
    // Since the last line. unevenSum: how far the rate given lies off the curve through its four neighbours (OffCurve, degrees a second), over unevenOf frames: a
    // real head's rate is smooth (under 1 in a hard shake), and what is more than that is the rate's own noise, which the lead hands to the picture's pose.
    int frames = 0, noRate = 0, notHeads = 0, notYet = 0, unevenOf = 0;
    double sizeSum = 0.0, sizeMost = 0.0, apartSum = 0.0, apartWorst = 0.0, unevenSum = 0.0;
    // One frame. located: the head's turn as located. rate: its turn rate as given, radians a second (null: none given this frame). own: given as the head
    // itself has it. makeUp: carried on for its lateness. t: the frame's time, seconds from any start. seconds: since the last frame (0: none). hz: the pull's
    // cut-off. period: a frame's length, seconds. aheadMs: how much later than the frame's display time the head is located already ("poseahead"). drawnAge: how
    // many poses old the last picture's was. fresh: started again. beside: the turn the filter beside it gave this frame. True: out is the picture's turn.
    // False: fallen back; the caller keeps beside.
    bool Step(const Turn& located, const double* rate, bool own, bool makeUp, double t, double seconds, double hz, double period, double aheadMs, uint32_t drawnAge, bool fresh,
              const Turn& beside, Turn& out);
    // The rate taken as shown to be the head's from the first frame on (a head replay's: it is the made-up head's own, and a run's first second should be this
    // filter's, not the one's beside it). The fit still judges it every second.
    void TakeAsShown() { fit.told = true; }
    // Once a second: "Head rate: ..." into out (always, if a frame came), the fit's own line into fitOut when it has one; the sums start again.
    int Line(char* out, size_t size, char* fitOut, size_t fitSize, bool own, bool makeUp);
};

// One frame's line for the log ("Head test: t ..."), as tools\headfit.ps1 reads it.
enum How { kNamed, kGuessed, kLag, kNewest };  // the picture's pose: named by its own main view; not named, as old as the last one's; pose_lag's guess; this frame's own
enum By { kByFixed, kByAdapt, kByRate, kByFallback };  // this frame's pose: the fixed filter and lead; "poseadapt"; "poserate"; "poserate" fallen back to the filter beside it
struct Frame {
    double ms = 0.0;  // the frame's display time, from the test's start
    Pose truth, located, filtered, picture;
    uint32_t age = 0;
    How how = kNewest;
    bool haveCamera = false, cameraOwn = false;  // own: the build the picture's own main view named; else the newest build turned by the pose it is taken to have
    Angles camera, game;                         // its main camera's turn as drawn, and as the game made it
    double cameraPlace[3] = {}, gamePlace[3] = {};  // the world, metres: where it was drawn from (between the eyes), and where the game put it
    double moveMm[3] = {};                       // how far the head's place and the eye's arm moved it
    bool look = false;                           // turned from the look yaw written
    double lookYaw = 0.0, handYaw = 0.0;         // degrees: the look yaw written and the hand's yaw it was turned with
    uint32_t pose = 0, build = 0;
    // The head as the aim's limit and the walk take it this frame (the filter's own pose, or the smooth one beside a pose candidate); how this frame's pose was
    // made, and the size of the rate it was carried by (degrees a second; 0 without one).
    Turn smooth;
    By by = kByFixed;
    double rate = 0.0;
    // The world's turn (at the end of this file): how far the pose the picture is handed over with stands from the cameras its own main-view uploads carried,
    // degrees. world: by the view build most of them carried (each counted for the newest build its rows fit). worldLast: by the last of
    // them before the first eye's picture went out. worldFirst: by the frame's first. world under 0: no main-view upload to go by, and nothing of it is written;
    // either of the others under 0: that one not known. mixed: its uploads carried more than one view build.
    double world = -1.0, worldLast = -1.0, worldFirst = -1.0;
    bool mixed = false;
};
int FrameLine(const Frame& f, char* out, size_t size);

// Once a second: the slide's mean and worst in yaw and pitch, the head's fastest, the pictures' ages.
struct Second {
    int frames = 0, ages[4] = {};  // drawn with the pose 1 frame old, 2, older, not named
    double yawSum = 0.0, yawWorst = 0.0, pitchSum = 0.0, pitchWorst = 0.0, fastest = 0.0;
    void Add(const Angles& slide, double speed, uint32_t age, bool named);
    int Line(char* out, size_t size) const;
    void Clear() { *this = Second{}; }
};

// How rough a row of angles is at its middle one: how far the third of five in a row (evenly timed) lies off the cubic through the other four, degrees. A smooth
// motion reads next to nothing (a shake of 35 degrees each way three times a second: 0.03 at its turning points, nothing at its fastest), a zigzag of e each way
// on alternate frames 2.67 e, an error of e (root mean square) with no pattern 0.94 e at the middle frame of them. The marks, for the picture's pose at 100 to
// 300 degrees a second: 0.06 is the fixed filter as the user accepts it, 0.6 was "really bad" ("poseadapt 1", 2026-11-12).
double OffCurve(double a, double b, double c, double d, double e);

// ---- Which view build a picture is named by, and the pose told against the cameras drawn (camera.cpp, xr_runtime.cpp; weapon.txt "nameby") ----
// The headset turns each picture from the pose it is told the picture was drawn with to where the head is. Told right, an uneven pose can only move the picture's
// edge; told a pose the picture was not drawn with, the whole world moves by the difference (the user, 2026-11-12, of the pose closest to the head: "the whole
// world was shaken"). The pose told is the one of the view build the picture is named by, and the picture's own main-view uploads (352 bytes of view constants
// each, in the order the game makes them) carry the camera rows of the build each was made from. They need not all carry the same one.
//
// One picture's main-view uploads by the view build each carried, as they come. camera.cpp keeps two of these a picture: one of the uploads that fit one view
// build alone (or builds alike that were all turned with one pose: counted for the newest of them), for the count, and one of every upload by the newest build
// its rows fit, which is the one the picture is named by.
struct Naming {
    static constexpr int kBuilds = 8;  // a picture's uploads carry the last few builds only: as many as camera.cpp keeps
    uint32_t build[kBuilds] = {};      // the builds they carried, each at its number's place (build % kBuilds)
    uint32_t count[kBuilds][3] = {};   // how many carried it: before the first eye's picture went out, between the two eyes' pictures, after the second
    uint32_t first = 0, last = 0;      // the build of the first and of the last of them (0: none came)
    uint32_t lastBeforeEyes = 0;       // and of the last before the first eye's picture went out (0: none came before it)
    int eyes = 0;                      // the eyes' pictures gone out so far (0 to 2)
    void Add(uint32_t carried);
    void EyePicture();
    uint32_t Of(uint32_t carried) const;             // how many carried that build
    uint32_t Of(uint32_t carried, int stage) const;  // ... in one of the three stretches (0 to 2)
    uint32_t All() const;
    int Builds() const;     // how many builds they carried between them
    uint32_t Most() const;  // the build most of them carried; of two with as many, the newer; 0 with none
    // The build the picture is named by. Rule 0: byFirst, as it ever was (the caller's own: the build the frame's first main-view upload fits, the oldest not shown
    // yet where the builds are alike). 1: the build its last upload before the first eye's picture carried (that moment not marked, or nothing before it: its last of
    // the frame). 2: the build most of them carried. With no upload at all, byFirst under any rule.
    uint32_t Named(int rule, uint32_t byFirst) const;
};

// The camera a head pose stands for, by the mapping one view build used: that build's camera (its rows: right, up, back) with its own head's turn taken off and the
// pose's put on. A build's rows are the body's turn times its head's (viewturn.h: the body's level rows turned by the head), so this is those rows as a turn, times
// the turn from the build's head to the pose.
Turn CameraOfPose(const float* buildRows, const Turn& buildHead, const Turn& pose);
// Degrees between a camera as its rows have it and one as a turn.
double CameraApart(const float* rows, const Turn& camera);

// Once a second ("World turn: ..."): how far the pose the headset is told stands from the cameras the picture was drawn with, by the picture's own uploads and by no
// naming. With every picture named right it reads 0.00; named a view build off, the head's turn in one frame. The pose told is made a camera by the mapping of the
// view build the uploaded rows fit, the one those cameras were drawn with, never by that of the build the pose belongs to: the body may have turned between the two
// (the stick, a scripted camera), and that turn is no part of what the headset gets wrong. "The cameras the picture was drawn with" are taken to be
// those of the view build most of its main-view uploads carried. In a picture whose uploads carry two builds, a few carry the other one (eight of 22 to 42,
// 2026-11-12, the frame's first among them); where the rest of them stand in the frame had not been seen when this was written, and a picture's last upload before
// an eye's picture may be one of them as its first is. So the line says the same against those two uploads as well, and how many pictures were so mixed.
struct WorldSecond {
    static constexpr double kOver = 0.05;  // degrees: more than this is not rounding
    int frames = 0, over = 0, mixed = 0, none = 0, handsOff = 0, lastOf = 0, unmarked = 0, firstOf = 0;
    double sum = 0.0, worst = 0.0, lastSum = 0.0, lastWorst = 0.0, firstSum = 0.0, firstWorst = 0.0;
    // One picture. turn: by the build most of its main-view uploads carried. byLast: by its last before the first eye's picture went out (marked false: that moment
    // was not marked, or nothing came before it, and it is by the frame's last). byFirst: by its first. Either under 0: not known. mixed: its uploads carried more
    // than one build. handsOff: its gun and arms were placed for another build than it was named by.
    void Add(double turn, double byLast, bool marked, double byFirst, bool mixed, bool handsOff);
    void AddNone();                          // a picture with no main-view upload to go by
    int Line(char* out, size_t size) const;  // 0 (and nothing written) with no picture
    void Clear() { *this = WorldSecond{}; }
};

// Once a second ("Edge: ..."): how far the pose of the picture handed over stands from the head as located, before any filter, for the frame it is shown in: the
// nearest thing to a replay's slide that can be had with a person in the headset (the true head is not known there, only the located one). Only the frames in which
// the located head turns more than 30 degrees a second count, and a second with fewer than 12 of them says nothing.
struct EdgeSecond {
    static constexpr int kMost = 160, kLeast = 12;
    static constexpr double kTurning = 30.0;  // degrees a second
    int frames = 0, turning = 0;
    double fastest = 0.0;
    float whole[kMost] = {}, yaw[kMost] = {}, pitch[kMost] = {};  // of the frames in which the head turned: the sizes
    // slide: the turn from the picture's pose to the located head, as the picture's own frame has it (Slide). whole: its size. speed: how fast the located head turns.
    void Add(const Angles& slide, double whole, double speed);
    int Line(char* out, size_t size);  // 0 (and nothing written) with under kLeast frames of turning; it sorts what it holds
    void Clear() { *this = EdgeSecond{}; }
};

}

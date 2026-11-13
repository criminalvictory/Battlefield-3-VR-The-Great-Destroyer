# Judges a head replay (build\headtest.txt, src\headtest.h; run by build\guntools\headshake.ps1) from its per-frame lines in the log ("Head test: t ..."): how
# far the picture slid off the head, how much black that left at the picture's edge, how well the game's camera stood where the head was, how old the
# pictures' poses were. Pure: it only reads the log.
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\headfit.ps1 -Log <file> [-From hh:mm:ss] [-To hh:mm:ss] [-Fov "50 50 50 50"]
#       [-Lens "59.3 50.1 49.1 60.4"] [-LensRight "50.4 59.1 49.9 59.6"] [-Csv <file>] [-Skip 0.5] [-Run n] [-FitBy picture|head]
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\headfit.ps1 -SelfTest
# -From and -To are the log's own clock (hh:mm:ss or hh:mm:ss.fff); without them every per-frame line in the file is taken. Several runs in the lines:
# the last is judged (-Run n: the n-th). -Skip: the run's first seconds left out (the motion easing in: 0.25 s of it); the frames after the run's
# "seconds" (the ease back) are left out too.
#
# The angles are src\headtest.h's: yaw left positive, pitch up positive, roll to the left shoulder positive, degrees; the head's place in mm in the
# room, the camera's in metres in the world.
#
# (a) THE SLIDE: the turn from the picture's pose (what the compositor is told the picture was drawn with) to the true head at the time the picture is
#     shown, as the picture's own frame has it, split into yaw and pitch. The compositor turns the picture by this much, and that far the picture's edge
#     comes into view. Mean (of the sizes), 95%, worst; by the head's speed. And the delay that explains it best (the picture's pose is the true head
#     of so many ms before), with what a plain delay leaves.
# (a2) THE ROUGHNESS of the picture's pose: how far each frame's pose lies off the smooth curve (the cubic) through its four neighbours, the yaw and the
#     pitch each: median, 90% of the frames under, worst; and the share of frames in which the pose steps BACK against the motion either side of it. By
#     the true head's speed about that axis. This is what the picture's own frame, its edge and all that is drawn flat in it (the HUD) do against the
#     eye when the compositor turns the picture back by its pose's error: the slide's size is how much edge shows, its roughness is the jitter. The marks
#     (2026-11-12, the user's own shakes): at 100 to 300 degrees a second a median of 0.06 is the fixed filter, which the user accepts; 0.6 was "really
#     bad" ("poseadapt 1"); the fixed filter never steps back, "poseadapt 1" did in 6 to 17% of the frames. Only frames evenly timed (five in a row) are
#     taken. Beside it, the same for the filter's own pose of each frame, the located head, the true head and the smooth head (the aim's and the walk's).
# (b) THE BLACK: for each eye and each side, the degrees of the lens's view the picture does not cover, measured at the middle of that side (from the
#     lens's edge inwards along the level line through the middle for outward and inward, along the upright one for up and down). Convention: -Fov is
#     the picture's half-angles "left right up down" for the left eye, -Lens the lens's own "left right up down" for the left eye (its left is its
#     outward side); the right eye is the left one mirrored (left and right swapped), unless -LensRight gives its own "left right up down". A slide's
#     yaw of +10 (the head 10 degrees left of the picture) takes 10 degrees of picture from the left side of both eyes: outward for the left eye,
#     inward for the right. The corners are measured along the line from each corner to the middle; the share is how much of the lens's view (by its
#     angles) is black; and the picture that would cover the lens's view is given in its own half-angles. -SelfTest checks the convention on slides
#     whose black is known.
# (c) THE WORLD ERROR: the room has one fixed place in the world while the soldier stands and the stick is left alone (the body's yaw and where it
#     stands): fitted over the run as the turn and the move that take the picture's pose to the camera drawn (their one common part: what is told and
#     what is drawn should be the same thing, so nothing of the error itself gets fitted away; -FitBy head fits it between the true head and the
#     camera instead, and the difference between the two is printed either way). Where the mapping changes (the stick's turn, a walk) the run is cut
#     and each stretch has its own. Then, a frame: the camera's place error in mm and turn error in degrees, how much each changes from one frame
#     to the next (the judder seen) and how far the place is off a straight line (the roughness), with what the change means in degrees for things
#     0.4, 2 and 10 m away; by the head's speed. Twice: the camera against the picture's pose (what the compositor is told against what was drawn: a
#     turn here is never corrected, a place neither) and the camera against the true head when the picture is shown (the turn of it is the slide,
#     which the compositor takes out; the place it cannot).
# (d) THE PICTURES' POSES: how many frames old each picture's pose was, how it was told, how often that changed.
# (e) THE WORLD'S TURN: how far the pose each picture went out with stands from the cameras the picture's own main-view uploads carried (the line's "world"
#     part, where the build writes it): by the view build most of them carried, by the last before the first eye's picture went out, by the frame's first;
#     mean, 95%, worst; the frames over 0.05 degrees; the pictures whose uploads carried more than one build. Told the pose it was drawn with, a picture reads
#     0.00; told another build's, the head's turn between the two builds: that far the headset moves the whole world, which no filter's evenness can hide.
#     (The camera of (c) is the build the picture was NAMED by, so (c) cannot see a wrong name; this can.)
# The exit code is 0 when a run was judged; -SelfTest's is the number of its checks that failed.
param([string]$Log = '', [string]$From = '', [string]$To = '', [string]$Fov = '50 50 50 50', [string]$Lens = '59.3 50.1 49.1 60.4', [string]$LensRight = '',
      [string]$Csv = '', [double]$Skip = 0.5, [int]$Run = 0, [string]$FitBy = 'picture', [switch]$SelfTest)

$ErrorActionPreference = 'Stop'
$inv = [Globalization.CultureInfo]::InvariantCulture

if (-not ('HeadFit.Fit' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace HeadFit
{
    public struct Q
    {
        public double x, y, z, w;
        public Q(double x, double y, double z, double w) { this.x = x; this.y = y; this.z = z; this.w = w; }
    }

    // src\headtest.h's angles and turns.
    public static class M
    {
        public const double Deg = Math.PI / 180.0;
        public static Q Mul(Q a, Q b)
        {
            return new Q(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                         a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
        }
        public static Q Inv(Q q) { return new Q(-q.x, -q.y, -q.z, q.w); }
        public static Q Unit(Q q)
        {
            double n = Math.Sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
            return n > 0 ? new Q(q.x / n, q.y / n, q.z / n, q.w / n) : new Q(0, 0, 0, 1);
        }
        public static double[] Rot(Q q, double[] v)
        {
            double tx = 2 * (q.y * v[2] - q.z * v[1]), ty = 2 * (q.z * v[0] - q.x * v[2]), tz = 2 * (q.x * v[1] - q.y * v[0]);
            return new double[] { v[0] + q.w * tx + (q.y * tz - q.z * ty), v[1] + q.w * ty + (q.z * tx - q.x * tz), v[2] + q.w * tz + (q.x * ty - q.y * tx) };
        }
        public static Q About(int axis, double degrees)
        {
            double h = degrees * Deg / 2, s = Math.Sin(h);
            return new Q(axis == 0 ? s : 0, axis == 1 ? s : 0, axis == 2 ? s : 0, Math.Cos(h));
        }
        public static Q FromAngles(double yaw, double pitch, double roll) { return Unit(Mul(Mul(About(1, yaw), About(0, pitch)), About(2, roll))); }
        public static double[] ToAngles(Q q)
        {
            double x = q.x, y = q.y, z = q.z, w = q.w;
            double rightX = 1 - 2 * (y * y + z * z), rightY = 2 * (x * y + w * z), rightZ = 2 * (x * z - w * y), upY = 1 - 2 * (x * x + z * z);
            double backX = 2 * (x * z + w * y), backY = 2 * (y * z - w * x), backZ = 1 - 2 * (x * x + y * y);
            double pitch = Math.Asin(Math.Min(Math.Max(-backY, -1.0), 1.0)) / Deg, yaw, roll;
            if (backX * backX + backZ * backZ > 1e-12) { yaw = Math.Atan2(backX, backZ) / Deg; roll = Math.Atan2(rightY, upY) / Deg; }
            else { yaw = Math.Atan2(-rightZ, rightX) / Deg; roll = 0; }
            return new double[] { yaw, pitch, roll };
        }
        public static double Between(Q a, Q b)
        {
            Q d = Mul(Inv(a), b);
            return 2 * Math.Atan2(Math.Sqrt(d.x * d.x + d.y * d.y + d.z * d.z), Math.Abs(d.w)) / Deg;
        }
        public static Q Slerp(Q a, Q b, double t)
        {
            double dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            if (dot < 0) { b = new Q(-b.x, -b.y, -b.z, -b.w); dot = -dot; }
            double wa = 1 - t, wb = t;
            if (dot < 0.9999995)
            {
                double angle = Math.Acos(dot), s = Math.Sin(angle);
                wa = Math.Sin((1 - t) * angle) / s; wb = Math.Sin(t * angle) / s;
            }
            return Unit(new Q(a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb));
        }
        public static double Len(double[] v) { return Math.Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
    }

    // One frame's line.
    public class Row
    {
        public double ms, clock = -1;
        public Q qTrue, qLoc, qFil, qPic, qCam, qGame;
        public double[] pTrue, pLoc, pFil, pPic;   // mm, the room
        public int age; public string how = "";
        public bool cam; public string camHow = "";
        public double[] cCam, cGame, move;         // metres in the world; the move in mm
        public int look; public double lookYaw, handYaw; public long pose, build;
        public bool haveSmooth; public Q qSmooth;  // the head as the aim's limit and the walk took it
        public string by = ""; public double rate; // how the frame's pose was made (fixed, adapt, rate, fallback), and the rate it was carried by (degrees a second)
        // The world's turn (degrees; NaN: not in the line): by the build most of the picture's main-view uploads carried, by its last before the first eye's picture, by its first.
        public double world = double.NaN, worldLast = double.NaN, worldFirst = double.NaN;
        public bool mixed;                         // its uploads carried more than one view build
        // Worked out.
        public double speed;
        public double[] slide;                      // yaw, pitch, roll, whole
        public double[][] black = new double[2][];  // each eye: outward, inward, up, down
        public int stretch = -1;
        public double[] ePic, eHead;                // the camera's place less where the pose puts it, mm, the world's axes
        public double tPic = double.NaN, tHead = double.NaN;  // and its turn off it, degrees
        public Q qePic, qeHead;                     // that turn itself (none: no error)
    }

    public class Stat
    {
        public int n; public double mean, p95, worst;
        public static Stat Of(List<double> v)
        {
            Stat s = new Stat(); s.n = v.Count;
            if (v.Count == 0) { s.mean = s.p95 = s.worst = double.NaN; return s; }
            List<double> c = new List<double>(v); c.Sort();
            double sum = 0; foreach (double d in c) sum += d;
            s.mean = sum / c.Count; s.worst = c[c.Count - 1];
            s.p95 = c[Math.Min(c.Count - 1, Math.Max(0, (int)Math.Ceiling(0.95 * c.Count) - 1))];
            return s;
        }
    }

    public class Result
    {
        public List<string> report = new List<string>();
        public Dictionary<string, double> key = new Dictionary<string, double>();
        public List<string> csv = new List<string>();
    }

    public static class Fit
    {
        static readonly CultureInfo Inv = CultureInfo.InvariantCulture;
        static string F(string format, params object[] args) { return string.Format(Inv, format, args); }
        static string N(double v, string format) { return double.IsNaN(v) ? "-" : v.ToString(format, Inv); }

        static bool Num(string s, out double v) { return double.TryParse(s, NumberStyles.Float, Inv, out v) && !double.IsNaN(v) && !double.IsInfinity(v); }

        // "name Y P R at X Y Z ...": the turn and the place from the word at.
        static bool Six(string[] w, int at, out Q q, out double[] p)
        {
            q = new Q(0, 0, 0, 1); p = new double[3];
            double a, b, c;
            if (w.Length < at + 7 || w[at + 3] != "at" || !Num(w[at], out a) || !Num(w[at + 1], out b) || !Num(w[at + 2], out c)) return false;
            q = M.FromAngles(a, b, c);
            return Num(w[at + 4], out p[0]) && Num(w[at + 5], out p[1]) && Num(w[at + 6], out p[2]);
        }

        // The runs in these lines: each "Head test: started" (or the frames' time going back) begins one. seconds: each run's own "for N s", or 0.
        public static List<List<Row>> Parse(string[] lines, List<double> seconds)
        {
            List<List<Row>> runs = new List<List<Row>>();
            List<Row> run = null;
            double last = -1;
            foreach (string line in lines)
            {
                int started = line.IndexOf("Head test: started", StringComparison.Ordinal);
                if (started >= 0)
                {
                    run = new List<Row>(); runs.Add(run); last = -1;
                    double s = 0; int f = line.IndexOf("; for ", started, StringComparison.Ordinal);
                    if (f >= 0) { int e = line.IndexOf(" s", f + 6, StringComparison.Ordinal); if (e > f) Num(line.Substring(f + 6, e - f - 6), out s); }
                    seconds.Add(s);
                    continue;
                }
                int at = line.IndexOf("Head test: t ", StringComparison.Ordinal);
                if (at < 0) continue;
                string[] parts = line.Substring(at + 13).Split(new string[] { " | " }, StringSplitOptions.None);
                if (parts.Length < 6) continue;
                Row r = new Row();
                if (!Num(parts[0].Trim(), out r.ms)) continue;
                string[] w = parts[1].Trim().Split(' ');
                if (w[0] != "true" || !Six(w, 1, out r.qTrue, out r.pTrue)) continue;
                w = parts[2].Trim().Split(' ');
                if (w[0] != "located" || !Six(w, 1, out r.qLoc, out r.pLoc)) continue;
                w = parts[3].Trim().Split(' ');
                if (w[0] != "filtered" || !Six(w, 1, out r.qFil, out r.pFil)) continue;
                w = parts[4].Trim().Split(' ');
                if (w[0] != "picture" || !Six(w, 1, out r.qPic, out r.pPic) || w.Length < 11 || w[8] != "age" || !int.TryParse(w[9], out r.age)) continue;
                r.how = w[10];
                w = parts[5].Trim().Split(' ');
                if (w[0] != "camera") continue;
                if (w.Length >= 9 && w[1] != "none" && parts.Length >= 10)
                {
                    string[] g = parts[6].Trim().Split(' '), mv = parts[7].Trim().Split(' '), lk = parts[8].Trim().Split(' '), pb = parts[9].Trim().Split(' ');
                    double[] mm = new double[3];
                    r.cam = Six(w, 2, out r.qCam, out r.cCam) && g[0] == "game" && Six(g, 1, out r.qGame, out r.cGame) && mv.Length >= 4 && Num(mv[1], out mm[0]) && Num(mv[2], out mm[1]) &&
                            Num(mv[3], out mm[2]);
                    r.move = mm; r.camHow = w[1];
                    if (lk.Length >= 5) { int.TryParse(lk[1], out r.look); Num(lk[2], out r.lookYaw); Num(lk[4], out r.handYaw); }
                    if (pb.Length >= 4) { long.TryParse(pb[1], out r.pose); long.TryParse(pb[3], out r.build); }
                }
                // (The line's later parts, where a build writes them: "smooth Y P R", "by <how> <rate>" and "world <most> last <last> first <first> [mixed]", a dash
                // for a number that is not known.)
                for (int k = 6; k < parts.Length; k++)
                {
                    string[] x = parts[k].Trim().Split(' ');
                    double sy, sp, sr;
                    if (x[0] == "smooth" && x.Length >= 4 && Num(x[1], out sy) && Num(x[2], out sp) && Num(x[3], out sr)) { r.qSmooth = M.FromAngles(sy, sp, sr); r.haveSmooth = true; }
                    else if (x[0] == "by" && x.Length >= 3) { r.by = x[1]; Num(x[2], out r.rate); }
                    else if (x[0] == "world" && x.Length >= 2 && Num(x[1], out sy))
                    {
                        r.world = sy;
                        for (int j = 2; j + 1 < x.Length; j += 2)
                        {
                            if (x[j] == "last" && Num(x[j + 1], out sp)) r.worldLast = sp;
                            else if (x[j] == "first" && Num(x[j + 1], out sp)) r.worldFirst = sp;
                        }
                        r.mixed = x[x.Length - 1] == "mixed";
                    }
                }
                if (line.Length > 13 && line[0] == '[')
                {
                    int h, m; double s;
                    string[] c = line.Substring(1, line.IndexOf(']') - 1).Split(':');
                    if (c.Length == 3 && int.TryParse(c[0], out h) && int.TryParse(c[1], out m) && Num(c[2], out s)) r.clock = h * 3600.0 + m * 60.0 + s;
                }
                if (run == null || r.ms < last - 1.0) { run = new List<Row>(); runs.Add(run); seconds.Add(0); }
                last = r.ms;
                run.Add(r);
            }
            // (A run with no frame is none.)
            for (int i = runs.Count - 1; i >= 0; i--) if (runs[i].Count == 0) { runs.RemoveAt(i); seconds.RemoveAt(i); }
            return runs;
        }

        // ---- The black ----
        // Whether the picture covers the direction d (the head's own frame: x right, y up, z back). slide: the true head as the picture's frame has it.
        static bool Covered(Q slide, double[] d, double[] tan)
        {
            double[] p = M.Rot(slide, d);
            if (p[2] > -1e-9) return false;
            double x = p[0] / -p[2], y = p[1] / -p[2];
            return x >= -tan[0] - 1e-12 && x <= tan[1] + 1e-12 && y <= tan[2] + 1e-12 && y >= -tan[3] - 1e-12;
        }
        // side: 0 left, 1 right, 2 up, 3 down; a: degrees from straight ahead.
        static double[] Along(int side, double a)
        {
            double s = Math.Sin(a * M.Deg), c = Math.Cos(a * M.Deg);
            return side == 0 ? new double[] { -s, 0, -c } : side == 1 ? new double[] { s, 0, -c } : side == 2 ? new double[] { 0, s, -c } : new double[] { 0, -s, -c };
        }
        // The degrees of the lens's view not covered at the middle of a side, from its edge inwards.
        static double BlackAt(Q slide, int side, double lens, double[] tan)
        {
            if (Covered(slide, Along(side, lens), tan)) return 0;
            if (!Covered(slide, Along(side, 0), tan)) return lens;
            double lo = 0, hi = lens;
            for (int i = 0; i < 40; i++) { double mid = (lo + hi) / 2; if (Covered(slide, Along(side, mid), tan)) lo = mid; else hi = mid; }
            return lens - lo;
        }
        // The same along the line from a corner of the lens's view to its middle. sx, sy: which corner (-1 left or down, +1 right or up).
        static double BlackCorner(Q slide, int sx, int sy, double[] lens, double[] tan)
        {
            double tx = Math.Tan(lens[sx < 0 ? 0 : 1] * M.Deg) * sx, ty = Math.Tan(lens[sy > 0 ? 2 : 3] * M.Deg) * sy;
            double[] corner = { tx, ty, -1 };
            double[] middle = { 0, 0, -1 };
            if (Covered(slide, corner, tan)) return 0;
            double lo = 0, hi = 1;
            if (!Covered(slide, middle, tan)) lo = hi = 0;
            for (int i = 0; i < 40 && hi > lo; i++) { double mid = (lo + hi) / 2; if (Covered(slide, new double[] { tx * mid, ty * mid, -1 }, tan)) lo = mid; else hi = mid; }
            double[] edge = { tx * lo, ty * lo, -1 };
            double dot = (corner[0] * edge[0] + corner[1] * edge[1] + 1) / (M.Len(corner) * M.Len(edge));
            return Math.Acos(Math.Min(1.0, dot)) / M.Deg;
        }
        // How much of the lens's view is black (by its angles): a grid of 24 x 24.
        static double BlackShare(Q slide, double[] lens, double[] tan)
        {
            int black = 0, all = 0;
            for (int i = 0; i < 24; i++)
                for (int j = 0; j < 24; j++)
                {
                    double az = -lens[0] + (lens[0] + lens[1]) * (i + 0.5) / 24, el = -lens[3] + (lens[3] + lens[2]) * (j + 0.5) / 24;
                    all++;
                    if (!Covered(slide, new double[] { Math.Tan(az * M.Deg), Math.Tan(el * M.Deg), -1 }, tan)) black++;
                }
            return 100.0 * black / all;
        }
        // The picture's half-angles (left, right, up, down) that would cover the lens's view.
        static double[] Needed(Q slide, double[] lens)
        {
            double[] need = { 0, 0, 0, 0 };
            for (int i = 0; i <= 8; i++)
                for (int j = 0; j <= 8; j++)
                {
                    if (i != 0 && i != 8 && j != 0 && j != 8) continue;  // (its edge)
                    double az = -lens[0] + (lens[0] + lens[1]) * i / 8.0, el = -lens[3] + (lens[3] + lens[2]) * j / 8.0;
                    double[] p = M.Rot(slide, new double[] { Math.Tan(az * M.Deg), Math.Tan(el * M.Deg), -1 });
                    if (p[2] > -1e-6) { need[0] = need[1] = need[2] = need[3] = 90; continue; }
                    double x = Math.Atan(p[0] / -p[2]) / M.Deg, y = Math.Atan(p[1] / -p[2]) / M.Deg;
                    need[0] = Math.Max(need[0], -x); need[1] = Math.Max(need[1], x); need[2] = Math.Max(need[2], y); need[3] = Math.Max(need[3], -y);
                }
            return need;
        }
        // One eye's numbers for a slide: the four sides (outward, inward, up, down), the four corners (up-out, up-in, down-out, down-in), the share, the
        // picture needed (outward, inward, up, down). fov and lens: left right up down for this eye; outward: 0 its left (the left eye), 1 its right.
        public static double[] EyeBlack(Q slide, double[] fov, double[] lens, int outward)
        {
            double[] tan = { Math.Tan(fov[0] * M.Deg), Math.Tan(fov[1] * M.Deg), Math.Tan(fov[2] * M.Deg), Math.Tan(fov[3] * M.Deg) };
            int inward = 1 - outward, so = outward == 0 ? -1 : 1;
            double[] need = Needed(slide, lens);
            return new double[] {
                BlackAt(slide, outward, lens[outward], tan), BlackAt(slide, inward, lens[inward], tan), BlackAt(slide, 2, lens[2], tan), BlackAt(slide, 3, lens[3], tan),
                BlackCorner(slide, so, 1, lens, tan), BlackCorner(slide, -so, 1, lens, tan), BlackCorner(slide, so, -1, lens, tan), BlackCorner(slide, -so, -1, lens, tan),
                BlackShare(slide, lens, tan), need[outward], need[inward], need[2], need[3] };
        }

        // ---- The delay ----
        // The true head at a time, between the frames either side. False outside the run.
        static bool TrueAt(List<Row> rows, double ms, out Q q)
        {
            q = new Q(0, 0, 0, 1);
            int n = rows.Count;
            if (n < 2 || ms < rows[0].ms || ms > rows[n - 1].ms) return false;
            int lo = 0, hi = n - 1;
            while (hi - lo > 1) { int mid = (lo + hi) / 2; if (rows[mid].ms <= ms) lo = mid; else hi = mid; }
            double span = rows[hi].ms - rows[lo].ms;
            q = span > 0 ? M.Slerp(rows[lo].qTrue, rows[hi].qTrue, (ms - rows[lo].ms) / span) : rows[lo].qTrue;
            return true;
        }
        // The delay (ms) at which a pose (which: 0 the picture's, 1 the filter's own) is nearest the true head of that much before; what is left then.
        static double BestDelay(List<Row> rows, int which, out double leftMean, out double leftWorst)
        {
            double best = 0, bestCost = double.MaxValue;
            for (int pass = 0; pass < 2; pass++)
            {
                double from = pass == 0 ? -60 : best - 1, to = pass == 0 ? 200 : best + 1, step = pass == 0 ? 1 : 0.05;
                for (double d = from; d <= to + 1e-9; d += step)
                {
                    double sum = 0; int count = 0; Q q;
                    foreach (Row r in rows)
                        if (TrueAt(rows, r.ms - d, out q)) { double a = M.Between(which == 0 ? r.qPic : r.qFil, q); sum += a * a; count++; }
                    if (count * 2 < rows.Count) continue;
                    if (sum / count < bestCost) { bestCost = sum / count; best = d; }
                }
            }
            List<double> left = new List<double>(); Q at;
            foreach (Row r in rows) if (TrueAt(rows, r.ms - best, out at)) left.Add(M.Between(which == 0 ? r.qPic : r.qFil, at));
            Stat s = Stat.Of(left); leftMean = s.mean; leftWorst = s.worst;
            return best;
        }

        static double Median(List<double> v) { if (v.Count == 0) return 0; List<double> c = new List<double>(v); c.Sort(); return c[c.Count / 2]; }
        // The value that share of them are under (0.5 the median, 1 the worst); none: not a number.
        static double Pct(List<double> v, double share) { if (v.Count == 0) return double.NaN; List<double> c = new List<double>(v); c.Sort(); return c[Math.Min(c.Count - 1, (int)(share * (c.Count - 1) + 0.5))]; }
        static readonly double[] SpeedEdges = { 30, 100, 300, 600 };
        static readonly string[] SpeedNames = { "under 30 deg/s", "30 to 100", "100 to 300", "300 to 600", "over 600" };
        static int SpeedBin(double s) { int b = 0; while (b < 4 && s >= SpeedEdges[b]) b++; return b; }
        static string Three(Stat s, string format) { return F("{0,8} {1,8} {2,8}", N(s.mean, format), N(s.p95, format), N(s.worst, format)); }

        public static Result Judge(List<Row> all, double seconds, double skip, double[] fov, double[] lensLeft, double[] lensRight, bool byHead, bool csv)
        {
            Result res = new Result();
            List<Row> rows = new List<Row>();
            foreach (Row r in all) if (r.ms >= skip * 1000.0 && (seconds <= 0 || r.ms <= seconds * 1000.0)) rows.Add(r);
            int n = rows.Count;
            res.key["frames"] = n;
            if (n < 10) { res.report.Add(F("Only {0} frames to judge ({1} lines, the first {2:0.##} s and the ease back left out).", n, all.Count, skip)); return res; }

            // The head's speed, frame to frame.
            for (int i = 1; i < n; i++)
            {
                double dt = (rows[i].ms - rows[i - 1].ms) / 1000.0;
                rows[i].speed = dt > 0 ? M.Between(rows[i - 1].qTrue, rows[i].qTrue) / dt : 0;
            }
            rows[0].speed = rows[1].speed;
            List<double> speeds = new List<double>(); foreach (Row r in rows) speeds.Add(r.speed);
            Stat sp = Stat.Of(speeds);
            double span = (rows[n - 1].ms - rows[0].ms) / 1000.0;
            string clock = "";
            if (rows[0].clock >= 0)
            {
                TimeSpan a = TimeSpan.FromSeconds(rows[0].clock), b = TimeSpan.FromSeconds(rows[n - 1].clock);
                clock = F(" ({0:00}:{1:00}:{2:00.0} to {3:00}:{4:00}:{5:00.0} by the log)", a.Hours, a.Minutes, a.Seconds + a.Milliseconds / 1000.0, b.Hours, b.Minutes, b.Seconds + b.Milliseconds / 1000.0);
            }
            res.report.Add(F("THE RUN: {0} frames over {1:0.0} s{2}, {3:0.0} a second; the head turning {4:0} degrees a second on average, {5:0} at its fastest", n, span, clock,
                             span > 0 ? (n - 1) / span : 0, sp.mean, sp.worst));
            res.key["fastest"] = sp.worst;

            // ---- (a) The slide ----
            List<double>[] sy = new List<double>[6], spi = new List<double>[6], sw = new List<double>[6];
            for (int b = 0; b < 6; b++) { sy[b] = new List<double>(); spi[b] = new List<double>(); sw[b] = new List<double>(); }
            foreach (Row r in rows)
            {
                Q s = M.Mul(M.Inv(r.qPic), r.qTrue);
                double[] a = M.ToAngles(s);
                r.slide = new double[] { a[0], a[1], a[2], M.Between(r.qPic, r.qTrue) };
                int b = SpeedBin(r.speed);
                sy[b].Add(Math.Abs(a[0])); spi[b].Add(Math.Abs(a[1])); sw[b].Add(r.slide[3]);
                sy[5].Add(Math.Abs(a[0])); spi[5].Add(Math.Abs(a[1])); sw[5].Add(r.slide[3]);
            }
            res.report.Add("");
            res.report.Add("(a) THE SLIDE: the turn from the picture's pose to the true head when the picture is shown (degrees; the size of it)");
            res.report.Add(F("  {0,-16} {1,6}   {2,-26} {3,-26} {4,-26}", "", "frames", "yaw: mean      95%    worst", "pitch: mean    95%    worst", "whole: mean    95%    worst"));
            for (int k = 0; k < 6; k++)
            {
                int b = k == 0 ? 5 : k - 1;
                Stat y = Stat.Of(sy[b]), p = Stat.Of(spi[b]), w = Stat.Of(sw[b]);
                res.report.Add(F("  {0,-16} {1,6}   {2}   {3}   {4}", k == 0 ? "all" : SpeedNames[b], y.n, Three(y, "0.00"), Three(p, "0.00"), Three(w, "0.00")));
                if (k == 0)
                {
                    res.key["slideYawMean"] = y.mean; res.key["slideYaw95"] = y.p95; res.key["slideYawWorst"] = y.worst;
                    res.key["slidePitchMean"] = p.mean; res.key["slidePitch95"] = p.p95; res.key["slidePitchWorst"] = p.worst;
                    res.key["slideWorst"] = w.worst;
                }
            }
            double leftMean, leftWorst;
            double delay = BestDelay(rows, 0, out leftMean, out leftWorst);
            res.key["delay"] = delay; res.key["delayLeft"] = leftMean;
            res.report.Add(F("  The picture's pose is nearest the true head of {0:0.0} ms before it is shown (what a plain delay leaves: {1:0.00} degrees on average, {2:0.00} at the worst).",
                             delay, leftMean, leftWorst));
            double filterDelay = BestDelay(rows, 1, out leftMean, out leftWorst);
            res.key["filterDelay"] = filterDelay;
            res.report.Add(F("  The filter's own pose of a frame is nearest the true head of {0:0.0} ms before that frame's ({1:0.00} on average, {2:0.00} at the worst left).", filterDelay,
                             leftMean, leftWorst));
            List<double> off = new List<double>(), offPlace = new List<double>();
            foreach (Row r in rows)
            {
                off.Add(M.Between(r.qTrue, r.qLoc));
                offPlace.Add(M.Len(new double[] { r.pLoc[0] - r.pTrue[0], r.pLoc[1] - r.pTrue[1], r.pLoc[2] - r.pTrue[2] }));
            }
            Stat so = Stat.Of(off), sop = Stat.Of(offPlace);
            res.report.Add(F("  The located head against the true one (\"predict\", \"noise\"): {0:0.000} degrees on average, {1:0.000} at the worst; its place {2:0.00} mm, {3:0.00}.", so.mean,
                             so.worst, sop.mean, sop.worst));

            // ---- (a2) The roughness ----
            // Each pose's yaw (without its jumps at +-180) and pitch, frame by frame: the pictures' poses, the filter's own pose of each frame, the located head, the
            // true head, the smooth head. Five frames evenly timed: the middle one off the cubic through the other four.
            string[] series = { "the pictures' poses", "the filter's pose of each frame", "the located head", "the true head", "the smooth head (the aim's, the walk's)" };
            double[][][] ang = new double[5][][];
            bool smoothThere = true; foreach (Row r in rows) if (!r.haveSmooth) { smoothThere = false; break; }
            for (int s = 0; s < 5; s++)
            {
                ang[s] = new double[2][]; ang[s][0] = new double[n]; ang[s][1] = new double[n];
                for (int i = 0; i < n; i++)
                {
                    Row r = rows[i];
                    double[] a = M.ToAngles(s == 0 ? r.qPic : s == 1 ? r.qFil : s == 2 ? r.qLoc : s == 3 ? r.qTrue : (r.haveSmooth ? r.qSmooth : r.qFil));
                    double yaw = a[0];
                    if (i > 0) { double was = ang[s][0][i - 1]; yaw = was + Math.IEEERemainder(yaw - was, 360.0); }
                    ang[s][0][i] = yaw; ang[s][1][i] = a[1];
                }
            }
            List<double> gaps = new List<double>(); for (int i = 1; i < n; i++) gaps.Add(rows[i].ms - rows[i - 1].ms);
            double frameMs = Median(gaps);
            List<double>[][][] offCurve = new List<double>[5][][];
            int[][][] back = new int[5][][], steps = new int[5][][];
            for (int s = 0; s < 5; s++)
            {
                offCurve[s] = new List<double>[2][]; back[s] = new int[2][]; steps[s] = new int[2][];
                for (int ax = 0; ax < 2; ax++)
                {
                    offCurve[s][ax] = new List<double>[6]; back[s][ax] = new int[6]; steps[s][ax] = new int[6];
                    for (int b = 0; b < 6; b++) offCurve[s][ax][b] = new List<double>();
                }
            }
            int even = 0;
            for (int i = 2; i < n - 2; i++)
            {
                bool ok = frameMs > 0;
                for (int k = i - 1; ok && k <= i + 2; k++) { double g = (rows[k].ms - rows[k - 1].ms) / frameMs; ok = g > 0.6 && g < 1.5; }
                if (!ok) continue;
                even++;
                for (int ax = 0; ax < 2; ax++)
                {
                    double[] head = ang[3][ax];
                    double speed = (Math.Abs(head[i] - head[i - 1]) + Math.Abs(head[i + 1] - head[i])) / 2 / (frameMs / 1000.0);
                    int b = SpeedBin(speed);
                    for (int s = 0; s < 5; s++)
                    {
                        double[] v = ang[s][ax];
                        double curve = Math.Abs((-v[i - 2] + 4 * v[i - 1] - 6 * v[i] + 4 * v[i + 1] - v[i + 2]) / 6.0);
                        offCurve[s][ax][b].Add(curve); offCurve[s][ax][5].Add(curve);
                        double before = v[i - 1] - v[i - 2], step = v[i] - v[i - 1], after = v[i + 1] - v[i];
                        bool stepsBack = before * after > 0 && step * before < 0 && Math.Abs(step) > 0.1;
                        steps[s][ax][b]++; steps[s][ax][5]++;
                        if (stepsBack) { back[s][ax][b]++; back[s][ax][5]++; }
                    }
                }
            }
            string[] binTag = { "u30", "30", "100", "300", "600", "all" };
            res.report.Add("");
            res.report.Add(F("(a2) THE ROUGHNESS: how far a frame's pose lies off the smooth curve through its four neighbours (degrees), by the head's speed about that axis; {0} of the {1} frames " +
                             "evenly timed (a frame {2:0.00} ms)", even, n, frameMs));
            res.report.Add("  The pictures' poses (what the picture's own frame, its edge and all that is flat in it do against the eye); back: frames stepping back against the motion either side");
            res.report.Add(F("  {0,-16} {1,-46} {2,-46}", "", "yaw: frames   median      90%    worst    back", "pitch: frames median      90%    worst    back"));
            for (int k = 0; k < 6; k++)
            {
                int b = k == 0 ? 5 : k - 1;
                StringBuilder line = new StringBuilder(F("  {0,-16}", k == 0 ? "all" : SpeedNames[b]));
                for (int ax = 0; ax < 2; ax++)
                {
                    List<double> v = offCurve[0][ax][b];
                    double share = steps[0][ax][b] > 0 ? 100.0 * back[0][ax][b] / steps[0][ax][b] : double.NaN;
                    line.Append(F(" {0,11} {1,8} {2,8} {3,8} {4,6}%  ", v.Count, N(Pct(v, 0.5), "0.000"), N(Pct(v, 0.9), "0.000"), N(Pct(v, 1.0), "0.00"), N(share, "0.0")));
                    string tag = (ax == 0 ? "rough_yaw_" : "rough_pitch_") + binTag[b];
                    res.key[tag + "_n"] = v.Count; res.key[tag + "_median"] = Pct(v, 0.5); res.key[tag + "_p90"] = Pct(v, 0.9); res.key[tag + "_back"] = share;
                }
                res.report.Add(line.ToString().TrimEnd());
            }
            res.report.Add(F("  The marks, yaw at 100 to 300 degrees a second: a median of 0.06 is the fixed filter as the user accepts it, 0.6 was \"really bad\" (\"poseadapt 1\"); stepping back: " +
                             "the fixed filter never, \"poseadapt 1\" in 6 to 17% of the frames. This run there: {0}, stepping back {1}%.", N(Pct(offCurve[0][0][2], 0.5), "0.000"),
                             N(steps[0][0][2] > 0 ? 100.0 * back[0][0][2] / steps[0][0][2] : double.NaN, "0.0")));
            res.report.Add("  The same (the median; yaw / pitch) for:");
            for (int s = 1; s < 5; s++)
            {
                if (s == 4 && !smoothThere) { res.report.Add(F("    {0,-40} not in these lines", series[s])); continue; }
                StringBuilder line = new StringBuilder(F("    {0,-40}", series[s]));
                for (int b = 0; b < 5; b++)
                    if (offCurve[s][0][b].Count + offCurve[s][1][b].Count > 0)
                        line.Append(F(" {0}: {1} / {2};", SpeedNames[b], N(Pct(offCurve[s][0][b], 0.5), "0.000"), N(Pct(offCurve[s][1][b], 0.5), "0.000")));
                res.report.Add(line.ToString().TrimEnd(';'));
                string tag = s == 1 ? "filter" : s == 2 ? "located" : s == 3 ? "true" : "smooth";
                for (int b = 0; b < 6; b++) res.key["rough_" + tag + "_yaw_" + binTag[b] + "_median"] = Pct(offCurve[s][0][b], 0.5);
            }
            // How each frame's pose was made, and the smooth head against the true one.
            Dictionary<string, int> made = new Dictionary<string, int>(); double fastestRate = 0;
            foreach (Row r in rows) { if (r.by == "") continue; if (!made.ContainsKey(r.by)) made[r.by] = 0; made[r.by]++; fastestRate = Math.Max(fastestRate, r.rate); }
            if (made.Count > 0)
            {
                StringBuilder madeText = new StringBuilder();
                foreach (KeyValuePair<string, int> kv in made) madeText.Append(F("{0}{1} {2}", madeText.Length > 0 ? ", " : "", kv.Value, kv.Key));
                res.report.Add(F("  How each frame's pose was made: {0} (fixed: the fixed filter and lead; adapt: \"poseadapt\"; rate: \"poserate\"; fallback: \"poserate\" kept the filter beside it); " +
                                 "the rate used up to {1:0} degrees a second.", madeText, fastestRate));
                res.key["byRate"] = made.ContainsKey("rate") ? made["rate"] : 0; res.key["byFallback"] = made.ContainsKey("fallback") ? made["fallback"] : 0;
            }
            if (smoothThere)
            {
                List<double> behind = new List<double>(); foreach (Row r in rows) behind.Add(M.Between(r.qSmooth, r.qTrue));
                Stat sb = Stat.Of(behind);
                res.report.Add(F("  The smooth head (what the aim's limit and the walk go by) is {0:0.00} degrees from the true head of its own frame on average, {1:0.00} in the worst 5%, {2:0.00} at the worst.",
                                 sb.mean, sb.p95, sb.worst));
                res.key["smoothBehindMean"] = sb.mean;
            }

            // ---- (b) The black ----
            double[] fovRight = { fov[1], fov[0], fov[2], fov[3] };
            double[][] rest = { EyeBlack(new Q(0, 0, 0, 1), fov, lensLeft, 0), EyeBlack(new Q(0, 0, 0, 1), fovRight, lensRight, 1) };
            List<double>[][] bl = new List<double>[2][];
            for (int e = 0; e < 2; e++) { bl[e] = new List<double>[13]; for (int k = 0; k < 13; k++) bl[e][k] = new List<double>(); }
            foreach (Row r in rows)
            {
                Q s = M.Mul(M.Inv(r.qPic), r.qTrue);
                r.black[0] = EyeBlack(s, fov, lensLeft, 0);
                r.black[1] = EyeBlack(s, fovRight, lensRight, 1);
                for (int e = 0; e < 2; e++) for (int k = 0; k < 13; k++) bl[e][k].Add(r.black[e][k]);
            }
            string[] sides = { "outward", "inward", "up", "down" }, corners = { "up-out", "up-in", "down-out", "down-in" }, eyes = { "left", "right" };
            res.report.Add("");
            res.report.Add(F("(b) THE BLACK: degrees of the lens's view the picture does not cover (the picture {0:0.#}/{1:0.#}/{2:0.#}/{3:0.#}, the left lens {4:0.#}/{5:0.#}/{6:0.#}/{7:0.#}, " +
                             "the right lens {8:0.#}/{9:0.#}/{10:0.#}/{11:0.#}: left/right/up/down)", fov[0], fov[1], fov[2], fov[3], lensLeft[0], lensLeft[1], lensLeft[2], lensLeft[3],
                             lensRight[0], lensRight[1], lensRight[2], lensRight[3]));
            res.report.Add(F("  {0,-6} {1,-9} {2,8} {3,8} {4,8} {5,8}     at the middle of each side", "eye", "side", "at rest", "mean", "95%", "worst"));
            for (int e = 0; e < 2; e++)
                for (int k = 0; k < 4; k++)
                {
                    Stat s = Stat.Of(bl[e][k]);
                    res.report.Add(F("  {0,-6} {1,-9} {2,8:0.0} {3,8:0.0} {4,8:0.0} {5,8:0.0}", eyes[e], sides[k], rest[e][k], s.mean, s.p95, s.worst));
                    res.key["black_" + eyes[e] + "_" + sides[k] + "_rest"] = rest[e][k];
                    res.key["black_" + eyes[e] + "_" + sides[k] + "_mean"] = s.mean;
                    res.key["black_" + eyes[e] + "_" + sides[k] + "_worst"] = s.worst;
                }
            res.report.Add(F("  {0,-6} {1,-9} {2,8} {3,8} {4,8} {5,8}     at each corner, along the line to the middle", "eye", "corner", "at rest", "mean", "95%", "worst"));
            for (int e = 0; e < 2; e++)
                for (int k = 0; k < 4; k++)
                {
                    Stat s = Stat.Of(bl[e][4 + k]);
                    res.report.Add(F("  {0,-6} {1,-9} {2,8:0.0} {3,8:0.0} {4,8:0.0} {5,8:0.0}", eyes[e], corners[k], rest[e][4 + k], s.mean, s.p95, s.worst));
                }
            for (int e = 0; e < 2; e++)
            {
                Stat s = Stat.Of(bl[e][8]);
                res.report.Add(F("  The {0} lens's view black: {1:0.0}% at rest, {2:0.0}% on average, {3:0.0}% in the worst 5% of frames, {4:0.0}% at the worst.", eyes[e], rest[e][8], s.mean,
                                 s.p95, s.worst));
                res.key["share_" + eyes[e] + "_mean"] = s.mean;
            }
            for (int e = 0; e < 2; e++)
            {
                Stat a = Stat.Of(bl[e][9]), b = Stat.Of(bl[e][10]), c = Stat.Of(bl[e][11]), d = Stat.Of(bl[e][12]);
                res.report.Add(F("  A picture that covers the {0} lens's view (its half-angles outward/inward/up/down): at rest {1:0.0}/{2:0.0}/{3:0.0}/{4:0.0}; in 95% of the frames " +
                                 "{5:0.0}/{6:0.0}/{7:0.0}/{8:0.0}; in every frame {9:0.0}/{10:0.0}/{11:0.0}/{12:0.0}.", eyes[e], rest[e][9], rest[e][10], rest[e][11], rest[e][12], a.p95, b.p95,
                                 c.p95, d.p95, a.worst, b.worst, c.worst, d.worst));
            }

            // ---- (c) The world error ----
            List<int> cams = new List<int>();
            for (int i = 0; i < n; i++) if (rows[i].cam) cams.Add(i);
            res.report.Add("");
            res.key["cameras"] = cams.Count;
            if (cams.Count < 20)
            {
                res.report.Add(F("(c) THE WORLD ERROR: only {0} of the {1} frames have their camera (no view turn published? the look not written?): not judged.", cams.Count, n));
            }
            else
            {
                int nc = cams.Count;
                // Each frame's own mapping: the room's turn in the world and where its origin is, from the pose taken as the head.
                double[] yaw = new double[nc]; double[][] origin = new double[nc][];
                for (int k = 0; k < nc; k++)
                {
                    Row r = rows[cams[k]];
                    Q qm = M.Mul(r.qCam, M.Inv(byHead ? r.qTrue : r.qPic));
                    yaw[k] = M.ToAngles(qm)[0];
                    if (k > 0) { while (yaw[k] - yaw[k - 1] > 180) yaw[k] -= 360; while (yaw[k] - yaw[k - 1] < -180) yaw[k] += 360; }
                    double[] p = byHead ? r.pTrue : r.pPic;
                    double[] at = M.Rot(qm, new double[] { p[0] / 1000, p[1] / 1000, p[2] / 1000 });
                    origin[k] = new double[] { r.cCam[0] - at[0], r.cCam[1] - at[1], r.cCam[2] - at[2] };
                }
                // Steadied (the middle of the 15 about each), then cut where it has gone a quarter of a degree or 4 cm from the stretch's start, or frames are missing.
                double[] sYaw = new double[nc]; double[][] sOrigin = new double[nc][];
                for (int k = 0; k < nc; k++)
                {
                    List<double> a = new List<double>(), x = new List<double>(), y = new List<double>(), z = new List<double>();
                    for (int j = Math.Max(0, k - 7); j <= Math.Min(nc - 1, k + 7); j++) { a.Add(yaw[j]); x.Add(origin[j][0]); y.Add(origin[j][1]); z.Add(origin[j][2]); }
                    sYaw[k] = Median(a); sOrigin[k] = new double[] { Median(x), Median(y), Median(z) };
                }
                List<int[]> stretches = new List<int[]>();
                int start = 0;
                for (int k = 1; k <= nc; k++)
                {
                    bool cut = k == nc;
                    if (!cut)
                    {
                        double[] d = { sOrigin[k][0] - sOrigin[start][0], sOrigin[k][1] - sOrigin[start][1], sOrigin[k][2] - sOrigin[start][2] };
                        cut = Math.Abs(sYaw[k] - sYaw[start]) > 0.25 || M.Len(d) > 0.04 || rows[cams[k]].ms - rows[cams[k - 1]].ms > 100;
                    }
                    if (cut) { stretches.Add(new int[] { start, k }); start = k; }
                }
                int used = 0, dropped = 0, good = 0;
                List<double> mapYaw = new List<double>();
                string mapping = "";
                double offYaw = 0;
                for (int si = 0; si < stretches.Count; si++)
                {
                    int a = stretches[si][0], b = stretches[si][1];
                    if (b - a < 18) { dropped += b - a; continue; }
                    good++;
                    // The stretch's mapping: the mean turn, the middle origin.
                    Q first = M.Mul(rows[cams[a]].qCam, M.Inv(byHead ? rows[cams[a]].qTrue : rows[cams[a]].qPic));
                    double qx = 0, qy = 0, qz = 0, qw = 0;
                    for (int k = a; k < b; k++)
                    {
                        Row r = rows[cams[k]];
                        Q qm = M.Mul(r.qCam, M.Inv(byHead ? r.qTrue : r.qPic));
                        double sign = qm.x * first.x + qm.y * first.y + qm.z * first.z + qm.w * first.w < 0 ? -1 : 1;
                        qx += sign * qm.x; qy += sign * qm.y; qz += sign * qm.z; qw += sign * qm.w;
                    }
                    Q map = M.Unit(new Q(qx, qy, qz, qw));
                    List<double> ox = new List<double>(), oy = new List<double>(), oz = new List<double>();
                    for (int k = a; k < b; k++)
                    {
                        Row r = rows[cams[k]];
                        double[] p = byHead ? r.pTrue : r.pPic;
                        double[] at = M.Rot(map, new double[] { p[0] / 1000, p[1] / 1000, p[2] / 1000 });
                        ox.Add(r.cCam[0] - at[0]); oy.Add(r.cCam[1] - at[1]); oz.Add(r.cCam[2] - at[2]);
                    }
                    double[] o = { Median(ox), Median(oy), Median(oz) };
                    double[] ma = M.ToAngles(map);
                    mapYaw.Add(ma[0]);
                    if (good == 1)
                    {
                        mapping = F("the room's yaw in the world {0:0.00} degrees (off a plain yaw by {1:0.000}), its origin at {2:0.000} {3:0.000} {4:0.000}", ma[0],
                                    M.Between(map, M.FromAngles(ma[0], 0, 0)), o[0], o[1], o[2]);
                        res.key["mapYaw"] = ma[0]; res.key["mapX"] = o[0]; res.key["mapY"] = o[1]; res.key["mapZ"] = o[2];
                        // The other way of fitting it, to compare.
                        double ax = 0, ay = 0, az = 0, aw = 0;
                        for (int k = a; k < b; k++)
                        {
                            Row r = rows[cams[k]];
                            Q qm = M.Mul(r.qCam, M.Inv(byHead ? r.qPic : r.qTrue));
                            double sign = qm.x * map.x + qm.y * map.y + qm.z * map.z + qm.w * map.w < 0 ? -1 : 1;
                            ax += sign * qm.x; ay += sign * qm.y; az += sign * qm.z; aw += sign * qm.w;
                        }
                        offYaw = M.Between(map, M.Unit(new Q(ax, ay, az, aw)));
                    }
                    for (int k = a; k < b; k++)
                    {
                        Row r = rows[cams[k]];
                        r.stretch = good;
                        double[] at = M.Rot(map, new double[] { r.pPic[0] / 1000, r.pPic[1] / 1000, r.pPic[2] / 1000 });
                        r.ePic = new double[] { (r.cCam[0] - o[0] - at[0]) * 1000, (r.cCam[1] - o[1] - at[1]) * 1000, (r.cCam[2] - o[2] - at[2]) * 1000 };
                        r.tPic = M.Between(M.Mul(map, r.qPic), r.qCam);
                        r.qePic = M.Mul(M.Inv(M.Mul(map, r.qPic)), r.qCam);
                        at = M.Rot(map, new double[] { r.pTrue[0] / 1000, r.pTrue[1] / 1000, r.pTrue[2] / 1000 });
                        r.eHead = new double[] { (r.cCam[0] - o[0] - at[0]) * 1000, (r.cCam[1] - o[1] - at[1]) * 1000, (r.cCam[2] - o[2] - at[2]) * 1000 };
                        r.tHead = M.Between(M.Mul(map, r.qTrue), r.qCam);
                        r.qeHead = M.Mul(M.Inv(M.Mul(map, r.qTrue)), r.qCam);
                        used++;
                    }
                }
                res.key["stretches"] = good;
                res.report.Add(F("(c) THE WORLD ERROR: {0} frames with their camera, fitted in {1} stretch{2}{3}; by the {4}", used, good, good == 1 ? "" : "es",
                                 dropped > 0 ? F(" ({0} frames in stretches under a quarter second left out: the body turning or walking)", dropped) : "",
                                 byHead ? "true head and the camera" : "picture's pose and the camera"));
                if (good == 0)
                {
                    res.report.Add("  No stretch of a quarter second with the room standing still in the world: the soldier walks or the stick turns all the while. Not judged.");
                }
                else
                {
                    res.report.Add("  The first stretch's mapping: " + mapping + F("; fitted by the {0} instead it comes out {1:0.000} degrees away.", byHead ? "picture's pose" : "true head", offYaw));
                    if (good > 1)
                    {
                        double lo = double.MaxValue, hi = double.MinValue; foreach (double y in mapYaw) { lo = Math.Min(lo, y); hi = Math.Max(hi, y); }
                        res.report.Add(F("  The room's yaw in the world from stretch to stretch: {0:0.00} to {1:0.00} degrees.", lo, hi));
                    }
                    for (int which = 0; which < 2; which++)
                    {
                        List<double>[] place = new List<double>[6], change = new List<double>[6];
                        for (int b = 0; b < 6; b++) { place[b] = new List<double>(); change[b] = new List<double>(); }
                        List<double> turn = new List<double>(), turnChange = new List<double>(), rough = new List<double>(), near = new List<double>(), mid = new List<double>(),
                                     far = new List<double>(), side = new List<double>(), up = new List<double>(), along = new List<double>();
                        for (int k = 0; k < nc; k++)
                        {
                            Row r = rows[cams[k]];
                            if (r.stretch < 0) continue;
                            double[] e = which == 0 ? r.ePic : r.eHead;
                            double size = M.Len(e);
                            int b = SpeedBin(r.speed);
                            place[b].Add(size); place[5].Add(size);
                            turn.Add(which == 0 ? r.tPic : r.tHead);
                            double[] own = M.Rot(M.Inv(r.qCam), e);
                            side.Add(Math.Abs(own[0])); up.Add(Math.Abs(own[1])); along.Add(Math.Abs(own[2]));
                            if (k > 0 && cams[k - 1] == cams[k] - 1 && rows[cams[k - 1]].stretch == r.stretch)
                            {
                                Row was = rows[cams[k - 1]];
                                double[] ew = which == 0 ? was.ePic : was.eHead;
                                double c = M.Len(new double[] { e[0] - ew[0], e[1] - ew[1], e[2] - ew[2] });
                                change[b].Add(c); change[5].Add(c);
                                near.Add(Math.Atan(c / 400.0) / M.Deg); mid.Add(Math.Atan(c / 2000.0) / M.Deg); far.Add(Math.Atan(c / 10000.0) / M.Deg);
                                // The turn error's own change: the two frames' errors, one against the other.
                                turnChange.Add(which == 0 ? M.Between(was.qePic, r.qePic) : M.Between(was.qeHead, r.qeHead));
                                if (k > 1 && cams[k - 2] == cams[k] - 2 && rows[cams[k - 2]].stretch == r.stretch)
                                {
                                    double[] e2 = which == 0 ? rows[cams[k - 2]].ePic : rows[cams[k - 2]].eHead;
                                    rough.Add(M.Len(new double[] { e[0] - 2 * ew[0] + e2[0], e[1] - 2 * ew[1] + e2[1], e[2] - 2 * ew[2] + e2[2] }));
                                }
                            }
                        }
                        string tag = which == 0 ? "pic" : "head";
                        Stat p = Stat.Of(place[5]), c5 = Stat.Of(change[5]), t = Stat.Of(turn), tc = Stat.Of(turnChange), ro = Stat.Of(rough), a04 = Stat.Of(near), a2 = Stat.Of(mid),
                             a10 = Stat.Of(far);
                        res.key[tag + "PlaceMean"] = p.mean; res.key[tag + "PlaceWorst"] = p.worst; res.key[tag + "ChangeMean"] = c5.mean; res.key[tag + "ChangeWorst"] = c5.worst;
                        res.key[tag + "TurnMean"] = t.mean; res.key[tag + "TurnWorst"] = t.worst; res.key[tag + "RoughMean"] = ro.mean;
                        res.report.Add(which == 0 ? "  The camera against the picture's pose (what the compositor is told against what was drawn: nothing corrects this):"
                                                  : "  The camera against the true head when the picture is shown (the turn of it is the slide, which the compositor takes out; the place it cannot):");
                        res.report.Add(F("    {0,-44} {1,8} {2,8} {3,8}", "", "mean", "95%", "worst"));
                        res.report.Add(F("    {0,-44} {1}", "its place off, mm", Three(p, "0.00")));
                        res.report.Add(F("    {0,-44} {1}", "  that changing from frame to frame, mm", Three(c5, "0.00")));
                        res.report.Add(F("    {0,-44} {1}", "  that off a straight line (rough), mm", Three(ro, "0.00")));
                        res.report.Add(F("    {0,-44} {1}", "  the change seen at 0.4 m, degrees", Three(a04, "0.000")));
                        res.report.Add(F("    {0,-44} {1}", "  the change seen at 2 m, degrees", Three(a2, "0.000")));
                        res.report.Add(F("    {0,-44} {1}", "  the change seen at 10 m, degrees", Three(a10, "0.0000")));
                        res.report.Add(F("    {0,-44} {1}", "its turn off, degrees", Three(t, "0.000")));
                        res.report.Add(F("    {0,-44} {1}", "  that changing from frame to frame, degrees", Three(tc, "0.000")));
                        res.report.Add(F("    of its place off, on average: {0:0.00} mm sideways, {1:0.00} up or down, {2:0.00} along the view", Stat.Of(side).mean, Stat.Of(up).mean,
                                         Stat.Of(along).mean));
                        StringBuilder by = new StringBuilder("    by the head's speed (place off mean/worst, its change mean/worst, mm):");
                        for (int b = 0; b < 5; b++)
                        {
                            Stat pb = Stat.Of(place[b]), cb = Stat.Of(change[b]);
                            if (pb.n == 0) continue;
                            by.Append(F(" {0}: {1}/{2}, {3}/{4} ({5});", SpeedNames[b], N(pb.mean, "0.0"), N(pb.worst, "0.0"), N(cb.mean, "0.0"), N(cb.worst, "0.0"), pb.n));
                        }
                        res.report.Add(by.ToString());
                    }
                }
                // What is rough, each by itself: off a straight line a frame, as the "Head rough" and "Eye rough" lines count it.
                List<double> rTrue = new List<double>(), rPic = new List<double>(), rTrueTurn = new List<double>(), rPicTurn = new List<double>(), rFilTurn = new List<double>(),
                             rGame = new List<double>(), rMove = new List<double>(), rCam = new List<double>();
                for (int i = 2; i < n; i++)
                {
                    Row a = rows[i - 2], b = rows[i - 1], c = rows[i];
                    rTrue.Add(M.Len(new double[] { c.pTrue[0] - 2 * b.pTrue[0] + a.pTrue[0], c.pTrue[1] - 2 * b.pTrue[1] + a.pTrue[1], c.pTrue[2] - 2 * b.pTrue[2] + a.pTrue[2] }));
                    rPic.Add(M.Len(new double[] { c.pPic[0] - 2 * b.pPic[0] + a.pPic[0], c.pPic[1] - 2 * b.pPic[1] + a.pPic[1], c.pPic[2] - 2 * b.pPic[2] + a.pPic[2] }));
                    rTrueTurn.Add(Math.Abs(M.Between(b.qTrue, c.qTrue) - M.Between(a.qTrue, b.qTrue)));
                    rPicTurn.Add(Math.Abs(M.Between(b.qPic, c.qPic) - M.Between(a.qPic, b.qPic)));
                    rFilTurn.Add(Math.Abs(M.Between(b.qFil, c.qFil) - M.Between(a.qFil, b.qFil)));
                    if (a.cam && b.cam && c.cam)
                    {
                        rGame.Add(1000 * M.Len(new double[] { c.cGame[0] - 2 * b.cGame[0] + a.cGame[0], c.cGame[1] - 2 * b.cGame[1] + a.cGame[1], c.cGame[2] - 2 * b.cGame[2] + a.cGame[2] }));
                        rMove.Add(M.Len(new double[] { c.move[0] - 2 * b.move[0] + a.move[0], c.move[1] - 2 * b.move[1] + a.move[1], c.move[2] - 2 * b.move[2] + a.move[2] }));
                        rCam.Add(1000 * M.Len(new double[] { c.cCam[0] - 2 * b.cCam[0] + a.cCam[0], c.cCam[1] - 2 * b.cCam[1] + a.cCam[1], c.cCam[2] - 2 * b.cCam[2] + a.cCam[2] }));
                    }
                }
                Stat s1 = Stat.Of(rTrueTurn), s2 = Stat.Of(rFilTurn), s3 = Stat.Of(rPicTurn), s4 = Stat.Of(rTrue), s5 = Stat.Of(rPic), s6 = Stat.Of(rGame), s7 = Stat.Of(rMove),
                     s8 = Stat.Of(rCam);
                res.key["roughPicTurn"] = s3.mean; res.key["roughTrueTurn"] = s1.mean; res.key["roughCam"] = s8.mean;
                res.report.Add("  What is rough by itself (a frame's step off the last frame's; mean / worst):");
                res.report.Add(F("    turns, degrees: the true head {0:0.000} / {1:0.00}, the filter's pose {2:0.000} / {3:0.00}, the pictures' poses {4:0.000} / {5:0.00}", s1.mean, s1.worst, s2.mean,
                                 s2.worst, s3.mean, s3.worst));
                res.report.Add(F("    places, mm: the true head {0:0.00} / {1:0.00}, the pictures' poses {2:0.00} / {3:0.00}, the game's own camera {4} / {5}, the head's move on it {6} / {7}, " +
                                 "the camera drawn {8} / {9}", s4.mean, s4.worst, s5.mean, s5.worst, N(s6.mean, "0.00"), N(s6.worst, "0.00"), N(s7.mean, "0.00"), N(s7.worst, "0.00"),
                                 N(s8.mean, "0.00"), N(s8.worst, "0.00")));
                int looked = 0, named = 0; foreach (int i in cams) { if (rows[i].look != 0) looked++; if (rows[i].camHow == "own") named++; }
                res.report.Add(F("  Of the {0} cameras: {1} the picture's own (named by its main view), {2} the newest build of the pose it is taken to have; {3} turned from the look yaw written.",
                                 cams.Count, named, cams.Count - named, looked));
            }

            // ---- (d) The pictures' poses ----
            int[] ages = new int[5]; int flips = 0;
            Dictionary<string, int> how = new Dictionary<string, int>();
            for (int i = 0; i < n; i++)
            {
                ages[Math.Min(Math.Max(rows[i].age, 0), 4)]++;
                if (!how.ContainsKey(rows[i].how)) how[rows[i].how] = 0;
                how[rows[i].how]++;
                if (i > 0 && rows[i].age != rows[i - 1].age) flips++;
            }
            StringBuilder told = new StringBuilder();
            foreach (KeyValuePair<string, int> kv in how) told.Append(F("{0}{1} {2}", told.Length > 0 ? ", " : "", kv.Value, kv.Key));
            res.report.Add("");
            res.report.Add(F("(d) THE PICTURES' POSES: 1 frame old {0} ({1:0}%), 2 {2} ({3:0}%), 3 {4}, older {5}, this frame's own {6}; told: {7}; the age changed {8} times ({9:0.0} a second)",
                             ages[1], 100.0 * ages[1] / n, ages[2], 100.0 * ages[2] / n, ages[3], ages[4], ages[0], told, flips, span > 0 ? flips / span : 0));
            res.key["age1"] = ages[1]; res.key["age2"] = ages[2]; res.key["flips"] = flips;

            // ---- (e) The world's turn ----
            List<double> wMost = new List<double>(), wLast = new List<double>(), wFirst = new List<double>();
            int wOver = 0, wMixed = 0;
            foreach (Row r in rows)
            {
                if (double.IsNaN(r.world)) continue;
                wMost.Add(r.world);
                if (r.world > 0.05) wOver++;
                if (!double.IsNaN(r.worldLast)) wLast.Add(r.worldLast);
                if (!double.IsNaN(r.worldFirst)) wFirst.Add(r.worldFirst);
                if (r.mixed) wMixed++;
            }
            res.report.Add("");
            res.key["worldFrames"] = wMost.Count;
            if (wMost.Count == 0)
                res.report.Add("(e) THE WORLD'S TURN: no frame's line has it (a build without this meter, or pictures with no main view uploaded).");
            else
            {
                Stat sm = Stat.Of(wMost), sl = Stat.Of(wLast), sf = Stat.Of(wFirst);
                res.report.Add(F("(e) THE WORLD'S TURN: the pose each picture went out with against the cameras its own main-view uploads carried (degrees; {0} of the {1} frames have it)",
                                 wMost.Count, n));
                res.report.Add("                                                       mean      95%    worst");
                res.report.Add(F("  by the view build most of them carried          {0}   over 0.05 degrees in {1} frames ({2:0.0}%)", Three(sm, "0.00"), wOver, 100.0 * wOver / wMost.Count));
                res.report.Add(F("  by the last before the first eye's picture      {0}", Three(sl, "0.00")));
                res.report.Add(F("  by the frame's first                            {0}", Three(sf, "0.00")));
                res.report.Add(F("  {0} pictures ({1:0.0}%) had main-view uploads of more than one view build. A picture told the pose it was drawn with reads 0.00; told another " +
                                 "build's, the head's turn between the two, and the headset moves the whole world by that much.", wMixed, 100.0 * wMixed / wMost.Count));
                res.key["worldMean"] = sm.mean; res.key["world95"] = sm.p95; res.key["worldWorst"] = sm.worst; res.key["worldOver"] = wOver; res.key["worldMixed"] = wMixed;
                res.key["worldLastMean"] = sl.mean; res.key["worldLastWorst"] = sl.worst; res.key["worldFirstMean"] = sf.mean; res.key["worldFirstWorst"] = sf.worst;
            }

            if (csv)
            {
                res.csv.Add("t_ms,head_speed,slide_yaw,slide_pitch,slide_roll,slide,age,how,black_left_outward,black_left_inward,black_left_up,black_left_down,black_right_outward," +
                            "black_right_inward,black_right_up,black_right_down,stretch,camera_vs_picture_mm,camera_vs_picture_x,camera_vs_picture_y,camera_vs_picture_z," +
                            "camera_vs_picture_degrees,camera_vs_head_mm,camera_vs_head_degrees,world_turn,world_turn_by_last,world_turn_by_first,mixed");
                foreach (Row r in rows)
                {
                    StringBuilder b = new StringBuilder();
                    b.Append(F("{0:0.000},{1:0.0},{2:0.0000},{3:0.0000},{4:0.0000},{5:0.0000},{6},{7}", r.ms, r.speed, r.slide[0], r.slide[1], r.slide[2], r.slide[3], r.age, r.how));
                    for (int e = 0; e < 2; e++) for (int k = 0; k < 4; k++) b.Append(F(",{0:0.00}", r.black[e][k]));
                    if (r.stretch >= 0)
                        b.Append(F(",{0},{1:0.000},{2:0.000},{3:0.000},{4:0.000},{5:0.0000},{6:0.000},{7:0.0000}", r.stretch, M.Len(r.ePic), r.ePic[0], r.ePic[1], r.ePic[2], r.tPic, M.Len(r.eHead),
                                   r.tHead));
                    else
                        b.Append(",,,,,,,,");
                    b.Append(F(",{0},{1},{2},{3}", double.IsNaN(r.world) ? "" : N(r.world, "0.0000"), double.IsNaN(r.worldLast) ? "" : N(r.worldLast, "0.0000"),
                               double.IsNaN(r.worldFirst) ? "" : N(r.worldFirst, "0.0000"), double.IsNaN(r.world) ? "" : (r.mixed ? "1" : "0")));
                    res.csv.Add(b.ToString());
                }
            }
            return res;
        }

        // ---- A made-up run for -SelfTest ----
        // A sine in yaw (degrees each way, hz) on a level head, the eyes on a pivot 10 cm behind and 8 cm below; the picture's pose the true head of lagMs before; the
        // camera that pose through a fixed place in the world (the room's yaw mapYaw, its origin) with its place wobbling wobbleMm along the world's x, wobbleHz a
        // second; the pictures' poses 1 frame old, every seventh 2. In the log's own lines. zigzag: the picture's pose that many degrees left and right of that on
        // alternate frames.
        public static string[] Synth(double degrees, double hz, double lagMs, double mapYaw, double[] origin, double wobbleMm, double wobbleHz, int frames)
        {
            return Synth(degrees, hz, lagMs, mapYaw, origin, wobbleMm, wobbleHz, frames, 0.0);
        }
        public static string[] Synth(double degrees, double hz, double lagMs, double mapYaw, double[] origin, double wobbleMm, double wobbleHz, int frames, double zigzag)
        {
            List<string> lines = new List<string>();
            lines.Add(F("[03:00:00.000] Head test: started (made up): a sine in yaw, {0:0.0} degrees each way, {1:0.00} a second; for {2:0.0} s; the eyes 0.100 m ahead of and 0.080 m above their pivot",
                        degrees, hz, frames / 72.0));
            Q map = M.FromAngles(mapYaw, 0, 0);
            for (int i = 0; i < frames; i++)
            {
                double t = i / 72.0;
                StringBuilder b = new StringBuilder();
                b.Append(F("[03:00:{0:00}.{1:000}] Head test: t {2:0.000}", (int)t, (int)((t - (int)t) * 1000), t * 1000));
                double[] times = { t, t, t - 0.023, t - lagMs / 1000 };
                string[] names = { "true", "located", "filtered", "picture" };
                Q pic = new Q(0, 0, 0, 1); double[] picAt = null;
                for (int k = 0; k < 4; k++)
                {
                    double yaw = degrees * Math.Sin(2 * Math.PI * hz * times[k]) + (k == 3 ? (i % 2 == 0 ? zigzag : -zigzag) : 0.0);
                    Q q = M.FromAngles(yaw, 0, 0);
                    double[] neck = M.Rot(q, new double[] { 0, 0.08, -0.10 });
                    double[] at = { 0.02 + neck[0], 1.61 - 0.08 + neck[1], -0.03 + 0.10 + neck[2] };
                    b.Append(F(" | {0} {1:0.0000} {2:0.0000} {3:0.0000} at {4:0.00} {5:0.00} {6:0.00}", names[k], yaw, 0.0, 0.0, at[0] * 1000, at[1] * 1000, at[2] * 1000));
                    if (k == 3) { pic = q; picAt = at; }
                }
                b.Append(F(" age {0} named", i % 7 == 6 ? 2 : 1));
                double[] cam = M.ToAngles(M.Mul(map, pic));
                double[] place = M.Rot(map, picAt);
                double wobble = wobbleMm / 1000 * Math.Sin(2 * Math.PI * wobbleHz * t);
                b.Append(F(" | camera own {0:0.0000} {1:0.0000} {2:0.0000} at {3:0.00000} {4:0.00000} {5:0.00000}", cam[0], cam[1], cam[2], origin[0] + place[0] + wobble, origin[1] + place[1],
                           origin[2] + place[2]));
                b.Append(F(" | game {0:0.0000} 0.0000 0.0000 at {1:0.00000} {2:0.00000} {3:0.00000}", mapYaw, origin[0], origin[1], origin[2]));
                b.Append(F(" | move {0:0.00} {1:0.00} {2:0.00} | look 1 48.3300 hand 0.0000 | pose {3} build {4}", (place[0] + wobble) * 1000, place[1] * 1000, place[2] * 1000, 100 + i, 200 + i));
                // The world's turn as a build with that meter writes it: every seventh picture (the one named 2 old) 1.25 degrees off by most of its uploads; every
                // third's last and first uploads 2.5 off, the first not known in the others; those pictures' uploads of two builds.
                b.Append(F(" | world {0:0.0000} last {1:0.0000} first {2}{3}", i % 7 == 6 ? 1.25 : 0.0, i % 3 == 0 ? 2.5 : 0.0, i % 3 == 0 ? "2.5000" : "-",
                           i % 7 == 6 || i % 3 == 0 ? " mixed" : ""));
                lines.Add(b.ToString());
            }
            lines.Add("[03:00:59.000] Head test: ended after the run: the head is back where it was");
            return lines.ToArray();
        }
    }
}
'@
}

# "a b c d" as four numbers, or $null.
function Four([string]$text) {
    $v = @($text -split '[\s,;/]+' | Where-Object { $_ })
    if ($v.Count -ne 4) { return $null }
    $out = New-Object double[] 4
    for ($i = 0; $i -lt 4; $i++) { $d = 0.0; if (-not [double]::TryParse($v[$i], [Globalization.NumberStyles]::Float, $inv, [ref]$d)) { return $null }; $out[$i] = [Math]::Abs($d) }
    return , $out
}
$fovFour = Four $Fov
$lensLeft = Four $Lens
if (-not $fovFour -or -not $lensLeft) { Write-Host '-Fov and -Lens are four angles each: "left right up down" for the left eye, degrees'; exit 1 }
# The right lens: its own, or the left one mirrored.
$lensRightFour = if ($LensRight) { Four $LensRight } else { , [double[]]@($lensLeft[1], $lensLeft[0], $lensLeft[2], $lensLeft[3]) }
if (-not $lensRightFour) { Write-Host '-LensRight is four angles: "left right up down" for the right eye, degrees'; exit 1 }
$byHead = $FitBy -eq 'head'
if ($FitBy -ne 'head' -and $FitBy -ne 'picture') { Write-Host '-FitBy picture or head'; exit 1 }

if ($SelfTest) {
    $failed = 0
    function Check([string]$what, [double]$got, [double]$want, [double]$within) {
        $ok = [Math]::Abs($got - $want) -le $within
        if (-not $ok) { $script:failed++ }
        Write-Host ('{0,-5} {1}: {2} (wanted {3} within {4})' -f $(if ($ok) { 'PASS' } else { 'FAIL' }), $what, $got.ToString('0.####', $inv), $want.ToString('0.####', $inv), $within.ToString('0.####', $inv))
    }
    # 1. The black's convention, on slides whose black is known (the picture 50 each way, the lens 59.3 out, 50.1 in, 49.1 up, 60.4 down).
    $fifty = [double[]]@(50, 50, 50, 50); $left = [double[]]@(59.3, 50.1, 49.1, 60.4); $right = [double[]]@(50.1, 59.3, 49.1, 60.4)
    $still = New-Object HeadFit.Q(0, 0, 0, 1)
    $b = [HeadFit.Fit]::EyeBlack($still, $fifty, $left, 0)
    Check 'at rest, the left eye outward: 59.3 less 50' $b[0] 9.3 0.001
    Check 'at rest, the left eye inward: 50.1 less 50' $b[1] 0.1 0.001
    Check 'at rest, up: none (49.1 inside 50)' $b[2] 0.0 0.001
    Check 'at rest, down: 60.4 less 50' $b[3] 10.4 0.001
    $yaw = [HeadFit.M]::FromAngles(10, 0, 0)  # the head 10 degrees left of the picture
    $b = [HeadFit.Fit]::EyeBlack($yaw, $fifty, $left, 0)
    Check 'the head 10 left of the picture, the left eye outward (its left): 9.3 and 10' $b[0] 19.3 0.001
    Check 'the left eye inward: none' $b[1] 0.0 0.001
    Check 'the left eye down, at the middle: the level edge a little lower (60.4 less atan(tan 50 cos 10))' $b[3] (60.4 - [Math]::Atan([Math]::Tan(50 * [Math]::PI / 180) * [Math]::Cos(10 * [Math]::PI / 180)) * 180 / [Math]::PI) 0.001
    $b = [HeadFit.Fit]::EyeBlack($yaw, $fifty, $right, 1)
    Check 'the right eye outward (its right): none' $b[0] 0.0 0.001
    Check 'the right eye inward (its left): 0.1 and 10' $b[1] 10.1 0.001
    $b = [HeadFit.Fit]::EyeBlack([HeadFit.M]::FromAngles(-10, 0, 0), $fifty, $left, 0)
    Check 'the head 10 right of the picture, the left eye outward: none' $b[0] 0.0 0.001
    Check 'the left eye inward: 0.1 and 10' $b[1] 10.1 0.001
    $b = [HeadFit.Fit]::EyeBlack([HeadFit.M]::FromAngles(0, 5, 0), $fifty, $left, 0)
    Check 'the head 5 above the picture, up: 49.1 less 45' $b[2] 4.1 0.001
    Check 'down: 60.4 less 55' $b[3] 5.4 0.001
    Check 'the picture that covers it then: 54.1 up' $b[11] 54.1 0.001
    $b = [HeadFit.Fit]::EyeBlack([HeadFit.M]::FromAngles(0, -12, 0), $fifty, $left, 0)
    Check 'the head 12 below the picture, down: 60.4 less 38' $b[3] 22.4 0.001
    $b = [HeadFit.Fit]::EyeBlack($still, [double[]]@(60, 51, 50, 61), $left, 0)
    Check 'a picture past the lens on every side: no black at any corner' ($b[4] + $b[5] + $b[6] + $b[7] + $b[8]) 0.0 0.001
    $b = [HeadFit.Fit]::EyeBlack($yaw, $fifty, $left, 0)
    # (Along the line from the corner to the middle the picture's left edge, now 40 degrees out, is met at tan 40 / tan 59.3 of the way.)
    $rad = [Math]::PI / 180
    $tx = [Math]::Tan(59.3 * $rad); $ty = [Math]::Tan(49.1 * $rad); $met = [Math]::Tan(40 * $rad) / $tx
    $cornerWant = [Math]::Acos(($tx * $tx * $met + $ty * $ty * $met + 1) / ([Math]::Sqrt($tx * $tx + $ty * $ty + 1) * [Math]::Sqrt(($tx * $tx + $ty * $ty) * $met * $met + 1))) / $rad
    Check 'the head 10 left: the corner up and out, black from the corner to the picture''s left edge' $b[4] $cornerWant 0.001
    Check 'the corner up and in is not black' $b[5] 0.0 0.001
    # 2. The slide's split.
    $picture = [HeadFit.M]::FromAngles(-50, 20, 3)
    $slide = [HeadFit.M]::ToAngles([HeadFit.M]::Mul([HeadFit.M]::Inv($picture), [HeadFit.M]::Mul($picture, [HeadFit.M]::FromAngles(-3, 2, 0))))
    Check 'a slide of 3 right and 2 up of a tilted picture: its yaw' $slide[0] -3.0 1e-9
    Check 'its pitch' $slide[1] 2.0 1e-9
    # 3. A made-up run: a sine of 35 degrees 2 a second, the picture 37 ms behind, the camera's place wobbling 3 mm 17 a second, the room turned 131.67 in the world.
    $origin = [double[]]@(1299.7, 85.5, -1140.6)
    $lines = [HeadFit.Fit]::Synth(35, 2, 37, 131.67, $origin, 3, 17, 2160)
    $seconds = New-Object System.Collections.Generic.List[double]
    $runs = [HeadFit.Fit]::Parse($lines, $seconds)
    Check 'the made-up run read: one run' $runs.Count 1 0
    Check 'all its frames' $runs[0].Count 2160 0
    $r = [HeadFit.Fit]::Judge($runs[0], $seconds[0], 0.5, $fifty, $left, $right, $false, $true)
    $k = $r.key
    $turn = 2 * [Math]::PI * 2
    Check 'the lag got back (37 ms)' $k['delay'] 37.0 0.5
    Check 'the slide''s worst in yaw: 70 sin(pi 2 x 0.037)' $k['slideYawWorst'] (70 * [Math]::Sin($turn * 0.037 / 2)) 0.05
    Check 'its mean: 2/pi of that' $k['slideYawMean'] (70 * [Math]::Sin($turn * 0.037 / 2) * 2 / [Math]::PI) 0.05
    Check 'no slide in pitch' $k['slidePitchWorst'] 0.0 0.001
    Check 'the head''s fastest, from frame to frame: 35 sin(2 pi 2 / 72) x 72' $k['fastest'] (35 * [Math]::Sin($turn / 72) * 72) 0.05
    Check 'the black outward for the left eye at the worst: 9.3 and the worst slide' $k['black_left_outward_worst'] (9.3 + 70 * [Math]::Sin($turn * 0.037 / 2)) 0.05
    Check 'one stretch' $k['stretches'] 1 0
    Check 'the room''s yaw in the world got back' $k['mapYaw'] 131.67 0.005
    Check 'its origin (x)' $k['mapX'] 1299.7 0.0005
    Check 'its origin (z)' $k['mapZ'] -1140.6 0.0005
    Check 'the wobble got back: the place off 3 mm at the worst' $k['picPlaceWorst'] 3.0 0.05
    Check 'on average 2/pi of 3 mm' $k['picPlaceMean'] (3 * 2 / [Math]::PI) 0.05
    Check 'changing 6 sin(pi 17/72) mm a frame at the worst' $k['picChangeWorst'] (6 * [Math]::Sin([Math]::PI * 17 / 72)) 0.05
    Check 'the camera''s turn against the picture''s pose: none' $k['picTurnWorst'] 0.0 0.001
    Check 'the camera against the true head: off by the slide' $k['headTurnWorst'] (70 * [Math]::Sin($turn * 0.037 / 2)) 0.05
    Check 'the ages: 1 frame old six in seven' ($k['age1'] / $k['frames']) (6.0 / 7.0) 0.002
    Check 'changing twice in seven frames' ($k['flips'] / $k['frames']) (2.0 / 7.0) 0.002
    Check 'a line a frame for the csv, and its heading' $r.csv.Count ($k['frames'] + 1) 0
    # 3a. The world's turn, as that run's lines have it: every seventh picture 1.25 degrees off by most of its uploads, every third 2.5 off by its last and
    #     by its first, which the others do not know.
    Check 'the world''s turn is in every frame''s line' $k['worldFrames'] $k['frames'] 0
    Check 'its worst: 1.25' $k['worldWorst'] 1.25 1e-9
    Check 'its mean: a seventh of that' $k['worldMean'] (1.25 / 7) 0.002
    Check 'over 0.05 degrees in a seventh of the frames' ($k['worldOver'] / $k['frames']) (1.0 / 7.0) 0.002
    Check 'by the last upload before the first eye''s picture: 2.5 in a third of them' $k['worldLastMean'] (2.5 / 3) 0.003
    Check 'by the first: only the frames that know it count (2.5 each)' $k['worldFirstMean'] 2.5 1e-9
    Check 'pictures with uploads of two builds: a seventh and a third, less those in both' ($k['worldMixed'] / $k['frames']) (9.0 / 21.0) 0.003
    # 3b. The roughness. That run's pictures' poses are a smooth sine's: next to nothing off their own curve, never stepping back. The same run with the picture's
    #     pose 0.3 degrees left and right of that on alternate frames reads 0.8 (16/6 of it) at any speed, and steps back wherever the head turns under 0.5 degrees
    #     a frame.
    Check 'a smooth pose is next to nothing off its own curve (100 to 300 a second)' $k['rough_yaw_100_median'] 0.0 0.01
    Check 'and never steps back' $k['rough_yaw_all_back'] 0.0 0.0
    Check 'frames in each speed bin from 30 to 600 (a sine of 440 at its fastest: 38 a second at its turning points)' ([Math]::Min($k['rough_yaw_30_n'], [Math]::Min($k['rough_yaw_100_n'], $k['rough_yaw_300_n']))) 354 5
    $zig = [HeadFit.Fit]::Synth(35, 2, 37, 131.67, $origin, 3, 17, 2160, 0.3)
    $seconds = New-Object System.Collections.Generic.List[double]
    $runs = [HeadFit.Fit]::Parse($zig, $seconds)
    $kz = ([HeadFit.Fit]::Judge($runs[0], $seconds[0], 0.5, $fifty, $left, $right, $false, $false)).key
    Check 'a pose zigzagging 0.3 each way reads 0.8 off its curve (100 to 300 a second)' $kz['rough_yaw_100_median'] 0.8 0.02
    Check 'the same at any speed' $kz['rough_yaw_all_median'] 0.8 0.02
    Check 'it steps back in no frame over 100 a second (a frame''s turn is more than the zigzag)' $kz['rough_yaw_100_back'] 0.0 0.0
    $slow = [HeadFit.Fit]::Synth(35, 0.1, 37, 131.67, $origin, 3, 17, 2160, 0.3)
    $seconds = New-Object System.Collections.Generic.List[double]
    $runs = [HeadFit.Fit]::Parse($slow, $seconds)
    $ks = ([HeadFit.Fit]::Judge($runs[0], $seconds[0], 0.5, $fifty, $left, $right, $false, $false)).key
    Check 'and in every frame of a head turning 22 a second at the most (a frame''s turn is less than it)' $ks['rough_yaw_u30_back'] 100.0 0.5
    Check 'the filter''s own pose in that run has none of it' $kz['rough_filter_yaw_100_median'] 0.0 0.01
    Check 'the slide is as large as before, to the zigzag (its mean)' $kz['slideYawMean'] $k['slideYawMean'] 0.31
    # 4. The mod's own lines (tools\headtesttest.cpp --synth, if it is built): the same lag and wobble, in a run with a nod in it.
    $exe = Join-Path (Split-Path -Parent $PSScriptRoot) 'build\headtesttest.exe'
    if (Test-Path $exe) {
        $made = Join-Path ([IO.Path]::GetTempPath()) ('headfit_selftest_{0}.log' -f $PID)
        try {
            & $exe --synth $made | Out-Null
            $seconds = New-Object System.Collections.Generic.List[double]
            $runs = [HeadFit.Fit]::Parse([IO.File]::ReadAllLines($made), $seconds)
            Check 'the mod''s own lines read (headtesttest --synth): one run' $runs.Count 1 0
            if ($runs.Count -eq 1) {
                Check 'all its frames' $runs[0].Count 2160 0
                Check 'its seconds from the started line' $seconds[0] 30.0 0.001
                $k = ([HeadFit.Fit]::Judge($runs[0], $seconds[0], 0.5, $fifty, $left, $right, $false, $false)).key
                Check 'its lag got back (37 ms)' $k['delay'] 37.0 0.5
                Check 'its wobble got back (3 mm at the worst)' $k['picPlaceWorst'] 3.0 0.05
                Check 'its room''s yaw in the world (180 less 48.33)' $k['mapYaw'] 131.67 0.005
                Check 'its camera''s turn against the picture''s pose: none' $k['picTurnWorst'] 0.0 0.001
                Check 'its pictures'' poses smooth: next to nothing off their curve at 100 to 300 a second' $k['rough_yaw_100_median'] 0.0 0.01
                Check 'the smooth head is in its lines (there the filter''s own pose, 23 ms behind the head: 2/pi of 70 sin(pi 2 x 0.023) on average)' $k['smoothBehindMean'] 6.42 0.3
                Check 'the world''s turn is in its lines: 1.25 in every seventh picture' $k['worldMean'] (1.25 / 7) 0.002
                Check 'at the worst 1.25' $k['worldWorst'] 1.25 1e-9
                Check 'by its last and by its first upload 2.5 in every third' ($k['worldLastMean'] + $k['worldFirstMean']) (5.0 / 3) 0.006
                Check 'its pictures with uploads of two builds' ($k['worldMixed'] / $k['frames']) (9.0 / 21.0) 0.003
            }
        } finally { Remove-Item $made -ErrorAction SilentlyContinue }
    } else { Write-Host 'SKIP  the mod''s own lines: build\headtesttest.exe is not built' }
    Write-Host "$failed failed"
    exit $failed
}

if (-not $Log) { Write-Host 'headfit.ps1 -Log <file> [-From hh:mm:ss] [-To hh:mm:ss] [-Fov "50 50 50 50"] [-Lens "59.3 50.1 49.1 60.4"] [-Csv <file>]   |   headfit.ps1 -SelfTest'; exit 1 }
if (-not (Test-Path $Log)) { Write-Host "No log at $Log"; exit 1 }
$Log = (Resolve-Path $Log).Path
function Clock([string]$text) {
    if ($text -match '^(\d+):(\d+):(\d+)(\.\d+)?$') {
        $s = [int]$matches[1] * 3600.0 + [int]$matches[2] * 60.0 + [int]$matches[3]
        if ($matches[4]) { $s += [double]::Parse('0' + $matches[4], $inv) }
        return $s
    }
    return -1.0
}
$fromAt = if ($From) { Clock $From } else { -1.0 }
$toAt = if ($To) { Clock $To } else { -1.0 }
if (($From -and $fromAt -lt 0) -or ($To -and $toAt -lt 0)) { Write-Host '-From and -To are the log''s own clock: hh:mm:ss or hh:mm:ss.fff'; exit 1 }

# The head test's lines between -From and -To.
$kept = New-Object System.Collections.Generic.List[string]
$stream = New-Object IO.FileStream($Log, [IO.FileMode]::Open, [IO.FileAccess]::Read, ([IO.FileShare]'ReadWrite, Delete'))
$reader = New-Object IO.StreamReader($stream)
try {
    while ($null -ne ($line = $reader.ReadLine())) {
        if ($line.IndexOf('Head test: ') -lt 0) { continue }
        if (($fromAt -ge 0 -or $toAt -ge 0) -and $line -match '^\[(\d+):(\d+):(\d+)\.(\d+)\]') {
            $at = [int]$matches[1] * 3600.0 + [int]$matches[2] * 60.0 + [int]$matches[3] + [int]$matches[4] / 1000.0
            if ($fromAt -ge 0 -and $at -lt $fromAt) { continue }
            if ($toAt -ge 0 -and $at -gt $toAt) { continue }
        }
        $kept.Add($line)
    }
} finally { $reader.Dispose(); $stream.Dispose() }
$seconds = New-Object System.Collections.Generic.List[double]
$runs = [HeadFit.Fit]::Parse($kept.ToArray(), $seconds)
if ($runs.Count -eq 0) { Write-Host "No per-frame ""Head test: t ..."" lines in $Log (headtest.txt ""log <seconds>"" writes them)"; exit 1 }
$pick = if ($Run -ge 1 -and $Run -le $runs.Count) { $Run - 1 } else { $runs.Count - 1 }
foreach ($line in $kept) { if ($line -match 'Head test: (started|ended|off|the headset|another)') { Write-Host ($line -replace '^\[([^\]]*)\] ', '$1  ') } }
if ($runs.Count -gt 1) { Write-Host "$($runs.Count) runs in these lines: run $($pick + 1) is judged (-Run n picks another)" }
$result = [HeadFit.Fit]::Judge($runs[$pick], $seconds[$pick], $Skip, $fovFour, $lensLeft, $lensRightFour, $byHead, [bool]$Csv)
foreach ($l in $result.report) { Write-Host $l }
if ($Csv -and $result.csv.Count) {
    [IO.File]::WriteAllLines($Csv, $result.csv.ToArray())
    Write-Host "A line a frame written to $Csv"
}
if ($result.key['frames'] -lt 10) { exit 1 }
exit 0

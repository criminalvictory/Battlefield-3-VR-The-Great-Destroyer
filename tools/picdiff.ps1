# Pictures of both eyes drawn centred compared, pixel by pixel, with pictures of the same still scene drawn off-centre (weapon.txt "offcentre X Y": each eye's window moved X of its
# width outward and Y of its height down). A picture drawn off-centre is the centred one moved by a whole number of pixels, so where the two overlap every pixel should be the same:
# if the game's lighting, shadows, ambient shading, fog or sky go by a centred picture anywhere, it shows here as a difference with a shape. No game needed: it only reads pictures.
#   picdiff.ps1 -Run <folder>                       the pictures of one run: c*.bmp drawn centred, o*.bmp drawn off-centre (as build\guntools\offcentre.ps1 saves them),
#                                                   the offset from the folder's offset.txt ("X Y") unless -X -Y are given
#   picdiff.ps1 -Centred "a.bmp;b.bmp" -Off "c.bmp;d.bmp" -X 0.2 -Y 0.2
#   picdiff.ps1 -SelfTest                           made-up pictures with known faults in them, and what the tool says of each (PASS or FAIL)
#   -X -Y          the offset the off-centre pictures were drawn with, as weapon.txt has it (shares of an eye's picture: outward, down).
#   -Mask          flat things, which sit at the same place of the picture whatever the projection (the HUD's corner panels): rectangles "x0,y0,x1,y1;..." in shares of an eye's
#                  picture, from its top left. Left out where either picture has them. The default covers the two bottom corners.
#   -MaskCentred   rectangles left out by where they are in the centred pictures; -MaskOff by where they are in the off-centre ones (the gun and hands, when they are in the
#                  pictures: the game draws them wrong in an off-centre picture as long as their own projection is not corrected).
#   -GunNote       a line for the report about the gun and hands (the script that took the pictures says what it did with them).
#   -NoVignette    the game's vignette is not looked for. By default it is: the game darkens each picture about its own middle (its last shader: 1 + strength x (distance from the
#                  middle, scaled) to a power), so the same piece of the scene is darkened differently in the two pictures; its numbers are fitted from the pictures and taken out.
#   -Margin        a band this wide (a share of the picture, 0.02 by default) along the overlap's four sides is left out: each side is where one of the two pictures ends, and what
#                  reads its neighbours across a picture's edge (bloom, ambient shading, any blur) cannot match close to it.
#   -Ratio -Slack  a cell of the map counts as differing when in EVERY centred/off-centre pair its mean difference is over Ratio x the largest between two centred pictures there
#                  + Slack. (Two off-centre pictures are not part of that measure: if they differ from each other there by more than that, the cell is marked on its own.)
#   -Before        how many of the centred pictures were taken before the off-centre ones (the rest after them). Default: half of them when there are twice as many as
#                  off-centre ones, as build\guntools\offcentre.ps1 takes them. With it the tool can tell whether the run held still: see below.
#   -Level         an even gain between the two kinds of picture (each eye's and each colour channel's own) is taken out before they are compared. For a run whose verdict
#                  says the off-centre picture is evenly brighter or darker: to see what else differs. Without it the gain is measured and said, not taken out.
#   -Out           where the difference image goes (a .png; beside it <name>_views.png, the pair itself made small; when cells differ <name>_worst.png, the cell furthest over
#                  the controls pixel for pixel; when off-centre pictures differ among themselves <name>_unsteady.png, that piece of each of them side by side). Default: the
#                  screenshots folder, or the run's folder.
# What it prints: the shift used and the shift measured from the pictures themselves; what is left out and how big it is; every pair's difference in the middle of the overlap
# and the pair used (the one that differs least there); the mean and the 99th percentile of the difference in the overlap per colour channel (0 to 255) for that pair and for the
# control pair (two centred pictures: the scene's own noise); a map of the overlap in cells with the mean difference in each; and the verdict line.
# What keeps the verdict from saying "the same" by accident: the shift must be told from the pictures (flat or dark ones tell nothing: no verdict); enough of the overlap must be
# left to compare; the centred pictures taken before and after the off-centre ones must be alike (a scene that moved, or a brightness still settling back, makes two pictures
# drawn alike differ and would hide a real difference: no verdict); the off-centre pictures must be as steady among themselves as the centred ones (else: differs); and what
# only the off-centre picture shows must not be black (the game did not draw it: differs).
# The exit code: 0 the same, 2 it differs (or the picture did not move as the offset says), 1 nothing could be compared or no verdict.
param([string]$Run = '', [string]$Centred = '', [string]$Off = '', [double]$X = [double]::NaN, [double]$Y = [double]::NaN, [string]$Mask = '0,0.90,0.25,1;0.75,0.90,1,1',
      [string]$MaskCentred = '', [string]$MaskOff = '', [string]$GunNote = '', [switch]$NoVignette, [double]$Margin = 0.02, [double]$Ratio = 1.3, [double]$Slack = 0.5, [int]$Grid = 12,
      [int]$Before = -1, [switch]$Level, [string]$Out = '', [switch]$SelfTest)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace PicDiff {

// A picture of both eyes side by side.
public class Pic {
    public int W, H;
    public byte[] Px;      // blue, green, red: 3 bytes a pixel, rows from the top
    public string Name;
    public bool Off;       // drawn off-centre
}

// One eye's half of the comparison, in the pixels of that eye's centred picture.
public class Eye {
    public int Index, W, H;        // 0 the left half, 1 the right; one eye's size
    public int Sx, Sy;             // an off-centre picture has the centred picture's (x, y) at (x + Sx, y + Sy)
    public int X0, Y0, X1, Y1;     // the overlap
    public byte[] Skip;            // 1: left out
    public long Overlap, Compared;
    public int Grid;
    public float[][] Gain;         // the gain from a centred picture to an off-centre one that is taken out by formula (the vignette's, an even one), per channel, at every 8th pixel; null: none
    public int GainW, GainH;
}

public class Stat {
    public Pic A, B;
    public double[] Mean = new double[3];   // blue, green, red
    public int[] P99 = new int[3];
    public long Count;
    public double[] Cell;       // mean difference in each cell of the grid, the channels together; -1: too little of it compared
    public double Middle;       // the same over the middle third of the overlap each way
    public double Area;         // the signed difference averaged over 8 x 8 pixels, its size on average: what is left when pixel noise cancels
    public double Signed;       // mean of B - A
    public string Label { get { return A.Name + "-" + B.Name; } }
}

public class Vignette {
    public double[] A = new double[3];
    public double Rho = 1, E = 1, Before, After;
    public bool Found;
    public double At(int c, double u, double v) {  // the factor at a place of the picture (shares from its top left)
        double dx = u - 0.5, dy = v - 0.5;
        return 1.0 + A[c] * Math.Pow(dx * dx + Rho * Rho * dy * dy, E);
    }
}

public class Options {
    public double X, Y, Ratio = 1.3, Slack = 0.5, Margin = 0.02;
    public string Flat = "", InCentred = "", InOff = "", GunNote = "", DiffPath = "", ViewsPath = "", WorstPath = "", UnsteadyPath = "";
    public int Grid = 12;
    public int Before = -1;        // how many of the centred pictures were taken before the off-centre ones; under 0: half of them when they are twice the off-centre ones, else not known
    public bool NoVignette, Level;
}

public class Result {
    public List<string> Lines = new List<string>();
    public int Verdict = 1;        // 0 the same, 2 differs, 3 the picture did not move as the offset says, 1 nothing compared or no verdict
    public string VerdictText = "";
    public bool VignetteFound;
    public int Flagged;
}

public static class Tool {
    static readonly CultureInfo Inv = CultureInfo.InvariantCulture;
    static string F(string format, params object[] args) { return string.Format(Inv, format, args); }

    public static Pic Load(string path, bool off) {
        using (var bmp = new Bitmap(path)) {
            var p = new Pic();
            p.W = bmp.Width; p.H = bmp.Height; p.Off = off;
            p.Name = System.IO.Path.GetFileNameWithoutExtension(path);
            var data = bmp.LockBits(new Rectangle(0, 0, p.W, p.H), ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
            try {
                p.Px = new byte[p.W * p.H * 3];
                for (int y = 0; y < p.H; ++y)
                    Marshal.Copy(IntPtr.Add(data.Scan0, y * data.Stride), p.Px, y * p.W * 3, p.W * 3);
            } finally { bmp.UnlockBits(data); }
            return p;
        }
    }

    static List<double[]> Rects(string text) {
        var list = new List<double[]>();
        foreach (string part in (text ?? "").Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries)) {
            string[] n = part.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
            if (n.Length != 4) throw new ArgumentException("a rectangle is x0,y0,x1,y1: " + part);
            var r = new double[4];
            for (int i = 0; i < 4; ++i) r[i] = double.Parse(n[i].Trim(), Inv);
            list.Add(r);
        }
        return list;
    }

    // Left out: every pixel (x, y) of the centred picture whose (x + dx, y + dy) lies in one of the rectangles.
    static void Mark(Eye e, List<double[]> rects, int dx, int dy) {
        foreach (double[] r in rects) {
            int x0 = (int)Math.Floor(r[0] * e.W) - dx, x1 = (int)Math.Ceiling(r[2] * e.W) - dx;
            int y0 = (int)Math.Floor(r[1] * e.H) - dy, y1 = (int)Math.Ceiling(r[3] * e.H) - dy;
            for (int y = Math.Max(0, y0); y < Math.Min(e.H, y1); ++y)
                for (int x = Math.Max(0, x0); x < Math.Min(e.W, x1); ++x)
                    e.Skip[y * e.W + x] = 1;
        }
    }

    // Left out: a band this wide along the overlap's four sides. Each side is where one of the two pictures ends.
    static void MarkBand(Eye e, int band) {
        for (int y = e.Y0; y < e.Y1; ++y)
            for (int x = e.X0; x < e.X1; ++x)
                if (x < e.X0 + band || x >= e.X1 - band || y < e.Y0 + band || y >= e.Y1 - band)
                    e.Skip[y * e.W + x] = 1;
    }

    static long CountCompared(Eye e) {
        long n = 0;
        for (int y = e.Y0; y < e.Y1; ++y)
            for (int x = e.X0; x < e.X1; ++x)
                if (e.Skip[y * e.W + x] == 0) ++n;
        return n;
    }

    public static Eye MakeEye(int index, int w, int h, int sx, int sy, int grid) {
        var e = new Eye();
        e.Index = index; e.W = w; e.H = h; e.Sx = sx; e.Sy = sy; e.Grid = grid;
        e.X0 = Math.Max(0, -sx); e.X1 = Math.Min(w, w - sx);
        e.Y0 = Math.Max(0, -sy); e.Y1 = Math.Min(h, h - sy);
        e.Skip = new byte[w * h];
        e.Overlap = (long)Math.Max(0, e.X1 - e.X0) * Math.Max(0, e.Y1 - e.Y0);
        e.Compared = e.Overlap;
        return e;
    }

    // Two pictures' difference over an eye's overlap: b less a (a made into what b should be by the vignette's gain, when a is centred, b off-centre and there is one).
    public static Stat Eval(Pic a, Pic b, Eye e) {
        var s = new Stat();
        s.A = a; s.B = b;
        int g = e.Grid, ow = e.X1 - e.X0, oh = e.Y1 - e.Y0;
        var cellSum = new double[g * g];
        var cellCount = new long[g * g];
        var hist = new long[3][];
        for (int c = 0; c < 3; ++c) hist[c] = new long[256];
        var sum = new double[3];
        double signed = 0, area = 0;
        long count = 0, blocks = 0;
        int ax = a.Off ? e.Sx : 0, ay = a.Off ? e.Sy : 0, bx = b.Off ? e.Sx : 0, by = b.Off ? e.Sy : 0;
        bool gain = e.Gain != null && !a.Off && b.Off;
        int nbx = (ow + 7) / 8;
        var blockSum = new double[3 * nbx];
        var blockCount = new int[nbx];
        float[] rowGain = gain ? new float[3 * e.GainW] : null;
        int eyeX = e.Index * e.W;
        var cellOfX = new int[e.W];
        for (int x = e.X0; x < e.X1; ++x) cellOfX[x] = Math.Min(g - 1, (x - e.X0) * g / ow);
        for (int y = e.Y0; y < e.Y1; ++y) {
            int cy = Math.Min(g - 1, (y - e.Y0) * g / oh);
            if (gain) {
                int gy = y >> 3, gy1 = Math.Min(gy + 1, e.GainH - 1);
                float fy = (y & 7) / 8f;
                for (int c = 0; c < 3; ++c)
                    for (int i = 0; i < e.GainW; ++i)
                        rowGain[c * e.GainW + i] = e.Gain[c][gy * e.GainW + i] * (1 - fy) + e.Gain[c][gy1 * e.GainW + i] * fy;
            }
            int ia = ((y + ay) * a.W + eyeX + e.X0 + ax) * 3, ib = ((y + by) * b.W + eyeX + e.X0 + bx) * 3, sk = y * e.W + e.X0;
            for (int x = e.X0; x < e.X1; ++x, ia += 3, ib += 3, ++sk) {
                if (e.Skip[sk] != 0) continue;
                int block = (x - e.X0) >> 3;
                double three = 0;
                for (int c = 0; c < 3; ++c) {
                    double d;
                    if (gain) {
                        int gx = x >> 3;
                        float gl = rowGain[c * e.GainW + gx], gr = rowGain[c * e.GainW + Math.Min(gx + 1, e.GainW - 1)];
                        // What the off-centre picture should have there, no brighter than a picture can be. A centred pixel at the top of the scale may have been brighter
                        // than it shows: an off-centre one brighter than the gain makes of it is no difference then.
                        double want = Math.Min(255.0, a.Px[ia + c] * (gl + (gr - gl) * ((x & 7) / 8f)));
                        d = b.Px[ib + c] - want;
                        if (a.Px[ia + c] >= 253 && d > 0) d = 0;
                    } else {
                        d = b.Px[ib + c] - a.Px[ia + c];
                    }
                    double ad = Math.Abs(d);
                    sum[c] += ad; signed += d; three += ad;
                    int bin = (int)(ad + 0.5);
                    hist[c][bin > 255 ? 255 : bin]++;
                    blockSum[c * nbx + block] += d;
                }
                blockCount[block]++;
                int cell = cy * g + cellOfX[x];
                cellSum[cell] += three; cellCount[cell]++;
                ++count;
            }
            if (((y - e.Y0) & 7) == 7 || y == e.Y1 - 1) {
                for (int i = 0; i < nbx; ++i) {
                    if (blockCount[i] >= 32) {
                        area += (Math.Abs(blockSum[i]) + Math.Abs(blockSum[nbx + i]) + Math.Abs(blockSum[2 * nbx + i])) / (3.0 * blockCount[i]);
                        ++blocks;
                    }
                    blockSum[i] = blockSum[nbx + i] = blockSum[2 * nbx + i] = 0; blockCount[i] = 0;
                }
            }
        }
        s.Count = count;
        s.Signed = count > 0 ? signed / (3.0 * count) : 0;
        s.Area = blocks > 0 ? area / blocks : 0;
        for (int c = 0; c < 3; ++c) {
            s.Mean[c] = count > 0 ? sum[c] / count : 0;
            long seen = 0, need = (long)Math.Ceiling(0.99 * count);
            for (int v = 0; v < 256; ++v) { seen += hist[c][v]; if (seen >= need) { s.P99[c] = v; break; } }
        }
        s.Cell = new double[g * g];
        double enough = 0.2 * ((double)ow / g) * ((double)oh / g), midSum = 0;
        long midCount = 0;
        for (int i = 0; i < g * g; ++i) {
            s.Cell[i] = cellCount[i] >= enough && cellCount[i] > 0 ? cellSum[i] / (3.0 * cellCount[i]) : -1;
            int cx = i % g, cy = i / g;
            if (cx >= g / 3 && cx < g - g / 3 && cy >= g / 3 && cy < g - g / 3) { midSum += cellSum[i]; midCount += cellCount[i]; }
        }
        s.Middle = midCount > 0 ? midSum / (3.0 * midCount) : (count > 0 ? (sum[0] + sum[1] + sum[2]) / (3.0 * count) : 0);
        return s;
    }

    // ---- The shift, measured from the pictures ----
    // An eye's picture made small: the brightness of every 4 x 4 pixels.
    static float[] Small(Pic p, int eye, int ew, out int w, out int h) {
        w = ew / 4; h = p.H / 4;
        var s = new float[w * h];
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                int sum = 0;
                for (int j = 0; j < 4; ++j) {
                    int i = ((y * 4 + j) * p.W + eye * ew + x * 4) * 3;
                    for (int k = 0; k < 4; ++k, i += 3) sum += p.Px[i] + 2 * p.Px[i + 1] + p.Px[i + 2];
                }
                s[y * w + x] = sum / 64f;
            }
        return s;
    }

    static double SmallCost(float[] c, float[] o, int w, int h, int dx, int dy) {
        int x0 = Math.Max(0, -dx), x1 = Math.Min(w, w - dx), y0 = Math.Max(0, -dy), y1 = Math.Min(h, h - dy);
        if (x1 - x0 < w / 8 || y1 - y0 < h / 8) return double.MaxValue;
        double sum = 0; long n = 0;
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x) { sum += Math.Abs(c[y * w + x] - o[(y + dy) * w + x + dx]); ++n; }
        return sum / n;
    }

    static double FullCost(Pic c, Pic o, int eye, int ew, int dx, int dy) {
        int x0 = Math.Max(0, -dx), x1 = Math.Min(ew, ew - dx), y0 = Math.Max(0, -dy), y1 = Math.Min(c.H, c.H - dy);
        if (x1 - x0 < ew / 8 || y1 - y0 < c.H / 8) return double.MaxValue;
        double sum = 0; long n = 0;
        for (int y = y0 + 2; y < y1; y += 5)
            for (int x = x0 + 2; x < x1; x += 5) {
                int ic = (y * c.W + eye * ew + x) * 3, io = ((y + dy) * o.W + eye * ew + x + dx) * 3;
                sum += Math.Abs(c.Px[ic] + 2 * c.Px[ic + 1] + c.Px[ic + 2] - o.Px[io] - 2 * o.Px[io + 1] - o.Px[io + 2]) / 4.0;
                ++n;
            }
        return n > 0 ? sum / n : double.MaxValue;
    }

    // Where an off-centre picture has the centred one's pixels: tried at the shift the offset says, at none, at its mirror images, along one axis only, at half and at twice, and
    // at the best shift of all those up to half the picture each way (looked for at a sixteenth of the size: a picture that moved by something nobody thought of is found by
    // that), each with the places round it; then to the pixel. best: the shift found; costs: the brightness difference there, at the shift the offset says, and at no shift.
    public static void MeasureShift(Pic c, Pic o, int eye, int ew, int sx, int sy, out int bestX, out int bestY, out double bestCost, out double saidCost, out double noneCost) {
        int w, h;
        float[] cs = Small(c, eye, ew, out w, out h), os = Small(o, eye, ew, out w, out h);
        int w16 = w / 4, h16 = h / 4, anyX = 0, anyY = 0;
        var c16 = new float[w16 * h16]; var o16 = new float[w16 * h16];
        for (int y = 0; y < h16; ++y)
            for (int x = 0; x < w16; ++x) {
                float sumC = 0, sumO = 0;
                for (int j = 0; j < 4; ++j)
                    for (int i = 0; i < 4; ++i) { sumC += cs[(y * 4 + j) * w + x * 4 + i]; sumO += os[(y * 4 + j) * w + x * 4 + i]; }
                c16[y * w16 + x] = sumC / 16; o16[y * w16 + x] = sumO / 16;
            }
        double any = double.MaxValue;
        for (int dy = -h16 / 2; dy <= h16 / 2; ++dy)
            for (int dx = -w16 / 2; dx <= w16 / 2; ++dx) {
                double cost = SmallCost(c16, o16, w16, h16, dx, dy);
                if (cost < any) { any = cost; anyX = dx; anyY = dy; }
            }
        var tries = new List<int[]>();
        foreach (int[] t in new[] { new[] { sx, sy }, new[] { 0, 0 }, new[] { -sx, sy }, new[] { sx, -sy }, new[] { -sx, -sy }, new[] { sx, 0 }, new[] { 0, sy }, new[] { sx / 2, sy / 2 },
                                    new[] { 2 * sx, 2 * sy }, new[] { 16 * anyX, 16 * anyY } })
            for (int j = -3; j <= 3; ++j)
                for (int i = -3; i <= 3; ++i)
                    tries.Add(new[] { (int)Math.Round(t[0] / 4.0) + i, (int)Math.Round(t[1] / 4.0) + j });
        double best = double.MaxValue; int bx = 0, by = 0;
        foreach (int[] t in tries) {
            double cost = SmallCost(cs, os, w, h, t[0], t[1]);
            if (cost < best) { best = cost; bx = t[0]; by = t[1]; }
        }
        bestCost = double.MaxValue; bestX = bx * 4; bestY = by * 4;
        for (int j = -5; j <= 5; ++j)
            for (int i = -5; i <= 5; ++i) {
                double cost = FullCost(c, o, eye, ew, bx * 4 + i, by * 4 + j);
                if (cost < bestCost) { bestCost = cost; bestX = bx * 4 + i; bestY = by * 4 + j; }
            }
        saidCost = FullCost(c, o, eye, ew, sx, sy);
        noneCost = FullCost(c, o, eye, ew, 0, 0);
        if (saidCost <= bestCost * 1.0005) { bestX = sx; bestY = sy; bestCost = saidCost; }  // (a tie goes to what the offset says)
    }

    // ---- The vignette ----
    // Sums of both pictures' values in n x n cells of an eye's overlap, per channel, over the pixels neither too dark nor too bright in either: their ratio is the vignette's gain there.
    static void Ratios(Pic c, Pic o, Eye e, int n, double[] sumC, double[] sumO, long[] count) {
        int ow = e.X1 - e.X0, oh = e.Y1 - e.Y0, eyeX = e.Index * e.W;
        for (int y = e.Y0; y < e.Y1; y += 2) {
            int cy = Math.Min(n - 1, (y - e.Y0) * n / oh);
            for (int x = e.X0; x < e.X1; x += 2) {
                if (e.Skip[y * e.W + x] != 0) continue;
                int cell = cy * n + Math.Min(n - 1, (x - e.X0) * n / ow);
                int ic = (y * c.W + eyeX + x) * 3, io = ((y + e.Sy) * o.W + eyeX + x + e.Sx) * 3;
                for (int k = 0; k < 3; ++k) {
                    int vc = c.Px[ic + k], vo = o.Px[io + k];
                    if (vc < 24 || vc > 232 || vo < 24 || vo > 232) continue;
                    sumC[3 * cell + k] += vc; sumO[3 * cell + k] += vo; count[3 * cell + k]++;
                }
            }
        }
    }

    // The game's vignette fitted to those ratios, both eyes at once: factor = 1 + A (dx^2 + rho^2 dy^2)^e about the picture's middle, A for each channel. Found when it explains
    // most of what the ratios show and is big enough to matter.
    static Vignette FitVignette(Eye[] eyes, double[][] sumC, double[][] sumO, long[][] count, int n) {
        var pts = new List<double[]>();  // channel, u, v of the cell's middle in the centred picture, the shift in shares, log of the ratio, weight
        foreach (Eye e in eyes) {
            int ow = e.X1 - e.X0, oh = e.Y1 - e.Y0;
            for (int cell = 0; cell < n * n; ++cell)
                for (int k = 0; k < 3; ++k) {
                    long cnt = count[e.Index][3 * cell + k];
                    if (cnt < 150 || sumC[e.Index][3 * cell + k] <= 0) continue;
                    double u = (e.X0 + (cell % n + 0.5) * ow / n) / e.W, v = (e.Y0 + (cell / n + 0.5) * oh / n) / e.H;
                    pts.Add(new[] { k, u, v, (double)e.Sx / e.W, (double)e.Sy / e.H, Math.Log(sumO[e.Index][3 * cell + k] / sumC[e.Index][3 * cell + k]), Math.Sqrt(cnt) });
                }
        }
        var best = new Vignette();
        if (pts.Count < 30) return best;
        double weight = 0, before = 0;
        foreach (double[] p in pts) { weight += p[6]; before += p[6] * Math.Abs(p[5]); }
        best.Before = best.After = before / weight;
        // The shape (rho, e) looked for over a grid, then narrowed round the best; A for each channel is fitted at every shape tried.
        double[] rhos = { 0.4, 0.5, 0.5625, 0.65, 0.75, 0.87, 1.0, 1.15, 1.33, 1.54, 1.7778, 2.0, 2.5 };
        double[] es = { 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 4.0, 5.0 };
        var u0 = new double[pts.Count]; var u1 = new double[pts.Count];
        Func<double, double, double[], double> fitAt = (rho, ex, a) => {
            double umax = 0;
            for (int i = 0; i < pts.Count; ++i) {
                double[] p = pts[i];
                double dx = p[1] - 0.5, dy = p[2] - 0.5, ex0 = dx * dx + rho * rho * dy * dy;
                dx += p[3]; dy += p[4];
                double ex1 = dx * dx + rho * rho * dy * dy;
                u0[i] = Math.Pow(ex0, ex); u1[i] = Math.Pow(ex1, ex);
                umax = Math.Max(umax, Math.Max(u0[i], u1[i]));
            }
            if (umax <= 0) return double.MaxValue;
            double total = 0;
            for (int k = 0; k < 3; ++k) {
                // The loss along A for this channel: looked at in 61 places, then narrowed round the best.
                Func<double, double> loss = amp => {
                    double l = 0;
                    for (int i = 0; i < pts.Count; ++i) {
                        if ((int)pts[i][0] != k) continue;
                        double f0 = 1 + amp * u0[i], f1 = 1 + amp * u1[i];
                        if (f0 < 0.05 || f1 < 0.05) return double.MaxValue;
                        l += pts[i][6] * Math.Abs(pts[i][5] - Math.Log(f1 / f0));
                    }
                    return l;
                };
                double lo = -0.94 / umax, hi = 0.94 / umax, bestA = 0, bestL = loss(0);
                for (int i = 0; i <= 60; ++i) {
                    double amp = lo + (hi - lo) * i / 60.0, l = loss(amp);
                    if (l < bestL) { bestL = l; bestA = amp; }
                }
                double step = (hi - lo) / 60.0;
                for (int round = 0; round < 12; ++round) {
                    step *= 0.5;
                    foreach (double amp in new[] { bestA - step, bestA + step }) {
                        double l = loss(amp);
                        if (l < bestL) { bestL = l; bestA = amp; }
                    }
                }
                a[k] = bestA; total += bestL;
            }
            return total / weight;
        };
        foreach (double rho in rhos)
            foreach (double ex in es) {
                var a = new double[3];
                double total = fitAt(rho, ex, a);
                if (total < best.After) { best.After = total; best.Rho = rho; best.E = ex; best.A = a; }
            }
        if (best.After < best.Before) {
            double nudge = 0.12;
            for (int round = 0; round < 7; ++round, nudge *= 0.5)
                foreach (double[] t in new[] { new[] { 1 + nudge, 1.0 }, new[] { 1 / (1 + nudge), 1.0 }, new[] { 1.0, 1 + nudge }, new[] { 1.0, 1 / (1 + nudge) } }) {
                    var a = new double[3];
                    double total = fitAt(best.Rho * t[0], best.E * t[1], a);
                    if (total < best.After) { best.After = total; best.Rho *= t[0]; best.E *= t[1]; best.A = a; }
                }
        }
        // Worth taking out: it explains at least 40% of what the ratios show, and changes some part of the overlap by half a percent or more.
        double most = 0;
        foreach (double[] p in pts)
            most = Math.Max(most, Math.Abs(best.At((int)p[0], p[1] + p[3], p[2] + p[4]) / best.At((int)p[0], p[1], p[2]) - 1));
        best.Found = best.After < 0.6 * best.Before && most >= 0.005;
        return best;
    }

    // The even gain between the two kinds of picture, for each eye and channel: the off-centre pictures' brightness against the centred ones' all over the overlap. The middle one
    // of the areas' ratios (each area by its pixels), with the vignette's part taken out of each first (null: none). A brightness worked out from what the whole picture holds
    // (an exposure that adapts) leaves this: the off-centre picture holds other parts of the scene.
    static double[][] FitLevel(Eye[] eyes, double[][] sumC, double[][] sumO, long[][] count, int n, Vignette vig) {
        var level = new[] { new[] { 1.0, 1.0, 1.0 }, new[] { 1.0, 1.0, 1.0 } };
        foreach (Eye e in eyes) {
            int ow = e.X1 - e.X0, oh = e.Y1 - e.Y0;
            for (int k = 0; k < 3; ++k) {
                var logs = new List<double[]>();  // the log of an area's ratio, its weight
                for (int cell = 0; cell < n * n; ++cell) {
                    long cnt = count[e.Index][3 * cell + k];
                    if (cnt < 150 || sumC[e.Index][3 * cell + k] <= 0 || sumO[e.Index][3 * cell + k] <= 0) continue;
                    double r = Math.Log(sumO[e.Index][3 * cell + k] / sumC[e.Index][3 * cell + k]);
                    if (vig != null && vig.Found) {
                        double u = (e.X0 + (cell % n + 0.5) * ow / n) / e.W, v = (e.Y0 + (cell / n + 0.5) * oh / n) / e.H;
                        double f0 = vig.At(k, u, v), f1 = vig.At(k, u + (double)e.Sx / e.W, v + (double)e.Sy / e.H);
                        if (f0 > 0.05 && f1 > 0.05) r -= Math.Log(f1 / f0);
                    }
                    logs.Add(new[] { r, Math.Sqrt(cnt) });
                }
                if (logs.Count < 12) continue;
                logs.Sort((a, b) => a[0].CompareTo(b[0]));
                double total = 0, run = 0;
                foreach (double[] l in logs) total += l[1];
                foreach (double[] l in logs) {
                    run += l[1];
                    if (run >= 0.5 * total) { level[e.Index][k] = Math.Exp(l[0]); break; }
                }
            }
        }
        return level;
    }

    // What is taken out by formula between a centred picture and an off-centre one, as a gain at every 8th pixel: the vignette's (when one was found) times an even one (null: none).
    static void SetGain(Eye e, Vignette v, double[] level) {
        e.GainW = (e.W >> 3) + 2; e.GainH = (e.H >> 3) + 2;
        e.Gain = new float[3][];
        for (int c = 0; c < 3; ++c) {
            e.Gain[c] = new float[e.GainW * e.GainH];
            double even = level != null ? level[c] : 1.0;
            for (int gy = 0; gy < e.GainH; ++gy)
                for (int gx = 0; gx < e.GainW; ++gx) {
                    double u = gx * 8.0 / e.W, w = gy * 8.0 / e.H;
                    double f0 = v.Found ? v.At(c, u, w) : 1.0, f1 = v.Found ? v.At(c, u + (double)e.Sx / e.W, w + (double)e.Sy / e.H) : 1.0;
                    e.Gain[c][gy * e.GainW + gx] = (float)((f0 > 0.05 && f1 > 0.05 ? f1 / f0 : 1.0) * even);
                }
        }
    }

    // ---- The best pair looked at closely ----
    // The signed difference in brightness over an eye's overlap (b less a, the vignette taken out as in Eval); NaN where left out.
    static float[] Signed(Pic a, Pic b, Eye e) {
        int ow = e.X1 - e.X0, oh = e.Y1 - e.Y0, eyeX = e.Index * e.W;
        var d = new float[ow * oh];
        int ax = a.Off ? e.Sx : 0, ay = a.Off ? e.Sy : 0, bx = b.Off ? e.Sx : 0, by = b.Off ? e.Sy : 0;
        bool gain = e.Gain != null && !a.Off && b.Off;
        for (int y = e.Y0; y < e.Y1; ++y) {
            int ia = ((y + ay) * a.W + eyeX + e.X0 + ax) * 3, ib = ((y + by) * b.W + eyeX + e.X0 + bx) * 3;
            for (int x = e.X0; x < e.X1; ++x, ia += 3, ib += 3) {
                int at = (y - e.Y0) * ow + x - e.X0;
                if (e.Skip[y * e.W + x] != 0) { d[at] = float.NaN; continue; }
                double sum = 0;
                for (int c = 0; c < 3; ++c) {
                    double va = a.Px[ia + c];
                    if (gain) {
                        int gx = x >> 3, gy = y >> 3, gx1 = Math.Min(gx + 1, e.GainW - 1), gy1 = Math.Min(gy + 1, e.GainH - 1);
                        float fx = (x & 7) / 8f, fy = (y & 7) / 8f;
                        float top = e.Gain[c][gy * e.GainW + gx] * (1 - fx) + e.Gain[c][gy * e.GainW + gx1] * fx;
                        float bottom = e.Gain[c][gy1 * e.GainW + gx] * (1 - fx) + e.Gain[c][gy1 * e.GainW + gx1] * fx;
                        va = Math.Min(255.0, va * (top * (1 - fy) + bottom * fy));
                        if (a.Px[ia + c] >= 253 && b.Px[ib + c] > va) va = b.Px[ib + c];  // (as in Eval: no difference where the centred pixel is at the top of the scale)
                    }
                    sum += (b.Px[ib + c] - va) * (c == 1 ? 2 : 1);
                }
                d[at] = (float)(sum / 4.0);
            }
        }
        return d;
    }

    public class Detail {
        public double TileX = 1, TileY = 1;      // the difference's steps across columns (rows) 16 apart in the off-centre picture, against its steps across the others
        public double EdgeShare, EdgePixels;     // the share of the difference that lies at the scene's edges, and the share of pixels that are at such edges
        public double SignedMean, AbsMean;
        public double Share80 = 1;               // the share of the pixels that holds 80% of the difference: small for thin lines and spots
        public double SlopeX, SlopeY, SlopeFit;  // the cells' signed differences as a plane: its change across the whole overlap from side to side and from top to bottom, and how much of the cells' spread the plane explains (0 to 1)
    }

    // What the difference looks like, over the cells given (null: the whole overlap).
    static Detail Describe(float[] d, Pic a, Eye e, bool[] cells) {
        var r = new Detail();
        int ow = e.X1 - e.X0, oh = e.Y1 - e.Y0, g = e.Grid, eyeX = e.Index * e.W;
        double stepXOn = 0, stepXOff = 0, stepYOn = 0, stepYOff = 0, edge = 0, all = 0, signed = 0;
        long nXOn = 0, nXOff = 0, nYOn = 0, nYOff = 0, edgePixels = 0, n = 0;
        var cellSigned = new double[g * g]; var cellN = new long[g * g];
        var sizeCount = new long[1024]; var sizeSum = new double[1024];  // the difference's size in quarters of a level
        int ay = a.Off ? e.Sy : 0, ax = a.Off ? e.Sx : 0;
        for (int y = 2; y < oh - 2; ++y) {
            int cy = Math.Min(g - 1, y * g / oh);
            for (int x = 2; x < ow - 2; ++x) {
                float v = d[y * ow + x];
                if (float.IsNaN(v)) continue;
                int cell = cy * g + Math.Min(g - 1, x * g / ow);
                if (cells != null && !cells[cell]) continue;
                cellSigned[cell] += v; cellN[cell]++;
                float left = d[y * ow + x - 1], up = d[(y - 1) * ow + x];
                if (!float.IsNaN(left)) {
                    if (((x + e.X0 + e.Sx) & 15) == 0) { stepXOn += Math.Abs(v - left); ++nXOn; } else { stepXOff += Math.Abs(v - left); ++nXOff; }
                }
                if (!float.IsNaN(up)) {
                    if (((y + e.Y0 + e.Sy) & 15) == 0) { stepYOn += Math.Abs(v - up); ++nYOn; } else { stepYOff += Math.Abs(v - up); ++nYOff; }
                }
                // At an edge of the scene: the centred picture's brightness changing by a lot within two pixels of this one.
                int i = ((y + e.Y0 + ay) * a.W + eyeX + x + e.X0 + ax) * 3, row = a.W * 3;
                int contrast = Math.Abs(a.Px[i + 6 + 1] - a.Px[i - 6 + 1]) + Math.Abs(a.Px[i + 2 * row + 1] - a.Px[i - 2 * row + 1]);
                double av = Math.Abs(v);
                all += av; signed += v; ++n;
                if (contrast > 32) { edge += av; ++edgePixels; }
                int bin = Math.Min(1023, (int)(av * 4));
                sizeCount[bin]++; sizeSum[bin] += av;
            }
        }
        if (nXOn > 0 && nXOff > 0 && stepXOff > 0) r.TileX = (stepXOn / nXOn) / (stepXOff / nXOff);
        if (nYOn > 0 && nYOff > 0 && stepYOff > 0) r.TileY = (stepYOn / nYOn) / (stepYOff / nYOff);
        if (n > 0) { r.EdgeShare = all > 0 ? edge / all : 0; r.EdgePixels = (double)edgePixels / n; r.SignedMean = signed / n; r.AbsMean = all / n; }
        if (n > 0 && all > 0) {
            double held = 0; long pixels = 0;
            for (int bin = 1023; bin >= 0 && held < 0.8 * all; --bin) { held += sizeSum[bin]; pixels += sizeCount[bin]; }
            r.Share80 = (double)pixels / n;
        }
        // The plane through the cells' signed means (least squares, each cell by its pixels).
        double sw = 0, sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0, sz = 0, sxz = 0, syz = 0;
        for (int i = 0; i < g * g; ++i) {
            if (cellN[i] == 0) continue;
            double w = cellN[i], x = i % g - (g - 1) / 2.0, y = i / g - (g - 1) / 2.0, z = cellSigned[i] / cellN[i];
            sw += w; sx += w * x; sy += w * y; sxx += w * x * x; sxy += w * x * y; syy += w * y * y; sz += w * z; sxz += w * x * z; syz += w * y * z;
        }
        double det = sw * (sxx * syy - sxy * sxy) - sx * (sx * syy - sxy * sy) + sy * (sx * sxy - sxx * sy);
        if (sw > 0 && Math.Abs(det) > 1e-9) {
            double pa = (sz * (sxx * syy - sxy * sxy) - sx * (sxz * syy - sxy * syz) + sy * (sxz * sxy - sxx * syz)) / det;
            double pb = (sw * (sxz * syy - sxy * syz) - sz * (sx * syy - sxy * sy) + sy * (sx * syz - sxz * sy)) / det;
            double pc = (sw * (sxx * syz - sxz * sxy) - sx * (sx * syz - sxz * sy) + sz * (sx * sxy - sxx * sy)) / det;
            double mean = sz / sw, total = 0, left = 0;
            for (int i = 0; i < g * g; ++i) {
                if (cellN[i] == 0) continue;
                double x = i % g - (g - 1) / 2.0, y = i / g - (g - 1) / 2.0, z = cellSigned[i] / cellN[i];
                total += cellN[i] * (z - mean) * (z - mean);
                left += cellN[i] * (z - pa - pb * x - pc * y) * (z - pa - pb * x - pc * y);
            }
            r.SlopeX = pb * (g - 1); r.SlopeY = pc * (g - 1);
            r.SlopeFit = total > 0 ? Math.Max(0, 1 - left / total) : 0;
        }
        return r;
    }

    // ---- Pictures out ----
    static void SavePng(byte[] bgr, int w, int h, string path) {
        using (var bmp = new Bitmap(w, h, PixelFormat.Format24bppRgb)) {
            var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
            for (int y = 0; y < h; ++y) Marshal.Copy(bgr, y * w * 3, IntPtr.Add(data.Scan0, y * data.Stride), w * 3);
            bmp.UnlockBits(data);
            bmp.Save(path, ImageFormat.Png);
        }
    }

    // The difference, half size, into a picture's row: grey, six times as bright as the largest difference of each 2 x 2 pixels; dark blue outside the overlap, dark green left out.
    static void DrawDiff(byte[] img, int imgW, int rowY, float[] d, Eye e) {
        int ow = e.X1 - e.X0, hw = e.W / 2, hh = e.H / 2;
        for (int y = 0; y < hh; ++y)
            for (int x = 0; x < hw; ++x) {
                float most = -1; bool inside = false;
                for (int j = 0; j < 2; ++j)
                    for (int i = 0; i < 2; ++i) {
                        int px = 2 * x + i, py = 2 * y + j;
                        if (px < e.X0 || px >= e.X1 || py < e.Y0 || py >= e.Y1) continue;
                        inside = true;
                        float v = d[(py - e.Y0) * ow + px - e.X0];
                        if (!float.IsNaN(v)) most = Math.Max(most, Math.Abs(v));
                    }
                int at = ((rowY + y) * imgW + e.Index * hw + x) * 3;
                if (!inside) { img[at] = 70; img[at + 1] = 0; img[at + 2] = 0; }
                else if (most < 0) { img[at] = 0; img[at + 1] = 60; img[at + 2] = 0; }
                else { byte v = (byte)Math.Min(255, (int)(most * 6)); img[at] = img[at + 1] = img[at + 2] = v; }
            }
    }

    // A picture of both eyes, a quarter size, into a picture's row.
    static void DrawSmall(byte[] img, int imgW, int rowY, Pic p) {
        int w = p.W / 4, h = p.H / 4;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                int b = 0, g = 0, r = 0;
                for (int j = 0; j < 4; ++j) {
                    int i = ((y * 4 + j) * p.W + x * 4) * 3;
                    for (int k = 0; k < 4; ++k, i += 3) { b += p.Px[i]; g += p.Px[i + 1]; r += p.Px[i + 2]; }
                }
                int at = ((rowY + y) * imgW + x) * 3;
                img[at] = (byte)(b / 16); img[at + 1] = (byte)(g / 16); img[at + 2] = (byte)(r / 16);
            }
    }

    // What only the off-centre picture shows (the part of it outside the centred picture's frame) against the rest of it: how bright each is on average, and how much of each is
    // black. Nothing to compare it with, but a part the game did not draw (things culled by a centred frustum) shows as black or as a jump in brightness.
    static void NewPart(Pic off, Eye e, out long pixels, out double mean, out double black, out double meanRest, out double blackRest) {
        long n = 0, nRest = 0, dark = 0, darkRest = 0;
        double sum = 0, sumRest = 0;
        for (int y = 0; y < e.H; ++y)
            for (int x = 0; x < e.W; ++x) {
                int i = (y * off.W + e.Index * e.W + x) * 3;
                int b = off.Px[i], g = off.Px[i + 1], r = off.Px[i + 2];
                int cx = x - e.Sx, cy = y - e.Sy;
                bool isNew = cx < 0 || cx >= e.W || cy < 0 || cy >= e.H;
                bool isBlack = b <= 4 && g <= 4 && r <= 4;
                if (isNew) { ++n; sum += (r + 2 * g + b) / 4.0; if (isBlack) ++dark; }
                else { ++nRest; sumRest += (r + 2 * g + b) / 4.0; if (isBlack) ++darkRest; }
            }
        pixels = n;
        mean = n > 0 ? sum / n : 0; black = n > 0 ? (double)dark / n : 0;
        meanRest = nRest > 0 ? sumRest / nRest : 0; blackRest = nRest > 0 ? (double)darkRest / nRest : 0;
    }

    static string Percent(double share) { return F("{0:0.0}%", 100.0 * share); }
    static string EyeName(int e) { return e == 0 ? "left eye" : "right eye"; }

    // ---- The whole comparison ----
    static double Median(List<double> values) {
        if (values.Count == 0) return 0;
        var sorted = new List<double>(values);
        sorted.Sort();
        return sorted.Count % 2 == 1 ? sorted[sorted.Count / 2] : 0.5 * (sorted[sorted.Count / 2 - 1] + sorted[sorted.Count / 2]);
    }
    // The mean difference, both eyes and all channels, of the centred/off-centre pair that differs least in the middle of the overlap.
    static double BestMean(Stat[][] pairs, int tests) {
        int least = 0;
        for (int i = 1; i < tests; ++i)
            if (pairs[i][0].Middle + pairs[i][1].Middle < pairs[least][0].Middle + pairs[least][1].Middle) least = i;
        double mean = 0;
        for (int e = 0; e < 2; ++e)
            for (int c = 0; c < 3; ++c) mean += pairs[least][e].Mean[c] / 6;
        return mean;
    }
    // An even gain's three numbers (kept blue, green, red) as they are said: red, green, blue; and as one word.
    static string Times(double[] gain) { return F("x{0:0.000} {1:0.000} {2:0.000}", gain[2], gain[1], gain[0]); }
    static string Evenly(double[] gain) {
        double share = (gain[0] + gain[1] + gain[2]) / 3 - 1;
        return F("{0:0.0}% {1}", 100 * Math.Abs(share), share >= 0 ? "brighter" : "darker");
    }

    public static Result Run(List<Pic> centred, List<Pic> off, Options o) {
        var res = new Result();
        var L = res.Lines;
        if (centred.Count < 1 || off.Count < 1) { L.Add("Nothing to compare: at least one picture drawn centred and one drawn off-centre are needed"); return res; }
        int W = centred[0].W, H = centred[0].H, ew = W / 2;
        foreach (Pic p in centred) if (p.W != W || p.H != H) { L.Add("The pictures are not all one size: " + p.Name); return res; }
        foreach (Pic p in off) if (p.W != W || p.H != H) { L.Add("The pictures are not all one size: " + p.Name); return res; }
        if (W % 2 != 0) { L.Add("The pictures are not two eyes side by side (an odd width)"); return res; }
        double exactX = o.X * ew, exactY = o.Y * H;
        int sx = (int)Math.Round(exactX), sy = -(int)Math.Round(exactY);
        L.Add(F("{0} pictures drawn centred, {1} drawn off-centre (offcentre {2:0.######} {3:0.######}); each {4} x {5}, an eye {6} x {5}", centred.Count, off.Count, o.X, o.Y, W, H, ew));
        L.Add(F("The shift: by the offset an off-centre picture is the centred one moved {0} pixels towards the nose and {1} up ({2} of the width, {3} of the height): the left eye's by ({4:+0;-0;0}, {5:+0;-0;0}), the right eye's by ({6:+0;-0;0}, {5:+0;-0;0}) (across, down)",
                Math.Abs(sx), Math.Abs(sy), Percent(Math.Abs(o.X)), Percent(Math.Abs(o.Y)), sx, sy, -sx));
        if (sx == 0 && sy == 0) {
            res.VerdictText = "no verdict: the offset is no pixels of the picture, so a picture drawn off-centre would be the centred one: nothing to compare";
            L.Add("Verdict: " + res.VerdictText);
            return res;
        }
        if (Math.Abs(exactX - Math.Round(exactX)) > 0.02 || Math.Abs(exactY - Math.Round(exactY)) > 0.02)
            L.Add(F("  NOT a whole number of pixels ({0:0.00} across, {1:0.00} down): the two pictures cannot match pixel for pixel, every edge will differ. Use offsets that are whole pixels of the picture", exactX, exactY));
        else if (sx % 16 != 0 || sy % 16 != 0)
            L.Add("  not a multiple of 16 pixels each way: what the game draws at a half, a quarter or less of the picture's size, and patterns that repeat across the picture (a dither, the ambient shading's noise), then lie differently on the scene in the two pictures, so fine lines and a grain can differ with no fault in the lighting");

        // The shift as the pictures themselves have it, from the last centred one and the first off-centre one. It counts only if the pictures can tell: the brightness at the
        // shift found must be clearly less apart than at every other shift the picture could have moved by (what the offset says, none, its mirror images, one axis only). Pictures with
        // nothing in them (flat, dark) or of too restless a scene tell nothing, and "as the offset says" would be an empty word.
        var said = new[] { sx, -sx };
        var found = new int[2][];
        bool moved = true, asSaid = true, told = true;
        Pic shiftC = centred[centred.Count - 1], shiftO = off[0];
        for (int e = 0; e < 2; ++e) {
            int bx, by; double bestCost, saidCost, noneCost;
            MeasureShift(shiftC, shiftO, e, ew, said[e], sy, out bx, out by, out bestCost, out saidCost, out noneCost);
            found[e] = new[] { bx, by };
            bool same = Math.Abs(bx - said[e]) <= 1 && Math.Abs(by - sy) <= 1;
            asSaid = asSaid && same;
            if (bx == 0 && by == 0) moved = false;
            double other = double.MaxValue;
            foreach (int[] h in new[] { new[] { said[e], sy }, new[] { 0, 0 }, new[] { -said[e], sy }, new[] { said[e], -sy }, new[] { -said[e], -sy }, new[] { said[e], 0 }, new[] { 0, sy } }) {
                if (Math.Abs(h[0] - bx) <= 1 && Math.Abs(h[1] - by) <= 1) continue;
                other = Math.Min(other, FullCost(shiftC, shiftO, e, ew, h[0], h[1]));
            }
            bool sure = bestCost + 0.25 < 0.75 * other;
            told = told && sure;
            L.Add(F("  measured from the pictures, {0}: ({1:+0;-0;0}, {2:+0;-0;0}), the brightness {3:0.00} apart there; {4:0.00} at the shift the offset says, {5:0.00} at no shift: {6}{7}{8}",
                    EyeName(e), bx, by, bestCost, saidCost, noneCost, same ? "as the offset says" : "NOT as the offset says",
                    same && (bx != said[e] || by != sy) ? " to a pixel (the comparison goes by the measured one)" : "",
                    sure ? "" : F("; but that is not told apart from the other shifts it could be ({0:0.00} at the nearest of them)", other)));
        }
        if (!told) {
            res.VerdictText = "no verdict: the pictures cannot tell how far they moved (flat, dark or too restless: no shift is clearly better than the others), so the offset is not proved and nothing is judged";
            L.Add("Verdict: " + res.VerdictText);
            return res;
        }
        if (!asSaid) {
            if (!moved) L.Add("  The off-centre pictures are where the centred ones are: the offset did not reach the game's projection (was it on when they were taken?).");
            else L.Add("  The pictures moved, but not by what the offset says: its meaning (its sign, its unit or the picture's size) is not as read. The comparison below goes by the measured shift.");
        }

        // The eyes: overlap and what is left out.
        var eyes = new Eye[2];
        List<double[]> flat = Rects(o.Flat), inCentred = Rects(o.InCentred), inOff = Rects(o.InOff);
        var flatShare = new double[2]; var gunShare = new double[2]; var bandShare = new double[2];
        int band = (int)Math.Round(Math.Max(0, o.Margin) * Math.Min(ew, H));
        for (int e = 0; e < 2; ++e) {
            eyes[e] = MakeEye(e, ew, H, found[e][0], found[e][1], o.Grid);  // (what the offset says, unless the pictures have it otherwise)
            if (eyes[e].Overlap < 64 * 64) { L.Add("The two pictures hardly overlap: nothing to compare"); return res; }
            Mark(eyes[e], flat, 0, 0); Mark(eyes[e], flat, eyes[e].Sx, eyes[e].Sy);
            long afterFlat = CountCompared(eyes[e]);
            Mark(eyes[e], inCentred, 0, 0); Mark(eyes[e], inOff, eyes[e].Sx, eyes[e].Sy);
            long afterGun = CountCompared(eyes[e]);
            MarkBand(eyes[e], band);
            eyes[e].Compared = CountCompared(eyes[e]);
            flatShare[e] = 1.0 - (double)afterFlat / eyes[e].Overlap;
            gunShare[e] = (double)(afterFlat - afterGun) / eyes[e].Overlap;
            bandShare[e] = (double)(afterGun - eyes[e].Compared) / eyes[e].Overlap;
        }
        // With next to nothing left to compare, nothing would differ: no verdict then.
        for (int e = 0; e < 2; ++e)
            if (eyes[e].Compared < 0.3 * eyes[e].Overlap) {
                res.VerdictText = F("no verdict: too little is left to compare in the {0}: {1} of its overlap, once the rectangles and the band along its sides are left out", EyeName(e),
                                    Percent((double)eyes[e].Compared / eyes[e].Overlap));
                L.Add("Verdict: " + res.VerdictText);
                return res;
            }
        Eye le = eyes[0], re = eyes[1];
        L.Add(F("The overlap: {0} x {1} pixels of each eye's picture ({2} of it); in the centred picture: the left eye's columns {3} to {4}, the right eye's {5} to {6}, rows {7} to {8}",
                le.X1 - le.X0, le.Y1 - le.Y0, Percent((double)le.Overlap / ((double)ew * H)), le.X0, le.X1, re.X0, re.X1, le.Y0, le.Y1));

        // The vignette and the even gain, from every centred/off-centre pair's brightness ratio over the overlap. The two are told apart in turns: the vignette fitted with the
        // even gain out of the ratios, the even gain measured again with the vignette out. The even gain is always measured and said; it is taken out only when asked (-Level).
        Vignette vig = new Vignette();
        var level = new[] { new[] { 1.0, 1.0, 1.0 }, new[] { 1.0, 1.0, 1.0 } };
        bool levelKnown = false;
        if (asSaid) {
            const int n = 24;
            var sumC = new[] { new double[3 * n * n], new double[3 * n * n] };
            var sumO = new[] { new double[3 * n * n], new double[3 * n * n] };
            var cnt = new[] { new long[3 * n * n], new long[3 * n * n] };
            foreach (Pic c in centred)
                foreach (Pic f in off)
                    for (int e = 0; e < 2; ++e) Ratios(c, f, eyes[e], n, sumC[e], sumO[e], cnt[e]);
            level = FitLevel(eyes, sumC, sumO, cnt, n, null);
            levelKnown = true;
            if (!o.NoVignette)
                for (int turn = 0; turn < 2; ++turn) {
                    var evened = new[] { new double[3 * n * n], new double[3 * n * n] };
                    for (int e = 0; e < 2; ++e)
                        for (int i = 0; i < 3 * n * n; ++i) evened[e][i] = sumO[e][i] / level[e][i % 3];
                    vig = FitVignette(eyes, sumC, evened, cnt, n);
                    level = FitLevel(eyes, sumC, sumO, cnt, n, vig);
                }
        }
        res.VignetteFound = vig.Found;
        double levelMost = 0;
        for (int e = 0; e < 2; ++e)
            for (int k = 0; k < 3; ++k) levelMost = Math.Max(levelMost, Math.Abs(level[e][k] - 1));
        bool levelNotable = levelKnown && levelMost >= 0.005;   // half a percent in some channel of some eye
        bool levelOut = o.Level && levelNotable;
        string levelSaid = F("the left eye's off-centre picture {0}, the right eye's {1}", Evenly(level[0]), Evenly(level[1]));

        // Every pair.
        var jobs = new List<Pic[]>();
        foreach (Pic c in centred) foreach (Pic f in off) jobs.Add(new[] { c, f });
        for (int i = 0; i < centred.Count; ++i) for (int j = i + 1; j < centred.Count; ++j) jobs.Add(new[] { centred[i], centred[j] });
        for (int i = 0; i < off.Count; ++i) for (int j = i + 1; j < off.Count; ++j) jobs.Add(new[] { off[i], off[j] });
        int tests = centred.Count * off.Count, centredControls = centred.Count * (centred.Count - 1) / 2;
        var stats = new Stat[jobs.Count][];
        var plain = new Stat[tests][];   // the centred/off-centre pairs with what is taken out by formula (the vignette, an even gain) left in
        bool byFormula = vig.Found || levelOut;
        if (byFormula) {
            Parallel.For(0, tests, i => { plain[i] = new[] { Eval(jobs[i][0], jobs[i][1], eyes[0]), Eval(jobs[i][0], jobs[i][1], eyes[1]) }; });
            for (int e = 0; e < 2; ++e) SetGain(eyes[e], vig, levelOut ? level[e] : null);
        }
        Parallel.For(0, jobs.Count, i => { stats[i] = new[] { Eval(jobs[i][0], jobs[i][1], eyes[0]), Eval(jobs[i][0], jobs[i][1], eyes[1]) }; });
        Func<Stat[], double> middle = s => 0.5 * (s[0].Middle + s[1].Middle);
        Func<Stat[], double> whole = s => (s[0].Mean[0] + s[0].Mean[1] + s[0].Mean[2] + s[1].Mean[0] + s[1].Mean[1] + s[1].Mean[2]) / 6;

        // Did the run hold still? Two pictures drawn alike are the measure of the scene's own noise only if nothing else changed between them. So: the centred pictures taken
        // before the off-centre ones against those taken after (a scene that moved meanwhile, or a brightness that had not settled back after the change, makes those differ, and
        // a control swollen by that would hide a real difference); and the off-centre pictures among themselves (still settling after the change, or something that flickers
        // when drawn off-centre). Each against what two centred pictures of one set differ by.
        int before = o.Before >= 0 ? o.Before : (centred.Count == 2 * off.Count ? off.Count : 0);
        if (before >= centred.Count) before = 0;
        var within = new List<double>(); var across = new List<double>(); var amongOff = new List<double>();
        {
            int at = tests;
            for (int i = 0; i < centred.Count; ++i)
                for (int j = i + 1; j < centred.Count; ++j, ++at)
                    (before > 0 && (i < before) != (j < before) ? across : within).Add(whole(stats[at]));
            for (; at < jobs.Count; ++at) amongOff.Add(whole(stats[at]));
        }
        double steadyW = Median(within), steadyX = Median(across), steadyO = Median(amongOff);
        bool centredSteady = within.Count == 0 || across.Count == 0 || steadyX <= 1.5 * steadyW + 0.3;
        bool offSteady = within.Count == 0 || amongOff.Count == 0 || steadyO <= 1.5 * steadyW + 0.3;

        // What is left out, said.
        L.Add("Left out of the comparison, because it cannot match by nature:");
        L.Add(F("  - what only one of the two pictures shows (outside the overlap): {0} of each eye's picture", Percent(1.0 - (double)le.Overlap / ((double)ew * H))));
        L.Add(F("  - flat things, at the same place of the picture whatever the projection (the HUD's panels; -Mask {0}), where either picture has them: {1} of the left eye's overlap, {2} of the right eye's",
                o.Flat.Length > 0 ? o.Flat : "none", Percent(flatShare[0]), Percent(flatShare[1])));
        if (inCentred.Count + inOff.Count > 0)
            L.Add(F("  - the gun and hands by where they are (in the centred pictures: {0}; in the off-centre ones: {1}): {2} of the left eye's overlap, {3} of the right eye's{4}",
                    o.InCentred.Length > 0 ? o.InCentred : "none", o.InOff.Length > 0 ? o.InOff : "none", Percent(gunShare[0]), Percent(gunShare[1]), o.GunNote.Length > 0 ? ". " + o.GunNote : ""));
        else
            L.Add("  - the gun and hands: no rectangle given for them. " + (o.GunNote.Length > 0 ? o.GunNote : "If they are in the pictures they will show as a difference: the game draws them wrong in an off-centre picture until their own projection is corrected"));
        L.Add(F("  - a band of {0} pixels along the overlap's four sides, each of which is where one of the two pictures ends (what reads its neighbours across a picture's edge cannot match there: bloom, ambient shading, any blur): {1} of the left eye's overlap, {2} of the right eye's",
                band, Percent(bandShare[0]), Percent(bandShare[1])));
        if (o.NoVignette) L.Add("  - the vignette: not looked for (-NoVignette)");
        else if (!asSaid) L.Add("  - the vignette: not looked for (the pictures did not move as the offset says)");
        else if (vig.Found) {
            double corner = 0, side = 0;
            for (int c = 0; c < 3; ++c) { corner += (1 - vig.At(c, 0, 0)) / 3; side += (1 - vig.At(c, 0, 0.5)) / 3; }
            double plainMean = 0, takenMean = 0;
            int bestPlain = 0;
            for (int i = 1; i < tests; ++i) if (middle(plain[i]) < middle(plain[bestPlain])) bestPlain = i;
            for (int e = 0; e < 2; ++e) for (int c = 0; c < 3; ++c) plainMean += plain[bestPlain][e].Mean[c] / 6;
            int bestTaken = 0;
            for (int i = 1; i < tests; ++i) if (middle(stats[i]) < middle(stats[bestTaken])) bestTaken = i;
            for (int e = 0; e < 2; ++e) for (int c = 0; c < 3; ++c) takenMean += stats[bestTaken][e].Mean[c] / 6;
            L.Add(F("  - the vignette: found. Each picture is darkened about its own middle: by {0} in its corners, {1} at the middle of its left and right sides (factor = 1 + A x (dx^2 + {2:0.###}^2 dy^2)^{3:0.##}, A {4:0.###} {5:0.###} {6:0.###} for red green blue; dx, dy from the middle in shares of the picture). The same piece of the scene is darkened differently in the two pictures: taken out by that formula. Left in, the best pair's mean difference is {7:0.00}; taken out, {8:0.00}",
                    Percent(corner), Percent(side), vig.Rho, vig.E, vig.A[2], vig.A[1], vig.A[0], plainMean, takenMean));
            L.Add("    (it must come out with the same numbers at every offset in the same place of the game: numbers that change with the offset are not a vignette but a difference in the lighting)");
        } else {
            L.Add(F("  - the vignette: none found (the two pictures' brightness ratio over the overlap is {0:0.0000} off 1 on average; a vignette's formula explains too little of it to count: {1:0.0000} left)", vig.Before, vig.After));
        }
        if (levelOut)
            L.Add(F("  - an even gain, taken out (-Level): all over the overlap the off-centre pictures are the centred ones {0} in the left eye, {1} in the right (red green blue): what a brightness worked out from the whole picture's content leaves (an exposure that adapts: the off-centre picture holds other parts of the scene){2}",
                    Times(level[0]), Times(level[1]), vig.Found ? "" : F(". Left in, the best pair's mean difference is {0:0.00}; taken out, {1:0.00}", BestMean(plain, tests), BestMean(stats, tests))));
        else if (o.Level && levelKnown)
            L.Add(F("  - an even gain (-Level): none worth taking out (the left eye's {0}, the right eye's {1}: under half a percent)", Times(level[0]), Times(level[1])));
        L.Add(F("  compared: {0} pixels of the left eye's picture ({1} of it), {2} of the right eye's ({3})", le.Compared, Percent((double)le.Compared / ((double)ew * H)), re.Compared,
                Percent((double)re.Compared / ((double)ew * H))));

        // The pairs, and the ones used.
        int best = 0;
        for (int i = 1; i < tests; ++i) if (middle(stats[i]) < middle(stats[best])) best = i;
        // The control: of the pairs of two centred pictures (of two off-centre ones if there are none), the middle one by the same region: the scene's usual noise, not its
        // quietest moment (the quietest of many pairs against the best pair across the change of projection would call a restless scene a difference).
        int control = -1;
        {
            var pool = new List<int>();
            for (int i = tests; i < tests + centredControls; ++i) pool.Add(i);
            if (pool.Count == 0) for (int i = tests + centredControls; i < jobs.Count; ++i) pool.Add(i);
            pool.Sort((a, b) => middle(stats[a]).CompareTo(middle(stats[b])));
            if (pool.Count > 0) control = pool[(pool.Count - 1) / 2];
        }
        var line = new System.Text.StringBuilder("Every pair's mean difference in the middle of the overlap (both eyes; the region the pair is chosen by). Centred against off-centre:");
        for (int i = 0; i < tests; ++i) line.Append(F("  {0} {1:0.00}", stats[i][0].Label, middle(stats[i])));
        L.Add(line.ToString());
        if (control >= 0) {
            line = new System.Text.StringBuilder("  controls, two pictures drawn alike (the scene's own noise):");
            for (int i = tests; i < jobs.Count; ++i) line.Append(F("  {0} {1:0.00}", stats[i][0].Label, middle(stats[i])));
            L.Add(line.ToString());
            L.Add(F("  used: {0} (the centred/off-centre pair that differs least there) and the control {1} (the middle one of the pairs of two pictures drawn centred: the scene's usual noise)", stats[best][0].Label,
                    stats[control][0].Label));
        } else {
            L.Add(F("  used: {0}. No control pair (two pictures drawn alike): the scene's own noise is not known", stats[best][0].Label));
        }
        if (within.Count > 0) {
            var held = new List<string>();
            if (before > 0) {
                held.Add(F("two centred pictures of one set (the {0} taken before the off-centre ones, or the {1} after) differ by {2:0.00} on average", before, centred.Count - before, steadyW));
                if (across.Count > 0) held.Add(F("one of the set before against one of the set after by {0:0.00}", steadyX));
            } else {
                held.Add(F("two centred pictures differ by {0:0.00} on average (which were taken before the off-centre ones and which after is not known here: -Before)", steadyW));
            }
            if (amongOff.Count > 0) held.Add(F("two off-centre pictures by {0:0.00}", steadyO));
            L.Add("Did the run hold still? Over the whole overlap, 0 to 255: " + string.Join("; ", held.ToArray()) + ". " +
                  (centredSteady && offSteady ? "It did, as far as that shows (each within 1.5 x the first + 0.3)." : "") +
                  (centredSteady ? "" : "NOT the centred pictures: those taken after the off-centre ones differ from those taken before. The scene or the picture's brightness moved meanwhile, or had not settled back. ") +
                  (offSteady ? "" : "NOT the off-centre pictures among themselves: still settling after the change, or something flickers when drawn off-centre."));
        } else {
            L.Add("Did the run hold still? Not known: it takes two centred pictures of one set (taken one after the other) to tell.");
        }

        // The numbers.
        L.Add("The difference in the overlap, 0 to 255: mean / 99th percentile for red, green, blue; then the mean of 8 x 8 pixel areas (pixel noise cancelled) and the mean signed difference (above 0: the off-centre picture is brighter)");
        for (int e = 0; e < 2; ++e) {
            Stat t = stats[best][e];
            L.Add(F("  {0}, {1,-9}: {2:0.00}/{3}  {4:0.00}/{5}  {6:0.00}/{7}   areas {8:0.00}   signed {9:+0.00;-0.00;0.00}", EyeName(e), t.Label, t.Mean[2], t.P99[2], t.Mean[1], t.P99[1], t.Mean[0],
                    t.P99[0], t.Area, t.Signed));
            if (control >= 0) {
                Stat c = stats[control][e];
                L.Add(F("  {0}, {1,-9}: {2:0.00}/{3}  {4:0.00}/{5}  {6:0.00}/{7}   areas {8:0.00}   signed {9:+0.00;-0.00;0.00}   (the control)", EyeName(e), c.Label, c.Mean[2], c.P99[2], c.Mean[1],
                        c.P99[1], c.Mean[0], c.P99[0], c.Area, c.Signed));
            }
        }
        if (levelKnown)
            L.Add(F("The off-centre pictures' brightness against the centred ones' all over the overlap (an even gain: the middle one of the areas' ratios, every pair, the vignette out): the left eye's {0}, the right eye's {1} (red green blue){2}",
                    Times(level[0]), Times(level[1]),
                    levelOut ? ": taken out (-Level)" : levelNotable ? ": NOT 1. A brightness worked out from what the whole picture holds would do this (an exposure that adapts: the off-centre picture holds other parts of the scene). Left in: -Level takes it out" : ": 1 within half a percent"));

        // What only the off-centre picture shows: not compared with anything, but looked at. All black where the rest of the picture is not, the game did not draw it: no
        // scene's own darkness does that. A good part of it black is said with the verdict, to be looked at.
        bool notDrawn = false;
        string newSaid = "";
        if (asSaid) {
            L.Add("What only the off-centre picture shows (its outer and lower parts, new in the picture: nothing to compare them with). The game must have drawn it: black there, or a brightness far from the rest's, means something is still culled or lit by a centred picture. Look at it in the small picture of the pair.");
            for (int e = 0; e < 2; ++e) {
                long pixels; double mean, black, meanRest, blackRest;
                NewPart(jobs[best][1], eyes[e], out pixels, out mean, out black, out meanRest, out blackRest);
                L.Add(F("  {0}, {1}: {2} of its picture is new; brightness {3:0.0} there against {4:0.0} in the rest; black {5} there against {6} in the rest", EyeName(e), jobs[best][1].Name,
                        Percent((double)pixels / ((double)ew * H)), mean, meanRest, Percent(black), Percent(blackRest)));
                if (pixels > 0 && black >= 0.9 && blackRest < 0.5) notDrawn = true;
                if (pixels > 0 && black >= 0.25 && black >= 3 * blackRest + 0.1)
                    newSaid += F("{0}{1} of what only the {2}'s off-centre picture shows is black ({3} of the rest of that picture)", newSaid.Length > 0 ? ", " : "", Percent(black), EyeName(e), Percent(blackRest));
            }
        }

        // The map: a cell differs when every centred/off-centre pair is over every pair of two centred pictures there (of two off-centre ones only when there are no such: what
        // is unsteady only when drawn off-centre must not pass for the scene's own noise). And on its own: a cell where two off-centre pictures differ from each other by far
        // more than two centred ones ever do (over twice that + 1: with only a few pictures of each kind a restless scene would be marked by chance at anything less).
        int g = o.Grid;
        var flagged = new bool[2][];
        var unsteady = new bool[2][];
        var flaggedIn = new int[2];
        int flaggedCount = 0, unsteadyCount = 0, validCells = 0, worstEye = -1, worstCell = -1, unsteadyEye = -1, unsteadyCell = -1;
        double worstOver = 0, unsteadyOver = 0;
        int controlsEnd = centredControls > 0 ? tests + centredControls : jobs.Count;
        for (int e = 0; e < 2; ++e) {
            flagged[e] = new bool[g * g];
            unsteady[e] = new bool[g * g];
            for (int i = 0; i < g * g; ++i) {
                double least = double.MaxValue, most = -1, mostOff = -1;
                bool valid = true;
                for (int k = 0; k < tests; ++k) { if (stats[k][e].Cell[i] < 0) valid = false; else least = Math.Min(least, stats[k][e].Cell[i]); }
                for (int k = tests; k < controlsEnd; ++k) { if (stats[k][e].Cell[i] < 0) valid = false; else most = Math.Max(most, stats[k][e].Cell[i]); }
                for (int k = controlsEnd; k < jobs.Count; ++k) { if (stats[k][e].Cell[i] < 0) valid = false; else mostOff = Math.Max(mostOff, stats[k][e].Cell[i]); }
                if (!valid) continue;
                ++validCells;
                if (control >= 0 && least > most * o.Ratio + o.Slack) {
                    flagged[e][i] = true; ++flaggedCount; ++flaggedIn[e];
                    if (least - most > worstOver) { worstOver = least - most; worstEye = e; worstCell = i; }
                }
                if (control >= 0 && mostOff > most * 2.0 + 1.0) {
                    unsteady[e][i] = true; ++unsteadyCount;
                    if (mostOff - most > unsteadyOver) { unsteadyOver = mostOff - most; unsteadyEye = e; unsteadyCell = i; }
                }
            }
        }
        res.Flagged = flaggedCount;
        L.Add(F("Where: the overlap in {0} x {0} cells, the mean difference in each (the pair used; beside it the control). '*': in every centred/off-centre pair over {1} x the largest between two centred pictures there + {2}. '~': two off-centre pictures differ from each other there by more than twice that largest + 1. '--': left out", g, o.Ratio, o.Slack));
        for (int e = 0; e < 2; ++e) {
            L.Add(F("  {0} (its columns run from the picture's left; the nose is to the {1}):", EyeName(e), e == 0 ? "right" : "left"));
            for (int row = 0; row < g; ++row) {
                var text = new System.Text.StringBuilder("   ");
                for (int col = 0; col < g; ++col) {
                    double v = stats[best][e].Cell[row * g + col];
                    text.Append(v < 0 ? "    -- " : F("{0,5:0.0}{1} ", Math.Min(v, 999.9), flagged[e][row * g + col] ? "*" : unsteady[e][row * g + col] ? "~" : " "));
                }
                if (control >= 0) {
                    text.Append("  |");
                    for (int col = 0; col < g; ++col) {
                        double v = stats[control][e].Cell[row * g + col];
                        text.Append(v < 0 ? "    --" : F("{0,6:0.0}", Math.Min(v, 999.9)));
                    }
                }
                L.Add(text.ToString());
            }
        }

        // The best pair's difference itself: what it looks like, and the pictures.
        var d = new float[2][]; var dc = new float[2][];
        var detail = new Detail[2];
        for (int e = 0; e < 2; ++e) {
            d[e] = Signed(jobs[best][0], jobs[best][1], eyes[e]);
            if (control >= 0) dc[e] = Signed(jobs[control][0], jobs[control][1], eyes[e]);
            detail[e] = Describe(d[e], jobs[best][0], eyes[e], flaggedCount > 0 ? flagged[e] : null);
        }
        if (o.DiffPath.Length > 0) {
            int rows = control >= 0 ? 2 : 1;
            var img = new byte[ew * (H / 2) * rows * 3];
            for (int e = 0; e < 2; ++e) {
                DrawDiff(img, ew, 0, d[e], eyes[e]);
                if (control >= 0) DrawDiff(img, ew, H / 2, dc[e], eyes[e]);
            }
            SavePng(img, ew, (H / 2) * rows, o.DiffPath);
        }
        if (o.ViewsPath.Length > 0) {
            var img = new byte[(W / 4) * (H / 4) * 2 * 3];
            DrawSmall(img, W / 4, 0, jobs[best][0]);
            DrawSmall(img, W / 4, H / 4, jobs[best][1]);
            SavePng(img, W / 4, (H / 4) * 2, o.ViewsPath);
        }
        if (o.WorstPath.Length > 0 && worstCell >= 0) {
            // The cell furthest over the controls, pixel for pixel and twice as big: the centred picture, the off-centre one moved back onto it, six times their difference.
            Eye we = eyes[worstEye];
            int half = Math.Min(128, Math.Min(we.X1 - we.X0, we.Y1 - we.Y0) / 2);
            int cx = we.X0 + (int)((worstCell % g + 0.5) * (we.X1 - we.X0) / g), cy = we.Y0 + (int)((worstCell / g + 0.5) * (we.Y1 - we.Y0) / g);
            int x0 = Math.Max(we.X0, Math.Min(we.X1 - 2 * half, cx - half)), y0 = Math.Max(we.Y0, Math.Min(we.Y1 - 2 * half, cy - half));
            int side = 2 * half, ow = we.X1 - we.X0, eyeX = worstEye * ew;
            var img = new byte[(6 * side) * (2 * side) * 3];
            Pic pa = jobs[best][0], pb = jobs[best][1];
            for (int y = 0; y < 2 * side; ++y)
                for (int x = 0; x < 2 * side; ++x) {
                    int px = x0 + x / 2, py = y0 + y / 2;
                    int ia = (py * W + eyeX + px) * 3, ib = ((py + we.Sy) * W + eyeX + px + we.Sx) * 3, at = (y * 6 * side + x) * 3;
                    float v = d[worstEye][(py - we.Y0) * ow + px - we.X0];
                    byte grey = float.IsNaN(v) ? (byte)0 : (byte)Math.Min(255, (int)(Math.Abs(v) * 6));
                    for (int c = 0; c < 3; ++c) {
                        img[at + c] = pa.Px[ia + c];
                        img[at + 2 * side * 3 + c] = pb.Px[ib + c];
                        img[at + 4 * side * 3 + c] = grey;
                    }
                }
            SavePng(img, 6 * side, 2 * side, o.WorstPath);
            L.Add(F("The cell furthest over the controls: the {0}'s columns {1} to {2}, rows {3} to {4} of its centred picture, pixel for pixel in the third picture below", EyeName(worstEye), x0,
                    x0 + side, y0, y0 + side));
        }
        if (o.UnsteadyPath.Length > 0 && unsteadyCell >= 0) {
            // The cell where the off-centre pictures differ most from each other: the same piece of the scene out of each of them, side by side, pixel for pixel.
            Eye ue = eyes[unsteadyEye];
            int half = Math.Min(128, Math.Min(ue.X1 - ue.X0, ue.Y1 - ue.Y0) / 2), side = 2 * half;
            int cx = ue.X0 + (int)((unsteadyCell % g + 0.5) * (ue.X1 - ue.X0) / g), cy = ue.Y0 + (int)((unsteadyCell / g + 0.5) * (ue.Y1 - ue.Y0) / g);
            int x0 = Math.Max(ue.X0, Math.Min(ue.X1 - side, cx - half)), y0 = Math.Max(ue.Y0, Math.Min(ue.Y1 - side, cy - half));
            var img = new byte[side * off.Count * side * 3];
            for (int p = 0; p < off.Count; ++p)
                for (int y = 0; y < side; ++y)
                    for (int x = 0; x < side; ++x) {
                        int src = ((y0 + y + ue.Sy) * W + unsteadyEye * ew + x0 + x + ue.Sx) * 3, dst = (y * side * off.Count + p * side + x) * 3;
                        img[dst] = off[p].Px[src]; img[dst + 1] = off[p].Px[src + 1]; img[dst + 2] = off[p].Px[src + 2];
                    }
            SavePng(img, side * off.Count, side, o.UnsteadyPath);
            L.Add(F("The cell where two off-centre pictures differ most from each other: the {0}'s columns {1} to {2}, rows {3} to {4} of its centred picture, out of each off-centre picture side by side in the fourth picture below",
                    EyeName(unsteadyEye), x0, x0 + side, y0, y0 + side));
        }

        // The verdict.
        double meanT = 0, meanC = 0, p99T = 0, p99C = 0, areaT = 0, areaC = 0, signedT = 0;
        for (int e = 0; e < 2; ++e) {
            for (int c = 0; c < 3; ++c) {
                meanT += stats[best][e].Mean[c] / 6; p99T = Math.Max(p99T, stats[best][e].P99[c]);
                if (control >= 0) { meanC += stats[control][e].Mean[c] / 6; p99C = Math.Max(p99C, stats[control][e].P99[c]); }
            }
            areaT += stats[best][e].Area / 2; signedT += stats[best][e].Signed / 2;
            if (control >= 0) areaC += stats[control][e].Area / 2;
        }
        string verdict;
        if (!asSaid) {
            res.Verdict = 3;
            verdict = moved ? F("the picture moved by ({0:+0;-0;0}, {1:+0;-0;0}) in the left eye and ({2:+0;-0;0}, {3:+0;-0;0}) in the right, not by what the offset says ({4:+0;-0;0}, {5:+0;-0;0}) and ({6:+0;-0;0}, {5:+0;-0;0}): the offset's meaning is not as read; the lighting is not judged",
                                found[0][0], found[0][1], found[1][0], found[1][1], sx, sy, -sx)
                            : "the off-centre pictures did not move: the offset did not reach the game's projection; nothing is judged";
        } else if (control < 0) {
            res.Verdict = 1;
            verdict = F("no control pair, so no verdict: the overlap differs by {0:0.00} on average (99th percentile {1:0})", meanT, p99T);
        } else {
            bool meanOk = meanT <= meanC * 1.2 + 0.2, p99Ok = p99T <= p99C * 1.25 + 3, areaOk = areaT <= areaC * 1.25 + 0.1;
            bool wholeOk = meanOk && p99Ok && areaOk;
            bool alike = flaggedCount == 0 && unsteadyCount == 0 && wholeOk;
            string numbers = F("mean {0:0.00} against the control's {1:0.00}, 99th percentile {2:0} against {3:0}, 8 x 8 areas {4:0.00} against {5:0.00}", meanT, meanC, p99T, p99C, areaT, areaC);
            if (alike && !offSteady) {
                // Within the controls, but the off-centre pictures themselves were moving: the pair used may only be the moment they passed the centred ones.
                res.Verdict = 2;
                verdict = F("lighting or shadows DIFFER: the off-centre pictures are not steady among themselves (two of them differ by {0:0.00} on average, two centred ones of one set by {1:0.00}): the picture was still settling after the change (a brightness that adapts: wait longer before the pictures are taken) or something flickers when drawn off-centre. Against the centred pictures the pair used is within the controls ({2}), which proves nothing while they move",
                            steadyO, steadyW, numbers);
            } else if (alike && !centredSteady) {
                // Within the controls, but the controls are swollen by whatever changed between the first centred pictures and the last.
                res.Verdict = 1;
                verdict = F("no verdict: the run did not hold still: a centred picture taken before the off-centre ones differs from one taken after by {0:0.00} on average, two of one set by {1:0.00}. The scene or the picture's brightness moved meanwhile, or had not settled back, so two pictures drawn alike are no measure of the scene's own noise here. The pair used is within them ({2}), which proves nothing. Run it again in a stiller place or with longer waits",
                            steadyX, steadyW, numbers);
            } else if (alike && before > 0 && (within.Count == 0 || amongOff.Count == 0)) {
                // Within the controls, but with one picture of a kind nothing says whether the run held still: the same as not knowing the scene's own noise.
                res.Verdict = 1;
                verdict = F("no verdict: the pair used is within the controls ({0}), but with only one picture of a set nothing says whether the run held still, and pictures drawn alike that are swollen by a scene that moved would hide a real difference. Take two or more of each kind", numbers);
            } else if (alike && levelOut) {
                res.Verdict = 2;
                verdict = F("lighting or shadows DIFFER, but only by an even gain, taken out here (-Level): {0}. Apart from it the overlap differs no more than two pictures drawn alike do ({1}; no cell of the map over the controls). A brightness worked out from what the whole picture holds would do this (an exposure that adapts: the off-centre picture holds other parts of the scene); each eye gets its own if the two numbers differ",
                            levelSaid, numbers);
            } else if (alike) {
                res.Verdict = 0;
                verdict = F("lighting and shadows the same: the overlap differs no more than two pictures drawn alike do ({0}; no cell of the map over the controls)", numbers);
                if (before == 0)
                    verdict += ". Which centred pictures were taken before the off-centre ones and which after is not known here (-Before), so whether the run held still across the change was not looked at";
                if (levelNotable)
                    verdict += F(". An even gain is there all the same, small enough to pass: {0}", levelSaid);
                if (meanC >= 1.5)
                    verdict += F(". But the scene is restless (two pictures drawn alike differ by {0:0.00} on average, 99th percentile {1:0}): a difference smaller than that is not ruled out here", meanC, p99C);
            } else {
                res.Verdict = 2;
                var kinds = new List<string>();
                double tile = Math.Max(Math.Max(detail[0].TileX, detail[0].TileY), Math.Max(detail[1].TileX, detail[1].TileY));
                double edgeShare = 0.5 * (detail[0].EdgeShare + detail[1].EdgeShare), edgePixels = 0.5 * (detail[0].EdgePixels + detail[1].EdgePixels);
                double signedF = 0.5 * (detail[0].SignedMean + detail[1].SignedMean), absF = 0.5 * (detail[0].AbsMean + detail[1].AbsMean);
                if (flaggedCount > 0) {
                    // Where the flagged cells are.
                    string where;
                    bool atSide = false;
                    if (flaggedCount >= 0.8 * validCells) where = "the whole overlap";
                    else {
                        var parts = new List<string>();
                        for (int e = 0; e < 2; ++e) {
                            int n = 0, top = 0, bottom = 0, left = 0, right = 0, minC = g, maxC = -1, minR = g, maxR = -1;
                            for (int i = 0; i < g * g; ++i) {
                                if (!flagged[e][i]) continue;
                                int col = i % g, row = i / g;
                                ++n;
                                if (row < 2) ++top; if (row >= g - 2) ++bottom; if (col < 2) ++left; if (col >= g - 2) ++right;
                                minC = Math.Min(minC, col); maxC = Math.Max(maxC, col); minR = Math.Min(minR, row); maxR = Math.Max(maxR, row);
                            }
                            if (n == 0) continue;
                            string place = "";
                            if (top >= 0.7 * n) place = ", along its top";
                            else if (bottom >= 0.7 * n) place = ", along its bottom";
                            else if (left >= 0.7 * n) place = e == 0 ? ", along its outer side" : ", along its nose side";
                            else if (right >= 0.7 * n) place = e == 0 ? ", along its nose side" : ", along its outer side";
                            atSide = atSide || place.Length > 0;
                            Eye ey = eyes[e];
                            int px0 = ey.X0 + minC * (ey.X1 - ey.X0) / g, px1 = ey.X0 + (maxC + 1) * (ey.X1 - ey.X0) / g, py0 = ey.Y0 + minR * (ey.Y1 - ey.Y0) / g, py1 = ey.Y0 + (maxR + 1) * (ey.Y1 - ey.Y0) / g;
                            parts.Add(F("{0} cells of the {1}'s overlap{2} (within columns {3} to {4}, rows {5} to {6} of its centred picture: {7:0.###},{8:0.###},{9:0.###},{10:0.###} as shares)", n, EyeName(e),
                                        place, px0, px1, py0, py1, (double)px0 / ew, (double)py0 / H, (double)px1 / ew, (double)py1 / H));
                        }
                        where = string.Join(" and ", parts.ToArray());
                    }
                    kinds.Add("in " + where);
                    if (atSide) kinds.Add("that is along a side of the overlap, where one of the two pictures ends: what reads its neighbours across a picture's edge (bloom, ambient shading, a blur) cannot match close to it, so look at the image before blaming the lighting");
                } else if (!wholeOk) {
                    kinds.Add(F("all over, a little: no single cell is over the controls, but the whole is (mean {0:0.00} against the control's {1:0.00}, 99th percentile {2:0} against {3:0}, 8 x 8 areas {4:0.00} against {5:0.00})",
                                meanT, meanC, p99T, p99C, areaT, areaC));
                }
                // What the difference looks like (said only when the centred and the off-centre pictures differ: not for off-centre pictures that only differ among themselves).
                bool described = flaggedCount > 0 || !wholeOk;
                if (described && tile >= 1.3) kinds.Add(F("with steps on a 16-pixel grid of the off-centre picture ({0:0.00} times the steps elsewhere): squares, as a list of lights kept per tile of the picture would leave", tile));
                double share80 = 0.5 * (detail[0].Share80 + detail[1].Share80);
                // The slope across the overlap, when a plane explains the cells: the stronger eye's.
                Detail sloped = detail[0].SlopeFit * (Math.Abs(detail[0].SlopeX) + Math.Abs(detail[0].SlopeY)) >= detail[1].SlopeFit * (Math.Abs(detail[1].SlopeX) + Math.Abs(detail[1].SlopeY)) ? detail[0] : detail[1];
                int slopedEye = sloped == detail[0] ? 0 : 1;
                double slopeSize = Math.Abs(sloped.SlopeX) + Math.Abs(sloped.SlopeY);
                // Evenly brighter or darker, each eye for itself (an exposure worked out per picture can send the two eyes different ways, and their mean would hide both).
                bool evenL = detail[0].AbsMean > 0 && Math.Abs(detail[0].SignedMean) >= 0.6 * detail[0].AbsMean;
                bool evenR = detail[1].AbsMean > 0 && Math.Abs(detail[1].SignedMean) >= 0.6 * detail[1].AbsMean;
                Func<int, string> evenSaid = eye => F("the {0}'s off-centre picture evenly {1} there, by {2:0.0} of 255", EyeName(eye), detail[eye].SignedMean > 0 ? "brighter" : "darker", Math.Abs(detail[eye].SignedMean));
                if (!described) { }
                else if (share80 <= 0.15)
                    kinds.Add(F("thin lines or small spots, not areas: 80% of the difference sits in {0} of the pixels there, {1} of it at the scene's edges (a shadow's or a thing's outline a pixel or two off would look so)", Percent(share80), Percent(edgeShare)));
                else if (flaggedIn[slopedEye] >= 8 && sloped.SlopeFit >= 0.6 && slopeSize >= 1.0 && slopeSize >= 2.0 * Math.Abs(sloped.SignedMean))
                    kinds.Add(F("a smooth slope across the overlap: in the {0} the off-centre picture gets {1} by {2:0.0} of 255 from its left to its right and {3} by {4:0.0} from its top to its bottom (a plane explains {5} of the cells' spread): what goes by the distance from the picture's middle would leave this (a vignette not taken out, a fog, sky or shading worked out from the middle)",
                                EyeName(slopedEye), sloped.SlopeX >= 0 ? "brighter" : "darker", Math.Abs(sloped.SlopeX), sloped.SlopeY >= 0 ? "brighter" : "darker", Math.Abs(sloped.SlopeY), Percent(sloped.SlopeFit)));
                else if (evenL && evenR && (detail[0].SignedMean > 0) == (detail[1].SignedMean > 0))
                    kinds.Add(F("the off-centre picture evenly {0} there, by {1:0.0} of 255", signedF > 0 ? "brighter" : "darker", Math.Abs(signedF)));
                else if (evenL && evenR)
                    kinds.Add(evenSaid(0) + ", " + evenSaid(1) + ": each eye its own way");
                else if (evenL || evenR)
                    kinds.Add(evenSaid(evenL ? 0 : 1) + (detail[evenL ? 1 : 0].AbsMean > 0 ? F(" (the {0}'s not evenly)", EyeName(evenL ? 1 : 0)) : ""));
                else if (meanT > 0 && areaT <= 0.2 * meanT)
                    kinds.Add(F("fine-grained: over areas of 8 x 8 pixels it averages out as pixel noise does ({0:0.00} of the pixels' difference; the control's {1:0.00}), so no area is lit differently; a grain or dither that sits still in the picture would do this",
                                areaT / meanT, meanC > 0 ? areaC / meanC : 0));
                else
                    kinds.Add(F("in patches, brighter and darker mixed (signed {0:+0.0;-0.0;0.0} against a size of {1:0.0}; {2} of it at the scene's edges)", signedF, absF, Percent(edgeShare)));
                if (unsteadyCount > 0)
                    kinds.Add(F("two off-centre pictures differ from each other in {0} cells of the map ('~') by far more than two centred ones ever do there: something flickers or pops when drawn off-centre, or a thing moved through the scene while they were taken (look at them side by side: the fourth picture)", unsteadyCount));
                if (!offSteady)
                    kinds.Add(F("the off-centre pictures are not steady among themselves (two of them differ by {0:0.00} on average, two centred ones of one set by {1:0.00}): still settling after the change, or something flickers", steadyO, steadyW));
                if (!centredSteady)
                    kinds.Add(F("the centred pictures taken before and after the off-centre ones differ too ({0:0.00} on average, {1:0.00} within a set): part of this may be the scene itself moving", steadyX, steadyW));
                if (levelOut)
                    kinds.Add(F("this with an even gain taken out first (-Level): {0}", levelSaid));
                else if (levelNotable)
                    kinds.Add(F("all over the overlap {0} (an even gain): a brightness worked out from what the whole picture holds would do this (an exposure that adapts: the off-centre picture holds other parts of the scene), each eye its own if the two numbers differ. Judge again with -Level to take it out and see what else differs", levelSaid));
                verdict = "lighting or shadows DIFFER: " + string.Join("; ", kinds.ToArray());
            }
        }
        if (notDrawn && asSaid) {
            // Whatever the overlap says: a picture whose new part is black is not the picture an off-centre window should give.
            res.Verdict = 2;
            verdict = "the game did not draw the part of the picture its window moved onto: " + newSaid + ". What lies outside a centred picture is still left out somewhere (culled, or cut off by a centred frame). In the overlap: " + verdict;
        } else if (newSaid.Length > 0) {
            verdict += ". Look at the new part of the off-centre picture (the small picture of the pair): " + newSaid;
        }
        res.VerdictText = verdict;
        L.Add("Verdict: " + verdict);
        return res;
    }

    // ---- Made-up pictures, to try the tool on (-SelfTest) ----
    public class Fake {
        public int Sx, Sy;              // the shift an off-centre picture is really drawn with (the left eye's; the right eye's across is the other way)
        public double Noise = 1.0;      // each picture's own noise, levels each way
        public double VignetteA;        // the game's vignette: 1 + A (dx^2 + dy^2)^1.5
        public bool Tiles, Brighter, EdgeMoved, Hud, Gun, Grain;
        public bool Flat;               // nothing in the scene: one grey
        public bool Flicker;            // a patch of the scene at a brightness of its own in each off-centre picture
        public bool NewBlack;           // what only an off-centre picture shows left black: the game drew no more than a centred picture holds
        public double OffLeft = 1, OffRight = 1;   // each eye's off-centre picture's brightness against its centred one's, all over (an exposure worked out per picture)
        public double[] OffSettle;      // the brightness of each off-centre picture in turn (still settling after the change)
        public double[] LateSettle;     // the brightness of each of the later centred pictures in turn (not settled back, or the scene moved meanwhile)
    }

    static float[] MakeScene(int size, int seed) {
        var rnd = new Random(seed);
        var img = new float[size * size * 3];
        for (int i = 0; i < img.Length; ++i) img[i] = 105;
        for (int oct = 0; oct < 4; ++oct) {
            int cell = 96 >> oct, gw = size / cell + 2;
            float amp = 44f / (1 << oct);
            var grid = new float[gw * gw * 3];
            for (int i = 0; i < grid.Length; ++i) grid[i] = (float)(rnd.NextDouble() * 2 - 1);
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x) {
                    int gx = x / cell, gy = y / cell;
                    float fx = (x % cell) / (float)cell, fy = (y % cell) / (float)cell;
                    for (int c = 0; c < 3; ++c) {
                        float a = grid[(gy * gw + gx) * 3 + c], b = grid[(gy * gw + gx + 1) * 3 + c], cc = grid[((gy + 1) * gw + gx) * 3 + c], dd = grid[((gy + 1) * gw + gx + 1) * 3 + c];
                        img[(y * size + x) * 3 + c] += amp * ((a * (1 - fx) + b * fx) * (1 - fy) + (cc * (1 - fx) + dd * fx) * fy);
                    }
                }
        }
        for (int k = 0; k < 14; ++k) {  // things with hard edges
            int x0 = rnd.Next(size - 80), y0 = rnd.Next(size - 80), w = 30 + rnd.Next(120), h = 30 + rnd.Next(120);
            float add = (float)(rnd.NextDouble() * 90 - 45);
            for (int y = y0; y < Math.Min(size, y0 + h); ++y)
                for (int x = x0; x < Math.Min(size, x0 + w); ++x)
                    for (int c = 0; c < 3; ++c) img[(y * size + x) * 3 + c] += add;
        }
        for (int i = 0; i < img.Length; ++i) img[i] = Math.Max(20, Math.Min(235, img[i] + (float)(rnd.NextDouble() * 10 - 5)));
        return img;
    }

    // One picture of both eyes of the made-up scene, drawn centred or off-centre, with its own noise and the faults asked for. gain: this picture's brightness all over; index:
    // which of its kind it is (0, 1, 2).
    static Pic MakePic(float[] scene, int size, int margin, int ew, int eh, bool off, Fake f, int seed, string name, double gain, int index) {
        var p = new Pic();
        p.W = 2 * ew; p.H = eh; p.Off = off; p.Name = name; p.Px = new byte[p.W * p.H * 3];
        var rnd = new Random(seed);
        var grain = new Random(12345);  // the same in every picture: it sits still in the picture
        var grainMap = new float[ew * eh];
        for (int i = 0; i < grainMap.Length; ++i) grainMap[i] = (float)(grain.NextDouble() * 8 - 4);
        for (int e = 0; e < 2; ++e) {
            int sx = off ? (e == 0 ? f.Sx : -f.Sx) : 0, sy = off ? f.Sy : 0;
            for (int y = 0; y < eh; ++y)
                for (int x = 0; x < ew; ++x) {
                    // The scene's place this pixel shows: an off-centre picture has the centred picture's (x, y) at (x + sx, y + sy).
                    int wx = margin + x - sx + e * 5, wy = margin + y - sy;
                    double factor = 1, add = 0, even = gain * (off ? (e == 0 ? f.OffLeft : f.OffRight) : 1.0);  // (even: the whole picture's brightness, lights and all)
                    // A patch of the scene at another brightness in each off-centre picture (0.88, 1, 1.12): a light that flickers when drawn off-centre.
                    if (f.Flicker && off && wx > margin + ew * 0.45 && wx < margin + ew * 0.60 && wy > margin + eh * 0.45 && wy < margin + eh * 0.60) factor *= 0.88 + 0.12 * index;
                    // A lamp's light in the scene: a pool round a place. Right, it falls on the scene. With the fault, an off-centre picture lights it only in the 16-pixel
                    // squares of the picture that a centred picture would have had it in.
                    double lx = margin + ew * 0.42 + e * 5, ly = margin + eh * 0.60, radius = ew * 0.20;
                    double dist = Math.Sqrt((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
                    if (dist < radius && !f.Flat) {
                        bool lit = true;
                        if (f.Tiles && off) {
                            int tx = (x / 16) * 16 + 8, ty = (y / 16) * 16 + 8;  // this square's middle, taken as a centred picture's pixel
                            double tdx = margin + tx + e * 5 - lx, tdy = margin + ty - ly;
                            lit = Math.Sqrt(tdx * tdx + tdy * tdy) < radius;
                        }
                        if (lit) add += 60 * (1 - dist / radius);
                    }
                    // A shadow with a hard edge; with the fault its edge is 2 pixels off in an off-centre picture.
                    int edgeAt = margin + (int)(ew * 0.70) + (f.EdgeMoved && off ? 2 : 0);
                    if (wx > edgeAt && wy > margin + eh * 0.15 && wy < margin + eh * 0.5 && !f.Flat) factor *= 0.55;
                    if (f.Brighter && off) factor *= 1.05;
                    double dxv = (double)x / ew - 0.5, dyv = (double)y / eh - 0.5;
                    double vig = 1 + f.VignetteA * Math.Pow(dxv * dxv + dyv * dyv, 1.5);
                    for (int c = 0; c < 3; ++c) {
                        double v = ((f.Flat ? 105.0 : scene[(wy * size + wx) * 3 + c]) * factor + add) * even * vig + (rnd.NextDouble() * 2 - 1) * f.Noise + (f.Grain ? grainMap[y * ew + x] : 0);
                        if (f.NewBlack && off && (x - sx < 0 || x - sx >= ew || y - sy < 0 || y - sy >= eh)) v = 0;
                        if (f.Hud && x > ew * 0.08 && x < ew * 0.18 && y > eh * 0.93 && y < eh * 0.97) v = 230;
                        if (f.Gun) {
                            if (!off && x > ew * 0.40 && x < ew * 0.60 && y > eh * 0.62) v = 30;
                            if (off && x > ew * 0.25 && x < ew * 0.95 && y > eh * 0.50) v = 30;
                        }
                        p.Px[(y * p.W + e * ew + x) * 3 + c] = (byte)Math.Max(0, Math.Min(255, (int)Math.Round(v)));
                    }
                }
        }
        return p;
    }

    // One made-up run: three pictures drawn centred, three off-centre, three centred again; what the tool says of them.
    public static Result TryFake(Fake f, Options o, int ew, int eh, int saidSx, int saidSy) {
        int margin = 220, size = Math.Max(ew, eh) + 2 * margin + 8;
        float[] scene = MakeScene(size, 7);
        var centred = new List<Pic>(); var off = new List<Pic>();
        for (int i = 0; i < 3; ++i) centred.Add(MakePic(scene, size, margin, ew, eh, false, f, 100 + i, "c" + (i + 1), 1.0, i));
        for (int i = 0; i < 3; ++i) off.Add(MakePic(scene, size, margin, ew, eh, true, f, 200 + i, "o" + (i + 1), f.OffSettle != null ? f.OffSettle[i] : 1.0, i));
        for (int i = 0; i < 3; ++i) centred.Add(MakePic(scene, size, margin, ew, eh, false, f, 300 + i, "c" + (i + 4), f.LateSettle != null ? f.LateSettle[i] : 1.0, i));
        o.X = (double)saidSx / ew; o.Y = -(double)saidSy / eh;
        return Run(centred, off, o);
    }
}

}
'@

function Options {
    $o = New-Object PicDiff.Options
    $o.Flat = $Mask; $o.InCentred = $MaskCentred; $o.InOff = $MaskOff; $o.GunNote = $GunNote; $o.NoVignette = [bool]$NoVignette; $o.Ratio = $Ratio; $o.Slack = $Slack; $o.Grid = $Grid
    $o.Margin = $Margin; $o.Before = $Before; $o.Level = [bool]$Level
    return $o
}

if ($SelfTest) {
    # Made-up pictures of one scene, 720 x 720 an eye, the off-centre ones moved 144 pixels towards the nose and 144 up; each case has one thing wrong (or nothing), and the verdict
    # it must get. Nothing is read or written.
    $cases = @(
        @{ name = 'nothing wrong: the same scene, each picture its own noise'; set = @{}; want = 0; says = 'lighting and shadows the same' },
        @{ name = 'a vignette (corners 28% darker), nothing else'; set = @{ VignetteA = -0.8 }; want = 0; says = 'lighting and shadows the same'; vignette = $true },
        @{ name = 'a vignette, not looked for'; set = @{ VignetteA = -0.8 }; want = 2; says = 'a smooth slope'; noVignette = $true },
        @{ name = 'a lamp lit by 16-pixel squares of a centred picture'; set = @{ Tiles = $true }; want = 2; says = '16-pixel grid' },
        @{ name = 'the off-centre picture 5% brighter'; set = @{ Brighter = $true }; want = 2; says = 'brighter' },
        @{ name = 'a shadow edge 2 pixels off'; set = @{ EdgeMoved = $true }; want = 2; says = 'thin lines' },
        @{ name = 'the offset never reached the picture'; set = @{ Sx = 0; Sy = 0 }; want = 3; says = 'did not move' },
        @{ name = 'the picture moved the other way across'; set = @{ Sx = -144 }; want = 3; says = 'not by what the offset says' },
        @{ name = 'the picture moved across only (the offset down never arrived)'; set = @{ Sy = 0 }; want = 3; says = 'moved by (+144, 0) in the left eye' },
        @{ name = 'the picture moved by 0.7 of what the offset says (another unit)'; set = @{ Sx = 100; Sy = -100 }; want = 3; says = 'moved by (+100, -100) in the left eye' },
        @{ name = 'a HUD panel in the bottom corner (masked by default)'; set = @{ Hud = $true }; want = 0; says = 'lighting and shadows the same' },
        @{ name = 'a HUD panel, no mask'; set = @{ Hud = $true }; want = 2; says = 'DIFFER'; mask = '' },
        @{ name = 'the gun in both pictures, bigger off-centre, masked by where it is'; set = @{ Gun = $true }; want = 0; says = 'lighting and shadows the same'; maskCentred = '0.39,0.61,0.61,1'; maskOff = '0.24,0.49,0.96,1' },
        @{ name = 'a grain that sits still in the picture'; set = @{ Grain = $true }; want = 2; says = 'fine-grained' },
        # What must never pass as "the same" by accident.
        @{ name = 'everything left out by a rectangle'; set = @{}; want = 1; says = 'too little is left to compare'; mask = '0,0,1,1' },
        @{ name = 'pictures with nothing in them (one grey)'; set = @{ Flat = $true }; want = 1; says = 'cannot tell how far they moved' },
        @{ name = 'no offset at all'; set = @{ Sx = 0; Sy = 0 }; want = 1; says = 'no pixels of the picture'; said = @(0, 0) },
        @{ name = 'the brightness still settling after each change of the projection'; set = @{ OffSettle = @(0.94, 0.92, 0.91); LateSettle = @(0.96, 0.975, 0.985) }; want = 2; says = 'not steady among themselves' },
        @{ name = 'the same, slower: the best pair is within the pictures drawn alike'; set = @{ OffSettle = @(0.97, 0.955, 0.95); LateSettle = @(0.975, 0.985, 0.99) }; want = 2; says = 'not steady among themselves' },
        @{ name = 'the scene moved between the first centred pictures and the last'; set = @{ LateSettle = @(1.04, 1.04, 1.04) }; want = 1; says = 'did not hold still' },
        @{ name = 'a patch that flickers in the off-centre pictures, one of them as the centred ones'; set = @{ Flicker = $true }; want = 2; says = 'differ from each other' },
        @{ name = 'what only the off-centre picture shows not drawn (black)'; set = @{ NewBlack = $true }; want = 2; says = 'did not draw' },
        # A brightness worked out from each picture's own content.
        @{ name = 'each eye''s off-centre picture at a brightness of its own'; set = @{ OffLeft = 1.05; OffRight = 0.96 }; want = 2; says = 'an even gain' },
        @{ name = 'the same, the even gain taken out (-Level)'; set = @{ OffLeft = 1.05; OffRight = 0.96 }; want = 2; says = 'only by an even gain'; level = $true },
        @{ name = 'an even gain and a lamp lit by squares, the gain taken out (-Level)'; set = @{ OffLeft = 1.05; OffRight = 0.96; Tiles = $true }; want = 2; says = '16-pixel grid'; level = $true },
        @{ name = 'nothing wrong, an even gain looked for (-Level)'; set = @{}; want = 0; says = 'lighting and shadows the same'; level = $true }
    )
    $failed = 0
    foreach ($case in $cases) {
        $fake = New-Object PicDiff.Tool+Fake
        $fake.Sx = 144; $fake.Sy = -144
        foreach ($k in $case.set.Keys) { $fake.$k = $case.set[$k] }
        $o = Options
        $o.Before = -1; $o.Level = $false
        if ($case.ContainsKey('mask')) { $o.Flat = $case.mask }
        if ($case.ContainsKey('maskCentred')) { $o.InCentred = $case.maskCentred; $o.InOff = $case.maskOff }
        if ($case.ContainsKey('noVignette')) { $o.NoVignette = $true }
        if ($case.ContainsKey('level')) { $o.Level = $true }
        $said = if ($case.ContainsKey('said')) { $case.said } else { @(144, -144) }
        $r = [PicDiff.Tool]::TryFake($fake, $o, 720, 720, $said[0], $said[1])
        $ok = $r.Verdict -eq $case.want -and $r.VerdictText -match [regex]::Escape($case.says)
        if ($case.ContainsKey('vignette')) { $ok = $ok -and $r.VignetteFound }
        if (-not $ok) { $failed++ }
        Write-Host ('{0}  {1}' -f $(if ($ok) { 'PASS' } else { 'FAIL' }), $case.name)
        Write-Host ('      ' + $r.VerdictText)
        if (-not $ok) { $r.Lines | ForEach-Object { Write-Host "      | $_" } }
    }
    Write-Host ("picdiff self-test: {0} of {1} cases as they should be" -f ($cases.Count - $failed), $cases.Count)
    exit $(if ($failed) { 1 } else { 0 })
}

# ---- The pictures ----
function Files([string]$list) { return @($list -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ }) }
$inv = [Globalization.CultureInfo]::InvariantCulture
$centredFiles = @(Files $Centred); $offFiles = @(Files $Off)
if ($Run) {
    if (-not (Test-Path $Run)) { Write-Host "No folder $Run"; exit 1 }
    $Run = (Resolve-Path $Run).Path
    $centredFiles += @(Get-ChildItem $Run -File | Where-Object { $_.Name -match '^c\d+\.(bmp|png)$' } | Sort-Object { [int]($_.BaseName.Substring(1)) } | ForEach-Object { $_.FullName })
    $offFiles += @(Get-ChildItem $Run -File | Where-Object { $_.Name -match '^o\d+\.(bmp|png)$' } | Sort-Object { [int]($_.BaseName.Substring(1)) } | ForEach-Object { $_.FullName })
    $said = Join-Path $Run 'offset.txt'
    if (([double]::IsNaN($X) -or [double]::IsNaN($Y)) -and (Test-Path $said)) {
        $n = @((Get-Content $said -TotalCount 1) -split '\s+' | Where-Object { $_ })
        if ($n.Count -ge 2) { $X = [double]::Parse($n[0], $inv); $Y = [double]::Parse($n[1], $inv) }
    }
}
if ([double]::IsNaN($X) -or [double]::IsNaN($Y)) { Write-Host 'picdiff.ps1 -Run <folder>   |   -Centred "a.bmp;b.bmp" -Off "c.bmp;d.bmp" -X <outward> -Y <down>   |   -SelfTest   (the offset is needed: -X -Y, or the folder''s offset.txt)'; exit 1 }
if (-not $centredFiles.Count -or -not $offFiles.Count) { Write-Host 'No pictures: at least one drawn centred and one drawn off-centre are needed (-Run <folder> with c*.bmp and o*.bmp, or -Centred and -Off)'; exit 1 }
foreach ($f in @($centredFiles + $offFiles)) { if (-not (Test-Path $f)) { Write-Host "No picture at $f"; exit 1 } }
$centredPics = New-Object 'System.Collections.Generic.List[PicDiff.Pic]'
$offPics = New-Object 'System.Collections.Generic.List[PicDiff.Pic]'
foreach ($f in $centredFiles) { $centredPics.Add([PicDiff.Tool]::Load((Resolve-Path $f).Path, $false)) }
foreach ($f in $offFiles) { $offPics.Add([PicDiff.Tool]::Load((Resolve-Path $f).Path, $true)) }

$o = Options
$o.X = $X; $o.Y = $Y
if (-not $Out) {
    $folder = if ($Run) { $Run } else { Join-Path (Split-Path $PSScriptRoot) 'screenshots' }
    if (-not (Test-Path $folder)) { New-Item -ItemType Directory -Path $folder -Force | Out-Null }
    $Out = Join-Path $folder ('picdiff_{0}.png' -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
}
$Out = [IO.Path]::GetFullPath($Out)
$o.DiffPath = $Out
$o.ViewsPath = ($Out -replace '\.png$', '') + '_views.png'
$o.WorstPath = ($Out -replace '\.png$', '') + '_worst.png'
$o.UnsteadyPath = ($Out -replace '\.png$', '') + '_unsteady.png'
foreach ($old in @($o.DiffPath, $o.ViewsPath, $o.WorstPath, $o.UnsteadyPath)) { if (Test-Path $old) { Remove-Item $old -Force } }
$r = [PicDiff.Tool]::Run($centredPics, $offPics, $o)
$r.Lines | ForEach-Object { Write-Host $_ }
if (Test-Path $o.DiffPath) { Write-Host "The difference image: $($o.DiffPath) (half size; top: the pair used, bottom: the control; grey: six times the difference; dark blue: outside the overlap; dark green: left out)" }
if (Test-Path $o.ViewsPath) { Write-Host "The pair itself, small: $($o.ViewsPath) (top: drawn centred, bottom: drawn off-centre)" }
if (Test-Path $o.WorstPath) { Write-Host "The cell furthest over the controls: $($o.WorstPath) (twice life size; left: drawn centred; middle: drawn off-centre, moved back onto it; right: six times their difference)" }
if (Test-Path $o.UnsteadyPath) { Write-Host "Where two off-centre pictures differ most from each other: $($o.UnsteadyPath) (life size; the same piece of the scene out of each off-centre picture, the first on the left)" }
switch ($r.Verdict) { 0 { exit 0 } 1 { exit 1 } default { exit 2 } }

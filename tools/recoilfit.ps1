# Fits the game's own kick in its weapon camera (design 6, probe P2) from the log's per-frame recoil lines. With build\recoil.txt "game 1" the game's
# kick is left in its data, "drift 0", "kick 0" and "buzz 0" keep the mod's own out, "log N" writes a line a frame (recoilrun.cpp), and the stand-in
# holds the gun still: the weapon camera less the look the mod wrote is then the game's kick alone. That is fitted against recoil.h's layer A (each
# round raises a target, the target comes back after a string, the camera follows it through a spring), and this says what the game's numbers are and
# which of recoil.h's constants they set: kStringGap, kDriftLinear, kShareLeast, Tune::sideSign's default, and whether the data's SpringDamping is a
# damping ratio. Pure: it only reads the log, and src\recoilguns.h and src\recoil.h for the gun's own numbers and the constants as they are now.
# How: the whole run fitted, each law (degrees a second, a share a second), from several starts, the recovery right after a string held to the data's
# own share of the one after; whether anything comes back inside a string (the best below one gap against the best above it; and, should the rise a
# round come out under the data's own, with the rise held at the data's and the gap from 0: with a linear law a recovery between the rounds is the
# same curve as a smaller rise); the string gap across a range with each round's own rise solved at each try (the rounds' own 90-100% otherwise
# pulls it), as the range the run can't tell apart; then each round's own rise and sideways pick, pull by pull, for the spread (each round's error
# its fit's and its place in its frame: a round is only known to the frame it was counted in), the first round's multiplier and the sign; and, with
# a linear law, the settle after the last round alone, as a free spring, for its frequency and damping. The turn says which way the log's yaw grows
# to the right. About 15-25 s (a few more when the rise is under the data's).
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\recoilfit.ps1 -Log build\bf3vr.log [-From 12:34:56] [-To 12:36:10] [-Gun M416]
# -From and -To are the log's own clock (hh:mm:ss or hh:mm:ss.fff); without them every per-frame line in the file is used. -Gun: the gun's name as the
# table has it (otherwise the log's lines name it). A stick turn in the same lines (weapon.txt "turnby 20") tells which way the log's yaw grows, for
# the sideways sign. The exit code counts what couldn't be fitted, and a rise a round that isn't the data's.
param([string]$Log = '', [string]$From = '', [string]$To = '', [string]$Gun = '')

$ErrorActionPreference = 'Stop'
$inv = [Globalization.CultureInfo]::InvariantCulture
$root = Split-Path -Parent $PSScriptRoot
if (-not $Log) { Write-Host 'recoilfit.ps1 -Log <file> [-From hh:mm:ss] [-To hh:mm:ss] [-Gun M416]'; exit 1 }
if (-not (Test-Path $Log)) { Write-Host "No log at $Log"; exit 1 }
$Log = (Resolve-Path $Log).Path
$problems = 0

# ---- The model (C#: some seconds of the game's kick stepped 1 ms at a time, thousands of times over, are too slow for PowerShell) ----
if (-not ('RecoilFit.Model' -as [type])) {
    Add-Type -TypeDefinition @'
using System;

namespace RecoilFit
{
    // The game's kick in its weapon camera as recoil.h's layer A has it: each round raises a target; after a string the target comes back (in degrees a
    // second, or a share a second); the camera follows the target through a spring. Stepped 1 ms at a time, as recoil.h steps it.
    public static class Model
    {
        public const double H = 0.001;

        // x'' = -w^2 (x - target) - 2 z w x' over h with the target held, exactly, for any damping: x = target + m0 y + m1 v, v = m2 y + m3 v (y = x - target).
        public static double[] Over(double w, double z, double h)
        {
            double[] m = new double[] { 1, 0, 0, 1 };
            if (!(h > 0) || !(w > 0) || double.IsInfinity(w) || double.IsNaN(z))
                return m;
            if (z < 0)
                z = 0;
            if (Math.Abs(z - 1.0) < 1e-6)
            {
                double e = Math.Exp(-w * h);
                m[0] = (1 + w * h) * e; m[1] = h * e; m[2] = -w * w * h * e; m[3] = (1 - w * h) * e;
            }
            else if (z < 1)
            {
                double a = z * w, wd = w * Math.Sqrt(1 - z * z), e = Math.Exp(-a * h), c = Math.Cos(wd * h), sd = Math.Sin(wd * h) / wd;
                m[0] = e * (c + a * sd); m[1] = e * sd; m[2] = -e * w * w * sd; m[3] = e * (c - a * sd);
            }
            else
            {
                double q = Math.Sqrt(z * z - 1), r1 = -w * (z - q), r2 = -w * (z + q), d = r1 - r2;
                double e1 = Math.Exp(r1 * h), e2 = Math.Exp(r2 * h);
                m[0] = (r1 * e2 - r2 * e1) / d; m[1] = (e1 - e2) / d; m[2] = r1 * r2 * (e2 - e1) / d; m[3] = (r1 * e1 - r2 * e2) / d;
            }
            return m;
        }

        // A target raised by 1 at time 0, the camera at rest: where the camera is tau later (Over's first number, without making one).
        public static double StepAt(double w, double z, double tau)
        {
            if (!(tau > 0) || !(w > 0))
                return 0;
            if (z < 0)
                z = 0;
            if (Math.Abs(z - 1.0) < 1e-6)
            {
                double e = Math.Exp(-w * tau);
                return 1 - (1 + w * tau) * e;
            }
            if (z < 1)
            {
                double a = z * w, wd = w * Math.Sqrt(1 - z * z), e = Math.Exp(-a * tau);
                return 1 - e * (Math.Cos(wd * tau) + a * Math.Sin(wd * tau) / wd);
            }
            double q = Math.Sqrt(z * z - 1), r1 = -w * (z - q), r2 = -w * (z + q);
            return 1 - (r1 * Math.Exp(r2 * tau) - r2 * Math.Exp(r1 * tau)) / (r1 - r2);
        }

        // One pull's rounds' own rises (rounds a to b), from its frames between `from` and `to` (s), the motion the rounds before it left (their rises
        // in inc) taken off. Each round's column is its own rise's motion: with a share law the recovery between the rounds takes a share of each
        // (so each is run on its own), with a linear one it takes the same whatever the rises (so it is added back first). NaN: a round with no frame
        // of its own. se: each one's error.
        public static double[] PullSolve(double[] p, int law, double gap, double cap, double[] rt, int a, int b, double[] inc, double[] ft, double[] y, double[] wgt, double from, double to, double[] se)
        {
            int count = b - a + 1, n = ft.Length;
            double[] got = new double[count];
            for (int c = 0; c < count; c++) { got[c] = double.NaN; se[c] = double.NaN; }
            System.Collections.Generic.List<int> idx = new System.Collections.Generic.List<int>();
            for (int i = 0; i < n; i++)
                if (wgt[i] > 0 && ft[i] >= from && ft[i] <= to) idx.Add(i);
            if (idx.Count < 3 || n == 0)
                return got;
            double[] wf = new double[idx.Count];
            for (int q = 0; q < idx.Count; q++) wf[q] = ft[idx[q]];
            double t0 = ft[0], w = p[5], z = p[6], delay = p[7], huge = 1e9;
            double[] free = new double[idx.Count];
            if (a > 0)
            {
                double[] ert = new double[a], einc = new double[a];
                Array.Copy(rt, ert, a);
                Array.Copy(inc, einc, a);
                free = Run(p, law, gap, cap, ert, null, einc, wf, null, t0);
            }
            double[] prt = new double[count];
            Array.Copy(rt, a, prt, 0, count);
            double[] offset = new double[idx.Count];
            if (law == 0)
            {
                // (What comes off: a target too big to reach nothing, run with the recovery and without; the same steps, so only the recovery differs.)
                double[] big = new double[count];
                big[0] = 1000;
                double[] still = (double[])p.Clone();
                still[3] = 0; still[4] = 0;
                double[] yb = Run(p, law, gap, huge, prt, null, big, wf, null, t0), y0 = Run(still, law, gap, huge, prt, null, big, wf, null, t0);
                for (int q = 0; q < idx.Count; q++) offset[q] = y0[q] - yb[q];
            }
            System.Collections.Generic.List<double[]> cols = new System.Collections.Generic.List<double[]>();
            System.Collections.Generic.List<int> which = new System.Collections.Generic.List<int>();
            for (int c = 0; c < count; c++)
            {
                double[] col;
                if (law == 1)
                {
                    double[] e = new double[count];
                    e[c] = 1;
                    col = Run(p, law, gap, huge, prt, null, e, wf, null, t0);
                }
                else
                {
                    col = new double[idx.Count];
                    for (int q = 0; q < idx.Count; q++) col[q] = StepAt(w, z, wf[q] - prt[c] - delay);
                }
                double size = 0;
                for (int q = 0; q < idx.Count; q++) size += col[q] * col[q];
                if (size > 1e-4) { cols.Add(col); which.Add(c); }
            }
            if (cols.Count == 0 || idx.Count < cols.Count + 2)
                return got;
            double[] rhs = new double[idx.Count], w1 = new double[idx.Count];
            for (int q = 0; q < idx.Count; q++) { rhs[q] = y[idx[q]] - free[q] + offset[q]; w1[q] = 1; }
            double[] sec;
            double rms;
            double[] b1 = Lsq.Solve(cols.ToArray(), rhs, w1, out sec, out rms);
            for (int c = 0; c < which.Count; c++) { got[which[c]] = b1[c]; se[which[c]] = sec[c]; }
            return got;
        }

        // Each round's own rise solved for these numbers, pull by pull (a sum of the spring's steps, one a round, from the pull's first round until
        // the recovery would start after its last; the motion the pulls before left taken off), then the whole run with them: its sum of squares.
        // A round with no frame of its own takes its pull's mean. inc (not null, as long as rt): the rises.
        public static double JointSse(double[] p, int law, double gap, double cap, double[] rt, int[] pullA, int[] pullB, double[] ft, double[] y, double[] wgt, double[] inc)
        {
            int nr = rt.Length, n = ft.Length;
            if (inc == null || inc.Length != nr)
                inc = new double[nr];
            bool[] known = new bool[nr];
            double w = p[5], z = p[6], delay = p[7], after = Math.Min(Math.Max(p[2], 0.6), 2.0) * gap, t0 = n > 0 ? ft[0] : 0;
            for (int j = 0; j < pullA.Length; j++)
            {
                int a = pullA[j], b = pullB[j];
                double from = rt[a] + delay - 0.02, to = rt[b] + delay + after;
                System.Collections.Generic.List<int> idx = new System.Collections.Generic.List<int>();
                for (int i = 0; i < n; i++)
                    if (wgt[i] > 0 && ft[i] >= from && ft[i] <= to) idx.Add(i);
                double sum = 0;
                int count = 0;
                if (idx.Count >= 3)
                {
                    double[] wf = new double[idx.Count];
                    for (int q = 0; q < idx.Count; q++) wf[q] = ft[idx[q]];
                    double[] free = new double[idx.Count];
                    if (a > 0)
                    {
                        double[] ert = new double[a], einc = new double[a];
                        Array.Copy(rt, ert, a);
                        Array.Copy(inc, einc, a);
                        free = Run(p, law, gap, cap, ert, null, einc, wf, null, t0);
                    }
                    System.Collections.Generic.List<double[]> cols = new System.Collections.Generic.List<double[]>();
                    System.Collections.Generic.List<int> which = new System.Collections.Generic.List<int>();
                    for (int k = a; k <= b; k++)
                    {
                        double[] col = new double[idx.Count];
                        double size = 0;
                        for (int q = 0; q < idx.Count; q++) { col[q] = StepAt(w, z, wf[q] - rt[k] - delay); size += col[q] * col[q]; }
                        if (size > 1e-4) { cols.Add(col); which.Add(k); }
                    }
                    if (cols.Count > 0 && idx.Count >= cols.Count + 2)
                    {
                        double[] rhs = new double[idx.Count], w1 = new double[idx.Count];
                        for (int q = 0; q < idx.Count; q++) { rhs[q] = y[idx[q]] - free[q]; w1[q] = 1; }
                        double[] se;
                        double rms;
                        double[] got = Lsq.Solve(cols.ToArray(), rhs, w1, out se, out rms);
                        for (int c = 0; c < which.Count; c++)
                        {
                            if (double.IsNaN(got[c])) continue;
                            inc[which[c]] = got[c]; known[which[c]] = true; sum += got[c]; count++;
                        }
                    }
                }
                for (int k = a; k <= b; k++)
                    if (!known[k]) inc[k] = count > 0 ? sum / count : p[0];
            }
            double[] m = Run(p, law, gap, cap, rt, null, inc, ft, null, t0);
            double s = 0;
            for (int i = 0; i < n; i++)
            {
                double d = y[i] - m[i];
                s += wgt[i] * d * d;
            }
            return double.IsNaN(s) ? double.MaxValue : s;
        }

        // p: the rise a round, the first round's multiplier, the string gap (in the gun's gaps), the recovery while a round left within 2 gaps, the
        // recovery after, the spring's frequency (rad/s), its damping ratio, the camera's round against the log's (s). law 0: linear (degrees a second),
        // 1: a share a second (recoil.h's: kept 1 - rate x 1 ms each ms). Rounds at rt (s), first[k]: a pull's first; inc not null: each round's own
        // rise instead. From t0 at rest; the camera at each frame time ft (in order), and the target there (targets, not null). A round and the
        // recovery's start each come where they fall inside a step, and a frame between two steps is read between them, so every number moves the
        // answer smoothly (a fit's slopes and the sureness of each number need that).
        public static double[] Run(double[] p, int law, double gap, double cap, double[] rt, bool[] first, double[] inc, double[] ft, double[] targets, double t0)
        {
            int n = ft.Length;
            double[] outv = new double[n];
            if (n == 0)
                return outv;
            double h = H, w = p[5], z = p[6];
            double[] m = Over(w, z, h);
            double x = 0, v = 0, target = 0, since = 1e9;
            int k = 0, i = 0;
            double t = Math.Min(t0, ft[0]) - 2 * h;
            double delay = p[7], sg = p[2] * gap, fg = 2.0 * gap, rIn = p[3], rOut = p[4];
            double lnIn = Math.Log(Math.Max(1 - rIn * h, 1e-300)), lnOut = Math.Log(Math.Max(1 - rOut * h, 1e-300));
            while (k < rt.Length && rt[k] + delay < t)
                k++;
            while (i < n)
            {
                // At rest with no round before the next frame: straight on to it.
                if (target == 0 && Math.Abs(x) < 1e-9 && Math.Abs(v) < 1e-7)
                {
                    double next = k < rt.Length ? rt[k] + delay : double.MaxValue;
                    while (i < n && ft[i] < next - h)
                    {
                        outv[i] = x;
                        if (targets != null) targets[i] = 0;
                        i++;
                    }
                    if (i >= n)
                        break;
                    if (next - h > t) { since += next - h - t; t = next - h; }
                }
                double xWas = x, end = t + h, at = t;
                while (k < rt.Length && rt[k] + delay < end)
                {
                    double ts = Math.Max(rt[k] + delay, at);
                    Move(ref x, ref v, ref target, ref since, ts - at, m, law, sg, fg, rIn, rOut, lnIn, lnOut, w, z);
                    double up = inc != null ? inc[k] : p[0] * (first[k] ? p[1] : 1.0);
                    double most = Math.Max(cap, Math.Abs(target));
                    target = Math.Max(-most, Math.Min(most, target + up));
                    since = 0;
                    at = ts;
                    k++;
                }
                Move(ref x, ref v, ref target, ref since, end - at, m, law, sg, fg, rIn, rOut, lnIn, lnOut, w, z);
                t = end;
                while (i < n && ft[i] <= t)
                {
                    double f = (ft[i] - (t - h)) / h;
                    outv[i] = xWas + (x - xWas) * (f < 0 ? 0 : f > 1 ? 1 : f);
                    if (targets != null) targets[i] = target;
                    i++;
                }
            }
            return outv;
        }

        // dt of it: the target comes back for the part of dt past the string gap (each part at its own rate), then the spring follows it.
        static void Move(ref double x, ref double v, ref double target, ref double since, double dt, double[] mStep, int law, double sg, double fg,
                         double rIn, double rOut, double lnIn, double lnOut, double w, double z)
        {
            if (!(dt > 0))
                return;
            if (target != 0 && since + dt > sg)
            {
                double a0 = Math.Max(since, sg), a1 = since + dt, inA = 0, inB = 0;
                if (sg < fg)
                {
                    inA = Math.Max(0, Math.Min(a1, fg) - a0);
                    inB = Math.Max(0, a1 - Math.Max(a0, fg));
                }
                else
                    inB = a1 - a0;
                if (law == 0)
                {
                    double by = rIn * inA + rOut * inB;
                    target = target > 0 ? Math.Max(target - by, 0) : Math.Min(target + by, 0);
                }
                else
                {
                    target *= Math.Exp((inA * lnIn + inB * lnOut) / H);
                    if (Math.Abs(target) < 1e-12) target = 0;
                }
            }
            double[] m = Math.Abs(dt - H) < 1e-12 ? mStep : Over(w, z, dt);
            double y = x - target;
            double nx = target + m[0] * y + m[1] * v;
            v = m[2] * y + m[3] * v;
            x = nx;
            since += dt;
        }

        public static double Sse(double[] p, int law, double gap, double cap, double[] rt, bool[] first, double[] ft, double[] y, double[] wgt)
        {
            return Sse(p, law, gap, cap, rt, first, null, ft, y, wgt);
        }
        public static double Sse(double[] p, int law, double gap, double cap, double[] rt, bool[] first, double[] inc, double[] ft, double[] y, double[] wgt)
        {
            double[] m = Run(p, law, gap, cap, rt, first, inc, ft, null, ft.Length > 0 ? ft[0] : 0);
            double s = 0;
            for (int i = 0; i < ft.Length; i++)
            {
                double d = y[i] - m[i];
                s += wgt[i] * d * d;
            }
            return double.IsNaN(s) ? double.MaxValue : s;
        }
    }

    public static class Fit
    {
        public static double Clamp(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }
        static double Logistic(double q) { return 1.0 / (1.0 + Math.Exp(-q)); }

        // The search's own numbers (any real) to the model's, each in its range, and back; and how fast each model number moves with its own.
        public static double[] ToModel(double[] q)
        {
            double[] p = new double[8];
            p[0] = Clamp(Math.Exp(q[0]), 1e-4, 20);
            p[1] = Clamp(Math.Exp(q[1]), 0.05, 20);
            p[2] = 4.0 * Logistic(q[2]);
            p[3] = Clamp(Math.Exp(q[3]), 1e-4, 2000);
            p[4] = Clamp(Math.Exp(q[4]), 1e-4, 2000);
            p[5] = Clamp(Math.Exp(q[5]), 1.0, 400);
            p[6] = Clamp(Math.Exp(q[6]), 0.002, 4);
            p[7] = -0.1 + 0.15 * Logistic(q[7]);
            return p;
        }
        public static double[] FromModel(double[] p)
        {
            double[] q = new double[8];
            q[0] = Math.Log(Clamp(p[0], 1e-4, 20)); q[1] = Math.Log(Clamp(p[1], 0.05, 20));
            double g = Clamp(p[2] / 4.0, 1e-4, 1 - 1e-4); q[2] = Math.Log(g / (1 - g));
            q[3] = Math.Log(Clamp(p[3], 1e-4, 2000)); q[4] = Math.Log(Clamp(p[4], 1e-4, 2000));
            q[5] = Math.Log(Clamp(p[5], 1.0, 400)); q[6] = Math.Log(Clamp(p[6], 0.002, 4));
            double d = Clamp((p[7] + 0.1) / 0.15, 1e-4, 1 - 1e-4); q[7] = Math.Log(d / (1 - d));
            return q;
        }
        public static double[] Slope(double[] q)
        {
            double[] p = ToModel(q), s = new double[8];
            for (int i = 0; i < 8; i++) s[i] = p[i];
            double g = Logistic(q[2]), d = Logistic(q[7]);
            s[2] = 4.0 * g * (1 - g);
            s[7] = 0.15 * d * (1 - d);
            return s;
        }

        // Nelder and Mead's simplex, from x0 with steps of `scale`, at most `most` looks.
        public static double[] Minimize(Func<double[], double> f, double[] x0, double scale, int most, out double best)
        {
            int n = x0.Length;
            double[][] s = new double[n + 1][];
            double[] fs = new double[n + 1];
            for (int i = 0; i <= n; i++)
            {
                s[i] = (double[])x0.Clone();
                if (i > 0) s[i][i - 1] += scale;
                fs[i] = f(s[i]);
            }
            int used = n + 1;
            while (used < most)
            {
                Array.Sort(fs, s);
                double size = 0;
                for (int i = 1; i <= n; i++)
                    for (int j = 0; j < n; j++)
                        size = Math.Max(size, Math.Abs(s[i][j] - s[0][j]));
                if (size < 1e-7 || Math.Abs(fs[n] - fs[0]) <= 1e-13 * (Math.Abs(fs[0]) + 1e-300))
                    break;
                double[] c = new double[n];
                for (int i = 0; i < n; i++)
                    for (int j = 0; j < n; j++)
                        c[j] += s[i][j] / n;
                double[] r = Toward(c, s[n], -1.0);
                double fr = f(r); used++;
                if (fr < fs[0])
                {
                    double[] e = Toward(c, s[n], -2.0);
                    double fe = f(e); used++;
                    if (fe < fr) { s[n] = e; fs[n] = fe; } else { s[n] = r; fs[n] = fr; }
                }
                else if (fr < fs[n - 1])
                {
                    s[n] = r; fs[n] = fr;
                }
                else
                {
                    double[] k = fr < fs[n] ? Toward(c, s[n], -0.5) : Toward(c, s[n], 0.5);
                    double fk = f(k); used++;
                    if (fk < Math.Min(fr, fs[n]))
                    {
                        s[n] = k; fs[n] = fk;
                    }
                    else
                    {
                        for (int i = 1; i <= n; i++)
                        {
                            for (int j = 0; j < n; j++) s[i][j] = s[0][j] + 0.5 * (s[i][j] - s[0][j]);
                            fs[i] = f(s[i]); used++;
                        }
                    }
                }
            }
            Array.Sort(fs, s);
            best = fs[0];
            return s[0];
        }
        static double[] Toward(double[] c, double[] w, double t)
        {
            double[] r = new double[c.Length];
            for (int j = 0; j < c.Length; j++) r[j] = c[j] + t * (w[j] - c[j]);
            return r;
        }

        // The law's best from each start (model numbers), searched again from the best: the model's numbers, its sum of squares.
        public static double[] Best(Problem pr, double[][] starts, int most, out double sse)
        {
            Func<double[], double> f = qa => pr.Sse(qa);
            double[] bestQ = null;
            sse = double.MaxValue;
            foreach (double[] start in starts)
            {
                double got;
                double[] qa = Minimize(f, pr.Searched(start), 0.3, most, out got);
                if (got < sse) { sse = got; bestQ = qa; }
            }
            for (int again = 0; again < 2 && bestQ != null; again++)
            {
                double got;
                double[] qa = Minimize(f, bestQ, 0.1, most, out got);
                if (got < sse) { sse = got; bestQ = qa; }
            }
            return bestQ == null ? null : pr.Full(bestQ);
        }

        // About how sure each searched number is: the sum of squares' curvature at the best (its own numbers), turned into the model's; NaN: not known
        // (or held). The residuals run together from frame to frame, so this is the least it could be off by, not the most.
        public static double[] Errors(Problem pr, double[] p, int count)
        {
            double[] q = pr.Searched(p);
            int n = q.Length;
            Func<double[], double> f = x => pr.Sse(x);
            double f0 = f(q), hs = 2e-3;
            double[,] hm = new double[n, n];
            for (int i = 0; i < n; i++)
                for (int j = i; j < n; j++)
                {
                    double v;
                    if (i == j)
                    {
                        double[] a = (double[])q.Clone(); a[i] += 2 * hs;
                        double[] b = (double[])q.Clone(); b[i] -= 2 * hs;
                        v = (f(a) - 2 * f0 + f(b)) / (4 * hs * hs);
                    }
                    else
                    {
                        double[] pp = (double[])q.Clone(); pp[i] += hs; pp[j] += hs;
                        double[] pm = (double[])q.Clone(); pm[i] += hs; pm[j] -= hs;
                        double[] mp = (double[])q.Clone(); mp[i] -= hs; mp[j] += hs;
                        double[] mm = (double[])q.Clone(); mm[i] -= hs; mm[j] -= hs;
                        v = (f(pp) - f(pm) - f(mp) + f(mm)) / (4 * hs * hs);
                    }
                    hm[i, j] = v; hm[j, i] = v;
                }
            double sigma2 = f0 / Math.Max(count - n, 1);
            double[,] inverse = Lsq.Inverse(hm);
            double[] slope = Slope(pr.Whole(q)), se = new double[8];
            for (int i = 0; i < 8; i++) se[i] = double.NaN;
            int at = 0;
            for (int i = 0; i < 8; i++)
            {
                if (!pr.Active[i]) continue;
                double c = inverse == null ? double.NaN : 2 * sigma2 * inverse[at, at];
                se[i] = c > 0 ? Math.Sqrt(c) * Math.Abs(slope[i]) : double.NaN;
                at++;
            }
            return se;
        }
    }

    // One fit's own terms: the law, the gun's gap and cap, the rounds and frames, which numbers are searched (the rest held at Held), the
    // recovery while firing tied to the one after (Tie > 0: that times it, the design's DecreaseFactor x ShootingRecoilDecreaseScale), and each
    // round's own rise (Inc, not null: instead of the rise a round and the first one's multiplier).
    public class Problem
    {
        public int Law;
        public double Gap, Cap, Tie;
        public double[] Rt, Ft, Y, Wgt, Inc;
        public bool[] First;
        public bool Joint;           // each round's own rise solved for every try of the rest (Model.JointSse): the pulls' first and last rounds
        public int[] PullA, PullB;
        public bool[] Active = new bool[] { true, true, true, true, true, true, true, true };
        public double[] Held = new double[8];

        public double[] Whole(double[] qa)
        {
            double[] q = Fit.FromModel(Held);
            int at = 0;
            for (int i = 0; i < 8; i++)
                if (Active[i]) q[i] = qa[at++];
            return q;
        }
        public double[] Full(double[] qa)
        {
            double[] p = Fit.ToModel(Whole(qa));
            for (int i = 0; i < 8; i++)
                if (!Active[i]) p[i] = Held[i];
            if (Tie > 0) p[3] = p[4] * Tie;
            return p;
        }
        public double[] Searched(double[] p)
        {
            double[] q = Fit.FromModel(p);
            int n = 0;
            for (int i = 0; i < 8; i++) if (Active[i]) n++;
            double[] qa = new double[n];
            int at = 0;
            for (int i = 0; i < 8; i++)
                if (Active[i]) qa[at++] = q[i];
            return qa;
        }
        public double Sse(double[] qa)
        {
            if (Joint)
                return Model.JointSse(Full(qa), Law, Gap, Cap, Rt, PullA, PullB, Ft, Y, Wgt, null);
            return Model.Sse(Full(qa), Law, Gap, Cap, Rt, First, Inc, Ft, Y, Wgt);
        }
    }

    public static class Lsq
    {
        // Gauss and Jordan with the largest pivot; null if singular.
        public static double[,] Inverse(double[,] a)
        {
            int n = a.GetLength(0);
            double[,] m = new double[n, 2 * n];
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < n; j++) m[i, j] = a[i, j];
                m[i, n + i] = 1;
            }
            for (int c = 0; c < n; c++)
            {
                int best = c;
                for (int r = c + 1; r < n; r++) if (Math.Abs(m[r, c]) > Math.Abs(m[best, c])) best = r;
                if (!(Math.Abs(m[best, c]) > 1e-300)) return null;
                if (best != c) for (int j = 0; j < 2 * n; j++) { double t = m[c, j]; m[c, j] = m[best, j]; m[best, j] = t; }
                double d = m[c, c];
                for (int j = 0; j < 2 * n; j++) m[c, j] /= d;
                for (int r = 0; r < n; r++)
                {
                    if (r == c || m[r, c] == 0) continue;
                    double f = m[r, c];
                    for (int j = 0; j < 2 * n; j++) m[r, j] -= f * m[c, j];
                }
            }
            double[,] o = new double[n, n];
            for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) o[i, j] = m[i, n + j];
            return o;
        }

        // y = sum of cols[k] x b[k], weighted: b, each one's standard error (se), the residual's rms. A whisker of ridge keeps it solvable.
        public static double[] Solve(double[][] cols, double[] y, double[] wgt, out double[] se, out double rms)
        {
            int k = cols.Length, n = y.Length;
            double[,] a = new double[k, k];
            double[] r = new double[k];
            double count = 0;
            for (int i = 0; i < n; i++) count += wgt[i];
            for (int p = 0; p < k; p++)
            {
                for (int q = p; q < k; q++)
                {
                    double s = 0;
                    for (int i = 0; i < n; i++) s += wgt[i] * cols[p][i] * cols[q][i];
                    a[p, q] = s; a[q, p] = s;
                }
                double t = 0;
                for (int i = 0; i < n; i++) t += wgt[i] * cols[p][i] * y[i];
                r[p] = t;
            }
            for (int p = 0; p < k; p++) a[p, p] += 1e-9 * (a[p, p] + 1e-12);
            double[,] inv = Inverse(a);
            double[] b = new double[k];
            se = new double[k];
            rms = double.NaN;
            if (inv == null) { for (int p = 0; p < k; p++) { b[p] = double.NaN; se[p] = double.NaN; } return b; }
            for (int p = 0; p < k; p++) { double s = 0; for (int q = 0; q < k; q++) s += inv[p, q] * r[q]; b[p] = s; }
            double res = 0;
            for (int i = 0; i < n; i++)
            {
                double m = 0;
                for (int p = 0; p < k; p++) m += cols[p][i] * b[p];
                res += wgt[i] * (y[i] - m) * (y[i] - m);
            }
            double dof = Math.Max(count - k, 1);
            rms = Math.Sqrt(res / dof);
            for (int p = 0; p < k; p++) se[p] = inv[p, p] > 0 ? rms * Math.Sqrt(inv[p, p]) : double.NaN;
            return b;
        }

        // A free spring's motion from tau 0 with any damping, and a rest it settles to: y = c0 + c1 f1 + c2 f2. Its frequency and damping ratio from a
        // grid, then refined: w, z, c0, c1, c2, the rms left.
        public static double[] Settle(double[] tau, double[] y)
        {
            double bestS = double.MaxValue, bw = 15, bz = 0.3;
            for (int a = 0; a < 80; a++)
            {
                double w = 3.0 * Math.Pow(40.0, a / 79.0);
                for (int b = 0; b < 80; b++)
                {
                    double z = 0.003 * Math.Pow(1000.0, b / 79.0);
                    double s = SettleSse(w, z, tau, y, null);
                    if (s < bestS) { bestS = s; bw = w; bz = z; }
                }
            }
            Func<double[], double> f = x => SettleSse(Math.Exp(x[0]), Math.Exp(x[1]), tau, y, null);
            double best;
            double[] r = Fit.Minimize(f, new double[] { Math.Log(bw), Math.Log(bz) }, 0.05, 600, out best);
            double[] c = new double[3];
            double sse = SettleSse(Math.Exp(r[0]), Math.Exp(r[1]), tau, y, c);
            return new double[] { Math.Exp(r[0]), Math.Exp(r[1]), c[0], c[1], c[2], Math.Sqrt(sse / Math.Max(tau.Length - 5, 1)) };
        }
        static double SettleSse(double w, double z, double[] tau, double[] y, double[] coef)
        {
            if (!(w >= 1 && w <= 400 && z >= 0.001 && z <= 5)) return double.MaxValue;
            int n = tau.Length;
            double[][] cols = new double[3][];
            for (int j = 0; j < 3; j++) cols[j] = new double[n];
            for (int i = 0; i < n; i++)
            {
                double t = tau[i], f1, f2;
                if (Math.Abs(z - 1) < 1e-3) { f1 = Math.Exp(-w * t); f2 = t * Math.Exp(-w * t); }
                else if (z < 1) { double a = z * w, wd = w * Math.Sqrt(1 - z * z), e = Math.Exp(-a * t); f1 = e * Math.Cos(wd * t); f2 = e * Math.Sin(wd * t); }
                else { double q = Math.Sqrt(z * z - 1); f1 = Math.Exp(-w * (z - q) * t); f2 = Math.Exp(-w * (z + q) * t); }
                cols[0][i] = 1; cols[1][i] = f1; cols[2][i] = f2;
            }
            double[] wgt = new double[n];
            for (int i = 0; i < n; i++) wgt[i] = 1;
            double[] se;
            double rms;
            double[] b = Solve(cols, y, wgt, out se, out rms);
            if (double.IsNaN(b[0])) return double.MaxValue;
            double s = 0;
            for (int i = 0; i < n; i++)
            {
                double m = b[0] + b[1] * cols[1][i] + b[2] * cols[2][i];
                s += (y[i] - m) * (y[i] - m);
            }
            if (coef != null) { coef[0] = b[0]; coef[1] = b[1]; coef[2] = b[2]; }
            return s;
        }
    }
}
'@
}

# ---- Reading the log ----
# The log's clock: [hh:mm:ss.mmm] at the start of each line. -From/-To the same (seconds of the day).
function DayTime([string]$text) {
    if (-not $text) { return $null }
    if ($text -notmatch '^(\d{1,2}):(\d\d):(\d\d)(?:\.(\d{1,3}))?$') { throw "'$text' isn't hh:mm:ss or hh:mm:ss.fff" }
    $ms = if ($matches[4]) { [int]($matches[4].PadRight(3, '0')) } else { 0 }
    return [int]$matches[1] * 3600.0 + [int]$matches[2] * 60 + [int]$matches[3] + $ms / 1000.0
}
function Clock([double]$t) {
    $d = $t % 86400.0
    $h = [Math]::Floor($d / 3600); $m = [Math]::Floor(($d - $h * 3600) / 60); $sec = $d - $h * 3600 - $m * 60
    return ('{0:00}:{1:00}:{2:00.0}' -f $h, $m, ([Math]::Floor($sec * 10) / 10)).Replace(',', '.')
}
function Num([string]$text) {
    $v = 0.0
    if ([double]::TryParse($text, [Globalization.NumberStyles]::Float, $inv, [ref]$v) -and -not [double]::IsNaN($v) -and -not [double]::IsInfinity($v)) { return $v }
    return [double]::NaN
}
function Wrap180([double]$d) { $r = $d % 360.0; if ($r -gt 180) { $r -= 360 } elseif ($r -le -180) { $r += 360 }; return $r }
function F([double]$v, [int]$places = 2) { if ([double]::IsNaN($v)) { return '-' }; return $v.ToString('F' + $places, $inv) }
function Pm([double]$v, [double]$se, [int]$places = 2) { $t = F $v $places; if (-not [double]::IsNaN($se)) { $t += ' +-' + (F $se $places) }; return $t }
function Median([double[]]$v) {
    $a = @($v | Where-Object { -not [double]::IsNaN($_) } | Sort-Object)
    if (-not $a.Count) { return [double]::NaN }
    if ($a.Count % 2) { return [double]$a[($a.Count - 1) / 2] }
    return ([double]$a[$a.Count / 2 - 1] + [double]$a[$a.Count / 2]) / 2
}

# recoilrun.cpp's per-frame line, its once-a-second line, its line as a gun's kick is taken out of its data; input.cpp's turn.
$frameRe = '^\[(\d\d):(\d\d):(\d\d)\.(\d{3})\] Recoil: f (\d+) shots (-?\d+) hold (\S+) eye (\S+) drift (\S+) (\S+) kick (\S+) (\S+) (\S+) back (\S+) lift (\S+) look (\S+) (\S+) camera (\S+) (\S+)\s*$'
$secondRe = '^\[(\d\d):(\d\d):(\d\d)\.(\d{3})\] Recoil: (\d+) rounds in the last second \(the (.+?), (two hands|one hand), at the eye (\S+)\): .*; (?:the weapon camera (\S+) degrees (above|below) the written look, (\S+) to the side|the weapon camera not compared)'
$takeRe = '^\[(\d\d):(\d\d):(\d\d)\.(\d{3})\] Recoil: the (.+?)''s own kick taken out of the game''s data: (\S+) up a shot \(x(\S+) the first\), sideways (-?\d+\.\d+)\.\.(-?\d+\.\d+), back (\S+) a second \(x(\S+) firing\), its camera spring (\S+)/(\S+); (\d+) rows \(data ([0-9A-F]{8}), its standing block at ([0-9A-F]{8}); its row ([^)]*)\)'
$turnRe = '^\[(\d\d):(\d\d):(\d\d)\.(\d{3})\] Turn test: the view turned (\d+) degrees (left|right) at once'
# (Names of their own: PowerShell's names don't tell case, and $From is a string parameter, which would keep "none" as '' and match nothing.)
try { $fromDay = DayTime $From; $toDay = DayTime $To } catch { Write-Host "-From/-To: $($_.Exception.Message)"; exit 1 }
function Inside([double]$day) {
    if ($null -ne $fromDay -and $null -ne $toDay -and $toDay -lt $fromDay) { return $day -ge $fromDay -or $day -le $toDay }  # (past midnight)
    return ($null -eq $fromDay -or $day -ge $fromDay) -and ($null -eq $toDay -or $day -le $toDay)
}

$frames = New-Object System.Collections.Generic.List[object]
$seconds = New-Object System.Collections.Generic.List[object]
$turns = New-Object System.Collections.Generic.List[object]
$takes = New-Object System.Collections.Generic.List[object]
$otherRecoil = 0
$lastDay = -1.0; $days = 0
$fs = [IO.File]::Open($Log, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]'ReadWrite, Delete')
$reader = New-Object IO.StreamReader($fs, [Text.Encoding]::Default)
try {
    while ($null -ne ($l = $reader.ReadLine())) {
        if ($l.IndexOf('] Recoil: ', [StringComparison]::Ordinal) -lt 0 -and $l.IndexOf('] Turn test: ', [StringComparison]::Ordinal) -lt 0) { continue }
        if ($l -notmatch '^\[(\d\d):(\d\d):(\d\d)\.(\d{3})\]') { continue }
        $day = [int]$matches[1] * 3600.0 + [int]$matches[2] * 60 + [int]$matches[3] + [int]$matches[4] / 1000.0
        if ($lastDay -ge 0 -and $day -lt $lastDay - 43200) { $days++ }  # (past midnight: the clock goes on)
        $lastDay = $day
        $t = $day + 86400.0 * $days
        if ($l -match $takeRe) {
            # (Any before -To: the data's own numbers are said once a gun a level, often long before.)
            if ($null -eq $toDay -or (Inside $day) -or $day -le $toDay) {
                $takes.Add([pscustomobject]@{ t = $t; gun = $matches[5]; up = (Num $matches[6]); first = (Num $matches[7]); sideMin = (Num $matches[8]); sideMax = (Num $matches[9])
                    decrease = (Num $matches[10]); scale = (Num $matches[11]); spring = (Num $matches[12]); damping = (Num $matches[13]); line = $l })
            }
            continue
        }
        if (-not (Inside $day)) { continue }
        if ($l -match $frameRe) {
            $frames.Add([pscustomobject]@{ t = $t; f = [int64]$matches[5]; shots = [int]$matches[6]; lookY = (Num $matches[16]); lookP = (Num $matches[17])
                camY = (Num $matches[18]); camP = (Num $matches[19]) })
        } elseif ($l -match $secondRe) {
            $seconds.Add([pscustomobject]@{ t = $t; rounds = [int]$matches[5]; gun = $matches[6]; hands = $matches[7]; up = (Num $matches[9]); dir = $matches[10]; side = (Num $matches[11]) })
        } elseif ($l -match $turnRe) {
            $turns.Add([pscustomobject]@{ t = $t; degrees = [int]$matches[5]; right = $matches[6] -eq 'right' })
        } else { $otherRecoil++ }
    }
} finally { $reader.Dispose(); $fs.Dispose() }

$span = if ($From -or $To) { " from $(if ($From) { $From } else { 'the start' }) to $(if ($To) { $To } else { 'the end' })" } else { '' }
Write-Host "recoilfit: $Log$span"
if ($frames.Count -lt 50) {
    Write-Host "  $($frames.Count) per-frame lines ('Recoil: f ...': recoil.txt ""log N"" writes them): too few to fit."
    exit ($problems + 1)
}

# ---- The gun's own numbers: as the log said them when its kick was taken out, else the table's (src\recoilguns.h), else the M416's ----
if (-not $Gun) {
    $named = @($seconds | Group-Object gun | Sort-Object Count -Descending)
    $Gun = if ($named.Count) { $named[0].Name } elseif ($takes.Count) { $takes[$takes.Count - 1].gun } else { 'M416' }
}
$data = [ordered]@{ up = 0.26; first = 2.0; sideMin = -0.3; sideMax = 0.1; decrease = 18.0; scale = 0.1; spring = 250.0; damping = 0.35; rate = 750.0; capUp = 10.0; capSide = 10.0 }
$dataFrom = 'the M416''s (the design''s; the gun not in the table)'
$tableFile = Join-Path $root 'src\recoilguns.h'
if (Test-Path $tableFile) {
    $text = [IO.File]::ReadAllText($tableFile)
    $rowRe = '\{"' + [regex]::Escape($Gun) + '",\s*"[^"]*",\s*(?:nullptr|"[^"]*"),\s*recoil::Kind::\w+,\s*\d+,\s*([\d.]+)f,\s*([\d.]+)f,\s*(\d+),\s*(\d+),\s*\{([^}]*)\}'
    $m = [regex]::Match($text, $rowRe)
    if ($m.Success) {
        $sig = @($m.Groups[5].Value -split ',' | ForEach-Object { Num ($_.Trim() -replace 'f$', '') })
        if ($sig.Count -eq 8) {
            $data.rate = Num $m.Groups[1].Value
            $data.up = $sig[0]; $data.sideMin = $sig[1]; $data.sideMax = $sig[2]; $data.decrease = $sig[3]; $data.first = $sig[4]; $data.scale = $sig[5]; $data.spring = $sig[6]; $data.damping = $sig[7]
            $dataFrom = "src\recoilguns.h's row for the $Gun"
            $caps = [regex]::Match($text.Substring($m.Index + $m.Length), '^\s*,\s*(?:\{\{[^}]*\}(?:\s*,\s*\{[^}]*\}){2}\}\s*,\s*){4}([\d.]+)f\s*,\s*([\d.]+)f')
            if ($caps.Success) { $data.capUp = Num $caps.Groups[1].Value; $data.capSide = Num $caps.Groups[2].Value }
        }
    }
}
$take = @($takes | Where-Object { $_.gun -eq $Gun }) | Select-Object -Last 1
if ($take) {
    foreach ($k in @('up', 'first', 'sideMin', 'sideMax', 'decrease', 'scale', 'spring', 'damping')) { if (-not [double]::IsNaN($take.$k)) { $data[$k] = $take.$k } }
    $dataFrom = "the log's line as its kick was taken out ($(Clock $take.t))" + $(if ($dataFrom -like 'src*') { ", its rate from src\recoilguns.h" } else { '' })
}
$gap = 60.0 / $data.rate
# recoil.h's constants as they are now.
$now = @{ kStringGap = '?'; kDriftLinear = '?'; kShareLeast = '?'; sideSign = '?'; kDriftDampingLeast = '?'; kFiringGaps = '?' }
$headerFile = Join-Path $root 'src\recoil.h'
if (Test-Path $headerFile) {
    $h = [IO.File]::ReadAllText($headerFile)
    foreach ($k in @('kStringGap', 'kShareLeast', 'kDriftDampingLeast', 'kFiringGaps')) { $mm = [regex]::Match($h, "constexpr float $k = ([-\d.]+)f;"); if ($mm.Success) { $now[$k] = $mm.Groups[1].Value } }
    $mm = [regex]::Match($h, 'constexpr bool kDriftLinear = (true|false);'); if ($mm.Success) { $now.kDriftLinear = $mm.Groups[1].Value }
    $mm = [regex]::Match($h, 'float sideSign = ([-\d.]+)f;'); if ($mm.Success) { $now.sideSign = $mm.Groups[1].Value }
}

# ---- The series: the weapon camera less the written look, a frame at a time ----
$all = @($frames | Sort-Object t)
$ok = @($all | Where-Object { -not ([double]::IsNaN($_.lookP) -or [double]::IsNaN($_.camP) -or [double]::IsNaN($_.lookY) -or [double]::IsNaN($_.camY)) })
if ($ok.Count -lt 50) {
    Write-Host "  $($all.Count) per-frame lines, $($ok.Count) with both the look and the camera ('- -': none written or built lately): too few to fit."
    exit ($problems + 1)
}
$t0 = $ok[0].t
$n = $ok.Count
$ft = New-Object double[] $n; $yp = New-Object double[] $n; $yy = New-Object double[] $n; $wgt = New-Object double[] $n
for ($i = 0; $i -lt $n; $i++) { $ft[$i] = $ok[$i].t - $t0; $yp[$i] = $ok[$i].camP - $ok[$i].lookP; $yy[$i] = Wrap180 ($ok[$i].camY - $ok[$i].lookY); $wgt[$i] = 1.0 }
# The rounds: a frame's count went down between it and the frame before; each dated evenly over that time, as recoil.h dates them.
$rtList = New-Object System.Collections.Generic.List[double]
$spanList = New-Object System.Collections.Generic.List[double]  # (the frame each was counted in: its time is known no better than that)
for ($i = 0; $i -lt $all.Count; $i++) {
    $k = $all[$i].shots
    if ($k -le 0) { continue }
    $prev = if ($i -gt 0) { $all[$i - 1].t } else { $all[$i].t - 1.0 / 72 }
    $span1 = [Math]::Min([Math]::Max($all[$i].t - $prev, 0.004), 0.1)
    for ($j = 0; $j -lt $k; $j++) { $rtList.Add($all[$i].t - $span1 + ($j + 0.5) * $span1 / $k - $t0); $spanList.Add($span1) }
}
$rt = $rtList.ToArray(); $rtSpan = $spanList.ToArray()
if ($rt.Count -lt 3) { Write-Host "  $($rt.Count) rounds in the per-frame lines: too few to fit."; exit ($problems + 1) }
# Pulls: a round more than 1.5 gaps and 50 ms after the one before is a pull's first (recoil.h's first shot).
$first = New-Object bool[] $rt.Count
$pulls = New-Object System.Collections.Generic.List[object]
for ($k = 0; $k -lt $rt.Count; $k++) {
    $first[$k] = $k -eq 0 -or ($rt[$k] - $rt[$k - 1]) -gt (1.5 * $gap + 0.05)
    if ($first[$k]) { $pulls.Add([pscustomobject]@{ a = $k; b = $k }) } else { $pulls[$pulls.Count - 1].b = $k }
}
$spacing = Median @(for ($k = 1; $k -lt $rt.Count; $k++) { if (-not $first[$k]) { $rt[$k] - $rt[$k - 1] } })
if ($rt.Count - $pulls.Count -lt 5) {
    # (One round a pull, as a gun set to single fire gives: the rise a round and the first one's multiplier come only as one, and no string ends.)
    Write-Host "  $($rt.Count) rounds in $($pulls.Count) pulls: fewer than 5 after a pull's first (the gun's fire mode single?): too few to fit a string."
    exit ($problems + 1)
}
if ([double]::IsNaN($spacing)) { $spacing = $gap }
# Frames left out: the turn (the look jumps more than 3 degrees in a frame; the camera follows a frame later), up to 1 s after a hole in the lines (a
# burst of pictures written holds the frames up), and anything wild.
$jumps = New-Object System.Collections.Generic.List[object]
for ($i = 1; $i -lt $n; $i++) {
    $d = Wrap180 ($ok[$i].lookY - $ok[$i - 1].lookY)
    if ([Math]::Abs($d) -gt 3) { $jumps.Add([pscustomobject]@{ t = $ft[$i]; d = $d }) }
}
$holes = 0
for ($i = 0; $i -lt $n; $i++) {
    if ([Math]::Abs($yp[$i]) -gt 30 -or [Math]::Abs($yy[$i]) -gt 30) { $wgt[$i] = 0.0 }
    foreach ($j in $jumps) { if ($ft[$i] -ge $j.t - 0.1 -and $ft[$i] -le $j.t + 0.6) { $wgt[$i] = 0.0 } }
}
for ($i = 1; $i -lt $n; $i++) {
    if ($ft[$i] - $ft[$i - 1] -gt 0.25) {
        $holes++
        # (Up to 1 s after it, never into the next pull: a pull with its first rounds left out can't be told round by round.)
        $stop = $ft[$i] + 1.0
        $nextRound = @($rt | Where-Object { $_ -gt $ft[$i] } | Select-Object -First 1)
        if ($nextRound.Count) { $stop = [Math]::Min($stop, $nextRound[0] - 0.2) }
        for ($k = $i; $k -lt $n -and $ft[$k] -le $stop; $k++) { $wgt[$k] = 0.0 }
    }
}
# At rest: 1.5 s or more after any round and not just before one (the camera's round may come a little before the log's frame).
$restP = New-Object System.Collections.Generic.List[double]; $restY = New-Object System.Collections.Generic.List[double]
$k = 0
for ($i = 0; $i -lt $n; $i++) {
    while ($k -lt $rt.Count -and $rt[$k] -le $ft[$i]) { $k++ }
    $after = if ($k -gt 0) { $ft[$i] - $rt[$k - 1] } else { 1e9 }
    $before = if ($k -lt $rt.Count) { $rt[$k] - $ft[$i] } else { 1e9 }
    if ($wgt[$i] -gt 0 -and $after -ge 1.5 -and $before -ge 0.15) { $restP.Add($yp[$i]); $restY.Add($yy[$i]) }
}
$baseP = if ($restP.Count -ge 10) { Median $restP.ToArray() } else { 0.0 }
$baseY = if ($restY.Count -ge 10) { Median $restY.ToArray() } else { 0.0 }
for ($i = 0; $i -lt $n; $i++) { $yp[$i] -= $baseP; $yy[$i] -= $baseY }
$used = 0; foreach ($w in $wgt) { if ($w -gt 0) { $used++ } }
$pullSizes = @($pulls | ForEach-Object { $_.b - $_.a + 1 })
$rateSeen = 1000 * ($ok[$n - 1].t - $ok[0].t)
$fps = if ($rateSeen -gt 0) { ($n - 1) / ($rateSeen / 1000) } else { 0 }
Write-Host ("  {0} frames ({1} a second; {2} used), {3} rounds in {4} pulls ({5}), from {6} to {7}" -f $n, (F $fps 0), $used, $rt.Count, $pulls.Count, ($pullSizes -join ', '), (Clock $ok[0].t), (Clock $ok[$n - 1].t))
Write-Host ("  The {0}: {1}; its rate {2} a minute ({3} ms a round; {4} ms between the rounds seen)" -f $Gun, $dataFrom, (F $data.rate 0), (F (1000 * $gap) 1), (F (1000 * $spacing) 1))
Write-Host ("  The camera off the look at rest ({0} frames): {1} up, {2} to the side (taken off){3}" -f $restP.Count, (F $baseP 3), (F $baseY 3), $(if ($restP.Count -lt 10) { ': too few frames at rest, none taken off' } else { '' }))
if ($holes) { Write-Host "  $holes holes of more than 0.25 s in the lines (a burst of pictures written?): up to 1 s after each left out" }
if ($restP.Count -lt 10) { $problems++ }

# ---- The fit: each law, from several starts (the design's numbers, the damping read as a coefficient, no spring, a slow recovery, other string gaps) ----
$designP = @(($data.up * 0.95), $data.first, 1.25, ($data.decrease * $data.scale), $data.decrease, [Math]::Sqrt([Math]::Max($data.spring, 1)), $data.damping, -0.01)
function Starts {
    $w0 = $designP[5]; $z0 = $designP[6]; $c = $z0 / (2 * $w0)
    $list = @(
        $designP,
        @($designP[0], $designP[1], 1.25, $designP[3], $designP[4], $w0, [Math]::Max($c, 0.003), -0.01),
        @($designP[0], $designP[1], 1.25, $designP[3], $designP[4], 60.0, 0.9, -0.01),
        @($designP[0], 1.0, 0.6, ($designP[4] / 4), ($designP[4] / 4), $w0, 0.2, -0.03),
        @($designP[0], $designP[1], 2.5, ($designP[3] * 3), ($designP[4] * 0.5), ($w0 * 0.7), 0.5, 0.0))
    return , ([double[][]]$list)
}
$cap = $data.capUp
# The recovery while a round left within 2 gaps held to the data's own share of the one after (DecreaseFactor x ShootingRecoilDecreaseScale, as the
# design reads it): free, it trades almost exactly with the string gap (both only say how much has come back by 2 gaps), so it's also fitted free
# once, as a check, and the string gap is told by the fit's sum of squares across a range of it rather than by its slope alone.
$tie = if ($data.scale -gt 0) { $data.scale } else { 0.1 }
function NewProblem([int]$law, [double]$tied, [double]$heldGap) {
    $pr = New-Object RecoilFit.Problem
    $pr.Law = $law; $pr.Gap = $gap; $pr.Cap = $cap; $pr.Tie = $tied
    $pr.Rt = $rt; $pr.Ft = $ft; $pr.Y = $yp; $pr.Wgt = $wgt; $pr.First = $first
    if ($tied -gt 0) { $pr.Active[3] = $false }
    if ($heldGap -ge 0) { $pr.Active[2] = $false; $pr.Held[2] = $heldGap }
    return $pr
}
$fits = @{}
$watch = [Diagnostics.Stopwatch]::StartNew()
foreach ($law in @(0, 1)) {
    $sse = 0.0
    $pr = NewProblem $law $tie -1
    $fp = [RecoilFit.Fit]::Best($pr, (Starts), 1200, [ref]$sse)
    $fits[$law] = [pscustomobject]@{ law = $law; p = $fp; sse = $sse; rms = [Math]::Sqrt($sse / [Math]::Max($used - 7, 1)) }
}
$sst = 0.0; for ($i = 0; $i -lt $n; $i++) { $sst += $wgt[$i] * $yp[$i] * $yp[$i] }
$best = if ($fits[0].sse -le $fits[1].sse) { $fits[0] } else { $fits[1] }
$other = if ($best.law -eq 0) { $fits[1] } else { $fits[0] }
$gapFree = $best.p[2]

# Each round's own rise and sideways pick, pull by pull (Model.PullSolve): from the pull's first round to $after gaps past its last, the motion the pulls
# before left taken off. A round with no frame of its own is left unknown (the pulls after it take the fit's own rise for it, and no pick).
$t0Run = [Math]::Min(0.0, $ft[0])
function Increments($pp, [double]$after) {
    $r = [pscustomobject]@{ p = (New-Object double[] $rt.Count); y = (New-Object double[] $rt.Count); seP = (New-Object double[] $rt.Count); seY = (New-Object double[] $rt.Count) }
    $workP = New-Object double[] $rt.Count; $workY = New-Object double[] $rt.Count
    for ($k = 0; $k -lt $rt.Count; $k++) {
        $r.p[$k] = [double]::NaN; $r.y[$k] = [double]::NaN; $r.seP[$k] = [double]::NaN; $r.seY[$k] = [double]::NaN
        $workP[$k] = $pp[0] * $(if ($first[$k]) { $pp[1] } else { 1.0 }); $workY[$k] = 0.0
    }
    foreach ($pl in $pulls) {
        $a = $pl.a; $b = $pl.b; $count = $b - $a + 1
        $since = $rt[$a] + $pp[7] - 0.02; $until = $rt[$b] + $pp[7] + $after * $gap
        $sp = New-Object double[] $count; $sy = New-Object double[] $count
        $gp = [RecoilFit.Model]::PullSolve($pp, $best.law, $gap, $cap, $rt, $a, $b, $workP, $ft, $yp, $wgt, $since, $until, $sp)
        $gy = [RecoilFit.Model]::PullSolve($pp, $best.law, $gap, $data.capSide, $rt, $a, $b, $workY, $ft, $yy, $wgt, $since, $until, $sy)
        for ($c = 0; $c -lt $count; $c++) {
            if (-not [double]::IsNaN($gp[$c])) { $r.p[$a + $c] = $gp[$c]; $r.seP[$a + $c] = $sp[$c]; $workP[$a + $c] = $gp[$c] }
            if (-not [double]::IsNaN($gy[$c])) { $r.y[$a + $c] = $gy[$c]; $r.seY[$a + $c] = $sy[$c]; $workY[$a + $c] = $gy[$c] }
        }
    }
    return $r
}
# The string gap. Each round's own rise solved for every try of the rest (the shares' own scatter, 90-100% a round, otherwise trades with the gap, and
# rises solved once for other numbers pull it too): the spring, its delay and the recovery fitted again at each gap from 1 to 3 (recoil.h's own among
# them), and once with the gap free in them. The range the run can't tell apart: those within twice the best's sum of squares. (The rounds are known
# only to the frame they were counted in: on runs made from the design's own law at 72 frames a second the best came out anywhere from 1.1 to 2 gaps
# with the true one, 1.25, at up to about one and a half times its sum of squares, while 1.0 and 2.5 were three times it and more. A few per cent was
# far tighter than the fit can tell.) Below one gap the target would come back between the rounds of a string, and a round's own rise can't then be
# told from what came back before the next: there the rise a round is fitted, and the best below one gap is set against the best at one gap or more
# (better by 10%: it comes back inside a string).
$gapApart = 2.0
$gapNow = Num $now.kStringGap
if ([double]::IsNaN($gapNow)) { $gapNow = 1.25 }
$pullA = [int[]]@($pulls | ForEach-Object { $_.a }); $pullB = [int[]]@($pulls | ForEach-Object { $_.b })
function JointProblem([double]$heldGap) {
    $pr = NewProblem $best.law $tie $heldGap
    $pr.Joint = $true; $pr.PullA = $pullA; $pr.PullB = $pullB
    $pr.Active[0] = $false; $pr.Active[1] = $false; $pr.Held[0] = $best.p[0]; $pr.Held[1] = $best.p[1]
    return $pr
}
$gapsTried = @(1.0, 1.1, 1.2, 1.25, 1.3, 1.4, 1.5, 1.75, 2.0, 2.5, 3.0)
if ($gapNow -ge 1.0 -and $gapNow -le 3.0 -and @($gapsTried | Where-Object { [Math]::Abs($_ - $gapNow) -lt 1e-6 }).Count -eq 0) { $gapsTried = @($gapsTried + $gapNow | Sort-Object) }
$byGap = @()
foreach ($g in $gapsTried) {
    $sseG = 0.0
    $start = [double[]]$best.p.Clone(); $start[2] = $g
    [void][RecoilFit.Fit]::Best((JointProblem $g), [double[][]]@(, $start), 400, [ref]$sseG)
    $byGap += [pscustomobject]@{ g = $g; sse = $sseG }
}
$gridBest = $byGap | Sort-Object sse | Select-Object -First 1
$sseRoundsFree = 0.0
$start = [double[]]$best.p.Clone(); $start[2] = [Math]::Max($gridBest.g, 1.0)
$pRoundsFree = [RecoilFit.Fit]::Best((JointProblem -1), [double[][]]@(, $start), 800, [ref]$sseRoundsFree)
$freeCounts = $pRoundsFree[2] -ge 1.0 -and $sseRoundsFree -lt $gridBest.sse
$gapRounds = if ($freeCounts) { $pRoundsFree[2] } else { $gridBest.g }
$leastRounds = if ($freeCounts) { $sseRoundsFree } else { $gridBest.sse }
$within = @(@($byGap) + $(if ($freeCounts) { @([pscustomobject]@{ g = $pRoundsFree[2]; sse = $sseRoundsFree }) } else { @() }) | Where-Object { $_.sse -le $gapApart * $leastRounds })
$below = @()
foreach ($g in @(0.5, 0.75, 0.9, 1.0, 1.25, 1.5, 2.0)) {
    $sseG = 0.0
    $start = [double[]]$best.p.Clone(); $start[2] = $g
    $pG = [RecoilFit.Fit]::Best((NewProblem $best.law $tie $g), [double[][]]@(, $start), 500, [ref]$sseG)
    $below += [pscustomobject]@{ g = $g; sse = $sseG; p = $pG }
}
$below += [pscustomobject]@{ g = $gapFree; sse = $best.sse; p = $best.p }  # (the fit with the gap free, either side of one)
$underOne = @($below | Where-Object { $_.g -lt 1.0 } | Sort-Object sse | Select-Object -First 1)
$overOne = @($below | Where-Object { $_.g -ge 1.0 } | Sort-Object sse | Select-Object -First 1)
$inside = $underOne.Count -and $overOne.Count -and $underOne[0].sse -lt 0.9 * $overOne[0].sse
$insideRatio = if ($underOne.Count -and $overOne.Count -and $underOne[0].sse -gt 0) { $overOne[0].sse / $underOne[0].sse } else { [double]::NaN }
# The rise a round with nothing coming back inside a string (the best fit with the gap at one or more) against the data's own (IncPerShot times the
# shares' mean). With a linear law a recovery between the rounds of a string is the same curve as a smaller rise a round, so the rise fitted free
# can't show it: the comparison above then says "no" whatever the game does. Under nine tenths of the data's, the run is fitted again with the rise
# held at the data's and the gap from 0 (the recovery from the round itself) upward, then once with the gap free from the best of those. It comes
# back inside a string when that fits better below one gap (by 10%) and its first round is then the data's own multiple of the rise (within 15%).
# A pull's first round rises before anything can come back: rises that are simply smaller than the data's (their shares' mean lower) show in the
# first round too, and are not taken for a recovery.
$shareNow = Num $now.kShareLeast
if ([double]::IsNaN($shareNow)) { $shareNow = 0.9 }
$riseData = $data.up * (1 + $shareNow) / 2
$riseNet = if ($overOne.Count -and $overOne[0].p) { $overOne[0].p[0] } else { [double]::NaN }
$riseLow = $riseData -gt 0 -and $riseNet -lt 0.9 * $riseData
$firstHeld = [double]::NaN; $firstOk = $false
if ($riseLow) {
    $heldRise = @()
    foreach ($g in @(0.0, 0.25, 0.5, 0.75, 0.9, 1.0, 1.25, 1.5, 2.0)) {
        $sseG = 0.0
        $prRise = NewProblem $best.law $tie $g
        $prRise.Active[0] = $false; $prRise.Held[0] = $riseData
        $start = [double[]]$best.p.Clone(); $start[0] = $riseData; $start[2] = $g
        $pG = [RecoilFit.Fit]::Best($prRise, [double[][]]@(, $start), 500, [ref]$sseG)
        $heldRise += [pscustomobject]@{ g = $g; sse = $sseG; first = $pG[1] }
    }
    $underHeld = @($heldRise | Where-Object { $_.g -lt 1.0 } | Sort-Object sse | Select-Object -First 1)[0]
    $overHeld = @($heldRise | Where-Object { $_.g -ge 1.0 } | Sort-Object sse | Select-Object -First 1)[0]
    $sseG = 0.0
    $prRise = NewProblem $best.law $tie -1
    $prRise.Active[0] = $false; $prRise.Held[0] = $riseData
    $start = [double[]]$best.p.Clone(); $start[0] = $riseData; $start[2] = [Math]::Max($underHeld.g, 0.02)  # (the search can't start on the gap's edge)
    $pRise = [RecoilFit.Fit]::Best($prRise, [double[][]]@(, $start), 800, [ref]$sseG)
    if ($pRise[2] -lt 1.0 -and $sseG -lt $underHeld.sse) { $underHeld = [pscustomobject]@{ g = ([Math]::Round($pRise[2] * 100) / 100); sse = $sseG; first = $pRise[1] } }
    $firstHeld = $underHeld.first
    $firstOk = [Math]::Abs($firstHeld - $data.first) -le 0.15 * $data.first
    $insideRatio = if ($underHeld.sse -gt 0) { $overHeld.sse / $underHeld.sse } else { [double]::NaN }
    $inside = $underHeld.sse -lt 0.9 * $overHeld.sse -and $firstOk
    if ($inside) { $underOne = @($underHeld) }
}
# The gap the numbers reported are fitted with: recoil.h's own while the run can't tell it from the best (within the range), else the best.
$nowWithin = @($within | Where-Object { [Math]::Abs($_.g - $gapNow) -lt 1e-6 }).Count -gt 0
$nowTried = @($byGap | Where-Object { [Math]::Abs($_.g - $gapNow) -lt 1e-6 }) | Select-Object -First 1
$nowRatio = if ($nowTried) { $nowTried.sse / [Math]::Max($leastRounds, 1e-12) } else { [double]::NaN }
$gapHeld = if ($inside) { $underOne[0].g } elseif ($nowWithin) { $gapNow } else { [Math]::Round($gapRounds * 20) / 20 }
$sseHeld = 0.0
$prHeld = NewProblem $best.law $tie $gapHeld
$start = [double[]]$best.p.Clone(); $start[2] = $gapHeld
$pHeld = [RecoilFit.Fit]::Best($prHeld, [double[][]]@($start, $designP), 1200, [ref]$sseHeld)
$p = $pHeld; $se = [RecoilFit.Fit]::Errors($prHeld, $pHeld, $used)
$heldRms = [Math]::Sqrt($sseHeld / [Math]::Max($used - 6, 1))
# The check: the recovery while firing free (the gap held as above).
$sseFree = 0.0
$prFree = NewProblem $best.law 0 $gapHeld
$pFree = [RecoilFit.Fit]::Best($prFree, [double[][]]@($p, $designP), 1200, [ref]$sseFree)
$r2 = if ($sst -gt 0) { 1 - $sseHeld / $sst } else { [double]::NaN }
$lawWord = @('linear (degrees a second)', 'a share a second')
Write-Host ("  The fit ({0:F1} s): linear's rms {1} degrees, a share's {2}; {3} of the camera's motion explained" -f $watch.Elapsed.TotalSeconds, (F $fits[0].rms 3), (F $fits[1].rms 3), $(if ([double]::IsNaN($r2)) { '-' } else { (100 * $r2).ToString('F1', $inv) + '%' }))
if ($sst -le 0 -or $p[0] -lt 0.02) { Write-Host '  The camera hardly moved with the rounds: the game''s own kick isn''t in it (recoil.txt "game 1"?), nothing to fit.'; $problems++ }
elseif ($r2 -lt 0.9) { Write-Host '  Less than 90% explained: the model doesn''t hold well; look at the per-frame lines before setting anything from these numbers.'; $problems++ }

# Each round's own, by the numbers reported, and each pull's peak and when it's back within 0.05 degrees (before the next pull).
# (A share's recovery is a share of each round's own, so its rounds are told until the next pull; a linear one's only until its target could be back
# at nothing, half a gap past its start.)
$incs = Increments $p $(if ($best.law -eq 1) { $gapHeld + 2.0 } else { [Math]::Max($gapHeld + 0.5, 0.6) })
$incP = $incs.p; $incY = $incs.y; $seP = $incs.seP; $seY = $incs.seY
$pullRows = @()
foreach ($pl in $pulls) {
    $a = $pl.a; $b = $pl.b; $count = $b - $a + 1
    $row = [pscustomobject]@{ rounds = $count; at = $rt[$a] + $t0; firstRise = $incP[$a]; laterRise = [double]::NaN; peak = [double]::NaN; peakAt = [double]::NaN; home = [double]::NaN; side = [double]::NaN }
    if ($count -gt 1) { $later1 = @($incP[($a + 1)..$b] | Where-Object { -not [double]::IsNaN($_) }); if ($later1.Count) { $row.laterRise = ($later1 | Measure-Object -Average).Average } }
    $sideKnown = @($incY[$a..$b] | Where-Object { -not [double]::IsNaN($_) })
    if ($sideKnown.Count) { $row.side = ($sideKnown | Measure-Object -Average).Average }
    $next = @($pulls | Where-Object { $_.a -gt $b } | Select-Object -First 1)
    $until = if ($next.Count) { $rt[$next[0].a] } else { $ft[$n - 1] }
    $pk = -1e9; $pkAt = 0.0; $backIn = [double]::NaN
    for ($i = 0; $i -lt $n; $i++) {
        if ($wgt[$i] -le 0 -or $ft[$i] -lt $rt[$a] -or $ft[$i] -ge $until) { continue }
        if ($yp[$i] -gt $pk) { $pk = $yp[$i]; $pkAt = $ft[$i] }
    }
    for ($i = 0; $i -lt $n; $i++) {
        if ($wgt[$i] -le 0 -or $ft[$i] -le $pkAt -or $ft[$i] -ge $until) { continue }
        if ([Math]::Abs($yp[$i]) -le 0.05) { $backIn = $ft[$i] - $rt[$b]; break }
    }
    if ($pk -gt -1e9) { $row.peak = $pk; $row.peakAt = $pkAt - $rt[$b] }
    $row.home = $backIn
    $pullRows += $row
}

# ---- The settle after the last round: from where the fit's target is back at nothing, a free spring's motion. With a linear law only: its target
# stops dead at nothing and the camera swings on about it, a swing of its own to fit. A share's target comes back to nothing smoothly: by the time
# it's there the swing has died, and what's left to fit is the target's own tail (slower and heavier damped than the spring: it would "disagree") ----
$last = $pulls[$pulls.Count - 1]
$targets = New-Object double[] $n
[void][RecoilFit.Model]::Run($p, $best.law, $gap, $cap, $rt, $first, $null, $ft, $targets, $t0Run)
$peakTarget = 0.0
for ($i = 0; $i -lt $n; $i++) { if ($ft[$i] -ge $rt[$last.a] -and [Math]::Abs($targets[$i]) -gt $peakTarget) { $peakTarget = [Math]::Abs($targets[$i]) } }
$settleFrom = [double]::NaN
for ($i = 0; $i -lt $n; $i++) {
    if ($ft[$i] -gt $rt[$last.b] -and [Math]::Abs($targets[$i]) -le 0.01 * $peakTarget) { $settleFrom = $ft[$i]; break }
}
$settle = $null
if ($best.law -eq 0 -and -not [double]::IsNaN($settleFrom)) {
    $sIdx = @(for ($i = 0; $i -lt $n; $i++) { if ($ft[$i] -ge $settleFrom -and $ft[$i] -le $settleFrom + 2.5) { if ($wgt[$i] -le 0) { break }; $i } })
    if ($sIdx.Count -ge 25) {
        $tau = New-Object double[] $sIdx.Count; $sy = New-Object double[] $sIdx.Count
        for ($j = 0; $j -lt $sIdx.Count; $j++) { $tau[$j] = $ft[$sIdx[$j]] - $settleFrom; $sy[$j] = $yp[$sIdx[$j]] }
        $sv = [RecoilFit.Lsq]::Settle($tau, $sy)
        $amp = [Math]::Sqrt($sv[3] * $sv[3] + $sv[4] * $sv[4])
        $settle = [pscustomobject]@{ w = $sv[0]; z = $sv[1]; rms = $sv[5]; amp = $amp; from = $settleFrom + $t0; count = $sIdx.Count; span = $tau[$tau.Count - 1] }
    }
}

# ---- The turn: which way the log's yaw grows for a turn to the right ----
$rightSign = [double]::NaN; $turnSaid = 'no turn in these lines: aim.h''s own word taken (the yaw grows turning right)'
if ($jumps.Count) {
    $j = $jumps[0]
    $said = @($turns | Where-Object { [Math]::Abs(($_.t - $t0) - $j.t) -le 1.5 }) | Select-Object -First 1
    if ($said) {
        $rightSign = [Math]::Sign($j.d) * $(if ($said.right) { 1 } else { -1 })
        $turnSaid = ('the turn {0} {1} at {2}: the log''s yaw went {3} degrees' -f $said.degrees, $(if ($said.right) { 'right' } else { 'left' }), (Clock ($j.t + $t0)), (F $j.d 1))
    } else {
        $rightSign = [Math]::Sign($j.d)
        $turnSaid = ('a look jump of {0} degrees at {1} with no "Turn test" line: taken as the probe''s turn to the right' -f (F $j.d 1), (Clock ($j.t + $t0)))
    }
    $disagree = @($jumps | Where-Object { $tt = $_; $s2 = @($turns | Where-Object { [Math]::Abs(($_.t - $t0) - $tt.t) -le 1.5 }) | Select-Object -First 1
        $s2 -and ([Math]::Sign($tt.d) * $(if ($s2.right) { 1 } else { -1 })) -ne $rightSign })
    if ($disagree.Count) { $turnSaid += "; $($disagree.Count) other turns disagree"; $problems++ }
} else { $rightSign = 1.0 }

# ---- The report ----
function Row([string]$what, [string]$fitted, [string]$design, [string]$nowText, [string]$sets) {
    Write-Host ('  {0,-30} {1,-28} {2,-34} {3,-28} {4}' -f $what, $fitted, $design, $nowText, $sets)
}
$names = @('rise a round', 'first round x', 'string gap', 'recovery firing', 'recovery after', 'spring', 'damping ratio', 'delay')
$bounds = @(@(1e-4, 20), @(0.05, 20), @(0, 4), @(1e-4, 2000), @(1e-4, 2000), @(1, 400), @(0.002, 4), @(-0.1, 0.05))
$byRatio = @($true, $true, $false, $true, $true, $true, $true, $false)  # (those searched by their logarithm: at an edge within 5% of it; the others within 1% of their range)
$atBound = @(for ($k = 0; $k -lt 8; $k++) {
        $lo = $bounds[$k][0]; $hi = $bounds[$k][1]
        if ($byRatio[$k]) { $p[$k] -le 1.05 * $lo -or $p[$k] -ge $hi / 1.05 } else { ($p[$k] - $lo) -le 0.01 * ($hi - $lo) -or ($hi - $p[$k]) -le 0.01 * ($hi - $lo) }
    })
$later = @(for ($k = 0; $k -lt $rt.Count; $k++) { if (-not $first[$k] -and -not [double]::IsNaN($incP[$k])) { $incP[$k] } })
$firsts = @(for ($k = 0; $k -lt $rt.Count; $k++) { if ($first[$k] -and -not [double]::IsNaN($incP[$k])) { $incP[$k] } })
$laterSe = @(for ($k = 0; $k -lt $rt.Count; $k++) { if (-not $first[$k] -and -not [double]::IsNaN($seP[$k])) { $seP[$k] } })
# (Each later round's place in its frame, as an error of its rise: half the frame it was counted in times the slope its own rise makes, the rise
# over the gap between rounds. That is the camera's slope in a string with nothing coming back inside it; with something coming back the camera's
# own slope is less, the rise's own still counts.)
$laterDate = @(for ($k = 0; $k -lt $rt.Count; $k++) { if (-not $first[$k] -and -not [double]::IsNaN($incP[$k])) { 0.5 * $rtSpan[$k] * [Math]::Abs($incP[$k]) / $spacing } })
$sides = @(for ($k = 0; $k -lt $rt.Count; $k++) { if (-not [double]::IsNaN($incY[$k])) { $incY[$k] } })

Write-Host ''
Write-Host ("  pull  rounds  at          first round's rise  the later ones'  peak (s after its last)  back within 0.05 deg  sideways a round")
foreach ($r in $pullRows) {
    Write-Host ('  {0,-5} {1,-7} {2,-11} {3,-19} {4,-16} {5,-24} {6,-21} {7}' -f ([array]::IndexOf($pullRows, $r) + 1), $r.rounds, (Clock $r.at), (F $r.firstRise 3), (F $r.laterRise 3),
        ('{0} ({1})' -f (F $r.peak 2), (F $r.peakAt 2)), $(if ([double]::IsNaN($r.home)) { 'not in its time' } else { (F $r.home 2) + ' s' }), (F $r.side 3))
}
Write-Host ''
Write-Host ("  The game's own kick in the camera, the $Gun; the best law {0}, the string gap held at {1}: {2} of the motion explained (rms {3} degrees)" -f $lawWord[$best.law], (F $gapHeld 2), $(if ([double]::IsNaN($r2)) { '-' } else { (100 * $r2).ToString('F1', $inv) + '%' }), (F $heldRms 3))
Row 'what' 'fitted (+- about)' 'the design (the data)' 'recoil.h now' 'would set'
Row '----' '-----------------' '---------------------' '------------' '---------'
# The rise a round, against the data's own: under nine tenths of it with nothing coming back inside a string (the gap at one or more), something
# does, or the rise isn't the data's; over eleven tenths, it isn't the data's. Each is to look at, and counted.
$riseSet = '- (as the data)'
if ($riseLow) {
    $riseSet = if ($inside) { 'look: ' + (F (($riseData - $riseNet) / $gap) 1) + ' deg/s comes back inside the string' }
               else { 'look: under the data''s ' + (F $riseData 3) + ' (its shares'' mean ' + (F ($riseNet / $data.up) 2) + '?), not a recovery inside the string' }
    $problems++
} elseif ($riseNet -gt 1.1 * $riseData) { $riseSet = 'not the data''s ' + (F $riseData 3) + ': look'; $problems++ }
Row 'rise a round, after the first' ((Pm $p[0] $se[0] 3) + ' deg' + $(if ($riseLow -and $inside) { '; ' + (F $riseNet 3) + ' with none back in a string' } else { '' })) ('{0} x {1}-1.0 = {2}' -f (F $data.up 2), $now.kShareLeast, (F $riseData 3)) '-' $riseSet
# Its spread: each later round's own rise as a share of the data's IncPerShot. The spread seen is the shares' own and each round's measuring error
# together. A round's error is its fit's own (from the fit of each pull) and its place in its frame (a round is known only to the frame it was
# counted in: dated half a frame off, its rise is off by that much of the camera's slope there, and the round after it by as much the other way;
# at 72 frames a second that alone is some 8% of a round's rise, as much as the 90-100% looked for). The error's part taken off, the rest is a
# uniform range's (its width the root of 12 times its variance), about the shares' mean. It's told only when the spread seen is twice each round's
# error or more (the lesser of its plain variance and the one its 10-90% range gives, so a stray round doesn't make a spread): else "keep".
$shareSet = 'keep'
if ($later.Count -ge 5 -and $data.up -gt 0) {
    $sh = @($later | ForEach-Object { $_ / $data.up } | Sort-Object)
    $nS = $sh.Count; $lo = [double]$sh[0]; $hi = [double]$sh[$nS - 1]
    $mean = ($sh | Measure-Object -Average).Average
    $var = (($sh | ForEach-Object { ($_ - $mean) * ($_ - $mean) }) | Measure-Object -Sum).Sum / ($nS - 1)
    $fitVar = (($laterSe | ForEach-Object { ($_ / $data.up) * ($_ / $data.up) }) | Measure-Object -Average).Average
    $dateVar = (($laterDate | ForEach-Object { ($_ / $data.up) * ($_ / $data.up) }) | Measure-Object -Average).Average
    $noiseVar = $fitVar + $dateVar
    $noise = [Math]::Sqrt($noiseVar)
    $p10 = [double]$sh[[int][Math]::Floor(0.1 * ($nS - 1))]; $p90 = [double]$sh[[int][Math]::Ceiling(0.9 * ($nS - 1))]
    $var = [Math]::Min($var, (($p90 - $p10) / 2.56) * (($p90 - $p10) / 2.56))  # (a bell's 10-90% range is 2.56 of its standard deviation)
    $width = [Math]::Sqrt(12 * [Math]::Max($var - $noiseVar, 0))
    $aHat = $mean - $width / 2; $bHat = $mean + $width / 2
    # (Told only when the spread seen is twice the error or more: else the spread is mostly the error's, and the error is only known roughly.)
    $resolved = $noiseVar -lt 0.25 * $var
    $least = if ($bHat -gt 0) { $aHat / $bHat } else { [double]::NaN }
    if ($resolved -and -not [double]::IsNaN($least)) { $shareSet = 'kShareLeast = ' + (F ([Math]::Round($least, 2)) 2) + 'f' } else { $shareSet = 'keep (the error is as big as the spread)' }
    Row ('its spread ({0} rounds)' -f $nS) ('{0}-{1} of {2} (10-90%: {3}-{4})' -f (F $lo 2), (F $hi 2), (F $data.up 2), (F $p10 2), (F $p90 2)) ('{0}-1.00 (uniform)' -f $now.kShareLeast) ('kShareLeast = ' + $now.kShareLeast + 'f') $shareSet
    Row '  its error taken off' ('{0}-{1}; each round +-{2}' -f (F $aHat 2), (F $bHat 2), (F $noise 3)) ('(its fit +-{0}, its place in its frame +-{1})' -f (F ([Math]::Sqrt($fitVar)) 3), (F ([Math]::Sqrt($dateVar)) 3)) '' $(if ($resolved) { 'its least as a share of its most: ' + (F $least 2) } else { 'not resolved' })
} else { Row 'its spread' 'too few later rounds' '0.90-1.00' ('kShareLeast = ' + $now.kShareLeast + 'f') 'keep'; $problems++ }
# The first round.
$fDirect = if ($firsts.Count -and $later.Count) { (Median $firsts) / (Median $later) } else { [double]::NaN }
Row 'a pull''s first round' ('x' + (Pm $p[1] $se[1] 2) + '; round by round x' + (F $fDirect 2)) ('x' + (F $data.first 2) + ' (FirstShotRecoilMultiplier)') '(the data''s own)' $(if ([Math]::Abs($p[1] - $data.first) -le 0.15 * $data.first) { '- (as the data)' } else { 'the first shot isn''t the data''s x' + (F $data.first 2) + ': look' })
# The recovery.
$ratio = if ($best.sse -gt 0) { $other.sse / $best.sse } else { 1.0 }
$linearSet = if ($ratio -lt 1.05) { 'not told apart: keep' } else { 'kDriftLinear = ' + $(if ($best.law -eq 0) { 'true' } else { 'false' }) }
Row 'recovery: its law' ($(if ($best.law -eq 0) { 'linear' } else { 'a share' }) + " (the other's fit x" + (F $ratio 2) + ')') 'linear (degrees a second)' ('kDriftLinear = ' + $now.kDriftLinear) $linearSet
# The string gap: first whether anything comes back inside a string (the best below one gap against the best at one or more: the rise a round fitted,
# or held at the data's when it came out under it), then the range the run can't tell apart (each round's own rise solved at each gap: within twice
# the best's sum of squares), the gap the numbers here were fitted with, and the free fit's own. recoil.h's own gap is kept while it's in that
# range; outside it (twice as bad as the best, or worse) the best is to set.
$gLo = ($within | Measure-Object g -Minimum).Minimum; $gHi = ($within | Measure-Object g -Maximum).Maximum
$insideWord = if ($riseLow -and $inside) { 'yes: the rise held at the data''s, below one gap fits x' + (F $insideRatio 2) + ' better' }
              elseif ($riseLow -and -not $firstOk -and $insideRatio -gt 1.0 / 0.9) { 'no: the first round is low too (x' + (F $firstHeld 2) + ' of the data''s rise)' }
              elseif ($riseLow) { 'no: the rise held at the data''s, below one gap fits x' + (F (1 / [Math]::Max($insideRatio, 1e-9)) 2) + ' worse' }
              elseif ($inside) { 'yes: below one gap fits x' + (F $insideRatio 2) + ' better' }
              else { 'no: below one gap fits x' + (F (1 / [Math]::Max($insideRatio, 1e-9)) 2) + ' worse' }
Row 'comes back inside a string' $insideWord ('no (kStringGap above 1: ' + (F (1000 * $spacing) 0) + ' ms between rounds)') ('kStringGap = ' + $now.kStringGap + 'f') $(if ($inside) { 'kStringGap = ' + (F $gapHeld 2) + 'f: look at the per-frame lines first' } else { '-' })
$gapRange = if ($within.Count -and -not $inside) { '{0}-{1} gaps ({2}-{3} ms)' -f (F $gLo 2), (F $gHi 2), (F (1000 * $gLo * $gap) 0), (F (1000 * $gHi * $gap) 0) } else { '-' }
$gapSet = if ($inside) { '(above)' }
          elseif ($gapHeld -eq $gapNow) { 'keep kStringGap = ' + $now.kStringGap + 'f (not told apart between ' + (F $gLo 2) + ' and ' + (F $gHi 2) + ')' }
          elseif ([double]::IsNaN($nowRatio)) { 'kStringGap = ' + (F $gapHeld 2) + 'f (the best; recoil.h''s ' + (F $gapNow 2) + ' is outside those tried)' }
          else { 'kStringGap = ' + (F $gapHeld 2) + 'f (the best; recoil.h''s ' + (F $gapNow 2) + ' fits x' + (F $nowRatio 2) + ' worse)' }
if (-not $inside -and $within.Count -and ($gLo -le 1.0 -or $gHi -ge 3.0)) { $gapSet += '; the range runs to the edge of those tried: look' }
Row 'recovery starts after' ((F $gapHeld 2) + ' gaps held (' + (F (1000 * $gapHeld * $gap) 0) + ' ms)') ('1.25 gaps (' + (F (1000 * 1.25 * $gap) 0) + ' ms)') ('kStringGap = ' + $now.kStringGap + 'f') $gapSet
Row '  not told apart from the best' $gapRange ('the best ' + (F $gapRounds 2) + ' (rounds solved at each; within x' + (F $gapApart 0) + ' of its sum of squares)') '' '-'
Write-Host ('  {0,-30} {1}' -f '  (the sum of squares by gap)', (($byGap | ForEach-Object { '{0} x{1}' -f (F $_.g 2), (F ($_.sse / [Math]::Max($leastRounds, 1e-12)) 2) }) -join ', '))
$unit = if ($best.law -eq 0) { 'deg/s' } else { '/s' }
$freeShare = if ($pFree[4] -gt 0) { $pFree[3] / $pFree[4] } else { [double]::NaN }
$freeGain = if ($sseFree -gt 0) { $sseHeld / $sseFree } else { 1.0 }
Row ('recovery within ' + $now.kFiringGaps + ' gaps') ((F $p[3] 1) + " $unit (held at x" + (F $tie 2) + ' of after)') ((F ($data.decrease * $data.scale) 1) + ' (x' + (F $data.scale 2) + ' ShootingRecoilDecreaseScale)') ('kFiringGaps = ' + $now.kFiringGaps + 'f') '-'
Row '  fitted free (a check)' ('x' + (F $freeShare 2) + ' of after; its fit x' + (F $freeGain 2) + ' better') '' '' $(if ($freeGain -gt 1.25) { 'the data''s share doesn''t hold: look' } else { '- (the data''s share holds)' })
Row 'recovery after' ((Pm $p[4] $se[4] 1) + " $unit") ((F $data.decrease 1) + ' (DecreaseFactor)') '(the data''s own)' $(if ([Math]::Abs($p[4] - $data.decrease) -le 0.2 * $data.decrease) { '- (as the data)' } else { 'not the data''s ' + (F $data.decrease 0) + ': look' })
# The spring.
$w0 = [Math]::Sqrt([Math]::Max($data.spring, 1))
Row 'camera spring' ((Pm $p[5] $se[5] 1) + ' rad/s, ' + (F ($p[5] / (2 * [Math]::PI)) 2) + ' Hz') ('root of ' + (F $data.spring 0) + ' = ' + (F $w0 1) + ' rad/s, ' + (F ($w0 / (2 * [Math]::PI)) 2) + ' Hz') 'driftW = sqrt(spring)' $(if ([Math]::Abs($p[5] - $w0) -le 0.15 * $w0) { '- (its frequency is the root of its constant)' } else { 'not the root of its constant: look' })
$asRatio = $data.damping; $asCoeff = $data.damping / (2 * $w0)
$dampWord = if ([Math]::Abs($p[6] - $asRatio) -le 0.3 * $asRatio) { 'SpringDamping is a ratio (as recoil.h takes it)' }
            elseif ([Math]::Abs($p[6] - $asCoeff) -le 0.3 * $asCoeff + 0.003) { 'SpringDamping is c in x'''' + c x'' + k x: the ratio is c / (2 root k)' }
            else { 'neither: the ratio is ' + (F $p[6] 3) + ' (x' + (F ($p[6] / [Math]::Max($data.damping, 1e-6)) 2) + ' of SpringDamping)' }
Row 'its damping ratio' (Pm $p[6] $se[6] 3) ((F $asRatio 2) + ' as a ratio; ' + (F $asCoeff 3) + ' as c') ('kDriftDampingLeast = ' + $now.kDriftDampingLeast + 'f') $dampWord
if ($settle) {
    $agree = [Math]::Abs($settle.w - $p[5]) -le 0.2 * $p[5] -and [Math]::Abs($settle.z - $p[6]) -le 0.3 * $p[6] + 0.02
    Row 'the settle alone' ((F $settle.w 1) + ' rad/s, ratio ' + (F $settle.z 3)) ('{0} frames over {1} s from {2}' -f $settle.count, (F $settle.span 1), (Clock $settle.from)) ('its swing ' + (F $settle.amp 2) + ' deg, rms ' + (F $settle.rms 3)) $(if ($agree) { 'agrees' } else { 'DISAGREES with the whole fit: look' })
    if (-not $agree) { $problems++ }
} elseif ($best.law -eq 1) { Row 'the settle alone' 'not fitted' 'a share''s target comes back smoothly: no swing of its own after it' '' '- (the whole fit''s spring stands)' }
else { Row 'the settle alone' 'not fitted' 'the target never back at nothing before the next thing' '' 'look'; $problems++ }
Row 'the camera''s round' ((Pm (1000 * $p[7]) (1000 * $se[7]) 0) + ' ms against the log''s') '(recoil.h dates it in its frame)' '-' '-'
# The sideways: its sign against the turn's.
$sideMean = if ($sides.Count) { ($sides | Measure-Object -Average).Average } else { [double]::NaN }
$sideSd = if ($sides.Count -gt 1) { [Math]::Sqrt((($sides | ForEach-Object { ($_ - $sideMean) * ($_ - $sideMean) }) | Measure-Object -Sum).Sum / ($sides.Count - 1)) } else { [double]::NaN }
$sideSe = if ($sides.Count -gt 1) { $sideSd / [Math]::Sqrt($sides.Count) } else { [double]::NaN }
$dataMean = ($data.sideMin + $data.sideMax) / 2
$sideRange = if ($sides.Count) { '{0}..{1}' -f (F (($sides | Measure-Object -Minimum).Minimum) 2), (F (($sides | Measure-Object -Maximum).Maximum) 2) } else { '-' }
Row 'sideways a round (log''s yaw)' ((Pm $sideMean $sideSe 3) + " ($sideRange)") ('{0}..{1} (its mean {2})' -f (F $data.sideMin 2), (F $data.sideMax 2), (F $dataMean 2)) '-' '-'
$signSet = 'not told (keep)'; $signFitted = '-'
# (With a recovery inside the string the sideways comes back to nothing between the rounds, from either side: each round's own pick isn't told then.)
if ($inside) { $signFitted = 'not told: the sideways comes back between the rounds of a string'; $problems++ }
elseif (-not [double]::IsNaN($sideMean) -and -not [double]::IsNaN($sideSe) -and [Math]::Abs($sideMean) -gt 2 * $sideSe -and $dataMean -ne 0) {
    $right = [Math]::Sign($sideMean) * $rightSign  # +1: the camera went right
    $sign = [Math]::Sign($dataMean) * $right
    $signFitted = 'the camera went ' + $(if ($right -gt 0) { 'right' } else { 'left' }) + ' for the data''s ' + $(if ($dataMean -lt 0) { 'negative' } else { 'positive' })
    $signSet = 'Tune::sideSign = ' + $(if ($sign -gt 0) { '1.0f (recoil.txt sidesign 1)' } else { '-1.0f (recoil.txt sidesign -1)' })
} elseif ($dataMean -eq 0) { $signFitted = 'the data''s pick is even: no sign to tell' }
else { $signFitted = 'the camera''s sideways not clear of its noise'; $problems++ }
Row 'sideways sign' $signFitted '+1: the data''s negative is left' ('Tune::sideSign = ' + $now.sideSign + 'f') $signSet
Write-Host "  (the turn: $turnSaid)"
Write-Host ("  ({0} once-a-second lines; {1} other recoil lines in these times)" -f $seconds.Count, $otherRecoil)
if ($jumps.Count -eq 0) { Write-Host '  No stick turn in these lines (weapon.txt "turnby 20"): the sign rests on aim.h''s word for the yaw.' }
foreach ($k in @(0, 1, 4, 5, 6, 7)) { if ($atBound[$k]) { Write-Host "  The fit's $($names[$k]) is at the edge of what it may be ($(F $p[$k] 3)): that number isn't found by this run."; $problems++ } }  # (the string gap and the recovery while firing are held)
Write-Host ''
Write-Host "These go into recoil.h and the CHANGELOG entry only after a look at the per-frame lines agrees: the camera's rise with each round,"
Write-Host 'its fall after a string, its small bounce. The design''s values are the studies'' best reading (design6_recoil.md 3.2, 13.2).'
exit $problems

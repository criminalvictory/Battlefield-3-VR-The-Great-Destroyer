# Like tools\cmpcentre.ps1 (a still against the middle of a bigger one, eye by eye), but first finds the whole-pixel shift between the two that makes a band of the picture
# agree best, and compares at that shift: two launches of the game seldom put the camera on the same pixel. Prints, per eye: the shift found, the mean difference over the
# band at no shift and at the best shift, and a 12 x 12 map of the mean difference at the best shift. Read-only but for the optional difference picture.
#   tools\cmpshift.ps1 -Small <a.bmp> -Big <b.bmp> [-Reach 40] [-BandTop 0.30] [-BandBottom 0.62] [-Out <png>] [-Gain 6]
param([Parameter(Mandatory)][string]$Small, [Parameter(Mandatory)][string]$Big, [int]$Reach = 40, [double]$BandTop = 0.30, [double]$BandBottom = 0.62, [string]$Out = '', [double]$Gain = 6)
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class CmpShift {
    static byte[] Load(string path, out int w, out int h) {
        using (var b = new Bitmap(path)) {
            w = b.Width; h = b.Height;
            var d = b.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            var px = new byte[w * h * 4];
            for (int y = 0; y < h; y++) Marshal.Copy(new IntPtr(d.Scan0.ToInt64() + (long)y * d.Stride), px, y * w * 4, w * 4);
            b.UnlockBits(d);
            return px;
        }
    }
    static double Lum(byte[] p, int i) { return 0.114 * p[i] + 0.587 * p[i + 1] + 0.299 * p[i + 2]; }
    // The mean difference of brightness over the band (every step-th pixel) with the big picture read (dx, dy) further right and down.
    static double Band(byte[] a, int sw, int se, byte[] b, int bw, int be, int eye, int off, int dx, int dy, int y0, int y1, int step, int margin) {
        double sum = 0; long n = 0;
        for (int y = y0; y < y1; y += step)
            for (int x = margin; x < se - margin; x += step) {
                int ai = (y * sw + eye * se + x) * 4;
                int bi = ((y + off + dy) * bw + eye * be + off + x + dx) * 4;
                sum += Math.Abs(Lum(a, ai) - Lum(b, bi)); n++;
            }
        return sum / Math.Max(1, n);
    }
    public static string Run(string small, string big, int reach, double bandTop, double bandBottom, string outPath, double gain) {
        int sw, sh, bw, bh;
        var a = Load(small, out sw, out sh);
        var b = Load(big, out bw, out bh);
        int se = sw / 2, be = bw / 2, off = (be - se) / 2, cells = 12;
        if (sh != se || bh != be || be < se) return "want two pictures of two square eyes side by side, the second not smaller: " + sw + "x" + sh + ", " + bw + "x" + bh;
        if (reach > off && be > se) reach = Math.Min(reach, off);
        var text = new System.Text.StringBuilder();
        text.AppendLine("small an eye " + se + ", big an eye " + be + " (its middle " + off + " pixels in); shifts tried up to " + reach + " pixels each way, over rows " + (int)(bandTop * se) + " to " + (int)(bandBottom * se) + " of the eye");
        byte[] opx = null; int ow = se, oh = se / 2;
        if (!string.IsNullOrEmpty(outPath)) opx = new byte[ow * oh * 4];
        for (int eye = 0; eye < 2; eye++) {
            int y0 = (int)(bandTop * se), y1 = (int)(bandBottom * se), margin = reach + 2;
            // coarse (every 4th pixel, shifts in steps of 2), then fine about the best
            double best = double.MaxValue; int bx = 0, by = 0;
            for (int dy = -reach; dy <= reach; dy += 2)
                for (int dx = -reach; dx <= reach; dx += 2) {
                    if (be == se && (Math.Abs(dx) > 0 || Math.Abs(dy) > 0) && (y0 + dy < 0 || y1 + dy >= se)) continue;
                    double v = Band(a, sw, se, b, bw, be, eye, off, dx, dy, y0, y1, 4, margin);
                    if (v < best) { best = v; bx = dx; by = dy; }
                }
            int cx = bx, cy = by;
            for (int dy = cy - 2; dy <= cy + 2; dy++)
                for (int dx = cx - 2; dx <= cx + 2; dx++) {
                    if (Math.Abs(dx) > reach || Math.Abs(dy) > reach) continue;
                    double v = Band(a, sw, se, b, bw, be, eye, off, dx, dy, y0, y1, 2, margin);
                    if (v < best) { best = v; bx = dx; by = dy; }
                }
            double none = Band(a, sw, se, b, bw, be, eye, off, 0, 0, y0, y1, 2, margin);
            best = Band(a, sw, se, b, bw, be, eye, off, bx, by, y0, y1, 2, margin);
            text.AppendLine((eye == 0 ? "left" : "right") + " eye: best shift (" + bx + ", " + by + ") pixels (right, down); the band's mean difference " + none.ToString("0.00") + " at no shift, " + best.ToString("0.00") + " at the best" + (Math.Abs(bx) == reach || Math.Abs(by) == reach ? "  [AT THE EDGE OF WHAT WAS TRIED: not a found shift]" : ""));
            var sum = new double[cells * cells]; var n = new long[cells * cells];
            for (int y = margin; y < se - margin; y++) {
                int cyc = y * cells / se;
                for (int x = margin; x < se - margin; x++) {
                    int ai = (y * sw + eye * se + x) * 4;
                    int bi = ((y + off + by) * bw + eye * be + off + x + bx) * 4;
                    double m = (Math.Abs(a[ai] - b[bi]) + Math.Abs(a[ai + 1] - b[bi + 1]) + Math.Abs(a[ai + 2] - b[bi + 2])) / 3.0;
                    int c = cyc * cells + x * cells / se;
                    sum[c] += m; n[c]++;
                    if (opx != null && (y & 1) == 0 && (x & 1) == 0) {
                        int oi = ((y / 2) * ow + eye * (se / 2) + x / 2) * 4;
                        byte v = (byte)Math.Min(255.0, m * gain);
                        opx[oi] = v; opx[oi + 1] = v; opx[oi + 2] = v; opx[oi + 3] = 255;
                    }
                }
            }
            for (int r = 0; r < cells; r++) {
                var line = new System.Text.StringBuilder("   ");
                for (int c = 0; c < cells; c++) line.Append((sum[r * cells + c] / Math.Max(1, n[r * cells + c])).ToString("0.0").PadLeft(6));
                text.AppendLine(line.ToString());
            }
        }
        if (opx != null) {
            using (var o = new Bitmap(ow, oh, PixelFormat.Format32bppArgb)) {
                var od = o.LockBits(new Rectangle(0, 0, ow, oh), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                for (int y = 0; y < oh; y++) Marshal.Copy(opx, y * ow * 4, new IntPtr(od.Scan0.ToInt64() + (long)y * od.Stride), ow * 4);
                o.UnlockBits(od); o.Save(outPath, ImageFormat.Png);
            }
            text.AppendLine("the difference at the best shift, " + gain + " times, half size: " + outPath);
        }
        return text.ToString();
    }
}
'@
$Small = (Resolve-Path $Small).Path; $Big = (Resolve-Path $Big).Path
[CmpShift]::Run($Small, $Big, $Reach, $BandTop, $BandBottom, $Out, $Gain)

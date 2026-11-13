# Compares two stills of both eyes side by side (build\eye_both.bmp copies) of different sizes: the smaller one against the middle of the bigger one, eye by eye, pixel for pixel.
# For a picture drawn wider at the same sharpness (more pixels, a wider angle): its middle should be the old picture. Prints each eye's mean difference (0 to 255) and a 12 x 12
# map of it, and saves a picture of the difference (six times, half size). Nothing is touched but the output picture.
#   tools\cmpcentre.ps1 -Small <a.bmp> -Big <b.bmp> [-Out <png>] [-Gain 6]
param([Parameter(Mandatory)][string]$Small, [Parameter(Mandatory)][string]$Big, [string]$Out = '', [double]$Gain = 6)
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class CmpCentre {
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
    // The result: for each eye the mean difference and a cells x cells map; the difference picture saved if a path is given.
    public static string Run(string small, string big, string outPath, double gain, int cells) {
        int sw, sh, bw, bh;
        var a = Load(small, out sw, out sh);
        var b = Load(big, out bw, out bh);
        int se = sw / 2, be = bw / 2;
        if (sh != se || bh != be) return "not two square eyes side by side: " + sw + "x" + sh + " and " + bw + "x" + bh;
        if (be < se) return "the second picture is the smaller one";
        int off = (be - se) / 2;
        var text = new System.Text.StringBuilder();
        text.AppendLine("small " + sw + "x" + sh + " (an eye " + se + "), big " + bw + "x" + bh + " (an eye " + be + "); the small one against the big one's middle, " + off + " pixels in from each side");
        Bitmap o = null; BitmapData od = null; byte[] opx = null; int ow = se, oh = se / 2;
        if (!string.IsNullOrEmpty(outPath)) { o = new Bitmap(ow, oh, PixelFormat.Format32bppArgb); opx = new byte[ow * oh * 4]; }
        for (int eye = 0; eye < 2; eye++) {
            var sum = new double[cells * cells]; var n = new long[cells * cells];
            double total = 0; long count = 0; long over8 = 0;
            for (int y = 0; y < se; y++) {
                int ai = (y * sw + eye * se) * 4;
                int bi = ((y + off) * bw + eye * be + off) * 4;
                int cy = y * cells / se;
                for (int x = 0; x < se; x++, ai += 4, bi += 4) {
                    int d = Math.Abs(a[ai] - b[bi]) + Math.Abs(a[ai + 1] - b[bi + 1]) + Math.Abs(a[ai + 2] - b[bi + 2]);
                    double m = d / 3.0;
                    int c = cy * cells + x * cells / se;
                    sum[c] += m; n[c]++; total += m; count++;
                    if (m > 8) over8++;
                    if (opx != null && (y & 1) == 0 && (x & 1) == 0) {
                        int oi = ((y / 2) * ow + eye * (se / 2) + x / 2) * 4;
                        byte v = (byte)Math.Min(255.0, m * gain);
                        opx[oi] = v; opx[oi + 1] = v; opx[oi + 2] = v; opx[oi + 3] = 255;
                    }
                }
            }
            text.AppendLine((eye == 0 ? "left" : "right") + " eye: mean difference " + (total / count).ToString("0.00") + " of 255; " + (100.0 * over8 / count).ToString("0.0") + "% of pixels differ by more than 8");
            for (int r = 0; r < cells; r++) {
                var line = new System.Text.StringBuilder("   ");
                for (int c = 0; c < cells; c++) line.Append((sum[r * cells + c] / Math.Max(1, n[r * cells + c])).ToString("0.0").PadLeft(6));
                text.AppendLine(line.ToString());
            }
        }
        if (o != null) {
            od = o.LockBits(new Rectangle(0, 0, ow, oh), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            for (int y = 0; y < oh; y++) Marshal.Copy(opx, y * ow * 4, new IntPtr(od.Scan0.ToInt64() + (long)y * od.Stride), ow * 4);
            o.UnlockBits(od); o.Save(outPath, ImageFormat.Png); o.Dispose();
            text.AppendLine("the difference, " + gain + " times, half size (left eye, right eye): " + outPath);
        }
        return text.ToString();
    }
}
'@
$Small = (Resolve-Path $Small).Path; $Big = (Resolve-Path $Big).Path
[CmpCentre]::Run($Small, $Big, $Out, $Gain, 12)

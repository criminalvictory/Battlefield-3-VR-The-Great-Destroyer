# The shift between two stills measured patch by patch (a still against the middle of a bigger one, or two alike), left eye: for a grid of patches, the whole-pixel shift
# of the big picture that agrees best with the small one there. A camera turned a little gives the same shift everywhere; another magnification gives a shift that grows
# in proportion to the distance from the picture's middle. Prints each patch's middle (pixels from the eye's middle), its best shift, and how much better that is than none;
# then the straight-line fit of the shift against the place. Read-only.
#   tools\localshift.ps1 -Small <a.bmp> -Big <b.bmp> [-Reach 16] [-Patch 200] [-Across 9] [-Down 9]
param([Parameter(Mandatory)][string]$Small, [Parameter(Mandatory)][string]$Big, [int]$Reach = 16, [int]$Patch = 200, [int]$Across = 9, [int]$Down = 9, [int]$Eye = 0)
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class LocalShift {
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
    public static string Run(string small, string big, int reach, int patch, int across, int down, int eye) {
        int sw, sh, bw, bh;
        var a = Load(small, out sw, out sh);
        var b = Load(big, out bw, out bh);
        int se = sw / 2, be = bw / 2, off = (be - se) / 2;
        var text = new System.Text.StringBuilder();
        text.AppendLine("an eye " + se + " against the middle of an eye " + be + "; patches " + patch + " square, shifts up to " + reach + "; eye " + eye);
        text.AppendLine("  patch middle (x, y from the eye's middle)   best shift (right, down)   difference at none / at best   texture");
        double sx = 0, sxx = 0, sxd = 0, sdx = 0, sy = 0, syy = 0, syd = 0, sdy = 0; int used = 0;
        int margin = reach + 2;
        for (int r = 0; r < down; r++)
            for (int c = 0; c < across; c++) {
                int px0 = margin + (int)((long)(se - 2 * margin - patch) * c / Math.Max(1, across - 1));
                int py0 = margin + (int)((long)(se - 2 * margin - patch) * r / Math.Max(1, down - 1));
                // how much there is to hold on to: the mean difference between neighbours 4 pixels apart
                double tex = 0; long tn = 0;
                for (int y = py0; y < py0 + patch; y += 2)
                    for (int x = px0; x < px0 + patch - 4; x += 2) { int i = (y * sw + eye * se + x) * 4; tex += Math.Abs(Lum(a, i) - Lum(a, i + 16)); tn++; }
                tex /= Math.Max(1, tn);
                double best = double.MaxValue, none = 0; int bx = 0, by = 0;
                for (int dy = -reach; dy <= reach; dy++)
                    for (int dx = -reach; dx <= reach; dx++) {
                        double sum = 0; long n = 0;
                        for (int y = py0; y < py0 + patch; y += 2)
                            for (int x = px0; x < px0 + patch; x += 2) {
                                int ai = (y * sw + eye * se + x) * 4;
                                int bi = ((y + off + dy) * bw + eye * be + off + x + dx) * 4;
                                sum += Math.Abs(Lum(a, ai) - Lum(b, bi)); n++;
                            }
                        double v = sum / n;
                        if (dx == 0 && dy == 0) none = v;
                        if (v < best) { best = v; bx = dx; by = dy; }
                    }
                double cx = px0 + patch / 2.0 - se / 2.0, cy = py0 + patch / 2.0 - se / 2.0;
                bool edge = Math.Abs(bx) == reach || Math.Abs(by) == reach;
                bool good = !edge && tex >= 3.0 && best < 0.6 * none;   // texture to hold on to, and a shift that is clearly better than none
                bool same = !edge && tex >= 3.0 && none < 1.5;          // or the patch already agrees at no shift
                if (same) { bx = 0; by = 0; good = true; }
                text.AppendLine(string.Format("  ({0,6:0}, {1,6:0})   ({2,3}, {3,3})   {4,6:0.00} / {5,6:0.00}   {6,5:0.0}  {7}", cx, cy, bx, by, none, best, tex, good ? "used" : (edge ? "at the reach's edge" : "not clear")));
                if (good) { sx += cx; sxx += cx * cx; sxd += cx * bx; sdx += bx; sy += cy; syy += cy * cy; syd += cy * by; sdy += by; used++; }
            }
        if (used >= 6) {
            double kx = (used * sxd - sx * sdx) / (used * sxx - sx * sx), ax = (sdx - kx * sx) / used;
            double ky = (used * syd - sy * sdy) / (used * syy - sy * sy), ay = (sdy - ky * sy) / used;
            text.AppendLine(string.Format("fit over {0} patches: shift across = {1:0.000} + {2:0.00000} x (pixels from the middle); shift down = {3:0.000} + {4:0.00000} x", used, ax, kx, ay, ky));
            text.AppendLine(string.Format("  read as a magnification: the big picture's middle is {0:0.0000} times the small picture across and {1:0.0000} times down", 1.0 + kx, 1.0 + ky));
        } else text.AppendLine("too few clear patches for a fit (" + used + ")");
        return text.ToString();
    }
}
'@
$Small = (Resolve-Path $Small).Path; $Big = (Resolve-Path $Big).Path
[LocalShift]::Run($Small, $Big, $Reach, $Patch, $Across, $Down, $Eye)

# The signed difference of brightness between a still and the middle of a bigger one (or two alike), cell by cell, left eye: the big picture's mean brightness less the small
# one's in each of 12 x 12 cells, and each cell's own mean in the small picture. A brightness worked out from the picture's size (a vignette, an exposure) shows as a smooth
# pattern here whatever the fine detail does; things that moved show as scattered cells. Read-only.
#   tools\cmpbright.ps1 -Small <a.bmp> -Big <b.bmp>
param([Parameter(Mandatory)][string]$Small, [Parameter(Mandatory)][string]$Big, [int]$Eye = 0)
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class CmpBright {
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
    public static string Run(string small, string big, int eye) {
        int sw, sh, bw, bh;
        var a = Load(small, out sw, out sh);
        var b = Load(big, out bw, out bh);
        int se = sw / 2, be = bw / 2, off = (be - se) / 2, cells = 12;
        var sa = new double[cells * cells]; var sb = new double[cells * cells]; var n = new long[cells * cells];
        for (int y = 0; y < se; y++) {
            int cy = y * cells / se;
            for (int x = 0; x < se; x++) {
                int ai = (y * sw + eye * se + x) * 4, bi = ((y + off) * bw + eye * be + off + x) * 4;
                int c = cy * cells + x * cells / se;
                sa[c] += 0.114 * a[ai] + 0.587 * a[ai + 1] + 0.299 * a[ai + 2];
                sb[c] += 0.114 * b[bi] + 0.587 * b[bi + 1] + 0.299 * b[bi + 2];
                n[c]++;
            }
        }
        var text = new System.Text.StringBuilder();
        text.AppendLine("the big picture's middle less the small picture, mean brightness by cell (0 to 255; + the big one brighter):");
        for (int r = 0; r < cells; r++) {
            var line = new System.Text.StringBuilder("   ");
            for (int c = 0; c < cells; c++) line.Append(((sb[r * cells + c] - sa[r * cells + c]) / n[r * cells + c]).ToString("+0.0;-0.0").PadLeft(7));
            text.AppendLine(line.ToString());
        }
        text.AppendLine("the small picture's own mean brightness by cell:");
        for (int r = 0; r < cells; r++) {
            var line = new System.Text.StringBuilder("   ");
            for (int c = 0; c < cells; c++) line.Append((sa[r * cells + c] / n[r * cells + c]).ToString("0").PadLeft(7));
            text.AppendLine(line.ToString());
        }
        return text.ToString();
    }
}
'@
$Small = (Resolve-Path $Small).Path; $Big = (Resolve-Path $Big).Path
[CmpBright]::Run($Small, $Big, $Eye)

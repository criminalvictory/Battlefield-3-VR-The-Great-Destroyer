# Reads build\reloadrec.bin made with weapon.txt "allrec": head of 8 ints (kind = the draw's index count, frame,
# eye, pass = the draw's start index, loaded, spare, ms, bones), then the bones, 12 floats each (3 rows of 4).
#   tools\allrec.ps1                 the kinds of draw recorded
#   tools\allrec.ps1 -Kind 3414 -Start 0 [-Ref n]   which bones move against its stillest one (or bone n)
#   tools\allrec.ps1 -Palm    the gloves' palms in the launcher's frame (bone 0 of 3414/0) through the recording
param([switch]$Palm, [int]$Kind = -1, [int]$Start = 0, [int]$Ref = -1, [string]$Path = 'E:\BF3 VR PROJECT\build\reloadrec.bin')
Add-Type -TypeDefinition @'
using System; using System.IO; using System.Linq; using System.Collections.Generic; using System.Text;
public static class AllRec {
  public class R { public int ms, loaded, spare, bones; public float[] m; }
  public static Dictionary<long, List<R>> Load(string path, int kind, int start) {
    var d = new Dictionary<long, List<R>>();
    using (var f = new BinaryReader(new BufferedStream(File.OpenRead(path), 1 << 20))) {
      long len = f.BaseStream.Length, pos = 0;
      while (pos + 32 <= len) {
        int k = f.ReadInt32(); f.ReadInt32(); f.ReadInt32(); int s = f.ReadInt32();
        var r = new R(); r.loaded = f.ReadInt32(); r.spare = f.ReadInt32(); r.ms = f.ReadInt32(); r.bones = f.ReadInt32();
        pos += 32 + r.bones * 48;
        long key = ((long)k << 32) | (uint)s;
        bool want = kind < 0 || (k == kind && s == start);
        if (want && kind >= 0) { var b = f.ReadBytes(r.bones * 48); r.m = new float[r.bones * 12]; Buffer.BlockCopy(b, 0, r.m, 0, b.Length); }
        else f.BaseStream.Seek(r.bones * 48, SeekOrigin.Current);
        List<R> l; if (!d.TryGetValue(key, out l)) d[key] = l = new List<R>(); l.Add(r);
      }
    }
    return d;
  }
  // p in the space of bone a (3x4 rows, orthonormal up to scale taken as 1).
  public static double[] Rel(float[] m, int a, int b) {
    double dx = m[b*12+3]-m[a*12+3], dy = m[b*12+7]-m[a*12+7], dz = m[b*12+11]-m[a*12+11];
    return new double[]{ m[a*12]*dx+m[a*12+4]*dy+m[a*12+8]*dz, m[a*12+1]*dx+m[a*12+5]*dy+m[a*12+9]*dz, m[a*12+2]*dx+m[a*12+6]*dy+m[a*12+10]*dz };
  }
  public static bool Near(float[] m, int k) { return (m[k*12] != 0 || m[k*12+5] != 0) && Math.Abs(m[k*12+3]) < 5 && Math.Abs(m[k*12+7]) < 5 && Math.Abs(m[k*12+11]) < 5; }
  static double Med(List<double> v) { v.Sort(); return v[v.Count/2]; }
  public static string Study(string path, int kind, int start, int rf) {
    var rows = Load(path, kind, start)[((long)kind << 32) | (uint)start].Where(r => Near(r.m, 0) && Near(r.m, 1)).ToList();
    var sb = new StringBuilder();
    sb.AppendFormat("records {0} ms {1}-{2} ammo {3}/{4} -> {5}/{6}\n", rows.Count, rows[0].ms, rows[rows.Count-1].ms, rows[0].loaded, rows[0].spare, rows[rows.Count-1].loaded, rows[rows.Count-1].spare);
    var used = new List<int>();
    for (int k = 0; k < rows[0].bones; ++k) if (rows.Count(r => Near(r.m, k)) > rows.Count * 95 / 100) used.Add(k);
    sb.AppendLine("bones used: " + string.Join(" ", used));
    var step = rows.Where((r,i) => i % 12 == 0).ToList();
    if (rf < 0) {
      double best = 1e9;
      foreach (int u in used) {
        var all = new List<double>();
        foreach (int v in used) if (u != v) {
          double lo = 1e9, hi = -1e9;
          foreach (var r in step) { var p = Rel(r.m, u, v); double s = p[0]+p[1]*1.3+p[2]*1.7; lo = Math.Min(lo,s); hi = Math.Max(hi,s); }
          all.Add(hi-lo);
        }
        double score = Med(all);
        if (score < best) { best = score; rf = u; }
      }
    }
    sb.AppendLine("reference bone " + rf);
    foreach (int u in used) {
      double[] lo = {1e9,1e9,1e9}, hi = {-1e9,-1e9,-1e9};
      foreach (var r in step) { var p = Rel(r.m, rf, u); for (int i = 0; i < 3; ++i) { lo[i] = Math.Min(lo[i],p[i]); hi[i] = Math.Max(hi[i],p[i]); } }
      var f = Rel(rows[0].m, rf, u);
      sb.AppendFormat("{0}: spread cm {1:F1} {2:F1} {3:F1}  first at {4:F3} {5:F3} {6:F3}\n", u, (hi[0]-lo[0])*100, (hi[1]-lo[1])*100, (hi[2]-lo[2])*100, f[0], f[1], f[2]);
    }
    sb.AppendLine("reference in the arms' space over time (ms: place, z axis):");
    int last = -1000;
    foreach (var r in rows) if (r.ms - last >= 500) { last = r.ms; var m = r.m; int a = rf*12;
      sb.AppendFormat("{0}: {1:F3} {2:F3} {3:F3}  z {4:F2} {5:F2} {6:F2}  y {7:F2} {8:F2} {9:F2} ammo {10}/{11}\n", r.ms, m[a+3], m[a+7], m[a+11], m[a+2], m[a+6], m[a+10], m[a+1], m[a+5], m[a+9], r.loaded, r.spare); }
    return sb.ToString();
  }
  public static string Palms(string path) {
    var gun = Load(path, 3414, 0)[((long)3414 << 32)].Where(r => Near(r.m, 0) && Near(r.m, 1)).ToList();
    var gl = Load(path, 13755, 15051)[((long)13755 << 32) | 15051u].Where(r => Near(r.m, 37)).ToList();
    var sb = new StringBuilder(); int last = -1000, gi = 0;
    double[] kR = {-0.505,1.129,0.150}, kL = {0.537,1.098,0.207};
    foreach (var r in gl) { if (r.ms - last < 400) continue; last = r.ms;
      while (gi + 1 < gun.Count && gun[gi+1].ms <= r.ms) ++gi;
      float[] g = gun[gi].m, m = r.m; var o = new StringBuilder();
      foreach (var pair in new[]{ new { b = 37, k = kR }, new { b = 1, k = kL } }) { var k = pair.k; int a = pair.b*12;
        double[] w = new double[3]; for (int i = 0; i < 3; ++i) w[i] = m[a+i*4]*k[0]+m[a+i*4+1]*k[1]+m[a+i*4+2]*k[2]+m[a+i*4+3];
        double dx=w[0]-g[3],dy=w[1]-g[7],dz=w[2]-g[11];
        o.AppendFormat("  {0:F3} {1:F3} {2:F3}", g[0]*dx+g[4]*dy+g[8]*dz, g[1]*dx+g[5]*dy+g[9]*dz, g[2]*dx+g[6]*dy+g[10]*dz); }
      sb.AppendFormat("{0} (right palm, left palm){1}\n", r.ms, o); }
    return sb.ToString();
  }
  public static string Kinds(string path) {
    var sb = new StringBuilder();
    foreach (var kv in Load(path, -1, 0).OrderByDescending(x => x.Value.Count).Take(30))
      sb.AppendFormat("{0} {1}: {2}\n", kv.Key >> 32, kv.Key & 0xffffffff, kv.Value.Count);
    return sb.ToString();
  }
}
'@
if ($Palm) { [AllRec]::Palms($Path) } elseif ($Kind -lt 0) { [AllRec]::Kinds($Path) } else { [AllRec]::Study($Path, $Kind, $Start, $Ref) }

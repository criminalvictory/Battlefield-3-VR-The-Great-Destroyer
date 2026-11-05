# Writes src\scopemarks.h from the mod's mesh dumps ("dump <count> <start>" while zoomed): what each sight draws of the game's sheet of markings, its place as seen from the zoomed eye.
# Each part: the dump's count and start, where its texture coordinates are in a vertex, and whether it's multiplied in (black lines, 0) or drawn in its colour (1).
# Its marks' places are given as the slope from the sight's line, in the world's terms: the zoomed eye sees a mark at (x - eye x) / distance in the model; the weapon's own projection and the world's
# are not the same, so that goes by the weapon's projection over the world's: what's under a mark in the game stays under it.
param([string]$Dir = (Join-Path $PSScriptRoot '..\build'), [string]$Out = (Join-Path $PSScriptRoot '..\src\scopemarks.h'))
$sights = @(
    # Eye: where the zoomed eye's line is across the marks (x, y) and how far back it is (z), in the part's bone 0: "the view in its bone 0" from glassdump. Weapon: the first person's projection, zoomed. World: the world's, 1 / tan(half the game's FOV).
    @{ Name = 'Acog';  Eye = @(0.0, 0.0317, -0.2674); Weapon = 7.596; World = 6.612; Color = @(1.0, 0.0, 0.0); Parts = @(@(516, 6957, 28, 0), @(12, 0, 28, 1)) },   # FOV 17.2
    @{ Name = 'Pso1';  Eye = @(0.0, 0.0329, -0.2812); Weapon = 7.596; World = 6.612; Color = @(0.0, 0.0, 0.0); Parts = @(@(480, 4713, 28, 0), @(54, 4659, 28, 1)) }, # FOV 17.2
    @{ Name = 'Pks07'; Eye = @(0.0, 0.0349, -0.2860); Weapon = 7.596; World = 11.55; Color = @(0.0, 0.0, 0.0); Parts = @(@(480, 606, 28, 0), @(54, 552, 28, 1)) },   # FOV 9.9
    @{ Name = 'Pka';   Eye = @(0.0, 0.0285, -0.2543); Weapon = 7.596; World = 5.671; Color = @(1.0, 0.0, 0.0); Parts = @(,@(12, 210, 24, 1)) },                        # FOV 20
    @{ Name = 'PkAs';  Eye = @(0.0, 0.0434, -0.4359); Weapon = 7.596; World = 3.487; Color = @(1.0, 0.0, 0.0); Parts = @(,@(3360, 0, 24, 1)) },                        # FOV 32
    @{ Name = 'Kobra'; Eye = @(0.0, 0.0402, -0.4236); Weapon = 7.596; World = 2.747; Color = @(1.0, 0.0, 0.0); Parts = @(,@(18, 0, 24, 1)) }                           # FOV 40
)
$over = if (Test-Path "$Dir\scopemarks.txt") { Get-Content "$Dir\scopemarks.txt" } else { @() }
foreach ($l in $over) { $p = $l -split ' '; foreach ($t in $sights) { if ($t.Name -eq $p[0]) { $t.Eye = @([double]$p[1], [double]$p[2], [double]$p[3]); $t.Weapon = [double]$p[4]; $t.World = [double]$p[5] } } }
$code = @'
using System; using System.IO; using System.Collections.Generic; using System.Globalization; using System.Text;
public static class ScopeMarks {
  static float Half(ushort h) { int s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF; double v = e == 0 ? Math.Pow(2, -14) * (m / 1024.0) : Math.Pow(2, e - 15) * (1 + m / 1024.0); return (float)(s != 0 ? -v : v); }
  // Triangles as lines of x, y, u, v.
  public static string Part(string dir, int count, int start, int uvAt, double[] eye, double scale, double[] reach) {
    string name = dir + "\\mesh_" + count + "_" + start + "_";
    int stride = 32, offset = 0;
    foreach (var l in File.ReadAllLines(name + "dump.txt")) { var p = l.Split(' '); if (p[0] == "vb" && p[1] == "0") { stride = int.Parse(p[3]); offset = int.Parse(p[5]); } }
    var vb = File.ReadAllBytes(name + "vb0.bin"); var ib = File.ReadAllBytes(name + "ib.bin");
    var sb = new StringBuilder(); var inv = CultureInfo.InvariantCulture;
    for (int i = start; i < start + count; i++) {
      int v = BitConverter.ToUInt16(ib, i * 2); int o = v * stride + offset;
      double x = Half(BitConverter.ToUInt16(vb, o)), y = Half(BitConverter.ToUInt16(vb, o + 2)), z = Half(BitConverter.ToUInt16(vb, o + 4));
      double u = Half(BitConverter.ToUInt16(vb, o + uvAt)), w = Half(BitConverter.ToUInt16(vb, o + uvAt + 2));
      double d = z - eye[2]; double sx = (x - eye[0]) / d * scale, sy = (y - eye[1]) / d * scale;
      reach[0] = Math.Max(reach[0], Math.Max(Math.Abs(sx), Math.Abs(sy)));
      sb.Append(string.Format(inv, "{{{0:F5}f, {1:F5}f, {2:F4}f, {3:F4}f}},", sx, sy, u, w));
      if ((i - start) % 6 == 5) sb.Append("\n    "); else sb.Append(' ');
    }
    return sb.ToString();
  }
}
'@
if (-not ('ScopeMarks' -as [type])) { Add-Type -TypeDefinition $code }
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('// Written by tools\scopemarks.ps1 from the mod''s mesh dumps. What each sight draws of the game''s sheet of markings when it''s zoomed: triangles of the slope from the sight''s line (x left, y up, in the')
[void]$sb.AppendLine('// world''s terms) and the place on the sheet.')
[void]$sb.AppendLine('#pragma once')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('namespace scopemarks {')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('struct Vertex {')
[void]$sb.AppendLine('    float x, y, u, v;')
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('struct Marks {')
[void]$sb.AppendLine('    const Vertex* dark;    // multiplied in: black where the sheet is white')
[void]$sb.AppendLine('    int darkCount;')
[void]$sb.AppendLine('    const Vertex* bright;  // drawn in its colour')
[void]$sb.AppendLine('    int brightCount;')
[void]$sb.AppendLine('    float color[3];')
[void]$sb.AppendLine('    float reach;           // the furthest of them from the line')
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
$rows = @()
foreach ($t in $sights) {
    $reach = [double[]]@(0.0)
    $scale = $t.Weapon / $t.World
    $tables = @{ 0 = ''; 1 = '' }; $counts = @{ 0 = 0; 1 = 0 }
    foreach ($part in $t.Parts) {
        if (-not (Test-Path "$Dir\mesh_$($part[0])_$($part[1])_dump.txt")) { "no dump of $($part[0])/$($part[1]) for $($t.Name)"; continue }
        $tables[[int]$part[3]] += [ScopeMarks]::Part($Dir, $part[0], $part[1], $part[2], [double[]]$t.Eye, $scale, $reach)
        $counts[[int]$part[3]] += $part[0]
    }
    foreach ($kind in 0, 1) {
        if ($counts[$kind]) { [void]$sb.AppendLine("inline constexpr Vertex k$($t.Name)$(if ($kind) { 'Bright' } else { 'Dark' })[] = {"); [void]$sb.AppendLine("    $($tables[$kind].TrimEnd())"); [void]$sb.AppendLine('};') }
    }
    $dark = if ($counts[0]) { "k$($t.Name)Dark" } else { 'nullptr' }; $bright = if ($counts[1]) { "k$($t.Name)Bright" } else { 'nullptr' }
    $rows += "inline constexpr Marks k$($t.Name){$dark, $($counts[0]), $bright, $($counts[1]), {$($t.Color[0].ToString('F1'))f, $($t.Color[1].ToString('F1'))f, $($t.Color[2].ToString('F1'))f}, $([string]::Format([Globalization.CultureInfo]::InvariantCulture, '{0:F4}', $reach[0]))f};"
    "$($t.Name): dark $($counts[0]), bright $($counts[1]), reach $($reach[0])"
}
foreach ($r in $rows) { [void]$sb.AppendLine($r) }
[void]$sb.AppendLine('')
[void]$sb.AppendLine('}')
[IO.File]::WriteAllText($Out, ($sb.ToString() -replace '(\d)\.0f', '$1.0f'))
$Out

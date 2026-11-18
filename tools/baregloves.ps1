# A recording made on a level whose soldier has bare arms (Semper Fidelis, The Great Destroyer: the arms are one draw, 18108/16053) written again with
# the gloves' draw in it, so the tools that read the game's own hands from a recording (tools\magfit.cpp's grip and vanilla, tools\holdpic.cpp) can
# read these too. Every record is kept as it is; after each bare-arms record comes a gloves' record (13755/15051) of the same frame, eye and time, its
# bones the bare hands' under the gloves' numbers (skin.cpp's kBareGlove: the right hand's 0-20 the right glove's 23-44, the left's 26-46 the left
# glove's 0-22). The bare hands' rest is the gloves' to within a centimetre (each bone's vertices' middle in the two meshes: 1 to 11 mm apart, the
# wrists 2 to 3 cm), so a palm read this way is within about a centimetre of the gloves' own. Nothing else is changed; the source is only read.
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\baregloves.ps1 -In build\survey\AKS74u_partial.bin -Out build\survey\AKS74u_partial_gloves.bin
# A record: 8 ints (the draw's index count, the frame, the eye, its start, the rounds, the spare, the ms, the bones' count), then 48 bytes a bone.
param([Parameter(Mandatory = $true)][string]$In, [Parameter(Mandatory = $true)][string]$Out)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $In)) { Write-Host "No recording at $In"; exit 1 }
if (Test-Path $Out) { Write-Host "$Out is there already: left as it is"; exit 1 }
if (-not ('BareGloves.Convert' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.IO;

namespace BareGloves
{
    public static class Convert
    {
        // skin.cpp's kBareGlove: each bare arm bone as the gloves', -1 the forearms' own.
        static readonly int[] Map = { 27, 41, 43, 40, 33, 42, 31, 25, 26, 38, 23, 28, 29, 35, 36, 32, 24, 30, 34, 39, 37, -1, -1, -1, -1, -1,
                                      11, 20, 10, 21, 22, 7,  9,  19, 15, 6,  8,  12, 13, 18, 17, 1,  14, 16, 3,  2,  0,  -1, -1, -1, -1, -1 };
        const int Gloves = 45;  // the gloves' hands' bones, 0-44

        public static string Run(string from, string to)
        {
            int records = 0, made = 0;
            var identity = new byte[48];
            Buffer.BlockCopy(new float[] { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 }, 0, identity, 0, 48);
            using (var r = new BinaryReader(new BufferedStream(File.OpenRead(from), 1 << 20)))
            using (var w = new BinaryWriter(new BufferedStream(File.Create(to), 1 << 20)))
            {
                long length = r.BaseStream.Length;
                while (r.BaseStream.Position + 32 <= length)
                {
                    var head = new int[8];
                    for (int i = 0; i < 8; i++) head[i] = r.ReadInt32();
                    int bones = head[7];
                    if (bones <= 0 || bones > 64) break;
                    byte[] body = r.ReadBytes(bones * 48);
                    if (body.Length != bones * 48) break;
                    foreach (int v in head) w.Write(v);
                    w.Write(body);
                    records++;
                    if (head[0] != 18108 || head[3] != 16053 || bones < 47) continue;
                    w.Write(13755); w.Write(head[1]); w.Write(head[2]); w.Write(15051); w.Write(head[4]); w.Write(head[5]); w.Write(head[6]); w.Write(Gloves);
                    var glove = new byte[Gloves * 48];
                    for (int g = 0; g < Gloves; g++) Buffer.BlockCopy(identity, 0, glove, g * 48, 48);
                    for (int i = 0; i < Map.Length && i < bones; i++)
                        if (Map[i] >= 0) Buffer.BlockCopy(body, i * 48, glove, Map[i] * 48, 48);
                    w.Write(glove);
                    made++;
                }
            }
            return records + " records kept, " + made + " gloves' records written after the bare arms'";
        }
    }
}
'@
}
$from = (Resolve-Path $In).Path
$to = if ([IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path (Get-Location).Path $Out }
$said = [BareGloves.Convert]::Run($from, $to)
Write-Host ('{0}: {1}' -f $to, $said)
if ($said -match ', 0 gloves') { Write-Host 'No bare-arms draw (18108/16053) in it: the copy is the recording as it was.'; exit 2 }
exit 0

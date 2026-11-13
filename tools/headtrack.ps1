# Makes a head track for the head replay (build\headtest.txt "track <file>", src\headtest.h) from a log's "View:" lines (weapon.txt "viewlog N" writes one
# a view build): the first "head X shown" field is the head's yaw in the room (degrees, turning left positive), "pitch head Y" its pitch (up positive),
# the line's own time its time. So a shake somebody really made in the headset can be played again with nobody in it.
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\headtrack.ps1 -Log <file> -From hh:mm:ss[.fff] -To hh:mm:ss[.fff] -Out <file> [-Raw] [-Smooth n] [-Hz 72]
# What it does with the lines:
#  - Their times are the log's clock (a millisecond, and a line can be late by a few), but each line is the head of one frame, and frames are 1/72 s
#    apart: each line is put on that grid (the grid follows the lines' own times, so a missing line leaves a gap of two frames, not a jump in speed).
#    Two lines of one frame: the first is kept. Lines marked "again" (a camera offered a second time) are left out.
#  - The yaw is unwrapped (no jump at +-180). Unless -Raw, the yaw's and the pitch's middle (their medians) are taken off: the replay plays a track
#    about the head as it is when the test starts.
#  - -Smooth n: a row for every frame (a missing one between its neighbours), then n times over each frame the mean of itself twice and its two
#    neighbours (1 2 1): what steps unevenly from frame to frame is evened out, a swing's shape all but kept (each pass takes 3% off a shake of 4 a
#    second, 0.8% off one of 2).
#  - The file: "#" lines saying where it is from, then rows "ms yaw pitch" (ms from the first row; degrees).
# What the head is in those lines: the head as the mod's filter gave it to the game that session (xr_runtime.cpp FilterViews), not the head as the
# headset located it, which no line has. With the fixed filter (low-passed at 6 Hz, carried on 80 ms) its swings are a little rounder and later than
# the real ones, and where the filter ran 40 degrees behind and took the head as it was, it jumps a frame. With weapon.txt "poseadapt 1" it is the head
# carried on by its raw speed: it steps unevenly from frame to frame, which is that filter's doing and not the head's (-Smooth 2 evens it out).
# It prints the track's length, its fastest turn and each swing's fastest, to check against what is known of the motion. The exit code is 0 when a
# track of at least 20 rows was written.
param([string]$Log = '', [string]$From = '', [string]$To = '', [string]$Out = '', [switch]$Raw, [int]$Smooth = 0, [double]$Hz = 72.0)

$ErrorActionPreference = 'Stop'
$inv = [Globalization.CultureInfo]::InvariantCulture
if (-not $Log -or -not $Out) { Write-Host 'headtrack.ps1 -Log <file> -From hh:mm:ss -To hh:mm:ss -Out <file> [-Raw] [-Smooth n] [-Hz 72]'; exit 1 }
if (-not (Test-Path $Log)) { Write-Host "No log at $Log"; exit 1 }
$Log = (Resolve-Path $Log).Path

# hh:mm:ss or hh:mm:ss.fff as seconds of the day; -1 if it isn't one.
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

# The lines: time (ms of the day), yaw, pitch.
$times = New-Object System.Collections.Generic.List[double]
$yaws = New-Object System.Collections.Generic.List[double]
$pitches = New-Object System.Collections.Generic.List[double]
$again = 0
$pattern = [regex]'^\[(\d+):(\d+):(\d+)\.(\d+)\] View: hand .*? \| head (-?[\d.]+) shown -?[\d.]+(.*?) \| pitch head (-?[\d.]+) shown'
$stream = New-Object IO.FileStream($Log, [IO.FileMode]::Open, [IO.FileAccess]::Read, ([IO.FileShare]'ReadWrite, Delete'))
$reader = New-Object IO.StreamReader($stream)
try {
    while ($null -ne ($line = $reader.ReadLine())) {
        if ($line.IndexOf('] View: hand ') -lt 0) { continue }
        $m = $pattern.Match($line)
        if (-not $m.Success) { continue }
        $at = [int]$m.Groups[1].Value * 3600.0 + [int]$m.Groups[2].Value * 60.0 + [int]$m.Groups[3].Value + [int]$m.Groups[4].Value / 1000.0
        if ($fromAt -ge 0 -and $at -lt $fromAt) { continue }
        if ($toAt -ge 0 -and $at -gt $toAt) { continue }
        if ($m.Groups[6].Value -match 'again') { $again++; continue }
        $times.Add($at * 1000.0)
        $yaws.Add([double]::Parse($m.Groups[5].Value, $inv))
        $pitches.Add([double]::Parse($m.Groups[7].Value, $inv))
    }
} finally { $reader.Dispose(); $stream.Dispose() }
if ($times.Count -lt 20) { Write-Host "Only $($times.Count) ""View:"" lines between $From and $To in $Log (weapon.txt ""viewlog N"" writes them): no track"; exit 1 }

# Each line on the frames' grid. The grid goes on a frame at a time and is pulled a tenth of the way to each line's own time, so it keeps step with
# the lines however the two clocks drift; a line at least half a frame late is the frame after.
$period = 1000.0 / $Hz
$frames = New-Object System.Collections.Generic.List[int]
$rowYaw = New-Object System.Collections.Generic.List[double]
$rowPitch = New-Object System.Collections.Generic.List[double]
$grid = $times[0]; $frame = 0; $doubled = 0; $late = 0.0; $lastTime = $times[0]
$frames.Add(0); $rowYaw.Add($yaws[0]); $rowPitch.Add($pitches[0])
for ($i = 1; $i -lt $times.Count; $i++) {
    $steps = [int][Math]::Round(($times[$i] - $grid) / $period)
    if ($steps -lt 1) { $doubled++; continue }  # (a second line of the same frame)
    $frame += $steps
    $grid += $steps * $period
    $off = $times[$i] - $grid
    if ([Math]::Abs($off) -gt $late) { $late = [Math]::Abs($off) }
    $grid += 0.1 * $off
    $frames.Add($frame); $rowYaw.Add($yaws[$i]); $rowPitch.Add($pitches[$i]); $lastTime = $times[$i]
}
$n = $frames.Count
# A frame as long as the lines say it is on the whole (the headset's clock against the log's: a quarter of a percent longer than 1/72 s that night).
if ($frame -gt 0) { $period = ($lastTime - $times[0]) / $frame }
# The yaw without its jumps at +-180.
for ($i = 1; $i -lt $n; $i++) {
    $d = $rowYaw[$i] - $rowYaw[$i - 1]
    while ($d -gt 180) { $rowYaw[$i] -= 360; $d -= 360 }
    while ($d -lt -180) { $rowYaw[$i] += 360; $d += 360 }
}
function Median([double[]]$v) { $s = $v | Sort-Object; return $s[[int][Math]::Floor($s.Count / 2)] }
$midYaw = Median ($rowYaw.ToArray()); $midPitch = Median ($rowPitch.ToArray())
if (-not $Raw) { for ($i = 0; $i -lt $n; $i++) { $rowYaw[$i] -= $midYaw; $rowPitch[$i] -= $midPitch } }
# -Smooth: a row for every frame, then each pass a frame's mean with its neighbours (1 2 1).
if ($Smooth -gt 0 -and $n -ge 3) {
    $total = $frames[$n - 1] + 1
    $evenYaw = New-Object double[] $total
    $evenPitch = New-Object double[] $total
    for ($i = 0; $i -lt $n - 1; $i++) {
        $first = $frames[$i]; $next = $frames[$i + 1]
        for ($k = $first; $k -lt $next; $k++) {
            $share = ($k - $first) / [double]($next - $first)
            $evenYaw[$k] = $rowYaw[$i] + ($rowYaw[$i + 1] - $rowYaw[$i]) * $share
            $evenPitch[$k] = $rowPitch[$i] + ($rowPitch[$i + 1] - $rowPitch[$i]) * $share
        }
    }
    $evenYaw[$total - 1] = $rowYaw[$n - 1]; $evenPitch[$total - 1] = $rowPitch[$n - 1]
    for ($pass = 0; $pass -lt $Smooth; $pass++) {
        $wasYaw = $evenYaw.Clone(); $wasPitch = $evenPitch.Clone()
        for ($k = 1; $k -lt $total - 1; $k++) {
            $evenYaw[$k] = 0.25 * $wasYaw[$k - 1] + 0.5 * $wasYaw[$k] + 0.25 * $wasYaw[$k + 1]
            $evenPitch[$k] = 0.25 * $wasPitch[$k - 1] + 0.5 * $wasPitch[$k] + 0.25 * $wasPitch[$k + 1]
        }
    }
    $frames.Clear(); $rowYaw.Clear(); $rowPitch.Clear()
    for ($k = 0; $k -lt $total; $k++) { $frames.Add($k); $rowYaw.Add($evenYaw[$k]); $rowPitch.Add($evenPitch[$k]) }
    $n = $total
}

# How fast it turns at each row (degrees a second, over the rows either side), and its swings: from one turn of direction to the next, 15 degrees or more.
$speed = New-Object double[] $n
$pitchSpeed = New-Object double[] $n
for ($i = 1; $i -lt $n - 1; $i++) {
    $dt = ($frames[$i + 1] - $frames[$i - 1]) * $period / 1000.0
    $speed[$i] = ($rowYaw[$i + 1] - $rowYaw[$i - 1]) / $dt
    $pitchSpeed[$i] = ($rowPitch[$i + 1] - $rowPitch[$i - 1]) / $dt
}
$fastest = 0.0; $fastestPitch = 0.0
for ($i = 0; $i -lt $n; $i++) {
    if ([Math]::Abs($speed[$i]) -gt $fastest) { $fastest = [Math]::Abs($speed[$i]) }
    if ([Math]::Abs($pitchSpeed[$i]) -gt $fastestPitch) { $fastestPitch = [Math]::Abs($pitchSpeed[$i]) }
}
$swingPeaks = New-Object System.Collections.Generic.List[double]
$swingSizes = New-Object System.Collections.Generic.List[double]
$start = 0; $way = 0; $peak = 0.0
for ($i = 1; $i -lt $n; $i++) {
    $now = if ($speed[$i] -gt 30) { 1 } elseif ($speed[$i] -lt -30) { -1 } else { 0 }
    if ($now -ne 0 -and $now -eq $way) { if ([Math]::Abs($speed[$i]) -gt $peak) { $peak = [Math]::Abs($speed[$i]) }; continue }
    if ($now -eq 0 -and $i -lt $n - 1) { continue }  # (slow: the swing may go on)
    if ($way -ne 0) {
        $size = [Math]::Abs($rowYaw[$i - 1] - $rowYaw[$start])
        if ($size -ge 15) { $swingPeaks.Add($peak); $swingSizes.Add($size) }
    }
    $start = $i - 1; $way = $now; $peak = [Math]::Abs($speed[$i])
}
$low = 1e9; $high = -1e9; $lowPitch = 1e9; $highPitch = -1e9
for ($i = 0; $i -lt $n; $i++) {
    if ($rowYaw[$i] -lt $low) { $low = $rowYaw[$i] }; if ($rowYaw[$i] -gt $high) { $high = $rowYaw[$i] }
    if ($rowPitch[$i] -lt $lowPitch) { $lowPitch = $rowPitch[$i] }; if ($rowPitch[$i] -gt $highPitch) { $highPitch = $rowPitch[$i] }
}

# The file.
$outFolder = Split-Path -Parent $Out
if ($outFolder -and -not (Test-Path $outFolder)) { New-Item -ItemType Directory -Path $outFolder -Force | Out-Null }
$text = New-Object System.Text.StringBuilder
[void]$text.AppendLine("# A head track for build\headtest.txt ""track <this file>"": rows ""ms yaw pitch"" (degrees; yaw left positive, pitch up positive).")
[void]$text.AppendLine("# From $([IO.Path]::GetFileName($Log)), $From to $To, its ""View:"" lines: $($times.Count) lines, $n rows (one a frame, a frame $($period.ToString('F3', $inv)) ms).")
if ($Raw) { [void]$text.AppendLine('# The angles as the lines have them (the head in the room).') }
else { [void]$text.AppendLine("# The yaw's middle ($($midYaw.ToString('F2', $inv))) and the pitch's ($($midPitch.ToString('F2', $inv))) taken off: about the head as it is when the test starts.") }
if ($Smooth -gt 0) { [void]$text.AppendLine("# A row for every frame, evened out $Smooth times over (each frame's mean with its neighbours, 1 2 1).") }
[void]$text.AppendLine("# Its fastest turn $($fastest.ToString('F0', $inv)) degrees a second in yaw, $($fastestPitch.ToString('F0', $inv)) in pitch; the head as the mod's filter gave it that session.")
for ($i = 0; $i -lt $n; $i++) {
    [void]$text.AppendLine(('{0} {1} {2}' -f ($frames[$i] * $period).ToString('F3', $inv), $rowYaw[$i].ToString('F2', $inv), $rowPitch[$i].ToString('F2', $inv)))
}
[IO.File]::WriteAllText($Out, $text.ToString(), (New-Object System.Text.UTF8Encoding($false)))

$seconds = $frames[$n - 1] * $period / 1000.0
Write-Host ('{0}: {1} rows over {2:F1} s from {3} lines ({4} of a frame already had, {5} marked "again" left out; a line at most {6:F1} ms off its frame)' -f $Out, $n, $seconds, $times.Count, $doubled, $again, $late)
Write-Host ('  yaw {0:F1} to {1:F1} degrees ({2:F1} from end to end), pitch {3:F1} to {4:F1} ({5:F1}){6}' -f $low, $high, ($high - $low), $lowPitch, $highPitch, ($highPitch - $lowPitch), $(if ($Raw) { '' } else { ', about their middles' }))
Write-Host ('  its fastest: {0:F0} degrees a second in yaw, {1:F0} in pitch' -f $fastest, $fastestPitch)
if ($swingPeaks.Count) {
    $sorted = @($swingPeaks | Sort-Object)
    $sizes = @($swingSizes | Sort-Object)
    $inBand = @($swingPeaks | Where-Object { $_ -ge 450 -and $_ -le 650 }).Count
    Write-Host ('  {0} swings of 15 degrees or more: each one''s fastest {1:F0} to {2:F0} degrees a second (the middle one {3:F0}; {4} of them between 450 and 650); their size {5:F0} to {6:F0} degrees (the middle one {7:F0})' -f
        $swingPeaks.Count, $sorted[0], $sorted[-1], $sorted[[int][Math]::Floor($sorted.Count / 2)], $inBand, $sizes[0], $sizes[-1], $sizes[[int][Math]::Floor($sizes.Count / 2)])
} else { Write-Host '  no swing of 15 degrees or more in it' }
if ($n -lt 20) { exit 1 }
exit 0

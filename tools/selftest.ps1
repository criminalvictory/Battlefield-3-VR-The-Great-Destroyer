# The never-break checks, run after every build once the campaign is in stereo (Battlelog's PLAY
# CAMPAIGN, then tools\resume.ps1; the mod switches to stereo itself), before anyone is called to the headset: what broke on
# 2026-10-30 (the view turning with the gun, the stick turn reversed, the HUD in one eye only, the
# right eye smeared, the frame rate) checked from the game itself with nobody in the headset
# (weapon.txt "standin 1" holds the rifle while the controllers sleep; last, build\freehands.txt
# "test 1" lets go of it and takes it up again by script, and "test 8" has the right hand reload it
# while the left holds it by its handguard alone). One line per check, PASS /
# FAIL / SKIP with its numbers; the exit code is the number of failures. The results also go to
# screenshots\selftest_<time>.txt. Fires nothing.
#   tools\selftest.ps1 [-NoTurn] [-Force]   (-NoTurn: skip the 3 s stick-turn test, which turns the view; -Force: run even when the headset looks worn)
param([switch]$NoTurn, [switch]$Force)
Add-Type -AssemblyName System.Drawing
$root = Split-Path $PSScriptRoot
$log = Join-Path $root 'build\bf3vr.log'
$weapon = Join-Path $root 'build\weapon.txt'
# weapon.txt as it was before the checks: put back at the end with the stand-in off (it's sticky in the game, and left on it throws the gun at any tracking gap:
# the user's "gun kicks"). The checks write test lines into it; until 2026-11-05 they left it holding only "standin 1".
$weaponBefore = if (Test-Path $weapon) { [IO.File]::ReadAllText($weapon) } else { '' }
$depth = Join-Path $root 'build\depth3d.txt'
$expectFile = Join-Path $PSScriptRoot 'selftest.expect'
$results = New-Object System.Collections.Generic.List[object]
function Result([string]$name, [string]$status, [string]$detail) {
    $results.Add([pscustomobject]@{ Check = $name; Status = $status; Detail = $detail })
    Write-Host ('{0,-5} {1,-34} {2}' -f $status, $name, $detail)
}
function LogLines { return @(Get-Content $log) }
function Since([int]$from) { $all = LogLines; if ($all.Count -le $from) { return @() } return @($all[$from..($all.Count - 1)]) }
function WriteFile([string]$path, [string]$text) {
    for ($i = 0; $i -lt 20; $i++) { try { [IO.File]::WriteAllText($path, $text); return } catch { Start-Sleep -Milliseconds 30 } }
}
# The forearms row (6) writes a test arms.txt marked by its first line; a run killed in the middle left
# it (the forearms on, the stand-in's test lines): put back what was there, or nothing.
$arms = Join-Path $root 'build\arms.txt'
$armsBackup = Join-Path $root 'build\arms.selftest.bak'
$armsMarker = '# selftest: put back by tools\selftest.ps1'
if ((Test-Path $arms) -and ((Get-Content $arms -TotalCount 1) -eq $armsMarker)) {
    if (Test-Path $armsBackup) { WriteFile $arms ([IO.File]::ReadAllText($armsBackup)) } else { Remove-Item $arms -ErrorAction SilentlyContinue }
    Remove-Item $armsBackup -ErrorAction SilentlyContinue
}
# The free hands rows (13, 13b) write a test freehands.txt marked by its first line (no key of that file in it: they're matched by name): a run killed in
# the middle left the scripted round going, as does any "test N" left there (a killed build\guntools\freefilm.ps1's has no marker; or one by hand),
# and rows 3-12 would run with the round dropping and taking the rifle. Free hands off first (the rifle back in the right hand), read with the
# stand-in on (with nobody in the headset only its frames run the hands, which read freehands.txt; weapon.txt put back as it was after), then what
# was there put back less any test line, or nothing.
$free = Join-Path $root 'build\freehands.txt'
$freeBackup = Join-Path $root 'build\freehands.selftest.bak'
$freeMarker = '# put back by the never-break checks'
$freeLeft = @(if (Test-Path $free) { Get-Content $free })
$freeMarked = $freeLeft.Count -gt 0 -and $freeLeft[0] -eq $freeMarker
if ($freeMarked -or @($freeLeft -match '^\s*test\b').Count) {
    if (-not $freeMarked) {  # (kept before the marker goes in: a run killed in the wait below finds the marker and puts this back)
        $freePut = @($freeLeft | Where-Object { $_ -notmatch '^\s*test\b' -and $_ -ne $freeMarker }) -join "`r`n"
        if ($freePut.Trim()) { WriteFile $freeBackup $freePut } else { Remove-Item $freeBackup -ErrorAction SilentlyContinue }
    }
    WriteFile $free "$freeMarker`r`noff`r`n"
    $freeGame = [bool](Get-Process bf3 -ErrorAction SilentlyContinue)
    if ($freeGame) { WriteFile $weapon 'standin 1 8 0' }
    Start-Sleep -Milliseconds 1000
    if (Test-Path $freeBackup) { WriteFile $free ([IO.File]::ReadAllText($freeBackup)) } else { Remove-Item $free -ErrorAction SilentlyContinue }
    Remove-Item $freeBackup -ErrorAction SilentlyContinue
    if ($freeGame) { WriteFile $weapon $weaponBefore }
    Write-Host 'freehands.txt held a test (a killed run''s, or one by hand): free hands off, then put back less it'
}
function Stamp([string]$line) {
    if ($line -match '^\[(\d+):(\d+):(\d+)\.(\d+)\]') { return [int]$matches[1] * 3600.0 + [int]$matches[2] * 60 + [int]$matches[3] + [int]$matches[4] / 1000.0 }
    return -1.0
}
function Median([double[]]$v) { if (-not $v -or $v.Count -eq 0) { return [double]::NaN } $s = $v | Sort-Object; return $s[[int][Math]::Floor($s.Count / 2)] }
function Wrap([double]$deg) { while ($deg -gt 180) { $deg -= 360 } while ($deg -lt -180) { $deg += 360 } return $deg }
function SaveEyes {
    $bmp = Join-Path $root 'build\eye_both.bmp'
    $text = Get-Content $depth -Raw
    $n = [int]([regex]::Match($text, 'save (\d+)').Groups[1].Value) + 1
    $before = if (Test-Path $bmp) { (Get-Item $bmp).LastWriteTime } else { [datetime]::MinValue }
    WriteFile $depth ($text -replace 'save \d+', "save $n")
    for ($t = 0; $t -lt 60 -and (-not (Test-Path $bmp) -or (Get-Item $bmp).LastWriteTime -eq $before); $t++) { Start-Sleep -Milliseconds 100 }
    Start-Sleep -Milliseconds 400
    if ((Get-Item $bmp).LastWriteTime -eq $before) { return $null }
    return [Drawing.Bitmap]::FromFile($bmp)
}
# The HUD's light blue in a region of the picture: pixels clearly bluer than they are red.
function HudBlue($img, [int]$x0, [int]$y0, [int]$w, [int]$h) {
    $n = 0
    for ($y = $y0; $y -lt $y0 + $h; $y += 3) {
        for ($x = $x0; $x -lt $x0 + $w; $x += 3) {
            $c = $img.GetPixel($x, $y)
            if ($c.B -gt 150 -and $c.B -gt $c.R + 60 -and $c.G -gt 110) { $n++ }
        }
    }
    return $n
}
# Detail across against detail down: the right eye smeared into streaks had almost none across.
function Streaks($img, [int]$x0, [int]$w) {
    $dx = 0.0; $dy = 0.0; $n = 0
    for ($y = 60; $y -lt $img.Height - 60; $y += 61) {
        for ($x = $x0 + 60; $x -lt $x0 + $w - 60; $x += 61) {
            $c = $img.GetPixel($x, $y); $r = $img.GetPixel($x + 4, $y); $d = $img.GetPixel($x, $y + 4)
            $b = $c.R + $c.G + $c.B
            $dx += [Math]::Abs($b - ($r.R + $r.G + $r.B)); $dy += [Math]::Abs($b - ($d.R + $d.G + $d.B)); $n++
        }
    }
    return ($dx / $n) / [Math]::Max($dy / $n, 0.1)
}
# The lines wanted ($want: pattern, what it is, "same"), in order: each one the first match after the one before; one marked "same" (logged in the
# same frame as the one before it) may also come just before that one. Each one's index in $lines, -1 when it's missing.
function InOrder([string[]]$lines, $want) {
    $hits = @(); $cursor = 0; $start = 0; $last = -1
    foreach ($w in $want) {
        if (-not $w[2]) { $start = $cursor }
        $hit = -1
        for ($i = $start; $i -lt $lines.Count; $i++) { if ($i -ne $last -and $lines[$i] -match $w[0]) { $hit = $i; break } }
        if ($hit -ge 0) { $cursor = [Math]::Max($cursor, $hit + 1) }
        $hits += $hit; $last = $hit
    }
    return , $hits
}
# An offline test program (build\<name>.exe), as rows 8-11 run theirs: its exit code is its number of failures; run from the project's folder.
function Offline([string]$check, [string]$name) {
    $exe = Join-Path $root "build\$name.exe"
    if (-not (Test-Path $exe)) { Result $check 'SKIP' "build\$name.exe not built"; return }
    Push-Location $root
    try { $out = & $exe 2>&1 } finally { Pop-Location }
    Result $check $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
}
$expect = @{}
if (Test-Path $expectFile) { Get-Content $expectFile | ForEach-Object { if ($_ -match '^(\w+)=(.*)$') { $expect[$matches[1]] = $matches[2] } } }

# 1. The game and the mod's parts.
if (-not (Get-Process bf3 -ErrorAction SilentlyContinue)) { Result 'BF3 running' 'FAIL' 'bf3.exe is not running'; exit 1 }
# Nobody in the headset: these checks turn the view and move the stand-in's rifle. The session is at 4 (visible) from the game's start whenever the
# headset is awake, worn or not, 5 (focused) once someone put it on, 3 asleep. So not while it's focused, nor at 4 after a 5 since it last woke (worn
# with the dashboard up), nor while the head moves like a person's ("Head rough": worn 0.09-0.90 mm off a straight line and 4-113 degrees a second;
# lying still 0.01-0.04 mm and 0-2). build\guntools\freefilm.ps1 decides it the same way.
$states = @(Select-String -Path $log -Pattern 'Session state -> (\d+)' | ForEach-Object { [int]$_.Matches[0].Groups[1].Value })
$xrState = if ($states.Count) { $states[-1] } else { 0 }
$focusedSinceWake = $false
for ($k = $states.Count - 1; $k -ge 0 -and $states[$k] -ge 4; $k--) { if ($states[$k] -eq 5) { $focusedSinceWake = $true } }
$rough = @(Select-String -Path $log -Pattern 'Head rough: turning (\d+) degrees a second; .* its place off a straight line by ([\d.]+) mm on average' |
    Select-Object -Last 8 | ForEach-Object { [pscustomobject]@{ turn = [double]$_.Matches[0].Groups[1].Value; off = [double]$_.Matches[0].Groups[2].Value } })
$headMoving = @($rough | Where-Object { $_.off -ge 0.06 -or $_.turn -ge 3 }).Count -gt 0
$worn = if ($xrState -eq 5) { 'the session is focused (state 5): someone is in it' }
        elseif ($xrState -eq 4 -and $focusedSinceWake) { 'it was focused since the headset last woke (5, now 4: worn with the dashboard up?)' }
        elseif ($xrState -ge 4 -and $headMoving) { 'the head moves like a person''s ("Head rough")' }
        else { $null }
if ($worn) {
    if (-not $Force) { Result 'Nobody in the headset' 'FAIL' "$worn; -Force runs the checks anyway"; exit 1 }
    Result 'Nobody in the headset' 'WARN' "$worn; run anyway (-Force)"
}
$all = LogLines
$text = $all -join "`n"
# The window as big as the game draws ("resizing to"), or fitted to the monitor with the game still drawing its size.
$stereoWindow = 'Window: resizing to \d+x\d+|Window: fitted to the monitor, .* the game still draws \d+x\d+'
$startup = @(
    @('Stereo, both eyes side by side', $stereoWindow),
    @('Wide field of view', 'FOV: source [\d.]+, render view 1\d\d'),
    @('God mode', "God mode: the server's soldier is at")
)
foreach ($s in $startup) { if ($text -match $s[1]) { Result $s[0] 'PASS' '' } else { Result $s[0] 'FAIL' "no '$($s[1])' in the log" } }
# The headset's resolution never changes with the desktop window (the user, 2026-11-01: "dont ever change the headset
# resolution unless i absolutely ask"): the game's picture stays two [window] size squares side by side.
$ini = Join-Path $root 'build\bf3vr.ini'
$size = 0
if (Test-Path $ini) {
    $inWindow = $false
    foreach ($l in Get-Content $ini) {
        if ($l -match '^\s*\[(.+)\]') { $inWindow = $matches[1] -eq 'window' }
        elseif ($inWindow -and $l -match '^\s*size\s*=\s*(\d+)') { $size = [int]$matches[1] }
    }
}
$swap = $all | Select-String 'Swapchains (\d+)x(\d+)' | Select-Object -Last 1
if (-not $swap -or $size -le 0) { Result 'Headset resolution' 'SKIP' 'no swap chain line, or no [window] size in bf3vr.ini' }
else {
    $w = [int]$swap.Matches[0].Groups[1].Value; $h = [int]$swap.Matches[0].Groups[2].Value
    if ($w -eq 2 * $size -and $h -eq $size) { Result 'Headset resolution' 'PASS' "${w}x${h}: $size per eye ([window] size)" }
    else { Result 'Headset resolution' 'FAIL' "the game draws ${w}x${h}, not $(2 * $size)x$size ([window] size $size per eye)" }
}
if ($text -match '(MH_CreateHook failed|hooking .* failed)') { Result 'Hooks' 'FAIL' $matches[1] } else { Result 'Hooks' 'PASS' '' }
# 1b. The right grip bound (free hands: the right hand's squeeze holds the rifle, its thumb on the stick shapes the free glove). Each controller
# profile's bindings are counted with the right squeeze and thumb touch in: Index 19, Touch 18, the Frame 20 (17, 16 and 18 without them). A runtime
# that refuses them gets the old set ("accepted without the right grip"): that hand then never lets the rifle go (safe, but no free hands). The
# right hand's own profile is judged when it's known, else all three.
$boundWant = [ordered]@{ 'valve/index_controller' = 19; 'oculus/touch_controller' = 18; 'valve/frame_controller' = 20 }
$rightProfile = $all | Select-String "Controllers: the right hand's profile is /interaction_profiles/(\S+)" | Select-Object -Last 1
$judged = @(if ($rightProfile) { $rightProfile.Matches[0].Groups[1].Value } else { $boundWant.Keys })
if ($rightProfile -and -not $boundWant.Contains($judged[0])) { Result 'Right grip bound' 'SKIP' "the right hand is a $($judged[0]): no grip bound there (the rifle never drops)" }
else {
    $status = 'PASS'; $notes = @()
    foreach ($p in $judged) {
        $short = ($p -split '/')[1] -replace '_controller$', ''
        $line = $all | Select-String ('Controllers: bindings for /interaction_profiles/' + [regex]::Escape($p) + ' accepted( without the right grip)? \((\d+)\)') | Select-Object -Last 1
        if (-not $line) {
            if ($rightProfile) { $status = 'FAIL'; $notes += "$short none accepted" } else { $notes += "$short not offered" }
            continue
        }
        $n = [int]$line.Matches[0].Groups[2].Value
        if ($line.Matches[0].Groups[1].Success) { if ($status -ne 'FAIL') { $status = 'WARN' }; $notes += "$short $n, without the right grip" }
        elseif ($n -ne $boundWant[$p]) { if ($status -ne 'FAIL') { $status = 'WARN' }; $notes += "$short $n, not $($boundWant[$p])" }
        else { $notes += "$short $n" }
    }
    if (-not ($notes | Where-Object { $_ -notmatch 'not offered' })) { $status = 'SKIP' }
    Result 'Right grip bound' $status ($(if ($rightProfile) { "the right hand's profile: " } else { 'the right hand not seen yet: ' }) + ($notes -join ', '))
}
# (The look angles are checked after the stand-in's hand is on, below: without a hand their search waits, and at the start of a run they weren't found yet.)

# 2. Frame rate (the last ten seconds' timing lines).
$timing = @($all | Select-String 'Frame timing \(ms per frame\): whole frame ([\d.]+)' | Select-Object -Last 10 | ForEach-Object { [double]$_.Matches[0].Groups[1].Value })
if ($timing.Count -ge 3) {
    $ms = Median $timing; $fps = 1000.0 / $ms
    $status = if ($fps -ge 70) { 'PASS' } elseif ($fps -ge 60) { 'WARN' } else { 'FAIL' }
    Result 'Frame rate' $status ('{0:F1} fps (median {1:F1} ms; worst {2:F1} ms)' -f $fps, $ms, ($timing | Measure-Object -Maximum).Maximum)
} else { Result 'Frame rate' 'SKIP' 'no timing lines yet' }

# 2b. Anything else busy on the CPU costs frames: a steady 72 is the rule.
$cpuSamples = (Get-Counter '\Process(*)\% Processor Time' -SampleInterval 2 -MaxSamples 1 -ErrorAction SilentlyContinue).CounterSamples
if (-not $cpuSamples) { Result 'Other programs on the CPU' 'SKIP' 'no counters' }
else {
    $others = @($cpuSamples | Where-Object { $_.InstanceName -notmatch '^(_total|idle|bf3|powershell.*|system)$' -and $_.CookedValue -gt 50 } | Sort-Object CookedValue -Descending)
    if ($others.Count) { Result 'Other programs on the CPU' $(if ($others[0].CookedValue -gt 150) { 'FAIL' } else { 'WARN' }) (($others | ForEach-Object { '{0} {1:F0}% of a thread' -f $_.InstanceName, $_.CookedValue }) -join ', ') }
    else { Result 'Other programs on the CPU' 'PASS' 'nothing else above half a thread' }
}

# 3. The view mustn't turn with the gun; the game must aim where it points (the camera follows
# the written look yaw). aimtest turns the hand 20 degrees from 1 s to 3 s, viewlog logs each frame.
# (A hand a little off level: at exactly 0 the aim's search can't tell its angles apart, and with them lost no test lines come.)
WriteFile $weapon 'standin 1 8 0'
Start-Sleep -Seconds 3
$from = (LogLines).Count
WriteFile $weapon "standin 1 8 0`nviewlog 5`nsixlog 5`naimtest 20"
Start-Sleep -Seconds 6
WriteFile $weapon 'standin 1'
# The look angles: found by now (the stand-in's hand has been on 9 s; at the very start of a run the search waits for a hand, so it failed there by its timing),
# and not lost again since the last find.
$allNow = LogLines
if ($allNow -match 'Aim: the look angles found') { Result 'Look angles found' 'PASS' '' } else { Result 'Look angles found' 'FAIL' "no 'Aim: the look angles found' in the log" }
$lastFound = ($allNow | Select-String 'Aim: the look angles found' | Select-Object -Last 1).LineNumber
$after = if ($lastFound) { $allNow | Select-Object -Skip $lastFound } else { @() }
if (-not $lastFound) { Result 'Look angles held' 'SKIP' 'none found' }
elseif ($after -match 'exactly level|too many to nudge|no look angles yet') { Result 'Look angles held' 'FAIL' 'the search is running again (lost them)' } else { Result 'Look angles held' 'PASS' '' }
$lines = Since $from
$t0 = ($lines | Select-String 'Aim: test: the hand' | Select-Object -First 1)
if (-not $t0) { Result 'View still when the gun turns' 'SKIP' 'the aim test did not start' }
else {
    $t0 = Stamp $t0.Line
    $before = @(); $during = @()
    foreach ($l in $lines) {
        if ($l -match 'View: hand ([-\d.]+) written ([-\d.]+) \| camera ([-\d.]+) at .* \| head ([-\d.]+) shown ([-\d.]+)') {
            $t = (Stamp $l) - $t0
            $row = [pscustomobject]@{ written = [double]$matches[2]; camera = [double]$matches[3]; offset = Wrap ([double]$matches[5] - [double]$matches[4]) }
            if ($t -gt 0.1 -and $t -lt 0.9) { $before += $row } elseif ($t -gt 1.3 -and $t -lt 2.8) { $during += $row }
        }
    }
    if ($before.Count -lt 5 -or $during.Count -lt 5) { Result 'View still when the gun turns' 'SKIP' "too few view lines ($($before.Count) before, $($during.Count) during): is the right hand (or the stand-in) aiming?" }
    else {
        $dw = Wrap ((Median ($during | ForEach-Object written)) - (Median ($before | ForEach-Object written)))
        $do = Wrap ((Median ($during | ForEach-Object offset)) - (Median ($before | ForEach-Object offset)))
        $ok = [Math]::Abs([Math]::Abs($dw) - 20) -lt 3 -and [Math]::Abs($do) -lt 1.5
        Result 'View still when the gun turns' $(if ($ok) { 'PASS' } else { 'FAIL' }) ('look yaw written {0:+0.0;-0.0} deg, view {1:+0.00;-0.00} deg (must stay ~0)' -f $dw, $do)
        # And frame by frame, the head still: the world's turn (shown + head) and the cameras' place, whatever the look yaw does. The game's eye swings 1.3 mm a degree round the soldier
        # (2.7 cm by this test), and a vehicle's offset left on foot turned the whole world with the gun (2026-11-04: the user's jitter).
        $bodies = @(); $places = @()
        foreach ($l in $lines) {
            if ($l -match 'View: hand .* \| head ([-\d.]+) shown ([-\d.]+)') { $bodies += [double]$matches[2] + [double]$matches[1] }
            elseif ($l -match 'Six: .* world ([-\d.]+) ([-\d.]+) ([-\d.]+) \| eye ([-\d.]+) ([-\d.]+) ([-\d.]+) \|') { $places += , @(([double]$matches[1] + [double]$matches[4]), ([double]$matches[2] + [double]$matches[5]), ([double]$matches[3] + [double]$matches[6])) }
        }
        $turned = 0.0
        for ($i = 1; $i -lt $bodies.Count; $i++) { $turned = [Math]::Max($turned, [Math]::Abs((Wrap ($bodies[$i] - $bodies[$i - 1])))) }
        Result 'World still when the gun turns' $(if ($bodies.Count -lt 20) { 'SKIP' } elseif ($turned -lt 0.03) { 'PASS' } else { 'FAIL' }) ('{0} frames, the whole world turned {1:F3} deg at most from one to the next' -f $bodies.Count, $turned)
        $apart = 0.0
        for ($i = 1; $i -lt $places.Count; $i++) { $d = 0.0; foreach ($k in 0..2) { $d += ($places[$i][$k] - $places[0][$k]) * ($places[$i][$k] - $places[0][$k]) }; $apart = [Math]::Max($apart, [Math]::Sqrt($d)) }
        Result 'Cameras still when the gun turns' $(if ($places.Count -lt 20) { 'SKIP' } elseif ($apart -lt 0.004) { 'PASS' } else { 'FAIL' }) ('{0} frames, the cameras {1:F1} mm at most from where they were (the game''s eye swings 27)' -f $places.Count, ($apart * 1000))
    }
    # Where the game fires: its weapon camera, built from the look angles, against the yaw written
    # (the "camera" of the view lines is the render camera before the mod's turn, which carries the
    # hand's turn by design). The test's own lines (aim.cpp, from 2026-10-31) or the once-a-second ones.
    $pairs = @()
    foreach ($l in $lines) {
        if ($l -match 'Aim test: written ([-\d.]+), weapon camera ([-\d.]+)') { $pairs += , @([double]$matches[1], [double]$matches[2], ((Stamp $l) - $t0)) }
    }
    if ($pairs.Count -eq 0) {
        $written = $null
        foreach ($l in $lines) {
            if ($l -match 'Aim: yaw at \S+ = ([-\d.]+)') { $written = [double]$matches[1] }
            elseif ($l -match 'weapon camera as the game wrote it: yaw ([-\d.]+)' -and $null -ne $written) { $pairs += , @($written, [double]$matches[1], ((Stamp $l) - $t0)) }
        }
    }
    $inTest = @($pairs | Where-Object { $_[2] -gt 1.2 -and $_[2] -lt 2.9 })
    if ($inTest.Count -eq 0) { Result 'Game aims where the gun points' 'SKIP' 'no weapon camera reading during the test' }
    else {
        $best = ($inTest | ForEach-Object { [Math]::Abs((Wrap (($_[1] - $_[0]) * 57.29578))) } | Measure-Object -Minimum).Minimum
        $ok = $best -lt 1.0
        Result 'Game aims where the gun points' $(if ($ok) { 'PASS' } else { 'FAIL' }) ('weapon camera within {0:F2} deg of the yaw written while the gun was turned ({1} readings)' -f $best, $inTest.Count)
    }
}

# 4. The stick's turn direction (F17: 3 s of the stick held right). The first run records the
# direction the user confirmed (2026-10-30 22:00: right turns right) in tools\selftest.expect.
if ($NoTurn) { Result 'Stick turn direction' 'SKIP' '-NoTurn' }
else {
    $from = (LogLines).Count
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'drawhunt.ps1') -Action f17 | Out-Null
    Start-Sleep -Seconds 5
    $nets = @(Since $from | Select-String 'Turn \(shown\): .* net ([-+\d.]+) degrees' | ForEach-Object { [double]$_.Matches[0].Groups[1].Value })
    $net = ($nets | Measure-Object -Sum).Sum
    if ($nets.Count -eq 0 -or [Math]::Abs($net) -lt 20) { Result 'Stick turn direction' 'SKIP' ('no turn measured (net {0:F1} deg): game not in front?' -f $net) }
    else {
        $sign = [Math]::Sign($net)
        if (-not $expect.ContainsKey('turn_right_sign')) {
            Add-Content $expectFile "turn_right_sign=$sign"
            Result 'Stick turn direction' 'PASS' ('recorded: right turns the shown yaw {0:+0;-0} ({1:F0} deg in 3 s)' -f $sign, $net)
        } else {
            $ok = [int]$expect['turn_right_sign'] -eq $sign
            Result 'Stick turn direction' $(if ($ok) { 'PASS' } else { 'FAIL' }) ('{0:F0} deg in 3 s (expected sign {1})' -f $net, $expect['turn_right_sign'])
        }
    }
}

# 5. Both eyes alike: the HUD's corners in each (twice, 4 s apart: a weapon switch's animation
# draws the gun differently in the two eyes for a few seconds, 2026-10-30), and no streaks.
$img = SaveEyes
if (-not $img) { Result 'Both eyes alike' 'SKIP' 'no picture saved' }
else {
    $w = $img.Width / 2; $h = $img.Height
    $corners = @(@('HUD bottom left', 0), @('HUD bottom right', ($w - 600)))
    $first = @{}
    foreach ($corner in $corners) {
        $l = HudBlue $img $corner[1] ($h - 300) 600 300
        $r = HudBlue $img ($w + $corner[1]) ($h - 300) 600 300
        $first[$corner[0]] = @($l, $r)
    }
    $again = $null
    foreach ($corner in $corners) {
        $l, $r = $first[$corner[0]]
        $ok = [Math]::Abs($l - $r) -lt [Math]::Max(60, 0.4 * [Math]::Max($l, $r))
        if (-not $ok) {
            if (-not $again) { $img.Dispose(); Start-Sleep -Seconds 4; $again = SaveEyes; $img = $again }
            if ($again) {
                $l = HudBlue $again $corner[1] ($h - 300) 600 300
                $r = HudBlue $again ($w + $corner[1]) ($h - 300) 600 300
                $ok = [Math]::Abs($l - $r) -lt [Math]::Max(60, 0.4 * [Math]::Max($l, $r))
            }
        }
        # A warning only: the HUD is see-through, and something dark close behind it in one eye (the
        # gun, with the eyes' parallax) darkens it there (2026-10-31). The HUD check that counts is
        # the overlay's own (below).
        Result $corner[0] $(if ($ok) { 'PASS' } else { 'WARN' }) "blue HUD pixels: left eye $l, right eye $r"
    }
    $hud = $all | Select-String 'Overlay: last second (\d+) HUD draws in the left eye, (\d+) copied into the right' | Select-Object -Last 1
    if (-not $hud) { Result 'HUD in both eyes' 'SKIP' 'no overlay count in the log (an older build?)' }
    else {
        $d = [int]$hud.Matches[0].Groups[1].Value; $c = [int]$hud.Matches[0].Groups[2].Value
        Result 'HUD in both eyes' $(if ($d -gt 0 -and $d -eq $c) { 'PASS' } else { 'FAIL' }) "last second: $d HUD draws in the left eye, $c copied into the right"
    }
    $sl = Streaks $img 0 $w; $sr = Streaks $img $w $w
    $ok = $sl -gt 0.45 -and $sr -gt 0.45
    Result 'No streaks in either eye' $(if ($ok) { 'PASS' } else { 'FAIL' }) ('detail across/down: left {0:F2}, right {1:F2} (streaks ~0.03)' -f $sl, $sr)
    $img.Dispose()
}

# 6. Wrist movement (on every level; build\arms.txt holds tests and tuning): the stand-in's left hand moved
# into reach, its wrist turned straight along the forearm solved for it, then bent 30 degrees of
# flexion about the wrist joint; the once-a-second readings must say so. arms.txt is put back as it
# was (no file: none again).
if (-not (Get-Process bf3 -ErrorAction SilentlyContinue)) { Result 'Forearms' 'SKIP' 'bf3.exe is not running' }
elseif ($text -notmatch $stereoWindow) { Result 'Forearms' 'SKIP' 'stereo is not on' }
else {
    $hadArms = Test-Path $arms
    $armsBefore = if ($hadArms) { [IO.File]::ReadAllText($arms) } else { $null }
    $from = (LogLines).Count
    try {
        # The marker line lets the next run put a file left by a killed run back (top of this script).
        if ($hadArms) { WriteFile $armsBackup $armsBefore }
        WriteFile $arms "$armsMarker`nlog 1`ntest move -0.25 0.05 0.35`ntest neutral`ntest bend 30 0 0`n"
        Start-Sleep -Seconds 4
    } finally {
        if ($hadArms) { WriteFile $arms $armsBefore } else { Remove-Item $arms -ErrorAction SilentlyContinue }
        Remove-Item $armsBackup -ErrorAction SilentlyContinue
    }
    $lines = @(Since $from)
    $readings = @(); $unread = 0
    # Case-sensitive: The Great Destroyer's "Bare arms: left wrist bent ..." lines aren't the gloves' readings.
    foreach ($l in @($lines | Where-Object { $_ -cmatch 'Arms: left' })) {
        if ($l -match 'Arms: left reach ([\d.]+)/[\d.]+ bend ([\d.]+) \(flexion ([-+]?[\d.]+) ulnar ([-+]?[\d.]+)\) twist ([-+]?[\d.]+)') {
            $readings += [pscustomobject]@{ reach = [double]$matches[1]; flexion = [double]$matches[3]; ulnar = [double]$matches[4]; standIn = $l -match "the stand-in's hand" }
        } else { $unread++ }  # not numbers (nan, inf)
    }
    $ours = @($readings | Where-Object standIn)
    if (-not ($lines -match 'Arms: ')) { Result 'Forearms' 'SKIP' 'no Arms lines in the log after writing arms.txt (an older build?)' }
    elseif ($readings.Count -eq 0 -and $unread -eq 0) { Result 'Forearms' 'FAIL' "no 'Arms: left' lines: is the stand-in's left hand drawn (weapon.txt standin 1, the left controller asleep)?" }
    elseif ($readings.Count -gt 0 -and $ours.Count -eq 0 -and $unread -eq 0) { Result 'Forearms' 'SKIP' "the left hand is a real controller's (the test moves the stand-in's)" }
    else {
        $off = @($ours | Where-Object { [Math]::Abs($_.flexion - 30) -gt 1 })
        $ok = $ours.Count -gt 0 -and $unread -eq 0 -and $off.Count -eq 0
        $flex = ($ours | ForEach-Object { '{0:+0.0;-0.0}' -f $_.flexion }) -join ' '
        Result 'Forearms' $(if ($ok) { 'PASS' } else { 'FAIL' }) ("{0} readings of the stand-in's hand, flexion {1} (30 +-1 wanted), {2} not numbers; reach {3:F2} m" -f $ours.Count, $flex, $unread, $(if ($ours.Count) { $ours[-1].reach } else { 0 }))
    }
}

# 7. Errors in the log since the game started.
$errors = @($all | Select-String -Pattern 'fail|error|exception' | Where-Object { $_.Line -notmatch "couldn't|didn't fire|without|no rounds" })
Result 'Log errors' $(if ($errors.Count -eq 0) { 'PASS' } else { 'WARN' }) $(if ($errors.Count) { "$($errors.Count): $($errors[-1].Line)" } else { '' })

# 8. The glove against the gun, offline (tools/collidetest.cpp), when it's built.
$collide = Join-Path $root 'build\collidetest.exe'
if (Test-Path $collide) {
    $out = & $collide 2>&1
    Result 'Glove collision (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Glove collision (offline)' 'SKIP' 'build\collidetest.exe not built' }

# 9. The magazine from the pouch at the pelvis pushed into the well, offline (tools/magtest.cpp).
$magtest = Join-Path $root 'build\magtest.exe'
if (Test-Path $magtest) {
    $out = & $magtest 2>&1
    Result 'Magazine by hand (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Magazine by hand (offline)' 'SKIP' 'build\magtest.exe not built' }
# The other guns' magazines, levers, lid and belt by hand, offline (tools/heldmagtest.cpp).
$heldmagtest = Join-Path $root 'build\heldmagtest.exe'
if (Test-Path $heldmagtest) {
    $out = & $heldmagtest 2>&1
    Result 'Other guns by hand (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Other guns by hand (offline)' 'SKIP' 'build\heldmagtest.exe not built' }

# 9b. The launcher loaded by the left hand, offline (tools/rockettest.cpp).
$rockettest = Join-Path $root 'build\rockettest.exe'
if (Test-Path $rockettest) {
    $out = & $rockettest 2>&1
    Result 'Launcher by hand (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Launcher by hand (offline)' 'SKIP' 'build\rockettest.exe not built' }

# 10. The forearm placed from its hand, offline (tools/armtest.cpp): src/armik.h's solver.
$armtest = Join-Path $root 'build\armtest.exe'
if (Test-Path $armtest) {
    $out = & $armtest 2>&1
    Result 'Arm IK (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Arm IK (offline)' 'SKIP' 'build\armtest.exe not built' }

# 11. The left hand steering the gun while it holds the handguard, offline (tools/steertest.cpp):
# src/steer.h on scripted hands (the rifle had followed a left hand beside or behind the right, 2026-10-31).
$steertest = Join-Path $root 'build\steertest.exe'
if (Test-Path $steertest) {
    $out = & $steertest 2>&1
    Result 'Two-hand steer (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Two-hand steer (offline)' 'SKIP' 'build\steertest.exe not built' }

# 11b. Free hands, offline: who holds the rifle (tools/grasptest.cpp: src/grasp.h on scripted hands, every row also run mirrored), the right
# glove's bones and poses as the left's twins (tools/twintest.cpp: handpose's mirror kit), loose things with the rifle as one of them
# (tools/loosetest.cpp) and the grenade launcher by hand (tools/m320test.cpp): the last two guard work the user confirmed in the headset.
Offline 'Who holds the rifle (offline)' 'grasptest'
Offline 'Right glove twins (offline)' 'twintest'
Offline 'Loose things (offline)' 'loosetest'
Offline 'Grenade launcher by hand (offline)' 'm320test'
# 11c. The charging handle and the bolt release by either hand, offline (tools/reloadtest.cpp: src/reload.cpp on scripted hands; its first row runs
# the left alone frame for frame against the reload of before, kept in tools/reloadtest_old.inc).
Offline 'Charging handle and bolt release by either hand (offline)' 'reloadtest'

# 12. The head's place moving the view (src/headmove.h): offline on scripted heads and a simulated soldier, then in the
# game: the walking stick read (the left stick walked nothing in runs started with the controllers asleep, 2026-11-01), and
# a head faked 20 cm to the right: the camera's move along the body's right, nothing up or back, the gun's draws not asked.
$headtest = Join-Path $root 'build\headmovetest.exe'
if (Test-Path $headtest) {
    $out = & $headtest 2>&1
    Result 'Head moves the view (offline)' $(if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }) ("$LASTEXITCODE failures; " + ($out | Select-Object -Last 1))
} else { Result 'Head moves the view (offline)' 'SKIP' 'build\headmovetest.exe not built' }
if ((LogLines) -match 'Pad: the game reads controller 0') { Result 'Walking stick read' 'PASS' '' }
else { Result 'Walking stick read' 'FAIL' "no 'Pad: the game reads controller 0' in the log: walking is on W/A/S/D" }
if (Get-Process bf3 -ErrorAction SilentlyContinue) {
    $keep = [IO.File]::ReadAllText($weapon)
    $from = (LogLines).Count
    WriteFile $weapon "standin 1`r`nsixwalk 0`r`nsixlog 4`r`nheadtest 0.2 0 0`r`n"
    Start-Sleep 3
    $six = @(Since $from | Select-String 'Six: head \S+ \S+ \S+\s+raw \S+ \S+ \S+ \| room (\S+) (\S+) (\S+) world' | Select-Object -Last 3)
    WriteFile $weapon "standin 1`r`nsixwalk 1`r`nheadtest 0 0 0`r`n"
    Start-Sleep 1
    WriteFile $weapon "standin 1`r`nsixwalk 1`r`nheadtest off`r`n"
    Start-Sleep 1
    WriteFile $weapon $keep
    if ($six.Count -eq 0) { Result 'Head moves the view' 'SKIP' 'no Six lines: no view turned (a cutscene?)' }
    else {
        $g = $six[-1].Matches[0].Groups
        $ok = [Math]::Abs([double]$g[1].Value - 0.2) -lt 0.002 -and [Math]::Abs([double]$g[2].Value) -lt 0.002 -and [Math]::Abs([double]$g[3].Value) -lt 0.002
        Result 'Head moves the view' $(if ($ok) { 'PASS' } else { 'FAIL' }) "head faked 0.2 m right: the view $($g[1].Value) right, $($g[2].Value) up, $($g[3].Value) back"
    }
}

# 13. Free hands: build\freehands.txt "test 1", hands.cpp's scripted round (src/freetest.h, 26 s) with a scripted right hand in the stand-in's
# place and a scripted left one: the right lets go (the rifle falls and lands with the game's own sound), the left takes it up by its pistol grip,
# the right takes the handguard, the left lets go (held by the right alone), takes the pistol grip back and drops it; the right takes it up, the
# left the handguard, the right lets go (held by the left alone) and takes it back. Its lines must come in order, with nothing not finite, no gun
# jump and loose never out of room. Then "off" (the rifle back in the right hand, before the round's next let-go 27 s in) and freehands.txt put
# back as it was (no file: none again; never a test line). Rows 3-12 ran first with the stand-in's right hand holding as always: the stand-in
# never lets go, so a change of hands there is a failure (and a SKIP up there now may mean the rifle left its hand).
$freeWant = @(
    @('Free hands: the right hand let go of the rifle''s pistol grip: it falls', 'the right let go: it falls', $false),
    @('Loose: m416gun let go of', 'loose took the rifle', $true),
    @('Loose: the rifle hit the ground at .*: land_', 'it landed with the game''s sound', $false),
    @('Free hands: the left hand picked the rifle up', 'the left took it up', $false),
    @('Free hands: the rifle in the left hand', 'in the left hand', $false),
    @('Hands: the right hand took hold of the handguard', 'the right took the handguard', $false),
    @('Free hands: the left hand let go of the rifle''s pistol grip: held by the right hand on its handguard alone', 'held by the right alone', $false),
    @('Free hands: the left hand took the rifle''s pistol grip from the right hand''s hold on its handguard', 'the left took the pistol grip back', $false),
    @('Hands: the right hand let go of the handguard', 'the right let go of the handguard', $false),
    @('Free hands: the left hand let go of the rifle''s pistol grip: it falls', 'the left let go: it falls', $false),
    @('Loose: m416gun let go of', 'loose took the rifle', $true),
    @('Free hands: the right hand picked the rifle up', 'the right took it up', $false),
    @('Hands: the left hand took hold of the (handguard|grenade launcher''s grip)', 'the left took the handguard', $false),
    @('Free hands: the right hand let go of the rifle''s pistol grip: held by the left hand on its handguard alone', 'held by the left alone', $false),
    @('Free hands: the right hand took the rifle''s pistol grip from the left hand''s hold on its handguard', 'the right took the pistol grip back', $false))
if (-not (Get-Process bf3 -ErrorAction SilentlyContinue)) { Result 'Free hands round (test 1)' 'SKIP' 'bf3.exe is not running' }
elseif ($text -notmatch $stereoWindow) { Result 'Free hands round (test 1)' 'SKIP' 'stereo is not on' }
else {
    WriteFile $weapon "standin 1 -55 0`r`n"
    Start-Sleep -Seconds 2
    $now = LogLines
    $from = $now.Count  # (the round's lines from here: the head's turn down isn't the round's)
    $early = if ($from -gt $all.Count) { @($now[$all.Count..($from - 1)] | Where-Object { $_ -match 'Free hands: the \w+ hand (let go of|picked|took) the rifle' }) } else { @() }
    Result 'Stand-in keeps the rifle' $(if ($early.Count) { 'FAIL' } else { 'PASS' }) $(if ($early.Count) { "$($early.Count) changes of hands in rows 3-12, the first: $($early[0])" } else { 'no change of hands in rows 3-12' })
    $freeKeep = if (Test-Path $free) { (@(Get-Content $free) | Where-Object { $_ -notmatch '^\s*test\b' -and $_ -ne $freeMarker }) -join "`r`n" } else { '' }
    if ($freeKeep.Trim()) { WriteFile $freeBackup $freeKeep }
    $freeOff = $false
    try {
        # Free hands off 0.7 s first, as the films' rounds start (the rifle back in the right hand, the round's clock from 0): "test 1" written over
        # a "test 1" already read would log nothing.
        WriteFile $free "$freeMarker`r`noff`r`n"
        Start-Sleep -Milliseconds 700
        WriteFile $free "$freeMarker`r`ntest 1`r`n"
        Start-Sleep -Seconds 3
        $lines = @(Since $from)
        $on = $lines | Select-String 'Free hands: test 1 on' | Select-Object -First 1
        if ($on) {
            # The round's clock by its first moment (the right's let-go, 1.04 s in): off 25.6 s in, before the next round's let-go at 27.04.
            $first = $lines | Select-Object -Skip ($on.LineNumber - 1) | Select-String 'Free hands: the right hand let go of the rifle''s pistol grip' | Select-Object -First 1
            $zero = if ($first) { (Stamp $first.Line) - 1.04 } else { Stamp $on.Line }
            $wait = [Math]::Min(26.0, $zero + 25.6 - (Get-Date).TimeOfDay.TotalSeconds)
            if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
        }
        WriteFile $free "$freeMarker`r`noff`r`n"
        $freeOff = $true
        Start-Sleep -Seconds 1
    } finally {
        if (-not $freeOff) { WriteFile $free "$freeMarker`r`noff`r`n"; Start-Sleep -Milliseconds 700 }
        if ($freeKeep.Trim()) { WriteFile $free ($freeKeep + "`r`n") } else { Remove-Item $free -ErrorAction SilentlyContinue }
        Remove-Item $freeBackup -ErrorAction SilentlyContinue
    }
    Start-Sleep -Seconds 1
    $lines = @(Since $from)
    $on = $lines | Select-String 'Free hands: test 1 on' | Select-Object -First 1
    if ($on) { $lines = @($lines | Select-Object -Skip ($on.LineNumber - 1)) }  # (the round's own lines: the "off" before it isn't the round's)
    if (-not $on) {
        $built = ($text -match 'Free hands: ') -or ($lines -match 'Free hands: ')
        Result 'Free hands round (test 1)' $(if ($built) { 'FAIL' } else { 'SKIP' }) $(if ($built) { "no 'Free hands: test 1 on' in the log: freehands.txt wasn't read" } else { "no 'Free hands:' lines in the log (an older build?)" })
    } else {
        $hits = InOrder $lines $freeWant
        $first = $lines | Select-String 'Free hands: the right hand let go of the rifle''s pistol grip' | Select-Object -First 1
        $zero = if ($first) { (Stamp $first.Line) - 1.04 } else { Stamp $on.Line }
        $found = @($hits | Where-Object { $_ -ge 0 })
        $freeMissing = @(for ($k = 0; $k -lt $hits.Count; $k++) { if ($hits[$k] -lt 0) { $freeWant[$k][1] } })
        $bad = @($lines | Where-Object { $_ -match 'not finite|the gun jumped|had no room' })
        $detail = '{0} of {1} in order' -f $found.Count, $freeWant.Count
        if ($found.Count) { $detail += (', the last {0:F1} s in' -f ((Stamp $lines[[int]($found | Measure-Object -Maximum).Maximum]) - $zero)) }
        if ($first) { $detail += ('; the first {0:F2} s after "test 1 on" (1.04 wanted)' -f ((Stamp $first.Line) - (Stamp $on.Line))) }
        if ($freeMissing.Count) { $detail += '; missing: ' + ($freeMissing -join ', ') }
        if ($bad.Count) { $detail += "; $($bad.Count) lines that mustn't be there, the first: $($bad[0])" }
        Result 'Free hands round (test 1)' $(if ($freeMissing.Count -eq 0 -and $bad.Count -eq 0) { 'PASS' } else { 'FAIL' }) $detail
    }
    $off = @(if ($on) { $lines | Where-Object { $_ -match 'Free hands: tests off' } })
    $put = if ($freeKeep.Trim()) { 'freehands.txt put back as it was, less any test' } else { 'freehands.txt deleted' }
    Result 'Free hands tests off' $(if ($off.Count) { 'PASS' } elseif ($on) { 'FAIL' } else { 'SKIP' }) $(if ($off.Count) { "after the round; $put" } else { "no 'Free hands: tests off' after the round; $put" })
}

# 13b. Either hand: build\freehands.txt "test 8" (src/freetest.h, 16 s), the tactical reload by the right hand while the left holds the rifle by its
# handguard alone: the left takes the handguard and the right lets go of the pistol grip; the right takes the seated magazine (that squeeze is the
# magazine's, never a regrip), draws it out and lets it fall, takes a fresh one at the belly, catches it under the well and pushes it home, and pulls
# the charging handle; then it takes the pistol grip back, not before 11.9 s into the round, and the left lets go of the handguard. Its lines must come
# in order, with nothing not finite, no gun jump, loose never out of room and no magazine in a hand gone; then "off" (15.5 s in, after the left's let-go
# at 14 s and before the next round's left takes the handguard at 17 s) must log "tests off", and freehands.txt is put back as it was (no file: none
# again; never a test line).
$eitherWant = @(
    @('Hands: the left hand took hold of the (handguard|grenade launcher''s grip)', 'the left took the handguard', $false),
    @('Free hands: the right hand let go of the rifle''s pistol grip: held by the left hand on its handguard alone', 'held by the left alone', $false),
    @('Pouch: the right hand has the seated magazine', 'the right took the seated magazine', $false),
    @('Pouch: the magazine drawn out by hand', 'drawn out', $false),
    @('Magazine: let go of, falling from the right hand', 'let go of: it fell', $false),
    @('Pouch: the right grip squeezed .*took a magazine', 'the right took a fresh one at the belly', $false),
    @('Pouch: the magazine caught under the well', 'caught under the well', $false),
    @('Pouch: the magazine pushed home', 'pushed home', $false),
    @('Reload: charging handle grabbed by the right hand', 'the right grabbed the charging handle', $false),
    @('Reload: charging handle let go by the right hand', 'the right let go of it', $false),
    @('Free hands: the right hand took the rifle''s pistol grip from the left hand''s hold on its handguard', 'the right took the pistol grip back', $false),
    @('Free hands: the rifle in the right hand', 'in the right hand', $false),
    @('Hands: the left hand let go of the (handguard|grenade launcher''s grip)', 'the left let go of the handguard', $false))
if (-not (Get-Process bf3 -ErrorAction SilentlyContinue)) { Result 'Either hand round (test 8)' 'SKIP' 'bf3.exe is not running' }
elseif ($text -notmatch $stereoWindow) { Result 'Either hand round (test 8)' 'SKIP' 'stereo is not on' }
else {
    WriteFile $weapon "standin 1 -55 0`r`n"
    Start-Sleep -Seconds 2
    $from = (LogLines).Count
    $freeKeep = if (Test-Path $free) { (@(Get-Content $free) | Where-Object { $_ -notmatch '^\s*test\b' -and $_ -ne $freeMarker }) -join "`r`n" } else { '' }
    if ($freeKeep.Trim()) { WriteFile $freeBackup $freeKeep }
    $freeOff = $false
    try {
        # Free hands off 0.7 s first (the rifle back in the right hand, the round's clock from 0), as row 13.
        WriteFile $free "$freeMarker`r`noff`r`n"
        Start-Sleep -Milliseconds 700
        WriteFile $free "$freeMarker`r`ntest 8`r`n"
        Start-Sleep -Milliseconds 3500
        $lines = @(Since $from)
        $on = $lines | Select-String 'Free hands: test 8 on' | Select-Object -First 1
        if ($on) {
            # The round's clock by the right's let-go (2.04 s in): off 15.5 s in.
            $first = $lines | Select-Object -Skip ($on.LineNumber - 1) | Select-String 'Free hands: the right hand let go of the rifle''s pistol grip' | Select-Object -First 1
            $zero = if ($first) { (Stamp $first.Line) - 2.04 } else { Stamp $on.Line }
            $wait = [Math]::Min(17.0, $zero + 15.5 - (Get-Date).TimeOfDay.TotalSeconds)
            if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
        }
        WriteFile $free "$freeMarker`r`noff`r`n"
        $freeOff = $true
        Start-Sleep -Seconds 1
    } finally {
        if (-not $freeOff) { WriteFile $free "$freeMarker`r`noff`r`n"; Start-Sleep -Milliseconds 700 }
        if ($freeKeep.Trim()) { WriteFile $free ($freeKeep + "`r`n") } else { Remove-Item $free -ErrorAction SilentlyContinue }
        Remove-Item $freeBackup -ErrorAction SilentlyContinue
    }
    Start-Sleep -Seconds 1
    $lines = @(Since $from)
    $on = $lines | Select-String 'Free hands: test 8 on' | Select-Object -First 1
    if ($on) { $lines = @($lines | Select-Object -Skip ($on.LineNumber - 1)) }  # (the round's own lines: the "off" before it isn't the round's)
    $put = if ($freeKeep.Trim()) { 'freehands.txt put back as it was, less any test' } else { 'freehands.txt deleted' }
    if (-not $on) {
        $none = $lines | Select-String 'Free hands: no test 8' | Select-Object -First 1
        $built = ($text -match 'Free hands: ') -or ($lines -match 'Free hands: ')
        if ($none) { Result 'Either hand round (test 8)' 'SKIP' "this build has no test 8 (an older one?): $($none.Line)" }
        else { Result 'Either hand round (test 8)' $(if ($built) { 'FAIL' } else { 'SKIP' }) $(if ($built) { "no 'Free hands: test 8 on' in the log: freehands.txt wasn't read" } else { "no 'Free hands:' lines in the log (an older build?)" }) }
    } else {
        $hits = InOrder $lines $eitherWant
        $first = $lines | Select-String 'Free hands: the right hand let go of the rifle''s pistol grip' | Select-Object -First 1
        $zero = if ($first) { (Stamp $first.Line) - 2.04 } else { Stamp $on.Line }
        $found = @($hits | Where-Object { $_ -ge 0 })
        $eitherMissing = @(for ($k = 0; $k -lt $hits.Count; $k++) { if ($hits[$k] -lt 0) { $eitherWant[$k][1] } })
        $bad = @($lines | Where-Object { $_ -match 'not finite|the gun jumped|had no room|in the hand is gone' })
        # The right's squeezes on the seated magazine (3.2 s in) and at the handle are the magazine's and the handle's (grasp's claim): its first take of the
        # pistol grip comes at 12.0 s, never before 11.9.
        $regrip = $lines | Select-String 'Free hands: the right hand (took the rifle''s pistol grip|picked the rifle up)' | Select-Object -First 1
        $regripAt = if ($regrip) { (Stamp $regrip.Line) - $zero } else { -1.0 }
        $early = $regrip -and $regripAt -lt 11.9
        $off = @($lines | Where-Object { $_ -match 'Free hands: tests off' })
        $detail = '{0} of {1} in order' -f $found.Count, $eitherWant.Count
        if ($found.Count) { $detail += (', the last {0:F1} s in' -f ((Stamp $lines[[int]($found | Measure-Object -Maximum).Maximum]) - $zero)) }
        if ($regrip) { $detail += ('; the right took the pistol grip back {0:F2} s in ({1})' -f $regripAt, $(if ($early) { 'too early: not before 11.9' } else { '12.0 wanted' })) }
        if ($eitherMissing.Count) { $detail += '; missing: ' + ($eitherMissing -join ', ') }
        if ($bad.Count) { $detail += "; $($bad.Count) lines that mustn't be there, the first: $($bad[0])" }
        $detail += $(if ($off.Count) { "; then tests off; $put" } else { "; no 'Free hands: tests off' after it; $put" })
        Result 'Either hand round (test 8)' $(if ($eitherMissing.Count -eq 0 -and $bad.Count -eq 0 -and -not $early -and $off.Count) { 'PASS' } else { 'FAIL' }) $detail
    }
}

$weaponAfter = if ($weaponBefore -match '(?m)^standin\b') { $weaponBefore -replace '(?m)^standin\b[^\r\n]*', 'standin 0' } else { "standin 0`r`n" + $weaponBefore }
WriteFile $weapon $weaponAfter
Write-Host 'weapon.txt put back as it was, the stand-in off'

$failed = @($results | Where-Object Status -eq 'FAIL').Count
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$report = Join-Path $root "screenshots\selftest_$stamp.txt"
$results | Format-Table -AutoSize | Out-String -Width 220 | Set-Content $report -Encoding utf8
Write-Host "$failed failed; written to $report"
exit $failed

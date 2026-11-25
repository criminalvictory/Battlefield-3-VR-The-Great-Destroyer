# The look angles' searches of a run, from its log: each time the look was lost (a new soldier, a load, a cutscene, the look gone from the camera) to
# its find, how long that took counting the frames alone (a "without a frame here" gap, the headset off or the game in a menu, left out), from the new
# soldier and from the first search after it, how many searches, what held them up (level or straight ahead, too many to nudge, nothing near), and
# which way it was found: again by an address that proved itself (quick), the last route, the first pass (the walk), the row swept, the reach, the pitch
# alone. Read only; the log is never written.
#   tools\aimsearch.ps1 [-Log build\bf3vr.log] [-All] [-Slow 10]
#     -All: every stretch, not only those over -Slow seconds of frames from the first search (or never found).
param([string]$Log = '', [switch]$All, [double]$Slow = 10)
$ErrorActionPreference = 'Stop'
if (-not $Log) { $Log = Join-Path (Split-Path $PSScriptRoot) 'build\bf3vr.log' }
if (-not (Test-Path $Log)) { Write-Output "no log at $Log"; exit 1 }

# Only the lines that tell (Select-String is quick on a 50 MB log), each with its time in seconds (a day added at midnight).
$pattern = 'a new soldier; the mod|LAUNCH LEVEL: (a level is on|out of the level)|Aim: (the look angles forgotten|the look angles at [0-9A-F]{8} stopped|' +
           'the look angles found|the row swept from|the reach looked through|no look angles yet|the game''s view is exactly level|' +
           '[0-9]+ pairs near the soldier match the camera, too many|the search took|trying the last look angles)|without a frame here|bf3vr loaded'
$day = 0
$last = -1.0
$events = New-Object System.Collections.Generic.List[object]
foreach ($m in Select-String -Path $Log -Pattern $pattern) {
    $line = $m.Line
    if ($line -notmatch '^\[(\d\d):(\d\d):(\d\d)\.(\d\d\d)\] (.*)$') { continue }
    $t = $day * 86400 + [int]$matches[1] * 3600 + [int]$matches[2] * 60 + [int]$matches[3] + [int]$matches[4] / 1000.0
    if ($last -ge 0 -and $t -lt $last - 3600) { $day += 1; $t += 86400 }
    $last = $t
    $text = $matches[5]
    $kind = switch -Regex ($text) {
        'a new soldier; the mod' { 'soldier'; break }
        '^LAUNCH LEVEL: a level is on' { 'level+'; break }
        '^LAUNCH LEVEL: out of the level' { 'level-'; break }
        '^Aim: the look angles forgotten' { 'cutscene'; break }
        '^Aim: the look angles at [0-9A-F]{8} stopped' { 'mismatch'; break }
        '^Aim: the look angles found again by the last route' { 'found-route'; break }
        '^Aim: the look angles found again by' { 'found-quick'; break }
        '^Aim: the look angles found by the pitch alone' { 'found-pitch'; break }
        '^Aim: the look angles found at' { 'found-at'; break }
        '^Aim: the row swept from .* proved; no route saved' { 'found-sweep'; break }
        '^Aim: the reach looked through .* proved; no route saved' { 'found-reach'; break }
        '^Aim: the reach looked through' { 'reach'; break }
        '^Aim: no look angles yet' { 'none'; break }
        'exactly level' { 'level'; break }
        'too many to nudge' { 'crowded'; break }
        '^Aim: the search took (\d+) ms' { 'took'; break }
        '^Aim: trying the last look angles' { 'quick'; break }
        'without a frame here' { 'gap'; break }
        '^bf3vr loaded' { 'start'; break }
        default { '' }
    }
    if ($kind) { $events.Add([pscustomobject]@{ T = $t; Kind = $kind; Text = $text }) }
}

function Show-Time([double]$t) {
    $t = $t % 86400
    return '{0:00}:{1:00}:{2:00.000}' -f [math]::Floor($t / 3600), [math]::Floor(($t % 3600) / 60), ($t % 60)
}

# The stretches: each opens at the first search after a find (every loss is followed by one), from the earliest loss said since that find (the run's
# start, a new soldier, a load, a cutscene, a mismatch) or from the search itself (a scripted moment over, a weapon switch the camera still matched
# for a while), and closes at the next find. A search's find
# lines come at one instant: the quick try's and the pitch alone's before its "found at" (or alone), the row swept's and the reach's after it: the
# stretch closed at that instant takes their way. A find with no search line before it (a quick try finding at once) closes the losses said before it.
function New-Stretch([double]$t, [string]$cause) {
    return [pscustomobject]@{ Start = $t; Cause = $cause; Soldier = $null; Searches = New-Object System.Collections.Generic.List[double]; None = 0; Level = 0
                              Crowded = 0; Reach = 0; Quick = 0; Gaps = New-Object System.Collections.Generic.List[object]; End = $null; Way = ''; At = '' }
}
$stretches = New-Object System.Collections.Generic.List[object]
$cur = $null      # the stretch searching now
$pending = $null  # the losses said since the last find, before any search
$closed = $null   # the last stretch closed
foreach ($e in $events) {
    $kind = $e.Kind
    if ($kind -in @('start', 'soldier', 'cutscene', 'mismatch', 'level-')) {
        $into = $cur
        if ($null -eq $into) {
            if ($null -eq $pending) { $pending = New-Stretch $e.T $kind }
            $into = $pending
        }
        if ($kind -eq 'soldier') { $into.Soldier = $e.T }
        continue
    }
    if ($kind -like 'found-*') {
        $way = $kind.Substring(6)
        if ($null -ne $closed -and [math]::Abs($closed.End - $e.T) -lt 0.05) {
            if ($way -in @('sweep', 'reach')) { $closed.Way = $way }
            if ($e.Text -match 'found at ([0-9A-F]{8})') { $closed.At = $matches[1] }
            continue
        }
        $s = if ($null -ne $cur) { $cur } else { $pending }
        if ($null -ne $s) {
            $s.Way = $way
            if ($e.Text -match 'found at ([0-9A-F]{8})') { $s.At = $matches[1] }
            $s.End = $e.T
            $stretches.Add($s)
            $closed = $s
        }
        $cur = $null
        $pending = $null
        continue
    }
    if ($kind -eq 'gap') {
        $into = if ($null -ne $cur) { $cur } else { $pending }
        if ($null -ne $into -and $e.Text -match '([\d.]+) s without a frame') { $into.Gaps.Add(@(($e.T - [double]$matches[1]), $e.T)) }
        continue
    }
    $ms = 0
    if ($kind -eq 'took' -and $e.Text -match 'the search took (\d+) ms') { $ms = [int]$matches[1] }
    if ($null -eq $cur -and $kind -in @('took', 'quick', 'none', 'level', 'crowded', 'reach')) {
        if ($kind -eq 'took' -and $null -ne $closed -and [math]::Abs($closed.End - $e.T) -lt 0.05) { continue }  # (the finding search's own line)
        $cur = if ($null -ne $pending) { $pending } else { New-Stretch ($e.T - $ms / 1000.0) 'search' }
        $pending = $null
    }
    if ($null -eq $cur) { continue }
    if ($kind -eq 'took') { $cur.Searches.Add($e.T - $ms / 1000.0) }
    elseif ($kind -eq 'none') { $cur.None++ }
    elseif ($kind -eq 'level') { $cur.Level++ }
    elseif ($kind -eq 'crowded') { $cur.Crowded++ }
    elseif ($kind -eq 'reach') { $cur.Reach++ }
    elseif ($kind -eq 'quick') { $cur.Quick++ }
}
if ($null -ne $cur) { $stretches.Add($cur) }

function Get-Frames($s, $from, $to) {
    if ($null -eq $from -or $null -eq $to) { return $null }
    $gap = 0.0
    foreach ($g in $s.Gaps) {
        $lo = [math]::Max($g[0], $from); $hi = [math]::Min($g[1], $to)
        if ($hi -gt $lo) { $gap += $hi - $lo }
    }
    return ($to - $from) - $gap
}

$rows = 0
$byWay = @{}
foreach ($s in $stretches) {
    # (From the new soldier when there is one, else from the loss: the first search after it, the quick tries logging none.)
    $anchor = if ($null -ne $s.Soldier) { $s.Soldier } else { $s.Start }
    $first = $null
    foreach ($q in $s.Searches) { if ($q -ge $anchor - 0.5 -and $null -eq $first) { $first = $q } }
    $fromLoss = Get-Frames $s $s.Start $s.End
    $fromSoldier = Get-Frames $s $s.Soldier $s.End
    $fromFirst = Get-Frames $s $first $s.End
    $timed = if ($null -ne $s.Soldier) { $fromFirst } else { $fromLoss }
    $way = if ($s.Way) { $s.Way } else { 'NOT FOUND' }
    if (-not $byWay.ContainsKey($way)) { $byWay[$way] = New-Object System.Collections.Generic.List[double] }
    if ($null -ne $timed) { $byWay[$way].Add($timed) }
    $slowOne = ($null -eq $s.End) -or ($null -ne $timed -and $timed -gt $Slow)
    if (-not $All -and -not $slowOne) { continue }
    $rows++
    $f = { param($v) if ($null -ne $v) { '{0:N1} s' -f $v } else { '-' } }
    Write-Output ('{0} {1,-8} -> {2} {3,-9} {4} | frames from the loss {5,7}, the new soldier {6,7}, its first search {7,7} | {8,3} searches: nothing near {9}, level {10}, too many {11}, reach {12}, quick {13}' -f
        (Show-Time $s.Start), $s.Cause, $(if ($null -ne $s.End) { Show-Time $s.End } else { '--:--:--.---' }), $way, $(if ($s.At) { $s.At } else { '        ' }),
        (& $f $fromLoss), (& $f $fromSoldier), (& $f $fromFirst), $s.Searches.Count, $s.None, $s.Level, $s.Crowded, $s.Reach, $s.Quick)
}
if (-not $rows) { Write-Output ('No stretch over {0} s of frames.' -f $Slow) }
Write-Output ''
Write-Output 'By way found: how many, and the seconds of frames to the find from the new soldier''s first search, or from the loss (median, the most).'
foreach ($k in ($byWay.Keys | Sort-Object)) {
    $v = @($byWay[$k] | Sort-Object)
    if ($v.Count) { Write-Output ('  {0,-10} {1,4}   median {2,6:N1} s   most {3,7:N1} s' -f $k, $v.Count, $v[[math]::Floor($v.Count / 2)], $v[$v.Count - 1]) }
    else { Write-Output ('  {0,-10} none timed' -f $k) }
}

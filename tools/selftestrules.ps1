# tools\selftest.ps1's own rules, offline (no game: selftest.ps1 itself needs bf3 running and is never run here; its functions are read out of it and
# run on made-up rounds). Each check prints PASS or FAIL; the exit code is the number failed.
#   r1  selftest.ps1 parses (no syntax error anywhere in it)
#   r2  its Free hands round's stall (free hands' "N s without a frame here", no frame for over half a second) is the line src\hands.cpp writes: its
#       format, filled in, matched by the pattern, the seconds read back
#   r3  a run's verdict (FreeVerdict): a failure beside a stall a WARN (the round runs once more), with the stall's words (how many, the longest); a
#       failure with no stall a FAIL; a pass a PASS whatever stalled; a skip a skip. The run again's (-Again): its failure a FAIL stalled or not, the
#       stall's words with it (the review of 2026-11-25: a round that stalls every time must still be able to fail)
#   r4  the round runs once more only after a stalled failure, once the settle has been waited for (once, before that run alone, its words in that
#       run's row), and that run's verdict stands: its own failure a FAIL, with no stall or stalled again
#   r5  the settle (FreeSettle) on a made-up log written as it waits: back once three frame timing lines in a row are at 70 a second or more, saying how
#       long it waited; with the stall's lines alone, after its most, saying so
#   tools\selftestrules.ps1   (from anywhere; reads tools\selftest.ps1 and src\hands.cpp beside it)
$root = Split-Path $PSScriptRoot
$failed = 0
function Check([string]$id, [bool]$ok, [string]$what) {
    if (-not $ok) { $script:failed++ }
    Write-Host ('{0} {1,-3} {2}' -f $(if ($ok) { 'PASS' } else { 'FAIL' }), $id, $what)
}

$path = Join-Path $PSScriptRoot 'selftest.ps1'
$tokens = $null; $errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$errors)
Check 'r1' ($errors.Count -eq 0) ("selftest.ps1 parses: {0} errors{1}" -f $errors.Count, $(if ($errors.Count) { ', the first: ' + $errors[0].Message + ' at line ' + $errors[0].Extent.StartLineNumber } else { '' }))

# Its stall pattern and the two functions, out of it as written (nothing else of it runs).
$stallAssign = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -eq '$freeStall' }, $true) | Select-Object -First 1
$functions = @($ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -in 'StallWords', 'FreeVerdict', 'FreeRounds', 'FreeSettle', 'LogLines', 'Since' }, $true))
if (-not $stallAssign -or $functions.Count -ne 6) {
    Check 'r2' $false "selftest.ps1 has no `$freeStall, StallWords, FreeVerdict, FreeRounds, FreeSettle, LogLines or Since"
    exit $failed
}
. ([scriptblock]::Create($stallAssign.Extent.Text))
foreach ($f in $functions) { . ([scriptblock]::Create($f.Extent.Text)) }

# r2: the line as src\hands.cpp formats it ("%.1f s without a frame here"), the seconds read back.
$source = [IO.File]::ReadAllText((Join-Path $root 'src\hands.cpp'))
$format = [regex]::Match($source, 'Log\("(Free hands: %\.1f s without a frame here[^"]*)"').Groups[1].Value
$line = '[22:00:51.123] ' + $format.Replace('%.1f', '1.5')
$read = [regex]::Match($line, $freeStall)
Check 'r2' ($format -and $read.Success -and [double]$read.Groups[1].Value -eq 1.5) ("hands.cpp's stall line `"{0}`" matched by the round's pattern, 1.5 s read back: {1}" -f $format, $(if ($read.Success) { $read.Groups[1].Value } else { 'NOT matched' }))

# r3: each verdict.
function Round([string]$status, [string[]]$stalls) { [pscustomobject]@{ Read = $true; Lines = @(); Stalls = @($stalls); Put = ''; Status = $status; Detail = '12 of 15 in order' } }
$stallLines = @('[22:00:51.123] Free hands: 0.6 s without a frame here (the headset off?): ...', '[22:00:58.000] Free hands: 1.5 s without a frame here (the headset off?): ...')
$cases = @(
    @('a failure beside two stalls', (Round 'FAIL' $stallLines), 'WARN', 'stalled 2 times in it (the longest 1.5 s'),
    @('a failure beside one stall', (Round 'FAIL' @($stallLines[0])), 'WARN', 'stalled 1 time in it (the longest 0.6 s'),
    @('a failure with no stall', (Round 'FAIL' @()), 'FAIL', ''),
    @('a pass beside a stall', (Round 'PASS' @($stallLines[1])), 'PASS', 'stalled 1 time'),
    @('a pass', (Round 'PASS' @()), 'PASS', ''),
    @('a skip', (Round 'SKIP' @()), 'SKIP', ''))
# The run again's: a failure a FAIL, stalled or not (the stall's words with it); a pass a PASS; a skip a skip.
$again = @(
    @('run again: a failure beside two stalls', (Round 'FAIL' $stallLines), 'FAIL', 'stalled 2 times in it (the longest 1.5 s'),
    @('run again: a failure beside one stall', (Round 'FAIL' @($stallLines[0])), 'FAIL', 'stalled 1 time in it (the longest 0.6 s'),
    @('run again: a failure with no stall', (Round 'FAIL' @()), 'FAIL', ''),
    @('run again: a pass beside a stall', (Round 'PASS' @($stallLines[1])), 'PASS', 'stalled 1 time'),
    @('run again: a skip', (Round 'SKIP' @()), 'SKIP', ''))
$wrong = @()
foreach ($c in ($cases + $again)) {
    $status, $detail = if ($c[0].StartsWith('run again')) { FreeVerdict $c[1] -Again } else { FreeVerdict $c[1] }
    $saysStall = $detail -match 'stalled'
    if ($status -ne $c[2] -or ($c[3] -and -not $detail.Contains($c[3])) -or (-not $c[3] -and $saysStall) -or -not $detail.StartsWith('12 of 15 in order')) {
        $wrong += ('{0}: {1} "{2}"' -f $c[0], $status, $detail)
    }
}
Check 'r3' ($wrong.Count -eq 0) ("a run's verdict, {0} cases: a failure beside a stall a WARN with the stall's words, without one a FAIL, a pass a PASS; run again, {1} cases: a failure a FAIL stalled or not, with the stall's words{2}" -f $cases.Count, $again.Count, $(if ($wrong.Count) { '; wrong: ' + ($wrong -join '; ') } else { '' }))

# r4: selftest.ps1's FreeRounds itself on made-up runs (its rows caught by a Result of the test's): run again only after a stalled failure, from the log's
# end then, after the settle (waited for once, between the two runs: what had run by then is noted), that run's verdict standing in its own row with the
# settle's words, a failure there a FAIL stalled or not; every run returned, the last one last.
$script:rows = @()
$script:details = @()
function Result([string]$name, [string]$status, [string]$detail) { $script:rows += "$name $status"; $script:details += $detail }
$flows = @(
    @(@((Round 'FAIL' $stallLines), (Round 'PASS' @())), 'Free hands round (test 1) WARN|Free hands round (test 1, again) PASS', 2),
    @(@((Round 'FAIL' $stallLines), (Round 'FAIL' @())), 'Free hands round (test 1) WARN|Free hands round (test 1, again) FAIL', 2),
    @(@((Round 'FAIL' $stallLines), (Round 'FAIL' @($stallLines[0]))), 'Free hands round (test 1) WARN|Free hands round (test 1, again) FAIL', 2),
    @(@((Round 'FAIL' @()), (Round 'PASS' @())), 'Free hands round (test 1) FAIL', 1),
    @(@((Round 'PASS' $stallLines), (Round 'FAIL' @())), 'Free hands round (test 1) PASS', 1))
$off = @()
foreach ($f in $flows) {
    $script:rows = @()
    $script:details = @()
    $script:queue = [System.Collections.Queue]::new($f[0])
    $script:asked = @()
    $script:settled = @()
    $runs = FreeRounds 1234 { param($at) $script:asked += $at; $script:queue.Dequeue() } { $script:settled += $script:asked.Count; '; (settled)' }
    $want = $f[1]
    $got = $script:rows -join '|'
    $fromOk = $script:asked[0] -eq 1234 -and ($script:asked.Count -eq 1 -or $script:asked[1] -eq -1)
    # The settle: once between the two runs (one run done by then), none with one run; its words at the end of the run again's row alone.
    $settleOk = if ($f[2] -eq 2) { $script:settled.Count -eq 1 -and $script:settled[0] -eq 1 -and $script:details[-1].EndsWith('; (settled)') -and -not $script:details[0].Contains('(settled)') } else { $script:settled.Count -eq 0 }
    if ($got -ne $want -or $runs.Count -ne $f[2] -or $script:asked.Count -ne $f[2] -or -not $fromOk -or $runs[-1] -ne $f[0][$f[2] - 1] -or -not $settleOk) {
        $off += "$want wanted, got $got ($($runs.Count) runs, asked from $($script:asked -join ' and '), settled after run(s) $($script:settled -join ' and '))"
    }
}
Check 'r4' ($off.Count -eq 0) ("selftest.ps1's FreeRounds: once more only after a stalled failure (from the log's end, after the settle), its own verdict in its own row, a failure there a FAIL stalled or not: {0} flows{1}" -f $flows.Count, $(if ($off.Count) { '; wrong: ' + ($off -join '; ') } else { '' }))

# r5: selftest.ps1's FreeSettle (with its own LogLines and Since) on a made-up log, written as it waits (a job, as the game writes its log): the frame
# timing lines of the stall of 2026-11-18 22:00 (4 to 11 frames a second), then 72 a second: it returns once three lines in a row are at 70 a second or
# more, saying how long it waited; with the stall's lines alone it returns after its most, saying so. (About 10 s.)
$log = [IO.Path]::GetTempFileName()
function Lines([double[]]$ms) { @($ms | ForEach-Object { '[22:00:50.146] Frame timing (ms per frame): whole frame {0}, of it waiting for the headset 0.1, our copy 0.2, handing over 8.5; {1} frames' -f $_.ToString('F1', [Globalization.CultureInfo]::InvariantCulture), [int](1000 / $_) }) }
$writer = {
    param($path, [string[]]$lines, [double]$gap)
    Start-Sleep -Milliseconds 600
    foreach ($l in $lines) {
        for ($try = 0; $try -lt 20; $try++) { try { [IO.File]::AppendAllText($path, $l + "`r`n"); break } catch { Start-Sleep -Milliseconds 25 } }
        Start-Sleep -Milliseconds ([int]($gap * 1000))
    }
}
$settles = @()
try {
    [IO.File]::WriteAllText($log, "[22:00:40.000] Bf3vr: the log's start`r`n" + ((Lines @(13.9, 13.9, 13.9)) -join "`r`n") + "`r`n")
    $job = Start-Job -ScriptBlock $writer -ArgumentList $log, (Lines @(54.5, 174.0, 239.6, 13.9, 14.1, 13.9, 13.9, 13.9, 13.9)), 0.4
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $settles += , @((FreeSettle 20), $clock.Elapsed.TotalSeconds)
    Wait-Job $job -Timeout 30 | Out-Null; Remove-Job $job -Force
    $job = Start-Job -ScriptBlock $writer -ArgumentList $log, (Lines @(54.5, 174.0, 239.6, 289.9, 203.0, 126.2, 111.6, 205.5, 128.7, 251.5)), 0.4
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $settles += , @((FreeSettle 4), $clock.Elapsed.TotalSeconds)
    Wait-Job $job -Timeout 30 | Out-Null; Remove-Job $job -Force
} finally {
    Remove-Item $log -ErrorAction SilentlyContinue
}
# (Loose on time: the job's own start can take seconds at the Idle class beside other work; what counts is which words come back, and before its most.)
$steadyOk = $settles[0][0] -match '^; run once the game drew 70 frames a second or more for 3 s \((\d+) s waited\)$' -and [int]$matches[1] -lt 20 -and $settles[0][1] -lt 25
$stalledOk = $settles[1][0] -eq '; run after 4 s without 3 s of 70 frames a second or more' -and $settles[1][1] -ge 4 -and $settles[1][1] -lt 12
Check 'r5' ($steadyOk -and $stalledOk) ("selftest.ps1's FreeSettle on a made-up log: the stall then 72 a second: `"{0}`" ({1:F1} s); the stall alone, at most 4 s: `"{2}`" ({3:F1} s)" -f $settles[0][0], $settles[0][1], $settles[1][0], $settles[1][1])

Write-Host ("{0} failed" -f $failed)
exit $failed

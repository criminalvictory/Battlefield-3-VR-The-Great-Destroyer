# tools\selftest.ps1's own rules, offline (no game: selftest.ps1 itself needs bf3 running and is never run here; its functions are read out of it and
# run on made-up rounds). Each check prints PASS or FAIL; the exit code is the number failed.
#   r1  selftest.ps1 parses (no syntax error anywhere in it)
#   r2  its Free hands round's stall (free hands' "N s without a frame here", no frame for over half a second) is the line src\hands.cpp writes: its
#       format, filled in, matched by the pattern, the seconds read back
#   r3  a run's verdict (FreeVerdict): a failure beside a stall a WARN (the round runs once more), with the stall's words (how many, the longest); a
#       failure with no stall a FAIL; a pass a PASS whatever stalled; a skip a skip
#   r4  the round runs once more only after a stalled failure, and that run's verdict stands: its own failure with no stall a FAIL, stalled again a WARN
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
$functions = @($ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -in 'StallWords', 'FreeVerdict', 'FreeRounds' }, $true))
if (-not $stallAssign -or $functions.Count -ne 3) {
    Check 'r2' $false "selftest.ps1 has no `$freeStall, StallWords, FreeVerdict or FreeRounds"
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
$wrong = @()
foreach ($c in $cases) {
    $status, $detail = FreeVerdict $c[1]
    $saysStall = $detail -match 'stalled'
    if ($status -ne $c[2] -or ($c[3] -and -not $detail.Contains($c[3])) -or (-not $c[3] -and $saysStall) -or -not $detail.StartsWith('12 of 15 in order')) {
        $wrong += ('{0}: {1} "{2}"' -f $c[0], $status, $detail)
    }
}
Check 'r3' ($wrong.Count -eq 0) ("a run's verdict, {0} cases: a failure beside a stall a WARN with the stall's words, without one a FAIL, a pass a PASS{1}" -f $cases.Count, $(if ($wrong.Count) { '; wrong: ' + ($wrong -join '; ') } else { '' }))

# r4: selftest.ps1's FreeRounds itself on made-up runs (its rows caught by a Result of the test's): run again only after a stalled failure, from the log's
# end then, that run's verdict standing in its own row; every run returned, the last one last.
$script:rows = @()
function Result([string]$name, [string]$status, [string]$detail) { $script:rows += "$name $status" }
$flows = @(
    @(@((Round 'FAIL' $stallLines), (Round 'PASS' @())), 'Free hands round (test 1) WARN|Free hands round (test 1, again) PASS', 2),
    @(@((Round 'FAIL' $stallLines), (Round 'FAIL' @())), 'Free hands round (test 1) WARN|Free hands round (test 1, again) FAIL', 2),
    @(@((Round 'FAIL' $stallLines), (Round 'FAIL' @($stallLines[0]))), 'Free hands round (test 1) WARN|Free hands round (test 1, again) WARN', 2),
    @(@((Round 'FAIL' @()), (Round 'PASS' @())), 'Free hands round (test 1) FAIL', 1),
    @(@((Round 'PASS' $stallLines), (Round 'FAIL' @())), 'Free hands round (test 1) PASS', 1))
$off = @()
foreach ($f in $flows) {
    $script:rows = @()
    $script:queue = [System.Collections.Queue]::new($f[0])
    $script:asked = @()
    $runs = FreeRounds 1234 { param($at) $script:asked += $at; $script:queue.Dequeue() }
    $want = $f[1]
    $got = $script:rows -join '|'
    $fromOk = $script:asked[0] -eq 1234 -and ($script:asked.Count -eq 1 -or $script:asked[1] -eq -1)
    if ($got -ne $want -or $runs.Count -ne $f[2] -or $script:asked.Count -ne $f[2] -or -not $fromOk -or $runs[-1] -ne $f[0][$f[2] - 1]) {
        $off += "$want wanted, got $got ($($runs.Count) runs, asked from $($script:asked -join ' and '))"
    }
}
Check 'r4' ($off.Count -eq 0) ("selftest.ps1's FreeRounds: once more only after a stalled failure (from the log's end), its own verdict in its own row: {0} flows{1}" -f $flows.Count, $(if ($off.Count) { '; wrong: ' + ($off -join '; ') } else { '' }))

Write-Host ("{0} failed" -f $failed)
exit $failed

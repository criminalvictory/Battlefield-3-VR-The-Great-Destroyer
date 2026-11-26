# tools\report.ps1's key lines of the mod's log (the player's report keeps only those from the log's middle), offline: its pattern ($keys) read out of
# it and run on the aim's own log lines as src\aim.cpp writes them (each line's beginning from its Log format, the numbers filled in). Nothing runs but
# the pattern. Each check prints PASS or FAIL; the exit code is the number failed.
#   k1  report.ps1 parses and sets $keys to a lone pattern once (the mod's log's; its other $keys are the ini files' lines)
#   k2  the aim's lines its in-game checks look for are kept (the review of the aim lane's branch, its F6): "Aim: firing with the gun ..." (a search's
#       hold let go for firing, MouseAim) and "Aim: the reach read ... slowly twice in a row" (memory read slowly by the reach: on AMD it names it)
#   k3  the aim's lines that say its state kept as before: the view steered with the mouse, no look angles yet, found, found again, trying the last
#       look angles again, the search took, mouse aim on or off, the scripted moment over
#   k4  the aim's every-second lines left out (they'd crowd the rest out): the weapon camera as the game wrote it, head moves written, yaw at, the
#       game moved the look yaw itself
#   k5  a scripted prompt by a hand's grip: every Use, Use grips and Use test line of src\hands.cpp kept (why a door didn't open by the grip)
#   tools\reportkeystest.ps1   (from anywhere; reads tools\report.ps1 and src\aim.cpp beside it)
$root = Split-Path $PSScriptRoot
$failed = 0
function Check([string]$id, [bool]$ok, [string]$what) {
    if (-not $ok) { $script:failed++ }
    Write-Host ('{0} {1,-3} {2}' -f $(if ($ok) { 'PASS' } else { 'FAIL' }), $id, $what)
}

$path = Join-Path $PSScriptRoot 'report.ps1'
$tokens = $null; $errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$errors)
# (The mod's log's pattern: the one $keys set to a lone string; the others are lists of the ini files' lines.)
$assigns = @($ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -eq '$keys' -and
            $n.Right -is [System.Management.Automation.Language.CommandExpressionAst] -and
            $n.Right.Expression -is [System.Management.Automation.Language.StringConstantExpressionAst] }, $true))
$keys = if ($assigns.Count -eq 1) { $assigns[0].Right.Expression.Value } else { '' }
Check 'k1' ($errors.Count -eq 0 -and $keys -ne '') ("report.ps1 parses ({0} errors), `$keys set to a lone pattern {1} times ({2} characters)" -f $errors.Count, $assigns.Count, $keys.Length)
if ($keys -eq '') { exit 4 }

# The aim's Log formats: each call's first string piece (the line's beginning), its conversions filled in (%s a word, %X a hex word, %f 1.5, the rest
# 12), after the log's time stamp.
$cpp = Get-Content -LiteralPath (Join-Path $root 'src\aim.cpp') -Raw
$formats = @([regex]::Matches($cpp, 'Log\("(Aim: [^"]*)"') | ForEach-Object { $_.Groups[1].Value })
function Fill([string]$format) {
    $filled = [regex]::Replace($format, '%[-+ #0]*\d*(\.\d+)?(hh|h|ll|l|z|j|t|L)?([diouxXeEfgGcsp%])', {
            param($m)
            switch -regex -casesensitive ($m.Groups[3].Value) { '%' { '%' } 's' { 'word' } '[xX]' { 'E71F720C' } '[eEfgG]' { '1.5' } default { '12' } }
        })
    return '[21:00:00.000] ' + $filled
}
# For each line's beginning: how many of aim.cpp's formats begin so, and how many of those the pattern keeps.
function Lines([string[]]$begins) {
    foreach ($b in $begins) {
        $f = @($formats | Where-Object { $_.StartsWith($b) })
        $kept = @($f | Where-Object { [regex]::IsMatch((Fill $_), $keys) })
        [pscustomobject]@{ Begin = $b; Found = $f.Count; Kept = $kept.Count }
    }
}
function Words($rows) { ($rows | ForEach-Object { '"{0}" {1} of {2}' -f $_.Begin, $_.Kept, $_.Found }) -join '; ' }

$wanted = @(Lines @('Aim: firing with the gun', 'Aim: the reach read'))
Check 'k2' (@($wanted | Where-Object { $_.Found -lt 1 -or $_.Kept -ne $_.Found }).Count -eq 0) ("the in-game checks' aim lines kept (kept of found in aim.cpp): " + (Words $wanted))
$state = @(Lines @('Aim: the view steered with the mouse', 'Aim: no look angles yet', 'Aim: the look angles found at', 'Aim: the look angles found again',
        'Aim: trying the last look angles', 'Aim: the search took', 'Aim: mouse aim', 'Aim: the scripted moment is over'))
Check 'k3' (@($state | Where-Object { $_.Found -lt 1 -or $_.Kept -ne $_.Found }).Count -eq 0) ("the aim's state lines kept: " + (Words $state))
$second = @(Lines @('Aim: the weapon camera as the game wrote it', 'Aim: %d head moves written', 'Aim: yaw at', 'Aim: the game moved the look yaw itself'))
Check 'k4' (@($second | Where-Object { $_.Found -lt 1 -or $_.Kept -ne 0 }).Count -eq 0) ("the every-second lines left out (kept of found): " + (Words $second))
# k5: a scripted prompt by a hand's grip (src\hands.cpp's Use, Use grips and Use test lines): every one kept, so a player's report says why a door
# didn't open by the grip; and "Use" inside another line (a word of a sentence) is no key line by itself.
$handsCpp = Get-Content -LiteralPath (Join-Path $root 'src\hands.cpp') -Raw
$useFormats = @([regex]::Matches($handsCpp, 'Log\("(Use( grips| test)?: [^"]*)"') | ForEach-Object { $_.Groups[1].Value })
$useKept = @($useFormats | Where-Object { [regex]::IsMatch((Fill $_), $keys) })
$inside = [regex]::IsMatch('[21:00:00.000] Free hands: nothing to Use: here', '(?i)\] Use( grips| test)?: ')
Check 'k5' ($useFormats.Count -ge 10 -and $useKept.Count -eq $useFormats.Count -and @($useFormats | Where-Object { $_ -like 'Use grips:*' }).Count -ge 1 -and
    @($useFormats | Where-Object { $_ -like 'Use test:*' }).Count -ge 1 -and -not $inside) ("hands.cpp's prompt lines kept: {0} of {1} (Use grips {2}, Use test {3}); inside another line {4}" -f
    $useKept.Count, $useFormats.Count, @($useFormats | Where-Object { $_ -like 'Use grips:*' }).Count, @($useFormats | Where-Object { $_ -like 'Use test:*' }).Count, $inside)
Write-Host ('{0} of 5 checks failed' -f $failed)
exit $failed

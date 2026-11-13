# weapon.txt kept as a film found it, for the film scripts (dot-sourced: . "<project>\tools\weaponkeep.ps1"). A film writes the whole file each time it
# changes a line. Until 2026-11-12 each film wrote its own list over it and its hand-over another: every line found there and on neither list was lost
# (poserate, poseadapt, armby, posebydraw, nameby and headwave, the night of 11-12), and "viewlog 1800" was written in.
#   WkStart <path> [-HandOver] [-Name x]   the file as found, byte for byte. -HandOver (a film that hands the game over): a copy in build\weapon.<x>.bak
#                                          while it runs; a run killed outright leaves it, and the next run takes it as found.
#   WkLines <lines>                        the film's lines, then every line found whose word none of them has. Never carried from the file: the words
#                                          that act once (a picture, a refill, a turn) and the scripted hands' (standin and the rest: the film says those).
#   WkPut <path> <lines or text>           WkLines of them written (CRLF), tried again while the mod has the file open.
#   WkBack <path>                          puts it back. First, once, if a scripted hand would be left on (a word of $wkHands the file as found has no line
#                                          for: all of them with -HandOver, those the film set without), the found lines with those at 0, 0.45 s for the
#                                          mod to read (it reads the file every 300 ms; the hands stay as last set while their line is gone). Then the
#                                          found bytes exactly; with -HandOver each of the found lines of $wkHands at 0 in its own place (the stand-in
#                                          off above all). scopeview and everything else as found.
#   WkSame <path>                          $true if the file now is what WkBack wrote; $wkSaid says what it wrote.
# The mod reads the first 1023 bytes only: the film's lines go first, so a file too long loses found lines, never the film's (said if it happens).
$wkLatin = [Text.Encoding]::GetEncoding(28591)  # (a byte a character: read and written back unchanged)
$wkOnce = @('burstnow', 'flashburst', 'refill', 'empty', 'rocketgame', 'turnby')
$wkHands = @('standin', 'forcehand', 'forceheadless', 'm320test', 'groundprobe', 'heldtest', 'rockettest')
$wkFound = $null; $wkFoundLines = @(); $wkHandOver = $false; $wkBak = $null; $wkSet = @{}; $wkBackText = $null; $wkSaid = ''

function WkKey([string]$line) { return (($line.Trim() -split '\s+')[0]).ToLowerInvariant() }
function WkRead([string]$path) {
    for ($i = 0; $i -lt 20; $i++) { try { return $script:wkLatin.GetString([IO.File]::ReadAllBytes($path)) } catch { Start-Sleep -Milliseconds 30 } }
    throw "could not read $path"
}
function WkWrite([string]$path, [string]$text) {
    $bytes = $script:wkLatin.GetBytes($text)
    for ($i = 0; $i -lt 20; $i++) { try { [IO.File]::WriteAllBytes($path, $bytes); return } catch { Start-Sleep -Milliseconds 30 } }
    throw "could not write $path"
}
function WkStart([string]$path, [switch]$HandOver, [string]$Name = 'film') {
    $script:wkHandOver = [bool]$HandOver; $script:wkSet = @{}; $script:wkBackText = $null; $script:wkSaid = ''
    $script:wkBak = if ($HandOver) { Join-Path (Split-Path $path) "weapon.$Name.bak" } else { $null }
    if ($script:wkBak -and (Test-Path $script:wkBak)) {
        $script:wkFound = WkRead $script:wkBak
        Write-Host "weapon.txt as found taken from $(Split-Path $script:wkBak -Leaf): a run before was stopped before it put the file back"
    } elseif (Test-Path $path) {
        $script:wkFound = WkRead $path
        if ($script:wkBak) { WkWrite $script:wkBak $script:wkFound }
    } else { $script:wkFound = $null }
    # (a file saved with a byte order mark: its first line's word read without it, so a found "standin 1" there is known and put at 0)
    $script:wkFoundLines = if ($null -ne $script:wkFound) { @(($script:wkFound -replace '^\xEF\xBB\xBF', '') -split "`r?`n" | Where-Object { $_.Trim() }) } else { @() }
}
function WkLines([string[]]$lines) {
    $film = @($lines | Where-Object { $_ -and $_.Trim() })
    $keys = @{}
    foreach ($l in $film) { $k = WkKey $l; $keys[$k] = $true; $script:wkSet[$k] = $true }
    $carried = @($script:wkFoundLines | Where-Object { $k = WkKey $_; -not $keys.ContainsKey($k) -and $script:wkOnce -notcontains $k -and $script:wkHands -notcontains $k })
    return @($film + $carried)
}
function WkPut([string]$path, $what) {
    $lines = if ($what -is [string]) { @($what -split "`r?`n") } else { @($what) }
    $text = ((WkLines $lines) -join "`r`n") + "`r`n"
    if ($text.Length -gt 1000) { Write-Host "  weapon.txt is $($text.Length) bytes: the mod reads its first 1023 only (the film's lines are first; found lines past it are not read)" }
    WkWrite $path $text
}
function WkBack([string]$path) {
    $zero = if ($script:wkHandOver) { $script:wkHands } else { @($script:wkHands | Where-Object { $script:wkSet.ContainsKey($_) }) }
    $have = @{}; foreach ($l in $script:wkFoundLines) { $have[(WkKey $l)] = $true }
    $missing = @($zero | Where-Object { -not $have.ContainsKey($_) } | ForEach-Object { "$_ 0" })
    if ($null -eq $script:wkFound) {
        $final = if ($script:wkHandOver) { (@($script:wkHands | ForEach-Object { "$_ 0" }) -join "`r`n") + "`r`n" } else { $null }
    } else {
        $final = $script:wkFound
        if ($script:wkHandOver) { foreach ($k in $script:wkHands) { $final = [regex]::Replace($final, "(?m)^(\xEF\xBB\xBF)?[ \t]*$k(?![A-Za-z0-9_])[^\r\n]*", "`${1}$k 0") } }
    }
    $said = @()
    if ($missing.Count -and $null -ne $final) {
        WkWrite $path (((@($final -split "`r?`n" | Where-Object { $_.Trim() }) + $missing) -join "`r`n") + "`r`n")
        Start-Sleep -Milliseconds 450
        $said += 'first, once: ' + ($missing -join ', ')
    }
    if ($null -ne $final) { WkWrite $path $final }
    elseif ($missing.Count) { WkWrite $path ((($missing) -join "`r`n") + "`r`n"); $final = (($missing) -join "`r`n") + "`r`n" }
    elseif (Test-Path $path) { Remove-Item $path -Force -Confirm:$false }
    $script:wkBackText = $final
    if ($null -eq $script:wkFound) { $said = @('there was none before the film: ' + $(if ($final) { 'written with ' + (($final.Trim() -split "`r?`n") -join ', ') } else { 'none again' })) + $said }
    elseif ($final -ceq $script:wkFound) { $said = @('as found, byte for byte') + $said }
    else {
        $was = @($script:wkFound -split "`r?`n"); $now = @($final -split "`r?`n"); $diff = @()
        for ($i = 0; $i -lt $now.Count; $i++) { if ($i -lt $was.Count -and $was[$i] -cne $now[$i]) { $diff += "'$($was[$i])' as '$($now[$i])'" } }
        $said = @('as found but ' + ($diff -join ', ')) + $said
    }
    $script:wkSaid = 'weapon.txt ' + ($said -join '; ')
    if ($script:wkBak -and (Test-Path $script:wkBak)) { Remove-Item $script:wkBak -Force -Confirm:$false }
    return $final
}
function WkSame([string]$path) {
    if ($null -eq $script:wkBackText) { return -not (Test-Path $path) }
    if (-not (Test-Path $path)) { return $false }
    return (WkRead $path) -ceq $script:wkBackText
}

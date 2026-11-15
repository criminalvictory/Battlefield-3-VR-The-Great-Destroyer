# Battlefield 3's own graphics settings set to the ones the mod was tuned with (2026-11-15: what works on the tuned PC is every player's
# baseline). Dot-sourced by tools\launch_vr.ps1. The values come from build\gamesettings.txt, "GstRender.Key value" a line (# for notes):
# each key found in the game's settings file (Documents\Battlefield 3\settings\PROF_SAVE_profile: plain text, "key value" a line) gets that
# value; a key not there is left to the game (said). Never the monitor's resolution, refresh rate or screen (a player's own), never a key but
# GstRender's. FullscreenEnabled 0 always, with the file or without it: the mod needs the game in a window.
$gsNever = @('GstRender.ResolutionWidth', 'GstRender.ResolutionHeight', 'GstRender.ResolutionHertz', 'GstRender.FullscreenRefreshRate',
             'GstRender.FullscreenScreen')
$gsInvariant = [Globalization.CultureInfo]::InvariantCulture

# The settings asked for, in the file's order: a list of @(key, value). $refused gets each line not taken, with why.
function GsRead([string]$path, [ref]$refused) {
    $list = New-Object System.Collections.Generic.List[object]
    $bad = New-Object System.Collections.Generic.List[string]
    if ($path -and [IO.File]::Exists($path)) {
        foreach ($raw in [IO.File]::ReadAllLines($path)) {
            $line = $raw.Trim()
            if (-not $line -or $line.StartsWith('#')) { continue }
            if ($line -notmatch '^(GstRender\.[A-Za-z0-9_]+)\s+(-?[0-9]+(\.[0-9]+)?)$') { $bad.Add("$line (not a GstRender setting and a number)"); continue }
            if ($gsNever -contains $matches[1]) { $bad.Add("$line (your monitor's own: never set)"); continue }
            $key = $matches[1]; $value = $matches[2]
            $had = $false
            for ($i = 0; $i -lt $list.Count; $i++) { if ($list[$i][0] -eq $key) { $list[$i] = @($key, $value); $had = $true } }  # (the last line wins)
            if (-not $had) { $list.Add(@($key, $value)) }
        }
    }
    if (-not @($list | Where-Object { $_[0] -eq 'GstRender.FullscreenEnabled' }).Count) { $list.Add(@('GstRender.FullscreenEnabled', '0')) }
    for ($i = 0; $i -lt $list.Count; $i++) { if ($list[$i][0] -eq 'GstRender.FullscreenEnabled' -and $list[$i][1] -ne '0') { $bad.Add("GstRender.FullscreenEnabled $($list[$i][1]) (the mod needs a window: 0 kept)"); $list[$i] = @('GstRender.FullscreenEnabled', '0') } }
    $refused.Value = $bad
    return , $list
}

function GsSame([string]$a, [string]$b) {
    $x = 0.0; $y = 0.0
    if ([double]::TryParse($a, [Globalization.NumberStyles]::Float, $gsInvariant, [ref]$x) -and [double]::TryParse($b, [Globalization.NumberStyles]::Float, $gsInvariant, [ref]$y)) {
        return [Math]::Abs($x - $y) -lt 1e-6
    }
    return $a -eq $b
}

# The game's settings text (one character a byte) with $list's values in. Returns @(text, changes, missing): changes "Key old -> new", missing
# the keys not in the file (left to the game). Nothing else in the text changes: the same lines, order and line ends.
function GsApply([string]$text, $list) {
    $changes = New-Object System.Collections.Generic.List[string]
    $missing = New-Object System.Collections.Generic.List[string]
    foreach ($kv in $list) {
        $key = $kv[0]; $value = $kv[1]
        $m = [regex]::Match($text, '(?m)^' + [regex]::Escape($key) + ' ([^\r\n]*)')
        if (-not $m.Success) { $missing.Add($key); continue }
        $old = $m.Groups[1].Value
        if (GsSame $old $value) { continue }
        $text = $text.Substring(0, $m.Groups[1].Index) + $value + $text.Substring($m.Groups[1].Index + $m.Groups[1].Length)
        $changes.Add(('{0} {1} -> {2}' -f ($key -replace '^GstRender\.', ''), $old, $value))
    }
    return , @($text, $changes, $missing)
}

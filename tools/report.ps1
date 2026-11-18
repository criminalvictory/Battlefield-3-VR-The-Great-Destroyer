# Everything about this PC, the game and the mod's last start in one file to send: "BF3 VR report.txt" in the mod's folder (the
# user, 2026-11-18, after a player wrote only that the game doesn't start: "we got to make a log that pretty much tells us
# everything"). Which copy of the game (Steam's or the EA app's), the EA app, Steam, Battlelog's links, VR, the mod's files, what
# runs, the crashes Windows saw, the launcher's, the injector's and the mod's logs, and Steam's and the EA app's own lines about
# starting the game. Private details are taken out (the Windows user and PC names, e-mail addresses, account and session ids,
# tokens): it's meant to be posted. Reads only; starts and changes nothing.
#   tools\report.ps1          make it ("Make a report.bat"), then show it in Explorer
#   tools\report.ps1 -Quiet   make it, nothing shown, no waiting
#   . tools\report.ps1        (the launcher: MakeReport, each time it stops, and once the mod is in)
param([switch]$Quiet)

$repRoot = Split-Path $PSScriptRoot
$repBuild = Join-Path $repRoot 'build'
$repFile = Join-Path $repRoot 'BF3 VR report.txt'

# Private details out of a line.
function RepPrivate([string]$text) {
    if (-not $text) { return '' }
    $t = $text
    foreach ($p in @($env:USERPROFILE, ($env:USERPROFILE -replace '\\', '/'))) {
        if ($p -and $p.Length -ge 6) { $t = [regex]::Replace($t, [regex]::Escape($p), '%USERPROFILE%', 'IgnoreCase') }
    }
    $t = [regex]::Replace($t, 'OneDrive - [^\\/:*?"<>|]+', 'OneDrive - <org>')
    foreach ($name in @($env:USERNAME, $env:COMPUTERNAME)) {
        if ($name -and $name.Length -ge 3) {
            $t = [regex]::Replace($t, '(?<![A-Za-z0-9])' + [regex]::Escape($name) + '(?![A-Za-z0-9])', '<private>', 'IgnoreCase')
        }
    }
    $t = [regex]::Replace($t, '[\w.+-]+@[\w-]+(\.[\w-]+)+', '<e-mail>')
    $t = [regex]::Replace($t, '(?i)\b(access_?token|refresh_?token|id_?token|token|auth_?code|authcode|sid|session(?:id)?|ticket|secret|password|pwd|key|signature|sig)=[^&\s"'',;]+', '$1=<removed>')
    $t = [regex]::Replace($t, '(?i)"([^"]*(?:id|token|email|name|persona|user|nucleus|auth|session|account)[^"]*)"\s*:\s*("[^"]*"|\d+)', '"$1":"<removed>"')
    $t = [regex]::Replace($t, '(?<![\w.])\d{9,}(?![\w.])', '<n>')
    $t = [regex]::Replace($t, '[A-Za-z0-9_+=-]{32,}', '<removed>')
    $t
}

function RepAdd([string]$line) { $script:repLines.Add((RepPrivate $line)) }
function RepSection([string]$title) { $script:repLines.Add(''); $script:repLines.Add("== $title") }
function RepCut([string]$line, [int]$most) { if ($line.Length -gt $most) { $line.Substring(0, $most) + ' ...' } else { $line } }
function RepTry([scriptblock]$probe, [string]$what) {
    try { & $probe } catch { RepAdd ("  ({0} couldn't be read: {1})" -f $what, $_.Exception.Message) }
}
# A log: all of it up to `last` lines, or its first `first` lines and its last `last`.
function RepLog([string]$title, [string]$path, [int]$first, [int]$last) {
    RepSection $title
    RepTry {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { RepAdd '  (not there)'; return }
        $item = Get-Item -LiteralPath $path
        $lines = @(Get-Content -LiteralPath $path -ErrorAction Stop)
        RepAdd ("  ({0} lines, written {1:yyyy-MM-dd HH:mm:ss})" -f $lines.Count, $item.LastWriteTime)
        if ($first -gt 0 -and $lines.Count -gt $first + $last) {
            foreach ($l in $lines[0..($first - 1)]) { RepAdd ('  ' + (RepCut $l 500)) }
            RepAdd ("  ... ({0} lines left out) ..." -f ($lines.Count - $first - $last))
            foreach ($l in $lines[($lines.Count - $last)..($lines.Count - 1)]) { RepAdd ('  ' + (RepCut $l 500)) }
        } else {
            if ($lines.Count -gt $last) { RepAdd ("  ... (its first {0} lines left out) ..." -f ($lines.Count - $last)) }
            foreach ($l in ($lines | Select-Object -Last $last)) { RepAdd ('  ' + (RepCut $l 500)) }
        }
    } $path
}

function MakeReport([string]$why) {
    $script:repLines = New-Object System.Collections.Generic.List[string]
    $now = Get-Date
    RepAdd ("BF3 VR report, made {0:yyyy-MM-dd HH:mm:ss} (UTC{1}), by {2}" -f $now, $now.ToString('zzz'), $why)
    RepAdd 'Send this whole file when asking for help. Private details are taken out: the Windows user and PC names, e-mail addresses, account and session ids, tokens.'

    RepSection 'The mod'
    RepTry {
        $ver = Join-Path $repRoot 'VERSION'
        RepAdd $(if (Test-Path -LiteralPath $ver) { '  Version: ' + ((Get-Content -LiteralPath $ver -TotalCount 3) -join ' ') } else { '  Version: no VERSION file (a build from the source)' })
        RepAdd "  Folder: $repRoot"
        foreach ($f in 'injector.exe', 'bf3vr.dll', 'gamesounds.exe') {
            $p = Join-Path $repBuild $f
            if (Test-Path -LiteralPath $p) {
                $i = Get-Item -LiteralPath $p
                RepAdd ("  build\{0}: {1:N0} bytes, {2:yyyy-MM-dd HH:mm}, SHA256 {3}" -f $f, $i.Length, $i.LastWriteTime, (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.Substring(0, 8))
            } else {
                RepAdd ("  build\{0}: MISSING (an antivirus may have removed it)" -f $f)
            }
        }
        RepAdd ("  Sounds made: {0} (build\sounds)" -f @(Get-ChildItem -LiteralPath (Join-Path $repBuild 'sounds') -Filter *.wav -File -ErrorAction SilentlyContinue).Count)
        $rt = Join-Path $repBuild 'runtime.txt'
        if (Test-Path -LiteralPath $rt) {
            RepAdd ("  build\runtime.txt: {0} (written {1:yyyy-MM-dd HH:mm})" -f ((Get-Content -LiteralPath $rt | Where-Object { $_ -notmatch '^\s*#' }) -join ' '), (Get-Item -LiteralPath $rt).LastWriteTime)
        }
        $ini = Join-Path $repBuild 'bf3vr.ini'
        if (Test-Path -LiteralPath $ini) {
            $keys = @(Select-String -LiteralPath $ini -Pattern '^\s*(prefer_steamvr|upload_copy|stream_view)\s*=' | ForEach-Object { $_.Line.Trim() })
            if ($keys.Count) { RepAdd ('  bf3vr.ini: ' + ($keys -join ', ')) }
        }
    } 'the mod''s files'
    RepTry {
        $caught = @(Get-MpThreatDetection -ErrorAction Stop | Where-Object { ($_.Resources -join ' ') -match '(?i)injector\.exe|bf3vr\.dll|gamesounds\.exe|launch_vr|report\.ps1' })
        if (-not $caught.Count) { RepAdd '  Windows Security: none of the mod''s files caught' }
        foreach ($c in $caught | Select-Object -Last 5) {
            RepAdd ("  Windows Security caught {0} at {1:yyyy-MM-dd HH:mm} (threat {2}, action {3})" -f ($c.Resources -join ', '), $c.InitialDetectionTime, $c.ThreatID, $c.ActionSuccess)
        }
    } 'Windows Security''s detections'

    RepSection 'This PC'
    RepTry {
        $os = Get-CimInstance Win32_OperatingSystem
        RepAdd ("  Windows: {0}, version {1} (build {2}), {3}" -f $os.Caption, $os.Version, $os.BuildNumber, $os.OSArchitecture)
        $insider = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\WindowsSelfHost\Applicability' -ErrorAction SilentlyContinue
        if ($insider -and $insider.BranchName) { RepAdd "  Windows Insider (preview builds): $($insider.BranchName)" }
        RepAdd ("  Memory: {0:N1} GB" -f ((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB))
        foreach ($c in Get-CimInstance Win32_Processor) { RepAdd "  Processor: $($c.Name.Trim())" }
        foreach ($g in Get-CimInstance Win32_VideoController) {
            $date = if ($g.DriverDate) { $g.DriverDate.ToString('yyyy-MM-dd') } else { '?' }
            $mode = if ($g.CurrentHorizontalResolution) { ", {0}x{1} at {2} Hz" -f $g.CurrentHorizontalResolution, $g.CurrentVerticalResolution, $g.CurrentRefreshRate } else { '' }
            RepAdd ("  Graphics: {0}, driver {1} ({2}){3}" -f $g.Name, $g.DriverVersion, $date, $mode)
        }
        $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
        RepAdd "  This report made as administrator: $admin"
    } 'this PC'

    RepSection 'Battlefield 3'
    $steamPath = [string](Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath -replace '/', '\'
    RepTry {
        # (Its own variables: "+=" on one of the caller's inside a script block made a string of it, so Steam's copy wasn't found.)
        $gameDir = $null
        $libs = @()
        if ($steamPath) {
            $libs += $steamPath
            $vdf = Join-Path $steamPath 'steamapps\libraryfolders.vdf'
            if (Test-Path -LiteralPath $vdf) { $libs += @(Select-String -LiteralPath $vdf -Pattern '"path"\s+"([^"]+)"' | ForEach-Object { $_.Matches[0].Groups[1].Value -replace '\\\\', '\' }) }
        }
        $dirs = New-Object System.Collections.Generic.List[string]
        $gf = Join-Path $repBuild 'gamefolder.txt'
        if (Test-Path -LiteralPath $gf) { $dirs.Add(([string](Get-Content -LiteralPath $gf -TotalCount 1)).Trim()) }
        foreach ($k in 'HKLM:\SOFTWARE\WOW6432Node\EA Games\Battlefield 3', 'HKLM:\SOFTWARE\EA Games\Battlefield 3') {
            $v = Get-ItemProperty $k -ErrorAction SilentlyContinue
            if ($v -and $v.'Install Dir') { $dirs.Add(([string]$v.'Install Dir').TrimEnd('\')) }
        }
        foreach ($lib in $libs | Select-Object -Unique) { $dirs.Add((Join-Path $lib 'steamapps\common\Battlefield 3')) }
        foreach ($d in @("$env:ProgramFiles\EA Games\Battlefield 3", "${env:ProgramFiles(x86)}\Origin Games\Battlefield 3")) { $dirs.Add($d) }
        $seen = @{}
        foreach ($d in $dirs) {
            if (-not $d -or $seen.ContainsKey($d.ToLower())) { continue }
            $seen[$d.ToLower()] = $true
            $exe = Join-Path $d 'bf3.exe'
            if (Test-Path -LiteralPath $exe) {
                $i = Get-Item -LiteralPath $exe
                RepAdd ("  bf3.exe in {0}: version {1}, {2:N0} bytes, {3:yyyy-MM-dd}" -f $d, $i.VersionInfo.FileVersion, $i.Length, $i.LastWriteTime)
                if (-not $gameDir) { $gameDir = $d }
            }
        }
        if (-not $gameDir) { RepAdd '  bf3.exe not found where Steam and the EA app install it' }
        $owned = @($libs | Where-Object { Test-Path -LiteralPath (Join-Path $_ 'steamapps\appmanifest_1238820.acf') })
        RepAdd $(if ($owned.Count) { "  Bought on: STEAM (Steam's manifest for Battlefield 3 is in $($owned[0]))" } else { '  Bought on: the EA APP or Origin (Steam has no manifest for it)' })
        $prof = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Battlefield 3\settings\PROF_SAVE_profile'
        RepAdd $(if (Test-Path -LiteralPath $prof) { "  The game's settings file: there ($prof)" } else { "  The game's settings file: NOT there ($prof): the game hasn't been started the normal way yet" })
        $sys = Join-Path $env:WINDIR 'SysWOW64'
        $missing = @('d3dx9_43.dll', 'd3dx11_43.dll', 'xinput1_3.dll', 'XAudio2_7.dll', 'D3DCompiler_43.dll', 'x3daudio1_7.dll' | Where-Object { -not (Test-Path -LiteralPath (Join-Path $sys $_)) })
        RepAdd $(if ($missing.Count) { "  DirectX June 2010 files MISSING (32-bit): $($missing -join ', ')" } else { '  DirectX June 2010 files (32-bit): there' })
        if ($gameDir) {
            $hooks = @('dxgi.dll', 'd3d11.dll', 'd3d9.dll', 'dinput8.dll', 'version.dll', 'winmm.dll' | Where-Object { Test-Path -LiteralPath (Join-Path $gameDir $_) })
            RepAdd $(if ($hooks.Count) { "  Other programs' hooks in the game's folder: $($hooks -join ', ') (ReShade and the like)" } else { "  Other programs' hooks in the game's folder: none" })
        }
        foreach ($k in 'HKCU:\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers', 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers') {
            $item = Get-Item -LiteralPath $k -ErrorAction SilentlyContinue
            if (-not $item) { continue }
            foreach ($n in $item.GetValueNames()) {
                if ($n -match '(?i)bf3\.exe|EADesktop|EALauncher|EASteamLauncher|Origin\.exe|steam\.exe') { RepAdd ("  Compatibility settings for {0}: {1}" -f $n, $item.GetValue($n)) }
            }
        }
    } 'the game'

    RepSection 'Steam, the EA app, Battlelog'
    RepTry {
        RepAdd $(if ($steamPath -and (Test-Path -LiteralPath (Join-Path $steamPath 'steam.exe'))) { "  Steam: installed ($steamPath)" } else { '  Steam: not installed' })
        $eaExe = Join-Path $env:ProgramFiles 'Electronic Arts\EA Desktop\EA Desktop\EADesktop.exe'
        RepAdd $(if (Test-Path -LiteralPath $eaExe) { '  EA app: installed, version ' + ((Get-Item -LiteralPath $eaExe).VersionInfo.ProductVersion -replace ',\s*', '.') } else { '  EA app: not in its usual folder' })
        $origin = Join-Path ${env:ProgramFiles(x86)} 'Origin\Origin.exe'
        if (Test-Path -LiteralPath $origin) { RepAdd ('  Origin (the EA app''s predecessor): installed, version ' + (Get-Item -LiteralPath $origin).VersionInfo.ProductVersion) }
        RepAdd ('  Default browser (Battlelog opens in it): ' + [string](Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\Shell\Associations\UrlAssociations\https\UserChoice' -ErrorAction SilentlyContinue).ProgId)
        foreach ($proto in 'origin2', 'link2ea', 'steam2ea') {
            $cmd = [string](Get-ItemProperty "Registry::HKEY_CLASSES_ROOT\$proto\shell\open\command" -ErrorAction SilentlyContinue).'(default)'
            RepAdd ("  {0}:// links (Battlelog's and Steam's way to the EA app) open: {1}" -f $proto, $(if ($cmd) { $cmd } else { 'nothing (not registered)' }))
        }
    } 'the launchers'

    RepSection 'VR'
    RepTry {
        RepAdd ('  OpenXR runtime, 64-bit: ' + [string](Get-ItemProperty 'HKLM:\SOFTWARE\Khronos\OpenXR\1' -ErrorAction SilentlyContinue).ActiveRuntime)
        RepAdd ('  OpenXR runtime, 32-bit (the game''s): ' + [string](Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Khronos\OpenXR\1' -ErrorAction SilentlyContinue).ActiveRuntime)
        $avail = Get-Item -LiteralPath 'HKLM:\SOFTWARE\WOW6432Node\Khronos\OpenXR\1\AvailableRuntimes' -ErrorAction SilentlyContinue
        if ($avail) { RepAdd ('  32-bit runtimes registered: ' + (($avail.GetValueNames() | ForEach-Object { $_ }) -join '; ')) }
        $vrpaths = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'openvr\openvrpaths.vrpath'
        RepAdd $(if (Test-Path -LiteralPath $vrpaths) { '  SteamVR: started on this PC before (openvrpaths.vrpath there)' } else { '  SteamVR: never started on this PC (no openvrpaths.vrpath)' })
        if ($env:ProgramW6432 -and (Test-Path -LiteralPath (Join-Path $env:ProgramW6432 'Virtual Desktop Streamer'))) { RepAdd '  Virtual Desktop Streamer: installed' }
    } 'VR'

    RepSection 'Running now'
    RepTry {
        $names = 'bf3', 'injector', 'EADesktop', 'EABackgroundService', 'EASteamLauncher', 'EALocalHostSvc', 'Origin', 'OriginWebHelperService', 'steam',
                 'vrserver', 'vrcompositor', 'vrmonitor', 'VirtualDesktop.Streamer', 'OVRServer_x64', 'RTSS', 'MSIAfterburner', 'Overwolf', 'Medal', 'obs64', 'obs32', 'Discord'
        $found = @(Get-Process -Name $names -ErrorAction SilentlyContinue | Sort-Object ProcessName -Unique)
        if (-not $found.Count) { RepAdd '  none of the game''s, the launchers'', VR''s or the overlays'' programs' }
        foreach ($p in $found) {
            $since = try { $p.StartTime.ToString('yyyy-MM-dd HH:mm:ss') } catch { '?' }
            RepAdd ("  {0} (since {1})" -f $p.ProcessName, $since)
        }
    } 'the programs running'

    RepSection 'Crashes and hangs Windows saw (its Application log, the last 2 days)'
    RepTry {
        $events = @(Get-WinEvent -FilterHashtable @{ LogName = 'Application'; Id = 1000, 1002; StartTime = (Get-Date).AddDays(-2) } -ErrorAction SilentlyContinue |
            Where-Object { $_.Message -match '(?i)bf3\.exe|injector\.exe|EADesktop|EASteamLauncher|EABackgroundService|Origin\.exe' } | Select-Object -First 12)
        if (-not $events.Count) { RepAdd '  none of the game''s, the mod''s or the EA app''s' }
        foreach ($e in $events) {
            RepAdd ("  {0:yyyy-MM-dd HH:mm:ss} event {1}: {2}" -f $e.TimeCreated, $e.Id, (RepCut ((($e.Message -split "`r?`n") | Where-Object { $_.Trim() } | Select-Object -First 7) -join ' | ') 700))
        }
    } 'Windows'' Application log'

    RepLog 'The launcher''s last run (build\launcher.log)' (Join-Path $repBuild 'launcher.log') 0 400
    RepLog 'The injector (build\injector.log)' (Join-Path $repBuild 'injector.log') 0 200
    RepLog 'The mod''s launch report (build\launch.log)' (Join-Path $repBuild 'launch.log') 0 200
    RepLog 'The mod''s log (build\bf3vr.log): its start and its end' (Join-Path $repBuild 'bf3vr.log') 80 80
    RepLog 'The sound maker (build\gamesounds.log): its end' (Join-Path $repBuild 'gamesounds.log') 0 15

    RepSection 'Steam''s own log: its lines about Battlefield 3 (logs\console_log.txt)'
    RepTry {
        $console = if ($steamPath) { Join-Path $steamPath 'logs\console_log.txt' } else { '' }
        if (-not $console -or -not (Test-Path -LiteralPath $console)) { RepAdd '  (not there)'; return }
        $hits = @(Select-String -LiteralPath $console -Pattern '1238820' -ErrorAction Stop | Select-Object -Last 40)
        if (-not $hits.Count) { RepAdd '  none' }
        foreach ($h in $hits) { RepAdd ('  ' + (RepCut $h.Line 400)) }
    } 'Steam''s log'

    RepSection 'The EA app''s own logs: its lines about starting games and errors (files written in the last 2 days)'
    RepTry {
        foreach ($dir in (Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'Electronic Arts\EA Desktop\Logs'), (Join-Path $env:ProgramData 'EA Desktop\Logs')) {
            foreach ($name in 'EADesktop.log', 'EASteamLauncher.log', 'EABackgroundService.log') {
                $p = Join-Path $dir $name
                if (-not (Test-Path -LiteralPath $p)) { continue }
                $item = Get-Item -LiteralPath $p
                if ($item.LastWriteTime -lt (Get-Date).AddDays(-2)) { continue }
                RepAdd ("  -- {0} (written {1:yyyy-MM-dd HH:mm}; its times are UTC)" -f $name, $item.LastWriteTime)
                $hits = @(Select-String -LiteralPath $p -Pattern '(?i)error|fail|denied|exception|crash|launch|1238820|bf3|battlefield 3|entitle|licen|terminat' -ErrorAction SilentlyContinue |
                    Where-Object { $_.Line -notmatch '(?i)\[CloudSync\] New file added' } | Select-Object -Last 50)
                if (-not $hits.Count) { RepAdd '  none' }
                foreach ($h in $hits) { RepAdd ('  ' + (RepCut $h.Line 400)) }
            }
        }
    } 'the EA app''s logs'

    RepSection 'End of the report'
    try {
        [IO.File]::WriteAllLines($repFile, $script:repLines, (New-Object Text.UTF8Encoding $false))
        return $repFile
    } catch {
        return $null
    }
}

if ($MyInvocation.InvocationName -ne '.') {
    Write-Host 'Making the report (about 10 seconds)...'
    $made = MakeReport 'Make a report.bat'
    if ($made) {
        Write-Host "Made: $made"
        Write-Host 'Send that file when asking for help (it''s shown in Explorer now). Private details are taken out.'
        if (-not $Quiet) { try { Start-Process explorer.exe ('/select,"{0}"' -f $made) } catch {} }
    } else {
        Write-Host 'The report couldn''t be written into the mod''s folder (is it read-only?).'
    }
    if (-not $Quiet) { Read-Host 'Press Enter to close' | Out-Null }
}

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

# Private details out of a line (review of 2026-11-18: a public post must carry none). The line is read decoded first (URL-encoded
# %5C and %3D, JSON's \\ and \"), so nothing hides behind an encoding. Then out: the names (the Windows user's, the PC's, the profile
# folder's, the account's full name and its words), the user's folder, e-mail and IP addresses, sign-in codes and tokens in any
# form (-AuthCode X, authCode=X, "authCode":"X"), account and user values (JSON or key: value), the person's name inside an audio
# output's name (a Bluetooth headset carries its owner's name), long numbers (account ids) and long codes.
function RepPrivate([string]$text) {
    if (-not $text) { return '' }
    $t = $text
    try { if ($t -match '%[0-9A-Fa-f]{2}') { $t = [Uri]::UnescapeDataString($t) } } catch {}
    $t = $t.Replace('\\', '\').Replace('\"', '"').Replace('\/', '/')
    foreach ($p in @($env:USERPROFILE, ($env:USERPROFILE -replace '\\', '/'))) {
        if ($p -and $p.Length -ge 6) { $t = [regex]::Replace($t, [regex]::Escape($p), '%USERPROFILE%', 'IgnoreCase') }
    }
    $t = [regex]::Replace($t, 'OneDrive - [^\\/:*?"<>|]+', 'OneDrive - <org>')
    foreach ($name in $script:repNames) {
        $t = [regex]::Replace($t, '(?<![A-Za-z0-9])' + [regex]::Escape($name) + '(?![A-Za-z0-9])', '<private>', 'IgnoreCase')
    }
    $t = [regex]::Replace($t, '[\w.+-]+@[\w-]+(\.[\w-]+)+', '<e-mail>')
    $t = [regex]::Replace($t, '(?<![\d.])(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})(?![\d.])', {
        param($m)
        $octets = 1..4 | ForEach-Object { [int]$m.Groups[$_].Value }
        if (($octets | Where-Object { $_ -gt 255 }) -or -not ($octets | Where-Object { $_ -ge 10 }) -or $m.Value -eq '127.0.0.1') { $m.Value } else { '<ip>' }
    })
    $t = [regex]::Replace($t, '(?i)(-{0,2}(?:auth_?code|login_?token|auth_?token|access_?token|refresh_?token|id_?token|cmdParamsBase64))(\s*[=:]\s*|\s+)"?[^\s"&,;]+', '$1$2<removed>')
    $t = [regex]::Replace($t, '(?i)\b(token|sid|session(?:id)?|ticket|secret|password|pwd|key|signature|sig|cookie|valu)=[^&\s"'',;]+', '$1=<removed>')
    $t = [regex]::Replace($t, '(?i)"([^"]*(?:id|token|email|name|persona|user|nucleus|auth|session|account|cookie|valu)[^"]*)"\s*:\s*("[^"]*"|\d+)', '"$1":"<removed>"')
    $t = [regex]::Replace($t, '(?i)\b(\w*(?:persona|nucleus|user_?id|user_?name|display_?name|e_?mail|account_?id)\w*)\s*[:=]\s*[^\s,;&"]+', '$1: <removed>')
    $t = [regex]::Replace($t, '(?i)((?:sounds play on|Sounds: ready \([^)]*\) on|the game plays on|the output moved to)\s+[^(,:]*\()([^)]*)', {
        param($m)
        if ($m.Groups[2].Value -match '(?i)realtek|nvidia|amd|intel|oculus|meta |steam|virtual|valve|index|frame|high definition|usb audio|hdmi|displayport') { $m.Value }
        else { $m.Groups[1].Value + '<device name>' }
    })
    $t = [regex]::Replace($t, '(?<![\d.])\d{9,}(?!\d|\.\d)', '<n>')
    $t = [regex]::Replace($t, '[A-Za-z0-9_+=-]{32,}', '<removed>')
    $t
}

function RepCut([string]$line, [int]$most) { if ($most -gt 0 -and $line.Length -gt $most) { $line.Substring(0, $most) + ' ...' } else { $line } }
# A line into the report: private details out first, then cut to `most` characters (cut first, half a code was left; review).
function RepAdd([string]$line, [int]$most = 0) { $script:repLines.Add((RepCut (RepPrivate $line) $most)) }
function RepSection([string]$title) { $script:repLines.Add(''); $script:repLines.Add("== $title") }
# A path built from a value read on the PC (a Steam library or a game folder on a drive that's gone): '' instead of an error.
function RepJoin([string]$dir, [string]$rest) { if (-not $dir) { return '' }; try { [IO.Path]::Combine($dir, $rest) } catch { '' } }
function RepFileThere([string]$path) { if (-not $path) { return $false }; try { [IO.File]::Exists($path) } catch { $false } }
# The account's full name and its words (a Microsoft account's local account carries the person's name).
function RepFullNames {
    try {
        $full = [string](Get-CimInstance Win32_UserAccount -Filter ("LocalAccount=True AND Name='{0}'" -f ($env:USERNAME -replace "'", "''")) -ErrorAction Stop | Select-Object -First 1).FullName
        if ($full) { @($full) + @($full -split '[\s.,_-]+') }
    } catch {}
}
function RepTry([scriptblock]$probe, [string]$what) {
    try { & $probe } catch { RepAdd ("  ({0} couldn't be read: {1})" -f $what, $_.Exception.Message) }
}
# A key under HKEY_LOCAL_MACHINE in its 64-bit or its 32-bit view, whichever PowerShell this runs in (a 32-bit one, started from a
# 32-bit program, would otherwise see the 32-bit view under both names); $null when it isn't there.
function RepHklm([string]$subkey, [switch]$Bits32) {
    try {
        $view = if ($Bits32) { [Microsoft.Win32.RegistryView]::Registry32 } else { [Microsoft.Win32.RegistryView]::Registry64 }
        [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine, $view).OpenSubKey($subkey)
    } catch { $null }
}
function RepHklmValue([string]$subkey, [string]$name, [switch]$Bits32) {
    $key = RepHklm $subkey -Bits32:$Bits32
    if ($key) { [string]$key.GetValue($name) } else { '' }
}
# The 64-bit Program Files, whichever PowerShell this runs in.
$repPrograms = if ($env:ProgramW6432) { $env:ProgramW6432 } else { $env:ProgramFiles }
# A log: all of it up to `last` lines, or its first `first` lines and its last `last`. A big one (a long play's mod log can be
# hundreds of megabytes) is read from its two ends only.
function RepLog([string]$title, [string]$path, [int]$first, [int]$last) {
    RepSection $title
    RepTry {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { RepAdd '  (not there)'; return }
        $item = Get-Item -LiteralPath $path
        if ($item.Length -gt 512KB) {
            RepAdd ("  ({0:N0} bytes, written {1:yyyy-MM-dd HH:mm:ss})" -f $item.Length, $item.LastWriteTime)
            if ($first -gt 0) {
                foreach ($l in @(Get-Content -LiteralPath $path -TotalCount $first -Encoding UTF8 -ErrorAction Stop)) { RepAdd ('  ' + $l) 500 }
                RepAdd '  ... (its middle left out) ...'
            } else {
                RepAdd ("  ... (all but its last {0} lines left out) ..." -f $last)
            }
            foreach ($l in @(Get-Content -LiteralPath $path -Tail $last -Encoding UTF8 -ErrorAction Stop)) { RepAdd ('  ' + $l) 500 }
            return
        }
        $lines = @(Get-Content -LiteralPath $path -Encoding UTF8 -ErrorAction Stop)
        RepAdd ("  ({0} lines, written {1:yyyy-MM-dd HH:mm:ss})" -f $lines.Count, $item.LastWriteTime)
        if ($first -gt 0 -and $lines.Count -gt $first + $last) {
            foreach ($l in $lines[0..($first - 1)]) { RepAdd ('  ' + $l) 500 }
            RepAdd ("  ... ({0} lines left out) ..." -f ($lines.Count - $first - $last))
            foreach ($l in $lines[($lines.Count - $last)..($lines.Count - 1)]) { RepAdd ('  ' + $l) 500 }
        } else {
            if ($lines.Count -gt $last) { RepAdd ("  ... (its first {0} lines left out) ..." -f ($lines.Count - $last)) }
            foreach ($l in ($lines | Select-Object -Last $last)) { RepAdd ('  ' + $l) 500 }
        }
    } $path
}

function MakeReport([string]$why) {
    $script:repLines = New-Object System.Collections.Generic.List[string]
    $script:repNames = @(@($env:USERNAME, $env:COMPUTERNAME, (Split-Path -Leaf $env:USERPROFILE)) + @(RepFullNames) |
        Where-Object { $_ -and $_.Length -ge 3 } | Select-Object -Unique | Sort-Object Length -Descending)
    $now = Get-Date
    RepAdd ("BF3 VR report, made {0:yyyy-MM-dd HH:mm:ss} (UTC{1}), by {2}" -f $now, $now.ToString('zzz'), $why)
    RepAdd 'Send this whole file when asking for help. Private details are taken out: the Windows user and PC names, e-mail addresses, account and session ids, tokens.'

    RepSection 'The mod'
    RepTry {
        $ver = Join-Path $repRoot 'VERSION'
        RepAdd $(if (Test-Path -LiteralPath $ver) { '  Version: ' + ((Get-Content -LiteralPath $ver -TotalCount 3) -join ' ') } else { '  Version: no VERSION file (a build from the source)' })
        RepAdd "  Folder: $repRoot"
        foreach ($f in 'injector.exe', 'bf3vr.dll', 'gamesounds.exe', 'gunshape.exe') {
            $p = Join-Path $repBuild $f
            if (Test-Path -LiteralPath $p) {
                $i = Get-Item -LiteralPath $p
                $hash = try { (Get-FileHash -LiteralPath $p -Algorithm SHA256 -ErrorAction Stop).Hash.Substring(0, 8) } catch { 'unreadable (an antivirus holding it?)' }
                RepAdd ("  build\{0}: {1:N0} bytes, {2:yyyy-MM-dd HH:mm}, SHA256 {3}" -f $f, $i.Length, $i.LastWriteTime, $hash)
            } else {
                RepAdd ("  build\{0}: MISSING (an antivirus may have removed it)" -f $f)
            }
        }
        $repWavs = @(Get-ChildItem -LiteralPath (Join-Path $repBuild 'sounds') -Filter *.wav -File -ErrorAction SilentlyContinue | Where-Object { $_.Length -gt 44 } | ForEach-Object { $_.BaseName.ToLowerInvariant() })
        # Which of this version's sounds aren't there (a new gun's, after an update in place): gamesounds.exe lists every one the mod plays
        # (--list: at once, the game not read); the launcher and the mod make the missing ones at the next start.
        $repPlays = @()
        try {
            $repCut = New-Object Diagnostics.ProcessStartInfo
            $repCut.FileName = Join-Path $repBuild 'gamesounds.exe'
            $repCut.Arguments = '--list'
            $repCut.WorkingDirectory = $repBuild
            $repCut.UseShellExecute = $false
            $repCut.CreateNoWindow = $true
            $repCut.RedirectStandardOutput = $true
            $repLister = [Diagnostics.Process]::Start($repCut)
            $repListed = $repLister.StandardOutput.ReadToEndAsync()
            if (-not $repLister.WaitForExit(10000)) { try { $repLister.Kill() } catch {} }
            elseif ($repLister.ExitCode -eq 0) {
                $repNames = @($repListed.Result -split "`r?`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ })
                if ($repNames.Count -and -not @($repNames | Where-Object { $_ -notmatch '^[a-z0-9_-]{1,63}$' }).Count) { $repPlays = $repNames }
            }
        } catch {}
        $repGone = @($repPlays | Where-Object { $repWavs -notcontains $_ })
        RepAdd ("  Sounds made: {0} (build\sounds){1}" -f $repWavs.Count, $(if (-not $repPlays.Count) { '' } elseif ($repGone.Count) { "; this version plays $($repPlays.Count), NOT THERE: $($repGone -join ', ') (made at the next start from the game's own files)" } else { "; all $($repPlays.Count) this version plays are there" }))
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
        $caught = @(Get-MpThreatDetection -ErrorAction Stop | Where-Object { ($_.Resources -join ' ') -match '(?i)injector\.exe|bf3vr\.dll|gamesounds\.exe|gunshape\.exe|launch_vr|report\.ps1' })
        if (-not $caught.Count) { RepAdd '  Windows Security: none of the mod''s files caught' }
        foreach ($c in $caught | Select-Object -Last 5) {
            $threat = try { [string](Get-MpThreat -ThreatID $c.ThreatID -ErrorAction Stop | Select-Object -First 1).ThreatName } catch { '' }
            RepAdd ("  Windows Security caught {0} at {1:yyyy-MM-dd HH:mm} ({2}; acted: {3})" -f ($c.Resources -join ', '), $c.InitialDetectionTime, $(if ($threat) { $threat } else { 'a threat' }), $c.ActionSuccess)
        }
    } 'Windows Security''s detections'

    RepSection 'This PC'
    RepTry {
        $os = Get-CimInstance Win32_OperatingSystem
        RepAdd ("  Windows: {0}, version {1} (build {2}), {3}" -f $os.Caption, $os.Version, $os.BuildNumber, $os.OSArchitecture)
        $insider = RepHklmValue 'SOFTWARE\Microsoft\WindowsSelfHost\Applicability' 'BranchName'
        if ($insider) { RepAdd "  Windows Insider (preview builds): $insider" }
        RepAdd ("  This report made by {0}-bit PowerShell {1}" -f $(if ([Environment]::Is64BitProcess) { 64 } else { 32 }), $PSVersionTable.PSVersion)
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
            $vdf = RepJoin $steamPath 'steamapps\libraryfolders.vdf'
            if (RepFileThere $vdf) { $libs += @(Select-String -LiteralPath $vdf -Pattern '"path"\s+"([^"]+)"' | ForEach-Object { $_.Matches[0].Groups[1].Value -replace '\\\\', '\' }) }
        }
        $dirs = New-Object System.Collections.Generic.List[string]
        $gf = Join-Path $repBuild 'gamefolder.txt'
        if (Test-Path -LiteralPath $gf) { $dirs.Add(([string](Get-Content -LiteralPath $gf -TotalCount 1)).Trim()) }
        foreach ($bits32 in $true, $false) {
            $v = RepHklmValue 'SOFTWARE\EA Games\Battlefield 3' 'Install Dir' -Bits32:$bits32
            if ($v) { $dirs.Add($v.TrimEnd('\')) }
        }
        foreach ($lib in $libs | Select-Object -Unique) { $dirs.Add((RepJoin $lib 'steamapps\common\Battlefield 3')) }
        foreach ($d in @("$repPrograms\EA Games\Battlefield 3", "${env:ProgramFiles(x86)}\Origin Games\Battlefield 3")) { $dirs.Add($d) }
        $seen = @{}
        foreach ($d in $dirs) {
            if (-not $d -or $seen.ContainsKey($d.ToLower())) { continue }
            $seen[$d.ToLower()] = $true
            $exe = RepJoin $d 'bf3.exe'
            if (RepFileThere $exe) {
                $i = Get-Item -LiteralPath $exe
                RepAdd ("  bf3.exe in {0}: version {1}, {2:N0} bytes, {3:yyyy-MM-dd}" -f $d, $i.VersionInfo.FileVersion, $i.Length, $i.LastWriteTime)
                if (-not $gameDir) { $gameDir = $d }
            }
        }
        if (-not $gameDir) { RepAdd '  bf3.exe not found where Steam and the EA app install it' }
        $owned = @($libs | Where-Object { RepFileThere (RepJoin $_ 'steamapps\appmanifest_1238820.acf') })
        RepAdd $(if ($owned.Count) { "  Bought on: STEAM (Steam's manifest for Battlefield 3 is in $($owned[0]))" } else { '  Bought on: the EA APP or Origin (Steam has no manifest for it)' })
        $prof = RepJoin ([Environment]::GetFolderPath('MyDocuments')) 'Battlefield 3\settings\PROF_SAVE_profile'
        RepAdd $(if (RepFileThere $prof) { "  The game's settings file: there ($prof)" } else { "  The game's settings file: NOT there ($prof): the game hasn't been started the normal way yet" })
        $sys = Join-Path $env:WINDIR 'SysWOW64'
        $missing = @('d3dx9_43.dll', 'd3dx11_43.dll', 'xinput1_3.dll', 'XAudio2_7.dll', 'D3DCompiler_43.dll', 'x3daudio1_7.dll' | Where-Object { -not (Test-Path -LiteralPath (Join-Path $sys $_)) })
        RepAdd $(if ($missing.Count) { "  DirectX June 2010 files MISSING (32-bit): $($missing -join ', ')" } else { '  DirectX June 2010 files (32-bit): there' })
        if ($gameDir) {
            $hooks = @('dxgi.dll', 'd3d11.dll', 'd3d9.dll', 'dinput8.dll', 'version.dll', 'winmm.dll' | Where-Object { RepFileThere (RepJoin $gameDir $_) })
            RepAdd $(if ($hooks.Count) { "  Other programs' hooks in the game's folder: $($hooks -join ', ') (ReShade and the like)" } else { "  Other programs' hooks in the game's folder: none" })
        }
        foreach ($item in @((Get-Item -LiteralPath 'HKCU:\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers' -ErrorAction SilentlyContinue),
                            (RepHklm 'SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers'))) {
            if (-not $item) { continue }
            foreach ($n in $item.GetValueNames()) {
                if ($n -match '(?i)bf3\.exe|EADesktop|EALauncher|EASteamLauncher|Origin\.exe|steam\.exe') { RepAdd ("  Compatibility settings for {0}: {1}" -f $n, $item.GetValue($n)) }
            }
        }
    } 'the game'

    RepSection 'Steam, the EA app, Battlelog'
    RepTry {
        # Steam's own launch options for Battlefield 3 (another mod may have left one there: a player's report, 2026-11-18): each Steam
        # account's userdata\<id>\config\localconfig.vdf, the app's block, its "LaunchOptions".
        foreach ($cfg in @(Get-ChildItem -LiteralPath (RepJoin $steamPath 'userdata') -Directory -ErrorAction SilentlyContinue | ForEach-Object { RepJoin $_.FullName 'config\localconfig.vdf' } | Where-Object { RepFileThere $_ })) {
            $text = [IO.File]::ReadAllText($cfg)
            # Every block named "1238820" (the game appears in more than one list), the first with a LaunchOptions line.
            $at = $text.IndexOf('"1238820"')
            $options = ''
            while ($at -ge 0 -and -not $options) {
                $open = $text.IndexOf('{', $at)
                $between = if ($open -gt $at) { $text.Substring($at + 9, $open - $at - 9) } else { 'x' }
                if ($open -gt 0 -and $between.Trim() -eq '') {
                    $depth = 0; $end = $open
                    for ($i = $open; $i -lt $text.Length; $i++) { if ($text[$i] -eq '{') { $depth++ } elseif ($text[$i] -eq '}') { $depth--; if ($depth -eq 0) { $end = $i; break } } }
                    $m = [regex]::Match($text.Substring($open, $end - $open + 1), '"LaunchOptions"\s+"((?:[^"\\]|\\.)*)"')
                    if ($m.Success) { $options = $m.Groups[1].Value }
                }
                $at = $text.IndexOf('"1238820"', $at + 9)
            }
            RepAdd ('  Steam''s launch options for Battlefield 3: ' + $(if ($options) { $options + '   (normally empty: anything here changes how Steam starts the game)' } else { 'none (as it should be)' }))
        }
    } 'Steam''s launch options'
    RepTry {
        RepAdd $(if (RepFileThere (RepJoin $steamPath 'steam.exe')) { "  Steam: installed ($steamPath)" } else { '  Steam: not installed' })
        $eaExe = Join-Path $repPrograms 'Electronic Arts\EA Desktop\EA Desktop\EADesktop.exe'
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
        RepAdd ('  OpenXR runtime, 64-bit: ' + (RepHklmValue 'SOFTWARE\Khronos\OpenXR\1' 'ActiveRuntime'))
        RepAdd ('  OpenXR runtime, 32-bit (the game''s): ' + (RepHklmValue 'SOFTWARE\Khronos\OpenXR\1' 'ActiveRuntime' -Bits32))
        $avail = RepHklm 'SOFTWARE\Khronos\OpenXR\1\AvailableRuntimes' -Bits32
        if ($avail) { RepAdd ('  32-bit runtimes registered: ' + ($avail.GetValueNames() -join '; ')) }
        $vrpaths = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'openvr\openvrpaths.vrpath'
        RepAdd $(if (Test-Path -LiteralPath $vrpaths) { '  SteamVR: started on this PC before (openvrpaths.vrpath there)' } else { '  SteamVR: never started on this PC (no openvrpaths.vrpath)' })
        if (Test-Path -LiteralPath (Join-Path $repPrograms 'Virtual Desktop Streamer')) { RepAdd '  Virtual Desktop Streamer: installed' }
    } 'VR'

    RepSection 'Running now'
    RepTry {
        $names = 'bf3', 'injector', 'EADesktop', 'EABackgroundService', 'EASteamLauncher', 'EALocalHostSvc', 'Origin', 'OriginWebHelperService', 'steam',
                 'vrserver', 'vrcompositor', 'vrmonitor', 'VirtualDesktop.Streamer', 'OVRServer_x64', 'RTSS', 'MSIAfterburner', 'Overwolf', 'Medal', 'obs64', 'obs32', 'Discord'
        $running = @(Get-Process -Name $names -ErrorAction SilentlyContinue)
        # (Every injector, each with its folder: one left from an earlier try, or from another copy of the mod, must stand out.)
        $found = @($running | Where-Object { $_.ProcessName -ne 'injector' } | Sort-Object ProcessName -Unique) + @($running | Where-Object { $_.ProcessName -eq 'injector' })
        if (-not $found.Count) { RepAdd '  none of the game''s, the launchers'', VR''s or the overlays'' programs' }
        foreach ($p in $found) {
            $since = try { $p.StartTime.ToString('yyyy-MM-dd HH:mm:ss') } catch { '?' }
            # Steam, the EA app and the game must run at the same level: one run as administrator and the other not, the EA app's helper
            # can't reach Steam and the game never starts ("Proxied launch timed out": a player's report, 2026-11-18). A program this one
            # can't open as the same user is elevated (or another user's).
            $level = ''
            if ($p.ProcessName -match '^(steam|EADesktop|bf3|EALocalHostSvc)$') { $level = try { $null = $p.Handle; ', normal (not as administrator)' } catch { ', AS ADMINISTRATOR (or another user)' } }
            if ($p.ProcessName -ne 'injector') { RepAdd ("  {0} (since {1}){2}" -f $p.ProcessName, $since, $level); continue }
            $file = ''
            try { $file = [string]$p.Path } catch {}
            if (-not $file) { try { $file = [string](Get-CimInstance Win32_Process -Filter ("ProcessId={0}" -f $p.Id) -ErrorAction Stop).ExecutablePath } catch {} }
            $folder = ''
            if ($file) { try { $folder = [IO.Path]::GetDirectoryName($file) } catch {} }
            $says = ''
            if (RepFileThere $file) { try { $says = [string][Diagnostics.FileVersionInfo]::GetVersionInfo($file).FileDescription } catch {} }
            $whose = if (-not $folder) { 'Windows doesn''t say from which folder: if it''s an old copy of the mod''s, end it in Task Manager (Details, injector.exe, End task)' }
                     elseif ($folder.TrimEnd('\') -ieq ([string]$repBuild).TrimEnd('\')) { 'this package''s own' }
                     elseif ((RepFileThere (RepJoin $folder 'bf3vr.dll')) -or ($says -match '^BF3 VR injector')) { 'ANOTHER FOLDER''S: a leftover from another copy of the mod, which would put that copy''s mod into the game (the launcher stops it at its next start; or end it in Task Manager: Details, injector.exe, End task)' }
                     else { 'not the mod''s (no bf3vr.dll beside it): another program of that name' }
            RepAdd ("  injector (since {0}, process {1}){2}: {3}" -f $since, $p.Id, $(if ($folder) { ', in ' + $folder } else { '' }), $whose)
        }
    } 'the programs running'

    RepSection 'Crashes and hangs Windows saw (its Application log, the last 2 days)'
    RepTry {
        $events = @(Get-WinEvent -FilterHashtable @{ LogName = 'Application'; Id = 1000, 1002; StartTime = (Get-Date).AddDays(-2) } -ErrorAction SilentlyContinue |
            Where-Object { $_.Message -match '(?i)bf3\.exe|injector\.exe|EADesktop|EASteamLauncher|EABackgroundService|Origin\.exe' } | Select-Object -First 12)
        if (-not $events.Count) { RepAdd '  none of the game''s, the mod''s or the EA app''s' }
        foreach ($e in $events) {
            RepAdd ("  {0:yyyy-MM-dd HH:mm:ss} event {1}: {2}" -f $e.TimeCreated, $e.Id, ((($e.Message -split "`r?`n") | Where-Object { $_.Trim() } | Select-Object -First 7) -join ' | ')) 760
        }
    } 'Windows'' Application log'

    RepLog 'The launcher''s last run (build\launcher.log)' (Join-Path $repBuild 'launcher.log') 0 400
    RepLog 'The launcher''s run before that (build\launcher.prev.log)' (Join-Path $repBuild 'launcher.prev.log') 0 150
    RepLog 'The injector (build\injector.log)' (Join-Path $repBuild 'injector.log') 0 200
    RepLog 'The mod''s launch report (build\launch.log)' (Join-Path $repBuild 'launch.log') 0 200
    # Its key lines from the whole of it (its middle is left out above: a player's held M4A1 not taken by the hands was explained
    # only there, 2026-11-18): the guns, the hands, the picture, the view, the controllers, problems.
    RepSection 'The mod''s log (build\bf3vr.log): its key lines from all of it'
    RepTry {
        $mlog = Join-Path $repBuild 'bf3vr.log'
        if (-not (Test-Path -LiteralPath $mlog)) { RepAdd '  (not there)'; return }
        $keys = '(?i)Held gun|Ammo: the gun in (hand|the hands)|Ammo: the soldier''s slots|Free hands|Hands: draws placed|Holster:|Weapon: bones|Swapchains|Window: fitted|Render FOV|FOV: source|Mode -> |Controllers: |Tracking: |Bare arms: (allowed|off|on)|Arms: |Recoil: (on|the gun)|Pickups?: |CRASH|PROBLEM|didn''t|refused|not known|unknown gun'
        $last = @{}
        $hits = @(Select-String -LiteralPath $mlog -Pattern $keys -Encoding UTF8 -ErrorAction Stop | ForEach-Object { $_.Line })
        # A line that repeats every second (with numbers changing) is kept once, its last time, with how many times it came.
        $order = New-Object System.Collections.Generic.List[string]
        foreach ($h in $hits) {
            $shape = ($h -replace '^\[[^\]]*\] ', '') -replace '[-\d.]+', '#'
            if (-not $last.ContainsKey($shape)) { $order.Add($shape); $last[$shape] = @($h, 1) } else { $last[$shape] = @($h, $last[$shape][1] + 1) }
        }
        RepAdd ("  ({0} key lines, {1} kinds; a kind that came again is shown once, its last time, with its count)" -f $hits.Count, $order.Count)
        foreach ($k in ($order | Select-Object -Last 150)) {
            $v = $last[$k]
            RepAdd ('  ' + $v[0] + $(if ($v[1] -gt 1) { "   (x$($v[1]))" } else { '' })) 600
        }
    } 'the mod''s log''s key lines'
    RepLog 'The mod''s log (build\bf3vr.log): its start and its end' (Join-Path $repBuild 'bf3vr.log') 80 80
    RepLog 'The game start before that: its launch report (build\launch.prev.log)' (Join-Path $repBuild 'launch.prev.log') 0 60
    RepLog 'The game start before that: its mod log''s end (build\bf3vr.prev.log)' (Join-Path $repBuild 'bf3vr.prev.log') 0 80
    RepLog 'The sound maker (build\gamesounds.log): its end' (Join-Path $repBuild 'gamesounds.log') 0 15
    RepLog 'The gun shape maker (build\guns\gunshape.log): its last run''s end' (Join-Path $repBuild 'guns\gunshape.log') 0 15

    RepSection 'Steam''s own log: its lines about Battlefield 3 (logs\console_log.txt)'
    RepTry {
        $console = RepJoin $steamPath 'logs\console_log.txt'
        if (-not (RepFileThere $console)) { RepAdd '  (not there)'; return }
        $hits = @(Select-String -LiteralPath $console -Pattern '1238820' -Encoding UTF8 -ErrorAction Stop | Select-Object -Last 40)
        if (-not $hits.Count) { RepAdd '  none' }
        foreach ($h in $hits) { RepAdd ('  ' + $h.Line) 400 }
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
                # (The EA app's start-up floods its log with license and catalogue lines about every game in the library; they pushed a failed
                # launch's own lines out of the last 50: a player's second report, 2026-11-18. Those are left out.)
                $noise = '(?i)\[CloudSync\] New file added|Skipping license request|requestLicenseRefresh|launchInfo for license refresh|evaluateLicenseRefresh|LicenseManager::requestLicense|No bundled OOA|suppressed by another offerId|fixParentChildRelationships|fillCatalogLocalUninstall|reconcileContentInstallInfo|getBaseSlug|fillLocalInstallProperties|Entitlement data (changed|loaded)|Reconciling content|handleEntitlementDataLoaded|entitlement store loaded|refreshEntitlements|reloadEntitlements|auto-refresh'
                $hits = @(Select-String -LiteralPath $p -Pattern '(?i)error|fail|denied|exception|crash|launch|1238820|bf3|battlefield 3|terminat|proxied|ProcessCoordinator' -Encoding UTF8 -ErrorAction SilentlyContinue |
                    Where-Object { $_.Line -notmatch $noise } | Select-Object -Last 60)
                if (-not $hits.Count) { RepAdd '  none' }
                foreach ($h in $hits) { RepAdd ('  ' + $h.Line) 400 }
            }
        }
    } 'the EA app''s logs'

    RepSection 'End of the report'
    try {
        # (UTF-8 marked as such, so Notepad and Windows PowerShell 5.1 read a folder's accented name right.)
        [IO.File]::WriteAllLines($repFile, $script:repLines, (New-Object Text.UTF8Encoding $true))
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

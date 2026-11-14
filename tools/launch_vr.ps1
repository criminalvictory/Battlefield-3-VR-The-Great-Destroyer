# BF3 in VR in one double-click ("Start BF3 VR.bat" in the folder above this one, or a desktop shortcut to this script): starts
# SteamVR, the mod's injector and Battlefield 3 (through Steam, or Battlelog in the browser for the EA app's copy), waits for the
# game and the mod, then clicks RESUME CAMPAIGN once the campaign has been played with the mod on this PC (the first time the
# game's menu is yours: START NEW CAMPAIGN or RESUME CAMPAIGN). The mod switches to the headset's view by itself the moment
# you're in a level. On the first start it also makes the mod's sounds from your own copy of the game (build\gamesounds.exe).
# Works from any folder: everything is found from this script's own place (build\ and tools\ under the same folder).
#   tools\launch_vr.ps1          resume the campaign where it was left (the first time: the game's menu is yours)
#   tools\launch_vr.ps1 -Menu    stop at the campaign menu (to pick a mission: Replay Missions)
#   tools\launch_vr.ps1 -Check   only the checks and the first start's sounds: says what it finds and what it would do, starts nothing
#                                (no SteamVR, no injector, no game) and changes none of the game's settings
param([switch]$Menu, [switch]$Check)

$root = Split-Path $PSScriptRoot
$build = Join-Path $root 'build'
$injector = Join-Path $build 'injector.exe'
$dll = Join-Path $build 'bf3vr.dll'
$log = Join-Path $build 'bf3vr.log'
$injectorLog = Join-Path $build 'injector.log'
$stateFile = Join-Path $build 'launcher_state.txt'
$Host.UI.RawUI.WindowTitle = 'BF3 VR launcher'

function Say([string]$text) { Write-Host ("[{0:HH:mm:ss}] {1}" -f (Get-Date), $text) }
function Stop-Here([string]$text) {
    Say $text
    Read-Host 'Press Enter to close' | Out-Null
    exit 1
}
# Anything unexpected: said plainly, never a window that closes before it can be read.
trap {
    Say "Something went wrong: $_"
    Say 'Run this again. If it happens again, the line above is the one to report.'
    Read-Host 'Press Enter to close' | Out-Null
    exit 1
}

# A gameplay line in the mod's log: the game's own view in use, the player's gun found, or the mod's own switch to the headset's
# view as the player's soldier comes in (tools\resume.ps1 waits for the same).
$gameplay = 'FOV: source 55\.0, render view 100|Ammo: the gun in hand|Mode -> stereo \(the player'

if (Get-Process bf3 -ErrorAction SilentlyContinue) {
    Stop-Here 'Battlefield 3 is already running. Close it first, then run this again (the mod loads only as the game starts).'
}
# (Every path of the package's own is taken literally: a folder named with [ ] in it would otherwise be read as a pattern.)
# Files that came in a downloaded zip carry Windows' "from the internet" mark: taken off this package's own programs and scripts so
# Windows doesn't stop them (nothing else is touched).
foreach ($spot in @(@($build, '*.exe'), @($build, '*.dll'), @($PSScriptRoot, '*.ps1'), @($root, '*.bat'))) {
    Get-ChildItem -LiteralPath $spot[0] -Filter $spot[1] -File -ErrorAction SilentlyContinue | Unblock-File -ErrorAction SilentlyContinue
}
foreach ($need in $injector, $dll) {
    if (-not (Test-Path -LiteralPath $need)) {
        Stop-Here ("The mod's file $need is missing. Unzip the whole package again (keep its folders as they are). If it's missing " +
                   "again right after unzipping, your antivirus removed it: allow it there (the injector loads the mod into the game, " +
                   "which antivirus programs sometimes mistake for an attack).")
    }
}
# Which build this is: the package's VERSION file, else (a build from the source) the last change in git.
$versionFile = Join-Path $root 'VERSION'
if (Test-Path -LiteralPath $versionFile) {
    Say ('BF3 VR ' + (Get-Content -LiteralPath $versionFile -TotalCount 1).Trim())
} else {
    $change = if (Get-Command git -ErrorAction SilentlyContinue) { git -C $root log -1 --format=%s 2>$null }
    Say ("The mod's build from {0:ddd d MMM, HH:mm}{1}" -f (Get-Item -LiteralPath $dll).LastWriteTime, $(if ($change) { ": $change" } else { '' }))
}

# A place under a folder, and whether a file is there: never an error, whatever the folder (a Steam library on a drive that's gone, a
# folder named with [ ] in it, a typing slip): Join-Path and Test-Path stop this window on those.
function Under([string]$dir, [string]$rest) { if (-not $dir) { return '' }; try { [IO.Path]::Combine($dir, $rest) } catch { '' } }
function FileThere([string]$path) { if (-not $path) { return $false }; try { [IO.File]::Exists($path) } catch { $false } }

# Steam and its libraries (SteamVR comes with Steam; the game may be Steam's copy).
$steamDir = [string](Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath -replace '/', '\'
function SteamLibraries {
    if (-not $steamDir) { return @() }
    $libraries = @($steamDir)
    $vdf = Under $steamDir 'steamapps\libraryfolders.vdf'
    if (FileThere $vdf) {
        try { foreach ($m in [regex]::Matches([IO.File]::ReadAllText($vdf), '"path"\s+"([^"]+)"')) { $libraries += ($m.Groups[1].Value -replace '\\\\', '\') } } catch {}
    }
    $libraries | Select-Object -Unique
}
# Battlefield 3's folder: as the EA app installed it (Steam's copy is installed through it too), else in Steam's libraries, else where
# Windows lists it among the installed programs, else in the EA app's and Origin's usual folders, else the folder the player gave here
# before (build\gamefolder.txt). Without a trailing \ (a quoted "...\" would swallow its closing quote on a program's command line).
$gameFolderFile = Join-Path $build 'gamefolder.txt'
function IsGameFolder([string]$dir) { FileThere (Under $dir 'bf3.exe') }
function GameFolder {
    $dir = [string](Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\EA Games\Battlefield 3' -ErrorAction SilentlyContinue).'Install Dir'
    if (IsGameFolder $dir) { return $dir.TrimEnd('\') }
    foreach ($lib in SteamLibraries) {
        $manifest = Under $lib 'steamapps\appmanifest_1238820.acf'
        $name = 'Battlefield 3'
        try { if ((FileThere $manifest) -and ([IO.File]::ReadAllText($manifest) -match '"installdir"\s+"([^"]+)"')) { $name = $Matches[1] } } catch {}
        $candidate = Under $lib "steamapps\common\$name"
        if (IsGameFolder $candidate) { return $candidate.TrimEnd('\') }
    }
    foreach ($programs in 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*') {
        foreach ($p in @(Get-ItemProperty $programs -ErrorAction SilentlyContinue)) {
            if ([string]$p.DisplayName -match '^Battlefield 3(\W|$)' -and (IsGameFolder $p.InstallLocation)) { return ([string]$p.InstallLocation).TrimEnd('\') }
        }
    }
    foreach ($usual in @((Under $env:ProgramFiles 'EA Games\Battlefield 3'), (Under ${env:ProgramFiles(x86)} 'Origin Games\Battlefield 3'),
                         (Under ${env:ProgramFiles(x86)} 'EA Games\Battlefield 3'))) {
        if (IsGameFolder $usual) { return $usual }
    }
    if (FileThere $gameFolderFile) {
        $given = ([string](Get-Content -LiteralPath $gameFolderFile -TotalCount 1 -ErrorAction SilentlyContinue)).Trim().Trim('"').TrimEnd('\')
        if (IsGameFolder $given) { return $given }
    }
    $null
}
# Whether Steam owns the copy (its manifest in one of Steam's libraries): then Steam starts it; else it's the EA app's.
function SteamCopy { foreach ($lib in SteamLibraries) { if (FileThere (Under $lib 'steamapps\appmanifest_1238820.acf')) { return $true } }; $false }
function SteamVrInstalled {
    foreach ($lib in SteamLibraries) { if (FileThere (Under $lib 'steamapps\common\SteamVR\bin\win64\vrserver.exe')) { return $true } }
    $paths = Under $env:LOCALAPPDATA 'openvr\openvrpaths.vrpath'
    if (FileThere $paths) {
        try { foreach ($r in ([IO.File]::ReadAllText($paths) | ConvertFrom-Json).runtime) { if (FileThere (Under $r 'bin\win64\vrserver.exe')) { return $true } } } catch {}
    }
    $false
}

$gameDir = GameFolder
if (-not $gameDir) {
    # Not found by itself (an install the EA app keeps somewhere unusual, a copy moved by hand): the player says where, kept for next time.
    Say 'Battlefield 3 wasn''t found by itself (not in the EA app''s or Steam''s records, nor in the usual folders).'
    Say 'If it''s installed, paste its folder here: the one with bf3.exe in it, for example C:\Program Files\EA Games\Battlefield 3.'
    Say 'If it isn''t installed: install it from Steam or the EA app and start it once the normal way (Battlelog''s PLAY CAMPAIGN) first.'
    while (-not $gameDir) {
        $answer = ([string](Read-Host 'Battlefield 3''s folder (just Enter to stop)')).Trim().Trim('"').TrimEnd('\')
        if (-not $answer) { Say 'Stopped: nothing started.'; Start-Sleep 3; exit 1 }
        if (IsGameFolder $answer) {
            $gameDir = $answer
            try { Set-Content -LiteralPath $gameFolderFile -Value $gameDir -Encoding UTF8 -ErrorAction Stop } catch {}
        } else {
            Say "There's no bf3.exe in $answer. Paste the folder that has bf3.exe in it."
        }
    }
}
Say "Battlefield 3: $gameDir"
if (-not $steamDir -or -not (FileThere (Under $steamDir 'steam.exe'))) {
    Stop-Here 'Steam isn''t installed. The mod needs SteamVR for the headset, and SteamVR comes with Steam: install Steam, then SteamVR from its Store (free), then run this again.'
}
if (-not (SteamVrInstalled)) {
    Stop-Here ('SteamVR isn''t installed. Install it from Steam''s Store (it''s free), connect your headset to it once (a Quest: Steam Link, ' +
               'Virtual Desktop in SteamVR mode, or Quest Link / Air Link), then run this again.')
}

# Other graphics hooks in the game folder (ReShade and the like) break the headset's view: say so first.
$hooks = foreach ($name in 'dxgi.dll', 'd3d11.dll', 'd3d9.dll', 'dinput8.dll') { if (FileThere (Under $gameDir $name)) { $name } }
if ($hooks) { Say ("Heads up: {0} in the game folder may fight the mod's view (ReShade or similar). Move it out if the headset shows nothing." -f ($hooks -join ', ')) }

# The mod needs the game in a window (it sizes the game's picture for the headset; in the game's own full screen the picture stays
# the monitor's): a game set to full screen is set to windowed in its own settings file, once, the old file kept beside it.
# A file it can't change (read-only, or kept from changes by Windows Security's Controlled folder access) only says so: the start goes on.
# No settings file at all: the game has never been started on this PC, and would open in full screen: said, and waited for.
$profileFile = Under ([Environment]::GetFolderPath('MyDocuments')) 'Battlefield 3\settings\PROF_SAVE_profile'
$bytes = $null
if (FileThere $profileFile) {
    try { $bytes = [IO.File]::ReadAllBytes($profileFile) } catch { Say "Battlefield 3's settings file couldn't be read ($($_.Exception.Message)): if the game opens in full screen, set it to windowed in its OPTIONS, VIDEO." }
}
if ($bytes) {
    $text = [Text.Encoding]::GetEncoding(28591).GetString($bytes)  # one letter a byte: an index is a byte's place
    $m = [regex]::Match($text, '(?m)^GstRender\.FullscreenEnabled 1\r?$')
    if ($m.Success -and $Check) { Say 'Battlefield 3 is set to full screen: a real start sets it to windowed (the mod needs a window for the headset''s picture).' }
    elseif ($m.Success) {
        $backup = "$profileFile.before-bf3vr"
        try {
            if (-not (FileThere $backup)) { Copy-Item -LiteralPath $profileFile -Destination $backup -ErrorAction Stop }
            $bytes[$m.Index + 'GstRender.FullscreenEnabled '.Length] = [byte][char]'0'
            [IO.File]::WriteAllBytes($profileFile, $bytes)
            Say "Battlefield 3 was set to full screen: set to windowed in its settings (the mod needs a window for the headset's picture). The old settings file is kept as $backup."
        } catch {
            Say (("Battlefield 3 is set to full screen, and its settings file couldn't be changed ({0}). In the game's OPTIONS, VIDEO, set FULLSCREEN " +
                  "to OFF: the mod needs a window for the headset's picture (if the headset still shows nothing, close the game and run this again).") -f $_.Exception.Message)
        }
    }
} elseif (-not (FileThere $profileFile)) {
    Say "Battlefield 3 has no settings on this PC yet ($profileFile isn't there): it seems it has never been started here, and it would open in full screen."
    Say 'Start it once the normal way first (Battlelog''s PLAY CAMPAIGN), set OPTIONS, VIDEO, FULLSCREEN to OFF and close it: the mod needs the game in a window.'
    if (-not $Check) { Read-Host 'Press Enter to start anyway, or close this window' | Out-Null }
}

# The first start: the mod's sounds (reloads, magazines, holsters, things landing) made from your own copy of the game. None ship with the
# mod: build\gamesounds.exe cuts them out of the game's own sound files into build\sounds, once.
$sounds = Join-Path $build 'sounds'
$cutter = Join-Path $build 'gamesounds.exe'
# (Start-Process reads its working folder as a pattern: given escaped. The sound cutter's output goes to its log through .NET's own start,
# as Start-Process can't write to a file in a folder named with [ ] at all.)
function Literal([string]$path) { [Management.Automation.WildcardPattern]::Escape($path) }
if (-not (Get-ChildItem -LiteralPath $sounds -Filter '*.wav' -File -ErrorAction SilentlyContinue | Select-Object -First 1)) {
    if (FileThere $cutter) {
        Say 'First start: making the mod''s sounds from your own copy of the game (once; about a minute)...'
        $cut = $null
        try {
            $start = New-Object Diagnostics.ProcessStartInfo
            $start.FileName = $cutter
            $start.Arguments = '--mod --game "{0}"' -f $gameDir
            $start.WorkingDirectory = $build
            $start.UseShellExecute = $false
            $start.CreateNoWindow = $true
            $start.RedirectStandardOutput = $true
            $start.RedirectStandardError = $true
            $cut = [Diagnostics.Process]::Start($start)
        } catch {
            Say ("build\gamesounds.exe, which makes the sounds, couldn't be started ($($_.Exception.Message)): your antivirus may have blocked it. " +
                 'The mod works without them (the reloads by hand are then silent); allow that file there to have them. This is tried again next time.')
        }
        if ($cut) {
            $said = $cut.StandardOutput.ReadToEndAsync(); $errors = $cut.StandardError.ReadToEndAsync()
            if (-not $cut.WaitForExit(600000)) { try { $cut.Kill() } catch {}; Say 'Making the sounds took more than 10 minutes: stopped.' }
            $cut.WaitForExit()
            try { [IO.File]::WriteAllText((Join-Path $build 'gamesounds.log'), $said.Result + $(if ($errors.Result) { "`r`n" + $errors.Result } else { '' })) } catch {}
            $made = @(Get-ChildItem -LiteralPath $sounds -Filter '*.wav' -File -ErrorAction SilentlyContinue).Count
            if ($cut.ExitCode -eq 0) { Say "Made $made sounds in $sounds." }
            else {
                Say (("The mod's sounds couldn't all be made ({0} made; build\gamesounds.log says why). The mod works without them: the reloads by " +
                      "hand are then silent. This is tried again next time.") -f $made)
            }
        }
    } else {
        Say 'No sounds yet (build\sounds is empty) and build\gamesounds.exe isn''t there to make them: the reloads by hand will be silent.'
    }
}

# Whether the campaign has been played with the mod on this PC before: then the game's menu is clicked for you (RESUME CAMPAIGN).
# A new player has no RESUME CAMPAIGN in the menu; the first time it's yours to click. Read now, before this start's log replaces the last.
$playedBefore = (FileThere $stateFile) -or ((FileThere $log) -and (Select-String -LiteralPath $log -Pattern $gameplay -Quiet))
if ($Check) {
    Say ('Check done. A real start would now: start SteamVR (or use the one running), start the injector, start the game {0}, and {1}.' -f
         $(if (SteamCopy) { 'through Steam (its copy)' } else { 'through Battlelog in the browser (the EA app''s copy)' }),
         $(if ($Menu) { 'stop at the campaign menu' } elseif ($playedBefore) { 'click RESUME CAMPAIGN for you' } else { 'leave the campaign menu to you (the first time)' }))
    exit 0
}

# SteamVR first, always (the user, 2026-11-01): without it the mod's tries to reach the headset stalled the game
# every few seconds ("the game is running really slow").
# And streaming over the Steam Frame's dongle (Valve's USB Wi-Fi adapter), not the home Wi-Fi: SteamVR takes the
# dongle only if the dongle's link to the headset is up when SteamVR starts; started before that, it streams over
# the home network and stays on it. The user's fix by hand: close SteamVR, wait for the headset to show the dongle
# active, start SteamVR again. This does the same. Any other headset: SteamVR's own link, nothing waited for.
function SteamVrUp { (Get-Process vrserver -ErrorAction SilentlyContinue) -and (Get-Process vrcompositor -ErrorAction SilentlyContinue) }
function DongleNic { Get-NetAdapter -ErrorAction SilentlyContinue | Where-Object { $_.InterfaceDescription -match 'for Valve' } | Select-Object -First 1 }
# The PC's address on the dongle's link to the headset, while it's up.
function DongleIp {
    $nic = DongleNic
    if (-not $nic -or $nic.Status -ne 'Up') { return $null }
    Get-NetIPAddress -InterfaceIndex $nic.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -notlike '169.254.*' } | Select-Object -First 1 -ExpandProperty IPAddress
}
# The PC's address SteamVR streams to the headset from, the newest since $since in its link driver's log
# ("Found primary link 10.35.78.25"); none before the headset connects.
function SteamVrLink([datetime]$since) {
    $file = if ($steamDir) { Join-Path $steamDir 'logs\driver_vrlink.txt' }
    if (-not $file -or -not (Test-Path $file)) { return $null }
    $link = $null
    foreach ($line in (Get-Content $file -Tail 4000 -ErrorAction SilentlyContinue)) {
        if ($line -match '^\w{3} (\w{3} \d{2} \d{4} \d{2}:\d{2}:\d{2}\.\d{3}) .*Found primary link (\S+)') {
            $when = [datetime]::ParseExact($Matches[1], 'MMM dd yyyy HH:mm:ss.fff', [Globalization.CultureInfo]::InvariantCulture)
            if ($when -ge $since) { $link = $Matches[2] }
        }
    }
    $link
}
function StopSteamVr {
    Get-Process vrmonitor, vrdashboard, vrcompositor, vrserver, vrwebhelper, vrstartup -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
    $t = 0
    while ((Get-Process vrserver, vrcompositor, vrmonitor -ErrorAction SilentlyContinue) -and $t -lt 20) { Start-Sleep 1; $t++ }
    Start-Sleep 3
}
function WaitForDongle {
    if (DongleIp) { return $true }
    Say 'Waiting for the Steam Frame''s dongle: turn the headset on and wait until it shows the adapter active...'
    $t = 0
    while (-not (DongleIp) -and $t -lt 300) { Start-Sleep 1; $t++ }
    [bool](DongleIp)
}

# None plugged in (a cable, another headset): SteamVR's own choice. A Valve Index on its cable (USB VID_28DE PID_2300)
# with the Frame's dongle still plugged in has no link to wait for either: five minutes, then giving up (2026-11-02).
$cabled = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue | Where-Object { $_.InstanceId -match '^USB\\VID_28DE&PID_2300\\' } | Select-Object -First 1
$dongle = if ($cabled) { $null } else { DongleNic }
if ($cabled) { Say 'A Valve Index is plugged in: no dongle to wait for.' }
if ($dongle -and (SteamVrUp)) {
    $link = SteamVrLink (Get-Process vrserver | Select-Object -First 1).StartTime
    if ($link -and $link -ne (DongleIp)) {
        Say "SteamVR is streaming over the home network ($link), not the dongle: restarting it."
        StopSteamVr
    }
}
for ($try = 1; -not (SteamVrUp) -and $try -le 2; $try++) {
    if ($dongle -and -not (WaitForDongle)) {
        Stop-Here 'The dongle''s link to the headset didn''t come up in 5 minutes. Check the headset is on and the dongle plugged in, then run this again.'
    }
    $since = Get-Date
    # Steam not running yet: it starts first (and may sign in or update before SteamVR), so it's given longer.
    $steamFirst = -not (Get-Process steam -ErrorAction SilentlyContinue)
    $wait = if ($steamFirst) { 240 } else { 90 }
    Start-Process 'steam://rungameid/250820'
    Say $(if ($steamFirst) { 'Starting Steam, then SteamVR (Steam may sign in or update first)...' } else { 'Starting SteamVR...' })
    $t = 0
    while (-not (SteamVrUp) -and $t -lt $wait) { Start-Sleep 1; $t++ }
    if (-not (SteamVrUp)) {
        Stop-Here (('SteamVR didn''t start within {0} seconds. Turn the headset on and connect it (a Quest: Steam Link, Virtual Desktop in ' +
                    'SteamVR mode, or Quest Link / Air Link), start SteamVR from Steam and wait until it shows the headset, then run this again.') -f $wait)
    }
    if ($dongle) {
        # Which way it streams once the headset connects (up to a minute; with the headset asleep, left be).
        $link = $null; $t = 0
        while (-not $link -and $t -lt 60) { Start-Sleep 1; $t++; $link = SteamVrLink $since }
        if ($link -and $link -ne (DongleIp) -and $try -lt 2) {
            Say "SteamVR took the home network ($link), not the dongle: restarting it."
            StopSteamVr
            continue
        }
    }
    Start-Sleep 5  # settled before the game asks for the headset
}
$link = if ($dongle) { SteamVrLink (Get-Process vrserver | Select-Object -First 1).StartTime }
if ($link -and $link -eq (DongleIp)) { Say 'SteamVR is running, streaming over the dongle.' }
elseif ($link) { Say "SteamVR is running, but streaming over the home network ($link). If the picture stutters: close the game and SteamVR, wait for the headset to show the dongle active, and run this again." }
else { Say 'SteamVR is running.' }

# The injector waits for bf3.exe and loads the mod into it as it starts, only into the campaign (Battlelog's PLAY CAMPAIGN): a
# multiplayer or co-op start is refused. Any of this package's injectors left over from before is stopped first.
Get-Process injector -ErrorAction SilentlyContinue | Where-Object { $_.Path -and ($_.Path -eq $injector) } | Stop-Process -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $injectorLog -ErrorAction SilentlyContinue
$started = Get-Date
try {
    $inject = Start-Process -FilePath $injector -WorkingDirectory (Literal $root) -WindowStyle Hidden -PassThru -ErrorAction Stop
} catch {
    Stop-Here ("Windows wouldn't start the mod's build\injector.exe ($($_.Exception.Message)). If your antivirus blocked or removed it: restore it " +
               'from its quarantine (Windows Security: Virus & threat protection, Protection history) and allow it there (only this file or this ' +
               'folder; never switch the antivirus off), unzip again if it''s gone, then run this again.')
}
$null = $inject.Handle  # (kept, so its exit code can be read once it's done)
Say 'The mod is waiting for the game.'

# Steam's copy is started through Steam, as the user starts it by hand: Steam starts the EA app signed in through the account Steam
# is linked to, and the EA app opens Battlelog. The EA app's own copy: Battlelog in the browser, whose PLAY CAMPAIGN has the EA app
# start the game.
if (SteamCopy) {
    Start-Process 'steam://rungameid/1238820'
    Say 'Starting Battlefield 3 through Steam: Steam starts the EA app, which opens Battlelog in your browser.'
} else {
    Start-Process 'https://battlelog.battlefield.com/bf3/'
    Say 'Battlefield 3 here is the EA app''s copy: Battlelog is opening in your browser (the EA app starts the game from there).'
}
Say 'In Battlelog, click PLAY CAMPAIGN (the one click left to you: the game''s own launch).'
Say '(If it asks you to sign in, sign in there with your EA account. This window never touches your account.)'
Say 'The mod is for the campaign only: it stays out of multiplayer and co-op by itself. Never use it online.'

# Up to 10 minutes for the game and the mod in it. What the injector says (build\injector.log) is shown as it comes: a start it
# refused (multiplayer, co-op, or one it couldn't read) and why. A build of the injector from before the campaign-only rule writes
# no log: then the mod's own log alone.
$deadline = $started.AddMinutes(10)
$seenGame = $false
$loaded = $false
$shown = @{}
while (-not $loaded -and (Get-Date) -lt $deadline) {
    Start-Sleep 1
    if (-not $seenGame -and (Get-Process bf3 -ErrorAction SilentlyContinue)) { $seenGame = $true; Say 'Battlefield 3 is starting.' }
    $injectorSays = if ((FileThere $injectorLog) -and (Get-Item -LiteralPath $injectorLog).LastWriteTime -ge $started.AddSeconds(-2)) { @(Get-Content -LiteralPath $injectorLog -ErrorAction SilentlyContinue) } else { @() }
    foreach ($line in $injectorSays) {
        if ($line -match '\] (REFUSED: .*)$' -and -not $shown.ContainsKey($line)) {
            $shown[$line] = $true
            Say $Matches[1]
            Say 'This window keeps waiting: close that game, then click PLAY CAMPAIGN in Battlelog.'
        }
    }
    $fresh = (FileThere $log) -and (Get-Item -LiteralPath $log).LastWriteTime -gt $started
    $first = if ($fresh) { Get-Content -LiteralPath $log -TotalCount 1 -ErrorAction SilentlyContinue } else { $null }
    $loaded = $first -match 'bf3vr loaded'
    if (-not $loaded -and $first -match '(bf3vr stays out: .*)' -and -not $shown.ContainsKey($first)) { $shown[$first] = $true; Say $Matches[1] }
    if (-not $loaded -and $inject.HasExited) {
        $last = $injectorSays | Where-Object { $_ -notmatch 'Waiting for|Found ' } | Select-Object -Last 2
        # Unless its last words name another cause (the game run as administrator, the wrong build), an antivirus stopping it or the mod as
        # it goes into the game is the likely one.
        $otherCause = ($last | Select-Object -Last 1) -match 'administrator|Bitness'
        Stop-Here ("The mod's injector stopped before the mod was in (exit code {0}){1} Close Battlefield 3 if it's running, then run this again.{2}" -f
                   $inject.ExitCode, $(if ($last) { ': ' + (($last | ForEach-Object { $_ -replace '^\[[^\]]*\] ', '' }) -join ' ') } else { '.' }),
                   $(if ($otherCause) { '' } else { ' If your antivirus stopped build\injector.exe or build\bf3vr.dll, allow them there (only these files or this folder).' }))
    }
}
if (-not $loaded) {
    if (-not $inject.HasExited) { Stop-Process -Id $inject.Id -Force -ErrorAction SilentlyContinue }
    if (-not $seenGame) { Stop-Here 'The game didn''t start within 10 minutes. The mod''s injector is stopped; run this again when you''re ready.' }
    Stop-Here 'Battlefield 3 started, but the mod didn''t load into it within 10 minutes (the lines above say why, if anything did). Close Battlefield 3 and run this again.'
}
Say 'The mod is in. It switches to the headset as soon as you''re in a level.'
Say 'When you''re done, close the game before you play online: never join a multiplayer server from Battlelog while this game is open.'
function MarkPlayed { try { Set-Content -LiteralPath $stateFile -Value ("The campaign has been played with the mod on this PC (last seen {0:yyyy-MM-dd HH:mm}): the launcher clicks RESUME CAMPAIGN." -f (Get-Date)) -Encoding ASCII } catch {} }

if ($Menu) {
    Say 'Pick your mission in the campaign menu (Replay Missions). This window closes in 10 s.'
    Start-Sleep 10
    exit 0
}

if ($playedBefore) {
    Say 'Waiting for the menu, then clicking RESUME CAMPAIGN for you (if the menu stays, click it yourself)...'
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'resume.ps1')
    if ($LASTEXITCODE -ne 0) {
        Say 'The menu click didn''t take: click RESUME CAMPAIGN yourself in the game''s menu.'
        Start-Sleep 15
        exit 1
    }
    MarkPlayed
    Say 'Campaign resumed. Put on the headset. This window closes in 10 s.'
    Start-Sleep 10
    exit 0
}

# The first time with the mod on this PC: a new player has no RESUME CAMPAIGN, so nothing is clicked for you.
Say 'First start with the mod here: in the game''s menu click START NEW CAMPAIGN and pick a difficulty (or RESUME CAMPAIGN if you''ve played the campaign before).'
Say 'A new campaign opens with Semper Fidelis (the train). The mod switches to the headset as the level starts: put the headset on then.'
Say 'From the next start on, this window clicks RESUME CAMPAIGN for you.'
$t = 0
$inLevel = $false
while (-not $inLevel -and $t -lt 1800 -and (Get-Process bf3 -ErrorAction SilentlyContinue)) {
    Start-Sleep 2; $t += 2
    $inLevel = (FileThere $log) -and (Select-String -LiteralPath $log -Pattern $gameplay -Quiet)
}
if ($inLevel) {
    MarkPlayed
    Say 'You''re in the level: put on the headset. This window closes in 10 s.'
} else {
    Say 'No level started while this window waited (30 minutes, or the game closed). Next time, the menu is yours again until a level has started.'
}
Start-Sleep 10

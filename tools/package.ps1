# Makes the release a player downloads: a clean build of one commit, laid out as tools\launch_vr.ps1 expects, with the settings the mod is
# played with (god mode on, holsters on since 0.1.4: pre-alpha), START HERE.txt and the third-party licences, zipped. Nothing of the game's own (no
# sounds: the launcher makes them on the player's PC from their own copy of the game), no logs, no dumps, nothing private.
#   tools\package.ps1 [-Commit HEAD] [-Version 0.1.0-prealpha] [-Worktree <folder>] [-Builder <bat>] [-SettingsFrom <folder>] [-Out <folder>]
#                     [-Forbid <word>,<word>] [-WithGunShapes] [-NoBuild]
# -Worktree: a git worktree of this repository the commit is checked out in and built in (made when it isn't there; it must hold no
#   changes of its own). -Builder: a batch file that builds the worktree given as its first argument into its build\ folder (32-bit
#   Release); without one, this script builds it as build.bat does, with this repository's own fetched OpenXR SDK and MinHook when
#   build\_deps has them. -SettingsFrom: the folder the played settings and what the mod learned are read from (bf3vr.ini, grip.txt,
#   pistol.txt, aimroute.txt, m320grip.bin; default build\). -WithGunShapes: the guns' collision shapes too (build\guns\*.bin, about
#   70 MB; made from the game's meshes: left out by default, the mod makes each on the player's PC with build\gunshape.exe). -NoBuild: package the worktree's build as it is (it must be of that commit).
#   -Forbid: words no file of the package may hold (names that must never ship), besides this PC's folders and users' folders.
# Writes <Out>\BF3-VR-<Version>\ and <Out>\BF3-VR-<Version>.zip (default Out: dist\, which git ignores).
param(
    [string]$Commit = 'HEAD',
    [string]$Version = '0.1.0-prealpha',
    [string]$Worktree = '',
    [string]$Builder = '',
    [string]$SettingsFrom = '',
    [string]$Out = '',
    [string[]]$Forbid = @(),
    [switch]$WithGunShapes,
    [switch]$NoBuild,
    # One exact injector.exe across releases (the one Windows Security's false alarm was reported to Microsoft for, so one clearance
    # covers every release): shipped instead of the one built, but only while the injector's code is unchanged since -InjectorSince.
    [string]$InjectorFrom = '',
    [string]$InjectorSince = ''
)
$root = Split-Path $PSScriptRoot
if (-not $Worktree) { $Worktree = Join-Path $env:TEMP 'bf3vr-package-worktree' }
if (-not $SettingsFrom) { $SettingsFrom = Join-Path $root 'build' }
if (-not $Out) { $Out = Join-Path $root 'dist' }
function Say([string]$text) { Write-Host $text }
function Fail([string]$text) { Write-Host "package: $text" -ForegroundColor Red; exit 1 }
if ($Version -notmatch '^[0-9A-Za-z][0-9A-Za-z.\-]*$') { Fail "-Version '$Version': letters, digits, dots and dashes only" }

# 1. The commit, checked out alone in its worktree.
$hash = (git -C $root rev-parse --verify "$Commit^{commit}" 2>$null)
if (-not $hash) { Fail "no commit '$Commit' in $root" }
$short = $hash.Substring(0, 7)
$date = (git -C $root show -s --format=%cs $hash).Trim()
if (-not (Test-Path (Join-Path $Worktree '.git'))) {
    Say "Making the worktree $Worktree at $short..."
    git -C $root worktree add --detach $Worktree $hash 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { Fail "git worktree add failed for $Worktree" }
} else {
    $changes = git -C $Worktree status --porcelain
    if ($changes) { Fail "the worktree $Worktree holds changes of its own; packaging needs it clean:`n$changes" }
    git -C $Worktree checkout --quiet --detach $hash 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { Fail "git checkout $short failed in $Worktree" }
}
if ((git -C $Worktree rev-parse HEAD).Trim() -ne $hash) { Fail "the worktree isn't at $short" }
if (git -C $Worktree status --porcelain) { Fail "the worktree isn't clean after the checkout" }
$wtBuild = Join-Path $Worktree 'build'

# 2. A clean full build of it, 32-bit Release.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = if (Test-Path $vswhere) { (& $vswhere -latest -products * -property installationPath | Select-Object -First 1) } else { $null }
if (-not $NoBuild) {
    $started = Get-Date
    if ($Builder) {
        Say "Building $short with $Builder..."
        $b = Start-Process -FilePath $Builder -ArgumentList ('"{0}"' -f $Worktree.TrimEnd('\')) -NoNewWindow -PassThru
        $null = $b.Handle  # (kept, so its exit code can be read once it's done)
        $b.WaitForExit()
        if ($b.ExitCode -ne 0) { Fail "the build failed ($Builder, exit $($b.ExitCode))" }
    } else {
        if (-not $vs) { Fail 'Visual Studio (its C++ workload) wasn''t found: needed to build' }
        $deps = Join-Path $root 'build\_deps'
        $fetched = if ((Test-Path (Join-Path $deps 'openxr-src')) -and (Test-Path (Join-Path $deps 'minhook-src'))) {
            ('"-DFETCHCONTENT_SOURCE_DIR_OPENXR={0}" "-DFETCHCONTENT_SOURCE_DIR_MINHOOK={1}" -DFETCHCONTENT_FULLY_DISCONNECTED=ON' -f
             ((Join-Path $deps 'openxr-src') -replace '\\', '/'), ((Join-Path $deps 'minhook-src') -replace '\\', '/'))
        } else { '' }
        $cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake'
        Say "Building $short (32-bit Release)..."
        & cmd /c ("call `"$vs\VC\Auxiliary\Build\vcvarsall.bat`" x86 >nul && set `"PATH=$cmake\CMake\bin;$cmake\Ninja;%PATH%`" && " +
                  "cmake -S `"$Worktree`" -B `"$wtBuild`" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl $fetched && cmake --build `"$wtBuild`"")
        if ($LASTEXITCODE -ne 0) { Fail "the build failed (exit $LASTEXITCODE)" }
    }
    foreach ($name in 'bf3vr.dll', 'injector.exe', 'gamesounds.exe', 'gunshape.exe') {
        $f = Join-Path $wtBuild $name
        if (-not (Test-Path $f)) { Fail "the build made no $name" }
        if ((Get-Item $f).LastWriteTime -lt $started.AddSeconds(-5) -and $Builder) { Say "  ($name was already up to date in the worktree's build)" }
    }
}
# (gunshape.exe: the mod makes each gun's collision shape with it on the player's PC, from their own copy of the game: src\shapemake.h)
$binaries = foreach ($name in 'bf3vr.dll', 'injector.exe', 'gamesounds.exe', 'gunshape.exe') {
    $f = Join-Path $wtBuild $name
    if (-not (Test-Path $f)) { Fail "$f isn't there (build it, or leave out -NoBuild)" }
    $f
}
if ($InjectorFrom) {
    if (-not $InjectorSince) { Fail '-InjectorFrom needs -InjectorSince: the commit that injector was built from' }
    if (-not (Test-Path -LiteralPath $InjectorFrom) -or (Split-Path $InjectorFrom -Leaf) -ne 'injector.exe') { Fail "-InjectorFrom: no injector.exe at $InjectorFrom" }
    git -C $root diff --quiet $InjectorSince $hash -- injector src/campaign.h
    if ($LASTEXITCODE -ne 0) { Fail "the injector's code changed since ${InjectorSince}: build a new one (leave out -InjectorFrom)" }
    $binaries = @($binaries | Where-Object { (Split-Path $_ -Leaf) -ne 'injector.exe' }) + @((Resolve-Path -LiteralPath $InjectorFrom).Path)
    Say ("The injector shipped: {0} (SHA256 {1}), its code unchanged since {2}" -f $InjectorFrom,
         (Get-FileHash -LiteralPath $InjectorFrom -Algorithm SHA256).Hash, $InjectorSince)
}

# 3. The C runtime built in: none of Microsoft's runtime DLLs needed, so a PC without the VC++ redistributable runs it.
$dumpbin = if ($vs) { Get-ChildItem (Join-Path $vs 'VC\Tools\MSVC\*\bin\Hostx64\x86\dumpbin.exe'), (Join-Path $vs 'VC\Tools\MSVC\*\bin\Hostx86\x86\dumpbin.exe') -ErrorAction SilentlyContinue | Select-Object -First 1 } else { $null }
$dependents = [ordered]@{}
if ($dumpbin) {
    foreach ($f in $binaries) {
        $lines = & $dumpbin.FullName /nologo /dependents $f
        $dlls = @($lines | Where-Object { $_ -match '^\s+(\S+\.dll)\s*$' } | ForEach-Object { $Matches[1] })
        $dependents[(Split-Path $f -Leaf)] = $dlls
        $runtime = @($dlls | Where-Object { $_ -match '^(vcruntime|msvcp|ucrtbase|api-ms-win-crt|concrt|vcomp)' })
        if ($runtime) { Fail ("{0} needs {1}: the C runtime isn't built in" -f (Split-Path $f -Leaf), ($runtime -join ', ')) }
    }
} else { Say 'dumpbin wasn''t found: the C runtime check is skipped' }

# 4. The package's folder.
$name = "BF3-VR-$Version"
$dist = Join-Path $Out $name
$zip = Join-Path $Out "$name.zip"
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
if (Test-Path $zip) { Remove-Item -Force $zip }
$pkgBuild = Join-Path $dist 'build'
$pkgTools = Join-Path $dist 'tools'
New-Item -ItemType Directory -Force $pkgBuild, $pkgTools | Out-Null
$crlf = "`r`n"
# A text file written with Windows line endings, in UTF-8 without a mark (the same bytes as ASCII for the scripts and settings, which are).
function WriteText([string]$path, [string]$text) {
    $text = ($text -replace "`r`n", "`n") -replace "`n", $crlf
    [IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding($false)))
}
function CopyText([string]$from, [string]$to) {
    if (-not (Test-Path $from)) { Fail "$from isn't there" }
    WriteText $to ([IO.File]::ReadAllText($from))
}

foreach ($f in $binaries) { Copy-Item $f $pkgBuild -ErrorAction Stop }
CopyText (Join-Path $Worktree 'Start BF3 VR.bat') (Join-Path $dist 'Start BF3 VR.bat')
CopyText (Join-Path $Worktree 'Start BF3 VR (VDXR).bat') (Join-Path $dist 'Start BF3 VR (VDXR).bat')
foreach ($doc in 'README.md', 'CHANGELOG.md', 'LICENSE') { if (Test-Path (Join-Path $Worktree $doc)) { CopyText (Join-Path $Worktree $doc) (Join-Path $dist $doc) } }
CopyText (Join-Path $Worktree 'Make a report.bat') (Join-Path $dist 'Make a report.bat')
foreach ($script in 'launch_vr.ps1', 'gamesettings.ps1', 'resume.ps1', 'drawhunt.ps1', 'report.ps1') { CopyText (Join-Path $Worktree "tools\$script") (Join-Path $pkgTools $script) }

# The settings the mod is played with. bf3vr.ini as it's played (the camera's and the look angles' places in the game, the picture's
# size and angle, the shadows, the world's keeps), god mode on whatever it is now, any line naming a folder of this PC left out.
$ini = Join-Path $SettingsFrom 'bf3vr.ini'
if (-not (Test-Path $ini)) { Fail "$ini isn't there: the played settings are read from it" }
$iniLines = [IO.File]::ReadAllLines($ini)
$kept = New-Object System.Collections.Generic.List[string]
$kept.Add('; BF3 VR pre-alpha: the settings it is played with. God mode is on ([god] enabled=1; End switches it in game, and the choice is')
$kept.Add('; kept here). [window] size is the picture''s size per eye: lower it (2000, 1600) for a slower graphics card; the view gets narrower.')
$section = ''
$godDone = $false
foreach ($line in $iniLines) {
    if ($line -match '^\s*\[([^\]]+)\]') { $section = $Matches[1].ToLower() }
    if ($line -match '[A-Za-z]:[\\/]' -or ($env:USERNAME.Length -ge 3 -and $line -match [regex]::Escape($env:USERNAME))) { continue }
    if ($section -eq 'god' -and $line -match '^\s*enabled\s*=') { $line = 'enabled=1'; $godDone = $true }
    $kept.Add($line)
}
if (-not $godDone) { $kept.Add(''); $kept.Add('[god]'); $kept.Add('enabled=1'); $kept.Add('value=100000') }
WriteText (Join-Path $pkgBuild 'bf3vr.ini') (($kept -join "`n") + "`n")
# weapon.txt: the hand-over lines that shape what's seen and felt (the bare forearms' roll, the window fitted to the monitor, the desktop
# window showing both eyes, the scope's own picture, the head's pose filter, the arm's line, the pose by the draw, the view's naming) and
# the held gun's numbers once a second in the log (for a report). Left out, at their defaults (off): the tests (forcehand, m320test,
# groundprobe, forceheadless, heldtest, rockettest, headwave, standin, viewlog, poserate) and steplog's count of footsteps, which hooks
# two of the game's functions for a log line a player doesn't need.
# No streamview line: the monitor, OBS and Discord show one eye fitted to the window, the mod's own default (bf3vr.ini [stereo]
# stream_view, Home switches it and remembers); "streamview 0" (both eyes side by side) is the films' alone (the user, 2026-11-15).
WriteText (Join-Path $pkgBuild 'weapon.txt') (@('bareshare 0.5 0.8 1', 'windowfit 1', 'heldlog 1', 'scopeview 720', 'poseadapt 1',
                                                 'armby 2', 'posebydraw 1', 'nameby 2') -join "`n")
WriteText (Join-Path $pkgBuild 'pickups.txt') "level 2`n"
WriteText (Join-Path $pkgBuild 'recoil.txt') (@('gun SMAW 0', 'gun Stinger 0', 'gun Javelin 0', 'gun RPG-7 0') -join "`n")
# Holsters on (0.1.4), the file saying how to switch them off (the mod skips lines starting with #; no launcher line: every default, the
# launchers at the left shoulder, behind the neck when a gun of slot 1 has it).
WriteText (Join-Path $pkgBuild 'holsters.txt') (@('# Holsters: 1 on, 0 off (the weapon buttons switch guns either way).', 'holsters 1') -join "`n")
# The game's own graphics settings the mod was tuned with, which the launcher sets at every start (tools\gamesettings.ps1): from the commit's
# release\settings, whatever -SettingsFrom says (the tuned PC's values, its monitor's left out: every player's baseline, 2026-11-15).
CopyText (Join-Path $Worktree 'release\settings\gamesettings.txt') (Join-Path $pkgBuild 'gamesettings.txt')
# What the mod tuned and learned on the test setup: the left glove's line-up (grip.txt), the train pistol's slide and magazine by hand
# (pistol.txt), the route to the look angles (aimroute.txt: no search at the first load), the game's own left hand on the grenade
# launcher's grip (m320grip.bin). Each only when it's there.
foreach ($learned in 'grip.txt', 'pistol.txt', 'aimroute.txt') {
    $f = Join-Path $SettingsFrom $learned
    if (Test-Path $f) { CopyText $f (Join-Path $pkgBuild $learned) } else { Say "  ($learned isn't in ${SettingsFrom}: left out)" }
}
$grip = Join-Path $SettingsFrom 'm320grip.bin'
if (Test-Path $grip) { Copy-Item $grip $pkgBuild } else { Say "  (m320grip.bin isn't in ${SettingsFrom}: left out)" }
if ($WithGunShapes) {
    $guns = Join-Path $SettingsFrom 'guns'
    if (-not (Get-ChildItem (Join-Path $guns '*.bin') -ErrorAction SilentlyContinue)) { Fail "-WithGunShapes: no $guns\*.bin" }
    New-Item -ItemType Directory -Force (Join-Path $pkgBuild 'guns') | Out-Null
    Copy-Item (Join-Path $guns '*.bin') (Join-Path $pkgBuild 'guns')
}

WriteText (Join-Path $dist 'VERSION') (@(
    "$Version (commit $short, $date)",
    'Battlefield 3 VR, pre-alpha: the single-player campaign only, god mode on, holsters on. Expect rough edges.',
    "Built from commit $hash.") -join "`n")
# START HERE.txt: the steps from README.md, shorter, for the player who has just unzipped the package (plain ASCII, for any editor).
$startHere = @'
Battlefield 3 VR <VERSION>: a pre-alpha test build
A fan-made VR mod for the Battlefield 3 campaign on PC.

PRE-ALPHA, PLEASE READ
- God mode is ON by default, on purpose: enemy fire can't kill you, so you can try everything
  without replaying the same fight. End on the keyboard switches it off and on.
- Not for a normal play-through yet: one mission (Rock and a Hard Place) plays from start to end
  in VR, most others haven't been played in VR yet. Holsters are new in this build.
  Expect rough edges and the odd crash.
- The launcher sets the game's graphics settings to the ones the mod was tuned with, at every
  start (build\gamesettings.txt: change a value there to keep one of yours). Your monitor's
  resolution stays yours; your old settings file is kept once as PROF_SAVE_profile.before-bf3vr.

WHAT YOU NEED
- Battlefield 3 for PC (Steam or the EA app), started once the normal way first.
- Steam with SteamVR (SteamVR is free in Steam's Store).
- A headset that works with SteamVR: Quest 3 or Quest 2 (Steam Link, Virtual Desktop, or Quest
  Link / Air Link), Valve Index, Steam Frame, and others. Nothing else to install.

INSTALL (ONCE)
1. Before unzipping: right-click the zip, Properties, tick Unblock, OK.
2. Right-click the zip, Extract All, into a folder of your own (C:\Games, for example; outside
   Documents and the desktop is best). Keep the folder together.
3. If your antivirus removes build\injector.exe or build\bf3vr.dll: the mod loads itself into the
   running game, which some antivirus programs distrust (Windows Security calls the injector a
   trojan by a machine-learning guess: a false alarm). Let just this folder be: Windows Security,
   Virus & threat protection, Manage settings, Exclusions, Add an exclusion, Folder, this folder
   (Start BF3 VR.bat opens those settings for you when it finds the injector gone). Never switch
   the antivirus off, and avoid "Allow on device": it allows that whole detection name. Then unzip
   again into the same folder.

PLAY
1. Turn the headset on and connect it to SteamVR (a Quest: Steam Link, Virtual Desktop, or Quest
   Link / Air Link). Set it to 72 Hz (the Index: 80 Hz). With Quest Link or Air Link also click,
   in SteamVR's Settings, OpenXR: "Set SteamVR as OpenXR Runtime".
   Virtual Desktop players can instead try VDXR, Virtual Desktop's own runtime (no SteamVR;
   experimental): double-click "Start BF3 VR (VDXR).bat" in step 2.
2. Double-click "Start BF3 VR.bat". If Windows asks, click Run (or More info, Run anyway).
   Leave its window open. The first time it also makes the mod's sounds from your own copy of the
   game (about a minute, once).
3. In Battlelog (it opens in your browser): sign in if it asks, then click PLAY CAMPAIGN.
4. The first time only: in the game's menu click START NEW CAMPAIGN (or RESUME CAMPAIGN).
   From then on the launcher window clicks RESUME CAMPAIGN for you.
5. Put the headset on as the level starts: the game switches to VR by itself.
   Sit as you'll play, look ahead, and hold the right stick's click 1.5 s to recenter (or F9).

IF THE GAME DOESN'T START (or anything's wrong)
Double-click "Make a report.bat". It makes "BF3 VR report.txt" in this folder: everything about
your PC, your copy of the game (Steam's or the EA app's) and the last start, with your private
details taken out. Send that one file when you ask for help.

CONTROLS (they work while the game's window is in front on the PC)
  Triggers                fire the gun in that hand; on a charging handle, lever or slide:
                          grab it, pull back, let go
  Grip                    hold the gun (let go and it drops); the other grip at its front: two hands
  Grip at your belly      a fresh magazine: push it into the gun
  Grip near a gun on the ground, or a hand lowered towards it: it flies into your hand
  Holsters                main gun on your right shoulder, a second gun and the rocket launcher
                          on your left (the launcher behind your neck when a second gun is
                          there). Either hand:
                          squeeze the grip there to draw, open it there to put the gun away.
                          "holsters 0" in build\holsters.txt switches them off.
  Left stick              walk (click: sprint)
  Right stick             turn; flick down: crouch; flick down and hold: prone; flick up: stand
  Right stick's click     jump; held 1.5 s: recenter
  Magazine button         tap: drop the magazine; hold: the game's own reload, or "HOLD [R]"
                          (Quest: X; Index: left A; Steam Frame: X)
  Use (the game's E)      Quest: Y; Index: left B; Steam Frame: Y. Hold it 0.7 s: switch weapons
  Jump (the game's Space) A
  Keyboard                F8 VR view / flat screen, F9 recenter, End god mode off / on
  No aim button: bring the gun up to your eye and look through its sights.

CAMPAIGN ONLY: the mod refuses to load into multiplayer or co-op. Never use it online. Once in,
it stays in that game until it closes: close the game before you play online, and never join a
multiplayer or co-op game from Battlelog while the campaign started with the mod is open.

SOMETHING WRONG?
The launcher window says what to do. If it can't find Battlefield 3 by itself, it asks for the
game's folder (the one with bf3.exe in it): paste it and press Enter. If the game starts but VR
doesn't work, it freezes, or the headset stays black in a level: open build\launch.log. It says
step by step whether the VR launch worked (8 steps, the last a level in the headset), and what to
try for a step that failed. Its lines are also in build\bf3vr.log, starting with LAUNCH.
README.md has the full
controls, what to expect in each mission and a troubleshooting list. To report a problem, open
an issue at
https://github.com/criminalvictory/Battlefield-3-VR-The-Great-Destroyer/issues
and attach build\bf3vr.log and build\injector.log (copy them first: the next start replaces them).
'@
WriteText (Join-Path $dist 'START HERE.txt') $startHere.Replace('<VERSION>', $Version)

# The licences of the code built into the programs, which their terms ask to come with them: THIRD-PARTY-NOTICES.txt from the commit, and
# the OpenXR SDK's (its loader, with JsonCpp) and MinHook's own licence files from the sources the build used (the worktree's own fetched
# copies, else this repository's build\_deps, which a -Builder such as wtbuild.bat builds from).
$pkgLicenses = Join-Path $dist 'licenses'
New-Item -ItemType Directory -Force $pkgLicenses | Out-Null
CopyText (Join-Path $Worktree 'THIRD-PARTY-NOTICES.txt') (Join-Path $pkgLicenses 'THIRD-PARTY-NOTICES.txt')
foreach ($licence in @(@('openxr-src\LICENSE', 'OpenXR-SDK-LICENSE.txt'), @('openxr-src\src\external\jsoncpp\LICENSE', 'JsonCpp-LICENSE.txt'),
                       @('minhook-src\LICENSE.txt', 'MinHook-LICENSE.txt'))) {
    $from = @((Join-Path $wtBuild "_deps\$($licence[0])"), (Join-Path $root "build\_deps\$($licence[0])")) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $from) { Fail "the licence _deps\$($licence[0]) isn't in $wtBuild or $root\build: the programs can't go out without it" }
    CopyText $from (Join-Path $pkgLicenses $licence[1])
}

# 5. Nothing private: no folder of this PC (this repository's own folder's name included), no user's folder, no e-mail address, none of
# the words given with -Forbid, in any file of the package (the programs' bytes included).
$problems = @()
$words = @($Forbid | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })  # (from cmd: one word list "a,b")
# The checkout's own folder name, unless it's the repository's public name (on GitHub's servers the folder is named after the
# repository, which the README and START HERE name in their links on purpose).
$leaf = Split-Path $root -Leaf
if ($leaf -ne 'Battlefield-3-VR-The-Great-Destroyer') { $words += $leaf }
foreach ($f in Get-ChildItem $dist -Recurse -File) {
    $text = [Text.Encoding]::GetEncoding(28591).GetString([IO.File]::ReadAllBytes($f.FullName))
    $rel = $f.FullName.Substring($dist.Length + 1)
    if ($text -match '[A-Za-z]:\\Users\\') { $problems += "$rel names a user's folder" }
    if ($env:USERNAME.Length -ge 3 -and $text -match ('(?i)[\\/]' + [regex]::Escape($env:USERNAME) + '[\\/]')) { $problems += "$rel names this PC's user" }
    if ($text.IndexOf($root, [StringComparison]::OrdinalIgnoreCase) -ge 0 -or $text.IndexOf($Worktree, [StringComparison]::OrdinalIgnoreCase) -ge 0) { $problems += "$rel names this PC's folders" }
    foreach ($w in $words) { if ($text.IndexOf($w, [StringComparison]::OrdinalIgnoreCase) -ge 0) { $problems += "$rel holds '$w'" } }
    if ($f.Extension -notin '.dll', '.exe', '.bin' -and $text -match '[\w.+-]+@[\w-]+\.[\w.]+') { $problems += "$rel holds an e-mail address" }
}
if ($problems) { Fail ("private or unwanted text in the package:`n  " + ($problems -join "`n  ")) }

# 6. The zip, its top folder the package's, each entry's folders parted by "/" as the zip format has them (Windows PowerShell's own
# zipping writes "\", which other unzippers take for part of a file's name).
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$stream = [IO.File]::Open($zip, [IO.FileMode]::CreateNew)
$archive = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create)
$zipProblems = @()
try {
    foreach ($f in Get-ChildItem $dist -Recurse -File | Sort-Object FullName) {
        $entry = $name + '/' + ($f.FullName.Substring($dist.Length + 1) -replace '\\', '/')
        try {
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $f.FullName, $entry, [IO.Compression.CompressionLevel]::Optimal)
        } catch {
            $zipProblems += "$entry couldn't be read into the zip: $($_.Exception.Message)"
        }
    }
} finally {
    $archive.Dispose()
    $stream.Dispose()
}
# Every program the player needs must really be in the zip (2026-11-14: Windows Security blocked injector.exe while it was packed,
# and a zip without it came out as if all was well).
$zipRead = [IO.Compression.ZipFile]::OpenRead($zip)
try { $inZip = @($zipRead.Entries | ForEach-Object { $_.FullName }) } finally { $zipRead.Dispose() }
foreach ($must in 'build/injector.exe', 'build/bf3vr.dll', 'build/gamesounds.exe', 'build/gunshape.exe', 'build/gamesettings.txt', 'Start BF3 VR.bat', 'Start BF3 VR (VDXR).bat', 'Make a report.bat', 'tools/report.ps1', 'tools/launch_vr.ps1', 'tools/gamesettings.ps1') {
    if ($inZip -notcontains "$name/$must") { $zipProblems += "$must isn't in the zip" }
}
if ($zipProblems) {
    Remove-Item -LiteralPath $zip -ErrorAction SilentlyContinue
    Fail ("the zip came out incomplete (an antivirus blocking a file?), so it was deleted:`n  " + ($zipProblems -join "`n  "))
}

Say ''
Say "Packaged $short as $dist"
foreach ($f in Get-ChildItem $dist -Recurse -File | Sort-Object FullName) { Say ('  {0,-34} {1,10:N0} bytes' -f $f.FullName.Substring($dist.Length + 1), $f.Length) }
foreach ($k in $dependents.Keys) { Say ("  {0} needs: {1}" -f $k, ($dependents[$k] -join ', ')) }
Say ('{0}: {1:N0} bytes' -f $zip, (Get-Item $zip).Length)

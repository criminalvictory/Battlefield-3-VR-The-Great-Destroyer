# Makes the release a player downloads: a clean build of one commit, laid out as tools\launch_vr.ps1 expects, with the settings the mod is
# played with (god mode on, holsters off: pre-alpha), zipped. Nothing of the game's own (no sounds: the launcher makes them on the player's
# PC from their own copy of the game), no logs, no dumps, nothing private.
#   tools\package.ps1 [-Commit HEAD] [-Version 0.1.0-prealpha] [-Worktree <folder>] [-Builder <bat>] [-SettingsFrom <folder>] [-Out <folder>]
#                     [-Forbid <word>,<word>] [-WithGunShapes] [-NoBuild]
# -Worktree: a git worktree of this repository the commit is checked out in and built in (made when it isn't there; it must hold no
#   changes of its own). -Builder: a batch file that builds the worktree given as its first argument into its build\ folder (32-bit
#   Release); without one, this script builds it as build.bat does, with this repository's own fetched OpenXR SDK and MinHook when
#   build\_deps has them. -SettingsFrom: the folder the played settings and what the mod learned are read from (bf3vr.ini, grip.txt,
#   pistol.txt, aimroute.txt, m320grip.bin; default build\). -WithGunShapes: the guns' collision shapes too (build\guns\*.bin, about
#   70 MB; made from the game's meshes: left out by default). -NoBuild: package the worktree's build as it is (it must be of that commit).
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
    [switch]$NoBuild
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
                  "cmake -S `"$Worktree`" -B `"$wtBuild`" -G Ninja -DCMAKE_BUILD_TYPE=Release $fetched && cmake --build `"$wtBuild`"")
        if ($LASTEXITCODE -ne 0) { Fail "the build failed (exit $LASTEXITCODE)" }
    }
    foreach ($name in 'bf3vr.dll', 'injector.exe', 'gamesounds.exe') {
        $f = Join-Path $wtBuild $name
        if (-not (Test-Path $f)) { Fail "the build made no $name" }
        if ((Get-Item $f).LastWriteTime -lt $started.AddSeconds(-5) -and $Builder) { Say "  ($name was already up to date in the worktree's build)" }
    }
}
$binaries = foreach ($name in 'bf3vr.dll', 'injector.exe', 'gamesounds.exe') {
    $f = Join-Path $wtBuild $name
    if (-not (Test-Path $f)) { Fail "$f isn't there (build it, or leave out -NoBuild)" }
    $f
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
foreach ($doc in 'README.md', 'CHANGELOG.md') { if (Test-Path (Join-Path $Worktree $doc)) { CopyText (Join-Path $Worktree $doc) (Join-Path $dist $doc) } }
foreach ($script in 'launch_vr.ps1', 'resume.ps1', 'drawhunt.ps1') { CopyText (Join-Path $Worktree "tools\$script") (Join-Path $pkgTools $script) }

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
WriteText (Join-Path $pkgBuild 'weapon.txt') (@('bareshare 0.5 0.8 1', 'windowfit 1', 'streamview 0', 'heldlog 1', 'scopeview 720', 'poseadapt 1',
                                                 'armby 2', 'posebydraw 1', 'nameby 2') -join "`n")
WriteText (Join-Path $pkgBuild 'pickups.txt') "level 2`n"
WriteText (Join-Path $pkgBuild 'recoil.txt') (@('gun SMAW 0', 'gun Stinger 0', 'gun Javelin 0', 'gun RPG-7 0') -join "`n")
WriteText (Join-Path $pkgBuild 'holsters.txt') "holsters 0`n"
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
    'Battlefield 3 VR, pre-alpha: the single-player campaign only, god mode on, holsters off. Expect rough edges.',
    "Built from commit $hash.") -join "`n")
WriteText (Join-Path $dist 'READ ME FIRST.txt') (@(
    "Battlefield 3 VR $Version - pre-alpha",
    '',
    'What you need: Battlefield 3 on PC (Steam or the EA app) with its campaign, Steam with SteamVR, and a headset that runs',
    'through SteamVR (Quest 3 / Quest 2 with Steam Link, Virtual Desktop in SteamVR mode or Quest Link / Air Link; Valve Index;',
    'Steam Frame; others). Nothing else to install.',
    '',
    'To play:',
    '  1. Unzip this whole folder anywhere (right-click the zip, Extract All).',
    '  2. Turn the headset on and connect it to SteamVR.',
    '  3. Double-click "Start BF3 VR.bat". It starts SteamVR and the game; in Battlelog click PLAY CAMPAIGN.',
    '     The first start also makes the mod''s sounds from your own copy of the game (about a minute, once).',
    '  4. First time: in the game''s menu click START NEW CAMPAIGN (or RESUME CAMPAIGN if you have played before).',
    '     From then on the launcher resumes the campaign for you. The mod switches to the headset in the level.',
    '',
    'Pre-alpha: god mode is on (you can''t die; the End key switches it), holsters are off, and only some missions have been',
    'played in VR so far. The campaign only: the mod refuses to load into multiplayer or co-op. Never use it online.',
    '',
    'README.md has the controls, the settings and what to do when something goes wrong.') -join "`n")

# 5. Nothing private: no folder of this PC (this repository's own folder's name included), no user's folder, no e-mail address, none of
# the words given with -Forbid, in any file of the package (the programs' bytes included).
$problems = @()
$words = @($Forbid | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ }) + @(Split-Path $root -Leaf)  # (from cmd: one word list "a,b")
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
try {
    foreach ($f in Get-ChildItem $dist -Recurse -File | Sort-Object FullName) {
        $entry = $name + '/' + ($f.FullName.Substring($dist.Length + 1) -replace '\\', '/')
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $f.FullName, $entry, [IO.Compression.CompressionLevel]::Optimal)
    }
} finally {
    $archive.Dispose()
    $stream.Dispose()
}

Say ''
Say "Packaged $short as $dist"
foreach ($f in Get-ChildItem $dist -Recurse -File | Sort-Object FullName) { Say ('  {0,-34} {1,10:N0} bytes' -f $f.FullName.Substring($dist.Length + 1), $f.Length) }
foreach ($k in $dependents.Keys) { Say ("  {0} needs: {1}" -f $k, ($dependents[$k] -join ', ')) }
Say ('{0}: {1:N0} bytes' -f $zip, (Get-Item $zip).Length)

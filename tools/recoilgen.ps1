# Writes src/recoilguns.h: every campaign weapon's recoil, one row a blueprint (design 6, 10.2).
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\recoilgen.ps1 [-Out <file>] [-Check]
# The game's own numbers come from tools/recoil/guns.tsv, stances.tsv and unlocks.tsv (read from the install's data by the census tools: patch
# over base); tools/recoil/kick.tsv holds what the data doesn't say (the kind, the draw it's known by, the kick in the hands), each line with its
# source. Run again on the same inputs, it writes the same bytes. It stops, writing nothing, on anything it can't place: a blueprint missing from
# either side, two rows with one name, two blueprints with one signature (but the Stinger and the Javelin), a number that doesn't read.
# -Check writes nothing: it says whether the header is what the tables make now, its line ends aside (exit 0), or not (exit 1).
param([string]$Out = '', [switch]$Check)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2

$dir = Join-Path $PSScriptRoot 'recoil'
if (-not $Out) { $Out = Join-Path (Split-Path -Parent $PSScriptRoot) 'src\recoilguns.h' }
$inv = [Globalization.CultureInfo]::InvariantCulture

# ---- Reading ----

# A tab-separated table: its header's names for each line's cells (lines starting # and blank ones skipped).
function Read-Table([string]$name) {
    $lines = @([IO.File]::ReadAllLines((Join-Path $dir $name)) | ForEach-Object { $_.TrimEnd("`r") } | Where-Object { $_ -ne '' -and -not $_.StartsWith('#') })
    $head = $lines[0].Split("`t")
    $rows = New-Object System.Collections.ArrayList
    for ($i = 1; $i -lt $lines.Count; $i++) {
        $cells = $lines[$i].Split("`t")
        if ($cells.Count -ne $head.Count) { throw "$name line $($i + 1): $($cells.Count) cells, the header has $($head.Count)" }
        $row = @{}
        for ($c = 0; $c -lt $head.Count; $c++) { $row[$head[$c]] = $cells[$c] }
        [void]$rows.Add($row)
    }
    return ,$rows
}

# A number as the tables write it ('' and '-' read as $null).
function Num([string]$s, [string]$what) {
    if ($s -eq '' -or $s -eq '-') { return $null }
    $v = 0.0
    if (-not [double]::TryParse($s, [Globalization.NumberStyles]::Float, $inv, [ref]$v)) { throw "${what}: '$s' isn't a number" }
    return $v
}
function NumOr([string]$s, [double]$otherwise, [string]$what) {
    $v = Num $s $what
    if ($null -eq $v) { return $otherwise }
    return $v
}

# ---- Writing C++ ----

function Cf([double]$v) {   # a float literal: 0.26f, -0.3f, 750.0f
    if ([double]::IsNaN($v) -or [double]::IsInfinity($v)) { throw "a number that isn't finite" }
    $s = $v.ToString('0.0#####', $inv)
    if ($s -eq '-0.0') { $s = '0.0' }
    return $s + 'f'
}
function Ci([double]$v) { return ([long][math]::Round($v)).ToString($inv) }
function Cs([string]$s) { return '"' + $s.Replace('\', '\\').Replace('"', '\"') + '"' }
function Round2([double]$v) { return [math]::Round($v, 2, [MidpointRounding]::AwayFromZero) }
function Round1([double]$v) { return [math]::Round($v, 1, [MidpointRounding]::AwayFromZero) }
function Show([double]$v) { return $v.ToString('0.0##', $inv) }

# A name as ByName compares it: its letters and digits, lower case.
function NameKey([string]$s) { return ($s.ToLowerInvariant() -replace '[^a-z0-9]', '') }

# ---- The models the kick's sizes are worked out with (recoil.h's) ----

$kShareMean = 0.95          # each round's share of its up: uniform in [0.9, 1.0] (recoil.h kShareLeast)
$kDriftDampingLeast = 0.35  # the drift's camera spring is never drawn lighter (recoil.h)
$kHeldStepShare = 0.33      # held fire's push back: each later round of a string adds this share of one round's back (recoil.h)
$kHeldHome = 0.08           # ... its spring, critically damped, within 5% of where it's going in this many seconds (recoil.h)
$kStringGap = 1.25          # a string goes on while the next round comes within this many of the gun's gaps (recoil.h)
$kHeldShareDefault = 1.8    # held fire's mean push back against one round's back where its row gives none (recoil.h Kick::heldShare)
$kAnchorPhysics = @(1.0, 1.2)   # the M416's own two-hand physics as carried (design 6, 4.3: cm, degrees): what the anchor's kick stands for

# The drift's camera spring's answer to a step, at t seconds after it (0 to about 1.3): x'' = k (1 - x) - 2 zeta sqrt(k) x'.
function StepShare([double]$t, [double]$k, [double]$z) {
    if ($t -le 0) { return 0.0 }
    if ($k -le 0) { $k = 250.0; $z = 1.0 }   # none (the AT4): drawn through 250 at damping 1
    if ($z -lt $kDriftDampingLeast) { $z = $kDriftDampingLeast }
    $w = [math]::Sqrt($k)
    if ($z -ge 1) { return 1 - [math]::Exp(-$w * $t) * (1 + $w * $t) }
    $s = [math]::Sqrt(1 - $z * $z)
    return 1 - [math]::Exp(-$z * $w * $t) * ([math]::Cos($w * $s * $t) + $z / $s * [math]::Sin($w * $s * $t))
}

# A struck spring (recoil.h's Omega and Gain): where it is t seconds after the strike over its peak, which comes `at` seconds after.
function Struck([double]$t, [double]$at, [double]$z) {
    if ($t -le 0) { return 0.0 }
    $s = [math]::Sqrt(1 - $z * $z)
    $w = [math]::Atan2($s, $z) / ($s * $at)
    return ([math]::Exp(-$z * $w * $t) * [math]::Sin($w * $s * $t)) / ([math]::Exp(-$z * $w * $at) * [math]::Sin($w * $s * $at))
}

# A struck spring's own constants (recoil.h's Omega and Gain).
function Omega([double]$at, [double]$z) { $s = [math]::Sqrt(1 - $z * $z); return [math]::Atan2($s, $z) / ($s * $at) }
function Gain([double]$z) { $s = [math]::Sqrt(1 - $z * $z); return [math]::Exp(-($z / $s) * [math]::Atan2($s, $z)) }

# Held fire's push back after a string's second round, over its step (recoil.h: its target a step up at the round, its spring critically damped,
# the target back to 0 once the string has ended kStringGap gaps after that round): t seconds after the round.
function HeldShape([double]$t, [double]$gapRate) {
    if ($t -le 0) { return 0.0 }
    $w = 4.74 / $kHeldHome
    $end = $kStringGap * $gapRate
    if ($t -le $end) { return 1 - [math]::Exp(-$w * $t) * (1 + $w * $t) }
    $x0 = 1 - [math]::Exp(-$w * $end) * (1 + $w * $end); $v0 = $w * $w * $end * [math]::Exp(-$w * $end)
    $u = $t - $end
    return ($x0 + ($v0 + $w * $x0) * $u) * [math]::Exp(-$w * $u)
}

# The kick's size that puts the drawn peak, the kick and the drift together as recoil.h draws them, at the recording's: the kick's shape and the
# drift's on one grid (0.1 ms steps over 0.5 s), the size by Newton's steps on the peak of size x kick + drift (the kick peaks at its class's time,
# not the recording's, so the drift's share is the one where the sum peaks). Returns the size and the drift's part where it peaks.
function SizeFor([double]$rec, [double[]]$kick, [double[]]$drift, [string]$what) {
    $n = $kick.Count
    $size = $rec
    $at = 0
    for ($step = 0; $step -lt 40; $step++) {
        $most = -1e9; $at = 0
        for ($i = 0; $i -lt $n; $i++) { $x = $size * $kick[$i] + $drift[$i]; if ($x -gt $most) { $most = $x; $at = $i } }
        if ([math]::Abs($most - $rec) -lt 1e-7) { break }
        if ($kick[$at] -le 1e-6) { throw "${what}: its drift alone peaks past the recording" }
        $size += ($rec - $most) / $kick[$at]
    }
    if ([math]::Abs($most - $rec) -gt 1e-5 -or $size -le 0) { throw "${what}: no kick puts its peak at the recording's" }
    return @($size, $drift[$at])
}
$kGridSteps = 5000   # the grid SizeFor's shapes are on: 0.1 ms steps over 0.5 s, from 0.1 ms after the first round

# ---- The classes' timings (design 6, 3.3): back's and the rise's time to their peak (ms) and damping ratio ----

$timings = @{
    pistol   = @(25, 0.8, 35, 0.8)
    auto     = @(30, 0.75, 50, 0.75)
    heavy    = @(35, 0.75, 60, 0.7)
    m320     = @(60, 0.9, 60, 0.9)
    launcher = @(40, 0.75, 80, 0.7)
    at4      = @(110, 0.75, 80, 0.7)   # its own launch, recorded on the roof (build\designs\at4_measured.md 5.4): the back peaking at 110 ms
    none     = @(30, 0.75, 50, 0.75)
}
$liftTiming = @(100, 0.7)

$kinds = @{
    pistol = 'kPistol'; smg = 'kSmg'; rifle = 'kRifle'; mg = 'kMg'; shotgun = 'kShotgun'; marksman = 'kMarksman'; bolt = 'kBolt'
    m320 = 'kM320'; rocket = 'kRocket'; missile = 'kMissile'; at4 = 'kAt4'; none = 'kNone'
}
$kindOrder = @('none', 'pistol', 'smg', 'rifle', 'mg', 'shotgun', 'marksman', 'bolt', 'm320', 'rocket', 'missile', 'at4')

# ---- The sights (design 6, 4.4): which of eye[8] an unlock's ZoomedKickbackFactor goes in ----

$eyeDefault = @(1.0, 1.0, 1.0, 0.75, 0.75, 0.5, 0.75, 0.5)   # irons (the gun's own), Kobra, PK-AS, PK-A, PSO-1, PKS-07, ACOG, another 6x+
function EyeColumn([string]$unlock) {   # -1 none; 7 a campaign scope; 70 a 6x rifle scope; 71 a ballistic scope
    $u = $unlock.ToLowerInvariant()
    if ($u -match '_sp_(villa|sniper)') { return 7 }
    if ($u -match 'kobra') { return 1 }
    if ($u -match 'pk-as') { return 2 }
    if ($u -match 'pka') { return 3 }
    if ($u -match 'pso-0?1') { return 4 }
    if ($u -match 'pks-07') { return 5 }
    if ($u -match 'acog') { return 6 }
    if ($u -match 'rifle_?scope') { return 70 }
    if ($u -match 'ballistic_?scope') { return 71 }
    return -1
}

# ---- The tables ----

$guns = Read-Table 'guns.tsv'
$stances = Read-Table 'stances.tsv'
$unlocks = Read-Table 'unlocks.tsv'
$kick = Read-Table 'kick.tsv'

$gunOf = @{}
foreach ($g in $guns) { if ($gunOf.ContainsKey($g.blueprint)) { throw "guns.tsv has $($g.blueprint) twice" }; $gunOf[$g.blueprint] = $g }
$stanceOf = @{}
foreach ($s in $stances) { $stanceOf["$($s.blueprint)|$($s.stance)|$($s.zoom)"] = $s }
$unlocksOf = @{}
foreach ($u in $unlocks) {
    if (-not $unlocksOf.ContainsKey($u.blueprint)) { $unlocksOf[$u.blueprint] = New-Object System.Collections.ArrayList }
    [void]$unlocksOf[$u.blueprint].Add($u)
}

$seen = @{}
foreach ($k in $kick) {
    if (-not $gunOf.ContainsKey($k.blueprint)) { throw "kick.tsv's $($k.name): $($k.blueprint) isn't in guns.tsv" }
    if ($seen.ContainsKey($k.blueprint)) { throw "kick.tsv has $($k.blueprint) twice" }
    $seen[$k.blueprint] = $true
    if (-not $kinds.ContainsKey($k.kind)) { throw "kick.tsv's $($k.name): no kind '$($k.kind)'" }
    if (-not $timings.ContainsKey($k.timing)) { throw "kick.tsv's $($k.name): no timing '$($k.timing)'" }
}
foreach ($g in $guns) { if (-not $seen.ContainsKey($g.blueprint)) { throw "guns.tsv's $($g.blueprint) has no line in kick.tsv" } }

$anchors = @($kick | Where-Object { $_.size -eq 'anchor' })
if ($anchors.Count -ne 1) { throw "kick.tsv needs one anchor line, has $($anchors.Count)" }
$anchorBack = Num $anchors[0].back_cm 'anchor back'
$anchorRise = Num $anchors[0].rise_deg 'anchor rise'

# ---- Each row ----

$rows = New-Object System.Collections.ArrayList
foreach ($k in $kick) {
    $g = $gunOf[$k.blueprint]
    $what = $k.name
    $hasSway = $g.sway_data -ne '' -and $g.sway_data -ne 'null'
    if ($hasSway -ne ($k.kind -ne 'none')) { throw "${what}: kind '$($k.kind)' but its gun sway is '$($g.sway_data)'" }
    $r = @{ k = $k; g = $g; name = $k.name }

    # The game's numbers.
    $r.rate = NumOr $g.rof 0 "$what rate"
    $r.speed = NumOr $g.speed 0 "$what speed"
    $r.magazine = NumOr $g.mag 0 "$what magazine"
    $r.pellets = NumOr $g.bullets_per_shell 0 "$what pellets"
    $r.rumble = @((NumOr $g.rumble_low 0 "$what rumble"), (NumOr $g.rumble_high 0 "$what rumble"), (NumOr $g.rumble_dur 0 "$what rumble"))
    $r.first = NumOr $g.first_shot_mult 0 "$what first shot"
    $r.scale = NumOr $g.shooting_dec_scale 0 "$what firing scale"
    if ($g.cam_spring -match '^none') { $r.spring = 0.0; $r.damping = 0.0 }   # no CameraRecoilData (the AT4)
    else { $r.spring = NumOr $g.cam_spring 0 "$what spring"; $r.damping = NumOr $g.cam_damping 0 "$what damping" }
    $r.up = @(); $r.sideMin = @(); $r.sideMax = @(); $r.decrease = @(); $r.capUp = 0.0; $r.capSide = 0.0
    foreach ($stance in @('Stand', 'Crouch', 'Prone')) {
        $u = @(); $a = @(); $b = @(); $d = @()
        foreach ($zoom in @('NoZoom', 'Zoom')) {
            if ($hasSway) {
                $s = $stanceOf["$($k.blueprint)|$stance|$zoom"]
                if ($null -eq $s) { throw "${what}: stances.tsv has no $stance $zoom row" }
                $u += Num $s.rec_inc "$what up"; $a += Num $s.horiz_min "$what side"; $b += Num $s.horiz_max "$what side"; $d += Num $s.rec_dec "$what decrease"
                $cu = Num $s.rec_max "$what cap"; $cs = Num $s.horiz_max_amp "$what cap"
                if ($stance -eq 'Stand' -and $zoom -eq 'NoZoom') { $r.capUp = $cu; $r.capSide = $cs }
                elseif ($cu -ne $r.capUp -or $cs -ne $r.capSide) { throw "${what}: its caps differ between its rows (one pair is all Row keeps)" }
            } else { $u += 0.0; $a += 0.0; $b += 0.0; $d += 0.0 }
        }
        $r.up += ,$u; $r.sideMin += ,$a; $r.sideMax += ,$b; $r.decrease += ,$d
    }
    $r.sig = @($r.up[0][0], $r.sideMin[0][0], $r.sideMax[0][0], $r.decrease[0][0], $r.first, $r.scale, $r.spring, $r.damping)

    # The sights' factors.
    $base = NumOr $g.kickback_zoomed 1 "$what kick factor"
    $found = @{}
    if ($hasSway -and $unlocksOf.ContainsKey($k.blueprint)) {
        foreach ($u in $unlocksOf[$k.blueprint]) {
            $f = Num $u.kickback_zoomed "$what $($u.unlock)"
            if ($null -eq $f) { continue }
            $col = EyeColumn $u.unlock
            if ($col -ge 0 -and -not $found.ContainsKey($col)) { $found[$col] = $f }
        }
    }
    if ($found.Count -eq 0) { $r.eye = @($base, $base, $base, $base, $base, $base, $base, $base) }   # no magnified sight on it: its own base
    else {
        $r.eye = @($base) + @(1..6 | ForEach-Object { if ($found.ContainsKey($_)) { $found[$_] } else { $eyeDefault[$_] } })
        $six = $eyeDefault[7]
        foreach ($col in @(7, 70, 71)) { if ($found.ContainsKey($col)) { $six = $found[$col]; break } }
        $r.eye += $six
    }

    # The kick in the hands.
    $t = $timings[$k.timing]
    $r.backAt = $t[0] / 1000.0; $r.backZ = $t[1]; $r.riseAt = $t[2] / 1000.0; $r.riseZ = $t[3]
    $backCm = Num $k.back_cm "$what back"; $rise = Num $k.rise_deg "$what rise"; $liftCm = NumOr $k.lift_cm 0 "$what lift"
    switch ($k.size) {
        'anchor' { $r.why = "the anchor: $(Show $backCm) cm, $(Show $rise) deg" }
        'given' { $r.why = "given, a placeholder: $(Show $backCm) cm, $(Show $rise) deg" }
        'measured' { $r.why = "measured: $(Show $backCm) cm, $(Show $rise) deg" }
        'recorded' {
            # The back as recorded (one round's own peak); the rise that puts the drawn peak, the kick and the drift's answer to the round together,
            # at the recording's.
            $recMs = Num $k.rise_ms "$what rise ms"
            $u0 = $r.up[0][0] * $kShareMean * $r.first
            $kickShape = New-Object double[] $kGridSteps; $driftShape = New-Object double[] $kGridSteps
            for ($i = 0; $i -lt $kGridSteps; $i++) {
                $tt = ($i + 1) * 0.0001
                $kickShape[$i] = Struck $tt $r.riseAt $r.riseZ
                $driftShape[$i] = $u0 * (StepShare $tt $r.spring $r.damping)
            }
            $rec = $rise
            $sized = SizeFor $rec $kickShape $driftShape $what
            $rise = Round2 $sized[0]
            $r.why = "recorded: $(Show $backCm) cm; $(Show $rec) deg (at $(Ci $recMs) ms) drawn with the drift's $(Show (Round2 $sized[1])) where they peak, $(Show $rise)"
        }
        'pair' {
            # A burst pair gap_ms apart, burst_first of a round's kick at once and the rest with the second round. The back: held fire's push back
            # from that second round (the string's, its row's own share) in with it; the rise: the drift's answer to both rounds in with it; each
            # drawn peak at the recording's.
            $gap = (Num $k.gap_ms "$what gap") / 1000.0
            $first = Num $k.burst_first "$what burst share"
            $gapRate = 60.0 / $r.rate
            $heldCm = Num $k.held_cm "$what held"
            if ($null -ne $heldCm) { throw "${what}: a recorded pair with a held-fire loop isn't sized yet" }
            $springMean = 1 / ((Gain $r.backZ) * (Omega $r.backAt $r.backZ) * $gapRate)   # (the back spring's own mean at its rate, a share of a round)
            $heldStep = [math]::Min($kHeldStepShare, [math]::Max($kHeldShareDefault - $springMean, 0))
            $u0 = $r.up[0][0] * $kShareMean
            $backShape = New-Object double[] $kGridSteps; $kickShape = New-Object double[] $kGridSteps; $driftShape = New-Object double[] $kGridSteps
            $zeros = New-Object double[] $kGridSteps
            for ($i = 0; $i -lt $kGridSteps; $i++) {
                $tt = ($i + 1) * 0.0001
                $backShape[$i] = $first * (Struck $tt $r.backAt $r.backZ) + (2 - $first) * (Struck ($tt - $gap) $r.backAt $r.backZ) + $heldStep * (HeldShape ($tt - $gap) $gapRate)
                $kickShape[$i] = $first * (Struck $tt $r.riseAt $r.riseZ) + (2 - $first) * (Struck ($tt - $gap) $r.riseAt $r.riseZ)
                $driftShape[$i] = $u0 * $r.first * (StepShare $tt $r.spring $r.damping) + $u0 * (StepShare ($tt - $gap) $r.spring $r.damping)
            }
            $recBack = $backCm; $rec = $rise
            $backCm = Round2 (SizeFor $recBack $backShape $zeros "$what back")[0]
            $sized = SizeFor $rec $kickShape $driftShape $what
            $rise = Round2 $sized[0]
            $r.why = "a recorded pair $(Ci ($gap * 1000)) ms apart: $(Show $recBack) cm with held fire's push back in it, $(Show $backCm) a round; " +
                     "$(Show $rec) deg with the drift's $(Show (Round2 $sized[1])) at the peak, $(Show $rise) a round"
        }
        'physics' {
            $pBack = $backCm; $pRise = $rise
            $backCm = Round1 ($anchorBack * [math]::Sqrt($pBack / $kAnchorPhysics[0]))
            $rise = Round1 ($anchorRise * [math]::Sqrt($pRise / $kAnchorPhysics[1]))
            $r.why = "physics, a placeholder: $(Show $anchorBack) sqrt($(Show $pBack)) = $(Show $backCm) cm, " +
                     "$(Show $anchorRise) sqrt($(Show $pRise) / $(Show $kAnchorPhysics[1])) = $(Show $rise) deg"
        }
        'none' { $backCm = 0.0; $rise = 0.0; $liftCm = 0.0; $r.why = 'no gun sway: no recoil' }
        default { throw "${what}: no size rule '$($k.size)'" }
    }
    if ($liftCm -gt 0) { $r.why += ", lift $(Show $liftCm) cm" }
    $r.back = $backCm / 100.0; $r.rise = $rise; $r.lift = $liftCm / 100.0
    $r.roll = Num $k.roll "$what roll"; $r.yaw = Num $k.yaw "$what yaw"; $r.jitter = Num $k.jitter "$what jitter"
    $r.burstFirst = Num $k.burst_first "$what burst share"
    $r.pairGap = (Num $k.pair_ms "$what pair") / 1000.0
    # Held fire's push back: none where the game fires one round at a time; a recorded loop's mean over this round's back; else recoil.h's own.
    $single = $g.fire_logic -notmatch 'Automatic|Burst'
    $held = Num $k.held_cm "$what held"
    $r.held = $null
    if ($k.kind -eq 'none' -or $single) {
        if ($null -ne $held) { throw "${what}: a held-fire push back, but it fires one round at a time" }
        $r.held = 0.0
    } elseif ($null -ne $held) {
        $r.held = Round2 ($held / $backCm)
        $r.why += "; held fire $(Show $held) cm over $(Show $backCm), $(Show $r.held)"
    }
    $r.body = NumOr $k.body 0 "$what body"
    $r.shape = $k.shape
    $r.default = $k.default -eq 'yes'
    [void]$rows.Add($r)
}

# ---- Checks ----

$names = @{}
for ($i = 0; $i -lt $rows.Count; $i++) {
    $r = $rows[$i]
    $bp = $r.k.blueprint
    $keys = @((NameKey $r.name), (NameKey $bp), (NameKey ($bp.Substring($bp.LastIndexOf('/') + 1))))
    if ($r.shape -ne '-') { $keys += NameKey $r.shape }
    foreach ($key in ($keys | Select-Object -Unique)) {
        if ($key -eq '') { throw "$($r.name): a name with no letters" }
        if ($names.ContainsKey($key) -and $names[$key] -ne $i) { throw "'$key' names both $($rows[$names[$key]].name) and $($r.name)" }
        $names[$key] = $i
    }
}
$bodies = @{}
foreach ($r in $rows) {
    if ($r.body -ne 0) {
        if ($bodies.ContainsKey($r.body)) { throw "body $($r.body) is both $($bodies[$r.body]) and $($r.name)" }
        $bodies[$r.body] = $r.name
    }
}
$signatures = @{}
foreach ($r in $rows) {
    if ($r.k.kind -eq 'none') { continue }
    $key = (@($r.rate, $r.speed, $r.magazine, $r.pellets) + $r.sig | ForEach-Object { $_.ToString('R', $inv) }) -join ' '
    if ($signatures.ContainsKey($key)) {
        $pair = "$($signatures[$key])+$($r.name)"
        if ($pair -ne 'Stinger+Javelin') { throw "$($signatures[$key]) and $($r.name) have one signature" }
    } else { $signatures[$key] = $r.name }
}
$defaults = @{}
for ($i = 0; $i -lt $rows.Count; $i++) {
    if ($rows[$i].default) {
        $kind = $rows[$i].k.kind
        if ($defaults.ContainsKey($kind)) { throw "two default rows for $kind" }
        $defaults[$kind] = $i
    }
}
foreach ($kind in $kindOrder) { if (-not $defaults.ContainsKey($kind)) { throw "no default row for $kind" } }
$m416 = $rows | Where-Object { $_.name -eq 'M416' }
$want = @(0.26, -0.3, 0.1, 18, 2, 0.1, 250, 0.35)
for ($i = 0; $i -lt 8; $i++) { if ([math]::Abs($m416.sig[$i] - $want[$i]) -gt 1e-9) { throw "the M416's sig[$i] reads $($m416.sig[$i]), not $($want[$i])" } }
if ($m416.rate -ne 750 -or $m416.back -ne 0.015 -or $m416.rise -ne 1.8) { throw "the M416's rate or kick moved" }

# ---- The header ----

$nl = "`n"
$o = New-Object System.Text.StringBuilder
function Line([string]$s) { [void]$o.Append($s).Append($nl) }
function Pair2($v) { return '{' + (Cf $v[0]) + ', ' + (Cf $v[1]) + '}' }
function Rows3($v) { return '{' + (Pair2 $v[0]) + ', ' + (Pair2 $v[1]) + ', ' + (Pair2 $v[2]) + '}' }

Line '#pragma once'
Line "// Every campaign weapon's recoil, from the game's own data (tools/recoil/*.tsv; written by tools/recoilgen.ps1, never by hand). The gun in hand"
Line "// reads its own numbers live (recoilrun.cpp); this is how it's known, how big its kick in the hands is, and its numbers when it has no gun sway."
Line "//   - The game's own (guns.tsv, stances.tsv, unlocks.tsv: the install's data, patch over base): the rate, the muzzle speed, the magazine, the"
Line "//     pellets, each stance's and zoom's climb a round (up, sideways, how fast it comes back) and caps, the first shot's multiplier, the scale"
Line "//     while firing, the camera spring, the rumble, and each sight's ZoomedKickbackFactor."
Line "//   - kick.tsv's, each line with its source: the kind, the draw and the shape it's known by, and the kick in the hands, one round from rest with"
Line "//     two hands at the hip. A recorded gun's from its own recording of the game's kick: the size whose drawn peak, the drift's answer to the"
Line "//     round with it, is the recording's; the M416's the anchor; a gun not recorded is a placeholder, its physics' square root about the M416's"
Line "//     (the pistols' and the launchers' the design's own); a measured one's as measured (the AT4's on the roof: its back's own timing too)."
Line "//     Its timings are its class's, its roll, sideways share and jitter the two-hand hold's. Held fire's push back: none for a gun the game fires a"
Line "//     round at a time, a recorded loop's own, else recoil.h's."
Line "// One row a blueprint, 58: the 40 guns, the launchers, the unused (the A-91, the Glock 18, the .44, the enemies' RPG-7) and the 9 with no gun"
Line "// sway (kind kNone: no recoil). The M320's row is also the one under the M416, whose 40 mm is the same data. The Stinger and the Javelin share"
Line "// theirs: the Stinger, which is drawn, comes first."
Line ''
Line '#include <cstdint>'
Line '#include <cstring>'
Line ''
Line '#include "recoil.h"'
Line ''
Line 'namespace recoilguns {'
Line ''
Line 'struct Row {'
Line '    const char* name;       // "M416": recoil.txt''s "gun <name>", the logs'
Line '    const char* blueprint;  // "weapons/m416/m416"'
Line '    const char* shape;      // skin.cpp''s HeldKind::shape ("an94" ...), nullptr if none'
Line '    recoil::Kind kind;'
Line '    uint32_t body;          // its first-person body''s colour draw (the profile''s id; the train pistol recoilrun.h''s kTrainPistol), 0 not known'
Line '    float rate, speed;      // RateOfFire, InitialSpeed.z'
Line '    int magazine, pellets;  // Ammo.MagazineCapacity, NumberOfBulletsPerShell'
Line '    float sig[8];           // its standing hip row and gun numbers: up, hmin, hmax, decrease, firstShot, shootingScale, spring, damping'
Line '    float up[3][2], sideMin[3][2], sideMax[3][2], decrease[3][2];  // [stand, crouch, prone][hip, zoomed]'
Line '    float capUp, capSide;'
Line '    float rumble[3];        // LowRumble, HighRumble, RumbleDuration (s)'
Line '    recoil::Kick kick;      // the kick in the hands: two hands at the hip'
Line '    float pairGap;          // seconds: two rounds within it are a burst pair (the AN-94 0.075), else 0'
Line '    float eye[8];           // ZoomedKickbackFactor: irons (the gun''s own), Kobra, PK-AS, PK-A, PSO-1, PKS-07, ACOG, another 6x+ scope (its'
Line '                            // campaign scope if it has one, else its 6x rifle scope, else its ballistic); one it can''t carry: the usual'
Line '};'
Line ''
Line 'inline constexpr Row kRows[] = {'
foreach ($r in $rows) {
    $k = $r.k
    $shape = if ($r.shape -eq '-') { 'nullptr' } else { Cs $r.shape }
    Line "    // $($r.name): $($r.why)"
    Line ("    {" + (Cs $r.name) + ', ' + (Cs $k.blueprint) + ', ' + $shape + ', recoil::Kind::' + $kinds[$k.kind] + ', ' + (Ci $r.body) + ', ' + (Cf $r.rate) + ', ' +
          (Cf $r.speed) + ', ' + (Ci $r.magazine) + ', ' + (Ci $r.pellets) + ',')
    Line ('     {' + (($r.sig | ForEach-Object { Cf $_ }) -join ', ') + '},')
    Line ('     ' + (Rows3 $r.up) + ', ' + (Rows3 $r.sideMin) + ',')
    Line ('     ' + (Rows3 $r.sideMax) + ', ' + (Rows3 $r.decrease) + ',')
    Line ('     ' + (Cf $r.capUp) + ', ' + (Cf $r.capSide) + ', {' + (($r.rumble | ForEach-Object { Cf $_ }) -join ', ') + '},')
    Line ('     {.back = {' + (Cf $r.back) + ', ' + (Cf $r.backAt) + ', ' + (Cf $r.backZ) + '}, .rise = {' + (Cf $r.rise) + ', ' + (Cf $r.riseAt) + ', ' + (Cf $r.riseZ) +
          '}, .lift = {' + (Cf $r.lift) + ', ' + (Cf ($liftTiming[0] / 1000.0)) + ', ' + (Cf $liftTiming[1]) + '},')
    $tail = '      .roll = ' + (Cf $r.roll) + ', .yaw = ' + (Cf $r.yaw) + ', .jitter = ' + (Cf $r.jitter) + ', .burstFirst = ' + (Cf $r.burstFirst)
    if ($null -ne $r.held) { $tail += ', .heldShare = ' + (Cf $r.held) }
    Line ($tail + '},')
    Line ('     ' + (Cf $r.pairGap) + ', {' + (($r.eye | ForEach-Object { Cf $_ }) -join ', ') + '}},')
}
Line '};'
Line 'inline constexpr int kRowCount = static_cast<int>(sizeof(kRows) / sizeof(kRows[0]));'
Line ''
Line '// The next letter or digit of a name, in lower case (0 at its end).'
Line 'inline char NameChar(const char*& p) {'
Line '    for (; *p; ++p) {'
Line '        const char c = *p;'
Line '        if ((c >= ''a'' && c <= ''z'') || (c >= ''0'' && c <= ''9'')) {'
Line '            ++p;'
Line '            return c;'
Line '        }'
Line '        if (c >= ''A'' && c <= ''Z'') {'
Line '            ++p;'
Line '            return static_cast<char>(c - ''A'' + ''a'');'
Line '        }'
Line '    }'
Line '    return 0;'
Line '}'
Line '// Two names alike in their letters and digits, whatever the case: "AS Val", "asval" and "as-val" are one.'
Line 'inline bool SameName(const char* a, const char* b) {'
Line '    for (;;) {'
Line '        const char x = NameChar(a), y = NameChar(b);'
Line '        if (x != y)'
Line '            return false;'
Line '        if (!x)'
Line '            return true;'
Line '    }'
Line '}'
Line ''
Line '// The row whose own numbers these are, as read from a gun''s data (sig as Row::sig: a gun with no camera spring, the AT4, has 0 and 0 there), each'
Line '// within 1e-4; of two that share them the first (the Stinger before the Javelin). Rows with no gun sway are never found this way. nullptr if none.'
Line 'inline const Row* BySignature(float rate, float speed, int magazine, int pellets, const float sig[8]) {'
Line '    if (!sig)'
Line '        return nullptr;'
Line '    auto within = [](float a, float b) { return a - b <= 1e-4f && b - a <= 1e-4f; };'
Line '    for (const Row& row : kRows) {'
Line '        if (row.kind == recoil::Kind::kNone || row.magazine != magazine || row.pellets != pellets || !within(row.rate, rate) ||'
Line '            !within(row.speed, speed))'
Line '            continue;'
Line '        bool same = true;'
Line '        for (int i = 0; same && i < 8; ++i)'
Line '            same = within(row.sig[i], sig[i]);'
Line '        if (same)'
Line '            return &row;'
Line '    }'
Line '    return nullptr;'
Line '}'
Line '// The row of the gun the mod draws with that body (its colour draw''s index count; the train pistol''s kTrainPistol), nullptr if none (0 is none).'
Line 'inline const Row* ByBody(uint32_t body) {'
Line '    if (body)'
Line '        for (const Row& row : kRows)'
Line '            if (row.body == body)'
Line '                return &row;'
Line '    return nullptr;'
Line '}'
Line '// The row of a held gun by its shape (skin::HeldShape(): "an94", "fim92" ...), nullptr if none.'
Line 'inline const Row* ByShape(const char* shape) {'
Line '    if (shape)'
Line '        for (const Row& row : kRows)'
Line '            if (row.shape && std::strcmp(row.shape, shape) == 0)'
Line '                return &row;'
Line '    return nullptr;'
Line '}'
Line '// The row by its name, its blueprint (whole, or its last part: "m240", "model_98b") or its shape, as SameName compares them (recoil.txt''s'
Line '// "gun <name>"); no two rows share one. nullptr if none.'
Line 'inline const Row* ByName(const char* name) {'
Line '    if (!name)'
Line '        return nullptr;'
Line '    const char* letters = name;'
Line '    if (!NameChar(letters))'
Line '        return nullptr;'
Line '    for (const Row& row : kRows) {'
Line '        const char* last = std::strrchr(row.blueprint, ''/'');'
Line '        if (SameName(row.name, name) || SameName(row.blueprint, name) || SameName(last ? last + 1 : row.blueprint, name) ||'
Line '            (row.shape && SameName(row.shape, name)))'
Line '            return &row;'
Line '    }'
Line '    return nullptr;'
Line '}'
$dm = @{}
foreach ($kind in $kindOrder) { $dm[$kind] = $rows[$defaults[$kind]].name }
$middles = ($kindOrder | Where-Object { $_ -ne 'none' } | ForEach-Object { $dm[$_] }) -join ', '
Line ('// A kind''s middle row, for a gun known only by its kind: ' + $middles + ';')
Line ('// kNone''s (' + $dm['none'] + ') has no recoil.')
Line 'inline const Row& Default(recoil::Kind kind) {'
Line '    switch (kind) {'
foreach ($kind in $kindOrder) {
    if ($kind -eq 'none') { continue }
    Line ('    case recoil::Kind::' + $kinds[$kind] + ':')
    Line ('        return kRows[' + $defaults[$kind] + '];  // ' + $dm[$kind])
}
Line '    default:'
Line ('        return kRows[' + $defaults['none'] + '];  // ' + $dm['none'])
Line '    }'
Line '}'
Line ''
Line '}  // namespace recoilguns'

$text = $o.ToString()
if ($text -match '[^\x09\x0A\x20-\x7E]') { throw 'the header would hold a character that is not plain ASCII' }
if ($Check) {
    # (Line ends aside: a checkout may have turned the written LF into CRLF.)
    $now = if (Test-Path $Out) { [IO.File]::ReadAllText($Out).Replace("`r`n", "`n") } else { '' }
    if ($now -ceq $text) { Write-Output "recoilgen: $Out is what the tables make"; exit 0 }
    Write-Output "recoilgen: $Out isn't what the tables make now (run tools\recoilgen.ps1)"
    exit 1
}
[IO.File]::WriteAllText($Out, $text, (New-Object System.Text.UTF8Encoding $false))
Write-Output "recoilgen: $($rows.Count) rows ($(@($rows | Where-Object { $_.k.kind -ne 'none' }).Count) with a gun sway) written to $Out"

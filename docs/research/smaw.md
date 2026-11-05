# The SMAW in BF3's first person, as measured (2026-11-02)

How the game's own arms hold, fire and reload the launcher, measured on Rock and a Hard Place with key 4 in hand
(weapon.txt `drawscan 30`, `allrec 9`, then `tools\allrec.ps1`). Pictures: `screenshots\smaw\` (not in git). Numbers are
the game's own bones, before the mod moves anything.

## Its draws

First-person draws (stencil ref 32) with the launcher in hand:

| Draw (indices, start, base) | Stride | What |
|---|---|---|
| 3414, 0, 0 | 48 | launcher part A. Bones 0, 1, 2. **Bone 0 is its frame** |
| 18222, 3414, 0 | 48 | part B. Its bone 0 is part A's bone 2 |
| 120, 21636, 0 | 48 | part C, on the same bone |
| 21756, 21756, 0 | 16 | their depth pass (3414 + 18222 + 120), bone 0 as part A's |
| 13755, 15051, 0 | 48 | gloves, as with the M416 |
| 7617, 49995, 0 | 16 | gloves' depth |
| 2016, 13035, 0 | 48 | bare forearms |
| 8529, 0, 0 and 4506, 8529, 0 | 48 | chest and sleeves, legs |
| 21189, 28806, 0 | 16 | body's depth |

Only the first bones of a palette are the launcher's: the rest of the 4096-byte upload is what was left in the buffer.
Part A's key is shared with other models, so it's told by its bones: bone 0 and bone 2 are 49.8 cm apart.

## The frame

Part A's bone 0: x left, y up, z forward (as the M416's body). In the arms' space it sits at (0.597, 1.545, 0.098) at rest,
z = (-1.00, 0, 0.07). Bone 2 is rigid on it at (0, 0.073, -0.493). Bone 1 moves against it during the reload (up to 45 cm):
it's what's loaded.

## The hold

Palms in the launcher's frame, using the gloves' bones (right root 37 with `kRightPalm`, left bone 1 with `kLeftPalm`):

- **Right palm at rest: (-0.026, -0.141, 0.061).** On the rear grip. It stays there while firing.
- **Left palm at rest: (0.039, -0.073, 0.315).** On the front grip, 25 cm ahead of the right.

## The shot and the reload (the rocket fired at 0 ms)

The reload starts by itself about 1.2 s after the shot and is over about 6.5 s after it:

| Time | Launcher in the arms' space | Right palm | Left palm |
|---|---|---|---|
| 0-1.2 s | at rest | on the grip | on the grip |
| 1.5-2.5 s | swings to nearly straight up (z = (-0.15, -0.92, 0.38)) | on the grip | stays (moves 2 cm) |
| 2.9-6.1 s | stays up | **off the grip**, up to 53 cm: fetches the rocket and loads it at the back | stays |
| 6.1-6.5 s | swings back | returns | stays |
| 6.5 s on | at rest | on the grip | on the grip |

So the game's **left hand holds the launcher all through the reload and the right does the loading**. The picture at 2.5 s
shows a tube with yellow bands coming up beside the launcher, at 3 s the right hand pushing it into the back.

## The aim

What the tests with the stand-in hand show (nobody in the headset, pictures from `flashburst 16`):

- **The rocket's direction follows the hand**, not the head. Hand 40 degrees left: its flame about 35-40 degrees left at 0.2 s. Hand raised 15 degrees with the head level (weapon.txt `raisehand 15`): its flame 15 degrees up, at the muzzle. Those are one shot each.
- **With the spread out, three shots straight ahead** put their sparks within a few degrees of the middle of the view. Not measured further than 0.2 s of flight.
- **Zoomed the user saw it go true**, unzoomed "way off". Data: 2 degrees and more unzoomed, 0 zoomed. The user doesn't want to shoot zoomed.
- **The start point:** the game starts it at the game's eye plus (-0.30, -0.10, 2.0) along the aim (x left, y up, z forward; `ForceSpawnToCamera`), so it flew parallel to the tube but from beside the head.

## What the mod does about it

`ClearLauncherSpread` in `skin.cpp`, a thread, on the launcher's first draw of a level (again after 10 minutes):

- **Spread:** `40mm_GunSway`'s dispersions in memory: min 1-8 in halves, max 5.5, 1.8 a shot, 15 a second. Exactly 10 found (same count in each run). Min and growth set to 0, max kept. A first version took anything ending 1.8, 15, zeroed other data and crashed the game. Never loosen that match.
- **Start and direction:** `ShotConfigData`, InitialPosition -0.3, -0.1, 2.0, InitialDirection 0, 0.01, 1, InitialSpeed 0, 0, 40, each padded to 16 bytes. Two copies in the mission's data. The direction is made 0, 0, 1. The position is rewritten at each draw of part A to the drawn muzzle seen from the game's eye (log with `launcherlog`: "the rocket starts at"). `launcherspawn 0` leaves it.

Shared with the M320 and the Stinger: the spread only, they have their own ShotConfigData.

**Open:** a bright streak from the bottom of the view to its middle at 0.2 s, seen once with the hand held 68 cm out (`standin 1 0 0 0.6 -0.2 0`), not from the normal hold. Not understood. It may be the rocket's trail.

**The iron sights (user: "shoots higher"):** zoomed, the game's aim on the launcher's frame is (0.0005, -0.0122, 0.9999), 0.7 degrees below its z, with the game's eye at (0.030, 0.170, -0.029). So the tube is raised 0.7 degrees from the aim (`launcherPose` in `hands.cpp`; weapon.txt `launcherup` tunes it live). **The user confirmed it in the headset: on target with the iron sights, no zoom.**
## Things tried

- `wcam` (moving the weapon camera) doesn't move the rig: only its turn counts. So the rig is moved by its bones.
- Holding the game's zoom gives 0 spread and no magnified world, but the first person is drawn too big then: the mod's FOV
  fix looks for the game's 55 degrees and the zoom has 40 (the log's "FOV: source 40.0"). To use it, that scan must learn the
  zoomed FOV. Not done: the plain spread is the game's normal 2 degrees.

## In the mod

weapon.txt `launcher 0` turns it off (on by default): `DrawLauncher` and `DrawLauncherRig` in `skin.cpp`, `launcherPose` in `hands.cpp`. The
frame bone goes where the right controller holds it, palm on the rear grip; the launcher, gloves and forearms move by that one
move, so the game's hold and reload stay; chest, sleeves and legs fold away as with the M416.
Effects go through `g_launcherEffect` and `GameGunMuzzle` (the muzzle's 0.6 m along z is from pictures, not measured).

## What it's made of (the mesh dumps, 2026-11-04)

On the frame (x left, y up, z to the muzzle), the launcher at rest:

| Bone | What | Where |
|---|---|---|
| Part A bone 2 | the launch tube | z -0.139 to 0.640, round y 0.09 |
| Part A bone 1 | **the rocket's case**: 64 cm long, 8.9 cm across (13.6 at the flange on its back: its rings, `rocketgrip radius`), the long yellow-banded tube at the back, a red band at its front | z -0.726 to -0.085, round x 0, y 0.0915 |
| Part A bone 0 | a small part by the trigger | z 0.15 to 0.17 |
| Part B bone 0 (part A's bone 2) | the launcher's body | z -0.191 to 0.624 |
| Part C | a thin line along the top | z 0.06 to 0.61 |
| Depth pass | bones 0, 1, 2 as part A's, so the case is its bone 1 too | |

Bone 1 at home is the frame's own turn, moved (0, 0.185, -0.245). In the game's reload it comes forward 2.7 cm at the start, is turned
20 degrees about its axis, taken away to the side (up to 45 cm), brought back and seated, and goes back the 2.7 cm at the very end. The
same mesh goes and comes back: nothing falls away.

As on the real SMAW: the sealed rocket's case goes on the back of the launcher. With it off, the launcher's back is an open tube 25 cm
behind the right palm.

## Its rockets

The soldier's weapons component (server soldier +0x4C0) has one firing state for each weapon, whatever is in hand: +0x248 the M416's
(so `ammo::Get` shows the rifle's with the launcher in hand), **+0x258 the SMAW's**, 1 loaded and 3 spare at the mission's start. A shot
took it to 0/3 at once, and the game's reload to 1/2 4.6 s after the shot.

## Loaded by the left hand

`src/rocket.cpp` (pure; `tools/rockettest.cpp` runs it offline), wired in `hands.cpp`; weapon.txt `launcherhands 0` goes back to the game's own
hold and reload. The user's asks, 2026-11-04: the spent case stays on until pulled off by hand; no flipping (it's positional); the game's lock-in
twist, out and in; snappy, magnetic but not too much.

- **The grip (`src/rocketposes.h`, from `tools/rocketfit.cpp`):** as a hand holds a cup: the palm against the case, the four fingers closed on it one way round (about 100 degrees), the
  thumb the other way (about 150), every part touching. In the game's reload the left hand stays on the front grip and the **right** hand changes the rocket; where along the case
  it is (z -0.07) and how far it's turned on it are taken from that (`tools/rocketgrip.cpp`), mirrored. Its fingers aren't: made for one camera, they stand 2 cm off the case with the
  thumb on the fingers' side. **Two grips hovered because I took the case for 12.7 cm across (its flange) instead of its body's 8.9.** The hand may turn about the case and hold it
  either way round: `MakeHold` turns the case in the hold, never the fingers.
- **The count:** the launcher gets a hidden round while it's in hand (`ammo::Update`, as the M416 has one), so the game never finds it empty
  and never starts its reload. The mod's count without it decides whether the trigger fires (`input.cpp`). **The HUD reads one high.**
- **Fired:** the spent case stays on. Nothing happens by itself.
- **Taking it:** a fresh squeeze of the left grip within 17 cm of the case's line (frame z -0.70..-0.20). The palm goes on it the way it faces, whichever
  way round, where along the case the hand is. Let go before it's open: it eases back.
- **Open:** a roll of the wrist (12 degrees) or a pull (2.5 cm) opens the lock: it twists 20.5 degrees about its axis and comes 2.7 cm forward, as in the game's
  own reload (rec 3.19-3.34 s, 1.57-1.88 s).
- **Off:** it follows the hand along its line; 16 cm back, or the hand 32 cm off the line for 3 frames, and it's free, gliding into the hand in 0.15 s. Let go
  on the way: it slides off by itself. Let go free: it falls for 1.3 s and is gone.
- **A new one:** a fresh squeeze over either shoulder (from the neck point, above 12 cm below it, behind 12 cm in front of it, within 60 cm). Its nose
  points the launcher's way whichever way the hand is turned: no flipping.
- **The catch:** the hand within 20 cm of the launcher's back, or the case's line within 11 cm of it, any orientation. Onto the line, 16 cm back and twisted open,
  in 0.15-0.3 s, the glove with it. Pulled 32 cm off: back in the hand until it's been away.
- **Home:** pushed along the line, or by itself after 0.30 s, or let go; in at 0.5 m/s. Then the lock: a roll of the wrist, or by itself after 0.30 s; 0.14 s
  to turn shut; only then the game's count. It settles 2.7 cm back in 0.2 s.
- **X held with it empty:** the game's own reload and animation, the hidden round taken out so the game finds it empty.

Sounds: `build\sounds\rocket_unlock/out/take/catch/in/lock.wav`, cut from the game's own `smaw_reload_wave` (1.72-2.00, 2.00-2.28, 2.76-3.05, 3.82-3.90, 4.12-4.42,
5.39-5.50 s), not in git.

## Weapon switches (film of the switch, 2026-11-04)

What each frame of a switch showed, and why:

- **The arm floating ahead of the rifle after the launcher, for 5 s.** In the last frame the rifle's drawn as it's put away, the game's hands are half a metre off it
  all at once. The hold was judged a frame late, so that one frame taught the gloves where they sit on the gun. Now a hand's bones are only noted during a frame, and taken at its end
  if the hold was good all through it (`TakeFrameOfHands`).
- **The launcher's hands.** Raising and putting it away the game swings its arms about it, and the palms stay on it (they alone say nothing). Once they've rested (palms at
  home within 1 cm, the forearms' middles within 5 mm, for 0.7 s) that rest is kept for good (`RigSet`), and the launcher itself is placed by its tube, bone 2.
  Only the game's own reload is followed. (`launcherGameHands`)
- **The tube a big black shape for a frame.** At the very end of the put-away the game parts bones 0 and 2 (0.39-0.44 m, not 0.498): part A's draw was taken for something
  else and left in place, its depth moved. Placed by bone 2 now.
- **Long black shapes across the view in the rifle's first frame.** Its parts went through the launcher's rig, which doesn't place gun parts.
- **All of a frame's arm draws go one way** (`g_rigThisFrame`, set at the frame's start), so a switch can't fall between an arm's depth and its colour.
- **Still there, not ours as far as I can tell:** dark jagged patches moving over the forearm for a quarter of a second while the game's weapon swings. All the arm's draws go through the kept rest then,
  so nothing of the arm moves: it looks like the shadow of the soldier's own third-person body (its low-resolution, stair-stepped edge). White spikes in the films were enemy tracers.

## The rifle in the car (the mission's start, gunpartlog, 2026-11-04)

Seated in the ride the game has the M416's parts in another pose, for about 18 s, and slides them back in half a second as the soldier is thrown out. On foot every one of them lines up with the
gun's body. The numbers are each part's place on the body, on foot and seated:

| Part | On foot | Seated |
|---|---|---|
| Grenade launcher's frame (draw 7848, bone 0) | 0, 0.008, -0.142 | 0, -0.006, -0.137 |
| Its tube (7848, bone 1) | -0.019, 0.012, 0.454 | -0.030, -0.044, 0.372, and turned a quarter about its length |
| Its grenade (1440, bone 0) | 0.015, 0.023, 0.231 | 0.028, 0.015, 0.372 |
| Rifle's bone 1 | 0, 0.045, 0.175 | 0, 0.030, 0.199 |
| Rifle's dust cover (bone 3) | -0.022, -0.082, 0.415 | -0.018, -0.096, 0.415 |
| Magazine (bone 4) | 0, 0.131, 0.343 | 0, 0.109, 0.338 |
| Rounds (depth pass bone 5, the rounds' own draw) | 0, 0.173, 0.309 | 0, 0.109, 0.338 |

The depth draw 9288 has the launcher's three bones in that order. The tube, 28 cm long, hangs out under the launcher's frame and leaves its window empty, so the grenade shows through: what looked like
the launcher clipping. Only the tube turns, so a check of the z axis alone calls it unchanged (the y axis has it: -1, 0, 0). `PinPart` and `PinSeatedBones` keep them all at the foot pose
while both the tube and the grenade are off by more than 3 mm: on foot the game's moves are left alone. `partshift` and `partpin` in weapon.txt try a pose on foot.

## The whole arms (2026-11-04)

Both forearms are solved from their hands on every level's gloves (`hands.cpp PlaceForearms`, `armik.h`), with the launcher too. Solved, each arm is drawn whole: the upper arm (`Result::upper`, hinged at the forearm's elbow end) and the collar bone with the body's axes; the chest and legs fold between the shoulders. Folding the sleeves into the forearm's middle left the forearm an open tube with a fin of skin at its elbow end.

Which frame each bone goes on (0 elbow end, 1 twist A, 2 twist B, 3 upper arm, 4 collar), by its twin among the colour draws (vertices at the same bind place, the weight they share):

| Part | Sleeves 8529 | Bare forearms 2016 | Body depth 21189 | Glove depth 7617 |
|---|---|---|---|---|
| Collar | 3, 8 | | 15, 42 | |
| Upper arm | 1, 2, 9, 12 | 1, 6, 11, 16 | 16, 17, 43, 44 | 13, 14 |
| Elbow end | 4, 6, 10, 13 | 0, 3, 10, 13 | 18, 41, 45, 54 | 15, 38 |
| Twist A | 5, 11 | 2, 12 | 19, 46 | 16 |
| Twist B | | 4, 14 | 20 | 17 |

A depth vertex a millimetre off its colour twin shows black. `weapon.txt armdump` writes a frame's palettes (`build\armdump_N.bin`), to compare every vertex offline: at most 0.1 mm apart now. `armnudge B N` lifts bone B of the draw with N indices 30 cm, to see what's on it.

A new arm mesh on another level needs this table before its wrists count as working.

**The arms never stretch.** The left glove on the handguard can be further from the shoulder than an arm is long: 1.08 m in the user's picture ("I look like Mr. Elastic"), with a hand 87 cm from the right and the arm 0.56 m. The upper arm's frame is rigid from the elbow end, so held at the estimated shoulder the collar stretched the sleeve between them 2.6 times. Now:

- **The collar goes to the upper arm's far end** (`Result::shoulder`): the estimated shoulder, or as far towards it as the bone's length reaches. Row 9 of `tools/armtest.cpp` checks 20000 random hands, both arms.
- **The chest turns** with how far the hands are apart along the body (100 degrees a metre, at most 45, eased): the left shoulder 11 cm forward for a rifle, as the game's own hold has it. The elbow's way stays in the square body's axes, as fitted.
- **Brought further than 12 cm the collar draws in** to the joint, gone by 26 cm: what ties it to the chest folded between the shoulders was a flap across the view (3%, not nothing: a zero matrix shows black).
- **A hold the arm can't reach lets go, for now:** more than 0.80 m from the square shoulder with the real hands more than 0.62 m apart, the glove goes back to the real hand and the gun to the right hand's aim (`g_holdOutOfReach`); it comes back under 0.74 m or with the hands within 0.52 m, the grip held all along. (At first the second test was the real hand's distance from the hold, 0.38 and 0.30 m: ordinary swings came off, 19 times in a run, since with the steer faded out that place is far from any hand.) In ordinary aiming the real hand is 20-30 cm from the hold. In the user's test: reach median 0.59 m, the hand's 0.26 m from the hold, off the gun 4 s of 118 while they reached out on purpose.

`build\arms.txt` "test guard" holds the handguard with nobody in the headset, the stand-in's left hand as the real hand ("test move -0.45 -0.05 0.45" leaves it behind); "log 1" logs both arms and the hold once a second.

## The rifle's aim (2026-11-04)

Zoomed, the game's eye on the M416's body (bone 2) is at (0, 0.112, 0.148), looking along (0, -0.005, 1): 0.29 degrees below the body's z. Free hands fire along the aim pose, so:

- **The rifle is raised 0.2865 degrees from the aim** (`rifleTurn` in `hands.cpp`, `weapon.txt rifleup`), as the launcher is. The left hand's steer goes through the raise.
- **Its bullets start on the sights' line at the muzzle** (`g_rifleShot`, `ShotStart`, 6 places found by InitialSpeed 600 with position and direction at rest, each checked before each write, once a frame). `rifleshotby 2.5 0 0` showed the game does use it: the hits moved 2.5 m. It's data of the 600 m/s guns: the AI's copies are 250. **Another gun picked up gets the M416's last start until it has its own.**
- **The chevron** (the scope's 12-index draw, `PlaceReticle`) sits 29 cm ahead of the zoomed eye in the model, 2 degrees off for each centimetre the eye is off the tube's axis. It's drawn on each eye's own line along the sights, the same size to the eye, so it shows where the bullets go from wherever the eye is in the tube. `reticlefar 0` puts it back.
- **The film:** `standin 1 -9 25 -0.066 0.082 0.403` looks down the tube at the car about 12 m off, and shots land within about 5 px (0.3 degrees) of the chevron's tip. Dust on the rock drifts: sparks on metal are the way to tell.

## The left hand and the front grip

The left glove goes with its controller like any free hand. A squeeze with the palm within 25 cm of the game's own left palm on the launcher
takes the front grip: the glove eases on over 5 frames onto the game's own left glove as its animation has it that moment (`LauncherLeftGrip`,
learned each frame), and points the launcher as it does the M416 (`steer.h`; `launchersteer 0` not). Only letting go lets go.
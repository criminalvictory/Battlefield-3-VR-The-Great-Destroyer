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

## Things tried

- `wcam` (moving the weapon camera) doesn't move the rig: only its turn counts. So the rig is moved by its bones.
- Holding the game's zoom gives 0 spread and no magnified world, but the first person is drawn too big then: the mod's FOV
  fix looks for the game's 55 degrees and the zoom has 40 (the log's "FOV: source 40.0"). To use it, that scan must learn the
  zoomed FOV. Not done: the plain spread is the game's normal 2 degrees.

## In the mod

weapon.txt `launcher 1` (off by default): `DrawLauncher` and `DrawLauncherRig` in `skin.cpp`, `launcherPose` in `hands.cpp`. The
frame bone goes where the right controller holds it, palm on the rear grip; the launcher, gloves and forearms move by that one
move, so the game's hold and reload stay; chest, sleeves and legs fold away as with the M416. The real left hand isn't used yet.
Effects go through `g_launcherEffect` and `GameGunMuzzle` (the muzzle's 0.6 m along z is from pictures, not measured).
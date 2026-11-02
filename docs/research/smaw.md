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

With the stand-in hand 30 degrees right of the head (`standin 1 0 30`) the rocket went 30 degrees right of the head, along
the hand, and blew up on the jeep there. The game's look follows the aim pose as with any gun. Before the launcher was placed,
the mod left it and the arms where the game put them, turned with the aim only: the tube at the bottom of the view, the
chest and sleeves showing, nothing at the controller. That's the likely "haywire".

`wcam` (moving the weapon camera) doesn't move the rig: only its turn counts. So the rig is moved by its bones.

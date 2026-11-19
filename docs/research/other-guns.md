# The guns that aren't the M416, and their sights

Measured 2026-11-05 on Rock and a Hard Place, with nobody in the headset.

## Getting one in hand without a pick-up

A checkpoint restores its own inventory, so editing the kit array or a `U_` asset does nothing at a respawn. What works is the weapon's blueprint: the M416's blueprint `+0x20` (its data, normally its own `+0x40`) pointed at the other gun's. The next soldier's slot 0 weapon is built from that data with the M416's counts of spare rounds. Blueprints are found by their name at `+8` (`Weapons/M416/M416`, `Weapons/AN94/AN94`, `Weapons/RPK/RPK`), and their addresses change with each launch.

A sight comes the same way: the soldier's checkpoint carries `U_M416_ACOG`, and the places in the gun's data that name, say, `U_AN94_PSO-1` are made to name that instead. The swapped gun is then built with the PSO-1.

All of this is in memory, for that launch only.

## Reading a gun

- `allrec N` records every draw's palette for N seconds. Fire first: a full gun won't reload.
- `dump N S` writes a draw's mesh; each vertex has its position in half floats at `+0`, bone indices at `+8` and weights at `+12` (stride 48; a sight's stride is 32, all on bone 0).
- `heldlog 1` with the zoom held gives the view on the gun's frame: the sights' line, and the FOV, whose ratio to the unzoomed 55 is that sight's power.
- Palettes hold only the bones a draw uses. Past those is stale data, which can look like bones.

## AN-94

| Draw | Key | Bones |
| --- | --- | --- |
| Colour | 16188/0 | 0 frame, 2 lever, 3 barrel, 4 seated magazine, 5 the game's spare copy of it |
| Rounds | 312/16278 | 0 the rounds, on the magazine |
| Depth | 16590/16590 | as colour, 6 the rounds |

The spare copy sits 1.086 m behind the gun until the reload needs it, so it's hidden unless the game's own reload is playing: with the gun out in the hand it would float behind it.

The gun's frame is bone 0. Magazine on the frame: x +-1.3 cm, y -16.2 to 3.3 cm, z 34.8 to 47.0 cm, curving forward. Lever knob at (-0.034 0.068 0.495), travel 11.3 cm. Iron sights' line y 0.092, straight along z. The barrel recoils 10.6 cm firing, the lever goes 5 cm forward with it.

Its reload, from R pressed: bone 4 hides and bone 5 takes the seat; about 0.7 s in, bone 4 shows in the left hand; at about 1.2 s the fresh one swings forward into the old one, which flies off forward at about 3.4 m/s and falls, turning over; the fresh one rocks into the seat by 1.9 s; the hand is back on the handguard by 2.5 s. An empty reload then goes over the top to the lever, draws it back 11.3 cm in about 0.4 s and lets go.

## RPK-74M

| Draw | Key | Bones |
| --- | --- | --- |
| Colour | 24492/0 | 0 lever, 1 magazine, 2 a small part, 3 frame |
| Rounds | 312/24492 | 0 the rounds |
| Other part | 36/24804 | 0 the body |
| Depth | 24840/24840 | as colour, 4 the rounds |

The gun's frame is bone 3, not bone 0. Long magazine on the frame: x +-1.8 cm, y -21.7 to 1.3 cm, z 37.1 to 58.2 cm. Lever bone's knob about (-0.05 0.04 0.48). Iron sights' line y 0.086. Its own reload pulls the one magazine off and brings it back, with no flick.

## The others this level has loaded

Whole guns in memory on this level, by their data under `Weapons/`: AEK-971, AK-74M, AN-94, G3A3, KH2002, M16A4, M240, M416, M4A1, M9, RPK, SMAW, FIM-92, the enemies' RPG-7. (The SVD's and SV98's textures are there, nothing else of them.) An AKS-74u, SKS, SVD, SG553 or AS Val can only be measured on a level that loads it.

The same switch works for all of them. A depth pass doesn't always keep the colour pass's bone order, and a body doesn't always start at index 0: check each draw's palette against the body's, bone by bone.

| Gun | Colour | Other draws | Depth | Frame | Magazine | Lever | Sights' line, zoomed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| AEK-971 | 21585/0 | rounds 318/21615 (bone 0), frame part 30/21585 | 21933/21933: as colour, 5 rounds | 4 | 0; 3 the spare that flies off | 2, knob (-0.031 0.054 0.451), 9.2 cm | (0 0.095 0.127), 0.21 degrees down |
| AK-74M | 21723/0 | rounds 312/21723, more magazine 894/22035, frame part 66/22929 (each bone 0) | 22995/22995: as colour, 4 rounds | 2 | 3 | 1, knob (-0.037 0.036 0.497), 9.9 cm | (0 0.070 0.127), straight |
| G3A3 | 23064/0 | | 23064/23064: as colour | 0 | 1 | 3 on its LEFT, (0.022 0.078 0.77), 17 cm | (0 0.109 0.126), 0.48 degrees down |
| KH2002 | 15237/0 | rounds 312/15237 | 15549/15549: as colour, 4 rounds | 3 | 1 | 2, a hook on top under the handle, (0 0.058 0.392), 8.5 cm | (0 0.083 0.122), 0.47 degrees up |
| M16A4 | 17013/312 | rounds 312/0, frame part 1350/17325 | 18675/18675: 0 and 1 magazine, 2 lever, 4 frame | 4 | 2 | 0, T handle (0 0.05 0.292) | (0 0.097 0.134) |
| M4A1 | 22194/312 | rounds 312/0 | 22506/22506: 0 and 4 magazine, 1 lever, 2 cover, 3 frame | 2 | 3 | 0, T handle (0 0.05 0.292) | (0 0.096 0.134) |
| M240 | 27588/2364 | belt 2304/0, the tape on its box 60/2304 | 29952/29952: 0-4 belt, 5 box, 8 lever, 9 frame, 10 lid | 3 | 4, the box | 2, knob (-0.062 -0.002 0.442), 12.9 cm | (0 0.087 0.124), 0.22 degrees down |
| AKS-74u | 17991/0 | rounds 312/18957, more magazine 894/18063 (each bone 0), frame part 72/17991 | 19269/19269: as colour, 4 rounds | 2 | 3 | 0, the bolt and its handle on the right, knob (-0.037 0.036 0.4715), 9.4 cm firing, 6.4 by the game's hand | (0 0.074 0.127), 0.42 degrees up |
| UMP-45 | 19314/234 | rounds 234/0, frame part 90/19548 | 19638/19638: 0 rounds, 1 magazine, 2 handle, 3 frame | 2 | 0 | 1 on its LEFT, knob (0.026 0.061 0.5715), 8.3 cm, then 51.8 degrees up into a notch | (0 0.097 0.080), 0.05 degrees down |
| Mk11 Mod 0 | 12456/2214 | a piece of two bones 1902/0 (0 the frame, 1 the magazine), rounds 312/1902 (bone 0); its rifle scope 12642/156 with glass 156/0 | 14670/14670: 0 frame, 1 and 2 magazine, 3 trigger, 4 port cover | 0 | 3 | none: its T handle is the frame's, (0 0.051 0.274) | through its scope: (0 0.0922 0.198), straight, FOV 8.7 |
| M249 | 24291/2808 | belt 2808/0 (five bones, the M240's links), its box 987/27099 (a cloth pouch alone on its own bone 0) | 28086/28086: 0-4 belt, 5 handle, 6 cover, 7 carrying handle, 8 frame, 9 box | 3 | none in the body: the box is its own draw | 0, knob (-0.0725 0.021 0.4715), 12.9 cm; 1 the feed cover, hinge (y 0.0575, z 0.4751), 102.6 degrees | (0 0.093 0.088), 0.68 degrees down; the aperture to the post 0.78 |

The Mk11 was measured 2026-11-18 on Operation Swordbreaker (gloves): [mk11.md](mk11.md); the M249 the same morning: [m249.md](m249.md). The two before it were measured 2026-11-18 on Semper Fidelis, where the soldier's arms are bare (one draw, 18108/16053) and the gloves' draw is not there. `tools/baregloves.ps1` writes such a recording again with the gloves' draw in it (the bare hands' bones under the gloves' numbers, `skin.cpp`'s kBareGlove), so `magfit` and `holdpic` read it as any other. Each has a note of its own: [aks74u.md](aks74u.md), [ump45.md](ump45.md).

The M1014 of Semper Fidelis (measured 2026-11-18; its own note is `m1014.md`): colour 13377/0, the sight's dots 324/13377 on
the frame, the shell 450/13701 (its own bone 0), depth 14151/14151 (bones 0 to 3 as the colour's, 4 the shell); frame bone 3;
no magazine: bone 0 is its bolt and handle (knob (-0.030 0.1045 0.492), 6.0 cm), 1 its trigger, 2 its fore-end; its sights'
line (0 0.139 0.198) zoomed, straight along z. Its shells go in by hand (`shellload.h`), not through `handmag`.

How each reloads in the game (all recorded with `allrec`, a part-used magazine and an empty one):

- **AEK-971:** as the AN-94's: bone 0 hides, the spare on bone 3 takes the seat and flies off 3 m, the fresh one on bone 0 swings in. An empty one then draws the lever 5.5 cm.
- **AK-74M:** pulled off and brought back, the left hand under it; an empty one then draws the lever 9.9 cm with the palm on the gun's right.
- **G3A3:** pulled down and away. An empty one first goes to the handle, far forward on the left, which swings out, draws back 17 cm and turns up into a notch; the magazine changes; the handle is slapped down last.
- **KH2002:** the left hand goes back behind the grip. An empty one then reaches over to the hook on top. The port is on the right at z 0.11 to 0.19, by the cheek.
- **M16A4, M4A1:** straight down and straight up; an empty one ends on the left at the bolt catch. Flat tops: with no sight fitted there's nothing to look through.
- **AKS-74u:** as the AK-74M's: rocked off forward about 20 degrees, carried away, the fresh one rocked back in; an empty one then goes over the top to the handle on the right and draws it 6.4 cm.
- **UMP-45:** drawn straight down 22 cm, with no rock, and the fresh one straight up. An empty one starts at the handle on the left: back 8.3 cm, turned 51.8 degrees up into its notch (about z through x 0, y 0.0564; it rests 6.55 cm back); the magazine changes; the handle is knocked down last.
- **M240:** the right hand draws the handle back 12.9 cm. The left lifts the lid (bone 5 turns about 100 degrees up), slides the box off sideways to the gun's left, brings a new one back the same way, lays the belt, closes the lid.
- **M249:** the M240's, step for step (its hand on the box and on the cover turned as the M240's to a hundredth): the right hand draws the handle 12.9 cm and pushes it home; the left lifts the cover 102.6 degrees, slides the box (its own draw) 8 cm to the gun's left at its seat's height and then takes it left and down, without a turn; brings it back in from the left at the seat's height, turned 5.5 degrees about the gun's length, settling; lays the belt, slaps the cover shut. By hand along that same way (m249.md).

### The M240's reload, measured

An empty reload, from R pressed (the colour palette, frame bone 3):

| Time | What |
| --- | --- |
| 0.4-1.4 s | the right hand draws the lever back (bone 2) |
| 1.9-2.3 s | the left hand to the lid's back end, (0.048 0.087 0.335) |
| 2.4-2.8 s | the lid (bone 5, depth 10) opens 103 degrees about x, through y 0.068, z 0.568, and stays up |
| 2.8-3.1 s | the hand down to the box, (0.117 -0.081 0.494) |
| 3.1-3.7 s | the box and the belt out to the gun's left and down |
| 3.9-4.5 s | a new box up into its seat, the belt hanging out of it |
| 4.8-5.3 s | the hand takes the belt and lays it |
| 5.5-5.9 s | the hand up on the lid and slaps it shut |
| 6.1-6.4 s | back on the front grip |

The belt is its own draw, 2304/0, five bones, the same five first in the depth pass. Each piece's own space runs along its bone's x or down its y: lying in the feed they're nearly unturned; the piece's ends are in `kM240Belt` (skin.cpp).

The game shortens the belt by its client's count, squashing bones 2, 3 and 4 to nothing (scale 0, all at one point) when it runs low. That count doesn't follow the server's when the mod writes a magazine in, so after a reload by hand the belt stays squashed until a shot wakes it. So the mod shows the pieces by the true count, about 2.6 rounds to a piece, and puts a squashed piece that should show back where the belt lies.

The strip of tape over the box is its own draw in the colour pass, 60/2304 (bone 0 is the box), and only triangles on the box's bone in the depth pass: indices 36039 to 36099 there. It stays on whatever's in the box. The rounds seen in the box are the belt's last pieces (bones 3 and 4 hang down into it), which go first as the belt shortens.

Casings and links come out of the right end of the feed's hole, about (-0.035 0.055 0.505).

### Port covers

The M4A1's and M16A4's meshes have the port's cover hanging open under the port, a plate on a hinge rod along z at (-0.0175 0.0177) and (-0.0175 0.0193): half a turn up about that is shut. On the M4A1 it's bone 1 (depth 2). On the M16A4 it's on the frame bone, 204 indices of the body: 2949 in the colour pass, 24393 in the depth pass, whose order is its own; to move it the body is drawn in three parts.

### Iron sights on the M16 and the M4

A gun's parts come in sockets (`SocketData`, in Venice-EBX's listing), each with an `UnlockAsset` that has to be in the soldier's kit. The carrying handle with its rear sight, and the front sight, `weapons/accessories/m16_ironsight/m16_ironsight_1p_Mesh` and `m16_sight_1p_Mesh`, hang on `Weapons/Common/NoOptics`. The test soldier's checkpoint carries the M416's ACOG instead, so the swapped gun comes with no sight at all; the soldiers around him have theirs. In the test: the places near the gun's blueprint that name `NoOptics` made to name that ACOG (the `sighttest` trick).

An attachment doesn't change a body's numbers: an RPK with its bipod is still 24492/0, the bipod two more draws (2952/0, 2952/2952) that move with it.

## The Stinger on this level

`Weapons/FIM92A/FIM92A_SP_Valley`. Unzoomed: colour 20298/354, a piece of the sight 354/0 (its bone 0 is bone 1 of the colour), depth 20298/20652. Frame bone 0, the tube along z from -0.345 to 1.174 with its axis at y 0.055; the grip under it at z 0.6, the front grip at z 0.82; bone 1 the sight on its left, bones 3 to 6 the aerial's panels on its right. Palms: right (-0.046 -0.132 0.593), left (0.025 -0.146 0.993). Zoomed the game draws another model altogether, rigid, stride 32 (10233/624 and its parts).

Nothing comes off it in its reload (4.3 s, by itself). About 1.4 to 2.3 s in, the aerial's four panels (bones 3 to 6) fold down flat against the tube by themselves, with the hand well away from them; at 2.4 s the left hand is round on the tube's right, (-0.135 0.044 0.852), takes the folded aerial and lifts it, and by 2.86 s it has sprung open. By hand: fired, it folds; lifted (grip or trigger), it springs open and that's the next missile (`src/flap.cpp`).

Its lock is in `LockingControllerData`: the floats LockTime 2, ReleaseTime 0.25, ReleaseOnNewTargetTime 0.5, SampleRate 0.06, HoldStillThreshold 10, RayLength 3000, AcceptanceAngle 5, with a pointer in front of them to `ZoomLevelLock`, an array of two {OutlineTaggedDistance, LockType}: [0] 5 (LockTypeCount, no lock), [1] 2 (LockOnHeat). Writing 2 into [0] makes it lock unzoomed: shots then fire at the live jet with `FireOnlyWhenLockedOn` still on. That flag is the third byte of four after the `LockingWeaponData`'s pointer to the controller, a null and the warning type.

The missile's `ShotConfigData`: InitialPosition (-0.2 -0.065 0.35) from the camera, InitialDirection (0 0.12 1), InitialSpeed (0 0 16), each padded to 16 bytes. Rewritten each frame as the SMAW's, the trail starts at the tube's end.

Where it is: the `rifle_after_bmps` checkpoint is at (1268 68 -835) and the jet is overhead within seconds of walking on. The road runs along x 1256 to the wrecked Growler; there the objective comes up ("Defeat Russian fast air", "Grab Stinger and take out Su25"), and the tube lies on the road at (1260.6 65.15 -637.7). It's taken by holding R (the prompt, far to the right of the picture, says so), within 2.5 m, into slot 1 and into the hand. With its 3 to 5 missiles gone the game goes back to the rifle by itself.

## How the game draws a sight

A sight's model has its parts one after another in one vertex buffer, each drawn with its own vertex-buffer offset, and its indices count from there. Read a part's vertices from the offset its draw binds, or they come out as another part's. All its vertices are in the model's space: x left, y up, z towards the muzzle, the eyepiece at -z.

Unzoomed: the body's two passes, its glass, and for the ACOG its chevron and red fibre. The glass is an ordinary transparent draw in the lit pass, about 18% opaque with a reflection, in a stencil 32 forward pass with depth test and no depth writes.

Zoomed, the game swaps in another model of the sight, with:

- the body, more detailed (6705/12 for the ACOG, against 9273/12);
- a domed glass with less tint (pixel shader `6f85641c70a67478`);
- black lines and rings on one flat plane across the view, drawn by multiplying (blend dest colour, zero; pixel shader `8f66b6f4d0a7b611`): the picture times one minus the sheet;
- marks in a colour on the same plane (pixel shader `17b60c01c6d59d96`, premultiplied): five times `external_Color`, red for the ACOG's chevron and the PK-A, PK-AS and Kobra, black for the PSO-1 and PKS-07.

All of those come from one 512x512 BC1 texture with one mip level: a sheet of every sight's markings (the ACOG's scale with 4 and 6, its ruler, chevrons, the PSO's range curve with 2 to 10 and 1.7, arcs, digits). It's bound only while a sight is zoomed.

The first person's projection zoomed is always 7.596 (FOV 15), whatever the world's is: a mark at model x, a distance d from the zoomed eye, shows at x / d × 7.596 on the screen, which is x / d × 7.596 / cot(world FOV / 2) of slope in the world.

| Sight | Glass (unzoomed) | Middle, radius | World FOV | Power | Zoomed eye in its bone 0 | Lines | Marks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ACOG | 72/9285 | (0 0.032 -0.067), 1.1 cm | 17.2 | 3.44 | (0 0.0333 -0.2674), looking 0.0056 down | 516/6957 | 12/0 |
| PSO-1 | 72/0 | (0 0.033 -0.104), 1.4 cm | 17.2 | 3.44 | (0 0.0329 -0.2812) | 480/4713 | 54/4659 |
| PKS-07 | 57/0 | (0 0.035 -0.126), 1.4 cm | 9.9 | 6.0 | (0 0.0349 -0.2860) | 480/606 | 54/552 |
| PK-A | 48/0 | (0 0.029 -0.047), 1.6 cm | 20 | 2.95 | (0 0.0285 -0.2543) | | 12/210 |
| PK-AS | 60/0 | (0 0.0435 -0.056), 1.35 cm | 32 | reflex | (0 0.0434 -0.4359) | | 3360/0 |
| Kobra | 210/0 | (0 0.0405 0.064), 1.25 cm | 40 | reflex | (0 0.0402 -0.4236) | | 18/0 |

The power is the game's own zoom, against its unzoomed FOV of 55. The reflex sights' marks are on a bone of their own, further off than the sight, so they stay put as the head moves.

Other draws: 129/9357 and 108/9486 the ACOG's red fibre, inner and outer; 4689/0 and 4689/4689 the PSO-1's mount.

`tools/scopemarks.ps1` writes `src/scopemarks.h` from dumps of those parts. A dump of a named draw takes the first draw with those numbers, any soldier's, so `dumpown` for the player's own.

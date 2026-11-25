# The Saiga 12K in the hands

Measured 2026-11-18 on Operation Swordbreaker (the garage after the parking-lot fight, nobody in the headset; the owner's save
POSTPARKINGLOTFIGHTSAVE): `build\survey\SAIGA12K*` (draw scans zoomed and unzoomed, a part-used reload that first fires two rounds, its seven rounds
fired dry and the empty reload the game then starts itself, the mesh picture, the films), the body's mesh dump `build\mesh_14970_0_*` and the log
`build\logs\bf3vr_1118_sbmeasure.log`. Built from those and from the game's own files on 2026-11-25, offline. **Nothing of this has run in the game
yet**: everything under "In VR" is from the tables and the offline tests (`build\saiga12ktest.exe` holds the rows, which skin.cpp builds its own from
(`src\saiga12krows.h`), to the meshes and the recordings, 17 checks; `build\heldmagtest.exe` works its reload by hand through handmag on those very
rows).

## Why no magazine was drawn after the swap

The morning's measurement put the Saiga in the soldier's hands by the test switch (the M4A1's blueprint made the Saiga's, `held-guns` memory): the gun
was drawn, fired and reloaded, but no magazine was drawn at all, and the game's reload closed its hand on nothing (the films
`SAIGA12K_film_reload_*.png`). Neither of the guesses (a draw of its own not looked for, its mesh not streamed in) is the cause; both meshes are in
the level's own bundle (Levels/SP_Earthquake/Building's MeshVariationDb lists `saiga_20k_1p_Mesh` and `saiga_20k_regularmag_1p_Mesh`). What decides it
is in the gun's blueprint (`Weapons/SAIGA20K/Saiga_20K`, read from the game's own files):

- Its body mesh (`weapons/saiga20k/saiga_20k_1p_Mesh`, 14970/0) has no magazine: three bones, the bolt with its handle, the trigger, the frame.
- Its magazine is a **skinned socket object** of its own (`WeaponSkinnedSocketObjectData`: `weapons/saiga20k/saiga_20k_regularmag_1p_Mesh`), on the
  gun's skeleton's bones 7 (the magazine) and 8 (its shells), made by a `SocketData` keyed to an **unlock**. Six sockets make a magazine: the
  `ExtendedMag` unlock's the long one (`saiga_20k_extendedmag_1p_Mesh`, 1218/0, 11 rounds), and each of the five unlocks of its primary-accessory slot
  the regular one: `Weapons/Common/NoPrimaryAccessory`, `U_Saiga_20k_Flashlight`, `U_Saiga_20k_Targetpointer`, `U_Saiga_20k_Flashsuppressor`,
  `U_Saiga_20k_Silencer`. A Saiga with none of these has no magazine at all, in the game itself.
- The swap gave the Saiga's blueprint the M4A1's slot and with it the M4A1's unlocks: none of the six. So the game made no magazine, there was no draw
  of it to find, and its own reload animated the hand round nothing.
- Every real Saiga in the campaign carries one: the extremists' (`PLR_Extremist_SHG`'s kit: `AI_Unlock`, `NoOptics` or the Kobra, and
  `NoPrimaryAccessory`; its drop is `defaultaidynamicpickup`, which hands the player the soldier's own unlocks, as the AN-94's PKS-07 came with it),
  Kaffarov's kit (a flashlight and a laser), The Great Destroyer's guards (a laser). `NoPrimaryAccessory` is `UnlockAvailability_All`; `AI_Unlock`
  (which swaps in the AI's firing data: 200 m/s, 14 rounds) is `UnlockAvailability_AIOnly`, so the player's taken Saiga has the player's data (7 rounds,
  300 m/s). No campaign kit carries the extended magazine.

So in play the magazine is drawn: three draws of its own, from its own buffers, each on a palette of its own. The mod takes them as the held gun's
magazine (below); it draws only the game's own meshes, so with the swap (no magazine made) it has nothing to draw in the hand either.

## What the game draws

Places are on the gun's frame bone (2): x to its left, y up, z to the muzzle, metres.

| Draw | Key | Bones |
| --- | --- | --- |
| Colour | 14970/0 | 0 the bolt and its handle (on the right), 1 the trigger, 2 the frame |
| Depth | 14970/14970 | the same three in the same order |
| Its magazine, the box | 1176/0 (own buffers, stride 48) | its bone 0 (the skeleton's 7) |
| Its shells | 450/1176 (stride 48) | its bone 0 (the skeleton's 8) |
| Their depth pass | 1626/1626 (stride 16) | 0 the box (200 vertices), 1 the shells (77) |

- The gun: z -0.012 to 0.837, y -0.112 to 0.090; the gun lies 2.3 mm right of its frame's x 0 (the bore, the sights, the magazine all at x -0.0023).
  Muzzle's middle (-0.0023 0.026 0.8374) (the data's muzzle flash at z 0.839).
- **The magazine's seat** (its bone on the frame): (0 0.0989 0.3760), not turned. Not recorded (it was never drawn): taken from the game's own far LOD
  of the gun, `saiga_20k_3p_Mesh`'s LOD4, which has every part on the frame's bone and the magazine in it as a box: its bottom's back and front
  corners (y -0.1305, z 0.4353) and (y -0.1083, z 0.5117) are the magazine's own far box's (its LOD3, on its own bone: (y -0.2296, z 0.0594) and (y
  -0.2069, z 0.1356)) moved by (0.0991 0.3759) and (0.0986 0.3761), the slope of the two bottoms the same to 0.4 degrees. The receiver's opening bears
  it out: its back corners at y -0.0060 (x -0.0172 and 0.0123, z 0.4126) and the magazine's own back corners at the same height (x -0.0163 and 0.0117,
  z 0.4136): 0.6 to 1.0 mm clear all round, the same height in both meshes to the half float.
- The magazine at its seat: a straight polymer box of six, x -0.0176 to 0.0131, y -0.131 to 0.029 (its feed lips 3.5 cm up inside the receiver), z 0.403
  to 0.512, leaning forward (its back edge z 0.4073 at the receiver's underside, 0.4204 at its middle, 0.4331 near its bottom; 7.8 cm from back to front
  all down it). In its own vertices' space (-0.0176 -0.2301 0.0266) to (0.0131 -0.0697 0.1361).
- The paddle (the release) hangs behind the well, z 0.372 to 0.388, y -0.006 to -0.045, 2.5 cm behind the magazine's back.
- The bolt's handle: on the right, its knob's end x -0.0447, y 0.032 to 0.041, z 0.473 to 0.479. The bolt goes 9.1 cm back with each shot.
- Sights: iron, on the top rail: the rear notch's floor y 0.0842 (z 0.49 to 0.51, its ears 7.7 and 6.6 mm to the sides), the front post's top y 0.0843
  at z 0.7007: the line level at y 0.0843. Zoomed (FOV 40) the game's one sample has the eye at (-0.002 0.084 0.127) looking 0.05 degrees up: on it.
  The first of the gun within 1 cm of the line ahead of the eye: the rear sight, z 0.490.
- Ejection: `FX_ShellEjection_Shotgun` at (-0.01 0.041 0.44), turned 20 degrees about x and y (its data).
- Palms at rest (the mod's convention): right (-0.032 -0.080 0.193), left (-0.006 -0.034 0.589).

## What the game does

- **Hold:** right hand on the pistol grip, left under the handguard.
- **Reload, a part-used magazine** (`SAIGA12K_partial.bin`, from 3.08 s): the left hand goes down to the magazine and holds it low (its middle at (0.040
  -0.122 0.473), 0.9 cm above its bottom, still 3.34 to 3.47 s), rocks it forward and pulls it down and away; comes back with the fresh one from the left
  and below, hooks it, and pushes it home from under it with the palm (the hand under the magazine, still, 4.89 to 4.97 s); back on the handguard by 5.3 s.
  The count goes back to 7 at 4.67 s.
- **Reload, empty** (`SAIGA12K_dry.bin`, the game starts it 0.24 s after the last round): the same swap (the hand still on the seated magazine 6.03 to 6.20
  s), then over the top to the handle on the right (8.30 s), the bolt drawn 10.0 cm (8.42 to 8.49), held, let go: home 8.69 to 8.78. The count goes from
  0 to 6 (an empty reload leaves the chamber's round out, as its data's capacity of 7 is six and the chambered one).
- **Firing:** single fire, 220 a minute, 9 pellets at 300 m/s (buckshot; the flechettes 450). Each round, on its frame against where it rested just
  before it: turned up 6.69 degrees at 78 ms (6.47 to 6.72 on all nine rounds of both recordings, 47 to 94 ms), its origin 0.69 cm back at 250 ms as it
  settles (0.66 to 0.76): a shotgun's climb about its grip, as the M1014's (7.15 degrees).
- **Counts:** 7 loaded (6 and one chambered); its magazine holds 6.

## In VR

One row in each per-gun table (`src\saiga12krows.h`: its draws and bones, its seat, its sights, port, palms and eye row, its magazine's numbers for the
hand; from which `skin.cpp` builds kSaiga12kBones, its kHeldKinds and kHeldEyes rows; ClearRifleSpread row 15; `gunprofile.h`'s name row 16;
`recoilguns.h` by `tools\recoil\kick.tsv`; `gunbodies.h`; `shaperecipes.h`; `loose.cpp`; `magsound.h`; `gamesounds.cpp`). The block is the AK family's,
the one the owner approved on the AN-94 and AK-74M, on the Saiga's own numbers.

- **Its magazine, a mesh of its own.** Its three draws are the held gun's magazine: the box (1176/0) as a piece drawn apart (`HeldMagBones::partCount`, as
  the M249's pouch), its shells (450/1176) as its rounds, and their depth pass (1626/1626) through a new field, `HeldMagBones::partDepthCount` and
  `partDepth` (a magazine's own depth draw on its own palette: its bone 0 the magazine, 1 the shells; 0 for every other gun, whose depth pass is the
  body's): seats 0 to 3 as every gun has them, each learned from the game's own draw at rest (`seatrest.h`), so the shells sit where the game puts them
  (their bone's rest isn't known offline). Out, all three are hidden; in the hand, lying and falling, each is drawn again at the magazine's place, as every
  gun's magazine is. Moved with the gun as every draw after its body.
- **Hold:** by its own palms, either hand; slot 1's or slot 0's shoulder as its holster (by its slot, as every held gun).
- **Magazine by hand:** no button, X does nothing. The hand takes the seated one (grip or trigger on it) and rocks it forward off its front edge at the
  receiver's underside; a little over 5 cm down it is free. A fresh one from the belly goes home by itself within 10 cm of the well, hooked and rocked in
  25 degrees (the AK family's: the game's own rocks it, its angle not recorded). A fresh one swept into the old one's back at 1.3 m/s or more knocks it
  out (the Afghan flick); slower it only touches.
- **The hand on it:** the game's own hold on the seated magazine (3.34 to 3.47 s of the part-used reload: `magfit ... grip ... seated`, new: the stretch
  taken with the magazine seated on the gun's own frame, as the recording never drew it), raised to the middle of what's below the receiver and each
  finger closed onto the mesh: palm 2.0 mm off, every finger on it (its parts within 3.2 mm, none more than 1.5 mm in).
- **The handle:** on the right; grip or trigger near the knob closes the game's own hand on it (over the top, its last two fingers on the knob, as its
  empty reload has it at 8.29 to 8.41 s), or a fist from below when the palm faces up (fitted on the knob). Drawn half of its 10 cm and let go, it chambers.
  No hold-open: after every empty magazine the handle is drawn.
- **No reload by the game, the trigger stopped at empty:** by its recoil row's body and shape (the hidden round, `ammo.cpp`), as every held gun.
- **Recoil and buzz:** its own recorded kick (above): 0.69 cm back, the rise 6.42 degrees with the drift's answer to the recorded 6.69; the buzz from its
  data (0.34, 1.0 for 0.2 s); single fire, no held fire.
- **Bullets:** from the iron sights' line (level, y 0.0843 at the muzzle's z), 300 m/s (and 450 with flechettes) found in its data.
- **Scatter:** its zoomed rows written over its unzoomed ones once in hand (ClearRifleSpread row 15, its GunSwayData `SAIGA20K_GunSway`).
- **Casings:** the hulls stay the game's own effect (`FX_ShellEjection_Shotgun`), as the M1014's: the mod has no 12-gauge casing and doesn't know the
  effect's mesh. Its port row is the effect's place.
- **Sounds:** cut from its own empty reload, `saiga20k/saiga20k_reload_long_wave` (4.1 s), against the bones of `SAIGA12K_dry.bin` (the wave's clock from
  the hand leaving the handguard; its loudest, the bolt home at 3.01 s, at the bones' 8.69 to 8.78): `mag_out_saiga12k` 0.395-0.695 s (the paddle and the
  magazine rocked off), `mag_in_saiga12k` 1.725-2.00 (hooked, then rocked home with its clicks), `lever_saiga12k` 2.595-2.825 (the knob taken and the bolt
  drawn), `bolt_saiga12k` 2.855-3.11 (let go and home). Made at the next start (`gamesounds --missing`).
- **Body and shape:** `gunbodies.h` kSaiga12k (22 hull points, 3.5 kg empty, its magazine in it at its seat: `gunbodyfit`'s parts take a mesh of its own
  now, `Part::ownBuffer`; its magazine is its lowest part, 3.1 cm across its bottom, 6 mm beyond its grip's bottom on either side, so it has a keel width
  of its own, 7 mm (`Gun::keelWide`): its bottom's corners go onto the keel with a guard 5 mm up, as the AKS-74u's, and stood upright it tips over, never
  standing on a corner: bodytest b6, which failed with the fit's own 5 mm); the collision shape's recipe `14970 2 0` (the body without its bolt, its
  trigger kept), made on the player's PC as every shape (51 x 101 x 317 cells).
- **Dropped magazine:** a straight polymer box, 0.23 kg empty, 45 g a shell, 6: the hollow plastic's knock.

## What is forgiving

- The magazine goes home from 10 cm; the handle chambers from half its travel; either the grip or the trigger takes the magazine or the handle.
- The handle's travel is the game's own pull (10.0 cm), more than the bolt's 9.1 firing: a longer pull is never refused.

## Not measured, not checked

- **Nothing here has run in the game.** The hold, the magazine's three draws taken as the held gun's, the reload by hand, the trigger stop, the recoil,
  the holster and a pick-up are still to film.
- **The magazine is only there when the game makes it.** A real pick-up (an extremist's drop) has it; the swap of the morning has none, and the mod has
  nothing to draw then (no magazine in the gun or the hand: the reload by hand still works, unseen). A swap test needs the Saiga's socket for
  `NoPrimaryAccessory` (SocketData E902AD5C) made to name an unlock the checkpoint's kit carries, as the sight test did with the PSO-1, or the real drop.
- **The seat is the far LOD's and the opening's, not a recording's.** To 1 mm by both; the game's own draws set the seat the mod uses (seats learned at
  rest), so an error here moves only the fitted hand and the table's numbers, not the magazine. The first draws in play say it: "Held gun: the Saiga 12K's
  magazine's seat 0 taken from the magazine at rest on its frame"; a film of the hand on the seated magazine shows whether it hovers or sinks.
- **The shells' bone (8) at rest isn't known** (the morning's films have no magazine): placed by the game in play. Whether the game hides them with the
  magazine empty, as the AK-74M's rounds, only a film shows.
- **The rock angle** is the AK family's 25 degrees: the game's own reload rocks the Saiga's too, but its magazine's bone was never recorded.
- **Its depth pass's magazine** is the first held gun with one of its own (`partDepthCount`): its seats 2 and 3 come from it. Whether its draws come in
  the frame's own view after the body's (the mod's frame for a part drawn apart) as the M249's box does, only the game shows: the log's "seat 2 taken" and
  "seat 3 taken" lines, no "magazine not drawn" lines while one is in the hand, and a film of the magazine in the hand keeping still on the glove as the
  gun moves (drawn on a frame old by one, it would trail the gun by a frame's move).
- **Two magazines at once:** a Saiga with both a flashlight and a laser (Kaffarov's kit) has two sockets making the same magazine: two copies of each
  draw, in the same place. Both are taken and moved alike; not seen.
- **The extended magazine** (1218/0, 450/1218, 1668/1668) is not built: no campaign kit carries it.
- **Its scatter and shot data share the M1014's numbers** (the same standing block but its recoil's three, the same 300 and 450 m/s): with both loaded
  (The Great Destroyer) a scan for either may find the other's too and write the zoomed scatter and the sights' start into both, as the 600 m/s guns' do.
- **Casings:** the game's own hulls, from where the game holds the gun, not the hand's: the effect's mesh count is to find (weapon.txt `flashsurvey`),
  as the M249's links.
- **Bare arms:** The Great Destroyer's guards' Saigas are met with Blackburn's bare arms; there the magazine's seat comes from the magazine at rest
  (`seatrest.h`), as the AKS-74u's on the train. Not run.
- **Its attachments:** the Kobra and the PK-AS have their rows already; the IRNV, the laser and the flashlight are drawn as the game draws them, moved with
  the gun.
- **Sounds:** the cuts are by the wave's levels and the bones' times, not heard.

## In-game checks (nobody in the headset first, then the owner)

1. An extremist's Saiga on Swordbreaker (the bazaar's or the parking lot's drops), taken up by hand: "Free hands: awake (the Saiga 12K in hand)", "Recoil:
   the Saiga 12K set up from slot N", "Rifle: its bullets' start found in N places", "Rifle: its scatter's numbers found", "Held gun: the Saiga 12K's
   magazine's seat 0 taken ...", seats 1, 2 and 3 the same way, "Held gun: a magazine's own draw 1176/0: vertex shader ..." once.
2. The hold: both eyes, both hands on it, no arm stretched, the magazine in the well and its shells as the game draws them (and with it when it lies).
3. Rounds at a far wall over the irons: the pellets round the front post's top; the kick at the eye (about 6.4 degrees up a shot); the buzz.
4. Dry: the trigger stops, no reload by itself. The magazine taken by hand and rocked off (`mag_out_saiga12k`), a fresh one from the belly home by itself
   (`mag_in_saiga12k`), the handle over and under (`lever_saiga12k`, `bolt_saiga12k`): firing again. The flick: a fresh one swept hard into the seated
   one's back knocks it out; gently, it only touches. The count after each box: 6 and the chamber's.
5. `heldtest 1` and `heldtest 2` (a scripted left hand: out and in; the flick), `heldtest 3` and `4` (the handle over and from under), filmed.
6. The holster: stowed at its slot's shoulder, drawn by either hand.
7. Let go: it lies as its body has it, its magazine in it; the magazine let go of lands as hollow plastic and lies 30 s.
8. The sounds made at the first start (`build\gamesounds.log`: none failed).
9. A swap test, if one is wanted without a real drop: the socket for `NoPrimaryAccessory` made to name the M4A1's kit's own accessory unlock first (see
   above), then the magazine's three draws appear in a `drawscan`.

## How the tables were made

```
gamesounds --mesh weapons/saiga20k/saiga_20k_regularmag_1p_mesh --seat 14970@0 2 0 0.0989 0.376 --out build   (its three draws' dumps, new)
magfit build saiga.bmp 1176 2@14970@0 0 -0.006 Saiga12k src\magposes\saiga12k.h grip build\survey\SAIGA12K_partial.bin 3340 3470 seated   (new: seated)
magfit build lev.bmp 14970 2 0 0.03 Saiga12kLever src\magposes\saiga12k_lever.h vanilla build\survey\SAIGA12K_dry.bin 8290 8410
magfit build und.bmp 14970 2 0 0.03 Saiga12kLeverUnder src\magposes\saiga12k_lever_under.h under -0.044 0.036 0.476
gunbodyfit build src\gunbodies.h
gunshape build build\guns\saiga12k.bin 0.003 14970 2 0        (then shapemaketest build rest, for its row in shaperecipes.h)
powershell -File tools\recoilgen.ps1                          (tools\recoil\kick.tsv: recorded 0.69 250 6.69 78, from SAIGA12K_dry.bin)
gamesounds --sounds mag_out_saiga12k --sounds mag_in_saiga12k --sounds lever_saiga12k --sounds bolt_saiga12k
saiga12ktest                                                  (the rows against the meshes, the recordings, the tables, the cuts)
```

The seat's numbers: the 3P meshes' LODs (`saiga_20k_3p_Mesh`'s LOD4, `saiga_20k_regularmag_3p_Mesh`'s LOD3) read from the game's own files as the
MeshSet and its chunk (the reader `gamesounds --mesh` uses; LOD0 alone is written as dumps). (`magfit` writes a `.start` file beside its header: not
kept in `src`.)

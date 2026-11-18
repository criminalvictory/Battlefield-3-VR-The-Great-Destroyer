# The UMP-45 in the hands

Measured 2026-11-18 on Semper Fidelis (the train), with nobody in the headset: `build\survey\UMP45_*` and the mesh dumps `build\mesh_19314_234_*`, `234_0`, `90_19548`. Built from those the same day, offline. **Not yet run in the game**: everything under "In VR" is from the tables and the offline tests.

## What the game draws

| Draw | Key | Bones |
| --- | --- | --- |
| Rounds | 234/0 | 0 the rounds (first in the buffer) |
| Colour | 19314/234 | 0 magazine, 1 the cocking handle, 2 frame |
| A strip along the sights' line | 90/19548 | 0 is the frame (x +-4 mm, y 0.092 to 0.095, z 0.29 to 0.59) |
| Depth | 19638/19638 | 0 the rounds, 1 magazine, 2 handle, 3 frame: its own order |

234 is also the index count of the M416's casing mesh. Those are instanced draws and go another way (`effects.cpp`); the rounds are a plain skinned draw, known only with the UMP-45's body drawn.

On the frame (bone 2):

- The gun: z 0.011 to 0.648, 5.6 cm across, y -0.105 to 0.102.
- Magazine: straight, x +-1.45 cm (its base plate +-1.9), y -0.2125 to 0.027, leaning 10.6 degrees forward: z 0.416 to 0.456 at the top, 0.456 to 0.500 at the bottom. 5.4 cm of it are inside the well (the well's mouth at y -0.027). In its own vertices' space: (-0.019 -0.309 0.074) to (0.019 -0.067 0.127).
- The cocking handle: on the **left**, over the handguard. A tube along z from 0.486 to 0.605 with the knob standing 2 cm out at z 0.565 to 0.58: its end at (0.026 0.061 0.5715).
- Muzzle: (0 0.033 0.648).
- Sights: the zoomed view sits at (0 0.097 0.080) looking 0.05 degrees down (FOV 40); at the muzzle's distance the line is at y 0.0965. The nearest of the gun on that line: the rear sight at z 0.271.
- Ejection: `FX_ShellEjection_45cal` at (-0.02 0.034 0.42), turned 45 degrees about y, 60 zoomed (the public Venice-EBX dump of `Weapons/UMP45/UMP45`); the hole in the right side of the mesh is at z 0.39 to 0.455.
- Palms at rest (the mod's convention): right (-0.053 -0.066 0.223), left (0.004 -0.026 0.538), ahead of the well.

## What the game does

- **Reload, a part-used magazine:** the left hand takes the magazine and draws it **straight down** about 22 cm, with no rock, then tilts it away; the fresh one comes straight up, seated 1.3 s after the hand first moved.
- **Reload, empty** (from R): 0.41-0.55 s the left hand draws the handle back 8.3 cm; 0.58-0.66 s it turns it **51.8 degrees up** into its notch, about the gun's z through x 0, y 0.0564, where it rests 6.55 cm back; 1.09-2.08 s the magazine out and the fresh one in, the handle staying in its notch; 2.30-2.40 s the hand **knocks the handle down** (it turns down where it rests, then flies home).
- **Fired dry:** nothing holds the handle or shows a bolt held open; the handle moves only in the reload.
- **Firing:** its kick on the frame bone: 1.76 cm back at 47 ms, 1.98 degrees up at 78 ms; held, 2.33 cm back on average over 26 rounds.
- **Counts:** 26 loaded (25 and one chambered), 600 rpm, bullets at 320 m/s.

## In VR

The G3A3's block where the two truly match (a no-button magazine, the handle forward on the left, drawn by the game's own hand), and two things of its own, both in `src/handmag.cpp`:

- **A straight well (`Gun::straight`).** No button: X does nothing. The hand takes the seated magazine and slides it straight down the well; it is free once its top is out (5.4 cm and the grab's slack: about 6 cm). The G3A3's is rocked off a hook; this one never rocks. A fresh one goes home by itself within 10 cm of the well, straight up.
- **The notch (`Gun::leverNotch`, `leverNotchAxis`, `leverNotchBack`).**
  - Grip or trigger at the knob closes the game's own hand on it (its last two fingers hooked on the knob).
  - Drawn back and let go: it springs home and chambers a round from half its 8.3 cm, as any lever.
  - Drawn back at least 85% of the way **and the hand then lifted 2 cm at the back** (the game's own hand rises 0.5 cm during the pull and 3.3 cm after it, as it turns the handle): the handle turns up into its notch in 0.06 s and leaves the hand. The lift is measured from the hand's height where the pull got that far, not from where it took the handle, and while the hand still goes further back that height follows its rise (up to 20 degrees): a pull that rises 10 or 15 degrees as it goes, or one taken 4 cm under the knob, chambers and never notches. It rests 6.55 cm back, turned 51.8 degrees up; the bolt is held open and nothing fires ("the chamber is empty: pull the lever").
  - **Knocked down:** a hand coming down on it (its middle within 8 cm of the knob, at or above the knob's height, moving down the gun at 0.3 m/s or more), or a fresh squeeze or trigger there. The handle turns down and springs home; with a magazine in that has rounds, a round is chambered.
  - The hand that turned it up does not knock it out on its way down to the magazine: a hand has to have been 10 cm clear of it (or unseen) once. A jump of the tracking (over 8 m/s) is no knock.
  - The game's own reload (X held) or a new life sends it home.
- **No knock-out by a fresh magazine** (`noKnock`): the game's reload never does it, the well is deep and straight, and the UMP is not on the user's list of guns for it; the catalogue lists it as a decision owed. Its back edge is measured, should it be wanted: {y, z} (0.017 0.417), (-0.093 0.436), (-0.2025 0.4565).
- **No reload by the game, the trigger stopped at empty, recoil and buzz:** by its recoil row (its recorded kick; buzz 0.34, 0.8, 0.2 s). The UMP-45's row is also the class default for SMGs known only by their kind: that default now has the recorded numbers, not the placeholder's.
- **Sounds:** cut from `ump/ump_reload_1p_full_wave`: `lever_ump45` 0.345-0.505 s, `notch_ump45` 0.515-0.665 (the turn into the notch), `mag_out_ump45` 0.995-1.16, `mag_in_ump45` 1.895-2.03, `bolt_ump45` 2.255-2.48 (the knock and the bolt home).
- **Body and shape:** `gunbodies.h` kUmp45 (22 hull points, 2.5 kg), `build\guns\ump45.bin`. Its body fit keeps points 1.2 cm apart instead of 1.5 (`gunbodyfit.cpp`'s `merge`): the wide base plate's left corner lies 1.37 cm from the keel's point and was merged away, so lying on its left side the magazine went 13 mm into the ground.
- **Loose magazine:** straight, polymer: 0.17 kg empty, 21 g a round, 25.
- **Casings:** the nearest the mod's own casings have is the 9 mm (a .45's case is 2 mm wider); the effect's own emitter was not read, the pistol's throw stands in.

## What is forgiving

- The handle works as a plain lever for anyone who never lifts it: the notch asks for a clear lift at the back.
- The knock takes any downward hand near the knob, or a squeeze; the palm need not hit the knob itself.
- The magazine goes home from 10 cm.

## Not measured, not checked

- **Nothing here has run in the game**, the notch least of all: `weapon.txt` "heldtest 9" plays it with a scripted hand (drawn back, lifted, the hand away, then down on it).
- **The arms on the train are bare** (18108/16053), which the held-gun path does not pose: on Semper Fidelis the left hand on the gun is expected to be the game's own, not the player's glove, until the bare arms are given to the held guns. The held guns learn the game's palms from the gloves' draw alone (`LearnRig`), and with gloves a magazine's seat is taken only while those rest on the gun. Bare arms have no gloves' draw, so there the magazine says it itself (`src/seatrest.h`): its place on the frame unchanged for half a second is its seat; in the game's own reload it is never still on the frame that long (the three recorded reloads replayed by `tools\seatresttest.cpp`: no seat taken from a magazine on its way, the seats of all three recordings the same to 0.1 mm). With gloves nothing of this is asked. Not run in the game. A level with gloves shows the gun as built (fallen Marines leave UMP-45s in Operation Guillotine; in Comrades the arms are Dima's, another set again).
- Whether a lift of 2 cm at the back is easy to do and hard to do by accident: only real hands say. If it locks by accident, raise `kNotchLift`; if it is hard to find, lower it or let it go in at the end of the pull by itself.
- The real gun holds its bolt open after the last round on a catch; the game shows none of that, and the mod follows the game.
- The middle four groups of its GunSwayData are from the public dump of the multiplayer build. Its crouched and prone rows are alike (1, 7, 0.1, 15, 1.25, 7, 0.1, 15): the scan takes the first for the crouch and the next for prone.
- Its bullets' start is found by their speed, 320 m/s, which only the MP-443 Grach shares (Kaffarov's kit): with both loaded, the UMP-45 in hand writes its sights' line into the pistol's shot data too, as the 600 m/s guns already do to one another.
- The casing's mesh (its index count) for the .45 was not measured.
- Its burst mode, and its sights on Comrades (RX01, EoTech), are not looked at here.
- The sound cuts are by the waves' levels and the bones' times, not heard.

## How the tables were made

```
powershell -File tools\baregloves.ps1 -In build\survey\UMP45_partial.bin -Out build\survey\UMP45_partial_gloves.bin   (and _empty)
magfit build ump.bmp 19314@234 2 0 -0.027 Ump45 src\magposes\ump45.h grip build\survey\UMP45_partial_gloves.bin 3200 3800
magfit build lev.bmp 19314@234 2 1 0.05 Ump45Lever src\magposes\ump45_lever.h vanilla build\survey\UMP45_empty_gloves.bin 380 570
gunbodyfit build src\gunbodies.h
gunshape build build\guns\ump45.bin 0.003 19314@234 2 0,1
powershell -File tools\recoilgen.ps1
gamesounds --sounds lever_ump45 --sounds notch_ump45 --sounds mag_out_ump45 --sounds mag_in_ump45 --sounds bolt_ump45
```

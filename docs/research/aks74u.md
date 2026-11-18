# The AKS-74u in the hands

Measured 2026-11-18 on Semper Fidelis (the train), with nobody in the headset: `build\survey\AKS74u_*` (zoomed and unzoomed draw scans, a part-used reload, an empty one, the real empty reload fired dry, the bones' tracks) and the mesh dumps `build\mesh_17991_0_*`, `72_17991`, `894_18063`, `312_18957`. Built from those the same day, offline. **Not yet run in the game**: everything under "In VR" is from the tables and the offline tests.

## What the game draws

| Draw | Key | Bones |
| --- | --- | --- |
| Colour | 17991/0 | 0 the bolt and its handle, 1 the trigger, 2 frame, 3 magazine |
| The magazine's other piece | 894/18063 | 0 is the body's bone 3 |
| Rounds | 312/18957 | 0 the rounds, at the magazine's top |
| A strip along the sights' line | 72/17991 | 0 is the frame (x +-4 mm, y 0.074, z 0.40 to 0.63) |
| Depth | 19269/19269 | as colour, 4 the rounds |

On the frame (bone 2; x left, y up, z to the muzzle):

- The gun: z 0.005 to 0.710, 4.3 cm across, y -0.111 to 0.078.
- Magazine: x +-1.45 cm, y -0.179 to 0.011, z 0.373 to 0.492, curving forward. Its back edge: z 0.378 at the top, 0.390 at y -0.084, 0.428 at y -0.169. In its own vertices' space: (-0.0144 -0.278 0.077) to (0.0146 -0.088 0.196).
- The receiver's underside over the well: y -0.003.
- The bolt's handle: on the right, its knob's end at (-0.037 0.036 0.4715). The bone goes back 9.4 cm with each shot; the game's own hand draws it 6.4 to 6.8 cm.
- Muzzle: (0 0.018 0.710).
- Sights: the zoomed view sits at (0 0.074 0.127) looking 0.42 degrees **up** (FOV 40): the front post's top (y 0.0784 at z 0.62) stands 1.7 mm higher than the rear notch (0.0767 at z 0.39). At the muzzle's distance the line is at y 0.0783. The nearest of the gun on that line going forward from the eye: the rear sight at z 0.386 (the receiver's cover lies 15 mm under the line).
- Ejection: the game's shell effect for it is `FX_ShellEjection_M_Russian` at (-0.0142 0.0394 0.4108), no turn (the public Venice-EBX dump of `Weapons/AKS74u/AKS74u`). The bolt shows through the receiver's right side at z 0.39 to 0.47, y 0.032 to 0.052.
- Palms at rest (the mod's convention: the right glove's root bone, the left's palm bone): right (-0.031 -0.080 0.193), left (0.003 -0.035 0.517), just ahead of the magazine. The right is the AK-74M's own to a millimetre.

## What the game does

- **Hold:** right hand on the pistol grip, left hand palm up under the short handguard.
- **Reload, a part-used magazine** (R at about 2.4 s of the recording): the left hand goes down to the magazine, rocks it out forward about 20 degrees, carries it down and away to the right; the same bone comes back as the fresh one, rocked in at 26 to 33 degrees, seated 1.4 s after the hand first moved. No flick: nothing knocks the old one out.
- **Reload, empty:** the same, then the left hand goes over the top of the gun to the handle on its right, draws it 6.4 cm and lets go. No bolt hold-open.
- **Firing:** bone 0 cycles with every shot. Its kick, on the frame bone against where it rested just before the round (as the nine guns' were read): 1.91 cm back at 47 ms, 2.17 degrees up at 78 ms; over 31 rounds held it sits 2.45 cm back on average.
- **Counts:** 31 loaded (30 and one chambered), 650 rpm, bullets at 440 m/s.

## In VR

One row in each per-gun table, no new mechanics code: the AKS-74u is an AK, and the AK family's block is the one the user approved on the AN-94 and AK-74M.

- **Hold:** by its own palms above, either hand (the free-hands foundation: the hand that takes it is the trigger hand, the other holds the handguard at the left palm's place, mirrored for a left-hander).
- **Magazine:** no button, X does nothing. The hand takes the seated one (grip or trigger on it) and rocks it off its front hook; a little over 5 cm down it is free. A fresh one from the belly goes home by itself within 10 cm of the well, rocked in 25 degrees as the AK family's. A fresh one swept into the old one's back edge at 1.3 m/s or more knocks it out (the tactical reload); slower, it only touches.
- **Bolt handle:** grip or trigger near the knob closes the game's own hand on it (over the top, as its empty reload has it), or a fist from below when the palm faces up. Drawn half of its 9.4 cm and let go, it chambers. Run dry, the trigger is stopped until a magazine is in and the handle has been drawn.
- **No reload by the game, the trigger stopped at empty:** by its recoil row's body and shape (the hidden round, `ammo.cpp`).
- **Recoil and buzz:** its own recorded kick (above); the buzz from its data (0.4, 0.6, 0.15 s).
- **Sounds:** cut from its own empty reload, `aks74u/aks74u_reload_1p_long_wave`: `mag_out_aks74u` 0.345-0.52 s, `mag_in_aks74u` 1.565-1.70, `lever_aks74u` 2.185-2.365, `bolt_aks74u` 2.375-2.55.
- **Body and shape:** `gunbodies.h` kAks74u (14 hull points, 2.7 kg), `build\guns\aks74u.bin` (the magazine and the bolt left out).
- **Loose magazine:** the AK-74's own 5.45 magazine (polymer: 0.23 kg empty, 10.2 g a round, 30).
- **Holster, pick-up:** nothing per gun: known to the hands, it goes to its slot's shoulder and is taken off the floor as any held gun.

## What is forgiving

- The magazine goes home from 10 cm; the handle chambers from half its travel; either the grip or the trigger takes the magazine or the handle.
- The handle's travel is the bolt's own 9.4 cm, not the 6.4 cm the game's hand draws it: a longer pull is never refused.

## Not measured, not checked

- **Nothing here has run in the game.** The hold, the reload, the trigger stop, the recoil, the holster and a pick-up are still to film.
- **The arms on the train are bare** (one draw, 18108/16053), which the held-gun path does not pose: it moves them with the gun as the game holds them. So on Semper Fidelis and The Great Destroyer the left hand on the gun is expected to be the game's own, not the player's glove, until the bare arms are given to the held guns. The held guns learn the game's palms from the gloves' draw alone (`LearnRig`), and with gloves a magazine's seat is taken only while those rest on the gun. Bare arms have no gloves' draw, so there the magazine says it itself (`src/seatrest.h`): its place on the frame unchanged for half a second is its seat; in the game's own reload it is never still on the frame that long (the three recorded reloads replayed by `tools\seatresttest.cpp`: no seat taken from a magazine on its way, the seats of all three recordings the same to 0.1 mm). With gloves nothing of this is asked. Not run in the game. This is the first thing to look at in a film; a level with gloves shows the gun as built (the AKS-74u drops on Operation Swordbreaker).
- The hands' tables were fitted from the bare arms' bones under the gloves' numbers (`tools\baregloves.ps1`); the two skeletons' rests agree to about a centimetre.
- The fit of the hand on the magazine went by the body draw's part of it (its outline); the other piece's ribs stand 3 mm further out on the left side.
- The middle four groups of its GunSwayData (jumping, sprinting, the two vaults) are from the public dump of the multiplayer build, not read from the level's memory. If they differ, the log says "no scatter's numbers left to change" and the gun keeps the game's unzoomed scatter.
- Its attachments on other levels (Uprising's suppressor, Kobra and light; Comrades' PK-A) are not looked at here. The Kobra and PK-A rows exist already.
- The sound cuts are by the waves' levels and the bones' times, not heard.

## How the tables were made

```
powershell -File tools\baregloves.ps1 -In build\survey\AKS74u_partial.bin -Out build\survey\AKS74u_partial_gloves.bin   (and _empty)
magfit build aks.bmp 17991 2 3 -0.003 Aks74u src\magposes\aks74u.h grip build\survey\AKS74u_partial_gloves.bin 3250 3800
magfit build lev.bmp 17991 2 0 0.03 Aks74uLever src\magposes\aks74u_lever.h vanilla build\survey\AKS74u_empty_gloves.bin 2150 2480
magfit build und.bmp 17991 2 0 0.03 Aks74uLeverUnder src\magposes\aks74u_lever_under.h under -0.037 0.036 0.4715
gunbodyfit build src\gunbodies.h
gunshape build build\guns\aks74u.bin 0.003 17991 2 3,0
powershell -File tools\recoilgen.ps1
gamesounds --sounds mag_out_aks74u --sounds mag_in_aks74u --sounds lever_aks74u --sounds bolt_aks74u
```

(`magfit` writes a `.start` file beside its header: not kept in `src`.)

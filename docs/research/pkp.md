# The PKP in the hands

Measured 2026-11-18 on Operation Swordbreaker (nobody in the headset): `build\survey\PKP*` (draw scans zoomed and unzoomed, a part-used reload that
first fires two bursts (`PKP_partial.bin`, 746 frames), an empty reload (`PKP_empty.bin`, 1152 frames), films of both reloads, the mesh picture) and the
mesh dumps `build\mesh_23298_2310_*` and `2310_0`. Built from those offline on 2026-11-25. **Nothing of this has run in the game yet**: everything under
"In VR" is from the tables and the offline tests (`build\pkptest.exe` holds the rows, which skin.cpp builds its own from (`src\pkprows.h`), to the dumps,
the shape and the recordings, measures the fitted hands against the meshes and the gun's shape, and works the whole reload through handmag on the PKP's own
table, 61 checks).

The model is the M240's belt reload the owner approved (2026-11-05: "Oh yeah, that's super good. I like it.") and the M249 built on it; the catalogue's
plan for the PKP (`weapons-catalog.md`): the M240's blocks mirrored, the feed on the right and the cases out to the left, the support hand reaching across.

## What the game draws

Places are on the gun's frame bone (7): x to its left, y up, z to the muzzle, metres.

| Draw | Key | Bones |
| --- | --- | --- |
| The belt | 2310/0 | 0-4 its five pieces, **numbered from the box**: 4 across the feed tray (its free end), 3 to 0 down the receiver's right side, 0 into the box |
| Colour | 23298/2310 | 0 the box, 1 the cases' port flap (its left), 2 the feed arm on the cover's right, 3 the part under the cover, 4 the charging handle (its right), 5 the links' port flap (the cover's left), 6 the trigger, 7 the frame, 8 the feed cover |
| Depth | 25608/25608 (= 2310 + 23298) | 0-4 the belt, 5-13 the body's 0-8 (the box 5, the handle 9, the frame 12, the cover 13) |

The game draws them in each view as: depth, the gloves (13755/15051), the body, the belt. The box is a bone of the body (as the M240's is), not a draw
of its own (as the M249's pouch is). Its recordings have the M249's unknown 96/0 too (892 and 924 frames) and a few small draws (1236/432, 24/0,
90/696) with the gun's bones; what they are isn't known.

- The gun: z 0.0045 to 1.2139 (1.209 m). Muzzle (0 0.0466 1.2139). Vertices: 4696 on the frame, 1464 the box's, 1758 the cover's, 365 under it, 148
  the links' flap's, 138 the feed arm's, 126 the handle's, 104 the cases' flap's, 40 the trigger's.
- **The box:** a steel box **29 cm across the gun**, x -0.136 to 0.154, y -0.186 to -0.002, z 0.503 to 0.599 (9.6 cm from back to front), a bail on its
  right end; its top under the gun at y -0.0197. Its bone 0 on the frame at (0 0.0761 0.4901), turned 0.028 degrees; in its own vertices' space
  (-0.136 -0.262 0.013) to (0.154 -0.0782 0.1094).
- **The handle:** on the right (x -0.055 to -0.020), its knob x -0.055 to -0.047, y 0.010 to 0.026, z 0.475 to 0.488 (middle (-0.0509 0.018 0.4814)).
- **The feed cover:** x -0.056 to 0.026, y 0.043 to 0.097, z 0.279 to 0.622; it turns about x alone through (y 0.0708, z 0.6155), its front end. It is
  **four bones the game turns together**: 8 the cover, 3 under it and 5 the links' flap on its left (with it to 0.10 and 1.29 mm or degrees all
  through the empty reload), and 2 the feed arm on its right, which goes with it as it opens (0.2 degrees off it, the median; 9 as it pops) but has a
  turn of its own besides: up to 39 degrees off the cover as the handle is drawn (the cover shut, 0.6-0.85 s; back and forth up to 18 as it goes
  home, still by 1.6 s), 37 to 59 once the cover has popped open (2.1-3 s), 42 while it rests open (3-5.1 s), up to 95.3 as it's slammed shut (5.27
  s), then once more to 32 (5.6 s), still from 5.77 s.
- **The belt laid** (the empty reload's 4.61 to 15.99 s, 821 frames within 0.4 mm and 0.2 degrees): its free end's piece across the tray, its middle
  at (-0.0200 0.0576 0.5557); its last piece's end into the box at (-0.0799 -0.0606 0.5541), by the box's top on the right. Pieces of 111, 220, 221,
  219 and 435 vertices.
- **Sights:** iron. The front post's tip (0 0.1124 1.125) in a round hood whose ears stand to y 0.1167. **No rear sight stands on the line:** the
  cover's top lies 1.64 cm under it at its nearest, the carrying handle 1.35 cm off it. Zoomed (FOV 40) the game's eye (its one sample, the view in each
  of the six bones agreeing to 0.6 mm) is at (0 0.1148 0.1588) looking 0.18 degrees down and 0.07 right, on the post's tip to 0.5 mm: the line from
  that eye through the tip lies 0.14 degrees below z and meets the muzzle's plane at y 0.1122. The first of the gun within 1 cm of that line ahead of
  the eye is the front sight's hood (z 1.118).
- **Its effects** (the public Venice-EBX dump of `Weapons/Pecheneg/Pecheneg`): the cases `FX_ShellEjection_762` (the G3A3's own) at (0.0204 0.0094
  0.4796) on its LEFT, at its flap (bone 1), turned (10.6 -1.2 -155) and not turned again zoomed: out to its left and a little down; the links
  `FX_BeltLinkEjection_M` at (0.0225 0.0691 0.5582), at the links' flap on the cover's left (bone 5). Bullets 560 m/s (the M60's and the M82A3's too,
  neither on a level where it's taken), 600 a minute, 100 in the box (`U_Pecheneg_ExtendedMag`'s 200: no campaign PKP carries it), its GunSwayData
  95D41E17.
- Palms at rest (the empty reload's rest, 620 frames): right (-0.054 -0.112 0.251), left (0.015 -0.073 0.715).

## What the game does

- **Hold:** right hand on the pistol grip, left under the handguard.
- **Reload** (one animation; the empty one recorded, its moments from the recording's first frame; the wave 6.53 s). **All of it is the right hand's**:
  the left stays on the handguard (within 0.8 mm) all through it.
  - 0.58-0.70 s the handle drawn 14.5 cm straight back (0.04 mm aside, 0.07 degrees); 0.84-1.36 s it rides home with the hand.
  - 1.6-1.77 s the hand on the cover's top; 1.75-2.02 s the cover up, to 98.5 degrees at most as it pops open, settling at 93.0 (the median of 226
    frames past 80) to 5.1 s.
  - 2.56-2.78 s the box off to the gun's **right** (x falling: the handle's side): its left end slides level while its right end tips down (8.2 cm
    along at 2.67 s, turned 19.5 degrees), then it drops away right and down, turned 27.5 degrees, 32 cm off by 2.78 s. The game's right hand holds it
    by the bail on its right end (2.3-2.56 s, its middle out at x -0.21 to -0.22, beyond the box's end) and pulls it right and down (its middle at x
    -0.33, y -0.20 by 2.70 s).
  - 3.70-3.99 s the same box back from the right and below: turned up to 42.7 degrees as it comes, then from 8.6 cm off its seat (6.7 cm to the right,
    5.0 cm below), turned 32.7 degrees, swung up into its seat in 0.094 s.
  - 3.99-4.05 s its belt hanging out of it (bones 0 to 3 standing on the right, turned 19 degrees, the tip turned over); 4.05-4.50 s the hand takes it
    and lays it in the tray; the count 100 at 4.59 s.
  - 5.11-5.24 s the cover slammed shut.
- **The belt's pieces** are never hidden by the game's count (0 squashed in both recordings).
- **Its count:** an open bolt's. The part-used box went 92 to 100 with 8 from the spare ones, the empty one 0 to 100: no chambered round counted.
- **Firing:** automatic, 600 a minute. Two bursts of 4 rounds from rest (`PKP_partial.bin`), on its frame against where it rested the frame before each:
  the first round 1.82 cm back at 47 ms and 2.48 degrees up at 63 ms, where the second round's count comes; the second burst's first round 1.75 cm and
  2.98 degrees, against a frame still coming back from the first (not taken). Held fire 1.1 to 2.6 cm back, 1.89 on average over each burst's held
  stretch. The handle never moves firing (an open bolt's).

## In VR

The M240's block on the PKP's own numbers, mirrored: one row in each per-gun table (`src\pkprows.h`: its belt, its box and its way, its draws and their
bones, its sights, port, palms and eye row, from which `skin.cpp` builds kPkpBones, its kHeldKinds and kHeldEyes rows and handmag's; ClearRifleSpread row
15; `gunprofile.h` row 16, "PKP"; `casings.cpp`; `loose.cpp`; `gunbodies.h`; `shaperecipes.h`; `recoilguns.h` by `tools\recoil\kick.tsv`; `gamesounds.cpp`;
`pickrules::kOpenBolts`). What's the PKP's alone:

- **Fed from the right.** Its belt's bones are numbered from the box (`handmag::BeltGun::reversed`, `BeltOrder`): every table stays by the draw's own
  bones (hanging, laid, pieces); only which is the free end the hand pinches (bone 4) and which goes into the box (bone 0) turns round. In the hand the
  belt bows out to the side its box lets it out (the anchor's: the PKP's right; the M240's and the M249's left, bit for bit as before); it shortens as it
  fires from the box's end. `heldtest 7` brings the belt to the feed from its right.
- **The box goes off and on along its own reload's way** (`handmag::BoxWay`, `src\magposes\pkp_way.h` from `tools\boxway.cpp`, which now reads a box
  that is a bone of the body's own draw). Off: 17 rows, 31 cm, free at **8.24 cm** along: as far as its left end slides level in the game's own reload
  before it drops away (the grab point 7.9 cm to the right and 6.8 mm up, the box turned 19.5 degrees, its right end down). Boxway's own rule (the last
  frame within 1 cm of the seat's height) would free it at 2 cm, its right end already going down by then: so it's given. On: 7 rows over 0.094 s, from
  8.6 cm off its seat (6.7 cm to the right, 5 cm below), turned 32.7 degrees, swung up into the seat. The magnet takes a fresh box within 10 cm of where
  its way on starts; `heldtest 7` brings it 3 cm further out on the side it comes in from (now the PKP's right; the M249's left as before).
- **Taken anywhere along it** (`handmag::Gun::wide`, from the fitted hold's place, 2 cm beyond its left end, to its bail at its right end, grabTo
  -0.136). A 29 cm box turning 19.5 degrees as it comes off sends its bail end 10 cm down while its left end slides 8 cm, so a box taken away from its
  end (held where the hand took it) now follows the nearer of two ways (`handmag.cpp` WayAlong): the grab point's (as the game's own hand draws it) and
  the held point's own (as the box turns there). Drawn as the box turns, the glove stays on the hand (0.7 cm off in its middle, 0.8 by its bail); drawn
  level, it still comes free after the hand's 8.8 cm, the glove drawn down with the box's turning end (3.9 cm in its middle, 7.6 by its bail). Taken at
  the fitted place, only the grab point's, as before; the M249's numbers in m249test come out byte for byte as before.
- **The cover is four bones** (`HeldMagBones::lidAlso`, in the colour and the depth passes): the hand turns 3, 5 and 2 with the cover's 8 (every other
  gun has none: bit for bit as before). Lifted by `lidride`'s turn to 93 degrees they sit as the empty reload's resting open to 0.14 mm and 0.06 degrees.
  The feed arm goes with the cover rigidly, as the game's own does while the cover opens; the game's then turns it its own way (42 degrees off the
  cover while it rests open, 39 as the handle is drawn, up to 95 as the cover is slammed shut): not done by hand (below).
- Nothing knocks the box out (`noKnock`); no magazine button; no bolt catch.

So, by hand (the free left hand, from the gun's right):
- **The cover:** taken on its top (the left hand's middle at (0.0237 0.1088 0.3506)) with the grip or the trigger, lifted; past its share it goes up
  by itself to 93 degrees (`lid_latch`, `lid_up`). Brought down past its share it slaps shut by itself (`lid_shut`).
- **The box:** only with the cover up. Taken anywhere along it and drawn to the right it comes free (`mag_out`): along its way after the hand's 9.0 cm
  (8.24 and the hand's 6 mm of dead give), level after 8.8 cm, 15 degrees down after 9.6 cm (the glove 3.5 cm off the hand), 30 degrees down (30 off
  its way) after 11.6 cm (the glove 6.8 cm off). Straight or steeply down (60 degrees and more) it stays in once the hand is 8 cm off its way, the glove
  eased back to the hand, never drawn further than 8 cm from it. Let go, it falls (steel: 0.6 kg empty, 26 g a round; metal's knock) and lies 30 s.
- **A fresh box** from the belly: brought by its seat's right and below, it goes home by itself along its way on (`mag_in`), its belt hanging out of it.
- **The belt:** pinched at its hanging end (at (-0.111 0.005 0.558), on the box's right side) (`belt`), carried in the hand as a curve back to the box
  bowing out to the gun's right (2.6 cm with the hand 3 cm over where it comes out of the box), taken by the tray within 10 cm of it with the cover up
  (`belt_lay`); let go of before, it hangs again. It's short: its tip hangs 10.5 cm from the feed, so the tray takes it as soon as it's brought towards it.
- **The handle:** on the right, 14.5 cm, the left hand reaching over the top or from under it (each fitted on its knob, chosen by which way the palm
  faces as it's taken; with the cover up the handle, not the box); drawn half way and let go, a round is chambered (`lever`, `bolt`). The trigger stops
  at empty with no reload by the game; nothing fires until the box is in, the belt laid, the cover shut and the handle pulled.
- **The belt shortens as it fires**, by the true count, from the box's end.
- **Recoil and buzz:** its own recorded kick (1.82 cm back; the rise 2.21 degrees, which with the drift's answer is the recorded 2.48; held fire 1.89
  cm, 1.04 of a round's push), the drift and the buzz from its data (1.0, 1.0 for 0.112 s).
- **Bullets** from the iron sights' line ((0 0.1122 1.2139), 0.14 degrees below z), 560 m/s found in its data (`IsBulletSpeed`); its scatter its own
  GunSwayData's (ClearRifleSpread row 15, 0.12 a shot as the G3A3's and the M240's).
- **Casings:** 7.62 brass (its 54R case is 1 mm longer: the table's nearest) from its port on the LEFT (0.020 0.009 0.480), the _762's throw turned 155
  degrees about z: out to its left and a little down.
- **Its count:** an open bolt (`pickrules::kOpenBolts`, by its shape: {"pkp", 100, 200}): a box holds its whole 100, as its own reloads count it.
- **Body and shape:** `gunbodies.h` kPkp (22 hull points; 8.2 kg empty, the Pecheneg's public figure, 10.8 kg with its 100 rounds of 26 g: the free
  hands' line says both; heavy to the ear; its belt lying in the feed added as a part, its box the body's own). Its box's 24.3 cm bottom lets it stand on
  it (bodytest b1, b3: it may). **Dropped on a side from 1 m it lands on its box's end and rolls over its top onto its other side** (bodytest b2: the one
  exemption, `Row::rolls`; trials: whatever its bounce, 0.2 to 0.8, or its weight's spread, 7 to 14.5 cm either side, it does; its middle lowered 4 cm,
  half its drops still did); at rest on a side all the same, tipped 24 or 40 degrees onto its top. The shape's recipe `23298@2310 7 0,4` (without its box
  and its handle; its cover with its feed arm and the part under it, both flaps and the trigger kept as they rest, unturned: `kRestPkp`), 67 x 114 x 437
  cells; made on the player's PC as every shape.
- **Holster:** by its slot (0: the right shoulder), as every held gun.

### The hands

The game's own reload is all its right hand's; the mod's is the free left hand's. So each of the game's holds is mirrored onto the left glove (`magfit
... vanillaright`, new: the game's right glove on the gun mirrored across the gun's middle onto the left glove, `handpose::Mirror`) and then fitted:

- **On the box:** the game's own at 2.39 s (its steadiest of 2.3 to 2.56 s: its right hand by the bail, its middle out at x -0.21, beyond the box's
  right end), mirrored onto the left glove in place as well as in shape: its middle out at x 0.21, beyond the box's **left** end, its palm 41 mm off the
  box; moved 3.5 cm in along the gun's x, 0.5 cm up and 1 cm forward and each finger closed onto the box joint by joint (`magfit ... soft`: a give of
  2.5 mm, steel; the palm within 7 mm; against the gun's shape): palm 6.7 mm; thumb 1.0 -0.5 -0.5; index, middle and little -0.5 each; ring -0.5 -1.5
  -1.5 mm (in it: within a glove's give); against the gun's shape (its box left out of it) every part 5 cm clear. Its middle on the gun is the row's
  grab (0.174 -0.0817 0.5594). So the hand snapped onto the box is at its left end and draws it off to the right from its far end, where the game's
  own hand pulls it by the bail, right and down. Taken by its bail it's held where it's taken and goes the bail's own way (the glove within 0.8 cm of
  the hand), but with the left end's fingers (`hands.cpp`: a held box's fingers are its fitted hold's). A hold fitted at the bail, for the game's own
  action, is the owner's call (vanilla first).
- **On the cover:** the game's own at 1.70 s, moved 1.25 cm in, 1 cm up and 1 cm back, the fingers closed (`soft`, 2.5 mm, the palm within 8 mm,
  against the shape): palm 7.2 mm; thumb -1.5 -1.9 -0.5; index -1.5 -1.5 -0.5; middle -1.9 -1.5 -0.5; ring -0.5 -1.9 -0.5; little 4.1 -0.5 -1.5;
  against the gun's shape the palm 7.0 mm off, the fingers 1.2 to 2.5 mm in (a glove's give). Its middle is the row's lid grab, 1.9 cm from the game's
  own mirrored hand there (the box's, 3.5 cm).
- **On the handle** (on the right: the left hand reaches across the gun): over the top and from under it, fitted on its knob (`magfit over`; `under
  ... palm 7`). Over: palm 10.0 mm; index 2.0 2.0 -0.5; middle -1.5 -0.5 1.0; ring 4.5 12.0 6.1; little -8.0 4.5 11.4; the thumb clear over the top.
  Under: palm 4.0; thumb -1.0 -1.0 8.0; index 7.3 -0.5 -0.5; middle 7.0 10.6 23.3; the ring and little closed below; against the gun and its knob (2 mm
  cells) the palm 6.0 mm off, the hand's deepest 1.0 mm in. Their middles 6.8 and 6.9 cm from the knob, both on the gun's right.
- **On the handguard:** the game's own hold, kept (vanilla first).
- **The belt's pinch:** the M240's (the glove's own).

## What is forgiving

- The cover and the handle take the grip or the trigger; the cover goes the rest of the way by itself past its share.
- The box is taken anywhere along it (held where the hand took it, away from its end) and comes free drawn along its way, as it turns, or level; a fresh
  box goes home from anywhere 10 cm round where its way on starts; the belt into the tray from 10 cm.

## Not measured, not checked

- **Nothing here has run in the game.** First to film (weapon.txt `heldtest 7` plays the whole belt reload with nobody in the headset, the box along its
  way): the hold in both eyes; the left hand reaching across to the cover and the handle on the right; the box off to the right and a fresh one home from
  the right; the belt hanging on the box's right, pinched and laid; a burst's kick at the eye.
- **The hands:** mirrored from the game's right hand and fitted by numbers (above), never seen in the headset; whether the left hand reaching across the
  gun to the handle on its right, over or under, looks right only the headset will show.
- **The sights:** no rear sight stands on the line, so the line is the game's own zoomed eye's to the front post's tip. Where a player's eye is, the
  post is what they put on the target; the bullets go along that line. Whether it reads that way in the headset is a check.
- **The links in flight:** its links' effect is the game's own; the mod moves a casing's or link's draw to the gun in hand only by its index count
  (`effects.cpp` IsCasingCount: 234, 114, and 303 the M240's links). The PKP's links' count isn't known: if it isn't one of those, they fly from the
  game's own gun, not the one in hand. Step 11 below.
- **The kick in long bursts:** only two bursts of 4 rounds were recorded: held fire's push (1.89 cm, a held share of 1.04) may be soft. Step 12.
- **The feed arm** (bone 2, 11.6 cm along the cover's right, by the tray where the belt is laid): by hand it goes with the cover rigidly and is still
  as the handle is drawn. The game's goes with the cover as it opens, then turns its own way: 37 to 59 degrees off the cover once it has popped open,
  42 while it rests open, up to 95 as it's slammed shut, and up to 39 as the handle is drawn with the cover shut (above). To match it: its own turn
  from the resting-open frames, blended in by how far the cover is up (and one by the handle's travel); seen with the cover up in the headset.
- **The body rolling over on its side** (above): from the physics alone; a check on the ground in the game.
- **Sounds:** cut from its own reload wave (`sound/weapons/handheld/pecheneg/pecheneg_reload_wave`, one take, 6.53 s, 48 kHz mono) by its levels against
  the bones' moments, not heard. The wave was read from the owner's game (`build\gamesounds\raw`'s chunk of it, decoded as gamesounds decodes it), not
  through gamesounds' own cutter; the clips are made from the player's own game at the next start (`gamesounds --missing`).
- **Its scatter's numbers:** the standing block from the public dump (the multiplayer build); if they differ in the level, the log says "no scatter's
  numbers left to change" and it keeps the game's.
- **The extended box (200):** the count alone (an open bolt's big box); no campaign PKP carries it.
- **Night Shift's PKP with a PKS-07:** not built. Its scope isn't a `kHeldSights` row for the PKP (the owner's rule: every magnified scope gets the second
  renderer's picture); whether its draws ride the cover's bone (`lidride`) or a mount of their own is for its recordings. Step 13.
- **96/0 and the small draws** in its recordings (above): with the box out, look under the gun for a piece left where the box was.

## In-game checks (nobody in the headset first, then the owner)

1. A PKP picked up by hand on Swordbreaker (the gunners' drops): "Free hands: awake (the PKP in hand)", "Recoil: the PKP set up from slot N", "Rifle: its
   bullets' start found in N places", "Rifle: its scatter's numbers found".
2. The hold: both eyes, both hands on it, no arm stretched, the belt in the tray and the box under it with the gun (and with it when it lies); the left
   hand under the handguard as the game holds it.
3. A burst at a far wall over the front post: the rounds round the post's tip; the kick at the eye; the buzz.
4. Dry: the trigger stops, no reload by itself; the belt gone from the box. Its count: met full, "an open bolt's box of 100, its whole count: its floor
   round from the spare ones on top, 101 rounds loaded"; a box pushed home "Magazine: in; 101 rounds loaded ... (a magazine holds 100)"; the rounds a box
   fires to the stop 100, or 101 when the game fires past the stop.
5. `heldtest 7`: the cover up (its feed arm and both flaps with it); the box drawn to the right, tipping down at its right end, free (the log's "Held
   magazine: pulled out by hand"); a fresh one brought by the seat's right and below: "near the well: going home by itself", swung up into the seat; the
   belt pinched at its end on the box's right and laid; the cover shut; the handle. Then by hand: the box taken at its left end, in its middle and by its
   bail, drawn to the right as it turns and level (taken away from its end, the glove stays where the hand is or is drawn down with the bail end, and the
   box comes off in the hand as it was taken); straight down and steeply down it stays in, the glove back on the hand once it's 8 cm off; with the cover
   up, the handle from under it and over it (the handle taken, not the box); a round chambered, firing again.
6. The cover shut with the belt not laid, or the handle not pulled: nothing fires.
7. The holster: stowed at its slot's shoulder, drawn by either hand.
8. Let go: it lies as its body has it, the box and the belt with it (dropped on a side it may roll over onto its other side: too lively?); the box let go
   of lands as metal and lies 30 s.
9. The sounds (`lever_pkp`, `bolt_pkp`, `lid_latch_pkp`, `lid_up_pkp`, `mag_out_pkp`, `mag_in_pkp`, `belt_pkp`, `belt_lay_pkp`, `lid_shut_pkp`) made at the
   first start in `build\sounds` (`build\gamesounds.log`: none failed), played at their moments.
10. The cases from its port on its left, out to the left and a little down; the links: step 11.
11. **The links' draw:** the PKP in hand, weapon.txt `flashsurvey 3000`, then a burst (nobody in the headset: `filmtrigger 2 1 1` with the stand-in's head
    45 degrees down or more, or the trigger by hand). The log's "Survey:" lines: the links are the kind that appears only while firing, near the cover's
    left; if its count is 303 (the M240's links) nothing is to be done, else that count goes into `effects.cpp` IsCasingCount (one number).
12. **A long burst's kick:** weapon.txt `allrec 8`, then 30 rounds or more held (3 s at 600 a minute) from rest, nobody in the headset (`filmtrigger 3 1
    1`). Its frame bone against where it rested before the burst, over the held stretch after the first round's peak: that mean goes into the PKP's
    `held_cm` in `tools\recoil\kick.tsv` (1.89 now), then `powershell -File tools\recoilgen.ps1` and `-Check`, and pkptest's p1 row.
13. **Night Shift's PKP with its PKS-07** (when that level is played): `drawscan 60` for its draws (the scope's body, glass and marks), its reloads
    recorded with `allrec` and its mesh dumped (`dump 23298 2310` and each new draw's `dump N S`), zoomed `heldlog 1` for the eye, `glassdump 2` for its
    glass and marks; then its `kHeldSights` row. None of these numbers is guessed here.

## How the tables were made

```
magfit build lid.bmp 23298@2310 7 8 0.06 PkpLid lid.h vanillaright build\survey\PKP_empty.bin 1600 1770
magfit build lid3.bmp 23298@2310 7 8 0.06 PkpLid src\magposes\pkp_lid.h soft lid.h.start 2.5 8 shape build\guns\pkp.bin
magfit build box.bmp 23298@2310 7 0 -0.03 Pkp box.h vanillaright build\survey\PKP_empty.bin 2300 2560
magfit build box3.bmp 23298@2310 7 0 -0.03 Pkp src\magposes\pkp.h soft box.h.start 2.5 7 shape build\guns\pkp.bin
magfit build over.bmp 23298@2310 7 4 0.03 PkpLeverOver src\magposes\pkp_lever_over.h over -0.0509 0.018 0.4814
magfit build under.bmp 23298@2310 7 4 0.03 PkpLeverUnder src\magposes\pkp_lever_under.h under -0.0509 0.018 0.4814 palm 7
boxway build\survey\PKP_empty.bin 23298@2310 7 23298@2310 0 0.1740 -0.0817 0.5594 0.009 -0.094 0.551 2563 2770 3700 4000 Pkp src\magposes\pkp_way.h
       free 0.0824 onstart 3891
gunbodyfit build src\gunbodies.h
gunshape build build\guns\pkp.bin 0.003 23298@2310 7 0,4         (then shapemaketest build rest, for kRestPkp in shaperecipes.h)
powershell -File tools\recoilgen.ps1                             (tools\recoil\kick.tsv: recorded 1.82 47 2.48 63, held 1.89, from PKP_partial.bin)
gamesounds --sounds lever_pkp --sounds bolt_pkp ...              (the nine clips; or the mod's own start, --missing)
pkptest                                                          (the rows against the dumps, the shape and the recordings, the hands against the
                                                                  meshes and the shape, the reload through handmag, the cover's four bones, the clips)
```

`magfit ... vanillaright` (new): as `vanilla`, from a reload the game does with its right hand: the right glove's hold on the gun, mirrored onto the
left glove (the gloves' palettes paired with the gun's by the same view's draw: each frame's first of each). `boxway` (new for this): a box that is a
bone of the body's own draw (its frame and its box from the same palette); `free <m>` gives where it comes free, `onstart <ms>` the frame its way on
starts from. The grab and the lid grab in `pkprows.h` are the fitted holds' middles as handmag measures them (`kHandInGlove`), 2.3 mm from magfit's
own; boxway was run again with that grab. `src\magposes\pkp_belt.h` is the recordings' own: hanging the empty reload's 4.016 s (the box seated, the
hand not yet on the belt), laid its 10.0 s, of its still stretch 4.61-15.99 s (pkptest holds both to the recordings: laid to 0.21 mm and 0.06 degrees,
hanging to 1.32 mm and 1.43). (`magfit` writes a `.start` file beside its header: not kept in `src`.)

## After merging into the main folder

The test data the offline tests read are already in the main `build` (the dumps, `survey\PKP_*`) except two things this worktree made in its own build:
`build\guns\pkp.bin` (pkptest's shape checks skip without it: `gunshape build build\guns\pkp.bin 0.003 23298@2310 7 0,4`) and the PKP's points in
`build\bodytest_points.bin` (bodytest fails its "every fitted body's outermost vertices" check without them: `gunbodyfit build src\gunbodies.h`, which
writes the same header, `--check` 0 numbers off).

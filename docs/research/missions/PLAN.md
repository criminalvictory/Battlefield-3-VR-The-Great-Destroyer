# The remaining on-foot missions in VR: one ordered plan

Written 2026-11-19 from the six surveys in this folder (`uprising.md`, `guillotine.md`, `comrades.md`,
`nightshift.md`, `kaffarov.md`, `destroyer.md`) and main at 7e99b44. Nothing here was run in the game:
every "works" in the surveys is read from the level data and the code, so each mission starts with a
run that proves or corrects its survey.

Done or in hand, and not planned here: Semper Fidelis (done), Operation Swordbreaker (being finished),
Rock and a Hard Place (done). Waiting by the owner's word: Going Hunting (jet), Thunder Run and Fear No
Evil (tank).

Campaign order of what is left: **Uprising, Operation Guillotine, Comrades, Night Shift, Kaffarov, The
Great Destroyer.** (The game's order is Guillotine before Comrades: Comrades ends by loading Thunder
Run. The priority note lists them the other way round.)

## How to read it

- **Two bars.** *Finish* = a player gets from the first checkpoint to the credits with no fail he cannot
  answer and no crash, even if a gun is drawn the game's way. *Full* = the owner's bar: hands holding
  every gun of the mission with its reload by hand, its sights, prompts by reach and grip. Items are
  ordered so every mission reaches *finish* first, then *full*.
- **Needs.** `G` = time in the running game (a measurement, a film or a test). `O` = offline build and
  offline tests. `H` = the owner in the headset.
- **Effort.** S = half a day or less, M = about a day, L = 2 to 3 days, XL = more.
- **Lanes.** There is one game and one headset, so all `G` work is one queue (lane G), never at night
  with someone in the headset and never while the owner plays. `O` work runs in parallel in as many
  lanes as there are helpers, each on its own branch; lanes that touch the same file region are named.
  A gun's order is always: measure in the game (its vanilla hold and reload filmed first), build
  offline, film, self-test and the never-break list, merge.
- **Blocks** says which mission cannot reach *finish* (or *full*) without the item.

## Where the six missions stand

| Mission | Stops *finish* today (unproven or missing) | Biggest items for *full* |
|---|---|---|
| Uprising | rat QTE, real stance in the patrol area, Humvee .50 seat, stab prompt with the knife | Saiga 12K, 870, PKP, ACOG and RX01 on the G3A3, knife in the hand, weapon light |
| Guillotine | mortar-only kit at the start, ladder on Throttle, bathroom fight, Javelin and the three T-90s | SCAR-H, ACOG row, 870, Javelin by hand, M98B, PKP |
| Comrades | nothing known (never run on foot); metro fight and gas mask need the grip fix or the buttons | Dima's arms, AS Val, G36C, SG 553, F2000, MP7, P90, PKP, 870 |
| Night Shift | the game must see the M40A5 zoomed (IR signal look-at); the scope cannot be used at all today | M40A5 with bolt, its night scope, thermal glass, MP7, claymore |
| Kaffarov | helicopter fight QTE (keys beyond the trigger) | Dima's arms, PP-2000, Saiga 12K, M82A3 and its thermal scope |
| Great Destroyer | three fights with real grips, the mash, the Block prompt, the sewer climb on Throttle | SG 553, MP7, police-car ride with the M9, IRNV |

What the table says about order: the *finish* blockers are almost all input (grips, stance, Throttle,
zoom seen by the game, one seat, one launcher). They are small and shared, so they come first. The
*full* bar is mostly guns and two big rigs (Dima's arms, the scoped rifle with thermal glass), which
run in parallel lanes behind them.

---

## Part A. Shared foundations

Ordered by missions unblocked per day of work. A1 to A5 are the *finish* foundations; A6 to A16 are the
*full* foundations.

### A1. The grips answer a fight with a real squeeze (fix in work)
- **For:** Destroyer (three fights, each fatal on a miss), Kaffarov (helicopter fight), Guillotine
  (bathroom), Comrades (metro fight, gas mask), Uprising (rat). Five of six missions.
- **Needs:** O: land the fix (a held or a fresh grip answers each prompt once) with its test cases. G+H:
  one run in Swordbreaker's cellar with real controllers; that is the proof for all of them.
- **Effort:** S (in work). **Blocks:** *finish* of Destroyer and Kaffarov by the grips; the buttons
  (Y/left B = E, A = Space, trigger = left mouse) are a way round where the key is one of those.
- **Parallel:** offline now; its proof is first in lane G.

### A2. Two prompt kinds never run: a fight's mash and Block
- **For:** Destroyer (final fight: Space ten times; Solomon: right mouse, action 0x3E). The mash also
  covers Uprising's van and Guillotine's blocked door (E six times, no fail).
- **Needs:** O: test cases for the 130 ms mash rhythm and the press/mash select; the guard holding the
  mod's own aim key off while a right-mouse prompt is open. G: Destroyer checkpoints Bossfight and the
  last train door (headless films with the test grip first, real grips after A1).
- **Effort:** S to test, S to fix. **Blocks:** Destroyer *finish*.
- **Parallel:** with A1 (same files: the fight reader; one lane).

### A3. The game's real stance, read not believed (fix in work)
- **For:** Uprising (standing in the patrol area fails the mission 1 s later; crouch forced at the pipe
  and in the rift), Destroyer (train roof, sewer duck-through), Night Shift (forced crouch at the SUV),
  Guillotine (forced crouch at the door), Comrades (forced stand at the wire).
- **Needs:** O: the stick's stance taken from the soldier's pose each frame, so a forced stance cannot
  leave it wrong. G: Uprising, Meat Market: stand on purpose, read the fail; crouch through the patrol.
- **Effort:** S to M (in work). **Blocks:** Uprising *finish*.
- **Parallel:** own lane (input and stance files); proof in lane G after A1's.

### A4. Throttle (action 0x00) reaches a climb while the game holds the camera
- **For:** Guillotine (ladder into the bank), Destroyer (sewer climb at the manhole; only Throttle and
  look are allowed there).
- **Needs:** G: two short runs, stick forward on each ladder. O only if it fails: Throttle in the
  trigger list's key table, a W key bit held while the stick is forward (the walking-stall fallback is
  the pattern).
- **Effort:** S to test, S to M to fix. **Blocks:** *finish* of Guillotine and Destroyer if the stick
  does not reach it (a player hangs on the ladder).
- **Parallel:** the test rides with each mission's first run (C1, F1).

### A5. The first run of each mission on today's build
- **For:** all six. Each survey ends in a test list; none was run. One run per mission, by checkpoint,
  headless where the films suffice, the log kept. It settles the unknowns that decide the rest of the
  order: Comrades on the game's own arms, Guillotine's mortar kit, the HALO fall, the rides, the
  overlays, the forced weapon changes against the holsters, the old crash sites in Destroyer.
- **Needs:** G, about half a day per mission. No build.
- **Effort:** 6 x S. **Blocks:** nothing by itself; it turns "unproven" into a list of real defects.
- **Parallel:** lane G only, in campaign order, between the proofs above. Offline lanes do not wait.

### A6. A mounted gun and a vehicle seat (pipeline item 11)
- **For:** Uprising (the Humvee's M2: an objective, 70 s or more on timers), Swordbreaker (the end
  turret: the owner's rule is both grips on the handles, moving together; the "overpass MG" of the
  owner's list is the bridge's M249, a gun held in the hands: docs/research/m249.md, not a mounted gun).
  Later the tank and jet missions build on the seat. The design, its review and step 1's offline parts:
  docs/research/mounted_gun.md.
- **Needs:** G: measure the seat (the runbook in docs/research/mounted_gun.md section 6: where the
  seat is in memory, the camera and the pivot, how the turret answers its own input, the handles). O:
  first step: head free in the seat with the body fixed to the vehicle, the gun turned by both grips on
  its handles moving together (the owner's rule) and past the arms' reach by the right stick, either
  trigger firing while its grip holds a handle, the game's own arms drawn (enough for *finish*, since
  Uprising's seat has no kill count). Second step: the player's own gloves on the handles.
- **Effort:** M for the first step, L for the two-grip gun. **Blocks:** Uprising *finish* only if the
  seat cannot be entered or left in VR today, which is not known (A5 answers it); *full* otherwise.
- **Parallel:** own lane; shares its measuring session with Swordbreaker's turret.

### A7. ACOG, RX01 and M145 as held-sight rows
- **For:** ACOG: Guillotine (SCAR-H, then the M4A1 from the bridge to the end), Uprising (G3A3),
  Comrades (G36C, SG 553), Kaffarov (drops, Blackburn's M416 already has it). RX01: Uprising,
  Comrades, Kaffarov. M145: Uprising (Montes's M240), Kaffarov.
- **Needs:** G: each sight's draw key and its place on one gun (the ACOG first on the M4A1). O: rows in
  `kHeldSights`; the ACOG is magnified, so it takes the second renderer's picture with the M416's
  lens and marks; RX01 and M145 are small.
- **Effort:** M (ACOG), S (RX01), S (M145). **Blocks:** Guillotine *full* (the kit sight all mission).
- **Parallel:** own lane (the sight table); the measurement rides with C1.

### A8. The scoped rifle: the game sees the zoom, the eye gets the glass
- **For:** Night Shift (M40A5: the IR-signal look-at wants zoom level 1 within 5 degrees, and with free
  hands the zoom is never pressed), Kaffarov (M82A3), Guillotine (M98B, optional).
- **Needs:** O: the launcher pattern (zoom held while the eye is at the scope) for rifles, and what
  the game's zoom then does to the picture (FOV, the zoomed model, a full-view thermal) held back from
  the eyes. G: the M40A5 in the recon nest: does the look-at fire, what is drawn.
- **Effort:** M. **Blocks:** Night Shift *finish*. A stop-gap is worth a look in the same session: with
  the rifle left as the game draws it, let the aim button press the game's zoom. If that passes the
  look-at, Night Shift reaches *finish* before any rifle work.
- **Parallel:** own lane (input and scope view); before A9 and A10.

### A9. A bolt worked by hand
- **For:** Night Shift (M40A5, straight-pull under the campaign unlock), Guillotine (M98B), Kaffarov
  (SV-98, rarely in reach).
- **Needs:** G: the M40A5's vanilla hold and bolt cycle filmed, its bones measured. O: a bolt block
  (lift, pull, push, close; straight-pull as its short form), no shot until closed; test cases.
- **Effort:** L. **Blocks:** Night Shift *full*.
- **Parallel:** own lane (a new block beside the AK and belt blocks); after A8's measurement.

### A10. A thermal picture inside the glass
- **For:** Night Shift (the level is night; a plain picture will be very dark), Kaffarov (M82A3;
  IRNV on drops), Destroyer (IRNV on the guards' MP7 and Saiga).
- **Needs:** G: how the game makes its thermal picture (the full-screen pass, its inputs). O: the same
  pass applied to the second renderer's picture only, per eye.
- **Effort:** L to XL. **Blocks:** *full* of Night Shift and Kaffarov. Stop-gap: the scope's own
  picture brightened, so night targets can be seen at all.
- **Parallel:** own lane (scope renderer); independent of the guns.

### A11. Dima's arms: the third arm rig
- **For:** Comrades (whole mission on foot), Kaffarov (plane to pool). Without it no gun in either is
  in the player's hands.
- **Needs:** G: one measuring session on Comrades' gate: the arm draw's key, bones, sleeves, the body.
  O: a table in `kArmDraws` with its role strings, fitted hands per gun, the wrist and no-stretch
  rules, the self-test.
- **Effort:** L to XL (the bare arms took about a day of commits). **Blocks:** *full* of Comrades and
  Kaffarov; not *finish* if A5 shows the game's own arms play through.
- **Parallel:** own lane, but it is in the busiest part of `src\skin.cpp`: one helper, merged before
  the Dima guns (D2, G2, G3) are fitted.

### A12. Guns that several missions share
Each: G measure (vanilla hold and reload filmed, body and magazine keys, bones), O a `kHeldKinds` row,
its reload block, hand fits, shape, recoil row, sounds, test cases. All can be measured from the
missions already reachable, several per session.

| Gun | Missions | Block to build on | Effort | Note |
|---|---|---|---|---|
| a. Saiga 12K | Uprising, Night Shift, Kaffarov (kit slot 1), Destroyer | AK block, box of shells | M | measured on Swordbreaker; its magazine was not drawn after a swap |
| b. PKP Pecheneg | Uprising, Guillotine, Comrades, Night Shift | the M249's belt, fed from the right | M | measured on Swordbreaker |
| c. MP7 | Night Shift (kit slot 1, forced twice), Destroyer (most guards), Comrades, Kaffarov | the pistol's grip magazine on a long gun | M to L | new block |
| d. SG 553 | Destroyer (forced kit for the sewers), Comrades | AK magazine, a bolt catch | M | |
| e. 870 MCS | Guillotine (kit secondary), Uprising, Comrades, Night Shift | the M1014's shells; the pump is new | L | |
| f. F2000 | Comrades, Kaffarov, Destroyer | bullpup, magazine behind the grip | M | |
| g. SCAR-H | Guillotine (kit primary), Kaffarov (armory, drops) | measured like the Mk11 | L | |
| h. Type 88 | Kaffarov, Destroyer | the belt block | M | |

- **Blocks:** *full* only. Until built, each is drawn the game's way with the game's reload.
- **Parallel:** one lane per gun; rows merge one at a time (same tables). Order by forced kits first:
  g, c, d, a, then e, b, f, h.

### A13. Forced weapon changes and kit swaps against holsters by hand
- **For:** Guillotine (mortar kit, knife forced, the M4A1 kit under a fade), Comrades (machete slot 7,
  then slot 0), Night Shift (knife, MP7 twice, claymore slot 4), Kaffarov (four kit swaps, weapon
  change locked three times), Destroyer (kits at the kick).
- **Needs:** O: each case written into the holster test from the surveys' kit tables. G: seen in each
  mission's first run (holsters by hand, 7e99b44, is itself not yet run in the game).
- **Effort:** S to M. **Blocks:** nothing known; a lost or dropped gun would be a regression.
- **Parallel:** offline now.

### A14. The knife: a Fire prompt with the knife up, a key for slots 4 and 7, then the knife in the hand
- **For:** Uprising (the stab at the bazaar door gives the first gun: if missed the chance passes),
  Night Shift (stealth kill: Fire, 5 m, about 3 s, then death), Guillotine (bathroom), Comrades and
  Kaffarov (machete), Night Shift's claymore (slot 4 cannot be brought back today).
- **Needs:** G: the trigger with the knife as the only weapon (A5, Uprising). O: slot keys; later a
  held knife and a stab by hand.
- **Effort:** S to prove, S for the keys, M to L for the knife in the hand. **Blocks:** Uprising
  *finish* if the trigger is held off with no held gun.
- **Parallel:** the proof in B1; the rest its own lane.

### A15. Prompts by reaching and holding grip
- **For:** every mission: about thirty prompts on Interact, five on Jump, two on Fire, one on Clutch
  held 1 s. The use and jump buttons answer the presses today.
- **Needs:** O: one reader of the game's interaction entities (place, radius, cone, action), merged
  from the AT4 branch's reach-and-hold, with Jump and Clutch added. G: one prompt per mission.
- **Effort:** L. **Blocks:** *full* only.
- **Parallel:** own lane, any time.

### A16. Smaller shared checks
- a. Weapon light and laser: where the beam points with the gun in the hand, and a toggle binding
  (Uprising, Guillotine, Night Shift's sewers, Kaffarov's basement, which is dark). G check, then S to M.
- b. Slow motion against the mod's own timers (taps, buzzes, reload timing): Guillotine's door and
  ambush, Comrades' bus, Night Shift's SUV. G check.
- c. Full-frame overlays in both eyes: Comrades' gas mask (half the mission), dirt, rain; Destroyer's
  flashbang. G check; M if the mask must be moved or hidden.
- d. Scenes with the player as an actor (about forty across the six): the generic body lock and per-eye
  body, one look each in A5.
- e. `ForcePlayerLookAt` against the mod's look writes (Night Shift three times). G check.
- f. Suppressor muzzle point for flash and smoke (Uprising's AKS-74u, Night Shift's guns). S.

---

## Part B. Uprising (SP_Earthquake2)

| # | Item | Needs | Effort | Blocks |
|---|---|---|---|---|
| B1 | First run from Level Start: film, dig-out, crawl; rat by held grip, fresh grip, trigger; stand in the patrol area; crouch prompt; bazaar door; the stab with the knife up; the AKS-74u with suppressor, Kobra and light | G, after A1 and A3 | S | *finish* |
| B2 | The Humvee's gunner seat, first step (A6): get in, turn, fire, the rocket, the get-up, the Osprey | G measure, O, G | M | *finish* if the seat fails today |
| B3 | Van push mash and the school gate at 1.0 m, the breach | G | S | no |
| B4 | Saiga 12K, PKP (A12 a, b) | O, G film | M each | *full* |
| B5 | 870 MCS (A12 e), one drop | O, G film | L | *full* |
| B6 | ACOG and RX01 on the G3A3 (A7) | O, G | M, S | *full* |
| B7 | Knife in the hand from the dig-out to the stab (A14); crouch prompt on Clutch held (A15) | O | M | *full* |
| B8 | Weapon light, suppressor muzzle (A16 a, f) | G, O | S to M | *full* |

Finish line: B1, B2. Parallel: B4 to B8 in offline lanes while lane G does B1 and B2.

## Part C. Operation Guillotine (SP_Bank)

| # | Item | Needs | Effort | Blocks |
|---|---|---|---|---|
| C1 | First run: trench scene, the mortar-only kit and MOUNT MORTAR, wall climb, blocked door, the door that hits you, the Humvee ride and slow-motion ambush, the bridge kit swap, the ladder on Throttle (A4), the atrium guard, the bathroom fight (A1), the rappel, the vault | G | M | *finish* |
| C2 | Javelin, step one: do the three T-90s die by the marine's Javelin if the player does nothing; if not, does the game's own Javelin lock with the mod's aim today | G | S | *finish* |
| C3 | Fixes from C1: the mortar kit's hands (one unmeasured item, no other slot), Throttle if needed | O | S to M | *finish* |
| C4 | SCAR-H (A12 g) and the ACOG on it and on the M4A1 (A7) | G measure, O | L + M | *full* (the kit primary) |
| C5 | Javelin by hand: held launcher, lock by held aim, its sight picture, reload, holster on the left shoulder | G measure, O | L | *full*; *finish* if C2 fails both ways |
| C6 | 870 MCS (A12 e), the kit secondary, with its laser | O | L | *full* |
| C7 | PKP (A12 b); standalone M320 in slot 0 or 1 | O | M each | *full* |
| C8 | M98B with its 12x scope (A8, A9), optional pickup | O | L + M | *full* |
| C9 | Prompts by reach and grip, three of them on Jump (A15); forced crouch, forced knife, kit swap (A3, A13) | O, G | S to M | *full* |

Finish line: C1, C2, C3 (and A1 for the bathroom, with the trigger and use button as the way round).

## Part D. Comrades (SP_Paris)

| # | Item | Needs | Effort | Blocks |
|---|---|---|---|---|
| D1 | First on-foot run on today's build, gate to the gas event, then by checkpoint to the end: what the game's own arms and guns do, aim, holsters with a two-gun kit, the wire cut's cone, the mask tap, the metro fight by buttons, the chase seated (two 15 s legs), the overlays, slow motion, the corridor fight's triggers staying quiet after | G | M | *finish* (unknown until run) |
| D2 | Dima's arms (A11), measured here at the gate | G measure, O | L to XL | *full* |
| D3 | AS Val (kit slot 0): AK block, built-in suppressor; its Comrades-only Kobra against the Kobra row | G measure, O | M | *full*, after D2 |
| D4 | AKS-74u, AK-74M, AEK-971, M1014, UMP-45 fitted to Dima's hands; the UMP-45 was never run at all | O, G film | S each | *full*, after D2 |
| D5 | G36C and SG 553 (A12 d): most police and GIGN rifles | O | M each | *full* |
| D6 | F2000, MP7, PKP, 870 (A12 f, c, b, e); P90 (top magazine, new) | O | M to L each | *full* |
| D7 | RX01 and ACOG rows (A7); foregrip hold, lasers | O | S to M | *full* |
| D8 | Gas mask overlay if it sits wrong in the headset (A16 c) | O | M | *full* |
| D9 | Wire, breach and door prompts by reach and grip (A15) | O | S after A15 | *full* |

Finish line: D1 and whatever it finds. The mission ends by loading Thunder Run, which waits: a campaign
player lands in a tank next (see the open questions).

## Part E. Night Shift (SP_Sniper)

| # | Item | Needs | Effort | Blocks |
|---|---|---|---|---|
| E1 | The stop-gap and the first run: the aim button pressing the game's zoom with the M40A5 as the game draws it; does the IR-signal look-at fire; then the lights, the rappel, the wall climb on Jump, the stealth kill by the trigger with a real crouch, the forced MP7, the SUV shot under the forced look, the mall defence, the Osprey | O (S), then G | M | *finish* |
| E2 | The zoom held at the eye for rifles (A8) | O, G | M | *finish* done properly |
| E3 | M40A5 as a held gun with its bolt (A9), suppressor, magazine by the paddle | G measure, O | L | *full* |
| E4 | Its night scope row (8.5x) by the Mk11's method | O, G | M | *full* |
| E5 | Thermal picture in the glass (A10); until then the brightened picture | O | L to XL | *full* |
| E6 | MP7 (A12 c) with suppressor; the EoTech row exists | O | M to L | *full* |
| E7 | Claymore: a slot-4 key first, then taken from the right chest and placed by hand | O | S, then M | *full* |
| E8 | Forced weapon changes (A13), the forced look (A16 e), stray shots in stealth only if play shows them | G, O | S to M | *full* |
| E9 | Saiga 12K, PKP, 870 drops (A12) | O | shared | *full* |

Finish line: E1 (or E2 if the stop-gap fails).

## Part F. Kaffarov (SP_Villa)

| # | Item | Needs | Effort | Blocks |
|---|---|---|---|---|
| F1 | First run: the HALO fall as a scripted camera, the landing with the PP-2000, the drogue kit, the SUV ride and its gate-guard QTE (trigger), the gatehouse, the pool-house overwatch with the M82A3 the game's way, the disarm fight, the villa, the dark basement, the 8 s sprint to the helicopter, the helicopter fight (A1), the pool, Blackburn's part and the shot at Cole | G | M | *finish* |
| F2 | The helicopter fight's keys read from the level (only E is known) and proven with real grips | O read, G | S | *finish* |
| F3 | Dima's arms here (A11, from D2) | G check | S | *full* |
| F4 | PP-2000 (no attachments; the only gun at the landing) | G measure, O | M | *full* |
| F5 | Saiga 12K (A12 a) with PK-AS, laser, light; the AEK-971 with this kit's foregrip, laser, light | O | M, S | *full* |
| F6 | M82A3 as a held gun, the zoom at the eye (A8), its 8.5x scope row, thermal (A10) | G measure, O | L + M | *full* |
| F7 | Weapon light toggle (A16 a) for the basement | O | S | *full*; near *finish* if the basement cannot be seen |
| F8 | Kit swaps against the holsters (A13); Blackburn's M416 with the drawn M320 that has no slot | O, G | S to M | *full* |
| F9 | Armory and drops: SCAR-H, F2000, MP7, Type 88 (A12); M27 IAR (the M416 family, S); USAS-12, M39 EMR, DAO-12, PDW-R, M60E4, SV-98 (new, M each) | O | M each | *full*, optional guns last |
| F10 | RX01, M145, ACOG rows (A7), IRNV (A10); machete and grenades on Dima's arms | O | S to M | *full* |

Finish line: F1, F2.

## Part G. The Great Destroyer (SP_Finale)

| # | Item | Needs | Effort | Blocks |
|---|---|---|---|---|
| G1 | First run by the ten checkpoints with the log: the interrogation fight, the window jump, the train jump on Jump (10.5 s window, fatal), the train as Semper Fidelis's clone, the kick won on release, the roof crouch, the Solomon fight with Block (A2), the sewers with the game's SG 553, the climb on Throttle (A4), the police car, the final fight with its mash (A2); the bare arms' keys off the train; the two old crash sites | G, after A1 and A2 | M | *finish* |
| G2 | Fixes from G1: Throttle, the mash, Block, stance memory after forced stances (A3), arm keys | O | S to M | *finish* |
| G3 | SG 553 (A12 d), heavy barrel and foregrip, irons: the forced kit for the sewers | G measure, O | M | *full* |
| G4 | MP7 (A12 c) | O | M to L | *full* |
| G5 | The police-car ride with the M9: free hands, the scripted 15-round refills against the reload by hand, aim writes in the swerving car | G, O | M | *full* |
| G6 | F2000, Saiga 12K, Type 88 (A12 f, a, h); the Kobra row on the M1014 | O | M each, S | *full* |
| G7 | IRNV sight (A10) | O | L | *full* |
| G8 | A hint for the train jump if players miss it; CLIMB OUT, OPEN, the window and the M1014 take by reach and grip (A15); a knife button | O | S to M | *full* |

Finish line: G1, G2.

---

## The order, as one queue

Lane G (the game, one at a time) and the offline lanes run side by side. Offline items start now.

**Stage 1: every mission to *finish*.**

| Order | Item | Lane | Runs beside |
|---|---|---|---|
| 1 | A1 real-grip fix landed; proof in the cellar | O then G+H | 2, 3, 6 to 10 |
| 2 | A3 real stance landed | O then G | 1, 3 |
| 3 | A2 mash and Block test cases, the aim-key guard | O (the fight reader, with 1) | 2 |
| 4 | B1 Uprising first run (rat, patrol, stab with the knife: A14's proof) | G | all offline |
| 5 | A6 step one / B2 the Humvee seat: measure, build, run | G, O, G | 6 to 10 |
| 6 | E1 the zoom stop-gap built (aim button presses the game's zoom on an unheld scoped gun) | O | 5 |
| 7 | A13 forced-change cases into the holster test | O | any |
| 8 | C1 + C2 Guillotine first run, with the ladder (A4) and the Javelin question | G | offline |
| 9 | C3 Guillotine fixes; C5 Javelin only if C2 says the tanks need the player | O | 10 |
| 10 | D1 Comrades first on-foot run; the arms measured in the same session (for A11) | G | offline |
| 11 | E1 Night Shift first run with the stop-gap; E2 if it fails | G, O | |
| 12 | F1 + F2 Kaffarov first run, the helicopter fight | G | |
| 13 | G1 + G2 Destroyer first run and fixes | G, O | |

After each mission's finish line: self-test, the never-break list, a CHANGELOG entry, a release when it
is worth one.

**Stage 2: *full*, in offline lanes that start during stage 1** (each needs a short measuring slot in
lane G, batched with the first runs above):

- Lane 1 (sights): A7 ACOG, RX01, M145.
- Lane 2 (kit guns, campaign order): SCAR-H, AS Val (after A11), M40A5 with A9, MP7, PP-2000, M82A3,
  SG 553.
- Lane 3 (shared drops): Saiga 12K, PKP, 870, F2000, Type 88, G36C, P90, the armory's rest.
- Lane 4 (arms): A11 Dima's arms, then the fits of the held guns on them.
- Lane 5 (scope): A8 done properly, then A10 thermal.
- Lane 6 (hands on the world): A15 prompts by reach and grip, A14 the knife, A6 step two, the Javelin
  by hand, the claymore, A16's light.

Lanes 2, 3 and 4 all add rows to the same tables in `src\skin.cpp`: build apart, merge one at a time,
self-test after each merge.

## Effort in sum (rough)

- Stage 1: about 8 to 12 days of lane G and small offline fixes, if the first runs find no new large
  defect. The known risks: the Humvee seat, the Javelin, the mortar kit, Comrades on the game's arms.
- Stage 2: about 15 gun rows (M to L each), three sight rows, two big rigs (Dima's arms, thermal
  glass), the bolt, the pump, the prompt reader: 30 to 40 helper-days, most of it in parallel.

## Open questions for the owner

1. **The missions that wait sit between the ones that play.** A campaign player goes Uprising, Going
   Hunting (jet), Guillotine, Comrades, Thunder Run and Fear No Evil (tank), Night Shift. If those three
   cannot at least be got through in the headset (seated, the game's own view), the campaign stops
   there whatever is built here. A minimum pass for them is worth a place right after stage 1.
2. **Guns before finish, or finish before guns?** The owner's method is every gun of a level first,
   then its sights, before the level is done. This plan puts *finish* for all six ahead of the guns,
   because players are downloading and the blockers are small; the guns then follow per mission in
   campaign order. Say if the per-level method should hold instead.
3. **Comrades and Kaffarov before Dima's arms exist:** released as "plays with the game's own arms"
   (as the README says today), or held back until the arms are in.

## Not verified

Everything in the surveys is read from the Venice-EBX dump and the code; nothing was run. The efforts
are the surveys' estimates. Whether today's build finishes any of the six is unknown until its first
run (A5).

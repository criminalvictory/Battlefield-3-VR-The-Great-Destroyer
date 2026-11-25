# Kaffarov (SP_Villa) in VR: survey

Campaign mission 11. Dima from the HALO jump to the pool, then Blackburn for the last minutes.
Read-only research, 2026-11-19: nothing was run in the game. Sources:

- **[data]** the public Venice-EBX dump, `Levels/SP_Villa/*` (file:line given), read with the
  event-graph scripts in the scratchpad (`scratchpad\missions\kaffarov\g.py`, `dump.py`, the
  per-sub-level graphs `g_*.txt`).
- **[cat]** `docs/research/weapons-catalog.md` section 2.11 (kits, pickups, drops; itself from the data).
- **[web]** the walkthrough survey in the QTE workflow's journal (wf_fe217b53-80e, results 2 and 3).
- **[mod]** main at 7e99b44: `src/skin.cpp` `kHeldKinds` / `kHeldSights`, `src/input.cpp`,
  `src/qtelist.h`, `src/qtefight.h`, and the project notes.

Everything marked *verify* is a reading of the data that needs one run in the game.

## 1. What the mod has for this mission today

| Thing | State |
| --- | --- |
| Dima's arms (Russian gloves and sleeves) | **No table.** The mod knows the Marines' gloves (`kGloves` 13755/15051) and Blackburn's bare arms only [mod; cat finding 7]. On this level the arms and whatever they hold are drawn the game's way |
| Held guns with a reload by hand | AEK-971, AK-74M, RPK-74M (drops), M416 (Blackburn). Rows exist; none has been seen on Dima's arms |
| Sights with a row | PK-A, PK-AS, Kobra, PSO-1, PKS-07, EoTech, the rifle scope (the Mk11's). The M416's ACOG is its own |
| Guns with no row | PP-2000, Saiga 12K, MP-443, M82A3, SCAR-H, USAS-12, M39 EMR, M27 IAR, F2000, M60E4, Type 88, DAO-12, PDW-R, MP7, SV-98; machete/knife |
| Sights with no row | the M82A3's thermal scope (8.5x), IRNV (1x thermal), RX01, M145, ACOG on anything but the M416 |
| A gun with no row | stays as the game draws it (its own hold, its own reload animation, bigger than life: the Swordbreaker roof report) |
| Interaction prompts on E | the use button (left Y / left B; `g_use` sends E) answers every one here |
| Fist-fight QTEs (QuickTimePrefab) | the fight reader (`qtefight.h`) answers the one awaited key; proven with the test grip on The Great Destroyer; **a real grip failed on 2026-11-19** (fix workflow running) |
| Scripted cameras | head-turned cutscene camera with the body locked (proven on the Comrades drive and the interrogation) |
| Sprint, jump, crouch, hold breath | left stick click (Shift: also the scope's hold-breath), A, B / the stick's stance |
| Weapon light / laser toggle | **no binding** (nothing in `input.cpp` sends the game's key) |

## 2. The mission in play order

Checkpoints [data]: Pre-landing Save (`Setup_Schematic.txt:476`), MidRailSave
(`Rail_Logic_Schematic.txt:1673`), GateHouse Save (`GateHouse_Logic_s.txt:231`), Pre-Pool House
Save (`Drive_Logic_s.txt:2911`), Pre-Villa Save, Pre-Villa Save2 (`Setup_Schematic.txt:489`, `:515`),
Halfway Villa, Pre-Basement Save (`Villa_SQUAD_Schematic.txt:641`, `:628`), a basement save
(`Basement_SQUAD_Schematic.txt:616`), Pre-Blackburn Save, Blackburn Save (`Setup_Schematic.txt:502`, `:541`).

Fails anywhere [data]: leaving the play area for 5 s (`OutOfBounds_SP_5S`, five of them), friendly
fire, a failed fight QTE, the helicopter timer.

### 2.1 The plane and the HALO jump (sub-level Halo)

- Kit `SP_Dima_Sp_10_HALO` (PP-2000 bare, machete) [cat]. Input restriction: walk and look only;
  fire, zoom, jump, stance, weapon change all off (`Halo_Logic_s.txt:577`).
- The briefing runs on a 59 s sequence. At 40.5 s the prompt **ID_H_INT_SP_VILLA_TO_START_JUMP**
  appears: Interact (E), press, radius 5 m, one use (`Halo_Logic.txt:2`). No timer on it.
- E starts a 63 s sequence: a canned three-actor scene (`MultipleActorScenario`,
  `SP_Villa_HaloJumpParachutes`, the weapons drogue's own animation), free fall, parachute, a white
  fade. The player steers nothing. Controls come back at its end, then the Pre-landing Save.
- **VR:** prompt = use button, AS IS. The fall is a scripted first-person scene of about a minute:
  *verify* that it is seen as a scripted camera (head free, body locked, level horizon while the
  body tumbles) and that no free hand or gun of the mod's floats in it. The hardest comfort moment
  of the mission.

### 2.2 The landing (Landing)

- On foot with the PP-2000 only: guards in idle loops (smoking, talking, watching, "find gear"),
  six to kill (`Landing_Logic`, an EventAndGate of 6). A played shot is noticed (`PlayerFireCheck`).
- The weapons drogue: prompt **ID_H_INT_SP_VILLA_OPEN_DROGUE**, Interact, press, radius 2.5 m, one
  use (`Landing_Logic.txt:2763`). It applies kit `SP_Dima_Sp_10` by script and the game forces one
  weapon-light toggle (`Landing_Logic_Schematic.txt:716`). The AEK-971, Saiga and three M67s lying
  in the case are display models (`AllowPickup False`, `Landing_Logic.txt:4044-4330`).
- Kit `SP_Dima_Sp_10` [cat]: slot 0 **AEK-971** (PK-A, laser, foregrip, flashlight), slot 1
  **Saiga 12K** (PK-AS, laser, flechette, flashlight), slot 6 M67, slot 9 MP-443 (no key reaches it),
  the machete.
- **VR:** prompt AS IS. The kit comes by script, not by a pickup: *verify* the holsters-by-hand
  memo takes a scripted kit (primary right shoulder, Saiga left) and does not hold on to the PP-2000.

### 2.3 The SUV ride to the gatehouse (Rail, RailSUV)

- Prompt **ID_H_TO_ENTER_VEHICLE** at the SUV: Interact, press, radius 1.5 m, within 55 degrees
  (`Rail_Logic.txt:155`).
- Not a vehicle seat: a canned three-actor scene with the SUV as an animated model
  (`PlayerCar_AnimSet`), 150 s long (`Rail_Logic_Schematic`, sequences 2EA8 and 8F5B). Look stays
  free; fire, zoom, jump, stance, weapon change are off (`Rail_Logic_Schematic.txt:890`). The game
  sets the current weapon itself twice (the MP-443, slot 9) and saves MidRailSave on the way.
- **QTE 1, the gate guard at the window** (about 123-126 s in): `QuickTimePrefab` started by the
  scene's TriggerProps, stopped at ScenarioFinished (`Rail_Logic_Schematic.txt:1206`). PC key: left
  mouse [web]. On success the game fires the pistol itself (a forced Fire trigger,
  `Rail_Logic_Schematic.txt:1490`) and the window breaks. Failed: "You were killed" after 1.0 s.
- **VR:** prompt AS IS. Ride: scripted scene, *verify* (which of the mod's two ride paths it takes:
  the scripted camera of the Comrades drive, or the seated follower of Rock's car; the pistol in the
  animated hands; nothing of the mod's hovering). QTE: FIGHT (one press, left mouse); the right
  trigger is that key too.

### 2.4 The gatehouse (GateHouse)

- Out of the SUV. Prompt **ID_H_MES_SP_VILLA_PRESS_E_TO_OPEN_GATE**: Interact, press, radius 1.5 m,
  one use (`GateHouse_Logic.txt:2`); it saves (GateHouse Save), plays the bio-scanner scene
  (`SP_Villa_GateHouse_BioScanner`, the squad's) and opens the gate; the three gatehouse guards are
  canned-scene soldiers (no drops). Then the fight starts.
- **VR:** prompt AS IS. Nothing else special.

### 2.5 The drive up to the villa, on foot (Drive, Drive_2)

- A running fight up the driveway: PMC riflemen, shotgunners (the commonest), SMG and machine
  gunners; two enemy `Villa_SUV`s arrive on canned paths (`Drive_Logic.txt:7760`, `:7907`: not
  enterable in play); scripted rockets (`InvisibleRPG7`) and cars blowing up. One zone switches
  jump and stance change off while inside (`Drive_Logic_s.txt:2188`). Ends at the Pre-Pool House Save.
- Drops from here on [cat]: AEK-971, AK-74M, F2000, SCAR-H (Kobra, PK-AS, PSO-1, ACOG, RX01, EoTech,
  rifle scope), M60E4 (ACOG, EoTech, M145, RX01), RPK-74M, Type 88, DAO-12, USAS-12, PDW-R, MP7;
  SV-98 rarely in reach. No drop: rocket soldiers, Mk11 snipers.
- **VR:** no launcher, no mounted gun, no vehicle for the player. Guns: section 3.

### 2.6 The pool house (Poolhouse, Poolhouse_Extra)

- The door on the right: an **unnamed Interact prompt**, press, radius **0.75 m**, one use
  (`PoolHouse_Logic.txt:2944`), switched on by a voice line. 0.25 s after it every input is locked
  (`PoolHouse_Logic_Schematic.txt:922`) and the scene `SP_Villa_Rifle_Disarm_Fight` plays (a guard
  bursts out, Dima takes his rifle). **Not a QTE**: the pool house has no quick-time trigger.
- Kit `SP_Dima_Sp_10_Sniper` (`PoolHouse_Logic_Schematic.txt:135`): slot 0 becomes the **M82A3 with
  the campaign-only thermal scope** (8.5x) [cat]. It is applied by a zone at the door, so it can
  come before the fight. The trigger that watches the gun is named "SVD" in the data; tooltips only:
  HOLD_BREATH and INFRA_RED on the first zoom (`:1427`), SWITCH_TO_SVD with the wrong gun in hand.
- Overwatch ("provide supportive fire"): five PMC spawners with thermal sights (shotgun, assault,
  LMG kits `SP_MERC_*_IRNV`) in the dark courtyard; **seven kills** (or reaching the next zone) move
  the scene on (`CountDown 8AE3`). No timer read; friendly fire fails.
- Afterwards a pickup in the pool house gives the AEK-971 back with its kit attachments
  (`PoolHouse_Logic.txt:4965`, radius 2.5 m). Then the Pre-Villa Save.
- **VR:** prompt AS IS (the use button; 0.75 m is close: stand at the door). The disarm scene:
  scripted camera, *verify*. **The M82A3 and its thermal scope are forced on the player and the mod
  has neither**: the owner's rule is a real second-renderer picture for every magnified scope, and
  this one is a thermal picture (new work for the scope renderer; the same need as Night Shift's
  M40A5). Hold-breath is Shift = the sprint click.

### 2.7 The garden and the villa (Garden, Villa_*)

- Garden: a scripted rocket takes the monument; fight; Pre-Villa Save2.
- Villa: Halfway Villa save; the flank with the scene `SP_Villa_HumanBomb_Fight` (fire is off while
  it plays, `Villa_FLANK1_Schematic.txt:750`); an enemy machine-gun position
  (`Villa_MG_Schematic`: AI orders only, **nothing the player mounts**); scripted rockets.
- The doors are the squad's (Kiril kicks them): **no door prompt for the player** [data].
- The main room: weapon change is locked (`Villa_MAINROOM.txt:586`), the ammunition is set, and a
  play **Fire trigger** is switched on (`Villa_MAINROOM.txt:907`); the player's shot starts
  `SP_Villa_Basement_Discovery` and lifts the lock. Then the Pre-Basement Save.
- **VR:** the right trigger is the input, AS IS. *Verify* the mod's shoulder draws while the game
  refuses weapon changes (the mod's record and the game's must not part).

### 2.8 The basement and the armory (Basement)

- Dark (`SP010_Dark`); tooltip **TOGGLE_FLASHLIGHT** at the entrance. Surprise shotgunners
  (`MOUTSurpriseSHG`), red-dot shotgun and SMG mercenaries. Two ammo crates in the level.
- The armory: five pickups, radius 2.5 m, slot 0 (`Basement_Logic.txt:12489-12817`): **SCAR-H**
  (RX01, foregrip, light), **USAS-12** (PK-A, light, frag rounds), **M39 EMR** (PK-AS, foregrip,
  light), **M27 IAR** (RX01, foregrip, light), **F2000** (EoTech, foregrip, light). All optional.
- **VR:** the light toggle has no binding in the mod; every kit gun and armory gun here carries a
  light. The five armory guns have no rows.

### 2.9 The helicopter (Chopper)

- Out to the helipad. **A timer:** entering the first zone starts 8.0 s; the zone at the helicopter
  is then switched off and 5.0 s later "Kaffarov escaped" fails the mission
  (`Chopper_LOGIC_Schematic`, Delay 4732 then 764E). So: sprint.
- Reaching the helicopter locks every input (`Chopper_LOGIC_Schematic.txt:510`) and starts
  **QTE 2**, `SP_Villa_Heli_Quicktime_Fight` with the `QuickTimePrefab`
  (`Chopper_LOGIC_Schematic.txt:589`). Keys: E first [web], the rest unknown from every source.
  Failed: fade after 5.0 s, mission failed.
- Both fall into the pool; kit `SP_Emperor_Beatup`; the scene `SP_Villa_DimaKaffarovTalk` (a 45 s
  sequence), fades, the Pre-Blackburn Save.
- **VR:** sprint = left stick click, AS IS. QTE: FIGHT; the key order doesn't matter to the reader,
  but this one needs keys other than the trigger's, so it depends on the real-grip fix. Scenes:
  scripted cameras, *verify* (a fight on a flying helicopter: level horizon).

### 2.10 Blackburn (Blackburn)

- Kit `UsHumanSpSoldier_SP_Villa_Blackburn`: **M416 with ACOG**; an M320 is drawn under it but the
  kit has no launcher slot, so it can't fire [cat]. Blackburn Save. A zone on the way switches fire
  off (`Blackburn_LOGIC_Schematic.txt:751`); out of bounds fails.
- The room: the scene `SP_Villa_Dima_Meet_Black` (75 s; walking locked, look free, `:593`) with
  **QTE 3**, a `QuickTimePrefab` started 0.1 s in whose button outputs are not wired: **no prompt
  is drawn**. PC: left mouse once, to shoot Cole; any other button, aim included, is a miss [web].
  The level then completes.
- **VR:** the M416 is the mod's own gun (Marine gloves: *verify* this level's Blackburn has them).
  QTE: FIGHT, the trigger is the key; *verify* the guard holds back the mod's aim and stance keys
  here and that the M320 snap does nothing with no launcher.

## 3. Every gun the player can hold

| Gun | How it comes | Slot | Attachments | Mod: held row | Reload by hand | Sight row |
| --- | --- | --- | --- | --- | --- | --- |
| PP-2000 | kit, HALO to the drogue | 0 | none | no | no | irons |
| Machete | kit | melee | | no (no knife in the mod) | | |
| AEK-971 | kit at the drogue; pickup again in the pool house; drops | 0 | PK-A, laser, foregrip, flashlight | **yes** | pulled or knocked out, lever | PK-A yes |
| Saiga 12K | kit at the drogue | 1 | PK-AS, laser, flechette, flashlight | no (measured on Swordbreaker: `build\survey\SAIGA12K*`) | no | PK-AS yes |
| M67 | kit | 6 | | grenades by hand exist on the glove levels; not on Dima's arms | | |
| MP-443 | kit slot 9, the SUV scene only | 9 | none | not needed: the scene's own animation holds and fires it | | |
| M82A3 | script, pool house | 0 | thermal scope 8.5x | no | no | **no (thermal)** |
| SCAR-H | armory pickup; drops | 0 | RX01, foregrip, light (drops: more) | no | no | RX01 no |
| USAS-12 | armory pickup; drops | 0 | PK-A, light, frag | no | no | PK-A yes |
| M39 EMR | armory pickup | 0 | PK-AS, foregrip, light | no | no | PK-AS yes |
| M27 IAR | armory pickup | 0 | RX01, foregrip, light | no | no | RX01 no |
| F2000 | armory pickup; drops | 0 | EoTech, foregrip, light | no | no | EoTech yes |
| AK-74M | drops | 0/1 | Kobra, PK-A, PK-AS, PSO-1, foregrip, bipod, IRNV | **yes** | yes | all but IRNV |
| RPK-74M | drops | 0/1 | bipod, sights, IRNV | **yes** | yes | all but IRNV, M145, RX01, ACOG |
| DAO-12 | drops (commonest enemy with the USAS) | 0/1 | Kobra, EoTech, RX01, flechette, light, laser, IRNV | no | no (shells one by one, the drum) | partly |
| PDW-R, MP7 | drops | 0/1 | EoTech, Kobra, RX01, ACOG, light, laser | no | no | partly |
| M60E4 | drops | 0/1 | ACOG, EoTech, M145, RX01 | no (belt blocks exist: M240, M249) | no | EoTech only |
| Type 88 | drops | 0/1 | bipod, sights, IRNV | no | no | partly |
| SV-98 | drop, rarely in reach | 0/1 | PKS-07 | no (turn bolt: new) | no | PKS-07 yes |
| M416 | Blackburn's kit | 0 | ACOG; M320 drawn, no slot | **yes** (the mod's first gun) | yes | yes |

Launchers: none for the player (rocket soldiers don't drop; `InvisibleRPG7` is the script's).
Mounted guns: none. Vehicles: none driven or crewed; the SUV is a canned scene.

## 4. Prompts and QTEs, one table

| Where | Prompt / input | Kind | Radius | Mod |
| --- | --- | --- | --- | --- |
| Plane | TO_START_JUMP, E | press | 5 m | AS IS (use button) |
| Landing | OPEN_DROGUE, E | press | 2.5 m | AS IS |
| SUV | TO_ENTER_VEHICLE, E | press | 1.5 m, 55 degrees | AS IS |
| SUV, gate guard | QuickTimePrefab, left mouse | one press; fail 1 s later | | FIGHT (trigger or grip) |
| Gatehouse | PRESS_E_TO_OPEN_GATE, E | press | 1.5 m | AS IS |
| Pool house door | unnamed, E | press, then a scripted fight | 0.75 m | AS IS; not a QTE |
| Pool house | zoom tooltips | text only | | nothing awaited |
| Main room | play Fire trigger | the player's shot | | AS IS (trigger) |
| Basement | TOGGLE_FLASHLIGHT tooltip | text only | | **no binding** |
| Helipad | 8 s to the helicopter | timer | | sprint |
| Helicopter | QuickTimePrefab, E then unknown | sequence; fail 5 s later | | FIGHT (needs the grip fix) |
| Blackburn | QuickTimePrefab, left mouse, no prompt drawn | one press | | FIGHT (trigger); verify the guard |

No ladders, crawls, climbs on Throttle, forced prone or timed sniper kill in the data. The forced
moments are: the plane (walk only), the SUV (seated scene), the pool-house fight and the helicopter
(all input locked), the main room (weapon change locked), Blackburn's room (walking locked).

## 5. What is missing, in order of need

"Needed" here is the owner's bar for a playable mission (view, aim, hands holding the mission's
guns, cutscenes, no crash). The game itself can probably be finished with the game-drawn guns.

1. **Dima's arms** (foundation, shared with Comrades): no table, so no gun on this level is held
   the mod's way until they are measured. Large.
2. **PP-2000** as a held gun (the only gun for the landing). Medium.
3. **M82A3** as a held gun, and **its thermal scope** as a real scope picture (forced overwatch).
   Medium + large (thermal in the second renderer; Night Shift needs the same).
4. **Saiga 12K** as a held gun (kit slot 1; already measured). Medium.
5. The AEK-971 with this kit's **foregrip, laser and flashlight** on Dima's arms: check and fit. Small.
6. **Real grips on fight QTEs** (the fix in progress), then the helicopter fight and the two
   left-mouse QTEs proven here. Small once the fix is in.
7. **A weapon light / laser toggle** on the controllers (dark basement). Small.
8. Scripted scenes to run once each: the HALO fall, the SUV ride, the disarm fight, the helicopter,
   the two talks. Fixes unknown until seen.
9. Scripted kit changes against the holsters-by-hand memo (four kit swaps, one give-back pickup,
   weapon change locked three times). Small to medium.
10. The armory's five guns and the drops' guns (SCAR-H, USAS-12, M39 EMR, M27 IAR, F2000, DAO-12,
    PDW-R, MP7, M60E4, Type 88, SV-98) and the sights RX01, M145, IRNV, ACOG off the M416.
    Optional for finishing; each a gun's work.
11. Machete, grenades on Dima's arms. Optional.

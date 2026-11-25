# Comrades (`SP_Paris`) for VR: survey, first checkpoint to the end

Read-only survey, 2026-11-19, against main `7e99b44`. Nothing was run in the game. Sources: the
public Venice-EBX dump (level files under `Levels/SP_Paris/`, the kit and the enemies' kits under
`Characters/Soldiers/Customizations/`), the mod's source (`src/skin.cpp` `kHeldKinds`,
`kHeldSights`, `kArmDraws`; `README.md`), `docs/research/weapons-catalog.md` (2.6) and the
campaign QTE survey of 2026-11-19. Tags: [data] read in the dump, [mod] read in the mod's code or
notes, [web] walkthroughs as the QTE survey quoted them, [inferred] my own reading, not checked.

You play **Dima** (GRU), with Vladimir and Kiril. The level is all on foot after one car ride:
no vehicle to drive, no mounted gun, no launcher, no ladder. After it the campaign loads
`SP_Tank` (Thunder Run, a tank mission that waits) [data: `Flow.txt:1840`].

## 1. What decides whether it is playable

1. **Dima's arms have no table in the mod.** The level draws `arms1p_rus_mesh`, not the Marines'
   gloves (`kArmDraws` knows the gloves 13755/15051, their depth pass, the bare forearms, the
   sleeves and the body: nothing Russian) [mod]. The README says so to players: "Where you play
   someone else (Dima in Comrades and Kaffarov ...), the arms and guns are still the game's own."
   No gun of this mission is in the player's own hands until that table exists. It was never
   looked at on foot: the only headset time in this level was the opening drive (2026-11-01).
2. **The gun in hand at the first checkpoint, the AS Val, is not a held gun** (`kHeldKinds` has
   no row). The second gun of the kit, the AKS-74u with its PK-A, is built.
3. **Five of the 13 guns a player can hold here are held guns in the mod** (AKS-74u, AK-74M,
   AEK-971, M1014, UMP-45); **eight are not** (AS Val, F2000, MP7, PKP, G36C, SG 553, P90,
   870 MCS).
4. **Every prompt and QTE can be answered from the controllers today** (E is Y / left B, Space is
   A, left mouse is the trigger), so none of them stops a player. None is yet done the owner's
   way (reach and hold the grip for prompts; either grip for fights, where a real grip failed on
   2026-11-19 and the fix is not in main).
5. **Three timed moments can kill or fail:** the gas mask (2.5 s), the chase (two 15 s legs) and
   the metro fight (three prompts, a miss fails the mission).

## 2. The kit and every gun, with the mod's state

Kit `Characters/Soldiers/Customizations/SP_Dima_Sp_06` (spawn template
`Gameplay/Soldiers/SoldierSpawnTemplates/Sp_006HumanSoldierSpawnTemplateData`) [data]:

| Slot | Weapon | Unlock asset | Sight | In the mod |
| --- | --- | --- | --- | --- |
| 0 (in hand) | AS Val | `Weapons/ASVal/U_ASVal` | `U_ASVal_Kobra_SP_Paris` (a Kobra unlock of this level only) | **No row.** AK pattern, so the AK block (rocked magazine, right handle, no hold-open); built-in suppressor (muzzle point, length). Whether its Kobra is the same mesh as `kHeldSights`' Kobra (6492/210) is unchecked |
| 1 | AKS-74u | `Weapons/AKS74u/U_AKS74u` | `U_AKS74u_PKA` (PK-A, 2.95x) | **Held gun** (`aks74u`, 17991), AK reload by hand; PK-A is a `kHeldSights` row with the second renderer's picture. Measured on the train's bare arms, never seen on Dima's |
| 6 | M67 | `Weapons/M67/U_M67` | | No grenade by hand yet (G on the keyboard) |
| 7 | Machete | `Weapons/Machete/U_Machete` | | Not by hand; the game draws it itself in the wire cut |

Pickups placed in the level (`WeaponUnlockPickupEntityData`, radius 2.5 m, the game's HOLD [R] =
X held, or the mod's magnet pickup) [data]:

| Where | Weapon | Parts | Ammo | In the mod |
| --- | --- | --- | --- | --- |
| Gas sluice (`GasEventSluice_Logic.txt:1361`) | M1014, slot 0 (alt 1) | lists F2000 EoTech, foregrip and light: cannot fit, so bare [inferred] | as placed | **Held gun** (`m1014`), shells by hand |
| Upper office (`Upper_Office_Logic.txt:4499`) | AKS-74u, slot 0 (alt 1) | Kobra | 200-400 | **Held gun**; Kobra row built |
| Upper office (`Upper_Office_Logic.txt:4579`) | AK-74M, slot 0 (alt 1) | Kobra + foregrip | 200-300 | **Held gun** (`ak74`); Kobra built; the foregrip has no hold of its own |

Drops (every kit below has `DisableDeathPickup False`) [data]:

| Who | Where they are | Weapon | Sights and parts | In the mod |
| --- | --- | --- | --- | --- |
| PLR assault (`TERR_assault`, `_NoGasMask`) | garage, offices, trading floor, street | AK-74M | none | **Held gun** |
| same | same | AEK-971 | none | **Held gun** |
| same | same | F2000 | none; EoTech on the unmasked ones | **No row** (bullpup, magazine behind the grip, forward-left handle) |
| PLR SMG (`TERR_SMG`) | garage, lobby, offices | MP7 | lists an AKS-74u laser: bare [inferred] | **No row** (magazine in the grip) |
| same | same | AKS-74u | none | **Held gun** |
| PLR SMG unmasked (`TERR_SMG_NoGasMask`) | street, chase | UMP-45 | none | **Held gun** (`ump45`), not yet run in the game |
| PLR shotgun (`TERR_SHG`) | offices, street | M1014 | none | **Held gun** |
| PLR heavy (`TERR_Heavy_Support`) | lower office, street | PKP Pecheneg | none | **No row** (belt from the right: the M240's blocks mirrored) |
| Bomb carrier (`TERR_bomber`) | lobby, street | AK-74M | none | never dropped in reach [inferred] |
| PLR rocket men (`Template_TERR_RPG_*`) | barricade, SWAT van | RPG-7 | | never dropped (`DisableDeathPickup`, catalog 1.4) |
| Police (`SP_FR_Policeman_*`) | barricade, bus, police cars | G36C | none | **No row** |
| same | same | 870 MCS | none | **No row** (pump, shells one by one) |
| same | same | P90 or UMP-45 | none | P90 **no row** (top magazine); UMP-45 held |
| GIGN (`SP_PoliceSWAT_01`) | SWAT van, chase intro | UMP-45 | RX01, EoTech, RX01 + laser | gun held; **RX01 no row**; laser none |
| same | same | P90 | EoTech, RX01, EoTech + laser | no row |
| GIGN rifles (`SP_PoliceSWAT_01_ASR`) | same | G36C | EoTech, RX01, ACOG, + foregrip, ACOG + heavy barrel, laser | no row; **ACOG only on the M416** |
| same | same | SG 553 | ACOG, RX01, EoTech, foregrip, heavy barrel, laser + foregrip | **No row** (AK magazine + bolt catch) |

Counts: built as held guns 5 (AKS-74u, AK-74M, AEK-971, M1014, UMP-45); not built 8 (AS Val, F2000, MP7, PKP, G36C, SG 553, P90, 870 MCS). Sights: Kobra, PK-A
and EoTech rows exist; RX01 has none; the ACOG exists only as the M416's. Lasers and weapon
lights: none. Friends' guns (Vladimir's A-91, Kiril's AS Val) are never the player's.

A gun with no row "stays as the game draws it" [mod: `skin.cpp:9139`]: the game's own hold and
reload, no recoil of the mod's. With the arms unknown that is true of every gun here today.

**No launcher, no mounted gun, no vehicle the player uses** [data: the only vehicle spawns are two
enemy `Wz11_SP_Paris` police helicopters, scripted, one made immortal and destroyed by a sequence;
every car and van is a prop].

## 3. Play order

The level's own start points [data: `Flow.txt:2962-3215`] give the order: Ride, Gate, Parking
Deck, Headlights, Sprinkler Fight, Gas Event, Lower Office, Office Stairs, Upper Office, Atrium,
Trading Floor, Elevator Fight, Lobby, Barricade, Bus, Police Cars, SWAT Van, Traffic Jam /
Vladimir Dies, Chase Intro, Chase, Train Station. Nineteen save points sit along it.

| # | Section (save) | What happens | Input the game takes | Mod's verdict |
| --- | --- | --- | --- | --- |
| 0 | Intro movie | `SP_Paris_Interrogation` video, skippable, then save `Save_IntroStart` | skip | Flat movie as between other missions; not looked at here |
| 1 | **Ride** (`Rail_Schema.txt`) | A three-actor scripted scene in the car through the tunnel to the garage gate, 74 s to the next step; log lines for place and time; the jammer armed in the car | Look only (restriction `A3C961AE`: throttle to roll); everything back at the end (`42D593F9`) | **DONE, the owner's approval 2026-11-01**: head-turned scripted camera, body locked, body per eye. Three weeks of changes since: watch it once again |
| 2 | **Gate** (`ParkingDeckGate_Logic.txt:155`), save `Save_ParkingDeck` | Prompt `ID_H_INT_SP_PARIS_TO_CUT_WIRE`: Interact, **press**, radius 1.0 m, within 120 degrees, **occlusion tested**, one use. Then all input off (`BEC05C3F`), the game forces **stand** and **slot 7** (machete), a 4.63 s cut, the gate pushed open, then forces **slot 0** | E | Works by the Use button (Y / left B). Not yet by reach and held grip (no interaction reader in main). Checks: the 120-degree cone is the game's look direction, which the mod steers; the forced stand against the stick's stance; the forced slots against the holsters' "press the key again" |
| 3 | **Parking deck**: first encounter, the "bunker", headlights, sprinklers (saves `SAVE_FirstParkingDeckEncounterDone`, `SAVE_ParkingDeckPostBunker`, `SAVE_HeadlightsFightDone`) | Firefights among cars. At the bunker two men are unaware until the player fires, throws a grenade (a ThrowGrenade detector, `Parkingdeck_Logic.txt:1739`), walks in, or 30 s pass | play | Plain shooting. The grenade is optional (none by hand in the mod). Full-frame screen dirt in the garage's look (`VE_Paris_ParkingGarage_01`) |
| 4 | **Gas event** (`GasEventSluice_*`) | Prompt `ID_H_INT_SP_PARIS_BREACH_DOOR`: Interact, press, radius 2.0 m, any angle, enabled when the squad is ready; a save; the breach scene (player is an actor); gas. **3.0 s after the breach ends the game awaits QuicktimeInteractDrag (E); at 5.5 s the player is killed** [data: Delay `A182`, Sequence `CA03` KillPlayer at 5499 ms]. On E: the first-person scene of the mask going on (`SP_Paris_Player_Put_On_GasMask`), look only (`76F7B516`). Squad puts masks on | E, then E | Breach: Use button. Mask: QTE verdict **AS IS** (a standalone trigger the grips answer, 2.5 s window); the Use button sends E too. From here to the lobby's exit a **full-frame gas-mask overlay** (`VE_SP_Paris_LensRain_01_GasMaskOn`, shader `Fullscreen_GasMask_WithRain`) |
| 5 | Stairs after the breach (`GasEventSluice_Squad_Logic.txt:783`) | `Sp_Paris_Stairs_Fight`: a man at the railing; the fight prefab starts with the scene, all input off (`A77E63DC`) | E, one press; a miss is **not fatal** (Vladimir does it) [web] | **FIGHT** (the prefab reader in main). M1014 pickup in this area |
| 6 | **Lower office**, office stairs (saves `Save_LowerOffice`, `Lower office`, `Save_NukeRoom`) | Firefights, a PKP gunner | play | Plain shooting, in the mask |
| 7 | **Upper office, atrium** (save `Save_UpperOffice`) | Firefights; the AKS-74u and AK-74M pickups; a squad scene | play | Plain shooting |
| 8 | **Trading floor** (`Trading_Floor_Squad.txt:239`), saves before and `PostTradingFloor` | Prompt `ID_H_INT_SP_PARIS_OPEN_DOOR`: Interact, press, radius 2.0 m, any angle, enabled when the squad is ready; a save; the door-kick scene, look only (`F537D7A2`), then a large firefight (the save comes when all are dead) | E | Use button; scene by the scripted-camera handling |
| 9 | **Corridor, lift** (`CorridorSluice_Squad_Logic.txt`), save `Elevator Fight Save` | Walking in turns all input off (`9675A7E1`); the jammer checked in the player's hands (a scene); then the corridor fight (`SP_Paris_Gas_Mask_Fight`), the fight prefab | E, one press; a miss is **not fatal** [web] | **FIGHT**; its Stop is never wired, so the six triggers stay live afterwards: the reader must stay quiet in play (the survey's "never stopped") |
| 10 | **Lobby** (`Lobby_AssaultEvent.txt`) | A look-at trigger, the chandelier scene, all input off (`D7C8F46E`); "stay on mission" message at the edges | none | Scripted camera |
| 11 | **Lobby exit, barricade** (`Heat_BarricadeFight.txt`), save `Barricades Save` | `SP_Paris_LobbyExit`: all input off (`B1496627`), masks off (the overlay ends), out into the street; police and PLR fight; rocket men | none, then play | Scripted camera, then shooting. Rain on the lens outdoors (`VE_SP_Paris_LensRain_01`, full frame) |
| 12 | **Bus, police cars, SWAT van** (saves `Bus Save`, `Bus`) | Street fight; a police helicopter overhead (scripted); GIGN out of a van | play | Plain shooting; the first G36C, SG 553, P90, 870 drops |
| 13 | **Vladimir dies** (`HeatStreet_Logic.txt`) | Walking into a trigger turns all input off (`4F62BDC0`); a rocket, **slow motion** (a sequence drives the time scale), the bus blows up, a four-actor scene | none | Scripted camera. Slow motion against the mod's own timers (reload taps, buzzes, the 130 ms mash) is unchecked |
| 14 | **Chase intro, chase** (saves `Chase`, then `Train Station`) | Run after the bomb carrier. Fail "HE ESCAPED" on five wrong-way triggers, on leaving the area, and on **two 15 s timers**, each closed by reaching the next trigger [data: `Chase_Logic_Schematics.txt`, Delays `1E6F`, `801D`] | play, sprint | Sprint is the left stick's click, the way the head faces. Seated players must turn by stick while sprinting: try it |
| 15 | **Train station** (`Chase_Logic_Schematics.txt:1052`) | On the platform all input off (`FBDC5D0E`), the fight prefab: **three successes** counted (CountDowns 1, 2, 3), each failure sends the fail branch; the carrier goes under a train | E, left mouse, Space [web]; **a miss fails the mission** | **FIGHT**. Answerable today by Y, the trigger and A; the grips' real-squeeze failure (2026-11-19) is not fixed in main |
| 16 | **End** | `SP_Paris_EndScene_FakeNuke` (the empty case), the blast's rumble and camera shake, fades, level complete, loads `SP_Tank` | none | Scripted camera; then the tank mission, which waits |

## 4. Prompts, in one place

| Prompt | Key | Kind | Radius, cone | Today | The owner's rule |
| --- | --- | --- | --- | --- | --- |
| Cut wire | Interact (E) | press, hold time 0 | 1.0 m, 120 degrees, occlusion tested | Use button | reach to it, hold a grip: not built (needs a reader of the interaction entities) |
| Breach door | Interact (E) | press | 2.0 m, 360 degrees | Use button | same |
| Open door (trading floor) | Interact (E) | press | 2.0 m, 360 degrees | Use button | same |
| Three weapon pickups | HOLD [R] | hold | 2.5 m | X held, or the magnet pickup | built |

No prompt here uses a key other than E and R. No hold-to-interact, no Jump, Fire, Prone or
Clutch prompt, no Throttle climb.

## 5. QTEs, in one place

| Where | Awaited | Kind | Window, fail | Verdict |
| --- | --- | --- | --- | --- |
| Gas mask | E (0x3C), standalone trigger `8B214FD3` | one tap | opens 3.0 s in, **death at 5.5 s** | AS IS |
| Stairs, railing | E | fight prefab, one press | not fatal | FIGHT |
| Corridor by the lift | E | fight prefab, one press | not fatal; never stopped | FIGHT, then quiet |
| Metro platform | E, left mouse, Space | fight prefab, three presses | **mission failed** | FIGHT |

A wrong one of the six quick-time keys during an open prompt fails a prefab fight, so the guard
that keeps the mod's own aim, stance and fire keys off during a prompt matters at the platform.

## 6. Other things a VR player meets

- **No crawl, climb, ladder, vault or swim is required** [data: no climb or ladder entity; the 12
  vault waypoints are the AI's]. [inferred: the chase's route was not walked.]
- **Forced stance:** only the wire cut (stand). No forced crouch or prone.
- **Forced weapon:** the wire cut (slot 7, then slot 0). Whatever the player held, the game puts
  the AS Val back in hand afterwards.
- **Screen overlays, full frame, the same in both eyes:** garage dirt, the gas mask from the gas
  event to the lobby's exit (about half the mission), rain on the lens in the street. Drawn once
  per eye they sit at no depth; how they look in the headset is unknown.
- **Slow motion** at the bus.
- **Edges:** "STAY ON MISSION" messages at the lobby and the street; out-of-bounds kills are the
  game's own.
- **Checkpoint loads** fade in from black each time (a Fade on every save's load).
- **Hand grenades:** in the kit, never needed.
- **Sniper moments, counted kills, stealth:** none.

## 7. What is missing, in the order to do it

1. Dima's arms (`arms1p_rus_mesh`): the table, the fitted hands on each gun, sleeves and body.
2. The AS Val as a held gun, with its Kobra checked against the Kobra row.
3. One run on foot from the gate to the gas event on the game's own arms, to see what today's
   build does there at all (aim, the game's hold, holsters with a two-gun kit, the overlays).
4. Prompts by reach and held grip (shared with every mission): wire, breach, door.
5. The grips' real-squeeze fix for the fights, then the platform fight and the mask tap with a
   held grip.
6. The street's guns: G36C and SG 553 first (most GIGN rifles), then F2000, MP7, P90, PKP, 870.
7. RX01 row; the ACOG on held guns.
8. Checks: slow motion, forced stand and slots, the chase seated, the never-stopped corridor
   fight staying quiet, the overlays.

## 8. Not verified

- Nothing here was run. Every "works by the Use button" is the README's mapping plus the data's
  action, not a played prompt.
- How today's build behaves with Dima's arms and a held-kind gun (the AKS-74u) is not known: the
  gun may be moved to the hand while the game's arms stay where the game draws them.
- The dump is the 2022 public one; it was not compared with the local install.
- Prompt windows inside the fights are in the animations, not in the level data.
- The chase's route and the platform's approach were read as triggers, not walked.

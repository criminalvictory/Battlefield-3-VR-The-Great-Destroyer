# Night Shift (SP_Sniper) for VR: survey

Mission 9 of 12. You play Blackburn with Campo, at night in Tehran: shoot out the lights of a landing
zone, sneak through back streets, cover Cole's Marines from a roof, a knife kill that goes wrong, a
sewer, the room where Miller was killed, the strike on Al-Bashir's meeting, his car stopped with one
shot, a mall held against five waves, the Osprey.

Read only. Nothing here was run in the game. Written 2026-11-19 against main 7e99b44.

Marks: **[data]** the level's own files (the public Venice-EBX text dump, `Levels/SP_Sniper/*`, file
and line given); **[web]** the Battlefield wiki's Night Shift page; **[mod]** this project's code and
notes today; **[inferred]** my reading.

## The short answer

Night Shift cannot be finished in VR today. Three things stop it, all in the first five minutes and
all about the sniper rifle:

1. **The M40A5 is not a held gun**, and with free hands on the game's zoom is never pressed (the
   left trigger is not aim-down-sights while `hands::On()`, `src/input.cpp:2838`). So its scope
   cannot be looked through at all.
2. **Its 8.5x thermal scope has no row** in `kHeldSights`, and the second renderer has no thermal
   picture.
3. **The game checks that you are zoomed.** "Confirm Cole's IR signal" is a look-at trigger that
   wants the M40A5 at zoom level 1 within 5 degrees of the signal, and nothing else moves the mission
   on (`ReconNest_LOGIC.txt:612`). A held gun that never tells the game it is zoomed stops there.

Everything else is playable in principle with what the mod has (buttons answer the prompts, the
scripted scenes use the body lock), but none of it has been run. There are no quick-time triggers,
no vehicle to drive, no mounted gun and no launcher on the path.

## Play order

Sub-levels stream in this order [data: `SP_Sniper.txt:11480-12128` and the stream gates]: ReconNest,
Backstreets, Overwatch, RocketApartment (+ Missile, art only), FailKnife, Sewers, MannequinRoom,
Mall_BehindStreet, Mall and Briefing, Plaza, AlBashirStrike, MallDefence.

Checkpoints, by their save names [data]: StartSave, AfterLightsOut, BackstreetsStart, OverwatchStart,
OverpassStart, Rocket, BackstreetsStart (again, after the knife), Sewers, SewerEnd, PreMannequinRoom,
PreAssaultStart, Intercept, AfterIntercept, MallDefenceStart, MallDefenceWave, Extraction.

| # | Where | What happens | What the player does |
| --- | --- | --- | --- |
| 1 | Recon nest (roof) | A scene with Campo and binoculars, all input locked; then you may move but not fire until Campo has spoken | Watch |
| 2 | Recon nest | "Disable lights in parking lot LZ": four lamps on wires. A tutorial counts two shots and then says STEADY AIM (hold breath) | M40A5, scoped, four shots |
| 3 | Recon nest | The Osprey lands the Marines (they rope down). "Confirm Cole's IR signal" | Look at the signal **through the scope** (see What is missing) |
| 4 | Recon nest | "Eliminate lookout", then a technical's patroller: "Don't let PLR patroller get away" | Two or three scoped kills |
| 5 | Backstreets | Rappel prompt, a truck patrol to hide from, a sentry (an enemy sniper's area that kills you until he is dead), a wall climb prompt | Use, hide, one scoped kill, Jump |
| 6 | Overwatch | Second rendezvous scene (Campo and Cole), cover the Marines crossing; four truck patrols; hold fire until told | Scoped kills from a roof |
| 7 | Rocket apartment | The knife is forced into your hand; STEALTH KILL prompt behind a guard; a 13 s scripted struggle; then run | Fire within 3 s |
| 8 | Fail knife, sewers | Flee with Campo, hold fire, drop into the sewer; the MP7 is forced; guards talk above a grille | Follow, stay out of sight |
| 9 | Mannequin room | Campo opens a door; the Miller scene (Campo only); a patrol with lights comes through in five timed groups; a grenade | Hide or fight with the MP7 |
| 10 | Mall, briefing | Meet Cole (scene, AI only); Campo slides down a ladder | Follow |
| 11 | Al-Bashir strike | Overwatch over the Marines' assault: timed enemy groups, a technical with a gunner, two M40A5 pickups and an ammo crate | Scoped kills |
| 12 | Intercept | Al-Bashir's SUV flees. Reach the spot in time; you are forced to a crouch, movement locked, the view pulled to the car, time slowed | One shot into the SUV |
| 13 | After the crash | The MP7 is forced; defend Campo and Al-Bashir at the wreck | MP7 |
| 14 | Mall defence | BREACH prompt at the mall door; the claymore kit; five waves; the Osprey | Use, two claymores, hold |
| 15 | Extraction | An 85 s scene aboard the Osprey (Al-Bashir dying), look only | Watch |

## Guns

### The kit

Spawn template `SpHumanSoldier_SP_Sniper`, kit `UsHumanSpSoldier_SP_Sniper` [data:
`AB01_SquadLogic.txt:293`; the kit's file in the dump].

| Slot | Gun | Blueprint | Attachments | Held gun in the mod | Reload in the mod |
| --- | --- | --- | --- | --- | --- |
| 0 (active) | M40A5 | `Weapons/M40A5/U_M40A5` | `U_M40A5_SP_SNIPER_NVG`: the campaign-only night scope (the game's "10x" thermal, FOV 7 = 8.5x), suppressor, straight-pull bolt rules, the campaign sniper bullet | **No** (no `kHeldKinds` row) | None. The game's own (X held = R) |
| 1 | MP7 | `Weapons/MP7/U_MP7` | EoTech, suppressor, extended magazine (40) | **No** | None. The EoTech's sight row exists (built for the M4A1) and would serve once the gun is held |
| 7 | Knife | `Weapons/Knife/U_Knife` | none | No (drawn the game's way) | none needed |

No grenades in the kit.

### Scripted kit changes [data]

| When | Kit | What it does | File |
| --- | --- | --- | --- |
| Mall defence start (save MallDefenceStart, on save and on load) | `UsHumanSpSoldier_SP_Sniper_Claymores` | Adds the M18 Claymore in slot 4 and selects it; a state trigger sets its ammo to 2 | `MallDefence_SQUAD.txt:89`, `:12` |
| Sewers | `UsHumanSpSoldier_SP_Sniper_FlashlightOn` (the MP7 with a flashlight added, selected) | The entity exists but **no event applies it** in the dump: only its player link is wired. Whether the light comes on in the game is open | `Sewers_SQUAD.txt:53` |
| Stairs guard (an enemy, not you) | `PLR_Sniper_KnifeKill`: AK-74M + knife | The guard's own kit | `RocketApartment_AVOID_GUARDS.txt:134` |

### Forced weapon changes [data]

The game changes the gun in your hand by itself four times. Each one goes past the holsters.

| Where | What | File |
| --- | --- | --- |
| Rocket apartment, a trigger before the guard | Input restricted (no weapon change, no reload, no jump, no melee key), then 0.5 s later the active slot is set to 7: the knife | `RocketApartment_AVOID_GUARDS.txt:1551`, `:32` |
| Sewer entrance | If you are not holding the MP7, the game fires a SelectWeapon2 input itself | `Sewers_SQUAD.txt:2`, `:656` |
| After the SUV crashes | Active slot set to 1: the MP7 | `AlBashir_Jump.txt:63` |
| Mall defence | The claymore kit selects slot 4 | above |

### Floor guns

| Gun | Where | Slot | Attachments | Held in the mod |
| --- | --- | --- | --- | --- |
| M40A5 x2 | The Al-Bashir strike overwatch, 50 rounds each, radius 2.5 m, shown on the minimap | 0 (alternative 1) | the same night scope unlock | No |

`AlBashirStrike_LOGIC.txt:15867`, `:15947`. Ammo crates: three (recon nest, radius 8.5 m; one in
`Default.txt`; one at the strike overwatch) [data].

### Drops (from the weapons catalogue, `docs/research/weapons-catalog.md:400-424`)

| Gun | Attachments | Held in the mod | Reload |
| --- | --- | --- | --- |
| AK-74M | random Kobra, PK-A, PK-AS, PSO-1; many with weapon lights | Yes (`ak74`) | by hand |
| KH2002 | as in Uprising | Yes (`kh2002`) | by hand |
| G3A3 | as in Uprising | Yes (`g3a3`) | by hand |
| AEK-971 | bare or random | Yes (`aek971`) | by hand |
| RPK-74M | bare or random | Yes (`rpk`) | by hand |
| AKS-74u | weapon light with bare, Kobra or PK-AS | Yes (`aks74u`) | by hand |
| M16A4 | bare (fallen Marines) | Yes (`m16a4`) | by hand |
| Saiga 12K | bare or Kobra | No | the game's |
| PKP | bare; one with PKS-07 | No | the game's |
| 870 MCS | weapon light (Takavar shotgunner) | No | the game's |
| SVD | PSO-1 | No; the snipers stand outside the playable area | n/a |

The enemies' sights on a held gun (PSO-1, PKS-07, PK-A, PK-AS, Kobra) are all built. The weapon
light on a picked-up gun is not (the catalogue lists the flashlight as not built).

### Launchers, mounted guns, vehicles

- **Launchers:** none. The "rocket" of the rocket apartment is a scripted explosion; the level fires
  invisible AK-74s and an invisible grenade as props [data].
- **Mounted guns:** none on the path. Friendlies carry an M240B. The technical's gun is the enemy's;
  the wiki describes an out-of-bounds jump that reaches it [web]. Not needed.
- **Vehicles:** none driven or ridden. Enemy trucks (Star 1466 with driver and guards, six spawns),
  one technical, Al-Bashir's SUV (an animation), two Ospreys (animations) [data].

## Scripted scenes with the player's body

Every one of these sends the player's own animation (the level's `TestAnimation` event on the player
spawn) and locks input. The mod's rule for all of them is the same: scripted camera, body locked to
it and drawn per eye, head free (built, generic). None has been seen on this level.

| Scene | Input while it plays | File |
| --- | --- | --- |
| Roof observation at the start (binoculars prop) | everything off, then move-only until Campo's line ends | `ReconNest_SQUAD.txt:693`, `:772` |
| Rappel to the street, with Campo, three ropes | look only | `Backstreets_SQUAD_OBJECTS.txt:527`, `BackStreets_SQUAD.txt:914` |
| Wall climb, with Campo | fire and look only | `Backstreets_SQUAD_OBJECTS.txt:611`, `BackStreets_SQUAD.txt:993` |
| Knife struggle with the stairs guard, 13 s | everything off; the guard's shot sets your health to 25 | prefab `SP_Sniper_failed_knife`; `RocketApartment_AVOID_GUARDS.txt:1472`, `:83` |
| Mall door breach | none read | `MallDefence_LOGIC.txt:203` |
| Extraction aboard the Osprey, 85 s | look only | `MallDefence_WAVE4.txt:1150`, the `MV-22B_Osprey_Prefab_Sniper` scenario |

Scenes without the player's body (AI only, you stand and watch): the Marines roping from the Osprey,
the second rendezvous, the guards talking in the sewer, Campo opening the door and the Miller room,
the meeting and the briefing with Cole, Campo's ladder climb and ladder slide, Al-Bashir captured.

Before the level a movie plays (`Movies/SP_Sniper_Interrogation`).

## Prompts

There is **no quick-time trigger** in this level (no action 0x3C to 0x41), so neither QTE version 2
nor the fight reader has anything to answer [data; the campaign survey agrees]. All five prompts are
interaction entities.

| Prompt | Key | Hold | Radius | Uses | What it starts | The mod today |
| --- | --- | --- | --- | --- | --- | --- |
| `TO_RAPPEL` (Backstreets) | Interact (E) | 0 | 2.0 m | many (turns itself off) | the rappel scene | The use button sends E: should work. Reach-and-hold-grip is not built for it |
| `TO_CLIMB_WALL` (Backstreets) | **Jump (Space)** | 0 | 3.0 m | many (turns itself off) | the wall climb scene | The jump button (or the right stick's click) sends Space: should work. Not a grip |
| `STEALTH_KILL` (stairs guard) | **Fire (left mouse)** | 0 | 5.0 m | 1 | the knife struggle | The right trigger sends left mouse. Open: with the knife forced into the hand and drawn the game's way, does the pull reach the game? Nothing in `FireButton` holds it off for a knife [mod] |
| `TO_BREACH` (mall door) | Interact (E) | 0 | 1.0 m | 1 | the breach scene | Use button: should work |
| `PRESS_E_TO_EXIT_SEWER` | Interact (E) | 0 | 1.0 m | 1 | nothing | Dead: disabled from the start and nothing enables it (`FailKnife_SQUAD_LOGIC.txt:293`) |

Files: `Backstreets_SQUAD_OBJECTS.txt:423`, `:475`; `RocketApartment_Logic.txt:1852`;
`MallDefence_LOGIC.txt:151`.

**The stealth kill's window.** Entering the trigger behind the guard opens the prompt. One second
later the guard's senses come on and a 2.0 s delay starts; when it ends the prompt closes, and 1.0 s
after that the level kills you. So about 3 s from stepping in to pulling the trigger. Leaving the
area closes the prompt too. The wiki adds that coming up crouched fails it [web]: a real crouch in
the room makes the game crouch (`headmove::CrouchWanted`), so a ducking player can fail it without
knowing why.

**Input triggers** (`ReconNest_SQUAD.txt:1403`, `:1418`; `Sewers_SQUAD.txt:656`): Fire 0x07 and
BreathControl 0x2D in the lights tutorial (they only drive the STEADY AIM hint), and SelectWeapon2,
which the game presses itself. None needs an answer.

## What the game checks about your aim

Three look-at triggers want the **M40A5 in hand, zoomed (zoom level 1)** and aimed at a point:

| Where | Cone | Occlusion | What it does | Needed to go on? |
| --- | --- | --- | --- | --- |
| Recon nest, Cole's IR signal (`ReconNest_LOGIC.txt:612`) | 5 degrees | no | starts the signal's sequence and the next step | **Yes: the only way on** |
| Overwatch (`Overwatch_GUARDS.txt:456`) | 1 degree | yes | stops a marker sequence 4 s later | No |
| Al-Bashir strike (`AlBashirStrike_LOGIC.txt:7456`) | 10 degrees | no | starts the assault | No: a 4 s delay does the same |

Three more at the wreck and one in the last wave need no weapon or zoom (5 and 10 degrees): they
only hold back enemy spawns while you look at the spawn places.

The game also **pulls your view** with its `ForcePlayerLookAt` prefab: twice in the recon nest and
once at the SUV (`ReconNest_LOGIC.txt:888`, `:1265`; `AlBashirStrike_LOGIC.txt:8377`). The mod writes
the look angles itself, so who wins there is open.

## Other things a VR player meets

- **Darkness.** The whole level is night. The flat game's scope is thermal; the wiki speaks of heat
  signatures seen through it [web]. A plain magnified picture will be very dark.
- **The bolt.** The night scope unlock switches the M40A5 to straight-pull rules (the bolt is worked
  without leaving the scope). A turn bolt by hand is new for the mod (catalogue 4.3 i).
- **Hold your fire.** A `PlayerFireCheck` watches for a shot in five stealth stretches (back streets,
  overwatch, rocket apartment, fail knife, missile). A shot there alerts the patrols; in the rocket
  apartment it shows HOLD YOUR FIRE. A stray trigger pull while handling the gun counts.
- **Stay hidden.** Back streets: an area that deals 200 damage while the enemy sniper lives ("You
  were made, don't break cover"). Sewers: an area that deals 200 damage, and two invisible rifles
  firing, if you walk into the patrol's sight ("Stay out of sight").
- **Stay close to Campo.** Leaving him fails the mission (recon nest, back streets, mall defence).
  Friendly fire fails it.
- **Timers.** The stealth kill, about 3 s. After the struggle, 15 s (16.5 s past a second trigger)
  to be moving along the escape, or invisible rifles open fire and the level kills you. Al-Bashir's
  car: 15 s to the first trigger, then 10 s between each of two more, or "Al-Bashir escaped, you
  were too slow"; then one shot into the SUV before its animation's end. The wiki notes a bug where
  the car crashes and the mission fails anyway [web].
- **Forced stances.** A forced crouch at the SUV shot (with movement, jump and stance locked: look,
  fire, zoom and reload only); forced standing at five checkpoint loads. The mod's stick stance
  keeps its own idea of crouch and prone, so the two can disagree after a forced one.
- **Slow motion** at the SUV shot (a `SlowMotionEntity` on a synced sequence, `AlBashir_Jump.txt`).
- **Claymores start the waves.** Wave 1 begins the moment you are no longer holding the claymore
  (polled once a second): after the second one is placed, or as soon as another gun is drawn
  (`MallDefence_VO.txt:2`). Drawing the rifle from the shoulder right away skips the claymores.
- **No ladders, crawls or climbs of your own.** The two ladder animations are Campo's. The rappel
  and the wall are the two prompts above. No prone prompt in the data.
- **Enemy flashlights.** Patrols carry weapon lights and switch them off when killed; picked-up guns
  have them too.

## What is missing

Blocking:

1. **The M40A5 as a held gun** (kit slot 0 and two pickups): body, palms, sight line, the bolt worked
   by hand (straight pull under this unlock), its box magazine by the paddle, the suppressor's
   muzzle. Hard.
2. **The night scope's row** in `kHeldSights` (eyepiece, radius, power 8.5 from the game's FOV 7,
   its marks). Medium once the gun is measured.
3. **Telling the game it is zoomed** while the eye is at the M40A5's scope, so the IR-signal trigger
   fires (zoom level 1, within 5 degrees, by the game's own aim). The Stinger's "zoom held when the
   tube is at the eye" is the pattern. What the game's zoom does to the picture with a held gun (FOV,
   the zoomed model, the thermal look over the whole view) must be handled. Medium.

Not blocking, but a player will hit them:

4. The thermal picture inside the glass (the second renderer draws a normal picture). Hard.
5. The MP7 as a held gun: magazine in the grip, T-handle, bolt catch by the thumb. Medium to hard.
6. The claymore: taken from the right chest and set by hand; the mod has no key for slot 4 (1, 2 and
   4 only), so once switched away it cannot be brought back. Medium.
7. The knife moment: check the trigger answers STEALTH KILL with the knife forced in, inside 3 s,
   and that a real crouch does not spoil it. Small.
8. Forced weapon changes against holsters-by-hand (knife, MP7 twice, claymore). Small to medium.
9. Reach-and-hold-grip for the three live prompts (rappel and breach by E; the wall by Jump). Small
   each once the AT4 branch's way is merged; the Jump and Fire keys are new for it.
10. `ForcePlayerLookAt` and the forced crouch at the SUV shot against the mod's look writes and
    stick stance. Small to check.
11. The six player scenes with the body lock and per-eye body. Check only.
12. Saiga 12K, PKP and 870 MCS as held guns (drops). Shared with other missions.
13. The weapon light on held guns. Shared with other missions.
14. Whether the sewer's flashlight kit is ever applied.

## Sources

- Venice-EBX, `Levels/SP_Sniper/*`, `Characters/Soldiers/Customizations/*`, `Weapons/M40A5/*`.
- `docs/research/weapons-catalog.md` section 2.9 and its tables.
- The campaign QTE survey (workflow wf_fe217b53-80e, third result), Night Shift's rows.
- `src/skin.cpp` `kHeldKinds`, `kHeldSights`; `src/input.cpp` (the button table, the left trigger's
  rule, `FireButton`).
- [Night Shift on the Battlefield wiki](https://battlefield.fandom.com/wiki/Night_Shift).

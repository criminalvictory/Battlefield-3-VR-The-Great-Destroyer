# Uprising (SP_Earthquake2) for VR: survey

Surveyed 2026-11-19 from the public Venice-EBX dump (`Levels/SP_Earthquake2/*`, the kits in
`Characters/Soldiers/Customizations/*`, the AI templates in `AI/AI_Templates/*`) and the mod's code on main
(7e99b44). **Nothing was run in the game**: every "works" below is read from data and code, not seen in the
headset. Tags: [data] the dump, [code] the mod's source, [inferred] my reading, [earlier] the campaign QTE
survey and `docs\research\weapons-catalog.md` section 2.3.

Blackburn, alone, at night, right after Swordbreaker's earthquake. The mission in one line: dig out with only
a knife, crawl past patrols, kill one guard by a prompt and take his suppressed AKS-74u, fight back through
the bazaar, the parking lot, the rift, the school, meet Montes, ride a Humvee's .50 for about 50 seconds, run
to the Osprey.

## 1. What is missing (short list)

| # | What | Kind | Where | Blocks finishing? | Effort |
|---|---|---|---|---|---|
| 1 | The rat's quick-time press (left mouse, one press, a miss kills and reloads the crawl) answered in the headset. The fight reader exists; real grips failed on Swordbreaker on 2026-11-19 (fix running); the trigger won a Fire prompt there. Unproven here | qte | Meat Market, the pipe | Yes until proven | small once the grip fix lands: one test |
| 2 | The game's real stance. Standing where the patrol area checks the pose fails the mission after 1 s; the level also forces crouch twice. The mod's stance is a belief that drifts ("flick up crouches", fix running) | other | Meat Market (crouch, pipe, patrols), Rift | Yes while the drift bug is in the build | the running fix, then a test here |
| 3 | The Humvee's gunner seat: getting in is an objective, then about 50 s on the .50 until a scripted rocket ends it. The mod has no mounted gun or vehicle-seat aim yet (pipeline item 11; the owner's rule: both grips on the handles) | mounted | Staging Area | Yes until a seat is at least playable (view level, gun turning, fire) | large for the two-grip gun; medium for a first playable seat |
| 4 | The stab prompt: an interaction on FIRE (radius 1.5 m, in front, one use) with the knife as the only weapon. The right trigger should answer it; never tried with a knife in hand. The owner's rule (reach and hold grip) is not built for it | prompt | Bazaar door | Yes if the trigger does not reach it (the chance passes) | small to prove; medium for reach-and-grip |
| 5 | Use prompts by reaching and holding grip (the owner's rule 2026-11-18). Today they work only by the use button (E): bazaar door, van push, school gate, school breach, gunner seat | prompt | 5 places | No (the use button presses E) | medium, shared with every mission |
| 6 | The crouch prompt: Left Ctrl HELD 1.0 s (Clutch). The stick's crouch is X (toggle), so the prompt stays on screen; it gates nothing | prompt | Meat Market | No | small: hold Ctrl 1 s when a Clutch prompt is near and the stick crouches |
| 7 | Saiga 12K as a held gun (bare or Kobra) | gun | drops: Bazaar, Parking Lot | No | medium (measured on Swordbreaker; its magazine was not drawn after a swap) |
| 8 | 870 MCS as a held gun: pump by hand, shells by hand, weapon light | gun | one drop: the Takavar shotgunner, Parking Lot | No | medium to large (the pump is a new mechanic; shells as the M1014) |
| 9 | PKP Pecheneg as a held gun, belt reload | gun | drops: Staging Area gunners | No | medium (measured on Swordbreaker; the M249's belt code is the model) |
| 10 | ACOG and RX01 on the G3A3 (no row in kHeldSights; the ACOG exists only as the M416's own) | scope | G3A3 drops, every area | No | medium for the ACOG (a magnified scope needs the second renderer's picture), small for the RX01 |
| 11 | The knife as a thing in the hand. From the dig-out to the stab it is the only weapon; the game draws it at the view (no hold of the mod's) | other | Meat Market to the bazaar door | No | medium |
| 12 | The weapon light on the AKS-74u (and on picked-up guns). The light belongs to the game's gun; with the gun moved into the hand the beam may stay on the game's aim | other | whole mission, at night | No | unknown: check first |
| 13 | The suppressor on the AKS-74u: a part drawn apart with no table row. The rigid-part rule should carry it after half a second; the muzzle point (flash, smoke) is the bare gun's, at 71 cm | other | whole mission | No | small |
| 14 | The van push: after E, six presses of E (quick-time InteractDrag, counted on release). Optional: killing the 8 guards clears the area too | qte | Parking Lot | No | none if the mash works as on the other missions; one test |
| 15 | M145 on the M240 (Montes's gun; only if he could drop it) | scope | Cafe on | No | small; likely never seen |

## 2. Play order

Starts (Flow.txt): Level Start, Meat Market, Bazaar, Parking Lot, Rift, School, Alley, StagingArea. Saves
(checkpoints): Meat Market (after the dig-out), Bazaar (at its start and at the door), Parking Lot, the Large
Building's garage (after the lot is cleared), School, Staging Area. [data]

### 2.1 Level start: the interrogation film and Solomon

- A film (`Movies/SP_Earthquake2_Interrogation`) with the game's cutscene input lock. [data]
- `Solomon_Intro`: a camera entity takes the view (`Solomon_Intro_LOG.txt:89`), run by a sequence on the
  voice-over's clock; every input off, look included (`Solomon_Intro_SCM.txt:86`). Three actors: Solomon, a
  PLR soldier, a Marine. Ends by itself. [data]
- **Verdict:** a camera cutscene: 3D, body on the game's camera, head free, as built. Nothing to press.

### 2.2 Meat Market: dig out, crawl, the pipe, the rat, the patrols

Kit: `UsHumanSpSoldier_SP_Earthquake2_Knife`: the knife alone, slot 0 (`MeatMarket_SCM.txt:333`). [data]

1. **Dig yourself out** (`Prefabs/anim/DigYourselfOut`): a first-person animation with the player as its
   actor, fade in, then the checkpoint. On the save a PLR soldier shoots a hurt Marine on the ground. [data]
   Verdict: a scripted first-person animation, as the mod's other scripted moments.
2. **Sprint is off** from here (`MeatMarket_SCM.txt:2251`). [data]
3. **"Crouch" prompt** (`MeatMarket_LOG.txt:920`): action Clutch (Left Ctrl, "crouch and hold" in
   `Input/InputConfiguration.txt:171`), HELD 1.0 s, radius 2.0 m, one use. Using it only arms a trigger that
   switches jump, stance change, sprint and prone off under the wreck (`:2172`). It gates nothing. [data]
   Verdict: the stick's crouch is X (the toggle), so the prompt is not answered and stays up; the player
   crouches anyway. [code: `src\input.cpp:443`]
4. **"Prone" prompt at the pipe** (`MeatMarket_LOG.txt:972`): action Prone (Z), no hold, radius 2.0 m. Its
   only output is a level event with no listener in the dump ("EntersPipe"). [data] Verdict: the stick's
   flick-and-hold sends Z: answered, and not needed anyway.
5. **Forced crouch** at a trigger by the pipe (`MeatMarket_SCM.txt:189`, set pose). [data] The mod's stance
   belief does not hear it. [code + the stance note]
6. **The rat** (`MeatMarket_SCM.txt:2330`, trigger volume A908): the shared quick-time prefab starts with a
   first-person animation (the player is actor 1); all six quick-time actions live; the animation asks for
   **left mouse, one press**; a miss: death, back to the Meat Market checkpoint (the crawl again). Never
   stopped afterwards: the six triggers stay live for the rest of the level. [data + earlier] Verdict: FIGHT
   in the reader (`src\qtefight.h`): it presses only while a prompt is open. Open: item 1.
7. **The patrols:** six guards with lights (`Prefabs/Logic/EnemyGuardFlashlight`, AK-74M with a guard
   light), three looping groups (AKS-74u with torches), a truck that unloads four (AK-74M/KH2002/G3A3 with
   torches), the PLR briefing scene with an SVD sniper and Solomon's man (canned actors, nothing drops), PLR
   finding a dead Marine. A guard that sees the player, a guard killed, or **the player standing** inside the
   patrol area (pose checked every 1.0 s, `MeatMarket_SCM.txt:240`, `:1992`) raises the alarm: the guards
   turn, 1 s later the mission fails. [data] Verdict: item 2.
8. Leaving the market streams the bazaar in; "RETRACE STEPS TO BAZAAR" shows.

### 2.3 Bazaar: the door, the stab, the first gun

1. Kit at the start: `PlayerUnarmed` (the knife in slot 0), a save. [data]
2. **"Open" prompt** (`Bazaar_LOG.txt:3221`): Interact (E), radius 2.0 m, any angle, one use. The player is
   made immortal, a save 0.15 s later, then `Prefabs/anim/Bazaar_OpenDoor`: a first-person animation. [data]
   Verdict: the use button presses E. Reach-and-grip: item 5.
3. **"Stab" prompt** (`Bazaar_LOG.txt:3273`): action **Fire**, radius 1.5 m, within 180 degrees, one use, no
   hold. It starts `Prefabs/anim/UnarmedStealthKill` (first person, the guard as its second actor), wakes the
   other three guards' senses, and gives the kit `UsHumanSpSoldier_SP_Earthquake2_Silenced`. If the guard's
   own scene ends first, a splitter switches the prompt off: the chance has passed. [data] Verdict: not a
   quick-time trigger, so no grip answers it; the right trigger is the game's fire and should. Item 4.
4. A check every 0.1 s whether the knife is still the weapon in hand; the first "no" completes "SECURE A
   WEAPON" (`Bazaar_SCM.txt:2`). [data] A held AKS-74u in VR is the game's active weapon too, so it passes.
   [inferred]
5. The fight through the bazaar: extremists (AEK-971 bare, AKS-74u bare), Saiga 12K men (bare or Kobra), RPK
   gunners, three "talk to ready" guards. [data]

### 2.4 Parking Lot

- A save on entering; on a load from this checkpoint the Silenced kit is applied again, all other guns
  removed (`ParkingLot_SCM.txt:25`). [data] A floor gun carried in is kept only when walking in, not on a
  restart from here.
- Eight guards (Takavars with KH2002 and Kobra, one with an 870 and a light, AK men, Saiga men, an
  extremist), four "guard spots the player" scenes, a check for the player's own shots, an ammo crate
  (`Gameplay/Logic/SpAmmoCrateWithTooltip`), a 5 s out-of-bounds fail. [data]
- **"Push" prompt** (`Prefabs/Logic/PushObject_1p.txt:132`): Interact (E), radius 2.0 m, one use, switched
  on 2 s into the area. Then fire, grenades, gadgets and the weapon switch are off; 1.3 s later a quick-time
  trigger on InteractDrag (E) counts six presses, each on its RELEASE; the sixth rolls the van onto the
  guards below. No fail, no timer. [data + earlier] Verdict: E by the use button; the mash is the grips',
  as on Guillotine's blocked door. Optional: the gate that opens the garage is "all eight dead", however
  they die.
- "CLEAR THE AREA OF ENEMIES", then the garage: a guard opens it from inside (a scene), a save inside.

### 2.5 Large Building (the garage) and the Rift

- Garage doors rumble and close (scenes, no input). Two Takavars, an RPK gunner. [data]
- **Rift:** at a trigger, movement is off for 0.5 s, the game forces **crouch**, a first-person stumble plays
  (the aftershock), an unseen AK fires a burst; three AK men, two extras the quake kills after 6.5 s; a 5 s
  out-of-bounds fail. [data] Verdict: a scripted moment; the forced crouch is item 2.

### 2.6 School

- A save on entering; the Silenced kit again on a load. [data]
- **"Open" prompt at the gate** (`School_LOG.txt:308`): Interact (E), **radius 1.0 m**, one use. Every input
  is off while `Prefabs/anim/School_OpenDoor` plays in first person. [data]
- **"Breach" prompt** (`Prefabs/Logic/BreachDoorSchool.txt:66`, text `ID_H_INT_SP_BANK_BREACH`): Interact
  (E), radius 2.0 m. The game switches to slot 0, a first-person kick plays, the man behind the door has his
  own scene. Two extremists (AEK-971). [data]
- Verdict: both by the use button; reach-and-grip is item 5. The 1.0 m radius is the smallest of the level.

### 2.7 Alley and Cafe: Montes

- On entering the cafe: fire, weapon switch, melee and grenades off for 14 s (`Cafe_SCM.txt:377`); the
  "meet Montes" scene (he has an M240 with the M145); he opens a garage door (a scene); "FOLLOW MONTES".
  [data] Verdict: a scene while the player keeps the view: nothing to press. The trigger stays dead for
  14 s: expected, not a fault.

### 2.8 Staging Area: the Humvee's .50, the rocket, the Osprey

1. A save; friendly Marines (M16A4, one M249 gunner, a wounded man dragged), enemies that respawn from 11
   points (AK-74M/KH2002/G3A3 mix, extremists, PKP gunners, one RPG man whose launcher never drops). [data]
2. "REACH EXTRACTION POINT", then **"Enter gunner seat"** (`StagingArea_LOG.txt:912`): Interact (E), radius
   1.5 m. The player is moved into `Vehicles/HumveeArmored/spec/HumveeArmored_hmg`; fire, seat and stance
   changes are off until the climb-in animation ends, then only seat and stance changes stay off (no way
   out). [data]
3. **Timers, not kills:** 35 s after getting in (20 + 15) the Osprey's event starts; about 15 s later a
   32.5 s sequence: an unseen RPG hits (the player is made immortal), fade out at 19.9 s, the player is moved
   out, the Humvee destroyed, a first-person get-up animation, fade in: "HOLD OUT FOR OSPREY" done. [data]
   So about 50 s on the gun; nothing in the data makes the player immortal before the rocket. [inferred]
4. "REACH OSPREY": a trigger volume; on entering, fire and grenades are off and the extraction scene plays in
   first person. End of the mission. [data]
- Verdict: item 3. The seat is the only vehicle of the mission; there is no driving.

## 3. Guns

The player's kit, from the stab to the end (`UsHumanSpSoldier_SP_Earthquake2_Silenced`): slot 0 AKS-74u
(`Weapons/AKS74u/U_AKS74u`) with suppressor, Kobra and flashlight; slot 6 M67; slot 7 knife. **No secondary,
no launcher.** [data]

| Gun | Blueprint | How it reaches the hands | Attachments | In the mod | Reload |
|---|---|---|---|---|---|
| Knife | `Weapons/Knife/U_Knife` | kit, slot 0 until the stab | none | not held: the game's draw | none |
| AKS-74u | `Weapons/AKS74u/U_AKS74u` | the stab's kit, slot 0 | suppressor, Kobra, flashlight | held (kHeldKinds 17991, "aks74u"); Kobra in kHeldSights; suppressor and light have no row | magazine by hand, rocked in, knock-out, handle (the AK block) |
| AKS-74u | same | drops: extremists (bare), torch men (bare, Kobra or PK-AS) | as listed | held; PK-AS in kHeldSights | same |
| AK-74M | `Weapons/AK74M/U_AK74M` | drops | bare, PSO-1, PK-A, PK-AS, Kobra; torch; guard light | held (21723); all four sights in kHeldSights | AK block |
| KH2002 | `Weapons/KH2002/U_KH2002` | drops (Takavars always Kobra) | bare, PK-AS, Kobra, PK-A, PSO-1; torch | held (15237) | built |
| G3A3 | `Weapons/G3A3/U_G3A3` | drops | bare, **ACOG**, **RX01**; torch | held (23064); ACOG and RX01 not in kHeldSights | built |
| AEK-971 | `Weapons/AEK971/U_AEK971` | drops (extremists) | bare | held (21585) | built |
| RPK-74M | `Weapons/RPK/U_RPK-74M` | drops (bazaar, garage) | bare, bipod, Kobra + foregrip, PK-A + bipod | held (24492) | built |
| Saiga 12K | `Weapons/SAIGA20K/U_SAIGA_20K` | drops | bare or Kobra | **not held** | to build |
| 870 MCS | `Weapons/Remington870/U_870` | one drop, Parking Lot | torch | **not held** | to build: pump, shells |
| PKP | `Weapons/Pecheneg/U_Pecheneg` | drops, Staging Area | bare | **not held** | to build: belt |
| M16A4 | `Weapons/M16A4/U_M16A4` | fallen or friendly Marines | bare | held (17013) | built |
| M249 | `Weapons/M249/U_M249` | the Marine gunner, if he falls | bare | held (merged 620ba08, not yet run in the game) | belt by hand |
| M240 | `Weapons/M240/U_M240` | Montes's; no fair way to take it | M145 | held (27588); M145 not in kHeldSights | built |
| M67 | `Weapons/M67/U_M67` | kit, slot 6 | | grenades by hand (built) | |
| SVD, .44, RPG-7 | | never: canned actors, no drop component | | | |

The level's bundles also carry the M416, the SMAW, the AN-94, the SCAR-L and the AUG: an unused second spawn
with Rock and a Hard Place's kit and stray references. Not in play. [earlier + data]

Holsters: one primary and no secondary, so every floor gun takes the AKS-74u's slot; the holsters-by-hand
rules cover that. The kit is applied again (all guns removed) on a load at the Parking Lot and the School.

Mounted: the Humvee's M2 .50 (section 2.8). Launchers: none for the player. Vehicles: none to drive.

## 4. Scripted first-person animations (the player is an actor)

Dig out; the pipe and the rat; the bazaar door; the stealth kill; the van push; the rift stumble; the school
gate; the breach kick; the Humvee climb-in; the get-up after the rocket; the Osprey. Eleven, none with a
choice. Each is the kind the mod already locks the body for; none has been watched on this level.

## 5. Stances, timers, fails

- Forced crouch: by the pipe, in the rift. Checked pose: standing in the Meat Market patrol area = alarm,
  mission failed 1 s later. Sprint off through the Meat Market.
- No ladders, no climbs on Throttle, no jump prompt, no sniper kills like Swordbreaker's roof. [data]
- Out-of-bounds, 5 s: Parking Lot, Rift, Staging Area.
- Friendly fire fails the mission (`Setup.txt`; the Marines at the end).
- The level's arms are `arms1p_bareglove03` (Swordbreaker's and Rock's gloves) with a dirt texture: the
  gloves' draws should be recognised as on those levels. [data + inferred]

## 6. Tests to run, in order

1. From "Level Start": the film, Solomon, the dig-out. Body lock, no crash.
2. The crouch and prone prompts; then stand up on purpose in the patrol area and read the fail.
3. The rat: a held grip, a fresh grip, the trigger. Then squeeze a grip in free play: nothing pressed.
4. The bazaar door by the use button; the stab by the trigger with the knife up; the AKS-74u in the hands
   with its suppressor, Kobra and light; where the light's beam points when the gun is turned by hand.
5. Each drop picked up once: Saiga 12K, 870, PKP (drawn the game's way today), G3A3 with ACOG and RX01.
6. The van: E, then the grip held: six counted.
7. The school gate at 1.0 m, the breach.
8. The gunner seat: the view, what turns the gun, fire, the rocket, the get-up, the Osprey.

Scratch files: `C:\Users\richy\AppData\Local\Temp\claude\91417f42479c2553\bcf52130-5a75-49e4-b525-073bebec299f\scratchpad\missions\uprising\`
(kits and AI templates fetched from the dump, per-area event graphs `s_*.txt`, prompts `gi.txt`, helper
scripts `ent.py`, `evq.py`).

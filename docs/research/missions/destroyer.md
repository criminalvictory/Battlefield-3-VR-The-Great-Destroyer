# The Great Destroyer (SP_Finale) for VR: survey

Read-only research, 2026-11-19. Nothing was run in the game. Sources:

- **[data]** the public Venice-EBX dump of `Levels/SP_Finale` (entities and their event connections,
  read with the scratch index `scratchpad\missions\destroyer\fin.pkl`; kits and enemy templates
  fetched from the same dump's `Characters/Soldiers/Customizations` and `AI/AI_Templates`).
- **[mod]** the code in main at 7e99b44 (`src\skin.cpp` kHeldKinds / kHeldSights, `src\input.cpp`,
  `src\qtewatch.cpp`, `src\qtefight.h`, `src\barestate.h`) and the project's notes.
- **[web]** one screenshot walkthrough (portforward.com) and the earlier campaign QTE survey's
  walkthrough half.
- **[proven]** seen in the game by the project (the interrogation fight with the test grip, the
  Semper Fidelis train).

Verdict words: **AS IS** (the mod answers it today), **BUTTON** (works by a controller button, not
yet by the owner's reach-and-grip design), **FIGHT** (the fist-fight reader answers it),
**UNPROVEN** (built, never run here), **NOT** (nothing answers it), **GAME** (the game draws and
runs it by itself, nothing held in VR).

## 1. The mission in one look

Blackburn, bare arms, handcuffed for most of it. Ten checkpoints [data]:

| # | Save name | Where it is taken |
|---|---|---|
| 1 | `First Save` | start of the escape, after the interrogation movie |
| 2 | (unnamed, `Street_SCM`) | on the street after the window jump |
| 3 | `TrainrideStart` | landed in the train |
| 4 | `SaveBeforeC4` | past the jammed door (the kick QTE comes seconds later) |
| 5 | `TrainrideRoof` | on the roof |
| 6 | `SaveBeforeBomb` | back inside, the bomb car |
| 7 | `SaveBeforeSolomon` | before the last door |
| 8 | `PENNSAVE1` | waking in the wreck under Penn Station |
| 9 | `MANHOLESAVE` | the end of the sewers, under the manhole |
| 10 | `Bossfight` | Times Square, after the car crash |

No grenades in any kit, no ammo crates, no mounted gun, no launcher in the hands, no vehicle the
player drives. One real ladder (`Architecture/Sewersystem/Sewer_Ladder`, in `Penn_ART`).
Mission-fail links found: a failed fist-fight QTE (escape, train, final), the missed train jump,
the kick not answered in 7 s. The sewers and the car chase have none (death only).

## 2. Guns the player can hold

Slots are the game's (0 primary, 1 secondary, 7 knife/cuffs). "Held" = a row in kHeldKinds (the
gun follows the controller, reload by hand).

| Gun (blueprint) | How it comes | Slot | Attachments | Mod today | Reload in the mod |
|---|---|---|---|---|---|
| Handcuffs (`Weapons/Handcuffs/Handcuff`) | spawn kit `UsHumanSpSoldier_SP_Finale`, slot 7 selected by script in the escape; kit `_Knife` (cuffs only, everything else removed) for the final fight | 7 (0 in the final fight) | none | bare cuffed arms as free hands (barestate.h "handcuffed"), held off during scripted scenes [mod] | none |
| M9 (`Weapons/M9/M9`) | spawn kit slot 0 (hidden behind the cuffs until the train); re-given at the train jump and at `TrainrideStart` (`GiveGun`); kit `CustomizeSoldierGunOnly` for the car chase (removes every other gun, rounds set by script: 0, then 15 at each empty) | 0 | `U_M9_New_York` (a bullet unlock) | the train pistol's own rig (11970/0: follows the right hand, two-hand hold, magazine and slide by hand) [mod, proven on the train] | magnetic magazine from the belly, slide by hand |
| M1014 (`Weapons/M1014/M1014`) | pickup at the jammed door (`WeaponUnlockPickupEntityData`, radius 3.0, 32 shells, prefers slot 0, alt 1); if the player is NOT holding it when the kick guard spawns, the kit `_Shotgun` forces one into slot 0 | 0 | pickup: extended tube + flechette; forced kit: + **Kobra** | held (13377) [proven in SF] | shells by hand, loading port or ejection port |
| AKS-74u (`Weapons/AKS74u/AKS74u`) | kit `_AKs74u_Only` applied when the kick QTE is won (slots 0 AND 1 both hold it, knife in 7) | 0 + 1 | none | held (17991) [proven in SF] | magazine by hand, knock-out, bolt handle |
| **SG 553 (`Weapons/SG553LB/SG553LB`)** | kit `UsHumanSpSoldier_SP_Finale_SIG` at `PENNSAVE1` (save and load): the gun for the whole sewer | 0 | heavy barrel + foregrip, no optic (irons) | **not held: GAME** | none (the game's own R) |
| Knife (`Weapons/Knife/Knife`) | in the `_SIG`, `_Shotgun`, `_AKs74u_Only` kits | 7 | none | not mapped (no melee button) | - |

Drops from the dead (optional, the default 2.5 m pickup by holding the reload key; the mod's X held
or the magnet pickup):

| Gun | Who drops it, where | Attachments | Mod today |
|---|---|---|---|
| UMP-45 | train: 8 `TERR_SMG_NoGasMask` + 6 `spec/TERR_SMG_No_Step_Out` | bare | held (19314), not yet proven in the game |
| M1014 | train: 4 `TERR_SHG_NoGasMask`; sewers: 2 | bare | held |
| AK-74M / AEK-971 / F2000 (one of three per soldier) | train: 1 `TERR_Assault`, 2 `TERR_Assault_NoGasMask`; sewers: 1 `TERR_Assault` | bare; the unmasked ones' F2000 has an **EoTech** | AK-74M, AEK-971 held; **F2000 not held** |
| **MP7** | train: 2 `TERR_SMG_Flashlight_NoGasMask` (weapon light); sewers: about 12 Solomon's guards (`TERR_SMG_SolomonsGuard`), the first one stands in front of you as you wake ("kill him and take his MP-7" [web]) | guards: **IRNV** (1x thermal) + flash suppressor + laser | **not held**; IRNV not in kHeldSights |
| **Saiga 12K** (`Weapons/SAIGA20K/Saiga_20K`) | sewers: 3 `TERR_SHG_SolomonsGuard` | **IRNV** + laser | **not held** |
| **Type 88 LMG** | sewers: 1 `TERR_LMG_NoGasMask` (spawned at trigger 9211); one more `TERR_LMG` placed at the manhole street with no spawn link found | bare | **not held** |

Never the player's [data]: Solomon's .44 (Taurus), Montes's Glock 18 / M16A4 / final-fight M9,
Whistler's M4A1 ACOG, the three CIA agents' M16A4s (they are fake-immortal and weapon change is
locked while cuffed), the train's M9 guard and the kick guard (scene actors).

Sights that can appear: iron sights everywhere by default; Kobra (only on the forced M1014; in
kHeldSights, never tried on the M1014); EoTech (F2000, in kHeldSights but the F2000 isn't held);
IRNV (MP7, Saiga: no row, and its picture is a full-screen thermal effect).

## 3. Play order: every scripted thing and the mod's verdict

### 3.1 Interrogation and escape (`Escape_*`, checkpoint 1)

| What | Data | Mod verdict |
|---|---|---|
| Opening movie `Movies/SP_Finale_Interrogation` (skippable) | `Flow.txt` PlayVideo | a flat movie: as every other level's |
| Seated scene, then the fight QTE (`InterrogationEscape_QTEScene` + `QuickTimePrefab`): **Space**, then **E** | input restriction 2C50: look only. Measured [proven]: Space 5.0 s after the triggers appear, window 1.72 s; E 8.47 s later, window 0.79 s. Fail = fade out, mission failed | **FIGHT**: won twice by the scripted test grip (2026-11-18). A REAL grip failed the same reader in Swordbreaker's cellar the next night; the fix (a held or fresh grip answers every prompt once) is in work and not proven. Until then: **NOT proven with real hands** |
| Head-turned cutscene camera, free hands kept out of the room | notes of 2026-10-31 / 11-01 | AS IS (built; the hands come in once the scene ends) |
| Hallway run, cuffed: 3 CIA agents (M16A4, fake-immortal) spawn 2 s after Montes reaches the corner; no gun (restriction 05E1: move, sprint, jump, stance; no weapon change, zoom or reload); "sprint or you are shot" [web] | `Escape_BREAKOUT` | AS IS: left stick, stick click = sprint |
| **Window: `ID_H_INT_SP_FINALE_JUMP_OUT_OF_WINDOW`**, an interaction on **Jump (0x0C)**, radius 2.0, press, one use | `Escape_LOG.txt:996`; starts a synced slow-motion sequence, two scripted AK shots, the jump animation; crash note: the 2026-10-31 train-jump crash fix (look writes dropped while the picture is unturned) lives here | **BUTTON**: A or a short right-stick click (Space). Not by reach-and-grip |

### 3.2 The street (`Street_*`, checkpoint 2)

| What | Data | Mod verdict |
|---|---|---|
| Run to the tracks with Montes, cuffed, civilians and traffic around (restriction EB3B: move, sprint, jump, fire, zoom; no crouch, prone, weapon change, reload) | `Street_SCM` | AS IS |
| **The jump onto the train**: `PlayerInputTrigger` F0932DC8 on **Jump (0x0C)**, client and server | A 40 s sequence (B2AC) starts 1 s after Montes reaches his mark AND the player enters the trigger at the fence. **At 13.5 s** the Jump trigger is enabled and the Space prompt is drawn while the player stands in the jump volume (3A0B: enter opens the gate, leave closes it). **At 24.0 s** the trigger is disabled: `ID_H_ALERT_FAILED_JUMP_TRAIN`, mission failed. So: a 10.5 s window, and only from inside the volume. A second volume on the tracks (8DD9) kills the player before 13.5 s and after 24.0 s | **BUTTON**: A / right-stick click. The grips press nothing here (0x0C is answered only inside a scripted moment, on purpose: a magazine squeeze must not jump). Not by a grip |
| Vault animation, fade out, kit re-given (M9 + cuffs) | scenario 96B9, `CustomizeSoldier` 69D3 | AS IS (scripted camera) |

### 3.3 The train (`TrainRide_*`, checkpoints 3 to 7)

The same train as Semper Fidelis, cloned (other ids), with other enemies and a longer end.

| What | Data | Mod verdict |
|---|---|---|
| Landing, the M9 in hand; ~7 s scripted opening without free arms | as SF | AS IS [proven in SF] |
| A hidden Fire trigger (`EIAFire` D743, after trigger 7B95): the first shot wakes two idling guards | `TrainRide_SCM.txt:4776` | nothing to answer |
| M1014 at the jammed door: pickup radius 3.0, hold the reload key ~0.6 s; plays the door scene | `TrainRide_LOG.txt:9116` | **BUTTON** (X held; taken by the hands after the scripted camera) [proven in SF]. Reach-and-grip: not in main |
| The kick (`TrainWinKickQTE`): **left mouse (0x3D)**, wins on RELEASE, 7.0 s then hard fail. Win applies the AKS-74u kit and takes the cuffs off | `TrainRide_SCM.txt:4821` | **AS IS** by design (the tap lets go after 70 ms); SF's twin was won [proven]; this release-counted one is not yet run |
| `PRESS [E] CLIMB OUT`: interaction on Interact, **radius 3.0**, press | `TrainRide_LOG.txt:5360` | **BUTTON** (Y) |
| Hanging: **Space** once (0x40 + 0x0C), opens 4.0 s after CLIMB OUT, no timeout | 4761, 4791 | AS IS [proven in SF] |
| Dangling: **Space mashed** (0x40, 0.4 s reset) | 4806 | AS IS, a mash by holding the grip [proven in SF] |
| Roof: forced crouch on load (`CharacterStateTrigger` 6F36), then **`ID_H_INT_SP_FINALE_CROUCH`** (an interaction on Clutch 0x2A, radius 10, only there to draw the prompt) with slow motion; what counts is the POSE at the sequence's check: standing = health set to 50 and a knock-down animation, not death | `TrainRide_LOG.txt:5412`, sequence 7100 | **AS IS, unproven**: any crouch counts (B, or the stick flicked down). Watch: the game forcing a stance may leave the mod's own stance memory wrong (g_crouchOn) |
| Roof blown, back inside, the bomb car, more guards | as SF | AS IS |
| `PRESS [E] OPEN` at the last door: interaction on Interact, **radius 2.2**, press (`SolomonEncounteronTrain`); forces standing and slot 1 | prefab line 140 | **BUTTON** (Y) |
| **Fight with Solomon** (`QuickTimePrefab`): **right mouse (0x3E)**, then **Space** [web]; a miss = fade out, death. On the win Blackburn detonates the charge: train crash | `TrainRide_SCM.txt:4132` | **FIGHT, unproven**: the first Block prompt anywhere; the guard must hold the mod's own aim key (right mouse) away while the prompt is open |

Held-gun state on the train: M9, M1014, AKS-74u, UMP-45, AK-74M, AEK-971 held. Not held if picked
up: F2000 (3 soldiers may carry one), MP7 (2 soldiers).

### 3.4 Under Penn Station (`Penn_*`, checkpoints 8 and 9)

| What | Data | Mod verdict |
|---|---|---|
| Waking in the wreck: all input off (CD48), fade in after 1 s, a first-person scenario with Solomon (immortal) walking off and one guard left; then free | `Penn_LOWERCHASE`, scenario 76FC | AS IS as a cutscene. Check that the bare arms are found again here (the uncuffed draw keys 18108/13371 were measured on the train; another sub-level may place them elsewhere in the buffer) |
| **The SG 553 in hand for the whole chase** | kit at save and load | **GAME**: not a held gun. The game's own rifle and arms, its own reload on the reload key. Playable, not VR |
| Forced crouch 0.5 s after trigger 4A2C with a first-person scenario (E0AF): ducking through | `CharacterStateTrigger` 9097 | AS IS, unproven; same stance-memory watch as the roof |
| Guards with IRNV sights; scripted **flashbangs** (`InvisibleFlashbang`, 7 firing points) | sequences 6EE0, 2336, 4688, D6CF | the game's whiteout over the whole view: not checked in the headset (comfort, and whether both eyes get it) |
| A dragged-wounded scene (`DragWoundedUpAgainstWall`) that breaks when shot at or seen; an oil barrel that blows; an LMG gunner near the end | 906F, BE5A, 969A | nothing to answer |
| The ladder (a real `LadderComponentData` ladder) | `Penn_ART` | walk into it and push forward: the stick; never tried in the mod (no ladder in the finished missions) |

No interaction prompt and no QTE in this whole part.

### 3.5 Manhole, pickup, car chase (`Chase_*`, checkpoint 9)

| What | Data | Mod verdict |
|---|---|---|
| At `MANHOLESAVE`: an **8 s scripted camera** (a `CameraEntity` taking control), objective "hold off for Montes" that succeeds by itself at its end | `Chase_Fight`, sequence A7A3 | a camera takeover: AS IS as other scripted cameras, unproven here |
| **The climb out** (`ClimbUp_Sewer`): trigger 4CD5 starts it; restriction E315 leaves only **Throttle** and look; a `PlayerInputTrigger` on **Throttle (0x00)**, analog: the animation's time is the integral of the forward push. 2 s in, the kit becomes the M9 alone with 0 rounds (the SG 553 and anything picked up are gone) | `Prefabs/anim/ClimbUp_Sewer.txt:361` | **NOT answered by a grip; very likely fine by the left stick** pushed forward (it reaches the game as the pad's stick or as W). Untested. If neither reaches the trigger the player hangs on the ladder for good |
| Montes's police car arrives, the door opens, Blackburn gets in: scripted (`MontesPickup`), no prompt | scenario chain 8744 | AS IS |
| **The ride** (sequence 1D7A; first-person canned scenarios `CarChase1p`): look only at first; at trigger 6C2A firing is allowed; the M9 gets 15 rounds by script each time it runs dry, with a scripted reload scene; a scripted RPG shot; Solomon's SUV; crash, fades | `Chase_RAIL` | **RIDE, unproven**. Nothing must be hit: the ride ends by its own animation and has no fail link. Open: whether the train-pistol rig's hands and reload by hand fight the scripted seat pose and the scripted 15-round refills; whether aim writes stay paused in the swerving car (shots then go where the game points) |

### 3.6 Times Square (`Fight_*`, checkpoint 10)

| What | Data | Mod verdict |
|---|---|---|
| All input off (C1B7), cuffs-only kit, the fight scenario `FinalFight` driven by its sound's timeline | `Fight_BOSSFIGHT` | cutscene handling AS IS |
| **Final fight** (`QuickTimePrefab`, started 1.0 s after both actors spawn): **Space, E, left mouse, E, then Space mashed** (10 presses) [web + data] | `Fight_BOSSFIGHT.txt:670` | **FIGHT, unproven**: the press prompts as the interrogation's; the MASH prompt of a fight has never been run. A miss = mission failed (a late miss gives the detonation ending) |
| Ending scene, movie `Dima_Epilogue`, credits; 6 s after the movie a Jump trigger (0x0C) exits to the menu | `Credits_SCM.txt:242` | A / right-stick click skips; a grip does too if the credits count as a scripted moment (harmless either way) |

## 4. Everything else a VR player meets

- **Rides:** the train (walked on, proven in SF: aim, bare arms, crash fix), the police car (3.5).
- **Forced stances:** crouch on the roof load and in the sewer duck-through; stand at two train
  triggers and at Solomon's door; stand for the whole car rail (three `CharacterStateTrigger`s).
- **Timers:** the train jump (10.5 s window from 13.5 s), the kick (7.0 s), the fist-fight windows
  (0.8 to 1.7 s measured in the interrogation).
- **Slow motion:** the window jump, the roof crouch, two more train moments (`SlowMotionEntity`).
- **No crawl, no sniper kill, no stealth kill, no door breach, no bomb defusal** in this mission.
- **Civilians** on the street and in Times Square (`FriendlyFire_FailCondition` is loaded by the
  level): don't fire at them.
- **Lights and lasers** on enemy guns (MP7 torch, laser pointers): the game's own, per eye unchecked.
- **Old crash sites** (2026-10-31): bf3.exe+134E5F0 just after the interrogation and at the train
  jump; root cause believed fixed in 5d9b394 (nudges of null slots), guards in place, not re-run
  since.

## 5. What is missing, in order of weight

1. **A real grip answering the fight QTEs** (interrogation, Solomon on the train, final fight). The
   reader works; the grip rule failed with real hands. Blocks: all three fights end the mission on a
   miss. In work.
2. **The final fight's mash prompt** and **the Block (right mouse) prompt**: built, never run.
3. **The SG 553 as a held gun** (measure, rows, hold and lever fits, shape, recoil row; the AK
   block: magazine pulled or knocked out, handle on the right). The sewer's own gun.
4. **The MP7 as a held gun** (grip-magazine SMG: the pistol's magazine way on a long gun, a new
   block) and the **IRNV thermal sight** (a row plus the thermal picture through the lens).
5. **The sewer climb on Throttle**: test the stick; if it fails, the small W-held change the QTE
   survey describes (qtelist KeyOf 0x00, a kKeyW bit, W's scan code).
6. **The police-car ride** with the M9: decide hands and reload during the canned seat scenes.
7. **Prompts by reach-and-grip** (window Jump, CLIMB OUT, OPEN, the M1014 take): today by buttons.
8. Optional drops: F2000 (+ EoTech on it), Saiga 12K (+ IRNV), Type 88; the Kobra on the forced
   M1014; the knife.
9. Checks without new code: the roof crouch by pose, forced stances against the mod's stance
   memory, the ladder, bare-arm draw keys in the sewer, the chase and the cuffed final fight,
   flashbang whiteout in both eyes.

## 6. In-game test list (mission order)

1. Replay Missions > The Great Destroyer: interrogation with a REAL grip held and with a fresh
   squeeze (Space, E).
2. Window: A at the prompt. Street: stand in the gap, A when the prompt shows; let it lapse once to
   see the fail alert.
3. Train: as SF; the kick (look for the release winning), roof crouch standing and crouched,
   Y at CLIMB OUT and OPEN, the Solomon fight ("the game awaits Block (right mouse)").
4. `PENNSAVE1`: "Bare arms: drawn the cuffs off" line; the SG 553 as the game draws it; take an MP7
   and zoom (IRNV); a flashbang; the duck-through; the ladder.
5. `MANHOLESAVE`: the camera, the climb by the stick (look for an "action 0x00" line), the pickup,
   the ride with and without firing.
6. `Bossfight`: every prompt, the mash counting down from 10; the credits.

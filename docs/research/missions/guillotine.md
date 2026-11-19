# Operation Guillotine (`SP_Bank`) in VR: survey

Read-only research, 2026-11-19. Nothing was run in the game. Sources:

- **[data]** the public Venice-EBX dump, `Levels/SP_Bank/*` (file and line given where it matters),
  the player kits from `Characters/Soldiers/Customizations/*` of the same dump;
- **[mod]** the code in main at `7e99b44` (`src\skin.cpp` `kHeldKinds` / `kHeldSights`,
  `src\recoilguns.h`, `src\input.cpp`, `src\qtelist.h`, `src\qtefight.h`);
- **[survey]** the campaign QTE survey of 2026-11-19 and `docs\research\weapons-catalog.md` 2.5;
- **[web]** walkthroughs quoted in that survey (GameFAQs, portforward, the Battlefield wiki).

Anything marked **check** is read from data only and has to be seen in the game once.

## 1. Short verdict

The mission is long (14 start points, 21 checkpoints) and is **not playable to the mod's standard yet**.
What stands in the way, most important first:

1. **The kit's guns are not held guns.** The SCAR-H with ACOG (the primary from the first wall to the
   end of the bridge) and the 870 MCS (the secondary for the whole mission) are not in `kHeldKinds`.
   They stay as the game draws them (`skin.cpp:9139`).
2. **The Javelin** (three T-90s on the bank plaza have to be destroyed) is not a held launcher and
   its lock-on by held aim has never been tried in VR.
3. **The ACOG on a held gun** is not in `kHeldSights`. The M4A1 the game hands over after the bridge
   carries one, and it stays the primary to the end.
4. **The mortar carry** at the very start: a kit with only the M224 in the arms, never seen in VR.
5. Two long **scripted rides** in a Humvee (36.7 s, then about 69 s ending in a slow-motion ambush)
   and nine more scenes that animate the player's body. The mod's cutscene camera and body lock are
   generic, but none of these has been run.
6. **Quick-time events:** one fist fight (the bathroom, fail = mission failed), one E mash (the
   blocked door), two single presses on the rappel. All fit what is built; the fight needs the
   real-grip fix that is in work.
7. Optional pickups with new mechanics: the M98B (bolt action, 12x scope, bipod), the standalone
   M320, the PKP.

No mounted gun and no vehicle is ever the player's. No sniper-kill scene like Swordbreaker's roof.

## 2. The mission in play order

Start points [data, `Flow.txt:1931-2096`, by SortIndex]: Level Start, Intro, Hill, Assault, House,
Convoy, Ambush, BridgeTopFight, PostBridge, Passage, Plaza, Bathroom, Bankregroup, Elevatorshaft, Vault.

Sub-levels that nothing ever streams in, so **cut content, not in play** [data, no StreamIn link in
`SP_Bank.txt`]: `Ministry_SUB` (the general with the G3A3, its two breach doors, the `ENTER_VEHICLE`
prompt of `Ride_LOG.txt:824`), `Highway_01_SUB`, `Highway_02_SUB`, `Ride_SUB`, `Breach_SUB`.
The catalog's "general in the ministry" line is from this cut part.

| # | Section (checkpoints) | What happens | Kit in the hands |
| --- | --- | --- | --- |
| 0 | Movie | `SP_Bank_Interrogation` plays first (`Flow.txt:1896`, skippable) | none |
| 1 | Intro, the hill (`SaveBeforeIntro` twice) | Trench scene, mortar carried down, set up by a prompt, wall climbed by a prompt | M224 only |
| 2 | Assault, the canal (`SaveAfterWall`, `CanalEnd`) | Run downhill under mortars, MG nests, the canal | SCAR-H, 870 |
| 3 | House, the apartment block (`SaveInHouse`) | Blocked door (prompt + E mash), a door that hits you (prompt, forced crouch, slow motion) | SCAR-H, 870 |
| 4 | Convoy | Brady's briefing, `GET IN` prompt, Humvee ride 36.7 s, fade | SCAR-H, 870 (no input) |
| 5 | Ambush, the bridge (`AmbushStartSave`, `BRIDGEHUTSAVE`, `POSTBRIDGESAVE`) | Ride goes on about 69 s, ambush in slow motion, fight under and on the overpass, T-90s chase (`FLEE TANK`), climb into the rift by a prompt, fade | SCAR-H, 870 |
| 6 | Passage (`PassageMiddleSave`) | Scene "trapped by the tank", sewer passage | **M4A1 + ACOG**, 870 |
| 7 | Plaza (`PassageDoorSave`, `SAMSITESAVE`, `REGROUPFLANKSAVE`, one unnamed) | Plaza fight, Javelin picked up, three T-90s, regroup, climb over (prompt), ladder and window (prompt + stick) | M4A1, 870, Javelin |
| 8 | Bank (`POSTBATHROOMSAVE`, `POSTBANKSAVE`, `SAVEBANKREGROUP`, `VAULTPASSAGESAVE`) | Bathroom fist fight, the atrium guard, atrium fight, regroup scene with Brady | M4A1, 870 (knife forced in the bathroom) |
| 9 | Vault (`Shaft`, `Vault`) | Rappel down the lift shaft (prompt + two quick-time presses), vault fight, the lid (prompt), end | M4A1, 870 |

## 3. Guns

### 3.1 The kits [data]

| When | Kit | Slots | Held in the mod? |
| --- | --- | --- | --- |
| Start (`Intro_SCM.txt:42`) | `Levels/SP_Bank/StartKitMortar`, RemoveAllExistingWeapons True | 0: `Weapons/Gadgets/M224/U_M224_SP` | No. A recoil row only (`recoilguns.h:485`, no sway) |
| 11 s after the mortar prompt (`Intro_SCM.txt:53`), and at every later start point | `UsHumanSpSoldier_SP_Bank` | 0: `Weapons/SCAR-H/U_SCAR-H` + `U_SCAR-H_Acog`; 1: `Weapons/Remington870/U_870` + `U_870_Target_Pointer` (laser); 6: M67; 7: knife | **SCAR-H: no** (recoil placeholder `recoilguns.h:134`). **870 MCS: no** (placeholder `:305`) |
| End of the bridge, on the fade (`Flow.txt:55`, fired by `Bridge_BMD` EventSync `513C` after Sequence `EFF9`'s "ExplosionTeleport") | `UsHumanSpSoldier_SP_005`, RemoveAllExistingWeapons False | 0: `Weapons/M4A1/U_M4A1` + `U_M4A1_ACOG`; 6, 7 as before. Slot 1 untouched | **M4A1: yes** (`kHeldKinds` "m4a1", magazine by button or hand, T handle, bolt slap, port flap). **Its ACOG: no row in `kHeldSights`** |
| After the three tanks (`Tank_TANKAFTERMATH.txt:165`) | `UsHumanSpSoldier_SP_Bank_NoJavelin` | 5: emptied | nothing to hold |

Notes:

- The M4A1 kit replaces whatever is in slot 0 at the bridge's end, a picked-up gun too. The mod's
  holster memo must take this as the game's doing (like the roof AT4 on Swordbreaker): **check**.
- `Customize_Javelin` (`Tank_PLAZA.txt:12`) is applied to a **marine** (`SoldierExtra` `35A8`), not to
  the player. The catalog's "on a checkpoint reload the kit puts it in slot 0" is wrong for the player.

### 3.2 Pickups placed in the level [data, `WeaponUnlockPickupEntityData`, radius 2.5 m each]

| Gun | Where | Slot | Attachments | Rounds | Held? |
| --- | --- | --- | --- | --- | --- |
| M320 standalone (`Weapons/Gadgets/M320/U_M320_HE`) | Assault, `Assault_LOG.txt:36968` (the crater past the first wall [web]) | 0, else 1 | none | 8 | **No.** The mod's M320 is the one under the M416, found by slot 2 (`ammo.cpp:283`); in slot 0 or 1 nothing of it applies |
| AEK-971 (`U_AEK971`) | Assault, `Assault_LOG.txt:37047` | 0, else 1 | heavy barrel, foregrip, **PK-AS** | 82 | Yes ("aek971"; PK-AS in `kHeldSights`). The foregrip and heavy barrel on a held gun: **check** |
| FGM-148 Javelin (`Weapons/FGM148/U_FGM148_SP`) | Plaza, `Tank_LOG.txt:8765` (the dead Javelin team [web]) | 5 | none | 0 by its data; a hint asks to resupply | **No** (recoil placeholder `recoilguns.h:440`, kind kMissile; the launcher holster place knows the kind) |
| M98B (`Weapons/Model98B/U_M98B`) | Bank atrium, `Bank_LOG.txt:4688` | 0, else 1 | **ballistic scope (12x)**, flash suppressor, bipod | 15 | **No** (placeholder `:395`). First bolt-action gun, first 12x scope, first bipod |
| M1014 (`U_M1014`) | Vault, `Vault_LOG.txt:4488` | 0, else 1 | lists F2000 EoTech, foregrip and flashlight (can't fit: bare) | 16 | Yes ("m1014", shells by hand) |

A supply crate stands in the bank (`Bank_LOG.txt`, `FromBF2/.../supply_crate`).

### 3.3 Dropped guns (see the catalog 2.5 for who carries what)

| Gun | Held? | Reload in the mod |
| --- | --- | --- |
| AK-74M | Yes | magazine pulled or knocked out by hand, lever over or under |
| AKS-74u (weapon light; bare, Kobra or PK-AS) | Yes | as the AK |
| AEK-971 | Yes | as the AK |
| RPK-74M | Yes | magazine by hand, lever |
| KH2002 (Kobra) | Yes | button or hand, finger on the fin |
| G3A3 | Yes | by hand, lever |
| M16A4, UMP-45, M249 (fallen marines; `UsSpSoldier_LMG` carries the M249) | Yes | button or hand; the M249's belt by hand |
| **PKP** (the MG nests: `EnemyNestGunner`, AI template `PLR_HeavySupport`; `RespawningBridgeEnemy_LMG`) | **No** (placeholder `recoilguns.h:269`) | none |
| Glock 18 (the bathroom soldier) | never drops | - |

The MG nests are AI soldiers behind sandbags, **not stationary guns**: there is no mounted weapon to man.

### 3.4 Sights and other attachments

| Attachment | On | In the mod |
| --- | --- | --- |
| ACOG | SCAR-H (kit), M4A1 (kit) | Built for the M416 only (`skin.cpp:466`, 3.44x). No `kHeldSights` row, so on a held M4A1 it has no lens picture and no eye line: **missing** |
| PK-AS | AEK-971 pickup, some AKS-74u | In `kHeldSights` |
| Kobra | KH2002, some AKS-74u | In `kHeldSights` |
| Ballistic scope 12x | M98B | Missing |
| Laser (target pointer) | 870 MCS | Never looked at |
| Weapon light | AKS-74u | Never looked at. The atrium guard scene reads the player's flashlight state (section 6) |
| Bipod, flash suppressor | M98B | Never looked at |

## 4. Launchers, mounted guns, vehicles

- **Javelin** [data]: `UseJavelinLogic` only shows hints (the weapon's name, "switch to" on consoles,
  `ID_H_MES_SP_BANK_HOLD_AIM_ON_TARGET_TO_LOCK_ON`, a resupply hint when rounds in it plus spare are
  at or under a threshold). It checks slot 5 and whether `Weapons/FGM148/FGM148_SP` is in hand. The
  lock is the game's own: aim held on the tank. Three T-90s (`Tank_LOG.txt:6972`, `:7119`, `:7266`),
  one after the other; an AND gate of all three `OnDestroyed` moves the mission on
  (`Tank_T90` `DCB6`). After that the kit takes the Javelin away. A marine also carries one, so
  whether the tanks die without the player: **check**; the objective text is
  "support the Javelin team" then "destroy remaining armor".
- **M224 mortar**: carried, never fired by the player (section 5.1). The deployed mortar
  (`Weapons/Gadgets/M224/M224_Deployed`, spawned as a vehicle) is Campo's.
- **M320 standalone**: section 3.2.
- **Mounted guns: none for the player.** The Humvees' turrets and the Venom are scenery or AI.
- **Vehicles: none driven.** The Humvee ride is an animation with the player as an actor, not a
  vehicle seat (section 5.4). The T-90s are AI on waypoints or on a sequence.

## 5. Scripted scenes, rides and prompts, in play order

Prompts are `GameInteractionEntityData`; all are a **press** (HoldToInteractTime 0). "Interact" is
the game's E (`EIAInteract`), "Jump" its Space (`EIAJump`). In the mod today the use button (Y) sends
E and the jump button (A, or a short right-stick click) sends Space, so every prompt below can be
answered by a button as is. The owner's rule (2026-11-18) is the hand reached to the place and the
grip held; whether main's reach-and-hold answers these prompts, the Jump ones too, is a **check**.

"Player is an actor" means a `MultipleActorScenario` animates the player's body: the mod's scripted
camera (head free, body locked, per-eye body) is what should carry it.

| # | Where | What | Input | Mod's verdict |
| --- | --- | --- | --- | --- |
| 5.1 | Intro: the trench (`Prefabs/anim/TrenchIntro`) | Scene with Campo, Cole, Matkovic, Montes; **player is an actor** (binoculars prop). Then kit = mortar only, and InputRestriction `3A43`: no fire, zoom, jump, stance change, reload, prone, melee, grenade, weapon change | none | Scene: generic, **check**. **Mortar carry: never run.** The game's own carry arms have to show; the mod's hands and gun logic meet a kit with one unmeasured item and no slot 1: **check first** |
| 5.2 | Intro: the wall (`Intro_LOG.txt:790`) | Prompt `ID_H_INT_SP_BANK_MOUNT_MORTAR`, enabled when the squad arrives. Starts `DeployMortar` (**player is an actor**, every input off). 9.25 s the shot's sound, 11 s the SCAR-H kit, 12 s the illumination round | Interact, r 2.0, 360 degrees, 1 use | Button as is. Scene: **check** (weapon switch in the middle of a scene) |
| 5.3 | Intro: over the wall (`Intro_LOG.txt:842`) | Prompt `ID_H_INT_SP_BANK_CLIMB_OVER`, 5 s after the squad is in place. Scenario `F672`, **player is an actor** | Interact, r 2.0 | Button as is. Scene: **check** |
| 5.4 | Assault | Free fight. 42 deadly mortar strikes on timers (`Assault_Mortars`, loops of 7 to 23 s) mark the wrong ground; out-of-bounds prefab with a 5 s warning ("stay on mission"). A Venom crashes on a 45 s sequence. M320 and AEK-971 on the ground | - | As any fight. Nothing forced |
| 5.5 | House: the blocked door (`Prefabs/Logic/BlockedDoor01.txt:122`, `:230`) | Prompt `ID_H_INT_SP_BANK_BREACH`, then the `PushVan` animation (**player is an actor**). 1.3 s later quick-time trigger `7A74B299` (E, `EIAQuicktimeInteractDrag` 0x3C): **mash, counted on release**, 6 steps, a 0.2 s reset after each; the sixth opens the door. No fail link | Interact, r 2.3, then E mashed | Prompt: button. Mash: QTE version 2 as built (one live trigger; a held grip taps every 130 ms, each tap is a press and a release). **AS IS, check** |
| 5.6 | House: the door that hits you (`House_LOG.txt:3311`, `Prefabs/anim/HitByDoor`) | Prompt `ID_H_INT_SP_BANK_BREACH` (enabled by a 4-input AND gate). The door flies open into the player: **player is an actor**, **pose forced to crouch** (`CPTForceToCroch`), a 4 s sequence with "GunDown" (no fire) and "GunUp", slow motion (down 1.0 s, up 0.3 s), an enemy's senses switched on | Interact, **r 1.0, 180 degrees** (must face it), 1 use | Button. **Check:** the forced crouch against the mod's stick stance (the mod must not think it is standing); the gun lowered while the hands hold it |
| 5.7 | Convoy (`Convoy_LOG.txt:358`) | `BradyBrief`, then prompt `ID_H_INT_SP_BANK_GET_IN`. `Mount_Dismount_Humvee_Player_Passanger`: **player is an actor**, mounted, riding and dismounting are three scenarios on the Humvee's transform. InputRestriction `CB11`: no fire, zoom, jump, stance, weapon change, reload, sprint; looking stays free. Sequence `1EA0` **36.7 s** drives two Humvees (water splashes), ends in a fade | Interact, r 2.0 | **RIDE.** Same family as the Rock and a Hard Place car and the Comrades drive: body is the game camera's, head free, body drawn per eye. Never run here: **check** (seated pose of the held rifle, the fade, no free hands) |
| 5.8 | Ambush (`Bridge_AMBUSH`) | New Humvee, the ride goes on (Sequence `BDD0` 69 s). `Convoy_ambush2`: RPG hit, **player is an actor** with its own transform, **slow motion** driven by a synced sequence (`7B81` to `SlowMotion 9A91`), InputRestriction `1BB9`. Checkpoint `AmbushStartSave`, the dismount scenario, input back | none | **RIDE + scene, check.** Slow motion is new for the mod: hands and reloads run on real time while the game slows |
| 5.9 | Bridge | Fight under the overpass, the hut (`BRIDGEHUTSAVE`), the top, the flank run. Out of bounds here is an invisible SVD shot. Three T-90s on sequences of 4, 29.9 and 31.9 s (`BMD_LOGIC`): "flee tank" | - | As any fight |
| 5.10 | Bridge: the rift (`Bridge_LOG.txt:122`) | After `POSTBRIDGESAVE`: prompt `ID_H_INT_SP_BANK_TO_CLIMB`. Scenario `2CCC`, **player is an actor**, input off till it ends. Then Sequence `EFF9` (10 s), "ExplosionTeleport", **fade out**, the M4A1 kit, the bridge unloaded | **Jump**, r 3.0, 360 degrees | Jump button as is. Scene and kit swap under a fade: **check** (the holster memo) |
| 5.11 | Passage (`Passage_CUTSCENE`, `TrappedByTank`) | Fade, the player **moved** to a new place and stood up (`CharacterStateTrigger B774`), scene with Campo, Montes, Matkovic and a red signal flare, **player is an actor**; movement off, looking free (InputRestriction `D16A`) | none | Scene: **check** |
| 5.12 | Plaza | Fight on the plaza, the roof (`SAMSITESAVE`, a HAWK site), the Javelin pickup, three T-90s | Javelin: aim held to lock | **Javelin missing** (section 4) |
| 5.13 | Plaza: the low wall (`Tank_LOG.txt:4110`) | Prompt `ID_H_INT_SP_BANK_CLIMB_OVER`, 3 s after its scene is ready. Advances scenario `CE8F` (Matkovic and the **player as actors**), input off till it ends | **Jump**, r 2.0 | Jump button as is. Scene: **check** |
| 5.14 | Plaza: ladder and window (`Prefabs/anim/ClimbingBankEntry`, `BankEntry_Ladder`, `Tank_LOG.txt:4058`) | Matkovic is boosted up and drops a ladder (a 20 s delay). **The ladder is climbed by holding forward:** trigger `1B3E782F` reads **Throttle** (0x00) as a value, integrates it and drives the climb animation's time. No fail, no time limit. Then prompt `ID_H_INT_SP_BANK_JUMP_IN_WINDOW`, enabled 16 s after its start. InputRestriction `E1C9` leaves throttle, jump, reload and weapon change free | Left stick forward (analog), then **Jump**, r 2.0, **180 degrees** | The grips don't answer Throttle (`KeyOf(0x00)` is 0), and need not: the walking stick is the game's throttle. **Check** that the stick still reaches the game while the game holds the camera (the W fallback counts too) |
| 5.15 | Bank: the bathroom (`Bank_BATHROOM.txt:570`, `Prefabs/anim/ToiletFight`) | On entering: InputRestriction `D9E3` (no fire, zoom, weapon change, reload, melee, grenade; walking free). 5 s later the **knife is forced into the hand** (slot 7). A soldier bursts in; Matkovic, the guard and the **player are actors**. **Fist fight** on the shared `QuickTimePrefab`, started 0.15 s into the scene, stopped when it ends. PC keys [web]: **E, then left mouse**. `Failed` goes to the level's fail logic: **mission failed, checkpoint reloaded** [data] (the wiki speaks of a second chance [web]) | the awaited key only | **FIGHT.** `qtefight.h` reads the one awaited key. Proven with the test grip (The Great Destroyer); **real grips failed on Swordbreaker 2026-11-19**, fix in work. The trigger and the use button answer too. The knife forced into the hand: the mod's hands with slot 7: **check** |
| 5.16 | Bank: the atrium guard (`Bank_SNEAKATTACK`, `Prefabs/anim/BankKillPlayer`) | A guard with his senses off and no melee target. Inside a zone near him the shared `PlayerFireCheck` logic opens the scripted kill on **Fire**: `SP_Bank_SFX_Stealth_Kill`, every input off, **player is an actor**. If the player's **flashlight is on** while looking at him (LookAt, 15 degrees), he wakes up and fights. Killing him any way also moves on (his `OnKilled` feeds the same gates) | **Fire** (the trigger) in the zone | Not a quick-time trigger and not a prompt: the trigger pull is the input. Walkthroughs call it the knife (F) [web]. **Check:** the gun must not also fire, the scene's body |
| 5.17 | Bank: the atrium, regroup | Fight (waves counted by kills), the M98B, a supply crate. Leaving back through the bathroom is a 1000-a-second damage area. `LobbyBrady` scene with Brady, Campo, Montes (the player is not an actor) | - | As any fight |
| 5.18 | Vault: the rappel (`Prefabs/anim/RapellScene2.txt:487`, `Vault_DESCENT.txt:719`, `:734`) | Prompt `ID_H_INT_SP_BANK_TO_RAPPEL`. 0.5 s later input off for the scene (**player is an actor**: rope, carabiner, a flashbang). Two quick-time triggers, each enabled by its own scene event, one live at a time, each disabling itself on the press: **Space** (`EIAQuicktimeJumpClimb` 0x40, `61E404C1`) and **left mouse** (`EIAQuicktimeFire` 0x3D, `CFCA2683`). The left-mouse press also saves checkpoint `Shaft`. **No fail link read.** Input back 7.8 s after a scene event | Interact, r 2.0, **180 degrees**; then one press each | Prompt: button. Presses: QTE version 2 as built (single triggers): a grip answers each. **AS IS, check** |
| 5.19 | Vault: the fight | Enemies down the corridor, the M1014, a blasting cap, a 50-a-second damage area | - | As any fight |
| 5.20 | Vault: the lid (`Prefabs/anim/VaultScene.txt:533`) | Prompt `ID_H_INT_SP_BANK_HELP_LIFT_THE_LID` once the squad stands at the crate. Scene (**player is an actor**), a camera shake, fade, every input off (`Vault_END`), 4 s later the next level loads | Interact, r 2.0 | Button. Scene: **check** |

Counted: 9 prompts in play (6 on Interact, 3 on Jump), 1 fist fight, 3 standalone quick-time
triggers (E mash, Space, left mouse), 1 analog climb, 2 rides, 11 scenes with the player's body
animated, 2 fades with the player moved, 1 forced crouch, 1 forced weapon (the knife), 2 slow-motion
moments.

## 6. Other things a VR player meets

- **No crawl, no real ladder, no swimming.** Every climb is a scene started by a prompt; the one
  "ladder" is the analog scene of 5.14.
- **Forced stances:** crouch at the door (5.6); "stand" set by the game when it moves the player
  (5.8, 5.11) and when it forces the knife (5.15).
- **Timers:** none on the player except the out-of-bounds warnings (5 s) and the deadly mortars of
  the Assault. The bathroom fight is the only timed input that fails the mission.
- **Night:** the first half is dark, lit by illumination rounds; the laser on the 870 and the weapon
  lights are the level's own answer. The mod has neither looked at.
- **The flashlight** matters once (5.16): the kit has none; an AKS-74u with a weapon light does.
- **Quick-time triggers in free play:** the bathroom fight is stopped at its scene's end, the other
  three disable themselves. So no trigger should stay live between scenes (the "open risk" of
  `qte-trigger-list`): **check** with qtepeek at the bathroom's exit.
- **Rebinds:** a Battlelog thread says the bathroom fight broke with melee rebound from F [survey].

## 7. What is missing, as a list

"Blocks" = the player is forced through it and the mod has nothing proven for it.

| # | Kind | Item | Where | Effort | Blocks |
| --- | --- | --- | --- | --- | --- |
| 1 | gun | SCAR-H as a held gun (frame, magazine by button and hand, charging handle, recoil recorded) | Kit primary, sections 2 to 5 | Large (a new rifle, measured like the Mk11) | Yes |
| 2 | scope | ACOG as a `kHeldSights` row (on the M4A1 and the SCAR-H) | Kit primary both halves | Medium (the M416's ACOG lens and marks exist) | Yes |
| 3 | launcher | FGM-148 Javelin: held, lock by held aim, its sight picture, reload, holster | Plaza, three T-90s | Large | Yes, unless the marines' Javelin is seen to finish the tanks |
| 4 | other | Mortar carry at the start (the game's own carry, the mod's hands out of the way) | Intro | Small if it only needs a look, medium otherwise | Yes, until seen |
| 5 | gun | 870 MCS as a held gun, pump by hand, shells by hand | Kit secondary, whole mission | Large (the M1014's shell loading helps) | No (the primary is enough) |
| 6 | ride | Humvee ride and ambush in slow motion, never run | Convoy, Ambush | Small to check, medium if the seated hold or slow motion need work | No, expected to work |
| 7 | qte | Bathroom fist fight with real grips (the fix in work) | Bank | Shared with Swordbreaker | Yes until the fix is in main; the trigger and Y are a way round |
| 8 | qte | Blocked door E mash and the rappel's two presses: built, unrun on this level | House, Vault | Small (a run) | No |
| 9 | prompt | Reach-and-hold-grip for this level's prompts, the three on Jump too | 9 places | Small to medium | No (buttons work) |
| 10 | other | Ladder by the stick while the game holds the camera | Plaza | Small (a run) | Yes if the stick doesn't reach the game there |
| 11 | other | Forced crouch at the door, forced knife in the bathroom, the kit swap under a fade | House, Bank, Bridge | Small each | No |
| 12 | other | Atrium guard: trigger pull starts the scripted kill | Bank | Small (a run) | No (he can be shot) |
| 13 | gun | M98B: bolt by hand, magazine | Bank atrium pickup | Large | No (optional) |
| 14 | scope | Ballistic scope 12x for the M98B, its bipod | Bank atrium pickup | Medium | No |
| 15 | launcher | M320 standalone (in slot 0 or 1) | Assault pickup | Medium (the under-barrel one exists) | No (optional) |
| 16 | gun | PKP as a held gun, belt by hand | MG nests' drops | Large (the M240 and M249 blocks help) | No (optional) |
| 17 | other | Laser on the 870, weapon light on the AKS-74u | Kit, drops | Small each | No |

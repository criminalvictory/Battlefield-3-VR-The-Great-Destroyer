# The campaign's weapons: every gun, launcher, gadget and attachment, and how each becomes a VR weapon

Research for *Battlefield 3 VR: The Great Destroyer*, written and checked 2026-11-11. Scope: the
**12 campaign missions only**. Multiplayer-only and co-op-only weapons are left out on purpose.
Nothing here was run in the game: it was read from the game's own files (offline), from the web, and
from this repository.

## What this means for us

1. **The campaign's 12 levels load 58 hand-held weapons** (the game's own count of weapon
   blueprints). **51 of them can end up in your hands**: **40 guns**, 6 launchers, 2 grenades or
   mines, and 3 melee or scripted items. The other 7 are carried only by story characters or by
   enemies that never drop them, are props, or are never given to anyone.
2. **37 of the 40 guns can be held in normal play**: 17 come in your own loadouts or by script, 7
   lie as placed pickups, and 13 are taken from the dead. The other 3 (M240B, SV-98, SVD) are edge
   cases that are nearly out of reach.
3. **Attachments: 24 kinds** (157 distinct attachment unlocks in the kits and pickups): 13 sights
   and scopes, 3 barrel parts, foregrip, bipod, light, laser, the M320 under the M416, extended
   magazines and two shotgun ammo types. **There is no underbarrel shotgun (M26 MASS) and no GP-30
   anywhere in the campaign**, and no magnifier.
4. **Mounted and vehicle weapons you use: 5 systems**: the PLR technical's machine gun, the M2HB
   .50 (Humvee turret and Abrams roof), the M1A2 Abrams (cannon and coaxial MG), the UAV
   laser-designation feed and the F/A-18F back seat; plus the M224 mortar, which you carry and set
   up while the crew fires it.
5. **Built in the mod today: 11 of the 40 guns** (M416, M4A1, M16A4, AK-74M, AN-94, AEK-971,
   KH2002, G3A3, RPK-74M, M240B and the train M9), plus the M320 under the M416, the SMAW and the
   Stinger, all with reloads by hand for the left hand. Only the M416 has free hands (either hand,
   drop, throw, pick up). **29 guns are new work**, and so are the Javelin, the AT4, the standalone
   M320, grenades, the claymore, the knife and every mounted or vehicle weapon.
6. **Most of the new guns are combinations of mechanics already built.** The AK blocks (rocked-in
   magazine, paddle, right-side handle) cover 11 guns: the seven AKs with no hold-open, the SVD, M39
   EMR and M82A3 (handle held back when empty) and the SG 553's magazine; the AR family (button
   magazine, T-handle, bolt catch) 5; the M240's belt mechanics 5. Only a handful of truly new
   mechanics are needed: the **turn bolt** (M40A5, M98B, SV-98), the **pump** (870 MCS), **shells
   loaded one by one** (870, M1014, DAO-12, with the DAO-12's drum wound by hand), the **P90's top
   magazine**, and the **HK notch-and-slap** (UMP-45, and the G3A3's missing notch).
7. **Two foundations block more guns than any single gun does:** every interaction by either hand
   (built for the M416 only), and **the arms of the other levels**: Dima's Russian arms (Comrades,
   Kaffarov) and Blackburn's bare cuffed arms (Semper Fidelis, The Great Destroyer) have no tables
   yet, except the train pistol's.
8. **Kaffarov alone** brings 19 guns you can hold. **Comrades** lets four new carbines (AKS-74u,
   AS Val, G36C, SG553) and three SMGs (UMP-45, MP7, P90) be recorded in one session.
9. **Corrections to older notes:** the Glock 18 can't be picked up in the campaign; the "Saiga
   20K" is a 12-gauge Saiga 12K; the pistols hold one round less than their data says (16 = 15
   plus one chambered); the Kaffarov weapons case at the landing holds display models, not
   pickups; the SKS and G53 on the notes' paddle-release list are in no campaign level.
10. **A soldier seen with a gun is not always a source:** only two soldier types can drop a gun
    (section 1.7). The others (canned-scenario actors, civilians, vehicle crews) never do, which
    takes Swordbreaker's UMP-45s and Uprising's SVD off the list of guns you can take.

## How to read this

Every fact carries a mark:

- **[data]** the game's own files, read offline from a Steam install (`bf3.exe` 1.6) and from
  [Venice-EBX](https://github.com/VeniceUnleashed/Venice-EBX) (a public text dump of BF3's data:
  kits, pickups, enemy templates, firing data, attachment unlocks, animation packages, sound
  waves). Also the game's own reload animations recorded in the game by this mod's tools
  (`build/survey/*.bin`). The kits and pickups were cross-checked against the install: 208 soldier
  kits (836 weapon entries; the set compared includes the co-op kits that shared bundles carry) and
  all 25 weapon pickups match.
- **[web]** the [Battlefield Wiki](https://battlefield.fandom.com/), [IMFDB's Battlefield 3
  page](https://www.imfdb.org/wiki/Battlefield_3), Wikipedia, army manuals and firearms references.
- **[inferred]** my reading of the above.
- **[mod]** what this project's code and playtests say about the mod today (not a fact about the
  game).

Where a whole table or list comes from one source, its heading says so and only exceptions are
marked.

Mission codes used in the tables, in campaign order (each level's own sort order in its data
[data]):

| # | Code | Mission | Level file | You play |
| --- | --- | --- | --- | --- |
| 1 | SF | Semper Fidelis | `SP_New_York` | Blackburn (bare, cuffed arms) |
| 2 | SB | Operation Swordbreaker | `SP_Earthquake` | Blackburn |
| 3 | UP | Uprising | `SP_Earthquake2` | Blackburn |
| 4 | GH | Going Hunting | `SP_Jet` | Hawkins (jet back seat) |
| 5 | OG | Operation Guillotine | `SP_Bank` | Blackburn |
| 6 | CO | Comrades | `SP_Paris` | Dima (Russian arms) |
| 7 | TR | Thunder Run | `SP_Tank` | Miller (tank) |
| 8 | FE | Fear No Evil | `SP_Tank_b` | Miller (tank) |
| 9 | NS | Night Shift | `SP_Sniper` | Blackburn |
| 10 | RH | Rock and a Hard Place | `SP_Valley` | Blackburn |
| 11 | KA | Kaffarov | `SP_Villa` | Dima, then Blackburn |
| 12 | GD | The Great Destroyer | `SP_Finale` | Blackburn (bare, cuffed arms) |

`SP_Bank` is Operation Guillotine, not Swordbreaker. How you get a weapon: **kit** (your
loadout at the start or a scripted loadout change), **script** (handed over or forced in a scene),
**pickup** (placed in the level), **drop** (taken from a dead enemy), **fallen friendly** (taken
from a dead friendly soldier; a dead squadmate fails the mission, so only unnamed ones count).

---

## 1. The count

### 1.1 Headline numbers

| What | Count | Source |
| --- | --- | --- |
| Hand-held weapon blueprints the 12 levels load | **58** | [data] |
| ...of which can end up in your hands | **51** | [data], reach [inferred] |
| Guns (firearms) you can hold | **40** (37 in normal play + 3 edge cases) | [data] + [web] |
| Launchers | 6 | [data] |
| Grenades and mines | 2 | [data] |
| Melee and scripted items | 3 | [data] |
| Never in your hands (story characters, enemy-only, props, never given) | 7 | [data] |
| Attachment kinds | **24** (157 distinct unlocks; 161 with the enemies' two lights, the M9's New York unlock and an M1014 kit that lists the gun itself) | [data] |
| Mounted and vehicle weapon systems you use | **5**, plus the scripted mortar | [data] + [web] |
| Weapon pickups placed in the levels | 25 entities, **20 real** (5 are display models) | [data] |
| Weapon or kit crates | **none** (ammo crates only refill ammo) | [data] |

58 = 40 guns + 3 guns never in your hands + 11 other items + 4 other items never in your hands.

### 1.2 The 40 guns

**Given to you by a kit or a script (17)** [data]: M9, MP-443 Grach, AKS-74u, AS Val, PP-2000,
MP7, M4A1, M416, SCAR-H, AEK-971, SG 553, Mk11 Mod 0, M40A5, M82A3, M1014, 870 MCS, Saiga 12K.

**Placed pickups only, never in a kit (7)** [data]: M249, M98B, M27 IAR, M39 EMR, USAS-12, F2000,
AK-74M (the last three also drop from enemies, and the M249 from fallen Marines).

**Taken from the dead only (13)** [data]: AN-94, KH2002, G3A3, RPK-74M, PKP Pecheneg, Type 88
LMG, M60E4, UMP-45, P90, G36C, DAO-12, PDW-R, and the M16A4 (from fallen Marines and The Great
Destroyer's CIA agents; a Marine's M16A4 was picked up off the road in a Rock and a Hard Place
playtest [mod]).

**Edge cases (3):**

- **M240B:** only Montes and other friendlies carry it. The wiki says Montes's can be grabbed for a
  few seconds in Swordbreaker if he dies, as the mission fails [web]. A player kit with an M240B
  and an M9 (`Levels/SP_Earthquake/Assets/PlayerUSLMG`) is linked to the player at Swordbreaker's
  bridge, but no event in the level data was found to apply it [data; unused: inferred]; the wiki
  says Blackburn defended his squad with an M240B in the pre-alpha "Fault Line" trailers and uses an
  M249 instead in the final game [web], so the kit is likely a leftover [inferred]. The mod already
  has it built [mod].
- **SV-98:** Kaffarov's mercenary snipers drop it [data], but they stand on roofs out of reach and
  the rifle only rarely falls where you can get it [web].
- **SVD:** two snipers in Night Shift stand outside the playable area [web]; the third, in
  Uprising's briefing scene, is a canned-scenario actor, a soldier type with no drop component, so
  his rifle never lands [data].

### 1.3 The other 11 items you hold or use [data]

- **Launchers (6):** M320 (standalone in Guillotine; under the M416 in Rock and a Hard Place),
  SMAW, FIM-92 Stinger (Rock and a Hard Place's own version), FGM-148 Javelin (campaign version),
  M136 AT4, M224 60 mm mortar (campaign version: carried and set up).
- **Grenades and mines (2):** M67 frag grenade, M18 Claymore.
- **Melee and scripted (3):** knife, machete (Dima), handcuffs (Blackburn's cuffed hands; a held
  "weapon" that is really a state).

### 1.4 In the files but never in your hands (7)

| Item | Why | Source |
| --- | --- | --- |
| A-91 | Carried only by Vladimir (friendly) and by Kaffarov himself; no kit, no pickup | [data] |
| Glock 18 | Carried by Montes (friendly) and by one Guillotine soldier who is a scene actor (the bathroom knife scene). He is a canned-scenario soldier, a soldier type with no drop component (1.7), so the gun never lands. Older notes said "obtainable": corrected here | [data] |
| .44 Magnum (Taurus) | Solomon's revolver; the wiki says you never interact with it | [data] + [web] |
| RPG-7V2 (the enemies' version, `RPG7_AI`) | Every rocket soldier's kit has `DisableDeathPickup` | [data] |
| `M67_AI` | The enemies' own grenade (10000 of them), never given to you | [data] |
| `Weapon_Dummy` | A prop Solomon holds in Fear No Evil's scene with Miller | [data] |
| M26 MASS | Loaded by every level, given by no kit, pickup or spawn | [data] |

### 1.5 Attachments: 24 kinds [data]

- **Sights and scopes (13):** Kobra (and the AS Val's Comrades-only Kobra), RX01, EoTech, PK-AS,
  PK-A, M145, ACOG, PSO-1, PKS-07, rifle scope (the game's 8x on the Mk11, 6x on the SCAR-H), 12x
  ballistic scope, IRNV (1x thermal), and the thermal sniper scope (the game's 10x; two
  campaign-only unlocks: the M40A5's night scope in Night Shift and the M82A3's IRNV scope in
  Kaffarov). Against the unzoomed view those named powers are 6.8x, 5.1x, 10.3x and 8.5x (see 4.7).
- **Barrel (3):** suppressor (the AS Val's is built in), flash suppressor, heavy barrel.
- **Rail and underbarrel (5):** foregrip, bipod, flashlight, laser, the M320 HE under the M416.
- **Magazines and ammo (3):** extended magazine (or tube), flechette, FRAG rounds.
- **Not counted as kinds:** iron sights and the M16/M4 carry-handle sight (the default with no
  optic); the M9's New York unlock (it only swaps the bullet for a sniper round, which is why the
  M9 kills in one shot there); the enemies' weapon light and guard light (`ai_torch`,
  `ai_guardlight`), which may come along on a dropped gun (unverified).

### 1.6 Mounted and vehicle weapons you use

| System | Missions | Source |
| --- | --- | --- |
| PLR technical's pintle machine gun (a KORD in the data, modelled as a PKT) | SB (finale: "Get on the 50 cal") | [data] + [web] |
| M2HB .50, open mount | UP (Humvee turret), TR and FE (Abrams roof) | [data] + [web] |
| M1A2 Abrams: 120 mm cannon, 7.62 mm coaxial MG, zoom and thermal, driving | TR, FE | [data] + [web] |
| UAV laser-designation feed | TR | [data] + [web] |
| F/A-18F back seat: 20 mm cannon, Sidewinders, flares, targeting pod (HARM, JDAM, A-10 gun run) | GH | [data] + [web] |
| M224 mortar, deployed (scripted: you set it up, the crew fires) | OG | [data] + [web] |

Rides with no weapon of yours: the Growler in Rock and a Hard Place (you start in its back; its
turret, an M2HB in the data, has its own gunner) [data + web], the Humvee ride in Guillotine (a
passenger animation; the ride's Humvees carry a remote HMG in the data, and one more Humvee at the
ministry a grenade machine gun with no reload [data]; no objective puts you on either [web]),
Kaffarov's SUV and Ka-60, and The Great Destroyer's police-car chase, where you shoot the M9 from the
car [data] + [web].

### 1.7 How dead soldiers' guns reach you

When an enemy (or friendly) soldier dies, the game drops his slot-0 gun as a pickup (one shared
pickup definition for every soldier) unless his kit says `DisableDeathPickup` or his soldier type has
no drop component [data]:

- it lies 180 s and is taken within 2.5 m (hold R in the game);
- it carries 16-33 % of a magazine and 33-66 % of the spares;
- it keeps the soldier's attachments [data; IMFDB notes guns on the ground don't show them but
  come with them, web];
- it goes into your slot 0 (alternative slot 1) and empties the linked slot 2, so taking any gun
  in Rock and a Hard Place drops the M416 together with its M320 [data; unverified in play];
- a soldier kit with several slot-0 entries is a random pick per spawn [inferred from the data's
  shape].

Kits that never drop (`DisableDeathPickup`) [data]: every rocket soldier, Semper Fidelis's "Noob"
UMP-45 terrorists, the train's M9 guard, Kaffarov's Mk11 snipers and one SV-98 sniper, and a
knife-only wounded Marine.

Soldier types that never drop [data]: only `aisoldier` and `soldierextra` carry the drop component.
The canned-scenario soldiers and civilians (`cannedscenariosoldier`, `cannedscenariocivilian`,
`civilianextra`) and the vehicle crews (`aisoldiervehicle`) have none, so their guns never land
whatever their kits say: the Guillotine Glock 18, Uprising's briefing-scene SVD, Swordbreaker's two
UMP-45 Marines at the bazaar, Kaffarov's three gatehouse guards (MP7 or PDW-R), one AN-94 soldier
in Rock and a Hard Place's briefing on the hill, and most of the civilian actors' M16A4s and M9s. A
template can spawn as either type, so the same soldier can drop his gun in one place and not in
another.

---

## 2. Mission by mission

Everything in this section is [data] (the level files: kits, scripted kit changes, pickups,
enemy and friendly templates) unless marked. "New" means the mod has nothing for it yet [mod].

### 2.1 Semper Fidelis (SF, `SP_New_York`): Blackburn, bare cuffed arms

| Weapon | How | Attachments |
| --- | --- | --- |
| M9 | spawn kit `UsHumanSpSoldier_SP_Finale` (M9 + handcuffs): in hand from the start, as Blackburn swings into the car shooting [data + web]; the level's scripts also apply `CustomizeSoldierGunOnly` (M9 alone) and the spawn kit again | New York unlock (sniper bullet, one-shot kills) |
| Handcuffs | kit, melee slot | none |
| M1014 | pickup at the jammed train door, 32 shells; if you aren't holding it, the trigger `OnPlayerHoldingWrongWeapon` forces kit `_Shotgun` | pickup: extended tube + flechette; forced kit: + Kobra, + knife |
| AKS-74u | script: taken from a PLR operative in a QTE (kit `_AKs74u`, + knife); the M1014 is lost | none |
| AKS-74u | drop: end-of-train PLR | none |
| UMP-45 | drop: 2 of the 11 UMP-45 spawns; the other 9 (8 soldiers and an actor) use the no-drop "Noob" kit. The wiki says you can't pick one up here [web]: unverified | none |

- Never yours: the train's M9 guard (no drop), Solomon's .44, Montes's Glock 18 and M16A4.
- No grenades, no ammo crates. The mission is replayed inside The Great Destroyer.
- New for the mod: the M1014 (today the train-door shotgun "falls back to the game's own hands"
  [mod]), the AKS-74u, the UMP-45, the QTEs on the right grip. The M9 exists only as The Great
  Destroyer's train pistol [mod]; this mission uses the same kit and arms [data], so it should carry
  over [inferred].

### 2.2 Operation Swordbreaker (SB, `SP_Earthquake`): Blackburn

| Weapon | How | Attachments |
| --- | --- | --- |
| M4A1 | kit `UsHumanSpSoldier_SP_Earthquake`, slot 0 | EoTech |
| Mk11 Mod 0 | kit, slot 1 (Chaffin's overwatch [web]) | 8x rifle scope |
| M67, knife | kit, slots 6 and 7 | none |
| M136 AT4 | script: Matkovic hands it over on the hotel roof for the dug-in sniper (prompt "Hold X to pickup RPG"); kit `PlayerAT4` puts it in slot 0, then the start kit returns | none |
| M249 | pickup at the bridge fight, 600 rounds | bipod + ACOG + extended box (200) |
| AK-74M | drop (insurgents, PLR) | bare, PSO-1, PK-A, PK-AS or Kobra |
| G3A3 | drop | bare, ACOG or RX01 |
| RPK-74M | drop | bare, bipod, Kobra + foregrip, PK-A + bipod, RX01 + bipod |
| Saiga 12K | drop | bare or Kobra |
| AKS-74u, AEK-971, PKP | drop | bare |
| KH2002 | drop: one rushing extra | random |
| M16A4, M249 | fallen friendly Marines | bare |

- Mounted: the PLR technical's machine gun at the end ("Get on the 50 cal") [data + web].
- Friendlies whose guns you can't fairly take: Campo (M4A1 EoTech), Cole (M4A1 ACOG), the EOD
  Marine (M16A4 EoTech), Montes (M240B with M145: the edge case above). The level's only UMP-45s
  belong to two Marines at the bazaar who are canned-scenario actors: they can't drop them at all
  [data].
- Unused in the data [inferred]: kit `PlayerUSLMG` (M240B + M9) at the bridge.
- Scenes: dragging Chaffin, the cellar fight QTE, the IED wire [web + data]. The hotel "sniper" is a
  scripted dummy [data] (the wiki gives him an M82; this level loads no M82A3 [data + web]). Ammo
  crates: yes.
- New for the mod: Mk11, AT4, M249, PKP, Saiga 12K, AKS-74u, M67, knife, the technical, the EoTech
  and rifle-scope rows, the ACOG on a held gun.

### 2.3 Uprising (UP, `SP_Earthquake2`): Blackburn

| Weapon | How | Attachments |
| --- | --- | --- |
| Knife | kit: the only weapon at the start (`SP_Earthquake2`, `_Knife`, `PlayerUnarmed`) | none |
| AKS-74u | script: the guard's gun after the stealth knife kill (kit `_Silenced`, + M67 + knife) [data + web] | suppressor + Kobra + flashlight |
| AK-74M | drop (PLR) | bare, PSO-1, PK-A, PK-AS or Kobra; weapon-light versions; one with a guard light |
| KH2002 | drop | bare, PK-AS, Kobra, PK-A or PSO-1; weapon light; the Takavars always Kobra |
| G3A3 | drop | bare, ACOG or RX01; weapon light |
| AKS-74u | drop | bare (the extremists), or a weapon light with bare, Kobra or PK-AS |
| AEK-971 | drop | bare |
| Saiga 12K | drop | bare or Kobra |
| 870 MCS | drop (Takavar shotgunner) | weapon light |
| RPK-74M | drop | random |
| PKP | drop: the gunners at the final Humvee | bare |
| SVD | none to take: the one sniper, in the Solomon briefing scene, is a canned-scenario actor with no drop component [data] | PSO-1 |
| M16A4, M249 | fallen friendly Marines | bare |

- Mounted: the Humvee's M2HB turret at the extraction ("Enter gunner seat"), then the run to the
  Osprey [data + web].
- A second player spawn, in the level's default layer, carries Rock and a Hard Place's kit (M416
  with ACOG and M320, SMAW, M67, knife): that is why this level loads the M416 and the SMAW [data].
  The start is knife-only in play [web], so it is unused [inferred].
- Scenes: the crawl and the rat QTE, the stab prompt on Fire [data]. Wiki glitch: stabbing the
  guard's legs gives a random PLR gun [web]. Ammo crates: yes.
- New for the mod: the knife-only start, the AKS-74u with suppressor and light, the 870 pump, the
  Saiga 12K, the M2HB, the PKP.

### 2.4 Going Hunting (GH, `SP_Jet`): Hawkins

- Kit: the knife only [data], seen only in a rare glitch where she ejects and the mission goes on
  [web].
- Everything else is the F/A-18F back seat (`Vehicles/F18_SP03/F16_SP_Ride`, seat 1 with gunner
  controls): pre-flight checks, canopy and take-off prompts, the 20 mm cannon, AIM-9 Sidewinder heat
  lock, flares, the targeting pod with AGM-88 HARMs against the SAM sites, a laser-called JDAM strike
  on parked jets, and an A-10 30 mm gun run on the helicopter [data + web].
- Wiki trivia: switching back to the laser after the HARMs fails the mission [web].
- No hand-held guns. The level also loads the AK-74M, KH2002, G3A3, M16A4 and M9 for soldiers seen
  from the air and for the carrier's crew; none is within reach [data; inferred]. New for the mod:
  all of it.

### 2.5 Operation Guillotine (OG, `SP_Bank`): Blackburn

| Weapon | How | Attachments |
| --- | --- | --- |
| M224 mortar | kit `StartKitMortar`: the only item, carried down the hill and set up by a prompt; Campo fires an illumination round [data + web] | none |
| SCAR-H | kit `UsHumanSpSoldier_SP_Bank`, slot 0 | ACOG |
| 870 MCS | same kit, slot 1 (the apartment doors [web]); kept to the end | laser |
| M67, knife | same kit | none |
| M320 (standalone) | pickup in a crater with a dead Marine just past the first wall, 8 HE; it goes into slot 0 (alternative slot 1) like a gun, so it replaces the SCAR-H or the 870 [data; which one: inferred] | HE rounds |
| AEK-971 | pickup in the canal assault, 82 rounds | heavy barrel + foregrip + PK-AS |
| M98B | pickup inside the bank, 15 rounds | 12x ballistic scope + flash suppressor + bipod |
| M1014 | pickup in the bank basement by the dead worker near the elevator, 16 shells [data + web] | lists F2000 parts by mistake: probably bare [inferred] |
| FGM-148 Javelin | pickup from the dead Javelin team at the bank plaza (slot 5; on a checkpoint reload kit `Customize_Javelin` puts it in slot 0); removed by kit `_NoJavelin` after the three T-90s | none |
| M4A1 | kit `UsHumanSpSoldier_SP_005` after the T-90 chase, replacing the SCAR-H | ACOG |
| AK-74M, KH2002, G3A3 | drop (PLR; the general in the ministry has a G3A3) | as in Uprising; the Takavars' KH2002 with Kobra |
| AKS-74u | drop | weapon light with bare, Kobra or PK-AS |
| AEK-971, RPK-74M, PKP | drop (the PKPs man the MG nests by the canal [web]) | bare or random |
| M16A4, UMP-45, M249 | fallen friendly Marines (the wiki confirms you can take their UMP-45s [web]) | bare |

- Never yours: the Glock 18 of the bathroom-scene soldier (see 1.4).
- Scenes: the mortar set-up, door kicks and breaches, Matkovic's bathroom knife QTE, the atrium
  knife kill, the lift-shaft rappel with a flashbang, a Humvee ride as a passenger [data + web].
  Ammo crates: yes.
- New for the mod: almost everything: the mortar carry, SCAR-H, 870, standalone M320, M98B, M1014,
  Javelin, UMP-45, PKP, M249, AKS-74u, the ACOG on held guns, the laser, the bipod, the 12x scope.

### 2.6 Comrades (CO, `SP_Paris`): Dima, Russian arms

| Weapon | How | Attachments |
| --- | --- | --- |
| AS Val | kit `SP_Dima_Sp_06`, slot 0 | Kobra (a Comrades-only unlock) |
| AKS-74u | same kit, slot 1 | PK-A |
| M67, machete | same kit | none |
| AKS-74u | pickup on the upper office floor, 200-400 rounds | Kobra |
| AK-74M | pickup on the upper office floor, 200-300 rounds | Kobra + foregrip |
| M1014 | pickup in the gas-sluice area | lists F2000 parts: probably bare [inferred] |
| AK-74M, AEK-971, PKP, M1014, MP7, AKS-74u, UMP-45 | drop (PLR terrorists) | bare (the MP7 lists another gun's laser: probably bare [inferred]) |
| F2000 | drop (PLR) | bare, or EoTech on the unmasked ones |
| G36C, 870 MCS, P90, UMP-45 | drop (French police) | bare |
| G36C | drop (GIGN) | EoTech, RX01, ACOG, EoTech or RX01 + foregrip, ACOG + heavy barrel, laser |
| SG 553 | drop (GIGN) | ACOG, foregrip, RX01, RX01 + foregrip, ACOG + heavy barrel, EoTech, laser + foregrip; two variants carry a G36C sight unlock that can't fit, so no sight at all [data + web] |
| UMP-45 | drop (GIGN) | RX01, EoTech, RX01 + laser |
| P90 | drop (GIGN) | EoTech, RX01, EoTech + laser |

- Allies: Vladimir (A-91 + MP-443), Kiril (AS Val): not yours.
- Scenes: the opening car ride, two optional bank QTEs, the gas mask, the machete cutting the wire,
  the jammer check, the metro fight QTE [data + web]. Ammo crates: yes.
- New for the mod: Dima's arms (every gun here waits on them), AS Val, AKS-74u, G36C, SG 553,
  UMP-45, MP7, P90, F2000, PKP, M1014, 870, machete, the EoTech and RX01 rows, the laser.

### 2.7 Thunder Run (TR, `SP_Tank`): Miller

- The M1A2 Abrams (`Vehicles/M1A2/M1Abrams_SP007`): you drive it and fire its 120 mm cannon and
  coaxial MG, with zoom and thermal sights [data + web].
- A UAV feed with the bombing display calls a strike on the desert fort (`Levels/SP_Tank/Prefabs/Dummy_UAV`)
  [data]; the wiki says A-10s through an MAV [web].
- On foot at the minefield: kit `SP_Tank_WithKit` = M4A1 (EoTech) + knife, no grenades [data]. The
  MICLIC detonator is picked up and fired through interactions, not a weapon [data + web].
- Then the M2HB on the Abrams roof (petrol station, car bombs) [data + web].
- On-foot drops: AK-74M, KH2002, G3A3 (PLR) [data]. The wiki also lists the AEK-971; the level data
  has none [data + web].
- Friendlies: Marines (M16A4, M249), the engineer (M16A4 + M9), tank crews (M240B + M9). No ammo
  crates.
- New for the mod: the tank, the UAV, the roof M2HB, the MICLIC scenes.

### 2.8 Fear No Evil (FE, `SP_Tank_b`): Miller

- The whole mission is in the Abrams: cannon and coax, then the roof M2HB for the hold-out after
  the Javelin hit [data + web]. A captured Javelin fired by the PLR hits your tank (scripted) [web].
- The kit's M4A1 (EoTech) is never used on foot [inferred].
- Enemies (AK-74M, KH2002, G3A3, AKS-74u, AEK-971, RPK-74M, rocket soldiers) are only seen from the
  tank [inferred]. Friendlies: Blackburn (M240B + M9), the rest of Misfit (each a UMP-45 or an M249
  at random) [data + web], two Marines with M16A4s and a tank crewman with an M240B [data]; none to
  take.
- A second player spawn, in the outro scene (Miller's execution), carries Rock and a Hard Place's kit
  (M416 with ACOG and M320, SMAW, M67, knife): that is why this level loads them [data]. Never
  yours [inferred].
- New for the mod: the same as Thunder Run.

### 2.9 Night Shift (NS, `SP_Sniper`): Blackburn

| Weapon | How | Attachments |
| --- | --- | --- |
| M40A5 | kit `UsHumanSpSoldier_SP_Sniper`, slot 0: the lights, the overwatch, Al-Bashir's car [data + web] | the campaign-only night scope unlock: 8.5x thermal scope + suppressor + straight-pull bolt rules + the campaign sniper bullet |
| MP7 | same kit, slot 1; in the sewers kit `_FlashlightOn` adds the light and selects it | EoTech + suppressor + extended magazine (40) (+ flashlight) |
| Knife | same kit; no grenades | none |
| M18 Claymore | kit `_Claymores` at the mall defence: slot 4, selected ("Deploy defensive perimeter") | none |
| M40A5 x2 | pickups at the Al-Bashir strike overwatch, 50 rounds each | the same night scope |
| AK-74M, KH2002, G3A3 | drop (PLR) | as in Uprising, many with weapon lights |
| AEK-971, RPK-74M | drop | bare or random |
| Saiga 12K | drop | bare or Kobra |
| PKP | drop | bare; one with PKS-07 |
| AKS-74u | drop | weapon light with bare, Kobra or PK-AS |
| 870 MCS | drop (Takavar shotgunner) | weapon light |
| SVD | drop in the data, but the snipers stand outside the playable area [web] | PSO-1 |
| M16A4 | fallen friendly Marines | bare |

- Friendlies: Campo (Mk11 with EoTech + suppressor), Cole (M4A1 ACOG) and an M240B gunner with
  night vision whose template is named for Montes (the wiki says Montes isn't in this mission and
  that Matkovic mans an M240B in it) [data + web]: not yours.
- Scenes: the knife kill of a sentry, a knife-fight QTE, shooting out the lights (an achievement
  asks for 4 lights with 4 bullets), the overwatch, Al-Bashir's car [web]. Ammo crates: 3 [data].
- New for the mod: everything (the turn bolt, the thermal scope picture, the MP7's grip magazine
  and light, claymores, the knife scenes).

### 2.10 Rock and a Hard Place (RH, `SP_Valley`): Blackburn

| Weapon | How | Attachments |
| --- | --- | --- |
| M416 | kit `UsHumanSpSoldier_Valley`, slot 0 | ACOG + M320 HE |
| M320 (under the M416) | same kit, slot 2, linked to slot 0 | HE rounds |
| SMAW | same kit, slot 5; refilled by a script while a BMP lives, and by ammo crates [data + web] | none |
| M67, knife | same kit; no pistol | none |
| FIM-92 Stinger | pickup in the wrecked Growler in the jet run (slot 1, 3-5 missiles, a hidden ammo crate within 10 m) | none |
| AN-94 | drop (Russian riflemen; the commonest pickup) | random: bare, foregrip, Kobra, Kobra + foregrip, PK-A, PK-AS, PKS-07, PSO-1 |
| RPK-74M | drop (Russian machine gunners) | always a bipod or foregrip, with PSO-1, PK-AS, PK-A, Kobra or none |
| M16A4 | fallen friendly Marines (the AAV crews die) | carry-handle sight |

- No drop: the rocket soldiers (AEK-971 + RPG-7) [data].
- One forest soldier with the PLR kit (AK-74M, KH2002 or G3A3) is a scripted actor on your side;
  his rifle would drop only if he died [data; inferred]. One AN-94 soldier in the briefing on the
  hill is a canned-scenario actor and never drops his [data]. One more enemy in the jet run's logic
  carries an M16A4 and an M9 (an AA-crew template), most likely the jet's pilot [data; inferred].
- Friendlies: Campo (M4A1 EoTech + M9), Cole (M4A1 ACOG), Montes (M240B): not yours.
- Optional knife takedowns (an achievement asks for one) [web]. Ammo crates: yes.
- Mod today: the whole mission is playable in VR and was finished in the headset; every gun of it
  is drawn with a reload by hand [mod]. Still open: either hand for everything but the M416's
  free hands, grenades, the knife, recoil.

### 2.11 Kaffarov (KA, `SP_Villa`): Dima, then Blackburn

| Weapon | How | Attachments |
| --- | --- | --- |
| PP-2000 | kit `SP_Dima_Sp_10_HALO` (applied by the HALO and landing scripts): the only gun until the weapons case | none (the HALO spawn's kit `UsHumanSpSoldier_SP_Villa` lists a PP-2000 with Kobra + extended magazine + flashlight, a Saiga 12K with PK-AS, flechette and light, an M67 and a knife; the HALO kit removes them all [data]; that they never show: [inferred]) |
| Machete | the same kit and the next | none |
| AEK-971 | kit `SP_Dima_Sp_10` from the weapons case at the landing, slot 0 | PK-A + laser + foregrip + flashlight |
| Saiga 12K | same kit, slot 1 | PK-AS + laser + flechette + flashlight |
| M67 | same kit, slot 6 | none |
| MP-443 | same kit, slot 9: used in the SUV drive QTE [web]; no key reaches slot 9 (only a mouse-wheel glitch does [web]) | none |
| M82A3 | script in the pool house after a fist-fight QTE: kit `SP_Dima_Sp_10_Sniper` replaces slot 0 (it can appear early if you stand at the door [web]) | the campaign-only IRNV scope (8.5x thermal) |
| AEK-971 | pickup in the pool house that gives it back after the M82A3 | PK-A + laser + foregrip + flashlight |
| SCAR-H | pickup in the basement armory | RX01 + foregrip + flashlight |
| USAS-12 | pickup in the basement armory | PK-A + flashlight + FRAG rounds |
| M39 EMR | pickup in the basement armory | PK-AS + foregrip + flashlight |
| M27 IAR | pickup in the basement armory | RX01 + foregrip + flashlight |
| F2000 | pickup in the basement armory | EoTech + foregrip + flashlight |
| AEK-971, AK-74M, F2000, SCAR-H | drop (PMC riflemen) | many: Kobra, PK-AS, PSO-1, ACOG, RX01, EoTech, the SCAR-H's rifle scope (the game's 6x), foregrip, bipod; no weapon lights; IRNV in the courtyard after the pool house |
| M60E4 | drop (PMC heavy gunners) | ACOG, EoTech, M145 or RX01 |
| RPK-74M, Type 88 | drop (PMC machine gunners) | bipod, extended box, Kobra, PK-A, PK-AS, PSO-1, ACOG, RX01, M145; IRNV |
| DAO-12, USAS-12 | drop (PMC shotgunners, the commonest enemy here) | extended magazine, Kobra, EoTech or RX01, flechette; weapon light, laser, IRNV |
| PDW-R, MP7 | drop (PMC SMG gunners) | extended magazine, EoTech, Kobra, RX01 or ACOG; weapon light, laser |
| SV-98 | drop in the data, but rarely within reach [web] | PKS-07 |
| M416 | kit `UsHumanSpSoldier_SP_Villa_Blackburn`: Blackburn's part at the end (he shoots Cole) | ACOG; the M320 is drawn but has no launcher slot, so it can't fire [data + web] |

- The weapons case at the landing holds display models of the AEK-971, the Saiga and three M67s
  (`AllowPickup` false); the kit comes by script [data].
- No drop: the rocket soldiers, the Mk11 snipers, one SV-98 sniper [data]; the wiki says the
  checkpoint guards' MP7s couldn't be picked up either [web], and the data agrees: the three
  gatehouse guards are canned-scenario soldiers, with no drop component [data].
- Allies: Vladimir (A-91), Kiril (AS Val). Kaffarov carries an A-91 and an M9. Not yours.
- Scenes: the HALO jump, the SUV drive QTE, the pool-house fist fight, the Ka-60 fight QTE, the
  fight with Kaffarov [data + web]. Ammo crates: yes.
- New for the mod: Dima's arms and nearly every gun: PP-2000, the AEK-971's light, laser and
  foregrip, Saiga 12K, MP-443, M82A3 with its thermal scope, SCAR-H, USAS-12, M39 EMR, M27 IAR,
  F2000, M60E4, Type 88, DAO-12, PDW-R, MP7, SV-98, the machete, and new sights on the AK-74M and
  RPK-74M (IRNV, M145, RX01, EoTech, ACOG).

### 2.12 The Great Destroyer (GD, `SP_Finale`): Blackburn, bare cuffed arms

| Weapon | How | Attachments |
| --- | --- | --- |
| M9 + handcuffs | spawn kit `UsHumanSpSoldier_SP_Finale` (the CIA escape, the street) | New York unlock |
| M9 | the train (the mod's "train pistol"); kit `CustomizeSoldierGunOnly` for the police-car chase: auto-aim, endless reloads, a zoom without iron sights [data + web] | bare in the chase |
| M1014 | pickup on the train (as in Semper Fidelis), or the forced kit | extended tube + flechette (+ Kobra) |
| AKS-74u | QTE kit `_AKs74u_Only` (fills slots 0 and 1: one gun) | none |
| SG 553 | kit `UsHumanSpSoldier_SP_Finale_SIG` for the Penn Station sewer chase, + knife | heavy barrel + foregrip, no optic (the wiki says "no attachments" [web]) |
| Handcuffs | kit `_Knife` for the final fight: handcuffs alone (the strangling QTE, then the brick) | none |
| UMP-45 | drop (PLR; can be taken on the train [web]) | bare |
| MP7 | drop (PLR with weapon lights; Solomon's guards) | weapon light; guards: IRNV + flash suppressor + laser |
| Saiga 12K | drop (Solomon's guards) | IRNV + laser |
| M1014, AK-74M, AEK-971, AKS-74u | drop | bare |
| F2000 | drop | bare or EoTech |
| Type 88 | drop (2 PLR gunners, one just before the car chase [web]) | bare |
| M16A4 | drop in the data from the CIA agents in the escape; unconfirmed in play | bare |

- Melee is off on the train [web]. Friendlies: Montes (Glock 18; an M9 in the final fight's kit),
  Whistler (M4A1 ACOG): not yours [data].
- No grenades, no ammo crates.
- Mod today: only the train M9 is supported (bare arms, a reload rehearsal) [mod].

---

## 3. Every weapon

One row each. Missions and attachments are [data]; **Mod today** is [mod] (the code and the
playtests as of 2026-11-11); **VR needs** and **Difficulty** are [inferred] from the family studies
in section 4. "Left" means built for a left support hand with the gun in the right hand. Every row
also needs the shared foundations of section 4.1 (either hand, free hands, the level's arms,
recoil), which aren't repeated.

Difficulty: **Built** (done, foundations left); **Easy** (blocks that exist, new numbers);
**Medium** (one new block); **Hard** (a new mechanic family, or new rendering work).

### 3.1 The 40 guns

**Pistols**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| M9 | Pistol, 9 mm: grip magazine with a button, slide, slide stop | SF spawn kit; GD kits (escape, train, street, car chase) | New York unlock (bullet only) | The Great Destroyer's train pistol only (bare arms): drawn, two-hand hold, magnetic magazine and a three-grip slide for the left hand; a rehearsal (no rounds change), no collision, behind `build/pistol.txt` | real rounds, the magazine on the loose physics, collision, either hand, the slide-stop release, a holster, the police-car ride | Medium |
| MP-443 Grach | Pistol, 9 mm: the same generic reload as the M9 | KA kit, slot 9 (the SUV drive QTE) | none | none | the M9's set measured on this gun; Dima's arms; a way to draw it (no key reaches slot 9) | Easy-Medium, low priority |

**Submachine guns and PDWs**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| UMP-45 | SMG, .45: paddle magazine, HK forward-left handle with a notch | OG fallen Marines; CO drops (police, GIGN, PLR); GD drops; SF drops (2 spawns, unverified) | bare; GIGN: RX01, EoTech, RX01 + laser | measured on Semper Fidelis and built offline 2026-11-18 ([ump45.md](ump45.md)): a held gun, its magazine straight down by hand, the handle with its notch and the knock; not run in the game yet | a film and a try; the bare arms on the train; burst; the RX01 | Easy-Medium |
| MP7 | PDW, 4.6 mm: magazine in the grip, rear T-handle, bolt catch | NS kit slot 1 (light added in the sewers); CO, KA, GD drops | NS: EoTech + suppressor + 40-round magazine; drops: EoTech, Kobra, RX01, extended, laser, light; GD guards: IRNV + flash suppressor + laser | none | magazine up into the trigger hand's grip; the 20-round falls free, the 40-round is pulled; bolt release by the trigger hand's thumb; two magazine profiles; the light | Medium |
| P90 | PDW, 5.7 mm bullpup: horizontal magazine on top | CO drops (police, GIGN) | bare; EoTech, RX01, EoTech + laser | none | a new top magazine (hinge at the front, latch at the rear); handles on both sides; cases out of the bottom | Hard |
| PP-2000 | SMG, 9 mm: magazine in the grip, folding handle on top at the front | KA kit (HALO jump to the weapons case) | none | none | the grip magazine; a pinch or hook pull from above; Dima's arms | Medium |
| PDW-R | PDW, 5.56 bullpup: STANAG behind the grip with a button, front side handle | KA drops | extended + EoTech, Kobra or RX01, or ACOG; light; laser | none | the KH2002's magazine; a forward side handle; 20 and 30-round sets; the ACOG on a held gun | Easy-Medium |
| AS Val | Suppressed 9x39 carbine (a PDW in BF3's classes): AK pattern | CO kit slot 0 | the Comrades-only Kobra | none | the AK family; the long built-in suppressor's muzzle; Dima's arms | Easy |

**Carbines**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| AKS-74u | Carbine, 5.45: AK pattern | SF and GD QTE kits; UP kit; CO kit slot 1 and a pickup; drops in SB, UP, OG, CO, NS, GD, SF | UP: suppressor + Kobra + flashlight; CO: PK-A; pickup: Kobra; drops: bare, light, Kobra, PK-AS | measured on Semper Fidelis and built offline 2026-11-18 ([aks74u.md](aks74u.md)): a held gun with the AK family's reload by hand; not run in the game yet | a film and a try; the bare arms on the train; the light moved to the drawn gun; the suppressor's muzzle; three arm rigs | Easy-Medium |
| M4A1 | Carbine, 5.56: AR | SB, TR, FE kits; OG kit after the T-90 chase | EoTech; ACOG (OG) | Left: button or hand magazine, magnet, T-handle, bolt slap, port flap; no verdict of its own (shares the M16A4's) | either hand; the EoTech row; the ACOG on a held gun | Built |
| SCAR-H | Carbine, 7.62: button magazine, left-side moving handle, bolt release | OG kit slot 0; KA pickup and drops | OG: ACOG; pickup: RX01 + foregrip + light; drops: ACOG, EoTech, RX01, rifle scope (the game's 6x), bipod, foregrip, IRNV | none | the M4 blocks plus a left-side handle; the ACOG on a held gun; foregrip; light | Medium |
| G36C | Carbine, 5.56: paddle magazine, folding handle on top at the rear | CO drops (police, GIGN) | bare; EoTech, RX01, ACOG, + foregrip, ACOG + heavy barrel, laser | none | magazine by hand; the top handle locked back when empty; a release in front of the trigger guard | Medium |
| SG 553 | Carbine, 5.56: paddle magazine, right handle, bolt catch on the left | GD kit (sewer chase); CO drops (GIGN) | GD: heavy barrel + foregrip; GIGN: ACOG, EoTech, RX01, foregrip, laser | none | the AK magazine plus the M4's bolt-catch slap, moved; foregrip | Easy-Medium |

**Assault and battle rifles**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| M416 | Assault rifle, 5.56: AR (HK416) | RH kit; KA (Blackburn's part); a leftover spawn in UP and FE (unused) | ACOG + M320 HE | Left: everything, approved; free hands (either hand, drop, throw, pick up) approved; every reload by either hand built in the working copy, not tried in the headset | the either-hand try-out; weapon picks while it lies; fire mode; recoil | Built |
| AK-74M | Assault rifle, 5.45: AK | drops in SB, UP, OG, CO, TR, NS, KA, GD; CO pickup | random Kobra, PK-A, PK-AS, PSO-1; lights; KA: foregrip, bipod + PSO-1, IRNV; pickup: Kobra + foregrip | Left: pull, flick, magnet, lever over and under | either hand; three arm rigs; IRNV, foregrip, bipod | Built (gun); Medium for its missions |
| AN-94 | Assault rifle, 5.45: AK (the baseline) | RH drops | random 5 sights, foregrip | Left, approved as the baseline | either hand; a real pickup in the headset; burst mode | Built |
| AEK-971 | Assault rifle, 5.45: AK | KA kit, pool-house pickup and drops; OG pickup and drops; drops in SB, UP, CO, NS, GD | KA: PK-A + laser + foregrip + light; OG: heavy barrel + foregrip + PK-AS; drops: many | Left | either hand; Dima's arms; the light and laser switch; foregrip | Built (gun); Medium in KA |
| KH2002 | Bullpup rifle, 5.56: STANAG with a button, hook on top | drops in UP, OG, TR, NS; one SB extra | random sights; light; Takavars: Kobra | Left, approved | either hand (a magazine behind the grip); burst mode | Built |
| G3A3 | Battle rifle, 7.62: HK roller-delayed | drops in SB, UP, OG, TR, NS | bare, ACOG, RX01; light | Left, approved; no notch | the notch and the HK slap; either hand; RX01 | Easy-Medium |
| F2000 | Bullpup rifle, 5.56: STANAG pulled by hand, handle forward on the left | KA pickup and drops; CO and GD drops | pickup: EoTech + foregrip + light; drops: EoTech, ACOG, RX01, foregrip, IRNV | none | measure it; the magazine pulled out behind the grip; the forward-left handle, no hold-open; cases out near the muzzle | Medium |
| M16A4 | Assault rifle, 5.56: AR, carry handle | fallen Marines (SB, UP, OG, TR, NS, RH); GD's CIA agents | carry handle; EoTech (the EOD Marine) | Left, approved; picked up in a playtest; the port flap's verdict pending | either hand | Built |

**Machine guns**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| RPK-74M | LMG, 5.45: AK, 45-round magazine | drops in SB, UP, OG, NS, RH, KA | bipod or foregrip with sights; IRNV (KA) | Left; the per-shot lever kick not tried | either hand; bipod; foregrip; RX01, IRNV | Built |
| M240B | GPMG, 7.62: belt, lid, box | edge case (Montes, SB); an unused kit | none | Left, approved: lid, box, belt pinch, lever, belt by count | either hand; the right-hand lever as in the game's own reload; links | Built |
| M249 | LMG, 5.56: belt from a box, left feed | SB pickup; fallen Marines (SB, UP, OG, TR) | pickup: bipod + ACOG + 200-round box | none | the M240's belt blocks; the ACOG riding the cover; bipod; a 200-round belt | Medium |
| PKP Pecheneg | GPMG, 7.62x54R: belt from the right | drops in SB, UP, OG, CO, NS | bare; one PKS-07 (NS) | none | the M240's blocks mirrored (feed right, cases left); the support hand reaches across | Medium-Hard |
| Type 88 LMG | LMG, 5.8 mm: belt, latched box | KA and GD drops | KA: bipod + ACOG, RX01, M145, extended or IRNV; GD: bare | none | the belt blocks plus a box latch, box before belt; bipod | Medium |
| M60E4 | GPMG, 7.62: belt, left feed | KA drops | ACOG, EoTech, M145 or RX01 (on the cover) | none | nearly the M240 as is; a sight riding the cover; carry handle | Medium |
| M27 IAR | Automatic rifle, 5.56: AR (HK416-based) | KA pickup | RX01 + foregrip + light | none | the M416's set; a 45-round magazine fit; foregrip; light | Easy |

**Shotguns**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| 870 MCS | Pump shotgun, tube magazine | OG kit slot 1; drops in UP, NS, CO | laser (OG); light (drops) | none | the pump; shells one by one into the tube; the port load; no shot until pumped | Hard |
| M1014 | Semi-automatic shotgun, tube magazine | SF and GD train pickup and forced kit; OG and CO pickups; CO and GD drops | extended tube + flechette (+ Kobra) [the train's, measured 2026-11-18: 5 loaded, no longer tube drawn] | Built 2026-11-18, not yet run in the game: a held gun with its shells by hand (m1014.md) | shells into the tube; the port load; bolt hold-open and its release button; the right-side handle; the train-door grab | Medium-Hard |
| Saiga 12K | Semi-automatic shotgun, AK pattern, box magazine | KA kit slot 1; drops in SB, UP, NS, GD | KA: PK-AS + laser + flechette + light; drops: bare or Kobra; GD guards: IRNV + laser | none | the AK family; its magazine is a separate mesh | Easy-Medium |
| USAS-12 | Automatic shotgun, box magazine | KA pickup and drops | pickup: PK-A + light + FRAG; drops: many | none | button magazine (separate mesh); the knob on the left of the forearm; full auto | Medium |
| DAO-12 | Revolving-drum shotgun | KA drops | always the 12-shell drum, with sights, flechette, light, laser or IRNV | none | wind the drum; shells through the loading gate | Medium-Hard |

**Marksman and sniper rifles**

| Gun | Family | Missions (how) | Campaign attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| Mk11 Mod 0 | Marksman rifle, 7.62: AR-10 | SB kit slot 1 | 8x rifle scope | none | the M16/M4 blocks; the 8x scope row | Easy |
| M39 EMR | Marksman rifle, 7.62: M14 | KA pickup | PK-AS + foregrip + light | none | rocked-in magazine; the right-side op rod, held back when empty; foregrip; light | Medium |
| SVD | Marksman rifle, 7.62x54R: AK-like | edge case (NS; Uprising's can't drop) | PSO-1 | none | the AK family, held back when empty | Easy-Medium, last |
| M40A5 | Bolt-action sniper rifle, 7.62 | NS kit slot 0 and 2 pickups | the night scope unlock (8.5x thermal + suppressor + straight-pull rules) | none | the turn bolt; the thermal picture in the glass; the suppressor's muzzle | Hard |
| M98B | Bolt-action sniper rifle, .338 | OG pickup | 12x ballistic scope + flash suppressor + bipod | none | the turn bolt (after the M40A5); bipod; the 12x row | Medium |
| SV-98 | Bolt-action sniper rifle, 7.62x54R | edge case (KA) | PKS-07 | none | the turn bolt; its sight is built | Medium, low priority |
| M82A3 | Semi-automatic .50 rifle | KA script (pool house) | the campaign-only IRNV scope (8.5x thermal) | none | rocked-in magazine; a big right-side handle, held back when empty; the thermal picture; the biggest recoil; the scripted hand-over | Medium-Hard |

### 3.2 Launchers, grenades, mines and melee

| Item | Family | Missions (how) | Attachments | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- | --- |
| M320 | 40 mm, single shot, breech swings out to the left | RH under the M416 (kit, slot 2); OG standalone pickup; KA drawn, unusable | HE only | Under the M416: grip, fire and reload by hand, left hand, approved; either hand in the working copy, not tried | either hand; the grenade from the drawn tube; its own collision; standalone: measure, a stocked hold, the same reload module | Built (under) / Medium (standalone) |
| SMAW | Rocket launcher: sealed case on the back, twist lock | RH kit slot 5 | none | Left, approved: grip, rocket at the muzzle, case off and on by hand | either hand; drop and pick up; a carry place; the spent case kept drawn while the rifle is out | Built |
| FIM-92 Stinger | Heat-seeking missile: gripstock, tube, IFF aerial | RH pickup | none | Left, approved: lock without zoom, missile from the tube, the aerial lifted by hand | lock only with the tube at the eye; the lock marker hidden; either hand; the pickup by hand | Built |
| FGM-148 Javelin | Anti-tank missile: command launch unit (CLU) on a tube | OG pickup (or kit on a checkpoint reload) | none | none | the CLU's two handgrips; its sight picture; lock without zoom; the missile from the tube; the tube swapped by hand (record it first) | Medium-Hard |
| M136 AT4 | Throw-away anti-tank launcher | SB script | none | none | hold, prime (pin, cocking lever, safety, button), fire, let go of the spent tube; its own spread cleared | Medium |
| M224 mortar | 60 mm mortar, a deploy weapon | OG kit at the start | none | none | the carry pose; the set-up prompt on the right grip; the scene as drawn | Easy |
| M67 | Frag grenade | kits in SB, UP, OG, CO, RH, KA | none | none: the game draws it, and its button (G) is unbound | taken from the left chest; the other hand's finger pulls the ring; toss and catch; thrown with the hand's own speed | Medium-Hard |
| M18 Claymore | Directional mine | NS kit (mall defence) | none | none | taken from the right chest; set on the ground facing the enemy's path | Medium |
| Knife | Melee | kits in SF, SB, UP, GH (her only item), OG, TR (on foot), NS, RH, GD; the only weapon at Uprising's start; not in KA's real kits (Dima carries the machete, Blackburn's kit is the M416 alone) | none | none | drawn from a sheath; a real stab or slash sends the game's melee; takedowns as locked scenes; prompts | Medium-Hard |
| Machete | Melee (Dima) | CO, KA kits | none | none | as the knife, with a chop; the wire cut is a prompt | Easy after the knife |
| Handcuffs | Cuffed hands (a held state) | SF, GD kits | none | The bare-arm rig with the cuff on the right wrist | QTEs on the right grip; the chain's swing (parked: it jittered) | Easy |

### 3.3 Attachments

| Attachment | Kind | Guns and missions | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- | --- |
| Kobra | 1x reflex (game zoom 1.43x) | drops nearly everywhere (AK-74M, KH2002, AKS-74u, RPK, AN-94, Saiga, AEK, DAO-12, USAS-12, PDW-R, MP7); AS Val (CO kit); AKS-74u (UP kit); M1014 (forced kit); pickups | Built (power 1.0, marks) | nothing by hand | Built |
| RX01 | 1x reflex (1.43x) | G3A3, RPK; CO's G36C, SG 553, UMP-45, P90; KA's SCAR-H, M27 IAR, F2000, M60E4, Type 88, PDW-R, MP7, shotguns | none | a sight row and its marks | Easy |
| EoTech | Holographic (game zoom 1.82x) | M4A1 (SB, TR, FE kits); MP7 (NS kit); F2000 pickup; many drops in CO, KA, GD | none | a sight row and marks; a decision on its power (1.82x as the game, or 1x as a real one) | Easy |
| PK-AS | Holographic (1.82x) | Saiga (KA kit), AEK (OG pickup), M39 (KA pickup), drops | Built (a window in its closed tube) | nothing | Built |
| PK-A | 2.95x | AKS-74u (CO kit), AEK (KA kit and pickup), USAS-12 (KA pickup), drops | Built (real picture) | nothing | Built |
| M145 | 2.95x machine-gun scope | M60E4, Type 88 (KA); Montes's M240B | none | a sight row with the real picture; it rides the feed cover | Easy-Medium |
| ACOG | 3.44x | M416 (RH); SCAR-H and M4A1 (OG kits); M249 (SB pickup); drops: G3A3, G36C, SG 553, SCAR-H, F2000, M60E4, Type 88, PDW-R | Built on the M416 only | a held-gun row; on belt-fed guns it rides the cover | Easy |
| PSO-1 | 3.44x, side mount | drops (AK-74M, KH2002, AN-94, RPK, AEK); the SVD | Built (real picture) | nothing | Built |
| PKS-07 | 6x | AN-94 (RH), one PKP (NS), SV-98 (KA) | Built | on the PKP it may ride the cover | Built |
| Rifle scope | the game's 8x (Mk11) and 6x (SCAR-H): 6.8x and 5.1x against the unzoomed view | Mk11 (SB kit); SCAR-H drops (KA) | none | a sight row; the power taken from the gun, not the mesh | Easy-Medium |
| Ballistic scope | the game's 12x (10.3x) | M98B (OG pickup) | none | a sight row at about 10x | Easy-Medium |
| IRNV | 1x thermal (1.43x) | KA courtyard drops (AEK, AK-74M, RPK, SCAR-H, F2000, Type 88, DAO-12, USAS-12); GD guards' MP7 and Saiga | none | a thermal picture inside the glass only; the second renderer switched on by need, not by power | Hard |
| 8.5x thermal sniper scope (the game's 10x) | Campaign-only unlocks | M40A5 (NS), M82A3 (KA) | none | the same thermal picture at 8.5x | Hard |
| Suppressor | Barrel | AKS-74u (UP kit), MP7 (NS kit), M40A5 (NS); the AS Val's built-in | none | the muzzle point and the gun's length | Easy |
| Flash suppressor | Barrel | M98B (OG); GD guards' MP7 | none | the muzzle point | Easy |
| Heavy barrel | Barrel | AEK (OG pickup), SG 553 (GD kit), GIGN G36C and SG 553 | none | nothing (no mesh; a heavier bullet) | Easy |
| Foregrip | Rail | AEK (KA kit, pickups), SG 553 (GD kit), AK-74M (CO pickup), the KA basement guns, drops | none | a vertical-grip support hold from the game's own foregrip pose | Easy-Medium |
| Bipod | Rail | M249 (SB), M98B (OG), RPK (most of them, in every level it drops in), AK-74M, SCAR-H, Type 88 (KA) | drawn on the RPK, no deploy | legs by hand; resting on surfaces | Medium |
| Flashlight | Rail | AKS-74u (UP kit), MP7 (NS sewers), AEK and Saiga (KA kit), the KA basement guns; the enemies' light on many drops | none | a switch by either hand; the light moved to the drawn gun | Medium-Hard |
| Laser | Rail | 870 (OG kit), AEK and Saiga (KA kit), drops in CO, KA, GD | none | the same switch; the dot and beam from the drawn gun | Medium-Hard |
| M320 HE | Underbarrel | M416 (RH) | Built (left) | either hand (in the working copy) | Built |
| Extended magazine or tube | Magazine | MP7 (NS kit), M1014 (train), M249 (SB), KA drops | none | per-magazine fits in the magazine module; 200-round belts | Easy-Medium |
| Flechette | Shotgun ammo | M1014, Saiga, DAO-12, USAS-12 | n/a | nothing | none |
| FRAG | Shotgun ammo | USAS-12 (KA pickup) | n/a | nothing | none |

### 3.4 Mounted and vehicle weapons

| System | Missions | Mod today | VR needs | Difficulty |
| --- | --- | --- | --- | --- |
| PLR technical's machine gun | SB | none | a seat; both hands on the spade grips; the gun turning about its pintle; triggers fire; the game's overheat | Hard |
| M2HB .50 | UP (Humvee), TR and FE (Abrams roof) | none | the same blocks; a full circle while seated (the body turned by the stick) | Hard (shares the above) |
| M1A2 Abrams | TR, FE | none | a seat; driving by the stick; the turret by two grips or a stick; cannon and coax; zoom and thermal | Hard |
| UAV feed | TR | none | the feed on a screen or as the full view; aim and lase | Medium |
| F/A-18F back seat | GH | none | the head's look sent to the game (locks follow the head); triggers fire; flares and weapon select by reaching or by buttons; the pod's view | Medium-Hard |
| M224 mortar, deployed | OG | none | the prompt on the right grip; the scene as drawn | Easy |

---

## 4. The mechanic families and building blocks

The project's rules for every weapon [mod]: each gun keeps **its own** mechanics, learned from **its
own** reload in the game (vanilla first, never the M416 copied); **every interaction works by either
hand**; every attachment works too; a mission's attachments are never changed. So the framework is a
set of blocks, each filled per gun from that gun's data and its recorded reload. The hand-tuned
profiles that exist (the M416's, the nine held guns', the launchers') are the first instances and the
check that what the framework learns matches what was approved in the headset.

### 4.1 Foundations every weapon needs

1. **Hand roles, not "the left hand"** [mod]: a trigger hand (on the pistol grip), a support hand (on
   the handguard, foregrip or front grip), a work hand (the one doing the reload step) and a free hand;
   zones per side; the right hand's poses are the approved left ones mirrored, never fitted again.
   Built for the M416 only: its free hands are committed (`4b5d2b6`) and approved, and every M416
   reload step by either hand is in the working copy, not yet tried in the headset.
2. **Hold rules** [mod; built for the M416]: the trigger hand's grip button holds the gun; a hand on
   the handguard alone keeps it up and aimed (nothing fires); let go of both and it falls, with the
   hand's speed and spin if thrown, lands with the game's own weapon-drop sound and lies as long as you
   leave it; a fresh squeeze within 25 cm picks it up by its pistol grip, and that hand becomes the
   trigger hand. For every other gun, the grip anchors come from the game's own palms on it (see the
   palm table in 4.10) and the collision hull from its own mesh (`build/guns/*.bin` holds 13 shapes:
   the ten drawn rifles and machine guns, the Stinger, and two for the SMAW; none for the M9).
3. **The game's own pickups taken by hand**: the dead soldiers' guns of section 1.7 and the placed
   pickups (2.5 m, the game's "hold R"). Today X held runs the game's pickup while the rifle lies
   (not tried in the headset) [mod].
4. **The arms of each level** [data: which arm mesh each level loads]:
   - gloved Marine arms (`arms1p_bareglove03`): SB, UP, OG, TR, FE, NS, RH and Blackburn's part of KA,
     with two variations of the same mesh: Miller's (`_miller`, TR and FE) and Uprising's
     (`_earthquake`). Measured on RH [mod]; that the other levels draw them the same way is likely
     but unchecked [inferred].
   - Dima's Russian arms (`arms1p_rus_mesh`): CO and KA (the variation `arms1p_rus_dima` in KA only).
     Not lined up [mod]. Every gun of those two missions waits on them.
   - Blackburn's bare, cuffed arms (`arms1p_black_bare_cuff_mesh`, plus a handcuff object): SF and GD.
     Lined up for the train pistol only [mod].
   - Hawkins's pilot arms (`arms1p_femalepilot_mesh`): GH, where no hand-held gun is used.
   - Nine of the twelve levels had never been opened in VR as of 2026-11-05 [mod].
5. **Body zones** (agreed 2026-11-05) [mod]: the held gun's ammo from the belly (built for every
   built gun); grenades from the left chest; C4 and claymores from the right chest (no campaign kit
   has C4 [data], so there it is the claymore alone); the primary's holster on the right shoulder and
   the secondary's on the left (holsters not built). No place agreed yet for launchers, the knife, a
   lone pistol, or a third gun.
6. **Loose physics** [mod]: magazines, the launchers' spent cases and the casings fall as rigid
   bodies onto the game's own ground (found by the game's own ray cast), lie 30 s and then go only
   while unseen; dropped guns lie with no limit. No shotgun shell exists yet. A loose object is drawn
   only inside its own gun's draw today, so it vanishes while another gun is out: **hosting** is
   still needed.
7. **Recoil**: a foundation, not built [mod]. Each gun's per-shot kick is in its data (gun sway)
   [data]; the launchers' recoil numbers are all zero and their kick lives in their first-person Fire
   clip [data]. Project rule: some recoil with two hands, rifles very unsteady in one hand, pistols
   lenient.
8. **Sounds**: every clip cut from the gun's own reload wave in the game's files, never recorded off
   the speakers [mod]. The cut times found for each gun are in 4.10 [data].
9. **Spread**: unzoomed shots in BF3 get extra spread (2.5 degrees for rifles, 2 for 40 mm and
   rockets; 3-7 degrees for the sniper rifles) [data]. The mod copies the zoomed spread over the
   unzoomed for the guns it draws [mod]; that data is shared with enemies carrying the same gun, so
   they get more accurate too (a decision owed). The AT4 has its own spread data [data].
10. **Fire mode**: a button, to be chosen when testing on Quest [mod]; no gun model has a selector
   that moves [data]. BF3's modes: no gun has a safe; burst on the AN-94 (2, default), AEK-971 (3),
   KH2002 (3, default, no auto), G36C (2), SG 553 (3), UMP-45 and PDW-R [data].
11. **Dry click, chamber check, rounds kept**: an empty gun clicks (its own `trigger_click` sound); the
   handle eased back shows the chamber; a magazine keeps its own rounds (all agreed [mod]; not built).

### 4.2 Magazine and feed families

| Family | How it works | Guns | Status |
| --- | --- | --- | --- |
| **A. STANAG, button, drops free** | Straight in and out; the trigger hand's button (X today) or a pull by hand; nothing knocks it out | M416, M16A4, M4A1 (built); M27 IAR, Mk11 | Built block |
| **B. Bullpup, magazine behind the grip** | The support hand reaches back past the trigger hand | KH2002 (button, built); PDW-R (button); F2000 (release in the grip, **not** drop-free: pulled by hand) | Built block, new reach for either hand |
| **C. Other straight boxes** | Button or latch, straight down | SCAR-H (ambidextrous button), USAS-12 (button on the right; separate magazine mesh), M98B (latch ahead of the trigger guard), M40A5 (paddle at the front of the trigger guard, no button), SV-98 (release not found) | Built block (straight pull, magnet), new numbers |
| **D. Rocked in, paddle or latch behind** | No button: the casual pull rocks it forward off its front hook; a fresh magazine can knock the old one out (the "Afghan flick", above 1.3 m/s); in hook first, rocked home; magnet 10 cm, 0.15 s | AK-74M, AN-94, AEK-971, RPK-74M (built); AKS-74u, AS Val, SG 553, Saiga 12K (separate mesh), SVD, M39 EMR (M14 latch), M82A3; straight HK boxes rocked a little: G3A3 (10 degrees, built), UMP-45, G36C | Built block |
| **E. Magazine in the pistol grip** | Up into the trigger hand's own grip from below; magnetic from any direction (7.5 cm, 55 degrees), a glide and a palm push | M9 (built as a rehearsal), MP-443, MP7, PP-2000 | Started (pistol), new for long guns |
| **F. Top horizontal magazine** | Front lugs act as a hinge, rear catch latches; lift the rear off, set the front, press the rear down | P90 | New |
| **G. Tube, shell by shell** | Each shell pushed up through the loading port under the receiver until it clicks; an empty gun is loaded through the ejection port | 870 MCS, M1014 | New (close to the M320's round-on-a-line insertion) |
| **H. Drum through a loading gate** | Wind the drum, then shells one by one through the gate at the right rear | DAO-12 | New |
| **I. Belt from a box under a feed cover** | Cover up, box off and on, belt pinched and laid in the tray, cover shut; the belt drawn by the true count | M240B (built); M249 (built offline 2026-11-18: its box a draw of its own, off and on along its own reload's way: 8 cm to the left, then down; m249.md); M60E4, Type 88 (feed from the left); PKP (built offline 2026-11-25: fed from the right, mirrored; its box the body's bone 0, off and on along its own reload's way to the right, tipping as it goes; its belt numbered from the box; pkp.md) | Built block |
| **J. One round into a breech** | Release, breech swings out, case out, round in along the bore, shut | M320 (built) | Built block |
| **K. Rockets and missiles** | A sealed case or tube on or under the launcher | SMAW (case on the back, twist lock, built); Stinger (aerial flap, built); Javelin (tube under the CLU); AT4 (the whole launcher thrown away) | Two built, two new |

Magazine variants that come with their own reload animation and mesh [data]: MP7 (20/40), PP-2000
(20/40), PDW-R (20/30), USAS-12 (7/11); and with a bigger count only: M249 and Type 88 (100/200),
M1014 (5/7, a longer tube drawn), DAO-12 (8/12). So a magazine profile belongs to the magazine mesh,
not to the gun [inferred].

### 4.3 Charging and bolt families

| Family | How it works | Guns | Status |
| --- | --- | --- | --- |
| **a. AR rear T-handle + bolt catch** | The T-handle stays still when firing; two-finger pull, 4.5 of 7 cm chambers; the bolt locks back when empty and a touch on the catch sends it home | M416, M16A4, M4A1 (built); M27 IAR, Mk11; the MP7's T-handle | Built block |
| **b. Bolt release pressed or slapped** | What BF3's own empty reload does instead of a handle | palm slap: M416, M16A4, M27 IAR, Mk11; thumb: M4A1, SCAR-H; slap: SG 553; button under the port: M1014; the trigger hand's thumb: MP7; button at the trigger guard (real gun): G36C | Built block (the touch), new places |
| **c. AK handle on the right, moves with each shot, no hold-open** | Over or under the gun by the far hand, a direct pull by the near hand; past half its travel it chambers; a kick each shot if the game doesn't move it | AK-74M, AN-94, AEK-971, RPK-74M (built); AKS-74u, AS Val, Saiga 12K; the SG 553's handle | Built block |
| **d. The same handle, but held back when empty** | No release button: pull the handle and let go | SVD, M39 EMR (op rod), M82A3 (big handle) | New flag on block c |
| **e. HK handle forward on the left, notch and slap** | Drawn back and turned up into a notch, it stays open; slapped down, it flies home | G3A3 (built without the notch), UMP-45 | New state |
| **f. Forward side handles, straight back** | Pulled and let go; the near hand pulls, the far hand reaches over or under | SCAR-H (left, moves with each shot), F2000 (left front), PDW-R (front), P90 (both sides), USAS-12 (knob on the left of the forearm), M1014 (bolt handle on the right) | Built block (lever), new places |
| **g. Handles on top** | Taken from above | KH2002 hook (built); G36C (top rear, folds, back when empty); PP-2000 (top front, moves with the bolt) | Built block, new places |
| **h. Open-bolt belt-fed handles** | On the right, still when firing; charge after the belt ran dry | M240B (built); M249 (built offline: on the right, 12.9 cm; the game draws it with the right hand); PKP (built offline: on the right, 14.5 cm; the game's right hand); M60E4, Type 88 (side to measure) | Built block |
| **i. Turn bolt** | Lift, back, forward, down; the case flies out on the back stroke; no shot until the bolt is down | M40A5, M98B, SV-98 | New |
| **j. Pump** | The support hand's fore-end slides back and forward; no shot until a full stroke | 870 MCS | New |
| **k. Pistol slide** | Grabbed, racked past 4.0 of 4.57 cm; locks open when empty | M9 (built, a rehearsal), MP-443 | Started |
| **l. Drum winding** | A ratchet twist of the key at the drum's front | DAO-12 | New (closest: the SMAW case's twist) |

### 4.4 Bolt state and round counting [data unless marked]

- Closed-bolt guns count the magazine plus one in the chamber (30+1); a tactical reload keeps the
  chambered round. Belt-feds count 100 with no extra round (open bolt). The DAO-12 has no chamber.
  Tube shotguns count tube plus chamber as one number (4+1, 6+1), so the mod has to keep the chamber
  itself [inferred].
- **Hold-open, no shot until released:** the AR family, SCAR-H, SG 553, G36C, MP7, M1014, the
  pistols, the UMP-45 (a catch on the left holds its bolt back after the last round [web]; the
  game's empty reload locks the handle in its notch instead, see 4.10.3), and the SVD, M39 EMR and
  M82A3 (released by their handle).
- **No hold-open, rack after every empty magazine:** the AK family (AK-74M, AN-94, AEK-971, RPK,
  AKS-74u, AS Val, Saiga), the KH2002 (its empty reload draws the hook), the G3A3 (its handle locked
  back in the notch by hand first), F2000, P90, PP-2000, PDW-R, the bolt guns, and the belt-feds once
  the belt has run out. The 870 is pumped, the DAO-12 has no chamber, and the USAS-12 is still to
  check (the sheets in 4.10 give the sources).
- The game reloads by itself (`AlwaysAutoReload`); the mod keeps one hidden round so it never does,
  which is why the ammo counter reads one high [mod].
- BF3's own empty reload never pulls the AR T-handle (M416, M16A4, M4A1, M27 IAR): it slaps or
  thumbs the bolt catch. The mod's T-handle pull uses the G36C's handle sound for that reason [mod].
- `tools/gamesounds.cpp` lists the SCAR-H and SG 553 among "the small rifles whose empty reload does
  pull a handle"; their reloads press or slap a bolt release instead [web: IMFDB captions; data: their
  waves]. Of that list's campaign guns, the G36C pulls its handle; the F2000 and M39 EMR do too.

### 4.5 Launcher blocks

- **Launcher rig** [mod]: a rigid launcher placed by the trigger hand's controller with the game's arms
  moved whole with it; built for the SMAW and the Stinger. To add: the AT4 (Swordbreaker), the Javelin
  and the standalone M320 (Guillotine).
- **Two-hand hold by side** [mod]: the support hand snaps onto the front grip and steers; built for a
  left support hand. The game's own SMAW reload loads with the **right** hand while the left keeps the
  front grip [data], so the vanilla clip already shows the other side.
- **The shot from the drawn muzzle**: every launcher, grenade and blade here spawns its shot from the
  camera plus a fixed offset (`ForceSpawnToCamera`) [data]: SMAW and AT4 (-0.3, -0.1, 2.0) m at
  40 m/s; Stinger (-0.2, -0.065, 0.35) at 16 m/s; Javelin (-0.15, -0.3, 0) at 16 m/s; M320
  (-0.1, -0.3, 0.86) at 45 m/s plus 1.5 up; M67 (0, 0, 0) at (0, 3, 18) m/s; the claymore at
  (0, -2, 1) m/s, 0.35 s into its clip; the knife an instant hit 0.3 s in. Rewriting that data each
  frame from the drawn object is built for the SMAW and the Stinger [mod]; the same block places a
  grenade at the hand with the hand's speed, a claymore where the hand set it, and the knife's hit
  along the blade [inferred].
- **Spread**: the SMAW, Stinger, Javelin and M320 share one sway entry (`40mm_GunSway`), already
  cleared [mod]; the AT4 has its own (1-5 degrees unzoomed) [data].
- **Lock-on without the game's zoom**: the Stinger locks by heat (2 s, 5 degrees) and the Javelin by
  "radar" (2.5 s, 5 degrees), both only zoomed in the data, and both fire only when locked [data].
  Built for the Stinger (the zoomed lock type copied into the unzoomed slot) [mod]. Project design:
  lock only with the tube at the eye; no lock marker; the game's tone kept.
- **Reload parts**: a case with a twist lock (SMAW, built); a keyed flap (the Stinger's aerial,
  built); a hinged breech with a case that tips out (M320, built); a whole launcher thrown away with
  priming presses (AT4, new); a tube swapped under a carried unit (Javelin, new, after recording it).
- **The mod's own round count**: a hidden round stops the game's auto-reload (built for the SMAW,
  the M320 and the Stinger, as for every gun with its magazine by hand) and the mod's count gates the
  trigger (the SMAW and the M320); the counter reads one high [mod].

### 4.6 Throwables, placeables and blades

- **Throwable** (M67) [mod: designed and approved, not built]: held by the grip; the other hand's
  index in the ring with that hand's trigger and a yank pulls the pin (pin and spoon fall); tossed and
  caught between hands as a loose body; let go mid-swing, it flies with the controller's speed and
  spin. The game's own grenade is then spawned there; its fuse (3.6 s after the throw, no cooking) and
  blast stay the game's [data]. The grenade action (G) has been unbound since 2026-10-31 [mod]. Three
  grenades per kit in the campaign [web].
- **Placeable** (claymore): set on the ground by hand, facing the enemy's path; its legs unfold; the
  game's mine is spawned there. At most 2 lie at once; each arms after 3 s and fires at a soldier
  within 1.5 m in a 60-degree cone moving faster than 3.8 m/s [data]. Whether the placed mine faces
  the shot's direction or the soldier's heading is untested [inferred].
- **Blade** (knife, machete): one-handed in either hand; a stab or slash read from the controller's
  motion sends the game's melee, aimed along the blade [inferred]. Near an enemy's back or side (any
  side when he is prone), the game's own takedown plays: 12 first-person takedown clips, four per
  stance [data], to show as a locked scene with the game's body. The knife breaks fences and glass
  [web].

### 4.7 Attachment blocks

- **One measurement per attachment mesh**, reused on every gun: sights, lights, lasers, rails and
  grips are shared meshes on a socket bone [data]; the mod's held-sight table already works this way
  for five sights [mod].
- **Sights** [mod: built for five sights plus the M416's ACOG]: the lens on the game's own glass,
  marks baked from the zoomed reticle mesh, a window cut into closed tubes, and the second renderer's
  real picture above 1.5x, per eye. Each sight's power comes from its zoom field of view against the
  unzoomed 55 degrees [data]: irons, Kobra, RX01 and IRNV 40 degrees (1.43x); EoTech and PK-AS 32
  (1.82x); PK-A and M145 20 (2.95x); ACOG and PSO-1 17.2 (3.44x); the SCAR-H's rifle scope 11.6
  (5.1x); PKS-07 9.9 (6.0x); the Mk11's rifle scope 8.7 (6.8x); the thermal sniper scopes 7 (8.5x);
  the ballistic scope 5.8 (10.3x). New work: the thermal or night picture inside the glass alone (the
  game turns the whole screen thermal when it zooms, which the mod never does) [data + inferred]; the
  second renderer switched on by need, not by power (the IRNV is 1.43x); the power looked up per gun
  when one mesh has two powers; sights that ride a moving feed cover.
- **Marks scale**: the mod bakes marks for a zoomed first-person field of view of 15, but the M40A5's
  night scope uses 8, the M82A3's 9, the SVD's PSO-1 15.8, and the AN-94's Kobra 18 [data]. Worth
  checking that the Kobra's marks were baked at the right scale [inferred].
- **Foregrip**: the game switches the gun to its foregrip animation set, so its own left hand moves
  onto the grip [data]; the support anchor moves with it [inferred]. The PKP's foregrip has no
  first-person mesh [data].
- **Bipod**: legs folded and unfolded by hand; feet resting on a surface found by the game's own ray
  cast; the game's limits while deployed are 45 degrees each side, 30 down and 15 up [data]; its
  deploy and undeploy sounds are in every LMG's sound patch [data]. The game deploys it by itself
  when you zoom prone or at a ledge [web]; whether the mod can set that state is unknown.
- **Light and laser**: one game action toggles both (the PC default key is T) [data]; the light is a
  real spot light (20-degree cone, 30 m) and the laser a red dot plus a beam mesh [data]. Both sit
  on the game's own first-person gun near the head, so they must be moved to the drawn gun, as the
  muzzle flash was [inferred]. The switch: a touch on the device or a button (not decided) [mod].
- **Barrel parts**: suppressors and flash suppressors move the muzzle point forward (for example to
  1.33 m on the M40A5) and change the flash; heavy barrels change only the bullet [data]. The muzzle
  flash, the bullet's start and the collision hull follow [inferred].
- **Underbarrel**: a linked slot with its own state, switched to for the moment of the shot (built
  for the M320) [mod]. No GP-30 or M26 MASS is given in the campaign [data].

### 4.8 Mounted and vehicle blocks [inferred unless marked]

- **Seat state**: the vehicle, its seat, its controls (tank, gunner or jet), whether your soldier is
  drawn, its camera; the body locked to the hull or turret ring, the head free, the horizon level.
- **Pivot gun** (technical, M2HB): both hands on the spade grips, either hand alone also aims; the gun
  turns about its pintle with the hands; the game's turret is driven toward that line; the game's own
  arms stay on the grips; triggers or thumbs fire. The game's numbers [data]: 550 rounds a minute,
  unlimited ammo, no reload, an overheat after about 5 s of fire.
- **Vehicle controls** (tank): drive by the left stick, the turret by two grips or one stick, the
  cannon (3 s reload) and the coax (overheats after about 4 s) [data], zoom and thermal at the sight.
- **Jet back seat**: the head's look sent to the game, so locks follow the head; the cannon overheats
  in about 1.7 s; Sidewinder lock 1.5 s; flares one at a time every 3 s [data].
- **Scripted mounts** (the mortar, the AT4 hand-over, the MICLIC detonator): the prompt and the game's
  own body, per eye.

### 4.9 Scripted moments

- Project rules [mod]: in scenes, the game's own rendered body plays per eye, with no free hands; every
  prompt goes on the right grip.
- Scripted weapon moments [data + web]: the M9 taken in Semper Fidelis; the train-door M1014 (a
  scripted first-person pickup, `PUS_player`) and the AKS-74u QTE; Uprising's stab prompt and the rat;
  the AT4 hand-over; the mortar set-up; the toilet and atrium knife kills; the rappel with a
  flashbang; the machete's wire cut; the metro fight; the MICLIC detonator; the sentry kill and knife
  fight in Night Shift; the M82A3 after the pool-house fist fight; the SUV shot with the MP-443; the
  Ka-60 fight; The Great Destroyer's escape QTE (a table leg), the police-car chase with the M9, and
  the final fight (the cuffs, then a brick).
- The mechanics survey's item 26 (holding the detonator and the jammer, placing claymores by hand)
  was a yes, which clashes with "no free hands in scenes" for the MICLIC detonator and the jammer: a
  decision owed [mod].

### 4.10 Gun sheets, family by family

What each gun needs when its mission comes: how the real one works [web], BF3's numbers and its own
reload step by step [data], what is known of its draws and bones, and what VR needs [inferred].
Reload step times come from the gun's own reload sound waves (the onset of each sound, decoded from
the game's files) or from the mod's recordings of the game's animation where they exist; what each
sound **is** was read from its order, the screenshots and the recorded twins [inferred]. "Tactical"
is the reload with rounds left, "empty" the one from empty.

Where the game's own hands rest on each measured gun [data, recorded; position on the gun's frame
bone, x to the left, y up, z to the muzzle, metres; the other hand's anchor is the same with x
negated]:

| Gun | Right palm (grip) | Left palm (handguard) |
| --- | --- | --- |
| M416 | (-0.063 -0.091 0.224) | (handguard line) |
| M16A4 | (-0.054 -0.123 0.256) | (0.014 -0.041 0.636) |
| M4A1 | (-0.054 -0.114 0.255) | (0.012 -0.041 0.612) |
| AK-74M | (-0.032 -0.080 0.193) | (0.001 -0.031 0.548) |
| AN-94 | (-0.023 -0.059 0.192) | (0.002 -0.018 0.561) |
| AEK-971 | (-0.024 -0.079 0.208) | (0.004 -0.031 0.551) |
| RPK-74M | (-0.032 -0.080 0.192) | (-0.001 -0.029 0.585) |
| KH2002 | (-0.025 -0.099 0.234) | (0.016 -0.059 0.447) |
| G3A3 | (-0.034 -0.082 0.241) | (0.020 -0.007 0.616) |
| M240B | (-0.023 -0.115 0.196) | (0.018 -0.065 0.678) |
| FIM-92 | (-0.046 -0.132 0.593) | (0.025 -0.146 0.993) |

BF3 holds guns lower with an optic fitted [web: IMFDB], so these may shift per sight [inferred].

#### 4.10.1 The AR family (button magazine, T-handle, bolt catch)

**M416 (HK416, the family's base).**
- Real [web]: 30-round STANAG pushed straight up, drops free; button on the right above the trigger
  guard; T-handle at the rear (latch on the left) that stays still when firing; bolt catch on the
  left; selector on the left; dust cover over the right-side port; forward assist.
- BF3 [data]: 30+1, 5 magazines, 750 rpm, auto and single (a December 2011 patch removed burst
  [data + web]); reload 1.75 s tactical, 2.5 s empty; recoil per shot 0.26 up, -0.3 to +0.1
  sideways, first shot x2.
- Its reload: tactical with the rifle straight ahead, the magazine straight down and up; empty with
  the rifle tilted, then the left palm smacks the bolt catch; the T-handle is never pulled [web:
  IMFDB; data]. Its sound is the M16's own long wave (2.905 s): magazine out 0.23-0.60 s, in
  1.29-1.51 s, catch slapped and bolt home 1.95-2.24 s [data]. It shares its animation with the M27
  IAR [web].
- Draws [mod]: 12768/0, 312/12768, depth 13080/13080; bones 0 handle, 1 trigger, 2 body, 3 dust
  cover, 4 magazine.
- Mod: everything for the left hand, approved; free hands approved; every step by either hand in the
  working copy, waiting for its try-out and six questions (see 5.5) [mod].

**M16A4.**
- Real [web]: the same controls as the M416; the real M16A4 fires single and 3-round bursts.
- BF3 [data]: the campaign's gun fires auto and single at 800 rpm (the multiplayer M16A3's modes; the
  wiki notes the French version calls it M16A4 on pick-up [web]); 30+1; reload 1.8 s / 2.37 s;
  recoil 0.26 up, -0.4 to +0.1, x2.5. Bare guns carry the carry handle with its rear sight.
- Its reload: tactical: "shoving in a new one rather quickly" [web]; empty: held at another angle,
  then "palming the bolt release" at about 1.7-2.0 s [web + data]. Waves `m16_reload_1p_long` 2.905 s
  and `_short` 2.367 s, cut as the M416's [data].
- Draws [mod]: 17013/312 (frame bone 4; T-handle 0, trigger 1, magazine 2; the port cover is part of
  the frame, 204 indices at 2949), rounds 312/0, depth 18675/18675.
- Mod: built for the left hand, with the M416's magazine hand; the slap within 7 cm of
  (0.03 -0.015 0.406) or a hand arriving from the left within 17 cm [mod].

**M4A1.**
- BF3 [data]: 30+1, 800 rpm, auto and single; reload 1.85 s / 2.48 s.
- Its reload, recorded [data: `build/survey/M4A1`]: tactical: 0-0.3 s the left hand to the magazine;
  0.23-0.45 s the release clicks and the magazine goes straight down 45 cm; 0.95-1.3 s a new one
  straight up (click 1.10-1.31 s); back on the handguard by 1.6-1.8 s. Empty: the same, then at
  1.7-2.0 s the left **thumb** presses the bolt release ("thumbing the bolt release" [web]), bolt home
  1.90-1.96 s, back by 2.33 s.
- Draws [mod]: 22194/312 (frame bone 2, port cover bone 1, depth frame bone 3), rounds 312/0, depth
  22506/22506.

**M27 IAR.**
- Real [web]: an HK416-based automatic rifle with ambidextrous controls; BF3 gives it a 45-round
  magazine.
- BF3 [data]: 45+1, 750 rpm, auto and single; reload 1.9 s / 2.5 s; its animation package
  (`M416_LMG`) has the M416's clip lengths (87 and 70 frames); IMFDB: the same reload as the HK416,
  "palming the bolt catch" [web].
- Sounds [data]: `m27iar_reload_long` (2.93 s): magazine out 0.25-0.45 s, in 1.16-1.24 s, catch and
  bolt home 1.97-2.06 s; short wave 2.37 s.
- VR: the M416's set, a hand refitted on the longer magazine, the foregrip and light of its pickup.
  Only Kaffarov loads it [data].

**Mk11 Mod 0.**
- Real [web]: an SR-25 (AR-10 family): 10 or 20-round box, button on the right, T-handle, bolt catch
  on the left, no forward assist, dust cover.
- BF3 [data]: 10+1, single, 550 m/s; reload 1.85 s / 2.4 s. The wiki: the same reload animation as
  the M4 and M16 [web]; IMFDB: "hits the bolt-release" [web].
- Sounds [data]: short wave 2.3 s (button 0.24 s, magazine away 0.35 s, in and tapped 1.34-1.41 s);
  long wave 2.9 s (out 0.22-0.30 s, in 1.21-1.28 s, catch and bolt home 1.94-2.01 s).
- VR: the M16 and M4 set; the 8x rifle scope; it is Swordbreaker's slot 1 next to the M4A1, so the
  secondary holster's gun [mod layout].

#### 4.10.2 The AK family (rocked-in magazine, paddle, right-side handle)

Shared [web + data]: the magazine hooks in at the front and rocks back until the paddle behind the
well clicks; to take it out, push the paddle and rock it forward; the handle is part of the bolt
carrier on the right, so it moves with every shot; there is no hold-open, so the handle is pulled
after every empty magazine. For a gun held in the **left** hand the right support hand is already on
the handle's side and needs a **near-side pull** (palm on the gun's right, as in the game's own empty
reload), which mirroring the left hand's over-and-under poses does not give, because the gun is not
mirrored [inferred]. A starting point: the game's own right hand drawing the M240's lever, already
measured [data].

**AK-74M.**
- Real [web]: 30-round polymer magazine; the big safety-selector lever on the right (up safe, middle
  auto, down semi); BF3's fixed stock still shows a folder's latch.
- BF3 [data]: 30+1, 650 rpm, auto and single; reload 2.1 s / 3.1 s; recoil 0.28 up, -0.3 to +0.2,
  x1.5; the lever travels 4-9 cm while firing (recorded).
- Its reload, recorded [data]: tactical: the left hand under the magazine pulls it off and brings it
  back (one magazine bone swings out up to 49 cm), **no flick**; empty: the same swap at 0.48-1.67 s,
  then the left palm on the gun's right draws the lever 10 cm (2.36-2.66 s) and lets go. Long wave
  3.633 s: off 0.43-0.60 s, last seat click 1.56-1.72 s, lever 2.28-2.47 s, bolt home 2.51-2.68 s;
  short wave 2.700 s.
- Draws [mod]: 21723/0 (frame bone 2; trigger 0, lever 1, magazine 3), magazine part 894/22035,
  frame part 66/22929, rounds 312/21723, depth 22995/22995.

**AN-94 (the baseline for most guns).**
- Real [web]: AK-74 magazines canted to the right (BF3 draws them straight); a cross-bolt safety at
  the trigger guard and a separate selector at the left rear; a 2-round burst at about 1800 rpm; the
  barrel and receiver recoil inside the housing; the stock folds to the right.
- BF3 [data]: 2-round burst (default), auto and single, 600 rpm; 30+1; reload 2.45 s / 3.3 s; recoil
  0.30 up, -0.3 to +0.2, x1.5; the barrel recoils 10.6 cm when firing and the lever goes 5 cm forward
  with it (recorded).
- Its reload, recorded [data]: both reloads are the flick: the seated magazine hides and the game's
  spare copy (parked 1.086 m behind the gun) takes its place; about 0.7 s a fresh one shows in the left
  hand; at about 1.2 s it swings into the old one, which flies off forward at about 3.4 m/s; the fresh
  one rocks home by 1.9 s; the hand is back by 2.5 s. Empty: then over the top to the lever, drawn
  11.3 cm in about 0.4 s. Long wave 3.9 s: strike 1.00-1.23 s, seat 1.74-1.92 s, lever 2.53-2.76 s,
  bolt home 2.81-3.00 s.
- Draws [mod]: 16188/0 (frame bone 0; lever 2, barrel 3, magazine 4, the spare copy 5), rounds
  312/16278, depth 16590/16590; lever knob at (-0.034 0.068 0.495), travel 11.3 cm.
- Playtest, 2026-11-05 [mod]: "Keep this as a baseline for like most of the guns."

**AEK-971.**
- Real [web]: a balanced action (a counter-mass cancels the recoil); AK-74 magazines; the production
  rifles fire 3-round bursts; BF3's model mixes an early prototype with a later upper.
- BF3 [data]: 30+1, 900 rpm, auto, single and 3-round burst; reload 2.55 s / 3.68 s; recoil 0.20 up,
  -0.3 to +0.5, x3.
- Its reload, recorded [data + web]: "it always reloads in a tactical manner": the fresh magazine
  knocks the old one out at about 1.34 s (it flies about 3 m), home by 2.06 s; empty: then the left
  hand over the top draws the lever only 5.6 cm (2.86-2.95 s) and lets go (3.19-3.23 s). Long wave
  4.2 s: strike 1.25-1.44 s, seat 1.99-2.13 s, lever 2.75-2.97 s, bolt home 3.02-3.17 s.
- Draws [mod]: 21585/0 (frame bone 4; magazine 0, trigger 1, lever 2, spare copy 3), rounds
  318/21615, depth 21933/21933; lever travel 9.2 cm.
- Kaffarov's kit turns the laser and the light on and off together [web + data].

**RPK-74M.**
- Real [web]: the AK pattern with a 45-round magazine, a bipod, a side-folding stock.
- BF3 [data]: 45+1, 700 rpm, auto and single; reload 3.0 s / 4.0 s (clips of 108 and 134 frames).
  BF3's lever does **not** move when firing (the wiki calls it an oversight on every machine gun
  [web]); the mod kicks it back 8 cm per shot [mod].
- Its reload [data recorded + web]: tactical: magazine pulled off and a fresh one brought in, no
  flick; empty: the left hand reaches **under** the gun for the handle while the right keeps it up
  ("charging the RPK underhanded style"). Cuts in its long wave: magazine out 0.58-0.80 s, in
  2.335-2.50 s, lever 3.265-3.46 s, bolt home 3.48-3.69 s.
- Draws [mod]: 24492/0 (frame bone 3; its bone 0 is the lever), rounds 312/24492, part 36/24804,
  depth 24840/24840; bipod 2952/0.

**AKS-74u.**
- Real [web]: an AK-74 magazine (BF3 draws a taped orange one with visible rounds); a side-folding
  skeleton stock (never folded in BF3).
- BF3 [data]: 30+1, 650 rpm, auto and single; reload 1.9 s / 3.0 s. Recorded 2026-11-18 on Semper
  Fidelis (`build/survey/AKS74u_*`); its measurements and what was built from them: [aks74u.md](aks74u.md).
- Its reload [data: its waves, 2.367 s and 3.367 s; meaning inferred]: pull-out-then-insert like the
  AK-74M, not a flick. Tactical: out about 0.3-0.5 s, home 1.27-1.53 s, back by about 2.0 s. Empty:
  out 0.36-0.42 s, home 1.51-1.57 s, then the left hand over the top to the right-side handle
  ("palming the bolt back" [web]), drawn 2.12-2.24 s, let go 2.38-2.44 s, done at 3.0 s. The wiki:
  it shares its handle animation with the AK-74M, AS Val, SVD and M39 EMR [web].
- VR: the AK-74M's set; Uprising's suppressor (muzzle at 0.853 m, a 290 m/s bullet [data]) and light;
  the arms of three levels.

**AS Val.**
- Real [web]: 9x39 mm, a built-in suppressor, an AK-derived action, a 20-round magazine with a
  paddle behind the well, the handle on the right; an AK-style safety lever, with the fire selector a
  separate control behind the trigger; a folding skeleton stock.
- BF3 [data]: 20+1, 900 rpm, auto and single; reload 1.7 s / 2.85 s; the magazine is part of the main
  mesh; it has its own Fire clip (the action visibly cycles); its muzzle effect sits at 0.86 m, far
  ahead of the other guns' 0.52-0.60 m.
- Its reload [data: waves 2.10 s and 3.37 s]: tactical: out 0.30-0.41 s, seated about 1.27 s, back
  by 1.4-1.7 s. Empty: out 0.33-0.45 s, seated about 1.47 s, then over the top to the handle
  ("Charging the Val" [web]): pull, release and bolt home at 2.11-2.29 s, done at 2.85 s. Its magazine
  sounds are the AKS-74u's own recordings (correlation 0.84-0.92) [data].

**Saiga 12K (the data's "Saiga 20K").**
- Real [web]: a Kalashnikov-pattern 12-gauge; the 12K has the short barrel and a side-folding stock;
  rocked-in magazine with a paddle; handle on the right; no last-round hold-open on standard guns.
- BF3 [data]: 12-gauge (the "20K" name is left over from Bad Company's S20K [web]); 6+1 (10+1
  extended); single at 220 rpm; reload 2.2 s / 3.5 s; buckshot (9 pellets), flechette, frag, slug.
- Its reload [data: waves 2.77 s and 4.1 s; web]: tactical: the paddle and the magazine rocked out at
  about 0.42 s, a full one hooked at about 1.13 s and rocked home at about 1.80 s. Empty: the same
  swap (out 0.48-0.56 s, in about 1.88 s), then over the top to the handle ("charging the Saiga
  overhand style"), pulled at about 2.75 s, let fly at about 2.99 s. IMFDB notes the magazine vanishes
  for a frame after each shot.
- Draws [data, read offline]: body 14970/0 (bone 0 the carrier and handle, 1 trigger, 2 frame), depth
  14970/14970. **The magazine is a separate mesh**: 1176/0, shells 450/1176, depth 1626/1626;
  extended 1218/0. The magazine code must find that draw.
- Open: whether the AK rules (by hand only, the flick) apply to it; it is not on the project's list
  of paddle guns [mod].

**SG 553.**
- Real [web]: IMFDB reads the markings of an SG 553-1 SP with an SG 552's handle; SIG 550-type
  magazines with studs to clip them together; a paddle behind the well [inferred]; the handle on the
  right of the carrier (moves); the bolt locks back when empty and is released by **lifting** the
  catch on the left; the stock folds to the right.
- BF3 [data]: 30+1, 700 rpm, auto, 3-round burst and single; reload 2.1 s / 2.65 s.
- Its reload [data: waves 2.70 s and 3.13 s; web]: tactical: out 0.41-0.47 s, rocked home 1.30-1.61 s,
  back by about 2.1 s. Empty: the same until 1.77 s, then at 2.12-2.24 s the left hand gives the bolt
  catch "a SIG-slap" and the bolt slams home. The right-side handle is not used.
- VR: the AK magazine, the M4's bolt-catch touch moved to its catch (a slap, or a push up as on the
  real gun), and the handle as a second way to chamber (a decision).

**SVD.**
- Real [web]: BF3's is an NDM-86, the .308 Chinese Dragunov, with a straight magazine; 10 rounds
  rocked in with a paddle; the handle on the right; after the last round the bolt is held back and
  released by pulling the handle; the PSO-1 on a rail on the left.
- BF3 [data]: 10+1, single, 530 m/s; reload 2.15 s / 3.3 s.
- Its reload [data: waves 3.0 s and 3.9 s; web]: tactical: paddle and magazine rocked out
  0.52-0.62 s, the new one hooked 1.45-1.61 s, home 1.78-1.85 s. Empty: the same until 1.76 s, then
  over the top to the handle, drawn 2.39-2.67 s, let go 2.84-2.91 s ("it properly locks back upon
  empty" [web]).

**M39 EMR.**
- Real [web]: BF3's is a Springfield M1A in a Sage EBR chassis: the M14 action; the magazine's front
  goes into the front catch and is rocked back until the latch behind it locks; the operating-rod
  handle on the right moves with each shot; the bolt is held back when empty; the safety is in the
  front of the trigger guard.
- BF3 [data]: 10+1, single, 570 m/s; reload 2.15 s / 3.4 s.
- Its reload [data: waves 2.8 s and 3.9 s]: tactical: rocked forward and out 0.60-0.78 s, the new one
  brought up 1.10-1.42 s, its front caught 1.52 s, rocked and latched 1.60-1.75 s. Empty: the same
  until 1.76 s, then the hand to the op rod 2.29-2.39 s, drawn 2.51-2.66 s, let go 2.87-2.94 s.

**M82A3.**
- Real [web]: BF3's model is a Barrett M107 with a thermal sight; short-recoil .50; a 10-round box
  rocked in front first; the catch at the back of the well and a big handle on the right [inferred;
  one source says left]; the bolt locks back when empty [web, secondary].
- BF3 [data]: 10+1, single, 560 m/s; reload 4.5 s / 6.0 s, the campaign's longest.
- Its reload [data: waves 4.9 s and 6.23 s]: tactical: catch and heavy magazine out 1.12-1.26 s, the
  new one brought up 1.55-2.28 s, hooked and rocked 2.85-3.02 s, latched 3.14-3.21 s, done at about
  4.35 s. Empty: out 1.00-1.28 s, in 2.57-2.71 s, latched 2.89 s, the hand to the handle 3.26-4.25 s,
  drawn 4.46-4.83 s, let go 5.06-5.26 s.
- VR: two hands to steady it; the thermal scope; the biggest kick in the campaign; the handle's side
  and whether it moves to be measured.

#### 4.10.3 HK handles and other handle variants

**G3A3.**
- Real [web + inferred]: a 20-round straight box; a button on the right of the well and, on military
  lowers, a paddle behind it; the cocking handle on the left of the tube over the handguard, still
  when firing: pulled back and turned up into a notch it holds the bolt open, slapped down out of the
  notch it chambers a round (the "HK slap"); no last-round hold-open; S-E-F selector on the left.
- BF3 [data]: 20+1, 550 rpm, auto and single; reload 2.0 s / 3.3 s; recoil 0.45 up (the strongest of
  the rifles), -0.2 to +0.2, x1.4.
- Its reload, recorded [data]: tactical: the magazine down and away and a new one in, the rifle held
  steady. Empty: first the left hand goes far forward on the left (palm at (0.065 0.054 0.749) at
  0.47 s), the handle swings out, is drawn back (0.45-0.55 s) and turned up into the notch (0.77 s),
  where it stays; then the magazine change (1.38-2.59 s); last the handle is slapped down
  (3.08-3.17 s). Full wave 3.967 s: handle 0.45-0.62 s, out 1.36-1.52 s, in 2.44-2.60 s, the slap
  3.03-3.32 s (the loudest sound); quick wave 2.533 s.
- Draws [mod]: 23064/0 (frame bone 0; magazine 1, trigger 2, handle 3), depth 23064/23064; the handle
  swings out 53 degrees, then comes back 17 cm.
- Mod: built for the left hand without the notch: the handle springs home [mod]. The notch is the
  first thing to add, and the UMP-45 needs the same block.

**UMP-45.**
- Real [web]: .45 ACP blowback, a 25-round box; an ambidextrous paddle behind the well, no button; the
  cocking lever far forward on the left, still when firing; pulled back and turned up it locks into an
  indent; the bolt locks open after the last round, released by a catch on the left; a side-folding
  stock.
- BF3 [data]: 25+1, 600 rpm, auto, single and burst; reload 1.85 s / 2.7 s; the magazine is part of
  the main mesh. Recorded 2026-11-18 on Semper Fidelis (`build/survey/UMP45_*`): the empty reload is
  the handle back 8.3 cm and 51.8 degrees up into its notch, the magazine straight down and a fresh
  one up, the handle knocked down; measurements and what was built: [ump45.md](ump45.md).
- Its reload [data: waves 2.30 s and 3.33 s; web]: tactical: the paddle and the magazine out at about
  0.28-0.40 s (it "briefly clips the magwell"), a new one seated at about 1.34 s. Empty: "the charging
  handle is locked back similar to the roller delayed HKs" and the reload ends "backhanding the bolt
  back into battery" [web]; its wave follows the G3A3's recorded empty reload step for step: the lever
  back and up into its notch at about 0.36-0.54 s, magazine out 0.90-1.01 s, in about 1.91 s, the
  backhand at 2.27-2.35 s [inferred mapping].
- VR: the G3A3's handle with the notch as a state (locked back and up when empty) and the HK slap or
  backhand as the gesture that frees it; the magazine by hand.

**G36C.**
- Real [web]: a 30-round polymer magazine with lugs; an ambidextrous paddle behind the well; the
  charging handle on top at the rear under the rail, folding, usable from either side, riding with the
  carrier (so it stays back when the bolt locks open); a bolt-catch button at the front of the trigger
  guard; a side-folding stock.
- BF3 [data]: 30+1, 750 rpm, auto, 2-round burst and single; reload 1.9 s / 2.85 s.
- Its reload [data: waves 2.552 s and 3.200 s; web]: tactical: paddle and magazine out 0.21-0.52 s,
  home 1.29-1.49 s, back by about 2.0 s. Empty: "the charging handle locks back when empty"; the same
  swap about 0.11 s later; then the left hand up to the handle at the top rear, taken and drawn
  2.37-2.44 s, let go 2.55-2.63 s. Its 2.365-2.585 s cut is the handle sound the mod borrows for the
  M4 family's T-handle [mod].
- VR: the magazine by hand with a small rock; the handle taken from above by either hand; when empty
  it is already back, so a short pull and let go sends the bolt home; the trigger-guard button as an
  option (a decision).

**SCAR-H.**
- Real [web]: the first-generation Mk 17 (CQC): a 20-round box, an ambidextrous release; the charging
  handle forward on the **left** (it can be moved to the right) and moving with every shot; the bolt
  locks back, with a paddle on the left; an ambidextrous selector (BF3's model always reads SEMI); a
  side-folding stock.
- BF3 [data]: 20+1, 600 rpm, auto and single; reload 2.1 s / 2.75 s.
- Its reload [data: waves 2.50 s and 3.167 s; web]: tactical: button and magazine out 0.27-0.41 s,
  the new one pulled up with the gun rolled to show its left side, home 1.52-1.64 s. Empty: the same
  swap, then at about 2.05-2.25 s the left thumb presses the bolt release (in the animation the thumb
  doesn't quite reach it [web]) and the bolt slams home; the left handle isn't touched.
- VR: the M4 family's button magazine and release touch, placed on its gun, plus a left-side handle
  that moves with each shot (the lever block on the left: the near hand pulls, the far hand reaches
  over or under).

#### 4.10.4 Bullpups, side handles and the P90

**KH2002.**
- Real [web]: an Iranian bullpup descended from an M16 copy: a STANAG behind the grip with an
  M16-type button on the right; the charging handle on top under the carry handle; the selector at
  the rear left; the port on the right by the cheek.
- BF3 [data]: 30+1 (the model's magazine is a 20-rounder [web]), 800 rpm, 3-round burst (default) and
  single, no auto; reload 2.9 s / 3.4 s; recoil 0.20 up, -0.4 to +0.4, x1.5.
- Its reload, recorded [data]: tactical: the left hand goes back behind the pistol grip, takes the
  magazine out down and back (its bone travels up to 34 cm) and puts a fresh one in. Empty: then over
  the top to the hook under the carry handle, drawn 6.6 cm with one finger (3.30-3.58 s:
  "finger-gunning the charging handle" [web]). Long wave 4.167 s: out 0.68-0.89 s, in 2.50-2.60 s,
  hook 3.21-3.39 s, let go 3.47-3.63 s.
- Draws [mod]: 15237/0 (frame bone 3; trigger 0, magazine 1, hook 2), rounds 312/15237, depth
  15549/15549. Cases come out by the cheek [data].
- Mod: the hook taken by the index finger over its fin; playtest: "that's wonderful" [mod].

**F2000.**
- Real [web]: the F2000 Tactical: a STANAG behind the grip; the release is in the grip ahead of the
  magazine, and the magazine is **not** drop-free ("needs to be pulled out manually"); the charging
  handle on the left, well forward above the handguard; no last-round hold-open; a selector disc
  under the trigger; cases go forward through a tube to a port near the muzzle on the right.
- BF3 [data]: 30+1, 850 rpm, auto and single; reload 2.5 s / 3.5 s; recoil 0.26 up, -0.4 to +0.5,
  x3; cases out at (-0.01 0.055 0.56). BF3's model has no underbarrel rail [web].
- Its reload [data: waves 3.167 s and 4.1 s; web]: the character takes "a quick glance at the loaded
  rounds" [web]. Tactical: release and magazine out 0.41-0.74 s, a rattle (the glance) 1.0-1.25 s,
  in 1.51-1.94 s, hand back 2.49-2.64 s. Empty: the same until 2.01 s, then five sharp sounds at
  2.74-3.07 s (the handle drawn and let go), hand back 3.52 s. Which hand works the handle, and how
  far it travels, is unknown until recorded.
- VR: the KH2002's reach behind the grip, pulled by hand (no button drop); the handle forward on the
  left (no notch); racked after every empty magazine.

**PDW-R (Magpul PDR).**
- Real [web]: a bullpup prototype taking STANAGs behind the grip, with an M16-style ambidextrous
  button and an ambidextrous charging handle; BF3's is yanked on the left near the front.
- BF3 [data]: 20+1 (30+1 with the extended magazine, a separate mesh and its own reload set), 750 rpm,
  auto, single and burst; reload 2.5 s / 3.4 s; firing clips that move something as it fires (to
  record).
- Its reload [data: waves 3.13 s and 4.17 s; web]: tactical: "thumbing out the magazine" at
  0.35-0.45 s, a fresh one up behind the grip, seated at about 1.8-1.9 s. Empty: then the left hand
  yanks the handle and lets go (2.63-2.82 s).

**P90.**
- Real [web + inferred]: a 50-round translucent magazine lying on top, its rounds turned 90 degrees on
  a ramp; the catches at its rear; cocking handles on both sides at the front, still when firing; no
  hold-open; a selector dial at the trigger; cases drop out of a chute behind the grip.
- BF3 [data]: 50+1, 900 rpm, auto and single; reload 2.5 s / 3.0 s; the magazine is part of the main
  mesh; cases eject downward.
- Its reload [data: waves 2.83 s and 3.17 s; web]: tactical: the left hand at the top rear unlatches
  it (a loud click at 0.49 s), lifts it off (0.59-0.70 s), a fresh one is laid on and pressed home
  (1.79-1.93 s). Empty: then the left hand pulls the **left** front handle and lets go (2.31-2.44 s).
- VR: a new magazine block (front lugs as a hinge, a latch at the rear); each hand uses the handle on
  its own side.

**USAS-12.**
- Real [web]: a gas-operated 12-gauge with a 10-round box or 20-round drum; a button on the right;
  the cocking handle runs in slots in the forearm and can go on either side; a carry handle with the
  rear sight (swapped for a rail with optics); 5.5 kg.
- BF3 [data]: 6+1 (10+1 extended), 275 rpm, auto and single; reload 2.5 s / 3.45 s; full-auto firing
  clips; the cocking knob sticks 5.4 cm out of the **left** of the forearm.
- Its reload [data: waves 2.9 s and 4.1 s; web]: tactical: button, out 0.48-0.55 s, seated about 1.84 s
  (the loudest click). Empty: the same, then the left fist on the knob, pulled 2.48-2.64 s, the bolt
  home at about 3.00 s ("pulls on the charging handle" [web]).
- Draws [data, read offline]: body 14136/0 (bone 0 the knob, 1 a thin part at the top rear (unknown),
  2 trigger, 3 frame), depth 14136/14136; **a separate magazine mesh** 696/0, shells 450/696, depth
  1146/1146; extended 840/0.

#### 4.10.5 Magazines in the grip: pistols, MP7, PP-2000

Every pistol but the M1911 shares one generic BF3 reload [web], and the M9, MP-443 and Glock 18
animation packages hold the same 11 animations with the same lengths [data].

**M9.**
- Real [web]: a Beretta 92FS type: a 15-round magazine (BF3's data says 16: 15 plus one chambered) with
  a reversible button behind the trigger guard; a slide-mounted safety and decocker; the slide stop on
  the left; the slide locks open after the last round.
- BF3 [data]: 15+1, 4 magazines, single shots; recoil 0.7 up, +-0.1 sideways; reload 1.35 s / 1.6 s.
- Its reload, recorded in The Great Destroyer [data]: tactical: the left hand leaves and the pistol
  tilts up to about 38 degrees; about 0.4 s later the magazine drops out along its line (21.2 cm in
  about 0.1 s, with no hand on it: the right thumb's button [inferred]); the left hand goes 64 cm left
  and 28 cm down out of view and comes back with a fresh magazine in a loose pinch; up its line; the
  flat palm pushes the last 7 mm; back to two hands; about 1.35 s in all. The empty reload was never
  recorded (the mod's hidden round keeps the game from reaching 0); IMFDB shows the slide locked
  during it [web], and its long wave adds the slide slamming home at 1.28-1.33 s, only about 0.15 s
  after the seat: a thumb on the slide stop, not a rack [inferred].
- Waves [data]: short 1.933 s (release and out 0.42-0.48 s, a click 0.57-0.61 s, in and seated
  0.95-1.10 s); long 2.2 s (seated 1.09-1.15 s, slide home 1.28-1.33 s, the loudest).
- Draws [mod]: 11970/0, depth 11970/11970; bones 0 magazine, 1 hammer, 2 trigger, 3 frame, 4 slide;
  the magazine's line raked 16 degrees, 21.2 cm of travel, the grip's mouth 11.08 cm below home; the
  slide travels 4.57 cm; the model is about 255 mm long against a real M9's 217 mm, so the game's grip
  puts the fingers 13-17 mm inside it.
- Mod: the train pistol (see 3.1) [mod].

**MP-443 Grach.**
- Real [web]: the Yarygin PYa: an ambidextrous frame safety (not a decocker) behind the slide stop;
  the magazine release in the base of the trigger guard on the left; 17 rounds (18 after 2004).
- BF3 [data]: 17+1 (its data says 18), the M9's reload times; its mesh has glowing aiming dots and a
  visible-rounds part.
- Waves [data]: short (release and out 0.42 s, 0.60 s, in and seated 0.96-1.06 s); long (in
  1.02-1.10 s, slide home 1.28 s).
- VR: the M9's set measured on this gun; the rounds part shown on a loaded magazine [inferred].

**MP7.**
- Real [web + inferred]: 20, 30 or 40-round magazines into the pistol grip; an ambidextrous paddle at
  the lower rear of the trigger guard; a T-handle at the rear top; ambidextrous bolt-catch levers above
  the grip; the bolt locks open after the last round; a folding vertical foregrip.
- BF3 [data]: 20+1 (40+1 extended), 950 rpm, auto and single; reload 2.4 s / 2.6 s; **two magazine
  meshes and two reload sets** (the extended magazine switches the animation set).
- Its reload [web + data]: the 20-round magazine drops out by gravity, the 40-round one is pulled out
  by hand; the empty reload ends with the trigger hand's thumb on a bolt release it doesn't quite reach
  [web; the hand read from the stills]; IMFDB says the 20-round magazine often fails to appear in the
  campaign. One wave pair serves both sets: short 3.37 s (seat at 1.87 s), long 3.57 s (the release at
  2.06 s) [data].
- VR: the magazine up into the trigger hand's own grip; two magazine profiles; the bolt released by the
  trigger hand's thumb (a button under it, perhaps), a support-hand slap, or the T-handle; Night
  Shift's light; the suppressor's muzzle (0.721 m, a 270 m/s bullet [data]).

**PP-2000.**
- Real [web]: 20 or 44-round magazines into the grip (a spare can be stored as a stock); the release's
  place wasn't found; the charging handle sits on top just behind the front sight, folds sideways and
  moves with the bolt; the front of the big trigger guard is a grip for the support hand.
- BF3 [data]: 20+1 (40+1 extended, its own mesh and set), 650 rpm, auto and single; reload 2.45 s /
  3.45 s.
- Its reload [data: waves 2.87 s and 3.87 s; web]: tactical: rolled on its side, the old magazine out
  0.54-0.76 s, a new one pushed up and seated at about 1.66-1.80 s. Empty: then the left hand takes the
  handle on top and draws it back; the bolt slams home at 2.68-2.98 s ("Cocking the PP-2000" [web]).
  Kaffarov's HALO kit is bare, with the standard magazine [data; IMFDB's still reads 21 rounds, web].

**Never yours, for the record:** the Glock 18 has the same generic reload plus a selector dial as its
own part [data]; the .44 Magnum swings its cylinder out, the cases are palmed out and a speedloader
goes in [web], one 3.9 s reload for both cases, and it is the only gun aimed right-handed by the game
[data].

#### 4.10.6 Belts: M240B, M249, M60E4, Type 88, PKP

Shared [data unless marked]: BF3's belt-fed guns have **one** reload, the same with rounds left or
empty; they count 100 with no extra round (open bolt); the game moves no handle when they fire, which
is right for an open bolt [web + inferred]; every optic on them (and the PKP's and Type 88's iron
sights) sits on the bone that is the feed cover on the measured M240, so the scope most likely rises
with the cover [inferred]. Their waves: the M249's and M60E4's are built from the M240's own samples
at the same moments; the PKP and Type 88 share another bank. The M240's cuts (lid latch, lid up,
magazine out and in, belt, belt laid, lid shut) are a template for the shared parts.

**M240B (built).** Its own reload, recorded [data]: 0.4-1.4 s the **right** hand draws the lever
12.9 cm; 1.86-2.33 s the left hand to the lid's rear; 2.4-2.8 s the lid opens 103 degrees and stays
up; 3.1-3.7 s the box and belt off to the left and down; 3.9-4.5 s a new box up into its seat with the
belt hanging out; 4.8-5.3 s the belt taken and laid; 5.77-5.92 s the lid slapped shut; 6.1-6.4 s the
hand back on the front grip. Draws [mod]: 27588/2364 (frame bone 3), belt 2304/0 (five bones), tape
60/2304, depth 29952/29952. Real [web]: feed from the left, links out to the right, cases down through
the bottom; a cross-bolt safety.

**M249.**
- Real [web]: 5.56 on a disintegrating belt fed from the **left**, from a box or a cloth pouch; the
  real gun also takes STANAG magazines in a well under the tray (BF3's model keeps the well, but the
  game has one 100-round "magazine"); the feed cover hinges at the front and opens by two latches at
  its rear; the handle on the right, pulled back and pushed home by hand; open bolt; a cross-bolt
  safety; the bipod folds under the gas tube.
- BF3 [data]: 100 rounds (200 with the extended box), 800 rpm, auto only; one reload, 6.2 s; one
  206-frame animation.
- Its reload [data: wave 7.20 s; web]: the handle drawn 0.57-0.83 s and sent home 1.13-1.22 s; the
  cover latch 2.28-2.38 s and up 2.46-2.87 s; the old box off 3.06-3.4 s; a new cloth box on
  3.96-4.24 s; the belt laid 4.70-5.12 s; the cover slapped shut 5.61-5.69 s; the hand back at about
  6.23 s. Every step lines up with the M240's within 20 ms.
- VR: the M240's block; the ACOG riding the cover; the bipod; a 200-round belt by count.

**M60E4.**
- Real [web]: 7.62, open bolt, a belt fed from the **left** only, from a 100-round box or pouch; the
  handle on the right, still when firing; the cover raised by a catch on its right side; a bipod and a
  carry handle on the barrel.
- BF3 [data]: 100, 580 rpm, auto only; one reload, 7.2 s (236 frames).
- Its reload [data: wave 7.77 s]: the handle 0.5-1.7 s; the cover latch and up 2.28-2.87 s (the
  M240's samples); old box off 3.27-3.42 s; new box on 4.23-4.35 s; the belt 4.70-6.1 s (longer than
  the M240's); the cover slapped shut 6.81-6.96 s (exactly the M240's sample); back 7.42 s.
- VR: the closest gun to the M240; every campaign M60E4 has a sight on its cover (ACOG, EoTech, M145
  or RX01); its own iron sights stay on the frame [data].

**Type 88 LMG.**
- Real [web]: 5.8 mm, a belt fed from the **left**, boxes of 100 or 200, an integral folding bipod;
  the handle's side and the cover latch weren't found.
- BF3 [data]: 100 (200 extended), 650 rpm, auto only; one reload, 7.4 s, the longest in BF3 (the
  wiki); its 3.6 s "short" time is never used.
- Its reload [data: wave 7.90 s; web]: the bolt drawn 0.48-1.00 s; the used box unlatched and off
  1.7-3.4 s; a new box brought up and latched 4.16-5.13 s; the belt set in the feed 5.5-6.3 s (the
  cover may open only now, after the box: unsure); the cover shut 6.52-6.64 s (the loudest); back
  7.14 s. BF3 swaps the box **before** the belt.
- VR: the belt block plus a box latch, in BF3's order; the bipod is always fitted in Kaffarov.

**PKP Pecheneg.**
- Real [web]: 7.62x54R on a non-disintegrating belt fed from the **right**, from a box on the right;
  the cover hinges at the front; the handle on the right, still when firing; cases out to the left;
  a carry handle on the barrel shroud.
- BF3 [data]: 100, 600 rpm, auto only; one reload, 5.38 s; BF3 shows it throwing links out (to the
  left) as if the belt came apart [web: IMFDB]. No campaign kit has its bipod; its foregrip has no
  first-person mesh.
- Its reload [data: wave 6.53 s; web]: the bolt pulled 0.42-0.94 s; the cover popped open 1.54-1.65 s;
  old box off 2.0-2.9 s; new box on 3.47-3.92 s; the belt laid 4.17-4.42 s; the cover slammed
  5.02-5.13 s; back 5.59 s. Which hand pulls the handle is unknown until recorded.
- VR: the M240's block **mirrored**: the box, the belt and the support hand's work on the right, the
  cases on the left; with the gun in the right hand, the support hand reaches across under it.

#### 4.10.7 Shells one by one: 870 MCS, M1014, DAO-12

Shared: a 12-gauge shell is 6.4 x 2.1 cm [data]; it can be drawn in the hand by issuing the gun's own
shell draw at the hand's pose (870: 450/12246; M1014: 450/13701; DAO-12: 450/14559) [data, read
offline]; it would be carried by its base as the M320's round is [inferred]; dropped shells land with
the game's own shotgun-shell sounds [data]. In the game all three can fire in the middle of a reload
[web].

**870 MCS.**
- Real [web]: a pump-action tube shotgun: each shell is pressed against the carrier through the
  loading port under the receiver and pushed forward until the latch clicks; the fore-end on twin
  action bars: pulled back it unlocks, extracts and throws the case out to the right and lets the next
  shell onto the carrier; pushed forward it chambers and locks; a lever at the front left of the
  trigger guard opens a cocked, unfired action; a cross-bolt safety; a shell can be laid through the
  ejection port into an empty gun.
- BF3 [data]: 4+1 (6+1 with an extended tube no campaign kit uses), 12 pellets; the game pumps by
  itself after every shot (0.48 s); reload: 1.15 s, then 0.67 s per shell.
- Its reload [data: waves; web]: each wave is a start, one loop per shell (0.738 s) and an end
  (0.955 s). Tactical (start 0.54 s): the gun rolls its belly toward the left hand, which pushes shells
  up one at a time ("topping off"); a click 0.29-0.36 s into each loop. Empty (start 2.39 s): the
  fore-end pulled back (0.17 s), a shell dropped into the ejection port (1.06 s, "chamberloading the
  870"), the fore-end pushed forward (1.62 s), then the tube filled. The pump after each shot: back at
  0.25-0.33 s, forward at 0.50-0.57 s of its 0.87 s wave.
- Draws [data, read offline]: body 12246/0 (bone 0 trigger, 1 a small part on top at the rear: the
  rear sight [inferred], 2 the fore-end with its action bars and bolt, 3 frame), shell 450/12246, a
  sight dot 24/12696, depth 12720/12720; the fore-end is 16 cm long, 0.32-0.48 m along the gun.
- VR: the support hand's fore-end grip becomes a slider (only full strokes count; it still steers);
  no shot until a full cycle, and the game's own pump hidden; shells into the tube; the port load when
  empty; a live round pumped out falls as a loose shell; the laser's switch.

**M1014.**
- Real [web]: a gas-operated Benelli M4: the large bolt handle on the right moves with the bolt; the
  bolt locks open after the last shell from the tube (it can't be locked open while the tube holds
  one); a bolt-release button under the ejection port; a cartridge-drop lever under the receiver; a
  lifter over the loading port; an empty gun is "combat loaded" through the port, then the release.
- BF3 [data]: 4+1 (6+1 with the extended tube, a longer tube drawn), single shots at 220 rpm; reload:
  1.0 s, then 0.55 s per shell.
- Its reload [data: waves; web]: tactical (start 0.57 s, loop 0.50 s, end 0.80 s): shells pushed up
  one at a time, a click about 0.2 s into each loop. Empty (start 1.93 s, end 0.71 s): a shell dropped
  into the port at about 0.97 s, the release pressed and the bolt slamming home at about 1.11-1.18 s
  (the loudest sound), then the tube filled. The handle is never pulled.
- Draws [data, read offline]: body 13377/0 (bone 0 the rear sight [inferred], 1 trigger, 2 the bolt
  group, 3 frame), sight dots 324/13377, shell 450/13701, depth 14151/14151; the extended tube
  696/0.
- The train door: the game plays a scripted first-person pickup there (`PUS_player`) [data].
- VR: shells into the tube; the port load and the release; the locked-open bolt drawn back; the handle
  as the AN-94's lever; the train-door gun taken by hand.

**DAO-12.**
- Real [web]: a 12-shell drum turned by a clockwork spring, wound with a key at the front; shells go
  in one at a time through a gate at the right rear; BF3's mixes the Striker's front key with the
  Protecta's automatic ejection and has no drum-advance lever.
- BF3 [data]: 8 shells (12 with the extended drum, which every campaign one has), no chamber, single
  shots; reload: 2.3 s, then 0.69 s per shell; one wave and one clip for every reload.
- Its reload [data: wave; web]: the left hand winds the drum by its key (about eight ratchet clicks at
  0.8-1.85 s), a click at 2.2-2.28 s (the gate opening [inferred]); then each shell, pinched by its
  base, is pushed into the gate (a click 0.24-0.31 s into each 0.72 s loop); the drum never turns
  between shells; the hand is back at about 3.36 s. It probably winds on every reload, not only when
  empty [inferred].
- Draws [data, read offline]: body 14559/0 (bone 0 trigger, 1 a thin piece on top, 2 the frame with
  the drum), shell 450/14559, depth 15009/15009. Nothing on the drum moves.
- VR: a ratchet twist of the key (the closest built thing is the SMAW case's twist), then shells
  through the gate.

#### 4.10.8 Turn bolts: M40A5, M98B, SV-98

Shared [data]: the game cycles a bolt gun only once the trigger is let go (`HoldBoltActionUntilFireRelease`),
and zoomed it also waits for the zoom to be let go and unzooms. That gives the mod a clean hook: hold
the game's fire input after a shot until the hand's bolt is back down. Two ways [inferred]: set the
game's bolt time near 0 for the player's gun (that data is shared with Kaffarov's enemy SV-98s: a
decision), or let fire go as the hand lifts the bolt so the game's cycle runs alongside the hand's
stroke. Either way the drawn bolt follows the hand, not the game's animation. Their unzoomed spread is
3-7 degrees, so the spread must be cleared as for the rifles [data]. Each empty reload opens the bolt
first, swaps the magazine, then closes the bolt [data].

**M40A5.**
- Real [web + inferred]: a Remington 700 short action: the handle on the right lifts about 90
  degrees, goes back, forward and down, cocking as it opens; no hold-open; a detachable box with a
  paddle at the front of the trigger guard; the safety behind the handle.
- BF3 [data]: 10+1, bolt time 1.1 s; reload 2.7 s / 4.0 s, held off until fire and zoom are let go.
  Night Shift's unlock turns off the zoom rules and switches to straight-pull animations, so the bolt
  cycles without leaving the scope; its bullet kills in one shot anywhere [web].
- Its cycle and reload [data: waves; web]: the bolt (1.77 s, worked by the right hand): lifted and
  drawn 0.25-0.39 s, at its stop (case out) 0.54 s, forward and down 0.88 s. Tactical (3.3 s, by the
  left hand): release and magazine out 0.40-0.68 s, a new one brought up 1.2-1.8 s, seated 1.97 s, a
  palm tap 2.08 s, back at about 3.1 s. Empty (4.7 s, by the right hand): the bolt opened 0.13-0.39 s
  and at its stop 0.56 s, magazine out 1.03-1.26 s, in 1.92-2.31 s, the bolt forward stripping a round
  2.97-3.04 s, down 3.35 s.
- VR: the turn bolt by either hand (the fore-end alone holds the gun while the trigger hand works the
  bolt, or the support hand reaches over); no shot until it is down; the eye can stay at the scope; a
  tactical swap keeps the chambered round; cases out on the back stroke; the thermal picture; the
  suppressor's muzzle at 1.33 m [data].

**M98B.**
- Real [web]: a Barrett 98B in .338 Lapua Magnum: nine lugs in three rows (about 60 degrees of lift
  [inferred]), a stroke of about 13.8 cm; a latch ahead of the trigger guard worked by the trigger
  finger; a safety that can't go on until the bolt has been cycled; a bipod.
- BF3 [data]: 5+1 (cut from 10 "for balance" [web]), bolt time 1.3 s (the slowest), it unzooms to
  cycle; reload 2.9 s / 4.7 s; 650 m/s.
- Waves [data]: the bolt (1.73 s): lifted 0.31-0.38 s, back 0.50-0.57 s, forward 0.83-0.90 s, down
  1.09 s. Tactical (3.33 s): release and out 0.71-0.95 s, a new one up 1.45-1.62 s, seated 2.14-2.28 s.
  Empty (5.3 s): the bolt opened 0.37-0.54 s, out 1.47-1.66 s, a new one up 2.17-2.34 s, in
  2.83-2.90 s, forward 3.95-4.02 s, down 4.18-4.26 s.
- VR: the M40A5's set with its own numbers; the bipod; the 12x scope ("substantial sway unless a bipod
  is used" [web]).

**SV-98.**
- Real [web]: 7.62x54R, three front lugs (about 60 degrees [inferred]), a 10-round box (the release's
  place wasn't found), a safety behind the handle, a folding bipod.
- BF3 [data]: 10+1, bolt time 1.25 s; reload 2.4 s / 4.8 s; 520 m/s.
- Waves [data]: the bolt (2.12 s, "somewhat dramatic" [web]): the hand reaches the knob by about 0.7 s,
  lifted 0.72 s, back 0.91 s, forward and down 1.11-1.18 s. Tactical (2.61 s): out 0.59-0.95 s, in
  1.91-1.98 s. Empty (5.0 s): the bolt opened 0.71-1.01 s, out 1.60-2.22 s, in 3.01-3.08 s, forward
  and down 3.96-4.23 s.
- Its PKS-07 is the mesh the mod already handles [inferred].

#### 4.10.9 Launchers, grenades, mines and blades

**M320.**
- Real [web]: single-shot 40 mm; the barrel release in front of the trigger guard is pushed up and the
  breech swings out to the left about a front pin; no extractor, so the case tips out or is pulled by
  its rim; a double-action-only trigger and an ambidextrous safety. BF3's underslung one has no sight
  to raise and no safety [web + data].
- Under the M416 [data, recorded]: a weapon of its own in slot 2, 1+3, reloading itself in 2.9 s: the
  barrel springs 15 degrees open in 0.25 s, the left hand pulls the round out and puts a fresh one in,
  the barrel snaps shut in 0.093 s with a 0.7-degree bounce.
- Standalone [data]: its own reload clip (95 frames, 3.17 s); the pickup holds 8 rounds; its wave
  (3.32 s) has hits at 0.07, 0.43, 0.76-0.93, 1.25-1.59, 2.06 (loudest) and 2.50 s: open, case out,
  round, home, shut in the underslung's order [inferred]; held like a rifle; its leaf sight zooms to
  35 degrees.

**SMAW.**
- Real [web]: an 83 mm reusable launcher; each rocket comes sealed in a container that mates to the
  launcher's back and locks; a 9 mm spotting rifle (it does nothing in BF3).
- BF3 [data]: 1+3 in the data (the wiki says 4+1); its reload starts by itself about 1.2 s after the
  shot (clip 185 frames, 6.17 s). Recorded: 1.5-2.5 s the launcher swings nearly upright with the left
  hand on the front grip; 2.9-6.1 s the **right** hand fetches the rocket: the case (64 cm long,
  8.9 cm across) comes forward 2.7 cm, turns 20.5 degrees, swings 45 cm aside and back, is seated and
  twisted shut; 6.1-6.5 s back. The mod's sound cuts: unlock 1.72-2.00 s, out 2.00-2.28 s, take
  2.76-3.05 s, catch 3.82-3.90 s, in 4.12-4.42 s, lock 5.39-5.50 s [mod].

**FIM-92 Stinger.**
- Real [web]: a gripstock (trigger, safety-actuator, uncage switch, a fold-out IFF aerial) on a
  throw-away tube, with a battery-coolant unit of about 45 s; to reload, the gripstock moves to a new
  tube.
- BF3 [data, recorded]: reload 4.3 s (130 frames): nothing comes off the tube; at about 1.4-2.3 s the
  aerial's four panels fold; at 2.4 s the left hand reaches round to the tube's right and lifts them;
  they spring open by 2.86 s.

**FGM-148 Javelin.**
- Real [web]: a reusable command launch unit (day sight, thermal sight with two fields, the seeker's
  view; a four-position switch on the left handgrip, attack select on the right) on a throw-away tube
  with its own battery-coolant unit (4 minutes); the seeker cools in about 10 s; top attack by default;
  a soft launch; to reload, the CLU goes onto a new tube.
- BF3 [data]: 1 missile, spare count -1 (probably unlimited) plus a refill script; reload 3.0 s (clip
  105 frames, the motion ending at about 2.9 s), held off until fire and zoom are let go; its sound is
  the SRAW's, with one loud clunk at 1.16 s: a new tube seated in one movement [inferred]; it locks only
  zoomed ("radar", 2.5 s, 5 degrees, 500 m), fires only when locked; the missile climbs (top attack:
  upward gravity while guided, guidance after 1.75 s, the motor at 0.5 s); the CLU's zoomed view is
  about 4.9x [inferred from its zoom data]. Never recorded: Guillotine hasn't run in VR.

**M136 AT4.**
- Real [web]: a throw-away 84 mm recoilless launcher: the transport pin out, the sight covers open, the
  cocking lever pushed forward and down by the right thumb, the forward safety held down, the red
  trigger button pressed.
- BF3 [data]: 1+4 in the data (the wiki says 4+1); a real reload clip (130 frames, 4.33 s) with a loud
  clunk at 1.14 s and clicks at 2.59, 2.83 and 2.99 s: a fresh tube, then priming [inferred; the wiki's
  screenshot is captioned "Priming an AT4 for fire", web]; its projectile is the RPG-7's; its own spread
  (1-5 degrees unzoomed); while it is out the scene blocks weapon changes, and the start kit returns
  after delays of 3.5, 4.2 and 5 s.

**M224 mortar.** Carried in both arms (its own carry, crawl and sprint clips); its "fire" places it:
1.3 s later the deployed mortar appears 1.5 m in front on flat ground [data]. In Guillotine a prompt at
the wall sets it up and Campo fires the illumination round; you never fire it yourself [data + web].

**M67.**
- Real [web]: a 400 g steel sphere; the spoon held against the palm; the pin pulled by its ring,
  usually by the other hand; a 4-5 s fuse from the moment the spoon flies off; thrown about 35 m.
- BF3 [data]: hold-and-release; holding never cooks it; the pin comes out in the first clip (0.83 s,
  the other hand) and the grenade leaves 0.17 s into the throw clip (1.33 s), from the camera at 3 m/s
  up and 18 m/s forward; it keeps 0.3 of its speed per bounce; it goes off 3.6 s after the throw (112
  damage, 7 m radius, 1.5 m inner); its own deploy, pin and throw sounds (5, 5 and 4 takes).

**M18 Claymore.**
- Real [web]: a curved case marked FRONT TOWARD ENEMY, folding scissor legs, a peep sight, fired by a
  "clacker" on a wire.
- BF3 [data]: held in both hands; its placing clip pauses 0.4-0.57 s while it is set down and the mine
  appears 0.35 s in; it sticks to surfaces tilted up to 45 degrees; at most 2; it arms after 3 s; a
  60-degree, 1.5 m cone at more than 3.8 m/s (vehicles 3 m, after 4 s); 50 health. Crouched or prone
  soldiers get past it [web].

**Knife and machete.**
- BF3 [data]: one-handed; an attack is an instant hit 0.3 s in, 100 damage; the knife is raised upright
  when a takedown target is in range; 12 first-person takedowns: standing from the back 2.43 s, front
  2.33 s, left 3.17 s, right 2.40 s; crouching 2.17, 1.97, 2.17 and 2.83 s; prone 3.17, 2.67, 2.53 and
  3.10 s. On PC, F is the quick knife and 6 the knife in hand [web].
- The machete has the knife's data and its own mesh; Comrades' wire cut is a 4.63 s scene with two
  chops (at 1.12 and 2.40 s in its sound) [data]; the wiki says the rifle's stock can cut it too [web].

**Handcuffs.** A placeholder weapon whose animation package holds only the cuffed bare hands' poses
(deploy, idle, run, sprint, crawl, vault); the fights with them are scenes [data].

---

## 5. The prep plan

Everything in this section is a proposal [inferred], built on the project's agreed order [mod]:
the M416's mechanics by either hand, then free hands for every gun, then the game's pickups taken by
hand, then every gun's and attachment's mechanics learned from its own recorded reload, gun by gun,
vanilla first.

### 5.1 Principles

1. **Foundations before guns.** Every new gun built before either hand works for all would have to be
   done twice.
2. **Shared mechanics before single guns.** One block (the AK handle, the belt, the shell, the turn
   bolt) covers several guns at once; build the block on the gun that needs it first in the campaign.
3. **Missions in campaign order** to break ties, since the next playthrough meets them in that order.
4. **Record before building.** Every gun's own reload is recorded in the game first, and its sounds
   cut from its own waves.

### 5.2 Build order

| Step | What | Guns it covers | First needed in |
| --- | --- | --- | --- |
| 0 | Finish what is started: try the M416's reloads by either hand; free hands for every held gun and the launchers; the game's pickups by hand; loose objects kept drawn while another gun is out; weapon picks while a gun lies | all | every mission |
| 1 | The arms of the other levels: check the gloved arms on Swordbreaker, Uprising, Guillotine, Thunder Run and Night Shift; line up Dima's arms; the bare cuffed arms beyond the train pistol | every gun in CO, KA, SF, GD | SF (1) |
| 2 | Record the game's own reloads, level by level (5.4) | all new guns | before each gun |
| 3 | The AK family extended: AKS-74u, AS Val, Saiga 12K (its separate magazine draw), SG 553 (plus the bolt-catch touch); then the held-back-when-empty flag for the SVD, M39 EMR and M82A3 | 7 guns | SF (1) |
| 4 | The AR family extended, and the sight rows: Mk11, M27 IAR; EoTech, RX01, the ACOG on held guns, M145, the rifle scope, the ballistic scope | 2 guns, about 20 guns' sights | SB (2) |
| 5 | The HK notch and slap | G3A3, UMP-45 | SB (2) for the G3A3, OG (5) for the UMP-45 |
| 6 | The belt family: M249 and M60E4 (the M240's layout and sounds), Type 88 (a latched box), PKP (mirrored) | 4 guns | SB (2) |
| 7 | Shells: the shell in the hand, the tube, the port load, the chamber kept by the mod; then the M1014, the 870's pump, the DAO-12's winding | 3 guns | SF (1) |
| 8 | Side and top handles: SCAR-H (left, moving), F2000, PDW-R, USAS-12, G36C (top, back when empty), PP-2000 (top) | 6 guns | OG (5) |
| 9 | Magazines in the grip: the M9 with real rounds, then the MP7 (two magazines, the thumb's release), the PP-2000, the MP-443 | 4 guns | SF (1) |
| 10 | The turn bolt and the thermal picture: M40A5, then M98B and SV-98; the thermal picture also serves the M82A3 and every IRNV | 3 guns + 2 scopes | OG (5) for the M98B, NS (9) for the M40A5 |
| 11 | The P90's top magazine | 1 gun | CO (6), enemy drops only |
| 12 | Attachments: foregrip, bipod, the light and laser (switch and relocation), suppressor muzzles, extended magazines | many | UP (3) for the light and suppressor |
| 13 | Launchers, grenades, mine and blades: AT4, Javelin, the standalone M320, the M224's carry and set-up prompt, the M67, the claymore, the knife, the machete | 8 items | SB (2) |
| 14 | Mounted and vehicle weapons: the seat and the pivot gun (technical, Humvee and roof M2HB), the jet, the tank and the UAV | 5 systems | SB (2), UP (3), GH (4) |
| 15 | Recoil for everything, the last foundation still open | all | every mission |

If the next mission isn't played in campaign order, take that mission's column in 5.3 and pull its
steps forward.

### 5.3 What each mission can't be finished without

The minimum to play each mission through in VR, beyond the foundations of step 0 and that level's
arms [data + web for what the mission requires; inferred for the list]:

| Mission | Must work | Worth having (pickups and drops) |
| --- | --- | --- |
| 1 SF | the M9 for real; the M1014 (forced at the train door); the AKS-74u from the QTE; QTEs on the right grip | the UMP-45 (if it drops) |
| 2 SB | M4A1 with EoTech; Mk11 with the 8x scope (overwatch); the AT4 (the hotel sniper); the technical's machine gun (the finale); the cellar QTE | M67, knife, M249, AK-74M, G3A3, RPK, Saiga, AKS-74u, AEK, PKP |
| 3 UP | the knife-only crawl and the stab prompt; the AKS-74u with suppressor, Kobra and light; the Humvee's M2HB | AK-74M, KH2002, G3A3, 870, Saiga, PKP, RPK |
| 4 GH | the jet's back seat: cannon, Sidewinder lock by the head, flares, the targeting pod, HARMs, the JDAM laser, the A-10 gun run | none |
| 5 OG | the mortar carry and prompt; SCAR-H with ACOG; the 870; the Javelin (three T-90s); the M4A1 with ACOG; the knife QTEs; the rappel | the standalone M320, the canal AEK, the M98B, the vault M1014, UMP-45 and M16A4 from fallen Marines, PKP |
| 6 CO | Dima's arms; AS Val; AKS-74u with PK-A; the machete's wire cut; QTEs | G36C, SG 553, P90, UMP-45, MP7, F2000, M1014, 870, the office AK-74M and AKS-74u |
| 7 TR | the tank (drive, cannon, coax, zoom, thermal); the UAV; the roof M2HB; the M4A1 on foot; the MICLIC scenes | AK-74M, KH2002, G3A3 on foot |
| 8 FE | the tank; the roof M2HB | none |
| 9 NS | the M40A5 with its thermal scope; the MP7 with its light in the sewers; the claymore at the mall; the knife scenes | the spare M40A5s, AEK, Saiga, PKP, RPK, 870, AKS-74u |
| 10 RH | done in VR; open: either hand beyond the M416, the M67, the knife | — |
| 11 KA | Dima's arms; the PP-2000 (HALO); the AEK-971 kit with laser, light and foregrip; the Saiga; the MP-443's QTE; the M82A3 with its thermal scope; the Ka-60 QTE; the M416 at the end | the basement five (SCAR-H, USAS-12, M39 EMR, M27 IAR, F2000), M60E4, Type 88, DAO-12, PDW-R, MP7, IRNV sights |
| 12 GD | the M9 (escape, street, police-car chase); the M1014 at the train door; the AKS-74u QTE; the SG 553 in the sewers; the final fight's QTEs | UMP-45, MP7, Saiga, F2000, Type 88 |

### 5.4 What to record from the game first

**Already done offline, no game needed** [data]: for every campaign gun, its first-person draw keys and
bone order can be read from its mesh's header in the game's files, and the box of each bone's part
from its vertices. Checked against the 10 guns the mod measured in the game: the same keys and the
same frame bones. Each gun's animation package also gives its clip names and lengths (tactical
versus empty reload, deploy, fire), and its reload waves give the order and times of its steps (all
in 4.10). What still needs the game: how the bones **move** (the animation curves are compressed and
were not decoded), where the game's hands go, and each sight's line.

**The recipe, per gun** [mod]: put the gun in the soldier's hands with the blueprint test switch;
record its reload with `weapon.txt allrec` (fire first: a full gun won't reload; record both the
tactical and the empty reload); dump its mesh; build its collision shape; fit the hands on its
magazine and handle (magfit); hold the zoom to read its sights' line; cut its sounds from its own
waves. The test switch today swaps the M416's blueprint, which the player holds only in Rock and a
Hard Place (and Blackburn's last minute of Kaffarov), so it must first learn to swap from **that
level's own slot-0 gun** [inferred].

**Extra things to record:**
- the bolt guns' cycle after a shot, not only their reload;
- the 870's pump after a shot;
- which hand pulls the handle on the M249, M60E4, PKP, Type 88 and F2000;
- the belt-feds' cover with a sight on it (does the sight rise with it?);
- the M9's empty reload (the hidden round must be off for it);
- the shared grenade, knife and takedown clips (`s_basicassets`), and each level's scripted scenes;
- the game's own recoil per shot, for the recoil foundation.

**Sessions, by how much each covers** (which guns each level loads [data]):

| Level | Guns to record there | Why |
| --- | --- | --- |
| Comrades (`SP_Paris`) | AS Val and AKS-74u (in the kit), G36C, SG 553, UMP-45, MP7, P90, F2000, PKP, M1014, 870 MCS; also Dima's arms | 4 carbines, 3 SMGs and 4 more in one level |
| Kaffarov (`SP_Villa`) | PP-2000, AEK-971 (kit), Saiga 12K, MP-443, M82A3; SCAR-H, USAS-12, M39 EMR, M27 IAR, F2000 (the basement); M60E4, Type 88, DAO-12, PDW-R, SV-98; Mk11 and SVD (loaded there too) | the most guns of any level; the second session on Dima's arms |
| Guillotine (`SP_Bank`) | SCAR-H (kit), 870 (kit), M4A1 with ACOG, the standalone M320, M98B, M1014, Javelin, M224, M249, PKP, UMP-45 | the launchers and the first bolt gun |
| Swordbreaker (`SP_Earthquake`) | M4A1 and Mk11 (kit), AT4, M249, PKP, Saiga, AKS-74u | mission 2's own guns |
| Night Shift (`SP_Sniper`) | M40A5 (kit), MP7 with the 40-round magazine (kit), the claymore, SVD | the turn bolt and the thermal scope |
| Semper Fidelis or The Great Destroyer | M9 (empty reload), M1014, AKS-74u, UMP-45; GD also SG 553, MP7, Saiga, F2000, Type 88 | the bare arms |
| Uprising (`SP_Earthquake2`) | AKS-74u with suppressor and light, 870, Saiga, PKP | the light and suppressor on a held gun |

### 5.5 Open questions

**Decisions owed** (none taken here):

1. The light and laser switch: a touch on the device, or a button.
2. The magazine button for a gun held in the left hand (X is on the right controller on the Steam
   Frame and on the left on a Quest); the fire-mode button (to be chosen on Quest).
3. The EoTech's power: 1.82x as the game, or 1x as a real one; the IRNV's (1.43x or 1x) and how its
   thermal picture shows.
4. The Afghan flick on guns not on the paddle list: the Saiga 12K, G36C, UMP-45, and the M82A3's
   heavy magazine.
5. The SCAR-H and SG 553: also chamber by their handles, besides the game's bolt release? The G36C's
   release button for the trigger finger? The MP7's release on a button under the trigger hand's
   thumb?
6. The bolt guns: the shot held until the hand's bolt is down (the game's bolt time set near 0, shared
   with the enemies' SV-98s, or the game's cycle run alongside); is a live round lost when the bolt is
   worked?
7. The shotguns: one shell or two per reach to the belly; the 870 blocked until pumped; both the port
   load and "tube then rack" allowed; the M1014's release by its button only, or by a tug of the handle
   too; the DAO-12 wound on every reload or only when empty, and how many clicks.
8. The train-door M1014 taken by hand instead of the scripted pickup.
9. The pistol's slide stop: a touch, a button, or only the first trigger pull after a fresh magazine
   (as built).
10. Places: a lone pistol's holster (New York); Dima's MP-443 as a third gun; where launchers, the
    knife and the machete go; whether a blade let go of drops or goes back in its sheath; how the rifle
    shows while a blade is in hand.
11. The bipod: legs by hand with the mod's own rest, and whether to call the game's deploy (it needs
    the zoom held, prone or at a ledge).
12. Holding the breath and scope sway: the game's zoomed sway never reaches the headset today.
13. The police-car chase: the pistol in hand during a scripted ride; the game's auto-aim kept or not.
14. The Javelin's lock: shown inside its CLU picture, or hidden like the Stinger's marker.
15. The MICLIC detonator and the jammer: held items (the survey's item 26) or scenes.
16. The AT4: the full real priming, or only the game's own reload.
17. The claymore in one hand or two.
18. Mounted guns: thumbs or triggers; an overheat cleared by the charging handle or by waiting.
19. The tank: the full view or the sight as a screen; the turret by two grips or one stick; full
    circles while seated turned by the stick.
20. The jet: flares and weapon choice by reaching, or by buttons.
21. The spread fix also makes the enemies' guns of the same model more accurate.
22. The six questions left by the M416's either-hand build (the shut flick mirrored for the left hand;
    a magazine in hand when the rifle drops; the right glove's width; the right hand shutting the M320
    only by a slap or push; the rim pinch; the left index slapping the bolt release).

**Facts to check in the game** [data says one thing, unconfirmed in play]:

- whether the enemies' weapon light comes along, and works, on a gun you pick up;
- whether the pickups that list another gun's parts show bare (the vault and gas-sluice M1014s,
  every Comrades MP7, two GIGN SG 553 variants);
- whether `PlayerUSLMG` (M240B + M9) is ever applied in Swordbreaker;
- whether the CIA agents' M16A4s drop in The Great Destroyer, and Semper Fidelis's two droppable
  UMP-45s;
- that the Guillotine Glock 18 never drops (its soldier type has no drop component);
- whether the leftover player spawns with Rock and a Hard Place's kit (Uprising's default layer,
  Fear No Evil's outro scene) ever apply;
- who Night Shift's M240B gunner is (the template is named for Montes; the wiki says Matkovic);
- that the belt-feds' sights ride their feed covers;
- which hand works the handles on the M249, M60E4, PKP, Type 88 and F2000, and when the Type 88's
  cover opens;
- whether the M1014 shows its bolt locked back after the last shell, and whether the USAS-12 has a
  hold-open;
- the M82A3's handle (its side, whether it moves) and the SV-98's and M82A3's magazine releases;
- the vanilla pistol's slide lock (the hidden round must be off);
- whether the police-car chase's auto-aim can be turned off;
- which seat the Guillotine Humvee ride uses;
- whether Night Shift's thermal look is switched on by the zoom;
- whether Kaffarov's HALO spawn kit (PP-2000 with Kobra, extended magazine and light, a Saiga, a
  knife) ever shows before the bare kit replaces it;
- whether the other gloved-arm levels draw the arms as Rock and a Hard Place does;
- the UMP-45's lever sitting in its notch when empty (read from IMFDB and its sound pattern).

**Corrections to earlier notes and data mix-ups** [data unless marked]:

- The Glock 18 can't be picked up: the only enemy carrier is a canned-scenario actor, a soldier type
  with no drop component.
- A soldier the data lists with a gun is not always a source: canned-scenario and civilian soldier
  types have no drop component (1.7). That takes Swordbreaker's UMP-45s and Uprising's SVD off the
  list of guns you can take.
- The paddle-release list in the notes (mag-removed-by-hand) names the SKS and the G53: no campaign
  level loads either. Its campaign guns are the AN-94, AEK-971, AK-74M, AKS-74u, RPK-74M, SVD, G3A3,
  SG 553 and AS Val.
- The "Saiga 20K" is a 12-gauge Saiga 12K; the folder name is left over from Bad Company [web].
- Pistol magazines: the M9's 16 is 15 + 1, the MP-443's 18 is 17 + 1, the Glock 18's 20 is 19 + 1
  [data + web].
- The SMAW is 1 + 3 in the data and the AT4 1 + 4 (the wiki says 4 + 1 for both); the Javelin's spare
  count is -1, probably unlimited [data + web].
- The Kaffarov weapons case at the landing holds display models (`AllowPickup` false): 25 placed
  pickups, 20 real.
- The AT4 fires the RPG-7's rocket.
- Some pickups and kits list another gun's parts: the M1014s with F2000 parts, the Comrades MP7 (its
  one kit entry, so every MP7 there) with an AKS-74u laser, two GIGN SG 553 variants with a G36C
  sight.
- The Type 88's 3.6 s "shorter reload" is never used (its reload type ignores it).
- `tools/gamesounds.cpp` groups the SCAR-H and SG 553 with the guns whose empty reload pulls a handle;
  theirs press or slap a bolt release (see 4.4).

---

## How this was made

- The game's files were read offline from the install: the table-of-contents, superbundle and
  data-archive files, with read-only readers ported from `tools/gamesounds.cpp`; the 12 levels' 35,772
  data files were indexed, every soldier kit, spawn, pickup, kit change and vehicle seat listed, and
  each weapon's firing data, attachment sockets, animation package and reload waves dumped. The
  results were cross-checked against Venice-EBX. Nothing ran inside the game, nothing was built, and no
  file of the mod's build was touched.
- A level is attributed by the bundles it loads, so a shared prefab counts in every level that loads
  it. Whether each spawn or kit change fires in play was not tested in the game. Which vehicle seat
  holds which weapon was inferred from the seat numbers.
- The mod's state was read from its code and its playtest notes on 2026-11-11.
- A second pass the same day checked every "drop" against the soldier type each spawn uses (only
  `aisoldier` and `soldierextra` have the drop component), every player spawn of every level, the arm
  meshes each level loads, the mission pages of the wiki, and the mod's claims against `src/`.

## Sources

- Game data: a Steam install of the game (read offline) and [Venice-EBX](https://github.com/VeniceUnleashed/Venice-EBX).
- [IMFDB: Battlefield 3](https://www.imfdb.org/wiki/Battlefield_3): the reload captions and stills.
- Battlefield Wiki: the [mission pages](https://battlefield.fandom.com/wiki/Battlefield_3_Campaign)
  ([Uprising](https://battlefield.fandom.com/wiki/Uprising),
  [Night Shift](https://battlefield.fandom.com/wiki/Night_Shift),
  [Thunder Run](https://battlefield.fandom.com/wiki/Thunder_Run),
  [Kaffarov](https://battlefield.fandom.com/wiki/Kaffarov_(Mission)),
  [Operation Swordbreaker](https://battlefield.fandom.com/wiki/Operation_Swordbreaker),
  [Semper Fidelis](https://battlefield.fandom.com/wiki/Semper_Fidelis),
  [Going Hunting](https://battlefield.fandom.com/wiki/Going_Hunting),
  [Fear No Evil](https://battlefield.fandom.com/wiki/Fear_No_Evil),
  [Comrades](https://battlefield.fandom.com/wiki/Comrades),
  [Rock and a Hard Place](https://battlefield.fandom.com/wiki/Rock_and_a_Hard_Place),
  [Operation Guillotine](https://battlefield.fandom.com/wiki/Operation_Guillotine),
  [The Great Destroyer](https://battlefield.fandom.com/wiki/The_Great_Destroyer)) and each
  weapon's BF3 page (for example [MP443](https://battlefield.fandom.com/wiki/MP443),
  [M240B](https://battlefield.fandom.com/wiki/M240/Battlefield_3),
  [M136 AT4](https://battlefield.fandom.com/wiki/M136_AT4/Battlefield_3),
  [M98B](https://battlefield.fandom.com/wiki/M98B/Battlefield_3),
  [SV-98](https://battlefield.fandom.com/wiki/SV-98/Battlefield_3),
  [SVD](https://battlefield.fandom.com/wiki/SVD/Battlefield_3),
  [M82](https://battlefield.fandom.com/wiki/M82/Battlefield_3),
  [PKP Pecheneg](https://battlefield.fandom.com/wiki/PKP_Pecheneg/Battlefield_3),
  [M249](https://battlefield.fandom.com/wiki/M249_SAW/Battlefield_3),
  [M60](https://battlefield.fandom.com/wiki/M60/Battlefield_3),
  [RPK](https://battlefield.fandom.com/wiki/RPK/Battlefield_3),
  [Straight Pull Bolt](https://battlefield.fandom.com/wiki/Straight_Pull_Bolt),
  [IRNV scope](https://battlefield.fandom.com/wiki/IRNV_scope),
  [Bipod](https://battlefield.fandom.com/wiki/Bipod)).
- Walkthroughs: [Operation Guillotine (portforward)](https://portforward.com/games/walkthroughs/Battlefield-3/Operation-Guillotine.htm),
  [Going Hunting (portforward)](https://portforward.com/games/walkthroughs/Battlefield-3/Going-Hunting.htm).
- Real weapons: Wikipedia ([M40](https://en.wikipedia.org/wiki/M40_rifle),
  [Barrett 98B](https://en.wikipedia.org/wiki/Barrett_Model_98B), [SV-98](https://en.wikipedia.org/wiki/SV-98),
  [SR-25](https://en.wikipedia.org/wiki/Knight%27s_Armament_Company_SR-25),
  [Dragunov](https://en.wikipedia.org/wiki/Dragunov_sniper_rifle),
  [M39 EMR](https://en.wikipedia.org/wiki/M39_Enhanced_Marksman_Rifle),
  [Barrett M82](https://en.wikipedia.org/wiki/Barrett_M82), [PK](https://en.wikipedia.org/wiki/PK_machine_gun),
  [PKP](https://en.wikipedia.org/wiki/PKP_Pecheneg), [M249](https://en.wikipedia.org/wiki/M249_Squad_Automatic_Weapon),
  [M60](https://en.wikipedia.org/wiki/M60_machine_gun), [FN MAG](https://en.wikipedia.org/wiki/FN_MAG),
  [RPK-74](https://en.wikipedia.org/wiki/RPK-74), [M27 IAR](https://en.wikipedia.org/wiki/M27_Infantry_Automatic_Rifle),
  [QJY-88](https://en.wikipedia.org/wiki/QJY-88), [M2](https://en.wikipedia.org/wiki/M2_Browning),
  [FN F2000](https://en.wikipedia.org/wiki/FN_F2000), [AN-94](https://en.wikipedia.org/wiki/AN-94),
  [AEK-971](https://en.wikipedia.org/wiki/AEK-971), [HK G3](https://en.wikipedia.org/wiki/Heckler_%26_Koch_G3),
  [FN P90](https://en.wikipedia.org/wiki/FN_P90), [PP-2000](https://en.wikipedia.org/wiki/PP-2000),
  [HK UMP](https://en.wikipedia.org/wiki/Heckler_%26_Koch_UMP),
  [AS Val](https://en.wikipedia.org/wiki/AS_Val), [USAS-12](https://en.wikipedia.org/wiki/Daewoo_USAS-12),
  [Armsel Striker](https://en.wikipedia.org/wiki/Armsel_Striker)),
  [Modern Firearms](https://modernfirearms.net/), [American Rifleman](https://www.americanrifleman.org/),
  the US Army's FM 23-14 and FM 23-90 and the M18A1 and M136 training material, and the Navy's
  Remington 870 pages on [tpub](https://tpub.com/).
- This repository: [rock-and-a-hard-place-weapons.md](rock-and-a-hard-place-weapons.md),
  [smaw.md](smaw.md), [pistol-manual-reload.md](pistol-manual-reload.md),
  [other-guns.md](other-guns.md), `build/survey/` (the recorded reloads), `build/gamesounds/` (the
  decoded waves), `CHANGELOG.md`.

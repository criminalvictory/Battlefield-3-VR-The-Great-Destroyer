# The AT4 (Operation Swordbreaker's M136)

The owner, 2026-11-18: "The AT4 and M249.. get them in VR with manual reloads .. the whole gyst"; on the AT4: "That's just a one shot launcher.. once
it's shot, it's MEANT to be thrown away and discarded.. a smart player would holster their rifle before picking up the AT4, but they could momentarily
drop their rifle on the floor, pick up the AT4, shoot it, discard it, then pick up their rifle again." Players reported it "not rendered in VR": the
game drew it fixed to the head, as there was no held-gun row for it.

This note is what was measured offline, what is built, what is not, and what the roof measurement (`build\designs\at4_design.md`, runbook R0-R12) must
confirm. Nothing of it has run in the game yet.

## What the game has (measured offline)

**The scene** (`build\designs\sb_at4_probe.md`): the AT4 is no pickup but a scripted use point, a `GameInteractionEntityData` on the hotel roof at
(-456.142 94.189 -2013.601): the reload key (`EIAReload`) pressed within 2 m, any angle, once (`MaxUses` 1, `HoldToInteractTime` 0), its prompt
`ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG`. Its use saves a checkpoint ("RPG"), then the kit `PlayerAT4` puts the M136 in slot 0 in place of the M4A1 (the Mk11
stays in slot 1) and an input restriction blocks fire and weapon changes; the scene's fire trigger is `EIAFire` (7); `HotelDestruction` gives the start
kit back after 3.5-5 s.

**The game's use** (read in `build\bf3_mem.exe`, 2026-11-18; the listings are in the session's scratchpad `at4b\usefn`):
- The interaction update (008B8940) tests its target's type by its slot 0 (008B8EBB: 023D97A8, `ServerInteractionEntity`, class 0x2EB, one class: its
  vtable **020BC9F8**, found statically; its data's 021C6DD4). A target of kind 3 (a flag) takes a ray test and no key; any other: `HoldToInteractTime`
  0 asks 011BB370 (the action down now with its level zero before: a press), else 011BB350 (down); true: **008A9440(entity, soldier)**.
- 008A9440 (thiscall, the soldier on the stack, `ret 4`, 631 bytes): returns at once unless the entity is enabled (its +0xA bit 0) or its team allows
  the soldier, the soldier's +0x104 interface answers false at +0x20, its users list has room, the list of who may use it allows the soldier (008A8630),
  its uses (+0x94) are under `MaxUses` (data +0x6C), `DelayBetweenUses` (data +0x88) has passed, and the soldier isn't a user already. Then it adds the
  soldier to its users, counts the use, keeps the time and switches the point's update on: that update (008A1A90, slot 30) raises `OnInteraction` once
  the longest hold reaches `HoldToInteractTime`: at its next pass for the AT4's 0. Once used up it returns at its own check.
- The data: +0x60 `UseWithinRadius`, +0x64 `UseWithinAngle`, +0x6C `MaxUses`, +0x88 `DelayBetweenUses`, +0x8C `TeamId`, +0x90 `InputAction`, +0x94
  `HoldToInteractTime`, +0x98 `InteractionEntityType`, +0x9C `InteractionSid`. Enum values: `EIAReload` 15, `EIAInteract` 35, `EIAFire` 7.
- Not established: what the soldier's +0x104 interface answers at +0x20; the team bits' sense (the finder and the use read them opposite ways, the use only
  when the point isn't enabled); whether `OnInteraction` comes the same frame as the call.

**Its data** (the public Venice-EBX dump, `Weapons/M136/M136`): its `ShotConfigData` is the SMAW's to the bit (start (-0.3 -0.1 2.0) from the eye,
direction (0 0.01 1), 40 m/s, `ForceSpawnToCamera`), its projectile the RPG-7's; 1 + 4 rockets, `ReloadTime` 4.2 s, `AlwaysAutoReload`; single fire;
no rumble (0 and 0). Its `GunSwayData`: standing and crouched unzoomed its least 1 degree still and 3 moving (most 5), zoomed 0 and 2 (most 2 and 3);
prone 0 either way; the standing block's numbers repeat (the crouched rows are the standing's, the prone rows its sprint's). Its first-person muzzle
effect `FX_Muzz_Generic_AntiTank_1P` at (0 0 1). Its sounds: only its reload wave (`m136at4_reload_1p_wave`); no deploy wave of its own.

**Its mesh** (`build\mesh_15987_0_*`, at4test t2-t3): one draw and its depth pass (15987/0, 15987/15987) on four bones: 0 the tube (the frame), 1 the
cocking lever (on top at its right, z 0.436-0.468), 2 the safety (its right, z 0.545-0.558), 3 the folding front grip (turned down under the tube,
z 0.871-0.885). The tube's front ring's middle (-0.001 0.053) at z 0.995 (the muzzle), 12.2 cm across. The sights stand 6 cm left of the tube: the
rear aperture a ring of 4.6 mm in the leaf at z 0.614-0.616 round (0.0601 0.1279); the front sight a plate topped by three posts, the middle one's tip
(0.0560 0.1250 0.965). Their line lies **0.47 degrees under** the tube's z and **0.68 right** of it (it meets the tube's line 5.2 m out across, 9.2 m
down), and the game's own zoomed camera (the draw scan's eye (0.061 0.129 0.491)) is on that line within 0.6 mm, looking along it. (The design's "7 mm
left of the sight line" took the rear sight's box for its aperture.)

**Its palms** (`build\survey\AT4_fire.bin`, at4test t4): the right (-0.091 0.023 0.409), on the firing mechanism; the left (0.037 -0.047 0.883), on the
front grip; within 0.5 mm of the row.

## What is built

**Held in the hands (B1)**
- `skin.cpp`: a `kHeldKinds` row (frame bone 0, depth 15987/15987, its palms, its muzzle, its sights' line with its up 0.47 and its new `right` 0.68)
  with the marker `kAt4Gun` (1001): its data found by `FindAt4` (exact numbers only, plain read-write memory only): its rocket's start (its own list,
  rewritten every frame to the drawn tube's front ring while it's in a hand, over its own data only) and its sway's blocks found one after the other,
  each stance's unzoomed rows made its zoomed ones through the rifles' rows (one hand, `riflespread 0`). Not `ClearLauncherSpread` (its 40mm match would
  take the AT4's prone transitions). Its sight isn't `Sight()`'s (launchers have no eye guard), as the Stinger's isn't.
- `gunprofile.h`: row 14 (`kAt4Row`), held by its front grip; `Profile::right`; `SightsTurn` (raise by up, then turn by right), which every place that
  turns a gun by its sights uses (both hands' poses, the other pose, the aim back from the drawn gun, the steer's conjugation, the kick's fold): with
  right 0 it is the raise alone, bit for bit (at4test t5, recoiltest 14c).
- `ammo.cpp`: the AT4 found by its blueprint's name among the slots (never a fixed slot: the Stinger's way would read the Mk11), its hidden round while
  it's in hand (fired, the game's count stays above none: no reload by itself), `At4Get`, `At4Slot`, `At4GameReload`; the hands' slot rule treats it as
  a launcher with its own count.
- `input.cpp`: the trigger stops at the mod's none ("the AT4 is spent: let go of it to throw it away"); X held with it spent asks the game's own reload
  (its hidden round out: a fresh tube as the game animates it) for a scene that waits for a second rocket.
- `hands.cpp`: its shot from its own count's drop (the recoil's extra shot), its front grip's buzz share, its slot by name for recoil and for a lying gun
  (`SlotOfProfile`: every launcher by name).
- Recoil: `tools\recoil\kick.tsv`'s line has its body and shape (its kick still design 6's placeholder until R8); `recoil.h` buzzes a launcher whose data
  has no rumble at the SMAW's (1.0 for 55 ms, its front grip's hand 0.8, a late 0.6), so its blast is felt (only the AT4's data has none).
- Its collision shape's recipe (`shaperecipes.h`: frame bone 0, its lever, safety and front grip left out; proved byte for byte by shapemaketest "all")
  and its loose body (`gunbodies.h` by gunbodyfit: 22 hull points, 5.0 kg empty, a polymer knock).
- The quick-time guard: neither version presses the left mouse button while a launcher is in a hand (the roof awaits Fire 0x07 with the AT4 out);
  version 2 says it once when such a trigger is awaited.
- Its holster: behind the neck while the Mk11 is in slot 1 (holster.h's launcher rule, unchanged; at4test t8); a key-2 draw the scene refuses is pressed
  again at 0.6 s and given up at 1.5 s.

**Taken by reach and grip (B2), generic for every scripted prompt**
- `pickups.cpp`: the use point's checks apart from the pickups' own (a failure leaves the pickups as they are): its vtable's slots and its class (then a
  target that is a use point is read and published: its action, kind, hold, uses, enabled bit, radius, transform, prompt id), and its use's code, the
  update's type test and key path through its call, every byte and FNV-1a 64 (then a hand's use is the game's own call, made on the game's thread after
  the game's own update, where the game's own call runs; refused if the point isn't the target any more, is off, used up, a flag, or 0.5 s late; its uses
  read again after: a use the game's own checks refused is no use; a fault turns it off for the session). `GameWouldTake` answers 2 for a use point on
  the reload key.
- `usepoint.h` (pure): only while the game targets the point; the reach a disc of 0.5 m round the prompt's point from 0.3 m under it to 1.2 m over it,
  widening 0.2 m a metre up, or the capsule round where the thing lies once measured; a tick on entering; an empty hand's fresh squeeze held 0.25 s asks
  the use once; with a gun in a hand: refused, the holsters' double buzz and a line ("holster it or drop it first"); the squeeze is the use's till let go
  (the grasp is told the hand is busy: no lying gun's take, no magnet, no regrip comes of it).
- `hands.cpp`: the decision each frame before the grasp, the request, its outcome; the prompt's own key tapped when the game's use can't be made (R for
  the reload's action); the gun the scene gives within 8 s of a use comes into the hand that used it, gliding in from the prompt's point (a pickup's
  arrival). `kUseCapsules` takes where the AT4 lies once R4 measures it.
- `input.cpp`: X held with nothing in the hands at a reload-key prompt presses R as a pickup (no reload's side effects); with a gun in a hand it sends
  nothing there (R would use the prompt with the gun in hand) and says why.

**After the shot (B3)**: let go of, the spent tube falls as any gun let go of (its body, its bounces' knocks the hollow plastic's); its first landing is
the roof's own sound of an AT4 let fall (`drop_at4`, made on each player's PC from the level's own wave `sp03_sfx_rooftop_dropat_wave`: its clatter's
first strike, 4.04-4.26 s; chosen by the numbers, not heard).

## Not built

- **Hosting**: a thing lying is drawn only with its own gun's draw. When the scene gives the start kit back, the spent tube on the floor vanishes ("the
  game's own switch took it"); a rifle dropped before the take vanishes when the AT4 comes. Drawing either while the game draws another gun (keeping the
  draw's state, as the casings design does) is the next step (about a day).
- **Where the rifle comes back**: on this branch, when the kit gives the M4A1 back it comes into the right hand, as at any change of gun after a gun lay.
  The rule (back on its shoulder if it was holstered, on the floor where it was dropped, never into a hand by itself) belongs with the holsters' redesign
  (see the merge, below).
- The cocking lever by hand (B4): after the vanilla reload is recorded (R7). A second AT4 by hand: only if the scene waits after a miss (R9).
- Its kick from its own launch (R8): design 6's placeholder until then.
- Films: a scripted hand reaching and squeezing at the prompt, the AT4 in both eyes, the rocket along the sights, the trigger's stop, the kick and the
  buzz, the discard.

## What the roof measurement must confirm or fill in

1. **R2** (the level loaded): the mod's line `Pickups: scripted use points: the game's use of one and its call are as read`. Otherwise the line names
   the row that isn't, and a hand's use is the key.
2. **R4** (at the prompt): `Use: the game targets a scripted prompt ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG (its key's action 15 ...)`: the static vtable
   020BC9F8 is the live one, the action is 15, the hold 0, `MaxUses` 1, the transform read (its place logged). If the line never comes, the target's
   vtable (rd.ps1 -Type) goes into `pickups.cpp`'s `useVt` and `kUseWords`. Also fill in where the AT4 lies (its two ends and radius, the world): one
   row of `hands.cpp`'s `kUseCapsules`.
3. **R5** (the take): the time from the press to the kit (the save's hitch, the first 15987 draw): `kUseGiftMs` (8 s now) covers it; whether a press
   alone took it.
4. **R6** (in hand): the draws 15987/0 and 15987/15987 as the row has them; the palms at rest (the row's, from the garage); the lines `AT4: its rocket's
   start found in N places` and `AT4: its scatter's numbers found in N places`; `Ammo: the AT4: 1 rocket in it, 4 more ... (its hidden round in ...)`;
   when the fire trigger appears (`QTE: the game awaits Fire ... with a launcher in a hand: not answered by the grips` with QTEs on).
5. **R7** (the vanilla reload, with main's DLL): which bone moves (the lever's priming, B4); with this branch the hidden round keeps the game from
   reloading by itself, so R7 is main's DLL or X held with it spent.
6. **R8** (the shot): loaded 1 to 0 (a real rocket: the mod's shot, kick and buzz come of it) or staying 1 (a scripted shot: then nothing of the mod's
   sees it, and the trigger never stops: a shot by the scene's own trigger would need another tell); the rocket's start at the front ring; the kick on
   the frame bone (back cm and ms, rise degrees and ms: `kick.tsv`'s line as "recorded", then `recoilgen.ps1`); `CRASH?` lines (the two eye views).
7. **R9** (a miss): whether the scene waits; if it does, X held with it spent gives a fresh tube (the game's own reload).
8. **R10** (the scans): the rocket start's exact numbers (the SMAW's) and the sway rows as FindAt4 has them.

## The in-game checks (this branch)

1. The level start: the use points' line (above). At the roof's prompt: the `Use: the game targets ...` line; a reach toward the floor at the prompt
   ticks the hand.
2. With the M4A1 in a hand: a squeeze at the prompt is refused (two short pulses, `Use: ... not used: the right hand holds the M4A1 (holster it or drop
   it first)`); X held there sends nothing and says why.
3. Rifle holstered (or dropped): the empty hand at the prompt, the grip held a quarter second: `the game's own use asked`, `the game used the prompt`,
   the checkpoint saved, then `Use: the AT4 the scene gave ... comes into the left hand`; the AT4 in that hand, drawn in both eyes, the other hand on
   its front grip.
4. Its sights: the aperture and the middle post line up on the target; the rocket leaves from the tube's front ring and lands where they point.
5. A grip squeeze never fires it. The trigger fires it once: a kick, a strong buzz on both hands, `AT4: the rocket gone: the tube is spent`; pulled
   again: `the trigger didn't fire: the AT4 is spent`.
6. Let go: it falls and rolls, `drop_at4` on its first landing. The kit back: the M4A1 in the right hand; the tube on the floor gone (no hosting yet).
7. X held with nothing in the hands at the prompt: R held, the game's own use (the fallback).

## Merging with the holsters' redesign (branch worktree-wf_71ea793e-429-1)

- The use's gift is a take for the holsters: in `OnGunChanged`'s gift block, mark its slot as a take's (`g_holsterTaken |= 1u << slot`, the redesign's
  `Frame::taken`), so the AT4 has no home: it is in the hand that took it and goes on a shoulder only by a hand. Both branches change that function:
  the gift block sets `g_pick.landed*` before the take's arrival branch, where the redesign adds `HolsterArriveSlots(take)`; keep both.
- The holstered M4A1 must keep its home while the kit's AT4 holds slot 0: d86655d's `NoteSlots` keeps one home per slot and a new identity in slot 0
  overwrites it (the AT4's id and blueprint). The redesign's "the holstered gun stays on its shoulder while the game's slot holds the new gun" has to
  keep it per gun, and recognise the kit's fresh M4A1 (the same blueprint, a new object, no take's mark) as that gun back in its slot: at its place
  (`Arrive`'s `kAtPlace`), never into a hand; drawn by a hand as any draw.
- The dropped rifle (the owner's flow b) needs the same keeping: remembered where it lay and put back lying when the kit gives it back (or drawn
  there all along, with hosting). On this branch it vanishes with the take and comes back into the right hand.
- d86655d's refusal of a take that would swap out a holstered gun (`TakeOffShoulder`) is a pickup's; the AT4's use isn't one, so nothing refuses it
  there.
- Textual conflicts to expect: `input.cpp`'s `XButton` (both change its branches), `pickups.h`'s `Tick` and `GameWouldTake`, `hands.cpp`'s
  `NoteSwaps` (the redesign's `g_holsterTaken` and this branch's `byUse` sit in the same block). None changes the other's behaviour.

## Tests

- `tools\at4test.cpp` (new, 28 checks): t1 the tables; t2 the mesh's bones and front ring; t3 its sights (the aperture's ring, the middle post, the line's
  angles, the sights point at the muzzle, the game's zoomed camera on the line); t4 the palms in its recording; t5 the sights' turn (bit for bit the
  raise with right 0); t6 the use by reach and grip; t7 the quick-time guard; t8 its holster's place and the refused key; t9 the game's use's code
  against `bf3_mem.exe` (13 pieces of code, 4 runs of bytes, 6 words; each row word for word in `pickups.cpp`).
- `tools\pickruntest.cpp` R18 (12 checks): the use point published, served with a stand-in for the game's use, each refusal, the game's own checks
  refusing, a fault, the checks not as read.
- Changed: bodytest (a tube's ring has no keel: b6's keel rule and b4's 4 s are a flat bottom's), learntest (the learner refuses the AT4's body, as it
  must a tube's; it's measured, never learned), recoiltest (the AT4's buzz the SMAW's; 14c the fold with a sight's right), profiletest (p3b), picktest,
  shapemaketest's table (its recipe). Every offline test program passes (44), with gunbodyfit --check and recoilgen -Check.

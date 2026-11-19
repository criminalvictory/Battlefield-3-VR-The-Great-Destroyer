# The AT4 (Operation Swordbreaker's M136)

The owner, 2026-11-18: "The AT4 and M249.. get them in VR with manual reloads .. the whole gyst"; on the AT4: "That's just a one shot launcher.. once
it's shot, it's MEANT to be thrown away and discarded.. a smart player would holster their rifle before picking up the AT4, but they could momentarily
drop their rifle on the floor, pick up the AT4, shoot it, discard it, then pick up their rifle again." Players reported it "not rendered in VR": the
game drew it fixed to the head, as there was no held-gun row for it.

This note is what was measured offline, what the roof run with main's DLL confirmed since (`build\designs\at4_measured.md`), what is built,
what is not, the in-game checks and the merge with the holsters' redesign. Nothing of this branch has run in the game yet.

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

**Confirmed on the roof since** (main's DLL, 2026-11-18 evening: `build\designs\at4_measured.md`): the use point's vtable 020BC9F8 is the live one, its
action 15, its radius 2, a press; the game targets it from 1.8 m whichever way the head looks; the kit's counts come 0.1 s after the press and the
tube's first draw 0.7 s; the palms and the sights' line are the row's (the zoomed eye on the line within 0.6 mm); the rocket start's two exact
matches in memory are two FiringFunctionData (the AT4's, and another's: RPG7_AI's for the roof's RPG soldiers by Venice-EBX); the scene doesn't wait
after a miss (its fire trigger closes at the shot); the game drops no tube; the kit gives the M4A1 back 1.3-3.3 s after the shot.

## What is built

**Held in the hands (B1)**
- `skin.cpp`: a `kHeldKinds` row (frame bone 0, depth 15987/15987, its palms, its muzzle, its sights' line with its up 0.47 and its new `right` 0.68)
  with the marker `kAt4Gun` (1001): its data found by `FindAt4` (exact numbers only, plain read-write memory only): its rocket's start, in its own
  FiringFunctionData alone (`at4shot.h`: the class's vtable 021C2A68 at its start, its RateOfFire 350 at +0x120 and its ReloadTime 4.2 at +0x13C with
  the shot at +0x60; the SMAW's is 350 and 5.8, the RPG-7's 100 and 4.8, RPG7_AI's 18 and 4.8, all with the same ShotConfigData to the bit), rewritten
  every frame to the drawn tube's front ring while it's in a hand (read again before each write), and put back as its data has it half a second after
  the AT4 leaves the hands (`RestoreAt4Shot`: nothing of the mod's left in the game's data). Its sway's blocks found one after the other, each stance's
  unzoomed rows made its zoomed ones through the rifles' rows (one hand, `riflespread 0`). Not `ClearLauncherSpread` (its 40mm match would take the
  AT4's prone transitions). Its sight isn't `Sight()`'s (launchers have no eye guard), as the Stinger's isn't.
- `gunprofile.h`: row 14 (`kAt4Row`), held by its front grip; `Profile::right`; `SightsTurn` (raise by up, then turn by right), which every place that
  turns a gun by its sights uses (both hands' poses, the other pose, the aim back from the drawn gun, the steer's conjugation, the kick's fold): with
  right 0 it is the raise alone, bit for bit (at4test t5 and recoiltest 14c, each against a frozen out-of-line copy of the code before it).
- `ammo.cpp`: the AT4 found by its blueprint's name among the slots (never a fixed slot: the Stinger's way would read the Mk11), its hidden round while
  it's in hand (fired, the game's count stays above none: no reload by itself), `At4Get`, `At4Slot`; the hands' slot rule treats it as a launcher with
  its own count. Nothing reloads it: one rocket, then thrown away (the user's "MEANT to be thrown away"; the roof's scene never waits for a second).
- `input.cpp`: the trigger stops at the mod's none ("the AT4 is spent: let go of it to throw it away"); X held with it spent sends R as with any gun
  (the hidden round keeps it full to the game: nothing reloads).
- `hands.cpp`: its shot from its own count's drop (the recoil's extra shot), its front grip's buzz share, its slot by name for recoil and for a lying gun
  (`SlotOfProfile`: every launcher by name).
- Recoil: `tools\recoil\kick.tsv`'s line has its body and shape (its kick still design 6's placeholder: the roof's numbers are in at4_measured.md 5.4);
  `recoil.h` buzzes a launcher whose data has no rumble at the SMAW's (1.0 for 55 ms, its front grip's hand 0.8, a late 0.6), so its blast is felt
  (only the AT4's data has none).
- Its collision shape's recipe (`shaperecipes.h`: frame bone 0, its lever, safety and front grip left out; proved byte for byte by shapemaketest "all")
  and its loose body (`gunbodies.h` by gunbodyfit: 22 hull points, 5.0 kg empty, a polymer knock).
- The quick-time guard: neither version presses the left mouse button while a launcher is in a hand (drawn by the mod, or the game's current slot a
  launcher by its kind: a load's fade-in, a scripted camera); the roof awaits Fire 0x07 with the AT4 out. Version 2 says it once when such a trigger is
  awaited. (It holds for the SMAW and the Stinger too: no quick-time event awaiting Fire with them in hand is known.)
- Its holster: behind the neck while the Mk11 is in slot 1 (holster.h's launcher rule, unchanged; at4test t8); a key-2 draw the scene refuses is pressed
  again at 0.6 s and given up at 1.5 s.

**Taken by reach and grip (B2), for the scripted prompts a hand uses**
- Which: a `ServerInteractionEntity` (a use point) on the interact key (EIAInteract 35: doors, breaches, climbing out, a rappel, a mortar, a seat) or
  the reload key (EIAReload 15: the game's own pickup key, the AT4's), a press (HoldToInteractTime 0: one call of the game's use finishes it, where a
  hold's user idles out after 0.4 s with its use counted), never a flag, the game's own reach 3 m at most (`usepoint::ByHand`). The campaign's 75 use
  points (Venice-EBX): 38 on the interact key and the AT4's on the reload key, every one a press; never a hand's: the pose prompts (EIAProne,
  EIAChangePose, EIAClutch: the four holds of 1 s are all of them; the AT4's roof has two, 100 m round, 8 m off), jumps, fires, weapon picks, the jet's
  and the tank's 300 m prompts. Not the game's own pickups (`ServerPickupEntity`: the train's M1014, a dead soldier's gun): those are a hand's takes of
  `pickups.h`, which free hands don't run with the train pistol in hand (the owner's first example, the train's shotgun, isn't served by this).
- `pickups.cpp`: the use point's checks apart from the pickups' own (a failure leaves the pickups as they are): its vtable's slots and its class (then
  a target that is a use point is read and published), and its use's code, the update's type test and key path through its call, every byte and
  FNV-1a 64 (then a hand's use is the game's own call 008A9440, made on the game's thread after the game's own update, where the game's own call runs).
  The ask is taken under its lock and the lock let go before the call: the render thread never waits on the game's use. Refused, each with its words,
  if it's late (0.5 s), not the target any more, a flag, no hand's prompt (checked again on the game's thread), off, used up, or the soldier isn't
  awake (the game's update does nothing for it then); its uses read again after: a use the game's own checks refused is no use. What came of it is
  the game's thread's own word, published at once (a tick gone stale after the use doesn't lose it); a use taken and under way is waited for, 5 s at
  most; one not taken in 1 s is given up and passed by. A take is never served in the tick a use was. A fault in the call: no hand's use of a prompt
  again this session, by the call or by the key (X held, the game's own key, stays the player's). pickups.txt `use 0` turns a hand's use off (with
  level 1, too). `GameWouldTake` answers 2 for a use point on the reload key (X held's R).
- `usepoint.h` (pure): only while the game targets the point; the reach a disc of 0.5 m round the prompt's point (the AT4's own 1.0 m: its tube lies
  about 0.65 m east of the point, at4_measured.md section 2) from 0.3 m under it to 1.2 m over it, widening 0.2 m a metre up, or the capsule round where
  the thing lies once measured; a tick on entering, left only 5 cm past it; an empty hand's fresh squeeze that nothing else of the hands' wants, held
  0.25 s, asks the use once. A gun in the other hand is no bar (a door opened with the rifle in the right hand; the AT4 taken with it there: the
  scene's kit takes it, as the game's own key does). Something else's squeeze at its start is never the use's (`hands.cpp`'s `UseYields`: the hand at
  one of the body's places, a holster's draw or put-away; within 25 cm of a gun in a hand: its front, its magazine, its handle, a regrip; within the
  grasp's own reach of a gun lying); a use's squeeze that takes something else first (a magazine, a grenade, the gun's front) is given up. The squeeze
  begun in reach is the use's till let go (the grasp is told the hand is busy).
- `hands.cpp`: the decision each frame before the grasp, the request (one at a time), its outcome; the prompt's own key (R for the reload's action,
  held 300 ms with no gun in either hand) only where the game's use isn't as read, never after a refusal or a fault; a prompt on the reload key gives a
  gun: the first change of gun within 3 s of its use (the kit's: 0.7 s, measured), never one a pick of the player's or a draw from a place made, comes
  into the hand that used it, gliding in from the prompt's point (a pickup's arrival; `glidefrom 1` doesn't apply to it). A door gives none.
  `kUseCapsules` takes where the AT4 lies once it's measured (a census at the prompt: not made yet).
- `input.cpp`: X held with nothing in the hands at a reload-key prompt presses R as a pickup (no reload's side effects); with a gun in a hand, R as
  before this prompt was a hand's (the game uses the prompt; its kit takes the gun in hand).

**After the shot (B3)**: let go of, the spent tube falls as any gun let go of (its body, its bounces' knocks the hollow plastic's); its first landing is
the roof's own sound of an AT4 let fall (`drop_at4`, made on each player's PC from the level's own wave `sp03_sfx_rooftop_dropat_wave`: its clatter's
first strike, 4.04-4.26 s; chosen by the numbers, not heard).

## Not built

- **Hosting**: a thing lying is drawn only with its own gun's draw. When the scene gives the start kit back, the spent tube on the floor vanishes ("the
  game's own switch took it"); a rifle dropped before the take vanishes when the AT4 comes (the kit takes its slot; the log says so). Drawing either
  while the game draws another gun (keeping the draw's state, as the casings design does) is the next step (about a day).
- **Where the rifle comes back**: on this branch, when the kit gives the M4A1 back it comes into the right hand, as at any change of gun after a gun
  lay. The rule (back on its shoulder if it was holstered, on the floor where it was dropped, never into a hand by itself) belongs with the holsters'
  redesign (below).
- The cocking lever by hand (B4): the roof shows the vanilla game has no priming before the first rocket (the lever moves only inside its reload): a
  lever by hand would be the mod's own addition, for the owner to decide. No second AT4: the scene never waits after a miss.
- Its kick from its own launch: design 6's placeholder (the roof's standing zoomed numbers are in at4_measured.md 5.4: back 0.6 cm at 110 ms, rise 0.4
  degree at 45 ms, lift 0.3 cm; the hip, VR's hold, not recorded).
- Seen on the roof at the take with main's DLL, not changed here (at4_measured.md section 11): recoil set up the M4A1's profile into the AT4's sway
  data for 0.9 s (the drawn body still the M4A1's while slot 0 held the AT4; put back, every shot after had the AT4's own); the held magazine read the
  AT4's one round as the M4A1 run dry (whether the returned M4A1 then wanted a manual reload wasn't checked).
- Films: a scripted hand reaching and squeezing at the prompt, the AT4 in both eyes, the rocket along the sights, the trigger's stop, the kick and the
  buzz, the discard.

## The in-game checks (this branch)

1. The level start: `Pickups: scripted use points: the game's use of one and its call are as read: a hand's use of a prompt is the game's own`.
2. At the roof's prompt: `Use: the game targets a scripted prompt ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG (its key's action 15: R, X held, hold 0.00 s, 2.0 m,
   on, 0 of 1 uses; at (-456.14 94.19 -2013.60)): reach for it and hold the grip: the game's own use, called on the game's thread`. The two
   `ID_H_INT_SP_EARTHQUAKE_TO_PRONE` prompts, if the game targets them, say `not a hand's`, and a squeeze there does nothing of theirs.
3. An empty hand reaching to the tube (within 1 m of the point at floor height) ticks once (`Use: the left hand at the prompt ...: hold the grip`).
4. Rifle holstered, dropped, or held in the other hand: the empty hand at the tube, the grip held a quarter second: `the game's own use asked (#1)`,
   `the game used the prompt ... the gun it gives within 3 s comes into that hand`, then `Use: the AT4 the scene gave 0.7 s after the prompt ... comes
   into the left hand (from the prompt's point)` and `Free hands: the left hand took the AT4 the scene gave`; the AT4 in that hand, drawn in both eyes,
   the other hand on its front grip. A hand on the rifle's handguard, at a shoulder or at the belly while doing so: never the prompt's.
5. `AT4: its rocket's start found in 1 places`; the aperture and the middle post line up on the target; the rocket leaves from the tube's front ring
   and lands where they point.
6. A grip squeeze never fires it. The trigger fires it once: a kick, a strong buzz on both hands, `AT4: the rocket gone: the tube is spent`; pulled
   again: `the trigger didn't fire: the AT4 is spent`.
7. Let go: it falls and rolls, `drop_at4` on its first landing. The kit back: the M4A1 in the right hand; the tube on the floor gone (no hosting yet);
   half a second later `AT4: out of the hands: its rocket's start put back as its data has it (1 place)`.
8. X held at the prompt, with nothing in the hands or with the M4A1 in hand: R held, the game's own use (as before this branch); the AT4 arrives.
9. With the Mk11 in hand (slot 1) at the prompt: whether the kit raises the AT4 (the game's own matter, as with its R): say what the log shows.
10. A door anywhere (EIAInteract within 3 m): the free hand at it, the grip held: the door's scene, no gun given (`the scene goes on`).

## Merging with the holsters' redesign (branch worktree-wf_71ea793e-429-1, 907b2b6 and its settle round)

What this branch needs from it, and where the two meet:
- **Flow (a), the rifle holstered** (owner: holster, take the AT4, fire, discard, draw the rifle from the shoulder): in `OnGunChanged`'s gift block,
  mark the gift's slot as a take's (`g_holsterTaken |= 1u << ammo::GameSlot()`, the redesign's `Frame::taken`) before the take branch's
  `HolsterArriveSlots(take)`/`Arrive`, so the AT4 has no home (it is in the hand that took it and goes on a shoulder only by a hand); check the gift
  path still runs `HolsterArriveSlots`. The holstered M4A1 keeps its home while the kit's AT4 holds slot 0 (the redesign's "weak away" record: a
  holstered gun the kit takes with no pickup comes back put away at its place); the kit's fresh M4A1 (same blueprint, a new object, no take's mark) is
  that gun back in its slot: `Arrive`'s `kAtPlace`, never into a hand.
- **Flow (b), the rifle dropped** (owner: drop it, take the AT4, fire, discard, pick the rifle up off the floor): covered by neither branch. The roof's
  AT4 is the `PlayerAT4` kit, not a pickup: the kit takes the lying rifle's slot and the game's M4A1 object goes. Needed: at the gift, record the lying
  rifle's world pose, kind and slot; when the level's kit puts that kind back into that slot, start it lying there (`kLoose` at that pose, drawn with
  the game's new M4A1) instead of in the right hand. Keeping it visible while the AT4 is out needs hosting; without it, say the gap to the owner (the
  rifle vanishes for the AT4's few seconds, then lies again where it was).
- The X-held fallback records no use (`input.cpp`'s pickup R): measured, the kit lands 0.1 s after R, while X is still held, so X held's own pick
  (`NotePick(-1)` every frame) sends the AT4 into the right hand; 907b2b6's finding 2 covers the rest.
- Textual conflicts to expect: `input.cpp`'s `XButton` (both change its branches: this branch only adds the prompt's R flag and a log line now),
  `pickups.h`'s `Tick` (both add fields) and `GameWouldTake`, `hands.cpp`'s `OnGunChanged` take branch and `NoteSwaps` (the redesign's
  `g_holsterTaken` and this branch's `byUse` sit in the same block), both branches' `void R18()` in `tools\pickruntest.cpp` (rename one).
- At the merge, regenerate the main build's `bodytest_points.bin` (`gunbodyfit build src\gunbodies.h` from the project folder: it has no AT4 yet, so
  bodytest exits 1 on it; the regenerated src\gunbodies.h is the same but for line endings).

## For the changelog at release

The AT4 drawn in the hands (both eyes), one rocket from its tube's end, no reload by itself, a spent tube thrown away; a scripted prompt on the
interact or reload key used by reaching for it and holding the grip (X held still works); the left mouse button never pressed by the quick-time
grips while a launcher (the SMAW, the Stinger, the AT4) is in the hands.

## Tests

- `tools\at4test.cpp` (34 checks): t1 the tables; t2 the mesh's bones and front ring; t3 its sights (the aperture's ring, the middle post, the line's
  angles, the sights point at the muzzle, the game's zoomed camera on the line); t4 the palms in its recording; t5 the sights' turn (bit for bit the
  raise as built before, with right 0); t6 the use by reach and grip (which prompts, the reach, the AT4's disc, the edge's tick, the hold, a gun in the
  other hand, a squeeze something else wants, one given up, nothing, both hands); t7 the quick-time guard; t8 its holster's place and the refused key;
  t9 the game's use's code against `bf3_mem.exe` (13 pieces of code, 4 runs of bytes, 6 words; each row word for word in `pickups.cpp`); t10 its
  firing data against the roof's dump (its own; never the SMAW's, the RPG-7's, the enemies' RPG-7's, another class; its start as last written).
- `tools\pickruntest.cpp` R18 (25 checks): published, served with a stand-in for the game's use, each refusal on either thread, no hand's prompts
  (the roof's pose prompts, a hold, 300 m, a jump), a door's, a stale tick after a served use, a use under way, one not taken in 1 s, `use 0`, a use
  and a take never in one tick, the game's own checks refusing, a fault (never called again, nor its key), the checks not as read.
- Changed: bodytest (a tube's ring has no keel: b6's keel rule and b4's 4 s are a flat bottom's), learntest (the learner refuses the AT4's body, as it
  must a tube's; it's measured, never learned), recoiltest (the AT4's buzz the SMAW's; 14c the fold with a right, against a frozen copy of the fold
  before), profiletest (p3b), picktest, shapemaketest's table (its recipe). Every offline test program passes (44), with gunbodyfit --check and
  recoilgen -Check.

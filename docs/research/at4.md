# The AT4 (Operation Swordbreaker's M136)

The owner, 2026-11-18: "The AT4 and M249.. get them in VR with manual reloads .. the whole gyst"; on the AT4: "That's just a one shot launcher.. once
it's shot, it's MEANT to be thrown away and discarded.. a smart player would holster their rifle before picking up the AT4, but they could momentarily
drop their rifle on the floor, pick up the AT4, shoot it, discard it, then pick up their rifle again." Players reported it "not rendered in VR": the
game drew it fixed to the head, as there was no held-gun row for it.

This note is what was measured offline, what the roof run with main's DLL confirmed since (`build\designs\at4_measured.md`), what is built (with
the roof's numbers put in and the two side effects the roof showed fixed, and the SMAW's rocket's start told from the AT4's and the RPG-7s'; then the
two reviews of that round settled), what is not, the in-game checks, the merge with main (620ba08) and the merge with holsters by hand (main
7e99b44, 2026-11-19: the owner's two AT4 flows built end to end with both together; then the two reviews of that settled, the owner's decision of
that night built: the AT4 taken only with both hands free of guns). Nothing of this branch has run in the game yet.

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
matches in memory are two FiringFunctionData (the AT4's, and RPG7_AI's for the roof's RPG soldiers: each instance's GUID sits 16 bytes before its
vtable in the roof's dump, Venice-EBX's M136 3385F7CF-15C2-4D2D-B571-1CB94130ABF4 and RPG7_AI 4A2ADB00-B529-4C70-925B-AD77F312AC44); the scene doesn't
wait after a miss (its fire trigger closes at the shot); the game drops no tube; the kit gives the M4A1 back 1.3-3.3 s after the shot. Its kick in the
hands from its own launch (hit 1, standing zoomed, the clean one): the tube at the eye 0.60 cm back at 109 ms, its own rise 0.39 degrees at 47 ms over
the camera's one-frame 0.95 (still 0.3 to 141 ms), 0.34 cm up at 62-78 ms; prone zoomed no motion of it in its first 50-60 ms.
The class's offsets are bf3_mem.exe's own reflection: FiringFunctionData's Shot at +0x60 (InitialSpeed at its +0x20: +0x88 its z) and its FireLogic
at +0xC0, FireLogicData's RateOfFire at its +0x60, ReloadThreshold +0x74 and ReloadTime +0x7C (+0x120, +0x134, +0x13C); the roof's dump of the M136's
reads 350, 0.8 and 4.2 there, its Venice-EBX numbers.

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
- `gunprofile.h`: row 15 (`kAt4Row`; the M249 merged first keeps 14), held by its front grip; `Profile::right`; `SightsTurn` (raise by up, then turn by right), which every place that
  turns a gun by its sights uses (both hands' poses, the other pose, the aim back from the drawn gun, the steer's conjugation, the kick's fold): with
  right 0 it is the raise alone, bit for bit (at4test t5 and recoiltest 14c, each against a frozen out-of-line copy of the code before it).
- `ammo.cpp`: the AT4 found by its blueprint's name among the slots (never a fixed slot: the Stinger's way would read the Mk11), its hidden round while
  it's in hand (fired, the game's count stays above none: no reload by itself), `At4Get`, `At4Slot`; the hands' slot rule treats it as a launcher with
  its own count. Nothing reloads it: one rocket, then thrown away (the user's "MEANT to be thrown away"; the roof's scene never waits for a second).
- `input.cpp`: the trigger stops at the mod's none ("the AT4 is spent: let go of it to throw it away"); X held with it spent sends R as with any gun
  (the hidden round keeps it full to the game: nothing reloads).
- `hands.cpp`: its shot from its own count's drop (the recoil's extra shot), its front grip's buzz share, its slot by name for recoil and for a lying gun
  (`SlotOfProfile`: every launcher by name).
- Recoil: `tools\recoil\kick.tsv`'s line has its body and shape and its kick as measured on the roof (size `measured`, a new rule of recoilgen's: back
  0.6 cm, rise 0.4 degrees, lift 0.3 cm, where design 6's placeholder had 1.0, 1.0 and 1.0); its timing `at4` (a new class: the back's spring peaking
  at 110 ms, the launchers' 40 ms drew the recording 0.33 cm rms off, 110 ms 0.10; the rise's 80 ms and the lift's 100 ms the launchers', nearest to it);
  `recoilguns.h` made again by recoilgen (the AT4's row alone changed: back {0.006, 0.11, 0.75}, rise {0.4, 0.08, 0.7}, lift {0.003, 0.1, 0.7}). The
  camera's own kick stays the data's (its drift: 4 a shot standing at the hip, 2 prone, -1..1, no spring). `recoil.h` buzzes a launcher whose data has
  no rumble at the SMAW's (1.0 for 55 ms, its front grip's hand 0.8, a late 0.6), so its blast is felt (only the AT4's data has none).
- Its collision shape's recipe (`shaperecipes.h`: frame bone 0, its lever, safety and front grip left out; proved byte for byte by shapemaketest "all")
  and its loose body (`gunbodies.h` by gunbodyfit: 22 hull points, 5.0 kg empty, a polymer knock).
- The quick-time guard: neither version presses the left mouse button while a launcher is in a hand (drawn by the mod, or the game's current slot a
  launcher by its kind: a load's fade-in, a scripted camera); the roof awaits Fire 0x07 with the AT4 out. Version 2 says it once when such a trigger is
  awaited. (It holds for the SMAW and the Stinger too: no quick-time event awaiting Fire with them in hand is known.) Merged with main's fist fights
  as one rule: input.cpp's QteStep works out the barred keys once; version 2 hands them to `qtewatch::Update`, which takes them out of what the frame
  awaits after the fights' step (`qtewatch::Bar`: a live trigger's and a fight's open prompt alike), version 1 leaves them out of its press. The
  fights' guard (SendKey, SendMouseButton) keeps a prompt's answer its own, so with a launcher in a hand at a prompt for the quick-time Fire the grips
  press nothing, the trigger's own left button still goes through the guard, and every other quick-time key is held up (at4test t7).
  Kept so on purpose (the merge's review, its finding 1): the bar holds in a fight's prompt too, though no launcher can fire in a fight. A fight whose
  prompt read open in free play (the fights' reader is built against the three that never stop) would otherwise have a grip fire a launcher in the
  hands; in a real fight's Fire prompt with a launcher in the game's current slot the trigger still answers it. No campaign fight is known to come
  with a launcher in the current slot (Swordbreaker's cellar fight is after the AT4 is gone; the enemies' RPG-7s can't be taken up); if one is
  found, lift the bar for a fight's prompt alone (`if (!frame.prompt)` before `Bar`) and change t7's fight case.
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
  0.25 s, asks the use once. A prompt that gives a gun (the reload key's: the AT4's) is used only with no gun in either hand (the owner, 2026-11-19:
  "Rifle dropped on the floor first", then "Better to physically drop the rifle and pick up the AT"): with a gun held by its grip or its front, flying
  into a hand, or a take or a draw of one under way, no tick, and the squeeze held 0.25 s is refused with the double click and a line saying what to
  do (`Use: ... not used: the right hand holds the M4A1: drop it or put it on a shoulder first ...`); a door gives no gun, a gun in the other hand no
  bar there (opened with the rifle in the right hand). Something else's squeeze at its start is never the use's (`hands.cpp`'s `UseYields`: the hand at
  one of the body's places, a holster's draw or put-away; within 25 cm of a gun in a hand: its front, its magazine, its handle, a regrip; within the
  grasp's own reach of a gun lying); a use's squeeze that takes something else first (a magazine, a grenade, the gun's front) is given up. The squeeze
  begun in reach is the use's till let go (the grasp is told the hand is busy).
- `hands.cpp`: the decision each frame before the grasp, the request (one at a time), its outcome; the prompt's own key (R for the reload's action,
  held 300 ms with no gun in either hand) only where the game's use isn't as read, never after a refusal or a fault; a prompt on the reload key gives a
  gun: the first change of gun within 3 s of its use (the kit's: 0.7 s, measured), never one a pick of the player's or a draw from a place made, comes
  into the hand that used it, gliding in from where that hand was as it asked (its grip then, kept in the world), else the prompt's point, else the
  hand's own place (a pickup's arrival; `glidefrom 1` doesn't apply to it). That is anywhere in the reach, not where the tube lay (not measured: the
  roof's AT4 lies about 0.65 m off its prompt's point): a seated player's hand may ask a metre over the roof, and the tube then shows up at the hand,
  often already in it. On the roof the floor tube goes at about the kit change (0.1 s; the take's film has it between 0.38 and 0.75 s), the M4A1's last
  draw is at 0.41 s and the AT4's first at 0.69 s: nothing of the AT4 can be drawn in between, so from its first frame it comes from that hand's place
  into that hand, the take bridging the gap (the window holds it 3 s). A door gives none. `kUseCapsules` takes where the AT4 lies once it's measured
  (a census at the prompt: not made yet; the disc stands); its glide could then start there.
- `input.cpp`: X held with nothing in the hands at a reload-key prompt presses R as a pickup (no reload's side effects); with a gun in a free hand
  (or one coming into a hand) nothing at all, neither the prompt's R nor a reload, the double click and `Input: X held at the prompt on the reload key
  (it gives a gun): R not sent: ...` (before the grenade launcher's and the shotgun's X held, which press R too); X's tap's R for a gun the mod
  doesn't hold alike. Free hands dormant or off, or frozen in a scripted moment: R as before (the game's own use; no hand can drop a gun then), so
  the prompt is never out of reach.

**After the shot (B3)**: let go of, the spent tube falls as any gun let go of (its body, its bounces' knocks the hollow plastic's); its first landing is
the roof's own sound of an AT4 let fall (`drop_at4`, made on each player's PC from the level's own wave `sp03_sfx_rooftop_dropat_wave`: its clatter's
first strike, 4.04-4.26 s; chosen by the numbers, not heard).

**Fixed since the roof's run (at4_measured.md section 11, both seen at the take on main's DLL; each with a test that fails on the code before)**
- Recoil: at the take the drawn body was still the M4A1's while slot 0 held the AT4, and `recoilrun`'s set-up took the AT4's six kick rows out of its
  data under the M4A1's name for 0.91 s (its rows by their numbers never match in the game: FiringFunctionData's +0x15C isn't the magazine, so the
  "another gun's numbers" check never fired). Now the slot's gun by its name (`ammo::RowInSlot`, its blueprint's row) is set against the gun drawn
  (by its body, else its shape): two guns (`pickrules::OtherGun`, rows of the same numbers one) is a change of gun under way, and nothing of that
  slot's data is touched; the drift is off and the slot's count is no round of the drawn gun's meanwhile; set up again once what's drawn is the slot's
  gun, or once the slot's name is known (or another) a moment late. The train pistol's set-up is as before (its key's slot is 0 whatever slot it's
  counted in). Log: `Recoil: the M4A1 drawn while slot 0 holds the AT4 (a change of gun under way): nothing of that gun's data touched ...`; the
  set-up line now ends `the slot's by its name ...`. recoiltest's checks unchanged; recoilruntest 22 (11 checks: the roof's take both ways, a name read
  late, none known): 5 of them fail with the check left out.
- The held magazine: the AT4's one round under the M4A1 still drawn was read as the M4A1 run dry. Now `pickrules::SeeDry` decides each frame: a drop
  is the gun's own rounds only while the hands' slot holds the very firing state counted the frame before and, by its name, the gun drawn; another gun
  there by its name is no round (nothing read; afresh once it's the drawn one again), a new firing state of the gun drawn (the kit's fresh M4A1, a gun
  of its kind taken up) has its chamber as its count is. Log: `Held magazine: slot 0 holds the AT4 now, not the M4A1 drawn (a change of gun under way)
  ...`, and `Held magazine: another M4A1 in slot 0 (a new gun there, no round fired) ...`. at4test t11 (the roof's take frame by frame): 2 fail on
  the rule before.
  The slot's name is ammo's, read at the top of the Present with the slot's firing state, while the count and the firing state the rule compares are
  read in hands::Update after the headset's waits: a kit's swap landing between the two (about a third of the frame) had the AT4's one round read under
  the M4A1's name, a new M4A1 with one round, its chamber "empty" (the numbers review's finding 2: the old side effect under another line). Now a
  frame whose firing state isn't the one ammo read the name with (`ammo::SlotFiringRead`) decides nothing (`DrySeen::kWaiting`: the count and the
  firing state before kept), a second at most (a take's window holds ammo's read as it was), then as before. Left as before: a slot's name that fails
  its first read (ammo reads it again after a second) is taken as no name, one gun with the drawn one (recoil the same, recoilruntest 22j); harmless
  in the end, the kit's M4A1 clears it. at4test t11 (the swap a frame ahead both ways, a name never read with it; hands.cpp's lines word for word).
- The SMAW (Rock and a Hard Place, main's code): its scan took any exact match of its rocket's start, which is the AT4's, the RPG-7's and the enemies'
  RPG-7's too, and wrote the player's tube into them; and its start was never put back. Now `ClearLauncherSpread` takes only the SMAW's own firing
  data's (`at4shot::SmawOwn`), told by the first rule that takes any: (1) by its slot: the FiringFunctionData a weapon of the soldier's own points at
  (each slot's firing state's WeaponFiringData at +0x94, its PrimaryFire at +0x08, the way recoilrun reads every gun's rate of fire; Rock's slot 5),
  whatever its numbers, never one with a known other launcher's numbers; (2) by Venice-EBX's numbers for it (RateOfFire 350, ReloadTime 5.8: never
  read in this game: the numbers review's finding 1, the merge review's 3); (3) neither telling it, each match whose firing data is its class's and
  no known launcher's (the old rule, the AT4's and both RPG-7s' left out: the SMAW never starts from the game's eye where it started from its tube
  before). Each taken is held to its own numbers as read then, by its writer every frame and its putting back (`LauncherShotThere`), and
  `RestoreLauncherShot` puts its start back as its data has it 36 frames after the last write (out of the hands), as the AT4's; the frame it was
  written in is stored as read, never one ahead (the merge review's finding 2: a draw on another thread could store one past Update's count and wrap
  the difference). Every other match is said whose it is; none taken, said plainly. While it's in a hand nothing changes for it. at4test t10b.

## Not built

- **Hosting**: a thing lying is drawn only with its own gun's draw. The rifle let go of on the floor before the take is not drawn for the AT4's few
  seconds (the kit takes its slot and its game gun with it), then lies again where it lay (built: below, flow (b)); the spent tube on the floor, or in
  the hand, vanishes as the kit gives the M4A1 back (the game's switch takes it; the vanilla game drops none either). Drawing either meanwhile (the
  casings design's way: keeping the draw's state) is the next step, about a day.
- The cocking lever by hand (B4): the roof shows the vanilla game has no priming before the first rocket (the lever moves only inside its reload): a
  lever by hand would be the mod's own addition, for the owner to decide. No second AT4: the scene never waits after a miss.
- Not measured on the roof, so not in: its kick at the hip (VR's hold; all four shots were zoomed) and its return past 141 ms; where the AT4 lies (its
  capsule: no census at the prompt); a kHeldEyes row (at4_measured.md 4.6's {15987, 0.491, 0.611} would be unread: Sight() leaves the launchers out).
- For the owner to decide (the numbers review's finding 4): launchers never count as "the eye at the sight", so in VR the AT4's camera climb is its
  data's unzoomed row, 4 degrees a shot standing, with the measured 0.4 of the tube's own: about 4.4 degrees of rise after the rocket has gone. Fired
  through its sights on the roof (zoomed) the game rose 1.34 (the camera's 0.95, the tube's 0.39). A VR player always aims through the real sights;
  the sighted feel would be the zoomed row (1 a shot) chosen by the AT4's measured eye. Built as the measured note chose (the launchers' rule); it
  changes nothing of where the rocket goes (it's gone by then).
- Films: a scripted hand reaching and squeezing at the prompt, the AT4 in both eyes, the rocket along the sights, the trigger's stop, the kick and the
  buzz, the discard.

## The in-game checks (this branch)

1. The level start: `Pickups: scripted use points: the game's use of one and its call are as read: a hand's use of a prompt is the game's own`.
2. At the roof's prompt: `Use: the game targets a scripted prompt ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG (its key's action 15: R, X held, hold 0.00 s, 2.0 m,
   on, 0 of 1 uses; at (-456.14 94.19 -2013.60)): reach for it and hold the grip: the game's own use, called on the game's thread (it gives a gun:
   with no gun in either hand, the rifle dropped or put on a shoulder first)`. The two
   `ID_H_INT_SP_EARTHQUAKE_TO_PRONE` prompts, if the game targets them, say `not a hand's`, and a squeeze there does nothing of theirs.
3. An empty hand reaching to the tube (within 1 m of the point at floor height), the rifle dropped or holstered, ticks once (`Use: the left hand at the
   prompt ...: hold the grip`); with the rifle in a hand, no tick (check 15).
4. Rifle holstered or dropped (held in a hand: refused, check 15): the empty hand at the tube, the grip held a quarter second: `the game's own use asked (#1)`,
   `the game used the prompt ... the gun it gives within 3 s comes into that hand`, then `Use: the AT4 the scene gave 0.7 s after the prompt ... comes
   into the left hand (from where that hand was as it took it)` and `Free hands: the left hand took the AT4 the scene gave`; the AT4 first seen where
   the hand was as it squeezed (low at the tube if the hand reached down to it; at the hand, or already in it, if it squeezed higher up: say which),
   then in that hand, drawn in both eyes, the other hand on its front grip. A hand on the rifle's handguard, at a shoulder or at the belly while doing
   so: never the prompt's.
4b. At the kit's change, with the M4A1 still drawn 0.3 s: `Recoil: the M4A1 drawn while slot 0 holds the AT4 (a change of gun under way): nothing of
   that gun's data touched ...` and no `the M4A1's own kick taken out of the game's data: 4.00 up a shot`; `Held magazine: slot 0 holds the AT4 now,
   not the M4A1 drawn ...`, and neither `it's run dry` nor `another M4A1 in slot 0 (a new gun there, no round fired): 1 rounds loaded` (the swap a
   frame ahead of the slot's name is waited for); the M4A1's bolt not shown locked back in its last moment in the hand. Then `Recoil: the AT4 set up
   from slot 0 ...: its body; ... the slot's by its name AT4` and its own kick taken out (4.00 up a shot, -1..1, back 4).
5. `AT4: its rocket's start found in 1 places`; the aperture and the middle post line up on the target; the rocket leaves from the tube's front ring
   and lands where they point.
6. A grip squeeze never fires it. The trigger fires it once: a kick (the tube pushed 0.6 cm back over a tenth of a second, 0.4 degrees up, 0.3 cm
   lifted, with the data's camera climb), a strong buzz on both hands, `AT4: the rocket gone: the tube is spent`; pulled again: `the trigger didn't
   fire: the AT4 is spent`.
7. Let go: it falls and rolls, `drop_at4` on its first landing. The kit back: where the M4A1 comes is the merge's (below: at its shoulder if it was
   holstered or held, lying where it lay if it was dropped); the tube on the floor gone (no hosting yet); half a second later `AT4: out of the hands:
   its rocket's start put back as its data has it (1 place)`. No line taking the M4A1's kick out under the AT4's name; `Held magazine: another M4A1 in
   slot 0 (a new gun there, no round fired): 31 rounds loaded, its chamber as its count is`; it fires at once once in a hand.
8. X held at the prompt with nothing in the hands: R held, the game's own use (as before this branch); the AT4 arrives. With the M4A1 in a hand: nothing
   sent, the double click, `Input: X held at the prompt on the reload key (it gives a gun): R not sent: the right hand holds the M4A1: drop it or put
   it on a shoulder first ...` (check 15).
9. With the Mk11 in hand (slot 1) at the prompt: whether the kit raises the AT4 (the game's own matter, as with its R): say what the log shows.
10. A door anywhere (EIAInteract within 3 m): the free hand at it, the grip held: the door's scene, no gun given (`the scene goes on`).
11. Rock and a Hard Place, the SMAW (main's code changed): the scan's `Launcher: its rocket's start at ... is the SMAW's own (the firing data of the
   soldier's weapon in slot 5: RateOfFire 350, ReloadTime 5.8, Venice-EBX's)`: copy that line whole (its numbers are the first read of the SMAW's in
   this game; `not Venice-EBX's 350 and 5.8` is no fault, only news). Any other match `Launcher: a rocket's start like the SMAW's at ... is the
   enemies' RPG-7: left alone` (or the RPG-7's, the AT4's, or `another's (no weapon of the soldier's has its firing data)`); `its rocket's start
   found in 1 more places`. Lines that mean its slot wasn't read: `... is the SMAW's own (its firing data's RateOfFire 350, ReloadTime 5.8)` (by its
   numbers) or `none of the N rocket starts like the SMAW's is its own by a weapon of the soldier's or by its numbers ...` (the old rule): say so; and
   `none of the N rocket starts like the SMAW's is its own: nothing written` means its rockets start from the game's eye: say so at once. In the hands
   it fires from its tube's end as before; holstered or let go, half a second later `Launcher: out of the hands: its rocket's start put back as its
   data has it (1 place)`, and never that line while it's in a hand; drawn again, from the tube's end again at once. The enemies' rockets start from
   their own tubes.
12. A fist fight (Semper Fidelis, The Great Destroyer) as main's: the fights' lines, the guard holding up the other keys; no `with a launcher in a
   hand` line (no launcher is in a hand in a fight). Should a fight's quick-time Fire ever come with a launcher in the game's current slot, the grips
   press nothing there and the trigger answers it (kept so: B1's guard).

With holsters by hand (the merge, 2026-11-19; the roof's own save `sb_RPG` loads past the take, so these want `sb_SQUADATROOFTOPSAVE` and the crawl):
13. Flow (a): the M4A1 put on the right shoulder by hand (`Holster: ... put away at the right shoulder`), the AT4 taken with the left by reach and grip.
   Within the 0.6 s before the AT4 is drawn: `Holster: the game put the AT4 into slot 0 with the M4A1 still drawn (a change of gun under way):
   nothing decided on it till it's drawn (2 s at most)`, and no `into the right hand as at a spawn` line, nothing flashing in the right hand; then
   `Holster: the M4A1 stays at the right shoulder ... no pickup` (said a moment on) and the AT4 in the left. A squeeze at the right shoulder with the
   AT4 in the other hand: silent; with both hands empty after the throw and before the give-back: double click and `... it's there again once the game
   gives it back`. The give-back: `the game put the M4A1 into slot 0 with the AT4 still drawn` again, then `Holster: the M4A1 back in its slot 0
   (the game gave it back): its holster the right shoulder again`; never `the M4A1 back in the right hand`. Drawn at the right shoulder: in the hand at
   once, no key 1 pressed, 31 rounds, fires at once. Try it twice: once thrown before the kit gives it back (about a second after the shot), once
   holding the spent tube till it vanishes (`Free hands: the AT4 in the hands went with the game's own switch (the M4A1 back at its place): the hands
   are empty`).
14. Flow (b): the M4A1 let go of on the floor a little away from the tube (not under the reaching hand: that takes the rifle first), the AT4 taken:
   `Free hands: the M4A1 that lay on the ground went with the scene's AT4 (its kit took slot 0): it lies there again once the game gives it back` (it
   vanishes from the floor: say so if it doesn't). The give-back: `Free hands: the M4A1 back in slot 0 (the scene gave it back, N s after it went, N m
   from the eye): it lies where it lay` and `... lies again where it lay before the scene took it (loose #N)`; look: the rifle where it was dropped, at
   rest, not falling through the roof; the tube gone. With the tube thrown before the give-back: a squeeze at the left shoulder (the Mk11's) draws
   nothing, `Holster: nothing drawn from the left shoulder: the M4A1 lies on the ground: take it first`, the rifle still on the floor. Taken off the
   floor by hand (in the hand, on no shoulder), put on the right shoulder by hand. A `couldn't be laid where it lay` line means it came into the right
   hand instead: copy it whole.
15. The M4A1 held in a hand at the tube (the owner's decision: "Rifle dropped on the floor first"): no tick at the tube; the other hand's grip held a
   quarter second there: a double click and `Use: the left hand holds the grip at the prompt ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG: not used: the right
   hand holds the M4A1: drop it or put it on a shoulder first (a prompt that gives a gun is used with no gun in either hand)`, nothing taken, the M4A1
   still in the hand; X held there: the same double click, `Input: X held at the prompt on the reload key (it gives a gun): R not sent: ...`, no reload
   either. Then put on the right shoulder (or let go of on the floor) with the other hand still at the tube: a tick at once; squeezed again: the use
   (check 13, or 14).
16. X held's R at the prompt with nothing in the hands instead (both flows): the AT4 into the right hand; (b)'s `went with the scene's AT4` line said
   as well.
17. Flow (b) through a death: the M4A1 dropped, the AT4 taken, a miss (or the window let run out): after the respawn `Free hands: the M4A1 that lay on
   the ground before the scene took it is laid there again once the scene gives it back, after this new life too (its level's checkpoint: within 6 m
   of the eye then)`; after the hit in the new life the M4A1 lies where it was dropped in the first (check 14's lines). An `isn't laid there again`
   line instead: copy it whole.
18. A quick squeeze at the tube, let go before the AT4 shows up (0.7 s): the AT4 comes into that open hand and stays there, as a gun at a spawn does,
   not dropped at the feet; squeezed and let go then: it falls.
19. Anywhere, a D-pad or Y switch between two guns: no `Holster: the game put the ... into slot N with the ... still drawn` line (said only for a gun
   put into the very slot drawn: the roof's kit, a take).

## Merged with main at 620ba08 (the fist fights' QTE reader d07768f, the M249 620ba08)

- The quick-time guard as one rule with the fights' guard (above, B1): qtelist.h is main's (its `Awaited` with the fights' `Skip`); the branch's
  `barred` moved to `qtewatch::Bar`, applied after `qtefight::Step` to a live trigger's keys and a fight's prompt alike; `qtewatch::Update(scripted,
  fights, barred)`; input.cpp's QteStep works out `barred` once for both versions.
- gunprofile.h: the M249 keeps row 14, the AT4 is row 15 (`kAt4Row`); profiletest p3b and at4test t1 say so.
- The M249's rows next to the AT4's (M249 first) in skin.cpp's kHeldKinds, shaperecipes.h, gunbodyfit.cpp and gunbodies.h (as gunbodyfit makes it
  from the meshes: --check 0 of 1856 off), learntest, picktest, profiletest, CMakeLists (at4test and m249test); recoilgen -Check passed on the merge.

## Merged with holsters by hand (main 7e99b44, 2026-11-19)

The owner, 2026-11-18: "Don't let guns get automatically put into holsters if they're picked up from the ground.. it needs to be physically placed on
the shoulder to lock as a permanent gun primary or secondary unless it is physically removed from the holster and dropped"; on the AT4: "a smart
player would holster their rifle before picking up the AT4, but they could momentarily drop their rifle on the floor, pick up the AT4, shoot it,
discard it, then pick up their rifle again".

**The merge's resolutions**
- `ammo.cpp`/`ammo.h`: both new readers kept side by side (this branch's `SlotFiringRead`, holsters' `SlotIdentity`).
- `hands.cpp`: the use point's globals and holsters' X-held raise globals both kept; after `holster::Update` holsters' take marks cleared and their
  give-back asked first, then the use by hand (`UseFrame`: it reads that frame's holster result for `UseYields`); at a take's arrival the scene's gift
  (`byUse`) keeps its own lines ("gone with the scene's kit", "took the AT4 the scene gave") and holsters' draw-back keeps its ("from its shoulder"),
  a plain floor take's words holsters' ("on no shoulder, the D-pad or Y brings it").
- `input.cpp`: R let go keeps both (`g_pickupRLetGoAt` after X held's pickup, and the prompt's R flag cleared); X held with nothing in the hands: a
  dropped gun's take that couldn't give a holstered gun back sends nothing (`take == 1 && HolsteredSwap()`), else R for a dropped gun (1) or, put
  away or not, for a prompt on the reload key (2).
- `tools\pickruntest.cpp`: holsters' give-back row renamed R19 (with its checks' row name), the use point's stays R18.
- `qtelist.h` is the same on both sides (no conflict: its `Awaited` takes the fights' `Skip` already). `build\bodytest_points.bin` regenerated
  (`gunbodyfit build <temp header>`: the points the same as before, the header the same as `src\gunbodies.h` but for its line endings; that file not
  rewritten).

**What the two needed of each other (built here)**
- The gift's slot marked a take's (`g_holsterTaken`, in `OnGunChanged`'s gift block, before the take's `Arrive`): the AT4 has no holster (in the hand
  that took it, on a shoulder only by a hand, never the kit's even in a life's first seconds); the gun before keeps its shoulder with the game (a take's
  mark with no swap told for its slot is no pickup's: main's 365b04b).
- The roof's timing, which the merge's rows had in one frame: the kit's AT4 is in slot 0 0.6 s before its first draw, and the kit's M4A1 back 0.9-1.3
  s before its (at4_measured.md sections 3 and 8). Holsters noted the new gun with the slots meanwhile, so (1) with the M4A1 put away, the new gun in
  its slot with no home was drawn into the right hand (holster.h's rule for a put-away gun with no home: the M4A1's last draws shown in the right hand
  for a third of a second, the AT4 then gliding into the left); (2) the spent AT4 lying took the slot's new gun's holster with it (the rule "a gun on the
  ground has no holster" went by the slot, not the gun), so the kit's M4A1 came into the right hand with no holster; (3) the give-back's window was
  1 s, shorter than the draw's lag. Now: `holster::Frame::drawnOther` (hands.cpp: the game's slot holds another gun by its name than the one in the
  hands, both known, 2 s at most) is a moment for holsters (no put-away, no draw, no put-away gun drawn into the right hand) and a gun let go of then is
  no gun of that slot's; the lying gun's holster goes by its own identity (`State::lyingId`, `lyingKind`); a give-back's window holds till its draw
  (`kArriveMost`, 3 s). Each with a holstertest row that fails without it (HT1, HT1b, HT2, HT3, HT5).
- A holstered gun the game gives back while the hands still hold the spent AT4 (the kit's M4A1 1.3-3.3 s after the shot) is at its shoulder all the
  same (`holster::Arrive`, kAtPlace; a pick just before: today's); the AT4 goes with the game's switch, the hands empty, said: `Free hands: the AT4 in
  the hands went with the game's own switch (the M4A1 back at its place): the hands are empty`.
- Flow (b): `pickrules::LyingTaken`/`LyingBack` (pure) and hands.cpp's record (`LyingGone`): at a scene's gift (a use by hand, or X held's R at the
  prompt: `input::PromptRSince`) taking the slot of the gun that lies, where it lay is kept; when the game's gun of its kind is back in that slot by the
  scene (no take, no pick of the player's, within 6 m of the eye, 120 s, the same life) it lies there again: the grasp kLoose, laid down still by
  `loose::Place` (no hand's, gun's or walk's speed), never into a hand or onto a shoulder by itself. Otherwise (a take, a pick, another slot, too far
  or not known, a new life, too long, a pickup's swap of its slot told just after): as any change of gun, said why. No room or no body for it: back in
  the right hand (as a lying gun the ground lost), never nowhere.

**The flows as built (what the player sees and gets)**
- (a) The M4A1 put on the right shoulder by hand (its holster). An empty hand reaches to the tube (a tick), holds the grip a quarter second: the game's
  own use. The kit puts the AT4 in slot 0; 0.7 s later it glides into that hand from where the hand was, on no shoulder; the M4A1 is "with the game"
  at the right shoulder (`Holster: the M4A1 stays at the right shoulder ...: the game took it out of its slot with no pickup`). A squeeze there
  meanwhile with both hands empty: refused, double click, "it's there again once the game gives it back"; with the AT4 in the other hand: silent (one
  gun in the hands). Fired once; thrown (it falls, `drop_at4`). The kit gives the M4A1 back (31 + 372): it's put away at the right shoulder (`Holster:
  the M4A1 back in its slot 0 (the game gave it back): its holster the right shoulder again`), the thrown tube gone from the floor; still holding the
  tube then: the tube vanishes from the hand, the M4A1 at the shoulder all the same. Drawn by hand at the right shoulder at once (the game's own weapon
  put away: no key), with the kit's 31 rounds, its chamber as its count is (at4test t11).
- (b) The M4A1 let go of on the floor (it lies, no holster any more: "physically removed and dropped"). The AT4 taken the same way (a hand near the
  lying rifle takes the rifle first: the thing under the hand goes first). The rifle on the floor vanishes as the kit takes its slot (`Free hands: the
  M4A1 that lay on the ground went with the scene's AT4 (its kit took slot 0): it lies there again once the game gives it back, nothing of it drawn
  meanwhile (no hosting)`). Fired, thrown (or still held). The kit gives the M4A1 back: it lies again exactly where it lay (`Free hands: the M4A1 back in
  slot 0 (the scene gave it back, ...): it lies where it lay, never into a hand by itself`; `... lies again where it lay before the scene took it (loose
  #N): take it off the floor by hand`), the tube gone; nothing at the right shoulder. Taken off the floor by hand (the magnet too): in the hand, on no
  shoulder; put on the right shoulder by hand: its holster again. Never lost, never two.
- (c) The M4A1 held in a hand at the tube: refused (settled below: the owner's decision, "Better to physically drop the rifle and pick up the AT"). No
  tick, the double click after the quarter second, a line saying to drop it or put it on a shoulder first; X held the same. Put on the shoulder: (a);
  let go of on the floor: (b).
- X held's R at the prompt with nothing in the hands instead of the reach and grip: the same, the AT4 into the right hand (X held's pick).

**Settled after the two reviews of e403947 (2026-11-19)**
- Both reviews' high finding, the owner's decision (controls-and-holsters, 2026-11-19 ~00:00: "Rifle dropped on the floor first", then "Better to
  physically drop the rifle and pick up the AT") not built: built. `usepoint.h`: a prompt that gives a gun (`Point::gives`, the reload key's) with a gun
  in a hand (`Frame::gunHeld`: hands.cpp's `GunInAHand`, by its grip or front, flying in, a take or a draw under way) has no tick, its squeeze held
  0.25 s is refused (`Out::refused`) with its own double click (`Refuse`, holster.h's pulses, so it's felt with "holsters 0" too), the squeeze still
  the use's till let go; a door is unchanged. `input.cpp`: X held with a gun in a free hand sends nothing at that prompt, before the grenade
  launcher's and the shotgun's branches (they press R too); with nothing in the hands, a gun coming into one bars it; X's tap's R alike
  (`hands::GunBarsGift`, `RefuseGift`). Free hands dormant or off, or frozen in a scripted moment: the game's own key as before, so the prompt is
  never out of reach. holstertest HT3
  (now the refusal, then the use once the rifle is on a shoulder or on the floor), at4test t6, t12.
- Both reviews' second finding (medium), the stale lying record after the lay-down: `g_holster.lyingSlot = -1` in the lay-down, so holsters read the
  M4A1 itself (not the thrown AT4); the left's Mk11 is refused while it lies. HT4's left shoulder, HT4c (the control: key 2 without it).
- The first review's third finding (medium), flow (b) through a death at the "RPG" checkpoint: the record is kept through a new life on its level,
  its 120 s from that life's start; a gun of its kind within 10 s of a new life (its own kit: a load of an earlier save) ends it, as at a spawn;
  another level's kit (holsters' `memoForgot`) ends it; the 6 m stands. HT7, HT7b, t12.
- The first review's fourth finding (medium), the scene's AT4 dropped when the grip was let go before it came: `grasp.h`'s `Frame::arriveUnarmed`
  (hands.cpp: a gift's arrival), armed only by a grip squeezed then, as a pick's draw; grasptest 84 (with the control that drops).
- The second review's third finding (medium), a rifle in a hand between the use and the AT4's draw: impossible at the use (the bar above), and closed
  for the 0.7 s after it: while a gift is on its way (`GiftOnItsWay`: a gift's use asked or made, its gun not in the hand yet; X held's R at the prompt
  with no change of gun since) holsters draw nothing (`Frame::pendingTake`) and the grasp takes nothing (its hands busy). HT8 (with its control).
- Lows fixed (cheap, and the in-game checks read the lines): a pick ends flow (b)'s record only for its own slot within 2 s of the give-back (the first
  review's fifth); a dropped gun of another slot at the gift (a Mk11) is said to go back into its slot, not "gone with the scene's kit" (its sixth,
  the second review's); `the X back in the right hand` names the gun in the hand now (the second review's); the change-of-gun-under-way line said
  only for a gun put into the very slot drawn, not on every D-pad switch (the second review's).
- Left open, low: the dropped Mk11 itself (above: in its slot, the D-pad or Y brings it, never laid on the floor again: no hosting); a weak "with the
  game" record for a gun that came by a take or a gift (a spent AT4 put on a shoulder, the bridge M249) never ends when its slot is refilled with
  another kind (its line false, nothing blocked); a dropped rifle within 25 cm of the squeeze at the tube takes the squeeze (the thing under the hand
  first: needs the tube's capsule, a census at the prompt); X held's take (and a hand's whose AT4 is drawn over 1 s after its slot) in a life's first
  10 s gets the kit's launcher holster (the roof's take is never that early: the crawl first).

**Still open (for the owner)**: the far draw of a holstered gun put into a pickup (the game's own 2.5 m reach) and the rest of 7e99b44's open list; a
spent AT4 put on a shoulder by hand and left there as the kit gives the M4A1 back is remembered "with the game" (a squeeze behind the neck then says
so; nothing else of it); hosting (drawing the dropped rifle while the AT4 is out, the spent tube after the give-back). Whether the refusal's words and
its double click are what the owner wants for X held at the prompt with the rifle in hand (the game's own prompt still says to hold the key).

## For the changelog at release

The AT4 drawn in the hands (both eyes), one rocket from its tube's end, its own kick as measured, no reload by itself, a spent tube thrown away; a
scripted prompt on the interact or reload key used by reaching for it and holding the grip (X held still works), the AT4 rising from where the hand
took it; the left mouse button never pressed by the quick-time grips while a launcher (the SMAW, the Stinger, the AT4) is in the hands. The SMAW's
rocket's start written in its own data alone (the enemies' RPG-7s start from their own tubes again) and put back when it leaves the hands. A weapon
the game swaps into the hands' slot never read as the gun drawn running dry, nor its kick taken out under that gun's name. With holsters: the AT4 is
taken with both hands free of guns (the rifle put on a shoulder or dropped on the floor first; with one in a hand, a double click says so); the rifle
holstered stays at its shoulder through the AT4 and is drawn from there after; the rifle dropped on the floor lies there again after the scene, a
death at its checkpoint too; a change of gun the game makes in the hands' slot decides nothing of the holsters till it's drawn.

## Tests

- `tools\at4test.cpp` (52 checks): t1 the tables; t2 the mesh's bones and front ring; t3 its sights (the aperture's ring, the middle post, the line's
  angles, the sights point at the muzzle, the game's zoomed camera on the line); t4 the palms in its recording; t5 the sights' turn (bit for bit the
  raise as built before, with right 0); t6 the use by reach and grip (which prompts, the reach, the AT4's disc, the edge's tick, the hold, a gun in the
  other hand at a door; the AT4's prompt with a gun in a hand refused with the double click, the tick once it's let go of, the bar as the hold ends, a
  refusal made outside Update; a squeeze something else wants, one given up, nothing, both hands); t7 the quick-time guard (`qtewatch::Bar` on a live trigger's keys
  and on a fist fight's prompt, with the fights' guard: the trigger's left button through, every other key held up; nothing barred bit for bit); t8
  its holster's place and the refused key; t9 the game's use's code against `bf3_mem.exe` (13 pieces of code, 4 runs of bytes, 6 words; each row word
  for word in `pickups.cpp`); t10 its firing data against the roof's dump (its own; never the SMAW's, the RPG-7's, the enemies' RPG-7's, another
  class; its start as last written); t10b the SMAW's (the roof's two matches the AT4's and the enemies' RPG-7's by their GUIDs, both taken by the scan
  before, never the SMAW's by its numbers; `at4shot::SmawOwn` in ten cases: by a weapon of the soldier's whatever its numbers, Venice-EBX's or not,
  else by its numbers, else none of the known others'; a slot holding RPG7_AI's data, unread data, another class, the AT4's and both RPG-7s':
  never; two with the SMAW's numbers: the slot's alone; the rule before losing a SMAW not numbered as Venice-EBX has it; each taken held to its own
  numbers; skin.cpp's lines word for word, the frame stored never ahead); t11 the take's side effects (`pickrules::SeeDry` and `OtherGun` frame by
  frame; each swap a frame ahead of the slot's name, as the rule before read it; a name never read with it; hands.cpp's lines word for word); t12
  the merge's flow (b) rules (`LyingTaken`, `LyingBack` case by case, through a death on its level, each end said why) and hands.cpp's and input.cpp's
  lines word for word (the bar, X held's and X's tap's refusal).
- `tools\holstertest.cpp` HT1-HT8 (220 checks in all): the flows end to end with both together (the use by reach and grip on holsters' frames, the
  gift as hands.cpp gives it, the roof's timing: the kit's AT4 0.6 s before its draw, its M4A1 1.3 s before its): (a) holstered, thrown before the
  give-back and in its gap, drawn early, drawn by hand after; the tube still in the hand; the M4A1 in a hand at the tube, with and without a holster:
  refused, then the use once it's on a shoulder or on the floor (HT3); (b) dropped, laid again, the left's Mk11 refused while it lies (HT4c its
  control), taken off the floor, holstered by hand; (b) through a death at the "RPG" checkpoint (HT7, HT7b); X held's R for both; nothing into a hand
  between a use and its gift (HT8, with its control); hands.cpp's lines word for word. HT1, HT1b, HT2, HT5 fail on the merge's holster.h alone.
- `tools\grasptest.cpp` 84: a scene's gift into an open grip unarmed, kept as at a spawn, armed once squeezed; into a squeezed grip armed; the control,
  a take's arrival into an open grip, let go of (the review's drop).
- `tools\loosetest.cpp` g10: `loose::Place` lays a gun still where it lay (at rest again within 0.5 s, within 5 mm and 1 degree, none of a moving
  gun's or the walk's speed, carried by a metre's origin), broken numbers refused.
- `tools\pickruntest.cpp` R20: the roof's prompt with a dropped gun near through pickups.cpp itself (GameWouldTake 2, no pickup's target: holsters'
  take check never asked); the use served, the kit's AT4 into slot 0 and its M416 back: no swap told either time (no pickup's), the dropped gun as
  it was, the prompt used up.
- `tools\recoilruntest.cpp` 22 (11 checks): the roof's take and the kit's return against recoilrun.cpp itself (the slot's gun by its name: never its
  data under the drawn gun's name, no round of it, put back; a name read late; none known as before).
- `tools\pickruntest.cpp` R18 (25 checks): published, served with a stand-in for the game's use, each refusal on either thread, no hand's prompts
  (the roof's pose prompts, a hold, 300 m, a jump), a door's, a stale tick after a served use, a use under way, one not taken in 1 s, `use 0`, a use
  and a take never in one tick, the game's own checks refusing, a fault (never called again, nor its key), the checks not as read.
- Changed: bodytest (a tube's ring has no keel: b6's keel rule and b4's 4 s are a flat bottom's), learntest (the learner refuses the AT4's body, as it
  must a tube's; it's measured, never learned), recoiltest (the AT4's buzz the SMAW's; 14c the fold with a right, against a frozen copy of the fold
  before), profiletest (p3b: row 15), picktest, shapemaketest's table (its recipe). Every offline test program passes (45 with main's m249test; never
  playtest or xrtest), with gunbodyfit --check and recoilgen -Check; again on the merge with holsters by hand (all 45, 2026-11-19), and again with
  the two reviews of e403947 settled (all 45: at4test 52, holstertest 220, grasptest 618; without the bar, t6's two rows and HT3 fail).

# The mounted gun: Swordbreaker's truck .50 and Uprising's Humvee M2 in VR

The owner, 2026-11-18, on the end of Operation Swordbreaker ("Get on the 50 cal"): "held with both hands ... two grips
held ... together and close ... move both of their arms ... at the same directions, like mirroring each other, so it can
move the machine gun". The same rule holds for Uprising's Humvee .50 (PLAN.md item A6, Uprising item B2).

This note is lane A6's design, written 2026-11-25 from the public Venice-EBX dump and the mod's code on main (85d59d2).
**Nothing here was run in the game.** Tags: [data] read from the dump; [code] read from the mod's source; [inferred] my
reading of the two; [measure] what the runbook (section 6) must settle in the game before a line is built on it.

**Reviewed and corrected, 2026-11-25.** An independent review found 14 problems (2 high, 5 medium, 7 low); each is answered
in place below and listed in section 10. The owner's three questions (section 8) are settled. Step 1's parts that need no
game are built and tested offline (section 5): `src/mountgun.h` (the law), `src/seatread.h` (the seat's reader),
`src/mountseat.cpp` (the seat's one state, the gates' source), `tools/mountguntest.cpp` (the review's broken laws among its
checks, each shown failing). A second review, of that build (1e2db2b), found 1 high, 4 medium and 8 low problems and a nit:
the steering's guards missed small pushes (a read that doesn't follow the turret ran it away, a wrong sign on a small first
move ran the pitch into its stop), a guard's stop left a dead gun, yes-or-no rows passed whatever they said, and a seated
torso's turn parts the hands from the handles on the truck too. Each is answered in place and listed in section 10b; the
test now has 77 checks. **All of it is off**: every number it needs is an unmeasured row (section 11), the first in-game run
of the runbook (2026-11-25, "fifty") did not reach the truck, and the log says once a run which row is missing. On foot
nothing changes.

## 1. In short

- **The game seats the player as a vehicle's second entry** (entry order 1, the "Humvee_Gunner" animation) on a turret
  of two rotation bodies: a ring turned by the input action `EIARoll` (mouse X, the pad's right stick X) and a gun
  turned by `EIAPitch` (mouse Y, right stick Y). Both are **rate-driven bodies with momentum**, not angles anyone writes.
  The camera rides the gun's pitch body. The soldier's own look angles are locked to the seat and do not aim anything.
  [data]
- **The two guns are opposite shapes.** On Uprising's Humvee the ring's axis stands 0.18 m in front of the gunner's
  eye: turning the gun is the gunner turning in place, the gun swinging round him. On Swordbreaker's truck the axis is
  the gun's own post, 0.63 m in front of the eye: the gun pivots in front of the gunner like a tiller. [data] One rule
  covers both (section 4.4): **the gun is put where the two hands put its handles**, whatever the shape.
- **The player's body stays fixed to the vehicle; only the gun moves.** The head is free, the horizon level, the eye at
  the gunner's place. The game's own camera, which swings with the gun, is never the view (it would turn the world with
  the player's hands). Bigger turns than the arms can make go on the right stick, as on foot.
- **The game's turret is steered by its own input**, the way its own controls turn it: each frame the mod compares
  where the hands put the gun with where the game's gun points (read off the game's camera, which looks along the
  barrel) and sends the difference as right-stick or mouse input. Nothing of the turret is written in memory: the game's
  server simulates the turret from the input, so shots land where the gun is drawn. [inferred, measure] The right
  stick's own turn never goes through the mouse in the seat (it would turn the ring: section 3).
- **Triggers fire, from a hand on a handle.** Either hand. Nothing reloads (endless ammunition, the game's overheat is
  the limit). The zoom is never pressed (it would magnify the headset's picture).
- **Everything else is quiet in the seat**: the look angles' search and writes, mouse aim, holsters, stance, jump,
  grenades, magazines, the quick-time grips, walking keys.
- **Two steps.** Step 1 (*finish*, M): the seat found, the fixed body and eye, the gun turned by the grips with the
  game's own arms drawn on it, the right-stick turn, the triggers firing, the rest quiet. Step 2 (*full*, L): the gloves
  drawn on the handles from the player's own shoulders, each hand taking and leaving its handle by reach, the buzz a
  round, the seat's crosshair hidden.
- **First in the game: section 6's runbook.** Four things decide the build and are not in the dump: how today's DLL
  behaves in the seat, where the seat is in memory, how the turret answers input, and where the game draws the arms and
  handles.

## 2. What the game does (from the dump)

### 2.1 The two guns

| | Swordbreaker (`SP_Earthquake`) | Uprising (`SP_Earthquake2`) |
|---|---|---|
| Vehicle | `Vehicles/technicaltruck/TechnicalTruck_Restricted` (team 2, spawned by an event) | `Vehicles/HumveeArmored/spec/HumveeArmored_hmg` inside `Prefabs/Logic/HumveeStagingArea` |
| Where | `MeatMarketLogic` spawn EB4B7D4C at (-610.41, 78.44, -2091.80), facing -x | the prefab at (126.44, 59.28, -68.18); the seat's prompt at (125.31, 60.44, -67.11) |
| Objective | `ID_H_OBJ_SP_EARTHQUAKE_GET_ON_50_CAL` | "Enter gunner seat", then "HOLD OUT FOR OSPREY" |
| Getting on | the vehicle's own entry: the use key (E, `EIAChangeVehicle`) within 5 m of the gunner's entry; no enter restriction | a scripted use point: E within 1.5 m, any angle (`ID_H_INT_SP_EARTHQUAKE2_ENTER_GUNNER_SEAT`), then the player is moved into seat 1 standing; a plain E at the Humvee itself should seat nobody (its spawn allows the primary entry only, and that entry's radius is 0) |
| Climb-in | an in-vehicle scenario (asset 357064800, scenario 3, actor 7) on the gunner's entering | the same asset, scenario 47, plus the door's animation; 4.33 s until "ScenarioFinished" |
| Time on the gun | about 19 s of fighting, then the earthquake; pulled out at about 29 s | 75 s or more (see 2.9); the survey's 50 s took the Osprey's start for the rocket |
| Getting off | locked; the game pulls the player out (`LeaveCurrentVehicle`, no exit point, physics off) into the end scene | locked; the game's rocket sequence: immortal, fade out, moved out by an exit point, Humvee destroyed, fade in, a get-up animation |
| Weapon | `Mounted_TechnicalTruck_Firing` | `Mounted_M2HB_Firing` |

The catalog's other mounted .50s (the Abrams roof in Thunder Run and Fear No Evil) wait with those missions; the seat
reader and the rules below are written to take them as rows later.

### 2.2 The turret's shape

Positions in the vehicle's frame (y up, z forward, x toward the driver's side: the Humvee's driver sits at x +0.69 and
its front-right passenger at -0.76), every rotation at rest, in metres. [data, summed along the component tree]

| | Truck (restricted technical) | Humvee M2 |
|---|---|---|
| Ring axis point (yaw body) | (0.008, 1.851, -1.414) | (-0.001, 1.915, -0.114) |
| Gun pivot (pitch body) | (0.008, 2.024, -1.438): on the ring's axis, 2.4 cm behind it | (-0.011, 2.410, 0.682): 0.80 m in front of the ring's axis |
| Camera (the game's eye) | (0.008, 2.226, -2.046): 0.61 m behind the pivot, 0.20 above; 0.63 m behind the axis | (-0.011, 2.678, -0.296): 0.98 m behind the pivot, 0.27 above; 0.18 m behind the axis |
| Rounds leave from | (0.006, 2.112, -0.225): 1.21 m ahead of the pivot, 9 cm above it | (0.002, 2.441, 1.668): 0.99 m ahead of the pivot, 3 cm above it |
| Known hand point | the left hand's IK target (0.147, 2.056, -1.745): 0.31 m behind the pivot, 0.14 m to the driver's side, 0.30 m in front of the eye and 0.17 m under it | none in the data: the gunner's animation holds both handles [measure] |
| Gunner's entry (the soldier's base) | on the ring, its offset cancelling the ring's: the vehicle's origin, turning with the ring | on the ring at its axis, 1.30 m up: he stands in the hatch and turns with the ring |
| Yaw | `EIARoll`, no limit (the data's -65/65 is switched off), held still in the world when the truck turns (`WorldSpaceLockEfficiency` 1) | `EIARoll`, no limit, turns with the hull (0) |
| Pitch | `EIAPitch`, -20 to +20 degrees, held in the world (1) | `EIAPitch`, -35 to +35 degrees (0) |
| Bodies' motion | mass 100, inertia 1, `AngularMomentumMultiplier` 125, `ForceModifier` 250, damping 0.2 (yaw) and 0.32 (pitch), acceleration and deceleration damping 0.1, linear input; the pitch also an angular momentum threshold of 120 | the same numbers |

What the shapes mean for the hands [inferred]: on the Humvee the ring turns about a line just in front of the gunner's
face, so the M2 swings round him and its handles stay in front of his chest: to aim left he turns left, hands and all.
On the truck the gun turns about its post 0.3 m in front of the hands: to aim left the handles go right. A real gunner
walks round the post; the game turns him with the ring. Neither is a choice for us to make: it is where the handles
are, and section 4.4's rule follows it for both.

### 2.3 The camera

- A `StaticCameraData` from `Vehicles/common/1pOpenGunner_CameraConfig` on the gun's pitch body (positions in 2.2), so
  it turns and tilts with the gun and looks along it: the camera's forward is the barrel's direction, the rounds' line
  parallel to the camera's and 11 cm under it on the truck, 24 cm under it on the Humvee (2.2's heights). [data]
- No free look of its own: its yaw and pitch angles are 0; mouse or stick input only leans it a few centimetres
  (`LeftCurve` 0.1 m, `UpCurve` 0.05 m) through `EIACameraYaw`/`EIACameraPitch`, averaged over 10 frames. [data]
- Field of view 60 (truck) and 55 (Humvee); the zoom (`AlternateView`, right mouse button) narrows it to 22 and cuts the
  turret's turn input to 0.3 (truck) or 0.4 (Humvee) of itself. Uprising tells the player to use it ("ZOOM GUN", 3 s
  after the climb-in). [data]
- `SoldierAnimatedCamera` on (0.5 s) on both gunner cameras: the camera may follow the soldier's animated head bone, at
  least in transitions. Whether it stays rigid on the pitch body in play decides how the gun's pivot is found
  (section 6, R4). [measure]

### 2.4 The aim the game reads

- The turret's two angles, integrated from `EIARoll` and `EIAPitch`. `LockSoldierAimingToEntry` is set on every seat:
  the soldier's own look angles (the five slots aim.cpp writes) follow the seat and aim nothing. [data]
- In the campaign the game's server runs inside bf3.exe (god.cpp's finding) and simulates vehicles from each frame's
  input as the client does; rounds start from the server's turret. So the turret must be turned **by input**: a turret
  angle written in the client's memory would be drawn but would not shoot there. [inferred from the engine's design;
  section 6, R5 checks that rounds land where the drawn gun points]
- Rounds start at the weapon component's point (2.2) along the pitch body's forward, 600 m/s (the firing data's shot
  setting; 350 is the bullet's own default, which the shot overrides: the review), spread 0.3 to 1.0 degrees (+0.4 a
  round, -5 a second). [data]

### 2.5 Input in the gunner's seat (PC, `Input/VeniceGunnerInputConcepts`, root map)

| Concept | Action | Keyboard / mouse | Pad |
|---|---|---|---|
| ConceptRoll | `EIARoll`: the ring | mouse X | right stick X |
| ConceptPitch | `EIAPitch`: the gun | mouse Y | right stick Y |
| ConceptFire | `EIAFire` | left button | right trigger |
| ConceptZoom | `EIAZoom` | right button | left trigger |
| ConceptReload | `EIAReload` | R | X |
| ConceptChangeVehicle | get off | E | B |
| ConceptInteract | use | E | Y |
| ConceptToggleCamera | camera | C | right stick click |
| ConceptFireCountermeasure | (none on these guns) | X | Y |
| ConceptSelectInventoryItem1-3 | vehicle weapons | 1, 2, 3 | D-pad |
| ConceptSelectPosition1-8 | change seat | F1-F8 | |
| ConceptMoveFB, ConceptYaw | throttle, yaw (nothing on a gunner's seat) | W/S, A/D | right trigger, left stick X |

Both gunners have the same pad curves (`PadInputCurve_OpenGunner_Roll`/`_Pitch`, and `MountedCamera_Yaw`/`_Pitch`): a
dead zone to 0.15 of the stick, then 0.03 at 0.25, 0.066 at 0.75, 0.125 at 0.875 and the most, 0.2, from 0.95. (The
first reading gave the Humvee none: that came from a duplicate entry, 05C54AEA, whose parent 8009E143 is `Excluded True`;
the active gunner entry 231BD75C on the ring C72E0337 carries the truck's curves. The review's third finding; a component
tree's sums must skip excluded parts.) The game's own pad dead zone comes first (`GstInput.Deadzone`, 0.30 on the owner's
PC), so the stick moves nothing until about 0.4. The mouse's speed is the player's vehicle mouse setting (1.48 on the
owner's PC), and each control scheme may invert the vertical (`GstInput.Scheme0-3FlipY`, `FlipCameraY`). [data]

### 2.6 Fire, heat, reload

Both guns: 550 rounds a minute, automatic, endless ammunition (`MagazineCapacity` -1), no reload in practice; heat 0.075
a round, cooling 0.5 a second, a 1.2 s penalty once hot, `OverHeatThreshold` 0.01 (a copy of the heat needs it too: the
review). At 550 a minute the heat climbs about 0.19 a second: about
5 s of fire to overheat, then about 3 s silent. [data] The game's HUD shows the heat. There is no charging handle in
the game's own handling of either gun; vanilla first, so none by hand.

### 2.7 The hands the game draws

- `Show1pSoldierInEntry` on, `ShowSoldierWeaponInEntry` off: the gunner's first-person soldier is drawn and his own
  rifle is not. His pose is the entry's animation (`AntEntryIdEnum_Humvee_Gunner`, both guns). [data]
- The truck adds an IK target for the left hand on its gun (2.2); its turret's angles do not reach the soldier's
  animation (`SoldierBaseIndex` 0, no rotations). The Humvee's turret feeds its yaw and pitch to the soldier's animation
  (`SoldierBaseIndex` 1, rotations Y and X), so his arms follow the M2's tilt. [data]
- Both levels' arms are `arms1p_bareglove03` (Swordbreaker's, Rock's and Uprising's gloves): if the seat draws the same
  first-person mesh, skin.cpp's glove draws are the ones it already tells apart. [inferred, measure]
- Which pass draws them is not in the data: the first-person views (the arms' space the mod re-poses on foot) or the
  world's. Nor which draws are the guns and their handles. [measure, R6]

### 2.8 The scenes round the seat

- Climb-in: an in-vehicle scenario with the player as the actor (both missions). The mod's rule for scenes holds: the
  game's body as it is drawn, locked, per eye, the head free, no hands of the player's.
- Uprising's restrictions: from the press, fire, get-off, seat change, stance, weapon change and the grenade are off;
  at the climb-in's end fire comes back and only get-off, seat change and stance stay off. [data]
- Swordbreaker's restriction from the player's entering: get-off, seat change, stance, weapon change and map zoom off;
  fire, zoom, reload and the turret on. At the end scene fire goes off too. [data]
- The six quick-time triggers armed at Uprising's rat stay live for the rest of that level (the survey). Nothing in the
  seat asks for one. [data]

### 2.9 Timelines (the game's server, from the player's press)

Swordbreaker [data]:
- 0 s: on entering, the restriction above, six runners spawned along paths (killed at their path's end), voice; the
  climb-in scenario on the gunner's entering. Five soldiers with search-and-destroy orders are already there (spawned
  1 s after the area starts).
- 18 s: "defend" succeeds, voice. 19 s: the earthquake's effects start. 25 s: spawns cancelled; 26 s: the five killed.
- About 28.7 s: the truck's own animation (`Technicalearthquake`, 101 keys of move and turn) starts, and 0.3 s later the
  player is taken out of the seat. The end scene follows with fire off.

Uprising [data, the order of events; the run time is the inferred part]:
- 0 s: the press; moved into seat 1; restrictions; climb-in. 4.33 s: climb-in done; fire on; enemies start;
  "ZOOM GUN" at about 7 s.
- 35 s: the Osprey's event (sounds, the enemies retreat, the friendly Humvee's loop changed).
- 52 s: an unseen RPG blows up a car near the player; 3-5 s later an extra soldier is spawned and runs a path.
- When he reaches its end, 15.35 s later, the rocket's sequence starts: the rocket on the player's Humvee (the player
  made immortal only now), fade, moved out, the get-up. 75 s or more on the gun in all (the runner's path and the
  rocket's place in its sequence are not in the dump's text).
- The player can die on the gun before the rocket (nothing makes him immortal earlier). [inferred]

## 3. What the mod does in the seat today (from the code; never seen)

Nothing in the mod knows a vehicle seat. What each part should do there, and why the first run must look:

- **The view.** camera.cpp turns whatever camera the game renders with. If the seat's camera has its w lanes filled it
  counts as scripted (viewturn.h `IsScriptedCamera`): the body follows the game's camera, which swings with the gun,
  level, the head free, no free hands. If not, it is the player's kind and xr_runtime.cpp follows it from the yaw it last
  held. Either way the view follows the turret. [code; which kind, measure]
- **The aim.** The look angles stop matching the camera, so aim.cpp searches again, nudging candidate floats near the
  soldier for 150 ms each. With `LockSoldierAimingToEntry` the game may also move the look itself, which aim.cpp takes for
  a scripted moment and stops writing. Mouse aim (`MouseAim`) steers "the game's view" toward the right hand by mouse
  moves while the angles are unknown, if `camera::GameWeaponAngles` still has the soldier's weapon camera fresh (200 ms):
  then the turret may already follow the right hand by luck, or never move. [code; measure]
- **The right stick.** Its turn goes into the written look yaw only while the look angles are written
  (`g_turnDirect`, xr_runtime.cpp). When they aren't (a scripted camera, or `aim::Update` writing nothing: the seat, where
  the look stops matching the camera), `input::UpdateTurn(false)` hands the stick to `SetMouseTurn`, and `TurnThread`
  sends it as mouse X, up to 1080 counts a second; `TakeTurn` gives the view none of it. In the gunner's seat mouse X is
  the ring's turn (`ConceptRoll`). **So today the right stick probably turns the turret**, at the mouse's speed. (The
  review's first high finding: the first version of this note said the stick aimed nothing in the seat.) [code + data]
  With the seat built, `UpdateTurn` itself takes the stick directly in a gun's seat, whatever its caller says
  (`mountgun::RouteStick`, the rule `tools/mountguntest.cpp` q3 runs: the second review's thirteenth finding).
- **The trigger.** With no gun profile the free hands sleep, `hands::FireHand` is the right, and the right trigger sends
  the left mouse button: the gun should fire. [code]
- **Stray keys.** The stick's stance flick sends X (countermeasures on this seat: none) and Z (unbound); a squeeze at a
  shoulder sends 1, 2 or 4 (1 and 2 select the seat's weapon slots, 4 is unbound); the weapon buttons 1 and 4; the head's
  real crouch Left Ctrl (bound to "crawl" in the seat); the left stick's click Left Shift; the grips' quick-time keys F
  and Z in a scripted moment (the climb-in is one by its camera); the use button E (get off, locked here). The left
  trigger does **not** zoom today: with free hands on (the default) input.cpp drops its right mouse button. [code + data]

So today's seat is probably a gun the right stick turns (through the mouse) and the right trigger fires, with the view
turning when the turret does. Whether Uprising can be finished that way is the first thing R2 answers (the plan's B2): if
it can, B2 need not wait for step 1.

## 4. The design

### 4.1 States

| State | When | What runs |
|---|---|---|
| On foot | no seat, or the seat not read for 0.25 s (a read missed for less holds the seat as it was) | everything as today |
| Other seat | **every seat not told for a gun by measured rows**: Rock and a Hard Place's Growler ride, Guillotine's Humvee ride, Kaffarov's SUV, The Great Destroyer's police car (its M9 needs the free hands and the look), a passenger's or driver's seat of the truck itself, a gun's seat whose rows aren't all as the laws take them (measured, every number filled, the yes-or-no rows measured yes) or whose runtime isn't built, and **a gun's seat whose steering stopped itself** (4.6's guards), till it's left | as today, exactly: never changed by this work |
| Seat, scripted | a gun's seat told and ready, its first moments (the climb-in) | the cutscene rules: the game's body and camera, level, head free, no hands of the player's; the seat's quiet list (4.8) already on |
| Seat, gun | the same, the climb-in over | sections 4.2 to 4.11 |

(The first version put "any seat the reader can't place in a row" under Seat, scripted: every vehicle seat without a
row would have had the cutscene rules and the quiet list, breaking finished missions. The review's second high finding;
`tools/mountguntest.cpp` s2-s7 and s10.)

**The one state the gates read** (`mountseat::Seated()`, `src/mountseat.cpp`): true only in Seat, scripted or Seat, gun,
that is only when the reader is measured and borne out, the seat is told for a gun by its entry and its vehicle's name
(both measured), every row that gun's view and steering need is measured (`mountgun::Ready`), and step 1's runtime parts
that need the game are built (the seat's view and the steering's outputs, section 5). False otherwise: everything as
before. A partly measured seat is never half on: with the stick held off the mouse and the look quiet but no steering of
our own, the gun would turn worse than today's.

**The seat reader** (`src/seatread.h`, built: written as poseread.h is: pure, read through camera.cpp's guarded reader,
heap objects and the image only, AMD-safe): from the local player (god.cpp's chain, `[[[bf3+0x1F80B58]+0x30]+0xBC]`)
the controllable it is attached to, checked by its vtable's type name (a `ClientVehicleEntity` or its kind expected, the
name cached by vtable), the entry index (0 to 15), the vehicle's data checked by its type name, and the data's name.
Every place is a row (`seatread::kLayout`): **all unmeasured** (R3), each with where it must come from, plus the code
that reads them, borne out by its bytes at startup. `Prove` refuses at the first unmeasured row before reading anything
of the game; a measured row not borne out turns the reader off for the run with one log line. Either way the mod
behaves as today.

**Which gun (the rows).** A row per number (`mountgun::kGuns`, section 11): the entry, the vehicle's name, the pivot and
camera offsets, the camera's rigidity, the pitch limits, the handles, the eye, the climb-in's length, the turret's answer
(its lag; each way's delay, signs and least input; the game's vertical invert; the pad's rates, the mouse's), the rounds'
line, the fire, step 2's draws and turn centre. Each beside the dump's figure where it has one (2.2), only to check a
measurement against: nothing is matched, placed or steered on a row that isn't measured, that lacks a number it needs (a
row's unfilled numbers are not numbers, never zeros; a table's pairs rise from the first with no gap), or, for the three
yes-or-no rows (the camera rigid on the gun, the rounds along it, the left button firing), that was measured as no: the
log names the row and why (`mountgun::RowState`, `Ready`; the second review's fourth and eleventh findings, r7 and r8).
The law's numbers come from the rows alone (`mountgun::AxisRowsOf`, `GeometryOf`): an `AxisRows` or a `Geometry` left
unfilled is refused (r9). A seat is told for a gun by its entry index and its vehicle's name, both read; a third vehicle
with the same seat type is never taken for one of these.

**The climb-in's end.** Seat, scripted ends when the game's camera has kept still for 0.5 s with nothing of ours sent,
at least 1 s after the seat appeared and at most the climb-in's measured length and a second (`kRowClimbIn`, R7: Uprising's
scene is 4.33 s in the data, Swordbreaker's untimed; the first build's fixed 6 s was a number from no source, the second
review's twelfth finding). At that moment the body takes the game camera's yaw (4.2), so nothing jumps.

**A read missed** (the reader's memory unreadable for a moment, a hitch) holds the seat as it was for 0.25 s; only a read
saying no vehicle, or another seat, ends a gun's seat at once (the second review's ninth finding: one unread frame had
given a frame of today's rules and a new climb-in of 1 to 6 s; s12).

### 4.2 The view: a body fixed to the vehicle, the eye at the gunner's place

- **Body yaw** β: set once from the game camera's yaw at the climb-in's end, then changed only by the right stick (4.5).
  It never follows the gun. On the truck, whose turret is held still in the world, β also keeps still in the world; on
  the Humvee (turret turns with the hull) it is kept relative to the hull. Both vehicles stand still while the player is
  on the gun (the truck moves only in the last 0.3 s before the pull-out).
- **Horizon level**, as in every scene: the vehicle's pitch and roll never reach the view.
- **The eye**: the gunner's place on the ring when the ring faces β and the gun is level, E = Y + Ry(β)·e, with Y the
  ring's axis point in the world and e the eye's offset in the ring's frame, plus the head's own movement as on foot
  (headmove.h's 6DoF and limits). e starts as the game camera's offset at zero pitch (2.2) and is moved to where the
  game's own palms say the gunner's eye is, if R6 finds the camera sits away from his head. The game camera's own place
  (which swings on a 0.6 to 1.0 m arm as the gun turns and tilts) is never the eye.
- **Y in the world**: taken **once**, at the climb-in's end, from the game camera: Y = C - Ry(ψ)·(p + Rx(θ)·c), with C
  its place, ψ and θ its yaw and pitch and p, c the row's offsets (exact while the camera is rigid on the pitch body, R4;
  `mountgun::RingFromCamera`, `BodyAtClimbEnd`). Not every frame: the camera leans by up to 10 cm sideways and 5 cm down
  with the very input that turns the turret (`LeftCurve`, `UpCurve`), so the eye would wobble with the hands (the
  review's eighth finding). Both vehicles stand still on the gun; a moving seat would take Y from the vehicle's
  transform (a row then).
- **The game's camera as it made it** (its rows and place before the mod turns it, each build): the steering reads the
  game's gun off it (4.6) and the climb-in's end waits on it. camera.cpp keeps it today only while the head test's detail
  is on (`NoteBuildPose`'s `made`), so the seat's view keeps its own copy, always, as it's built (the review's ninth
  finding; part of the seat's view, not built yet).
- **The picture** is drawn from that eye: the render and cull cameras get the eye's place and the head's turn about β, as
  the stick turn's view does on foot (camera.cpp's turn, views.cpp's eyes; `purerows` keeps the game camera's shake out).
  The game's camera turning with the gun never moves the picture.
- **The gunner's body and arms as the game draws them** are seen from that eye where the game puts them. If R6 shows them
  drawn in the first-person views (the arms' space follows the game camera), they get the turn and the shift between the
  game camera and the eye on their per-view uploads, as the scene body lock does (skin.cpp's per-view lock, viewturn's
  `TurnWith`, with the translation added); if they are world draws, nothing is needed.

Why not the game's own way, the body turning with the ring: then the view turns whenever the hands move the gun (the
world swinging by the player's own arms), and with the body turning, the handles never move relative to the hands, so the
hands could only push the gun like a joystick, not move it. The owner's rule is the hands moving the gun.

### 4.3 The hands on the handles

- **Take**: a fresh squeeze of a grip with that hand within 12 cm of a handle's grip point (a rough reach, the holsters'
  lesson: no precise finding). Either hand, either handle. The short buzz a holster draw already gives; no new sound.
- **Hold**: while the grip stays squeezed. The handle holds the hand: in step 2 the glove is drawn on the handle (its grip
  frame from the row, fingers closed by grasp.h's pose for that handle), the arm solved from the player's own shoulder
  (the wrists rule, never stretched); the controller may sit a few centimetres off it.
- **Let go**: the grip released, or (step 2) the controller 25 cm from its handle **after its take's offset is taken
  off** (`mountgun::Apart`: |hand - R(frame)·d - H|; a raw distance would drop the Humvee's handles mid-turn, the review's
  fifth finding). The glove then goes back to its controller. The gun stays where it was: a mounted gun does not fall.
- **The other hand**, not on a handle, is an empty glove at its controller (step 2) and does nothing in the seat: no
  holster, no grenade, no magazine.
- **Step 1** draws no hands of the mod's: the game's own gunner arms stay on the gun as the game animates them, and
  follow it as the hands move it. On the Humvee they turn with the ring as the player turns with it, so they stay close to
  the player's arms; on the truck their shoulders swing round the post with the gun, which shows more the further the
  gun is turned. Step 2 replaces them with the player's own.

### 4.4 Where the hands put the gun

The gun's yaw ψ about the ring's axis and pitch θ about its pivot put each handle i at
H_i(ψ, θ) = Y + Ry(ψ)·(p + Rx(θ)·h_i), with h_i the handle's point in the pitch body's frame (row).

- **At the take** the gun's angles (ψ0, θ0) are read off the game camera and each holding hand's offset from its handle
  kept: d_i = hand_i - H_i(ψ0, θ0). So the gun never jumps when a hand closes, and a player whose arms are shorter or
  longer than the gunner's still moves it as far as his hands move. **The offsets are kept in the holding frame, not the
  world** (the review's fifth finding: kept in the world, a 90-degree stick orbit on the truck left the gun 11 to 26
  degrees off, by the offset's direction; `tools/mountguntest.cpp` h10, h11): with both hands holding, the frame of the
  line between them across the ground (a seated torso's turn turns the offsets with the hands, a wrist's twist doesn't);
  with one hand, the body's yaw (the stick's orbit turns them). A change of frame (a hand taking hold or letting go, the
  hands closer than 8 cm) puts every offset into the new frame with its wanted place where it was: the gun never jumps
  (h13). A holding hand lost (not tracked) changes the frame from the last frame's heading, never from the lost hand's
  place, which is whatever the runtime last had (the second review's tenth finding: 30 cm off it moved the other hand's
  target 7.6 degrees, at (0, 0, 0) 10.3; now nothing, h15). `mountgun::Holds`, `HoldStep`.
- **Each frame** the gun's target is the ψ, θ that put the held handles where the hands want them: with both hands, the
  handles' middle where the hands' middle (less the offsets) wants it and their line turned as the hands' line turns,
  their own spacing kept, fitted at once (the tilt from the handles' height in closed form, the turn about the ring's
  axis in closed form, then one Gauss-Newton step on both); with one hand, its handle at its hand (two Gauss-Newton steps
  from the last answer). θ inside the pitch limits. The hands' spread and roll move nothing at all (h9): the gun cannot
  roll or stretch. `mountgun::Solve`.
- This is the owner's rule for both shapes: on the Humvee, both arms swinging left round the body turn the gun left; on
  the truck, both arms pushing left swing the barrel right about its post, as a pintle gun does. Up and down likewise
  (on the truck, handles up, muzzle down).
- **Smoothing**: a 1 cm hand wobble at a 0.3 m lever is 2 degrees of gun. The target goes through a speed-dependent
  low-pass (steady when the hands are slow, no lag when they sweep); with two hands the midpoint already halves the
  wobble. The game's own bodies add their weight on top.
- **Reach**: there is no arc. The gun goes as far as the hands can carry the handles: on the Humvee, as far as the player
  turns himself (a seated player about 90 degrees each way, a swivel chair all the way); on the truck, 60 to 90 degrees
  each way before the handles are out at arm's length to the side. The right stick does the rest.
- **Found building it (for R10, both guns): the turn's centre.** A seated player turns about his own spine, about 0.12 m
  behind the eye and 0.10 m under it. Neither gun's handles turn about that point: the Humvee's ring axis stands 0.30 m in
  front of the spine, the truck's post 0.75 m. The handles' circle and the hands' are then not one, and no offset rule
  keeps them together through a big turn. In the test's geometry (the handles made up: the Humvee's 0.55 m behind its
  pivot, the truck's right one the mirror of its left IK point), `tools/mountguntest.cpp` h12:
  - **the Humvee**, both hands: a torso turned 20 degrees turns the gun about 38, turned 45 about 75; the hands stand up to
    21 cm from where the handles can be. "The gun put where the hands put its handles" overturns.
  - **the truck**, both hands: a torso turned 20 degrees swings the gun 17 the other way (the handles go the torso's way,
    the barrel the other: the owner's call 1), turned 45 degrees 27; the hands stand 13 cm from the handles at 20 and
    34 cm at 45. One hand alone is lopsided: the left alone turned 45 one way swings the gun 7, the other way 45. (The
    second review's fifth finding: the first version said this was not a question for the truck.)
  
  Step 1 doesn't mind (the game's own arms are drawn; a hand holds while its grip is squeezed). Step 2 does: its gloves
  would sit up to 34 cm from the controllers, and its 25 cm let-go would drop the truck's handles at about 35 degrees of
  torso turn. So the turn's centre is a row for both guns (`kRowTurnCentre`: the fit about the ring's axis as built, or
  the hands' turn about the player's body taken as the gun's), settled in the headset at R10 with both hands and with one;
  step 2's hands (`kNeedHands`) wait on it, its let-go by distance with them.

### 4.5 Turning further: the right stick

- The right stick's turn (smooth, as on foot; the mod has no snap turn) turns the body about the ring's axis and the
  gun's target with it, so the handles stay in the hands. On the Humvee that is a turn in place; on the truck an orbit
  about the post (the gunner walking round his gun). **Its speed is held so the eye moves at most 0.75 m/s on its
  circle** (`mountgun::OrbitTurn`, h10 and o1): on foot's 266 degrees a second would carry the truck's eye sideways at
  2.9 m/s on its 0.63 m arm (the review's sixth finding). So the truck turns 68 degrees a second, the Humvee (0.18 m)
  236. My call, made for the owner (section 8, question 4): a capped smooth turn over a snap orbit, since the owner's
  answer to question 2 is "as on foot".
- The stick's turn **never goes through the mouse** in the seat (`mountgun::StickDirect`; xr_runtime.cpp's
  `input::UpdateTurn` takes it directly whenever the seat is on): the mouse's X is the ring's own turn there, and the
  turn thread would have fought the steering and turned the gun in the climb-in (section 3; the review's first high
  finding; q3: 2 s of a held stick, no count sent, where today's routing sends 2160).
- The stick's down and up do nothing in the seat (the stance is locked anyway; X is the seat's countermeasures).
- **Option for the owner to try, off by default**: `mountpush 1`, the body turning by itself while the hands hold the gun
  past 50 degrees from the body's front (at most 60 degrees a second, more the further past). It saves reaching for the
  stick and costs a turn the player did not ask for in so many words; players-first says off until the owner says.

### 4.6 Steering the game's turret

Each frame on the render thread, while in Seat, gun (`mountgun::SeatSteer`: `SteerAxis` for each axis with the rows
`AxisRowsOf` makes, built and tested against a simulated turret: section 5):

1. **Where the game's gun points**: the yaw and pitch of the game's camera rows as the game made them, before the mod's
   turn (the seat's own copy, 4.2). A camera older than 100 ms: send nothing.
2. **Where the turret would come to rest** if nothing more were asked: its angle, plus the turn the rates already asked
   will still make (those sent within the last delay), plus its momentum's coast (its rate times its lag). The rate is
   the law's own model of what it asked (through the delay and the lag), kept honest by the camera's measured rate (its
   turn between its own moves, which come at the game's ticks: a frame's step is no rate).
3. **The demand**: the rate that closes the gap between the target and that resting point over a horizon of three ticks
   or one and a half lags, plus the target's own rate (a steady sweep followed with no standing lag). The target is taken
   where it will be when the demand acts (its rate and its own slowing, never past where that slowing stops it: a hand
   stopping). With a first-order turret the law's roots are -1/lag and -1/horizon, both real: no overshoot. **Braking**:
   the resting point past the target while the turret turns toward it (a hand stopping sooner than the turret can coast
   to a stop) takes the gap away over one tick, as hard as the input allows: coasting from 30 degrees a second the
   turret stops 6 degrees on, the whole input the other way stops it in 2, a hand in about 4. Cut to the input's most.
   Under 0.15 degrees: nothing sent. **Under the least input that moves the turret** (the dead zones, the momentum's
   threshold), the least, held for its delay and lag once begun (a shorter pulse never gets past the threshold), while
   the gap is more than the least's own coast; else nothing: a standing error under that coast, never a hunt.
   (The first version's gain, "a third of what would close the gap in one tick", overshot by 8.3 degrees against a
   turret with the dump's momentum: t12.)
4. **The input**, two ways behind one call, the better one chosen by R5, **the mouse by default** (the review's fourth
   finding):
   - **mouse moves** (`SendInput`, as mouse aim does): counts from the demand, the fraction carried (no drift), at most
     the row's counts a second. Only with the game's window in front, as today.
   - **the pad's right stick**: pad.cpp already answers the game's XInput for controller 0 (its left stick walks); the
     right stick would go in the same answer. The demand goes through R5's own table of rate against stick, measured at
     the seat's curve's knees through the game's own pad dead zone (four points from 0.25 to 1 read its top half as a
     straight line and missed half the rate there). Not read for 500 ms (the known stall): the mouse instead.
   **The rows per way**: each way has its own answer row (`kRowPadAnswer`, `kRowMouseAnswer`: its delay to the camera's
   first turn, which way its X and Y turn the gun, the least input that turns each axis), since the pad waits a tick
   more for the game to read the stick and its least is the dead zone's edge, where the mouse's is a count step; the
   turret's own time to speed (`kRowLag`) is one for both. `mountgun::AxisRowsOf(gun, axis, way)` makes the law's rows
   from them alone, the pitch's limits from its row (r9).
5. **The input's strength** is learned only from a push held one way, strongly, past the turret's delay and three lags,
   never from the coast after a push (the review's fourth finding: t10 learns a turret 1.6 times the row's as 1.58 and
   holds one 3 times at the bound; the first version's "last half second against what was sent" read windows up to 26%
   off, t11). Kept within half and twice the row's figure. The player's own settings, read where the launcher already
   reads the profile (`GstInput` keys: the vehicle mouse sensitivity, the scheme's vertical invert, the pad dead zone),
   are to scale the owner's measured rows, so the learning has only small errors left (with the outputs: not built).
   **The game's vertical invert** is a row of its own (`kRowInvert`, R5's run with the invert switched: which profile key
   flipped a pitch sign, its value while the signs were measured, which way's pitch it flips): the runtime reads the
   player's key and `mountgun::SignOf` flips the pitch's sign for a player whose key differs (r10), so a player with the
   invert on steers right from the first push (the second review's second finding, its first fix).
6. **The guards** (`SteerRules`), each on one axis; whatever their reason, the axis stops for good, said once, and sends
   nothing more:
   - **A push begun from rest** (nothing asked for the turret's delay and two lags before it, the camera still: no input
     in flight, no coast to take for an answer): its first answer is judged at once, so the first move itself probes the
     sign. Past the delay, 0.5 degrees against the push: a wrong sign; 0.5 with it: answered; past the delay and two lags
     with 2 degrees asked and under a tenth of it turned: no answer. (The second review's second finding: "probe the
     sign with a small known input at the take". The first move after the climb-in is such a push.)
   - **A hard push** (a quarter of the most): the turret turning the other way faster and faster past its delay and half
     its lag (an overshoot's coast against the push only slows, so it never trips this).
   - **Any push held one way, whatever its size**: from the turret's delay and two lags on, window by window (a quarter
     second and half a degree asked at least), the turn the turret made against the turn asked of it: under a tenth of
     it twice running, no answer; against it by more than a quarter of it, a wrong sign. (The second review's first
     finding: the first build armed both guards on hard pushes alone, so a read that is fresh but doesn't follow the
     turret, a camera copy that stopped updating or follows the head bone, with a small target error never tripped
     either: the review's probe ran the turret 28 degrees in 20 s at 0.5 degrees of error, 723 at 13. Now stopped 0.36
     to 2 s after the push began, the smaller the error the later: the turret 1.5 degrees on at 0.5 of error, 14 at 13,
     43 at 40, t17; a game not reading the input at 0.5 degrees, t20.)
   - **A push into a stop answered by nothing** is the stop found, never a fault: past the middle of the axis's limits
     toward the push and within 10 degrees of the row's limit there, the target is held inside the stop where the turret
     stands (a row a degree or two off; the truck on a slope, its pitch held in the world). Further off: no answer (t19).
     A wrong sign that pins the gun at the other stop pushes away from it, so it stops as no answer (t18).
   
   With the game's invert on and a small first move (0.5 to 15 degrees), the gun now goes 0.9 to 5.8 degrees the wrong
   way before the stop (t18; the first build: the mouse's pitch into its stop at -20 and held there, sending every frame,
   the yaw 29 to 32 degrees). No false stop with the hand going back and forth, the rows off (the delay a tick either way,
   the lag a third to three times, the strength half to twice), steps there and back, a quick turn (t21: 0 of 134).
   **A stop ends the steering of both axes for the rest of that seat**: `mountgun::SeatSteer` calls `StopSteering`, and
   the next `SeatStep` puts the seat to Other seat, today's behaviour (the look, the free hands, the stick through the
   mouse), till the seat is left; the log says "the steering stopped on the yaw/pitch: <why>: ...'s seat works as before
   till it's left". (The second review's third finding: the first build's stop left the quiet list on and the stick off
   the mouse, a gun nothing turned for the rest of the seat, 70 s or more on Uprising; s11.)
7. **Nothing is sent** with no hand on a handle and no stick turn (the gun holds still), in Seat, scripted, with the
   game's window behind (mouse), or with the camera stale. A demand's last value is never left standing: the right
   stick goes back to zero on any stop, as the walk's left stick does (t6).

The drawn gun follows the hands as fast as the game turns it. If R5 finds the turret much slower than a quick sweep,
the hands (drawn on the handles in step 2) lag with it, which reads as weight; past 25 cm the hand lets go (4.3).

### 4.7 Fire

- The left mouse button is down while the trigger of a hand that holds a handle is pulled (either hand, both triggers
  one button: the owner's call, question 3), 0.6 to press and under 0.4 to let go, with input.cpp's release guard
  (`StepRelease`: a release the game never got is sent again, after Swordbreaker's roof, where the game kept firing by
  itself). A trigger of a hand not on a handle does nothing; a trigger pulled before its hand took hold waits to be let
  go (no burst by closing a hand on it), while a grip and trigger squeezed in the same frame fire. In Step 1, a hand
  counts as on its handle when its grip is squeezed (there is no drawn take yet). `mountgun::FireStep`,
  `mountseat::FireDown`, input.cpp's fire mapping while seated (f1-f3). **When it ships**, the README and the CHANGELOG
  say it in one line ("on a mounted gun, squeeze a grip and pull its trigger"), so nobody takes a trigger alone for a
  broken gun (the review's fourteenth finding).
- `FireButton`'s seat branch takes over from `hands::FireHand` in the seat; nothing of the held guns' rules (slide,
  tube, launcher) applies there.
- The overheat is the game's: it stops the rounds and shows its bar. The buzz a round (the recoil foundation) goes to
  each hand on a handle at the game's 550 a minute while the button is down, stopped by a copy of the game's heat
  (0.075 a round, 0.5 a second down, 1.2 s penalty: 2.6) until R8 finds the game's own firing state to read instead.
- Never the zoom: the left trigger sends no right button in the seat. If the game ever zooms anyway, the mod's render
  field of view must win (R7 checks).

### 4.8 Quiet in the seat (all states of the seat)

One table (`mountgun::Quiet`, `mountgun::What`), one state (`mountseat::Seated`): in the seat **every mapping but the
fire, the use button and the recenter** is quiet (the review's thirteenth finding: the first table missed the weapon
buttons, the head's crouch, the left stick's click, F and Z). Built and wired, each gate a single read of the state that
is false today (so on foot, and in every other seat, nothing changes):

| What | Why | Where (built) |
|---|---|---|
| The look angles: no writes, no search, no nudges, no mouse aim, no view tilt | they aim nothing here (`LockSoldierAimingToEntry`); a nudge in a vehicle is an unknown; the search's tilt is 24 counts of mouse X and Y, about 6 degrees of the ring and the gun | xr_runtime.cpp: as in a cutscene, `aim::Forget` with the seat's reason, no `aim::Update`; aim.cpp `Proves`: a search begun before the seat (its own thread) nudges nothing ("busy"), and `TiltView` sends nothing (the second review's sixth finding) |
| The right stick's turn through the mouse | the mouse's X is the ring's turn (section 3) | input.cpp's `UpdateTurn`: `mountgun::RouteStick`, the stick always direct in the seat (q3 runs that rule); the turn to `mountseat::NoteStickTurn` (the body's orbit) |
| Every mapping but fire and use: G, the jump and the stick's click, the head's crouch (Left Ctrl), the sprint (Left Shift), the picks (1, 2, 4), Y held's swap, the left trigger's right mouse button and the "zoom 1" test | the seat's own bindings (1 and 2 its weapons, Left Ctrl "crawl", the right button the zoom to 22 degrees) | input.cpp's mapping loop |
| Stance from the stick (X, Z) | locked by the level; X is countermeasures here | input.cpp: `UpdateStance` given a still stick, as a cutscene's |
| X and the D-pad's eject (the magazine out, R) | no magazine here | input.cpp before `XButton` |
| Walking keys and the left stick, and the walk under the head | the seat's throttle and yaw | input.cpp before `pad::SetMove` and the keys |
| The free hands and all they work: holsters (no draws, no put-aways, no keys 1/2/4; holster.h's frame isn't run), grenades by hand, magazines and reloads by hand, the pouch, the held guns' recoil rows, the M320 | the soldier's own gear is not out; the game's own gunner arms are drawn | hands.cpp's `Update`: returns as with free hands off, the game's arms as it draws them; `M320HeldBy` says no hand on the 40 mm (its flags would stand as they were when the seat began, and a trigger pull would have been the launcher's: the second review's seventh finding) |
| Quick-time answers on the grips | the grips hold the handles; the climb-in is a scripted moment by its camera | input.cpp `QteStep`: never a scripted moment's press, so qtewatch.cpp awaits only a fist fight's open prompt, as in play |
| The fire | the seat's own rule (4.7) | input.cpp's fire mapping: `mountseat::FireDown` |
| With nobody in the headset: the head's crouch, the walk under the head, a film's trigger | so a film shows the seat's own rules | input.cpp `HeadMoveAlone`: no Left Ctrl, no walk; `FireAlone`: the seat's fire rule, so a film's trigger with no grip on a handle fires nothing, said once a pull (the second review's eighth finding: the held guns' rules had fired it) |

The right stick's click (recenter when held) stays: in the seat a recenter puts the room's front at β and the head at the
eye. The use button stays (get off where a level allows it).

### 4.9 Controls in the seat

| | Steam Frame | Quest 2 / 3 |
|---|---|---|
| Take a handle | either grip near it | either grip near it |
| Fire | either trigger, its hand on a handle | the same |
| Aim | move the handles: both hands, or one | the same |
| Turn further | right stick left and right | right stick left and right |
| Recenter | right stick click held | right stick click held |
| Use (get off where a level allows it) | Y, as today | Y (left hand), as today |
| Everything else | nothing in the seat | nothing in the seat |

No D-pad and no View button anywhere: every control has a Quest twin. Left-handed players: either hand on either
handle, either trigger.

### 4.10 Getting on and off

- **Now**: the use button (E). On the truck, E within 5 m (the game picks the seat: which one is R1's first check; the
  level forbids changing seats once on, so a wrong seat would trap the player). On the Humvee, E at the prompt.
- **With the prompts-by-reach work (A15)**: reaching to the Humvee's prompt and holding a grip presses E there; on the
  truck, both hands reaching to the gun's handles and squeezing presses E (the handles' place known from the vehicle
  once the reader can see vehicles nearby, not before). The climb-in then plays as the game's own scene.
- **Off**: both levels do it themselves. The seat's going (the reader's pointer gone) ends every seat rule at once:
  the scene that follows is a scripted one as usual, then the look angles are searched again and the free hands wake
  with the soldier's gun when the game draws it, as after any cutscene. Swordbreaker's pull-out comes 0.3 s after the
  truck starts its earthquake motion: the body follows the vehicle for those frames (no jump), then the scene.
- **A death on the gun** (Uprising before the rocket, Swordbreaker's 19 s): a new soldier at the checkpoint; the reader
  sees no seat; all as after any death.

### 4.11 The HUD

The game draws the seat's crosshair (the truck's `SimpleCrosshair`) at the centre of its own camera, which is not the
headset's view: in VR it would sit nowhere near where the gun points. It is hidden in the seat, as the on-foot centre
crosshair is not shown. The heat bar and the objective stay. (Which HUD draws they are: R6 with `tail`.) No mark of the
mod's own on the target: aim by the gun's sights (the head leans behind them) and its tracers.

### 4.12 Settings and test words

With the runtime parts (not built yet; the log says the seat's state and once a run why it is off, `src/mountseat.cpp`):
- weapon.txt `mountlog N`: N seconds of one line a frame (the seat, the hands' targets, the game's angles, the error, what
  was sent and by which way, the learned gain).
- weapon.txt `mountsteer pad|mouse|auto`, `mountpush 0|1`, `mountseat 0` (the whole seat work off: today's behaviour).
- weapon.txt `mountprobe yaw|pitch mouse|pad N`: R5's steps sent by the mod itself in the seat (the counts or the stick
  held 1 s), so the runbook needs no outside script and the right stick's steps exist at all.
- build\freehands.txt `test N` gains seat tests (scripted hands taking the handles, sweeping, firing), so the films need
  nobody in the headset, like the holsters' tests.
- No player-facing setting: the rows' numbers are measured, the gain learned.

### 4.13 Never break, players first, AMD

- Unknown seats, a failed proof, a row that doesn't match, a row not measured, the runtime not built: today's behaviour,
  said once.
- Leaving the seat restores every rule; on-foot code paths are untouched when no seat is read (one boolean read a frame):
  every offline program's output is the same with the gates in as without (the build's run, section 5).
- Reads: the local player and its vehicle are heap objects, vtable-checked before every use; the game's camera rows are
  the ones camera.cpp already copies on the game's thread. Nothing the game mapped from the graphics card is read; no
  wait on the card. Only the right stick (or the mouse) is new output, and it is zero whenever the seat rules aren't
  steering; the stick's own turn never reaches the mouse in the seat (true now: section 3's path is gated).
- The frame cost: one 2x2 solve and a few vector sums a frame.

## 5. Build steps

**Step 1, *finish* (M, about a day after R1-R5):**
- **Built, 2026-11-25** (offline, off until measured):
  - `src\seatread.h`: the reader and its proofs, every place a row (all unmeasured). Its test is in
    `tools\mountguntest.cpp` (g1-g6, a made-up memory image as stancetest has).
  - `src\mountgun.h` (pure) + `tools\mountguntest.cpp` (77 checks): the rows (r1-r10: unmeasured, each need naming its
    first missing row and why, the yes-or-no rows measured as no and half-filled rows refused, the law's rows from the
    gun's rows alone, the game's invert flipping the pitch's sign), the states (s1-s12: the climb-in at its row's length,
    a read missed for 0.2 s holding the seat, a guard's stop putting it back as before), the quiet list and the stick's
    routing as input.cpp runs it (q1-q3), the handle law with its takes in the holding frame (h1-h15: a seated torso's turn
    on both guns with one hand and two, a lost hand's place moving nothing), the stick's orbit and mountpush (o1-o2), the
    target's smoothing (m1), the steering against a simulated turret (t1-t21: a rate input with momentum from 0.1 to 0.6
    of the way a tick, a 30 Hz tick, a delay of one or two ticks, the camera at the tick or between, the pad's dead zones
    and the seat's curve or the mouse's counts, a momentum threshold, the rows right or off by half or twice, a wrong sign
    big and small with the pitch's stops, a deaf game, a read that doesn't follow the turret, a stop met short of its row,
    no false stop with the rows off), end to end (e1), the fire rule (f1-f3), the time (z1: about 2 us a frame). The
    reviews' broken laws each shown failing (s10, h11, t8, t11, t12; q3 shows today's routing sending 2160 counts).
  - `src\mountseat.cpp`: the seat's one state each frame (the reader once its rows are measured, the gun told, the
    states), the log's lines (once a run: why the seat is off, each row's fault named; then each change of seat, a
    steering stop), `Seated()` and `Quiet()`, the fire in the seat; once the reader is off for the run nothing more of the
    game is read there; the gates wired in xr_runtime.cpp, aim.cpp, input.cpp and hands.cpp (4.8). The steering is there
    to call (`mountgun::SeatSteer` with `AxisRowsOf`'s rows); nothing calls it yet.
- **Left for step 1, needing the game** (each waits on its rows, R3-R7):
  - the seat's view: the body fixed to the vehicle and the eye at the gunner's place into xr_runtime.cpp, headmove.h and
    camera.cpp (4.2), with the game's camera kept as it made it, always (4.2's second bullet);
  - the steering's outputs: the mouse's counts (`SendInput`) and the pad's right stick in pad.cpp's XInput answer, the
    profile's keys read (the invert key for `SignOf`, the sensitivity scaling the rows), and `mountprobe` for R5 (4.12);
  - the hands' world places from the room through the body (`mountgun::RoomToWorld`), `HoldStep`, `Solve`, `SmoothStep`
    and `SeatSteer` called each frame, said by `mountlog` (its stop said by `mountseat.cpp` already);
  - the seat's scripted grips for the films (freehands.txt's seat test), so section 7's steps 12 to 14 run with nobody in
    the headset;
  - `kBuilt` in `src\mountseat.cpp` set true only then: until it is, no seat is ever a gun's.
- Hands: the game's own (4.3). A hand counts as on its handle while its grip is squeezed.

**Step 2, *full* (L):** the gloves drawn on the handles from the player's shoulders (skin.cpp's arm placement with a
handle as the target), takes within reach and lets-go by distance (both waiting on R10's turn centre, on both guns: 4.4),
the buzz a round, the hidden crosshair, `mountpush`, the reach-and-grip entry with A15. Every offline test program, at
Idle priority, before each merge.

The work shares its first game session with the plan's B2 (Uprising) and Swordbreaker's end; the tank's roof .50 later
takes a row.

## 6. The runbook: what must be measured in the game

Rules for every round: the owner not playing and nobody in the headset (or the owner's word); the owner's live save
backed up first (`build\saves\user_live_<date>_<time>`, hashes kept) and put back after; weapon.txt in its hand-over
state after every film (`standin 0`); every compile and offline test elsewhere at Idle priority while the game runs;
memory tools read-only (`tools\dwords.ps1`, `fields.ps1`, `owner.ps1`, `findcode.ps1`, run in the 32-bit PowerShell);
loading a mission from Replay Missions moves the campaign's resume point: say so first. Swordbreaker's gun is reached in
one playthrough from a save before it: **the level has no checkpoint after the cellar fight** (none in the bridge fight,
the meat market or the earthquake), and the truck is spawned only by the earthquake part's event (the objectives: CLEAR
THE WALKWAY AREA, HOLD THE WALKWAY AREA, GET ON 50 CAL at (-609.0, 79.9, -2091.2), DEFEND; the truck at (-610.4, 78.4,
-2091.8)). The first run (2026-11-25, "fifty": `build\logs\bf3vr_1125_fifty_part1.log`, `_part2.log`) did not reach it:
from the owner's save (the cellar fight's end) the laundry's street door at (-590.3, 77.0, -2079.8) stayed shut after
both loads (its sequence starts from the street-side breach prompt or the bridge fight's door trigger, which did not fire
after a load), and the route from `build\saves\sb_PARKOURSTARTSAVEb_2026-11-18` didn't finish in the time box. Its next
route, at least 2 hours in one session: from the parkour save, the bomb site and its 55 s timer, the breach door from the
street side, the laundry, the corridor and the landing, the crawl under the fallen shelf (prone at (-584.6, 73.2,
-2100.0), along x -584.7 to z -2108, down at (-589.9, 72.85, -2109.3); copy the checkpoint saved there), E at (-593.13,
69.39, -2116.99) for the cellar fight (`qtetest 600 held`), back out, the walkway fight, the hold, the truck (that run's
scripts: `scratchpad\fifty\sb.ps1`, `route.txt`, in the session's scratch folder, not the repository). Uprising's needs the first run of that mission (B1) to reach the Staging
Area.

**Cheaper first.** R3's rows are the player's, not the truck's: the attached controllable and the entry index sit on the
local player in every seat, so any seated stretch gives them (Rock and a Hard Place's Growler ride; Swordbreaker's
opening ride, seated about a minute by the level's data, which the 2026-11-25 check pointed at). A Replay Missions load
moves the campaign's resume point: back up the owner's save and say so first. Only the truck itself gives the truck's own
rows (its entry and name, R1, R2, R4 to R7). Uprising's Humvee gives the Humvee's rows, never the truck's: the dump gives
both turrets the same motion numbers, but each gun's rows are measured on that gun.

**R1. Getting there, and the seat it gives.** Walk to the truck with whatever gun is in hand; press the use button
within 5 m. Log:
the "Views:", "Aim:", "Free hands:" and "Input:" lines; a screenshot each second. Settles: which seat E gives (gunner or
driver), how long the climb-in takes, the objective's success.

**R2. Today's DLL in the seat, nothing changed.** 10 s still; then the right hand turned left and right (the stand-in's
hand turned, as the aim films do); the right stick held right 3 s (F17: no test word holds it left, and only F17 drives
the stick's mouse path with nobody in the headset); the right trigger 1 s; the left trigger once.
`viewlog 10` through it. Settles section 3: the camera's kind (scripted or the player's), whether the look search nudges
in the seat (if it does, stop and note: the gate is built, but off), whether anything turns the turret (mouse aim, the
stick through the mouse: section 3 expects it), whether it fires, whether the left trigger zooms the headset's picture
(section 3 expects not). **This decides whether Uprising's *finish* waits on step 1.**

**R3. Where the seat is in memory.** `dwords.ps1` on the local player on foot and in the seat: `-Chain 1F80B58,30,BC,0
-Start 0x380 -Length 0xC0` (the tool reads the chain's last step too, so the trailing 0 lands on the local player
itself; without it the dump is the player manager's: the review's twelfth finding). The words that change: a pointer
whose vtable's type (slot 0's class info, the way poseread names them) is the vehicle's, and a small integer (the entry
index, 1 expected). Then on that vehicle: `fields.ps1` for its data pointer and the data's name; `findcode.ps1` on the
image for the code that reads the player's two offsets (the proofs: their bytes and addresses). Each fills a row of
`seatread::kLayout` (section 11). Check that `+0x3D8` (the soldier) stays the same in the seat and that poseread's read of
it still works.

**R4. The camera and the pivot.** With `viewlog`, the game camera's rows and place per frame while the turret is turned
by known mouse moves (a script's `SendInput`, the game in front): a full circle of yaw and the pitch from limit to limit.
Fit the circle and the arc **on the frames with no input** (the camera leans with the input: LOW 8): the ring's axis
point and the pivot in the world, the camera's offsets (2.2's 0.61/0.20 and 0.98/0.27 expected), its forward against the
barrel's line, the pitch's stops. Settles the rows the pivot, the camera, its rigidity and the pitch limits (Y is taken
once at the climb-in's end either way).

**R5. How the turret answers.** Step responses, each held 1 s from rest, the camera's yaw and pitch per frame:
- mouse: 20, 50, 100 and 200 counts in one frame, each axis; a steady 5 counts a frame; and the smallest step that
  turns each axis at all (1, 2, 3 counts...);
- pad right stick (needs the pad output behind `mountprobe`): the dead zone's edge (the first stick that turns it), then
  the seat's curve's knees through the game's own dead zone, then the whole stick, each axis (both seats carry the same
  curve: section 2.5);
- each again with the game's vehicle invert as the owner has it, and once with it switched (then back): which profile
  key the switch changed (`GstInput.Scheme<N>FlipY` expected; the profile is plain text, read before and after), its
  value during the sign steps, and which pitch input it flipped (the pad's, the mouse's, both), or "none";
- **the rounds' line**: fire a burst at a wall with the turret at a known angle **and the mod's view turned 30 to 40
  degrees away from the barrel** (the stand-in's head turned): the hits must follow the barrel (the game's camera), not
  the view (the review's seventh finding: with the view and the barrel together the test can't tell them apart).
Settles the rows the lag (the turret's own), each way's answer (its delay to the camera's first turn, its two signs, its
least input on each axis), the game's vertical invert, the pad's rates, the mouse's, the rounds' line (a yes: the hits on
the barrel's line; a no keeps the seat off, said). The steering's way by default: the mouse (the pad only if R5 shows it
better). A row R5 can't fill stays unmeasured, and the seat stays as before.

**R6. What the game draws in the seat.** `tail` for one frame in the seat, and the draw hunt if needed:
- the arm draws' keys (the glove draws 13755/15051 and their depth pass expected), the views they are drawn with
  (first-person, in the arms' space, or world), their bones;
- the gun's draws (the M2 and its shield, the truck's gun) and where their place comes from;
- **the handles**: the game's own palms (the gloves' roots) in the seat at four turret angles and three pitches, against
  the camera: each handle's point and grip frame in the pitch body's frame (the truck's left one against its IK point,
  2.2), and where the gunner's head is against the camera;
- the seat's HUD draws: the crosshair, the heat bar.

**R7. The scenes and the zoom.** The climb-in's camera (still, moving, scripted kind?) and its length, the seat's first
frame to the scene camera's last move (`kRowClimbIn`; Uprising's 4.33 s in the data is measured too); one press of the
right mouse button in the seat, and what the headset's picture does; Swordbreaker's end: the truck's motion from
`viewlog`'s places, the frame the seat goes, the end scene; Uprising's rocket: fade, the move out, the get-up, the
Osprey.

**R8. The firing state** (step 2's buzz): the vehicle weapon's heat and its firing flag, by `fields.ps1`/`owner.ps1` on
its weapon object (from the vehicle's weapon component), checked against a 6 s burst to overheat.

**R9. Uprising's Humvee**, once B1 reaches the Staging Area: R1 to R7 on it, plus the shield: the M2's shield turns with
the ring about a line 0.18 m in front of the eye, so with the body held still and the gun turned 60 and 90 degrees, look
for armour passing through the head.

**R10. The turn's centre** (step 2, both guns, the owner in the headset): a seated torso's turn of 20 and 45 degrees each
way, with both hands and with each alone, on the truck first (it is reached first), then the Humvee: the fit about the
ring's axis as built against the hands' turn about the body taken as the gun's (4.4). Settles `kRowTurnCentre` for each
gun; step 2's gloves and its let-go by distance wait on it.

## 7. Filling the rows and proving it in the game

Nobody in the headset first (the stand-in and the films, as every runbook round), then the owner. Every compile and
offline test at Idle priority while the game runs; weapon.txt in its hand-over state after each round.

**A. Fill the reader's rows (R3), then prove the reader alone.** Any seat gives the player's two places (section 6's
"cheaper first"): Rock's Growler ride or Swordbreaker's opening ride before the truck.
1. In a seat: `tools\dwords.ps1 -Chain 1F80B58,30,BC,0 -Start 0x380 -Length 0xC0` on foot, then seated; the pointer that
   changes to an object whose type is the vehicle's, and the small integer (the entry index).
2. `tools\owner.ps1` / the vtable's getType on that pointer (its type name), `tools\fields.ps1` on it (its data pointer;
   the data's type name and its name: a pointer to characters or the characters inline), `tools\findcode.ps1` on
   `build\bf3_mem.exe` for the code that reads the player's two offsets (each one's bytes and address).
3. Write them into `seatread::kLayout` (`src\seatread.h`): each row `measured` true, its value, its source in its comment.
   Rebuild; `mountguntest` must still pass (g1 checks the shipped layout: change it to expect a proved layout, with the
   game's own bytes in `build\bf3_mem.exe` as stancetest's proofs do).
4. In the game: the log's `Mounted gun: the seat is read from the game (...)` at the first level, no "isn't as bf3.exe
   1.6.0.0 has it"; in Rock's ride (a Replay Missions load, said first) `a vehicle's seat (entry N of <name>): no gun's,
   as before`; on foot again, nothing. Nothing else changes anywhere.
5. Reach the truck (section 6's route) and get on with the use button (R1): the log's `Mounted gun: a vehicle's seat
   (entry <N> of <name>): no gun's, as before` names the gunner's entry and the vehicle's name. Write the truck's
   `kRowEntry` and `kRowVehicle` from it (`src\mountgun.h`, `kGuns[0].m`); rebuilt, the same seat says `the truck's .50
   ...'s seat (entry 1 of <name>), kept as before: <first row> not measured yet`.

**B. Fill the gun's rows (R4-R7), then build the runtime parts (section 5's list).**
6. R4 with `viewlog` and known mouse moves (frames with no input fitted): `kRowPivot`, `kRowCamera`, `kRowCameraRigid` (a
   yes or no), `kRowPitchLimits`, each checked against the dump's figure beside it (a measurement far off the dump's is
   said and looked at, not taken).
7. R5 (the mouse first; the pad with `mountprobe` once the outputs are built): `kRowLag`, `kRowMouseAnswer` (and
   `kRowPadAnswer`), `kRowInvert` (the invert run: the key, its value, which it flips), `kRowMouseYaw`, `kRowMousePitch`,
   `kRowPadYaw`, `kRowPadPitch`, `kRowBarrelLine` (the burst with the view turned away: a yes or no), and R2's
   `kRowFires` (a yes or no).
8. R6 (`tail` in the seat, the game's palms at four yaws and three pitches): `kRowHandleLeft`, `kRowHandleRight`,
   `kRowEye`; step 2's `kRowArmsDraws`, `kRowCrosshairDraw`. R7: `kRowClimbIn`.
9. Run `mountguntest` with the measured truck in place of the test table (its steering rows in the simulated turret:
   t1's, t17's, t18's and t21's criteria must hold with them).
10. Build the seat's view and the outputs (the profile's invert key read for `SignOf`, `SeatSteer` each frame, the seat's
    scripted grips for the films); set `kBuilt` true; rebuild; every offline program, at Idle priority.

**C. Prove it (nobody in the headset, then the owner).**
11. On the truck: the log's `Mounted gun: the gun's climb-in: the truck's .50`, then `on the gun` 1 s to the climb-in's
    measured length and a second later; the view still in the room while the turret moves, the head free, the horizon
    level.
12. The stand-in's scripted hands (freehands.txt's seat test, with the runtime): both grips squeezed and swept 25 degrees
    each way and up and down: `mountlog 10` shows the target, the game's angles, the error and what was sent; the gun
    follows within a few degrees and settles with no overshoot; one hand alone; no hand: nothing sent; no "the steering
    stopped" line. Then the pitch held at each stop: at most "a stop found" (never a stop of the steering).
13. The right stick (F17): the body and the gun turn together, no mouse X sent (the log's "Turn steps" line shows none
    from the turn thread), the eye's speed at most 0.75 m/s.
14. Either trigger fires only with its grip squeezed (a film's trigger alone: "no grip on a handle, nothing fired"); the
    overheat stops it; the button up after (no lone release).
15. Nothing stray in the log, with nobody in the headset too: no stance key, no Left Ctrl, no walk, no holster or weapon
    key, no zoom, no jump, no look writes, no search's nudge or view tilt ("busy" at most), no quick-time press, no
    grenade launcher.
16. The invert and the guards, on purpose: the game's vehicle invert switched before the launch (the profile key
    flipped): the first move goes the right way (`SignOf`). Then switched back in the game's options while seated (after
    the mod read the profile): the next move stops within a few degrees, the log says "the steering stopped on the
    pitch: the turret turned the other way ...", and the seat works as before till it's left (the look written, the
    stick through the mouse); left and taken again after a relaunch, steering again. The profile back to the owner's.
17. The earthquake and the pull-out, the end scene, the mission's end; a death on the gun and the checkpoint after it:
    the log's `Mounted gun: on foot` each time, the free hands back with the soldier's gun.
18. The never-break list on foot after the seat: aim follows the gun, the view still, the stick turn's sign, walking,
    shots landing, both eyes, 72 fps, the grip holding, a reload by hand. The self-test.
19. The owner: the feel of the handles on the truck's post; `mountpush` both ways; the orbit's speed.

Step 2: R10's turn centre first (both guns), then the gloves on the handles from the shoulders at full turn and pitch
(no stretch, wrists bent), takes and let-goes, the buzz stopping at the overheat, the crosshair gone, then Uprising's
Humvee from the climb-in to the Osprey.

## 8. The owner's questions, settled (2026-11-25)

1. On the truck, pushing the handles left swings the barrel right, as on a real post-mounted gun: **as is**. The gun goes
   where the hands put its handles.
2. Turning past the arms' reach: **the right stick, as on foot**; the body turning by itself at the edge (`mountpush`)
   stays off by default (built in the law, `mountgun::PushTurn`, off).
3. **Either trigger fires while its grip holds a handle.**
4. (The review's sixth finding, mine to settle with question 2's answer) The truck's stick turn is an orbit of the eye
   about the post: **a smooth turn as on foot, held to 0.75 m/s of the eye** (68 degrees a second on the truck), not a
   snap orbit the mod doesn't have. For the owner to feel at step 7.19.
5. (The second review's third finding, mine: players first) When the steering stops itself, the seat goes back to
   **today's behaviour for the rest of that seat** (the stick turning the turret through the mouse, the trigger firing),
   said in the log, never a gun nothing turns; and a push into a pitch stop that the turret doesn't answer is a stop
   found, never a fault (a row a little off, or the truck on a slope).

## 9. Not verified

Everything in sections 2 and 3 is read from the dump and the code. The pivot, camera and weapon places are the data's
component transforms summed at rest; the handles, the seat's draws, the turret's response, the seat's place in memory,
the climb-ins' lengths and which seat E gives on the truck are not known until section 6 is run. Uprising's time on the
gun is the data's chain of events with one runner's path in it, not a measurement. The steering is tested against a
simulated turret built from the dump's numbers as read (a rate input approached a part of the way each tick); the game's
own equations for its rotation bodies are not in the dump, and R5's step responses are what the law's rows come from.
The guards' thresholds (half a degree from rest, a tenth of the asked turn twice running, a quarter against it, ten
degrees in which a stop may be found) are set against that turret and the reviews' probes, with no false stop in t21's
134 runs; the game's own turret may show what the simulation doesn't (a stop that rebounds, camera noise at rest), and
section 7's step 12 (the films and `mountlog`) is where that shows. A no-answer at the least input alone says the least
row is too small.

How it was read: the vehicles' and weapons' files fetched from the dump (`Vehicles/technicaltruck/*`,
`Vehicles/HumveeArmored/*`, `Vehicles/common/1pOpenGunner_CameraConfig`, the open gunner's pad curves, the three
`Mounted_*_Firing` files), the levels' and `Input/*` files from a local copy of it, by three small readers in the
session's scratch folder (`mounted_a6`; **not in the repository**, a working aid only): a vehicle's component tree with
its offsets summed (it must skip excluded parents: section 2.5), a seat's input bindings per concept, and a level's
event graph (the earthquake area's in `eq_events.txt`).

## 10. The review of 2026-11-25 and what became of each finding

| # | Finding | Resolved |
|---|---|---|
| 1 (high) | The right stick already turns the turret through the mouse when the look writes stop (`g_turnDirect` false: `SetMouseTurn`/`TurnThread` send mouse X, the ring's turn); built as written, the seat's quiet look would have sent the stick to the turret, fighting the steering and turning it in the climb-in | Section 3 names it; 4.5 and 4.8: in the seat the stick is always taken directly (`mountgun::StickDirect`, wired in xr_runtime.cpp's `input::UpdateTurn` call), its turn to the body; q3: 2 s held, no count sent (today's routing: 2160); R2 expects it and asks whether today's DLL already finishes Uprising |
| 2 (high) | Seats no row matches were "Seat, scripted": Rock's ride, Guillotine's and Kaffarov's cars, The Great Destroyer's police car would have lost the look and the free hands | 4.1: only a seat told for a gun by measured rows, every row it needs measured and the runtime built, is ever one; every other seat exactly as today; s2-s7 (a passenger's seat, the ride, the police car, the truck with rows unmeasured, one row missing, the runtime not built), s10 the first table shown failing |
| 3 (medium) | The Humvee's pad curves are the truck's (the "none" was an excluded duplicate's) | 2.5 corrected; one curve for both (`kGuns`' dump rows, r4); R5 measures the pad's table at the curve's knees through the game's own dead zone (four points missed half the rate at the top) |
| 4 (medium) | The learned gain can't survive a flipped sign, the dead zones, the momentum threshold or the coast | 4.6: the guards (a wrong sign by the turret's acceleration against a hard push, no answer: the axis stops, said; t7, t9; without them t8 runs into the stop); the least input held through its delay and lag, a standing error under its coast, never a hunt (t5); the strength learned only in sustained one-way pushes (t10; the first learner off by up to 26%, t11); the profile's keys to scale the rows (with the outputs); R5 with the invert run and the least step; the mouse by default. **Not closed by that build**: its guards armed on hard pushes only, so a small first move with the invert on still ran the pitch into its stop (the second review's second finding); closed in 10b |
| 5 (medium) | The take offsets kept in the world frame; the let-go on a raw distance | 4.4: the offsets in the holding frame (the hands' line, or the body), turned with the stick's orbit (h10: 0.0004 degrees; the world frame 11.5 off, h11) and a torso's turn; the let-go after the offset (`Apart`). Found building it: on the Humvee a seated torso's turn overturns the gun and leaves the hands up to 42 cm from the handles (h12): R9's turn-centre row, the Humvee's step 2 let-go waits on it. (The second review: the truck too; now R10's row for both guns, 10b) |
| 6 (medium) | The truck's stick orbit at the on-foot 266 degrees a second carries the eye sideways at 2.9 m/s; "snap if set" doesn't exist | 4.5: the eye held to 0.75 m/s (68 degrees a second on the truck), the snap removed; section 8 question 4 (o1) |
| 7 (medium) | R5 can't tell the barrel's line from the render camera's | R5's burst with the view turned 30-40 degrees from the barrel; `kRowBarrelLine` a steering row |
| 8 (low) | The camera leans with the steering input: Y from it each frame wobbles the eye | 4.2: Y once at the climb-in's end (`BodyAtClimbEnd`); R4 fits the frames with no input |
| 9 (low) | The game's camera rows aren't kept by default | 4.2: the seat's view keeps its own copy, always (with the view, not built) |
| 10 (low) | The left trigger doesn't zoom today | Section 3 corrected; the seat's gate holds the right button up anyway |
| 11 (low) | 600 m/s, not 350; `OverHeatThreshold` 0.01 | 2.4 and 2.6 corrected |
| 12 (low) | R3's chain needs its last step; start at 0x380; no word holds the stick left | R3's command corrected; R2 uses F17 (right); `mountprobe` with the outputs (4.12) |
| 13 (low) | The quiet list missed the weapon buttons, the head's crouch, the left stick's click, F and Q | 4.8: one gate on every mapping but the fire, the use and the recenter, wired (input.cpp's loop, the stance, X and the D-pad, the walk, the quick-time grips: F and Z are the grips' quick-time keys; the mod sends no Q anywhere) |
| 14 (low) | Step 1 makes firing need a squeezed grip | The owner's call (question 3): it does; the README and CHANGELOG line when it ships (4.7) |
| nit | The scratch readers aren't in the repo; PLAN A6's "overpass MG" | Section 9 says so; PLAN A6 says the overpass gun is the bridge's M249, a gun in the hands |
| outside | `docs/research/missions/uprising.md` published the local scratch path | Replaced by the short form destroyer.md uses |

## 10b. The second review (2026-11-25, of 1e2db2b) and what became of each finding

That review built the merge base, the branch, main and a trial merge with every offline program, and ran probes of its own
against the law (a read stuck at the start angle, a read flipped, scaled or a frame wrong, the rows a tick or three lags
off, the invert on with small moves, pushes into a stop short of its row, a seated torso's turn, a lost hand). On foot
nothing changed, and the first review's two high findings were closed. What it found, and what became of it:

| # | Finding | Resolved |
|---|---|---|
| 1 (high) | A read that is fresh but doesn't follow the turret (a camera copy from another camera, stopped updating, or following the head bone) ran the turret away: both guards armed on hard pushes only, so a small error never tripped either (0.5 degrees of error: the turret 28 degrees on in 20 s; 13: 723); `Learn` silently dropped a zero answer | 4.6: every push held one way is judged against the turn it asked, whatever its size, window by window (under a tenth twice running: no answer) and, from rest, at once; t17 (0.25 to 40 degrees off, both inputs: stopped within 2 s, the turret 1.5 degrees on at 0.5 of error, 43 at 40), t20 (a deaf game at 0.5 degrees); the review's own probe A run again: stopped every time |
| 2 (medium) | Finding 4 of the first review not closed: with the game's vehicle invert on, a small first move ran the pitch into its stop and held it there sending every frame; the yaw went 29 to 32 degrees the wrong way | `kRowInvert` (R5's invert run) and `SignOf`: the pitch's sign flipped for a player whose key differs (r10); a push from rest judged at once, the first move probing the sign (the review's "probe at the take"); the windowed judgment catches a gun pinned at the other stop; t18: 0.5 to 15 degrees, the pitch at its 20-degree stops or the yaw free, both inputs: stopped after 0.9 to 5.8 degrees the wrong way |
| 3 (medium) | A guard's stop left a dead gun: the state stayed "on the gun", the quiet list on and the stick off the mouse, so that axis couldn't be turned at all for the rest of the seat | `mountgun::SeatSteer` and `StopSteering`: a stop on either axis ends both axes' steering and the next `SeatStep` puts the seat to Other seat, today's behaviour, till it's left, said in the log (s11); section 8 question 5 |
| 4 (medium) | The yes-or-no rows passed whatever they said (the rounds along the barrel, the left button firing, the camera rigid on the gun) | `RowKind::yes`: `Ready` asks for the yes and names a row measured as no (r7); the camera's rigidity is the steering's row too (it reads the game's gun off the camera) |
| 5 (medium) | A seated torso's turn parts the hands from the handles on the truck too (13 cm at 20 degrees, 34 at 45; one hand lopsided), not on the Humvee alone | 4.4 rewritten for both guns with h12's numbers (both hands and each alone); `kRowTurnCentre` a row for both guns (R10, in the headset), step 2's hands and their let-go by distance waiting on it |
| 6 (low) | The aim search's view tilt (24 counts of mouse X and Y) wasn't gated | aim.cpp `TiltView` sends nothing in a gun's seat (4.8) |
| 7 (low) | The M320's shot wasn't gated and its grip flag went stale in the seat | hands.cpp `M320HeldBy` says no hand on the launcher in a gun's seat, so a trigger there is never the 40 mm's (a shot already under way finishes as before) |
| 8 (low) | The headless paths (`HeadMoveAlone`, `FireAlone`) weren't gated, so a film couldn't prove the seat's rules | gated as `Sync` is: no Left Ctrl, no walk; a film's trigger by the seat's fire rule (no grip on a handle: nothing fired, said) (4.8) |
| 9 (low) | One unread frame dropped the seat to on foot and restarted the climb-in | a read missed holds the seat 0.25 s; only a read saying no vehicle or another seat ends it at once (4.1, s12) |
| 10 (low) | A lost hand's place fed the change of holding frame (30 cm off: the other's target 7.6 degrees; (0, 0, 0): 10.3) | `PairHeading` asks both hands tracked; a lost holding hand changes the frame from the last frame's heading (h15: nothing moves) |
| 11 (low) | Half-filled rows passed (a measure's numbers defaulted to 0; `Ready` read the first only; a 4-point pad table's most became 0) | every number not filled is not a number; `RowState` asks every number a row needs, a table's pairs whole and rising with no gap (r8); `PadTableOf` stops at the first pair not filled |
| 12 (low) | Numbers from no source in the law's interface (the `AxisRows` defaults, the `Geometry`'s pitch default, the climb-in's 6 s) | `AxisRows` and `Geometry` default to not a number and are refused (`RowsReady`, `Solve`); `AxisRowsOf(gun, axis, way)` tested (r9), the per-way rows split out (`kRowPadAnswer`, `kRowMouseAnswer`); the climb-in's length a row (`kRowClimbIn`, R7) |
| 13 (low) | q3 and s10 re-implemented the stick's routing inside the test, so nothing offline exercised the high finding's closure | the rule moved into `mountgun::RouteStick`, `StickAmount` and `MouseTurnRate`, which input.cpp's `UpdateTurn`, `TurnAmount` and `SetMouseTurn` now call; q3 runs them (s10 shows the first table's states, which need no routing) |
| 14 (nit) | PLAN.md A6 still said step 1 turns the gun by the right hand's aim or the stick; `mountseat::Frame` read the player chain every frame after the reader was off | PLAN.md A6 says the two grips and the right stick; `Frame` reads nothing once the reader is off for the run |

Found settling them: a push into a pitch stop that the turret doesn't answer is now the stop found, the target held inside
it (a row a degree or two off, the truck on a slope: its pitch is held in the world); before, that was either no stop at
all (a push held into the stop forever) or, with the new guards, a false stop (t19). And no false stop anywhere in the
review's own probes F and G or in t21's 134 runs.

## 11. The rows: every number the build needs, and where it must come from

None is measured (2026-11-25: the first run of the runbook, "fifty", did not reach the truck; read again at this build's
end, its report and its logs, `build\logs\bf3vr_1125_fifty_part1.log` and `_part2.log`, have no seat, vehicle or
"Mounted gun" line: nothing to fill). The code refuses on the first row a need names (`mountgun::Ready`, `RowState`,
`seatread::Prove`), and `src\mountseat.cpp` says it once a run in the log ("Mounted gun: the gunner's seats work as before:
the seat isn't read (... not measured yet: ...)", then each gun's first row and why: "not measured yet", "measured, but
not every number it needs", "measured, but its table doesn't rise" or "measured as no", then "the seat's own view and the
steering's outputs aren't built yet").

The seat's reader (`seatread::kLayout`, `src\seatread.h`), all R3, in any seat:

| Row | Expected | From |
|---|---|---|
| the player's attached controllable (ClientPlayer +?) | a pointer, the vehicle in the seat | `dwords.ps1 -Chain 1F80B58,30,BC,0 -Start 0x380 -Length 0xC0`, on foot and in a seat |
| the player's entry index (ClientPlayer +?) | 1 in the gunner's seat | the same dump |
| the vehicle's type name | ClientVehicleEntity or its kind | the vtable's getType (`owner.ps1`) |
| the vehicle's data (+?) | its VehicleEntityData | `fields.ps1` on the vehicle |
| the vehicle data's type name | VehicleEntityData or its kind | its getType |
| the vehicle data's name (+?) | a name (TechnicalTruck_Restricted, HumveeArmored_hmg?) | the data's name field |
| the code that reads the attached controllable; the code that reads the entry index | their bytes and addresses | `findcode.ps1` on `build\bf3_mem.exe` |

Each gun (`mountgun::kGuns`, `src\mountgun.h`), the truck and the Humvee alike, each measured on its own gun; every number
a row has must be filled (a table: two to six pairs, rising); the dump's figure only to check against:

| Row | Its numbers | Needed for | The truck's dump figure | The Humvee's | From |
|---|---|---|---|---|---|
| the gunner's entry index | 1 | telling the seat | 1 | 1 | R1, R3 |
| the vehicle's name | a text | telling the seat | (R3's) | (R3's) | R3 |
| the pivot from the ring's axis | 3 (m) | telling, the view, the steering | (0, 0.173, -0.024) | (-0.010, 0.495, 0.796) | R4 |
| the camera from the pivot | 3 (m) | telling, the view, the steering | (0, 0.202, -0.608) | (0, 0.268, -0.978) | R4 |
| the camera rigid on the gun | a yes (1) or no | the view, the steering (a no keeps both off) | (none) | (none) | R4 |
| the pitch limits | 2 (degrees) | the steering | -20, 20 | -35, 35 | R4 |
| the left handle | 3 (m) | the steering, step 2's gloves | (0.139, 0.032, -0.307): the left hand's IK point | (none) | R6 |
| the right handle | 3 (m) | the steering, step 2's gloves | (none) | (none) | R6 |
| the gunner's eye | 3 (m) | the view | (0, 0.375, -0.632): the camera at zero pitch | (-0.010, 0.763, -0.182) | R6 |
| the climb-in's length | 1 (s) | the view (the climb-in's end at most it and a second) | (none) | 4.33 | R7 |
| the turret's time to speed | 2: yaw, pitch (s) | the steering | (none) | (none) | R5 |
| the mouse's answer | 5: its delay to the camera's first turn (s), its X's sign on the yaw, its Y's on the pitch, the least count step that turns each (a share of its most) | the mouse's way | (none) | (none) | R5 |
| the pad's answer | 5: the same for the right stick | the pad's way | (none) | (none) | R5 with `mountprobe` |
| the game's vertical invert | a text (the profile key, or "none") and 2: its value during the sign steps, which pitch input it flips (1 the pad's, 2 the mouse's, 3 both) | the steering | (none) | (none) | R5's invert run |
| the rounds along the gun | a yes (1) or no | the steering | (none) | (none) | R5's burst, the view turned away |
| the left mouse button fires | a yes (1) or no | the steering | (the binding: yes) | (yes) | R2 |
| the yaw's and the pitch's rate at the stick | 2 to 6 (stick, degrees a second) pairs each | the pad's way | the curve's dead zone to 0.15 | the same | R5 |
| the mouse's X and Y | 2 each: degrees a count, the most counts a second | the mouse's way | (none) | (none) | R5 |
| the gunner's arm draws | 2 | step 2 | (13755/15051 expected) | (the same) | R6 |
| the seat's crosshair draw | 1 | step 2 | (none) | (none) | R6 |
| the hands' turn's centre | 1: 0 the fit about the ring's axis as built, 1 the hands' turn about the body taken as the gun's | step 2's hands and let-go | (none) | (none) | R10, in the headset (4.4) |

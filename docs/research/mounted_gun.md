# The mounted gun: Swordbreaker's truck .50 and Uprising's Humvee M2 in VR

The owner, 2026-11-18, on the end of Operation Swordbreaker ("Get on the 50 cal"): "held with both hands ... two grips
held ... together and close ... move both of their arms ... at the same directions, like mirroring each other, so it can
move the machine gun". The same rule holds for Uprising's Humvee .50 (PLAN.md item A6, Uprising item B2).

This note is lane A6's design, written 2026-11-25 from the public Venice-EBX dump and the mod's code on main (85d59d2).
**Nothing here was run in the game.** Tags: [data] read from the dump; [code] read from the mod's source; [inferred] my
reading of the two; [measure] what the runbook (section 6) must settle in the game before a line is built on it.

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
  server simulates the turret from the input, so shots land where the gun is drawn. [inferred, measure]
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
- Rounds start at the weapon component's point (2.2) along the pitch body's forward, 350 m/s, spread 0.3 to 1.0 degrees
  (+0.4 a round, -5 a second). [data]

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

The truck's gunner has pad curves (`PadInputCurve_OpenGunner_Roll`/`_Pitch`): a dead zone to 0.15 of the stick, then
0.03 at 0.25, 0.066 at 0.75, 0.125 at 0.875 and the most, 0.2, from 0.95. The Humvee's gunner has none: the stick goes
in as it is. So the same stick turns the two turrets at very different speeds; the mouse's speed is the player's
vehicle mouse setting. [data]

### 2.6 Fire, heat, reload

Both guns: 550 rounds a minute, automatic, endless ammunition (`MagazineCapacity` -1), no reload in practice; heat 0.075
a round, cooling 0.5 a second, a 1.2 s penalty once hot. At 550 a minute the heat climbs about 0.19 a second: about
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
- **The right stick.** In stereo its turn goes into the written look yaw (input.cpp's comment over `g_stickTurn`), which
  aims nothing in the seat; mouse aim alone would carry it to the turret. [code]
- **The trigger.** With no gun profile the free hands sleep, `hands::FireHand` is the right, and the right trigger sends
  the left mouse button: the gun should fire. [code]
- **Stray keys.** The stick's stance flick sends X (countermeasures on this seat: none) and Z (unbound); a squeeze at a
  shoulder sends 1, 2 or 4 (1 and 2 select the seat's weapon slots, 4 is unbound); the use button E (get off, locked
  here); the left trigger the right mouse button: **the zoom**, which narrows the game's view to 22 degrees. [code +
  data]

So today's seat is probably a fixed gun that fires straight ahead, with the view turning when the turret does. Whether
Uprising can be finished that way is the first thing R2 answers (the plan's B2).

## 4. The design

### 4.1 States

| State | When | What runs |
|---|---|---|
| On foot | no seat | everything as today |
| Seat, scripted | the first moments in the seat (the climb-in) and whenever the game's camera turns by itself with no input of ours (as aim.cpp's scripted test does for the look); also any seat the reader can't place in a row | the cutscene rules: the game's body and camera, level, head free, no hands of the player's; the seat's quiet list (4.8) already on |
| Seat, gun | in a known gunner's seat (a row), the climb-in over | sections 4.2 to 4.11 |
| Other seat | a seat that isn't a mounted gun's (Rock's ride, a future driver's seat) | as today: never changed by this work |

**The seat reader** (a new `seatread.h`, written as poseread.h is: pure, every constant borne out at startup by the
game's code bytes and type names, read through input.cpp's guarded reader, heap objects only, AMD-safe): from the local
player (god.cpp's chain, `[[[bf3+0x1F80B58]+0x30]+0xBC]`) the controllable it is attached to, checked by its vtable's type
name (a `ClientVehicleEntity` or its kind), and the entry index. The offsets come from R3. Its proofs failing turns the
whole seat work off for the run, with one log line: the mod then behaves as today.

**Which gun (the rows).** A row per gun: the ring axis point, the pivot and camera offsets, the handles' points and
their grip frames, the pitch limits, the eye's place, the actuator's defaults (2.2's numbers, the handles and the eye
from R6). A seat is matched to its row by the level, the entry index and the game's camera offset from the pivot
(0.61 m back and 0.20 up, or 0.98 and 0.27), so a third vehicle with the same seat type is never taken for one of these.

**The climb-in's end.** Seat, scripted ends when the game's camera has kept still for 0.5 s with nothing of ours sent,
at least 1 s and at most 6 s after the seat appeared (Uprising's scene is 4.33 s; Swordbreaker's is timed in R7). At
that moment the body takes the game camera's yaw (4.2), so nothing jumps.

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
- **Y in the world**: from the game camera each frame, Y = C - Ry(ψ)·(p + Rx(θ)·c), with C its place, ψ and θ its yaw and
  pitch and p, c the row's offsets; exact while the camera is rigid on the pitch body (R4). If R4 finds the camera
  animated, Y is taken once at the climb-in's end (both vehicles stand still) and from the vehicle's transform if one
  is ever needed for a moving seat.
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
- **Let go**: the grip released, or the controller 25 cm from its handle (the mod's other `kHandleLost`-style keep
  distances). The glove then goes back to its controller. The gun stays where it was: a mounted gun does not fall.
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
  longer than the gunner's still moves it as far as his hands move.
- **Each frame** the gun's target is the ψ, θ that put the held handles nearest to (hand_i - d_i): with both hands, the
  handles' midpoint at the hands' midpoint (closed form, then one correction step); with one hand, its handle at its
  hand (two Gauss-Newton steps from the last answer). θ inside the pitch limits. The hands' spread and roll are not used:
  the gun cannot roll or stretch.
- This is the owner's rule for both shapes: on the Humvee, both arms swinging left round the body turn the gun left; on
  the truck, both arms pushing left swing the barrel right about its post, as a pintle gun does. Up and down likewise
  (on the truck, handles up, muzzle down).
- **Smoothing**: a 1 cm hand wobble at a 0.3 m lever is 2 degrees of gun. The target goes through a speed-dependent
  low-pass (steady when the hands are slow, no lag when they sweep); with two hands the midpoint already halves the
  wobble. The game's own bodies add their weight on top.
- **Reach**: there is no arc. The gun goes as far as the hands can carry the handles: on the Humvee, as far as the player
  turns himself (a seated player about 90 degrees each way, a swivel chair all the way); on the truck, 60 to 90 degrees
  each way before the handles are out at arm's length to the side. The right stick does the rest.

### 4.5 Turning further: the right stick

- The right stick's turn (smooth at the mod's turn speed, or snap if set, exactly as on foot) turns the body about the
  ring's axis and the gun's target with it, so the handles stay in the hands. On the Humvee that is a turn in place;
  on the truck an orbit about the post (the gunner walking round his gun).
- The stick's down and up do nothing in the seat (the stance is locked anyway).
- **Option for the owner to try, off by default**: `mountpush 1`, the body turning by itself while the hands hold the gun
  past 50 degrees from the body's front (at most 60 degrees a second, more the further past). It saves reaching for the
  stick and costs a turn the player did not ask for in so many words; players-first says off until the owner says.

### 4.6 Steering the game's turret

Each frame on the render thread, while in Seat, gun:

1. **Where the game's gun points**: the yaw and pitch of the game's camera rows as the game made them, before the mod's
   turn (camera.cpp keeps them for the view build: `NoteBuildPose`'s `made`). A camera older than 100 ms: send nothing.
2. **The error**: e = target - game, the yaw wrapped, the pitch's target clamped to the gun's limits. Under 0.15 degrees:
   nothing sent.
3. **The demand**: a turn rate u = K·e plus the target's own rate (so a steady sweep is followed without a standing lag),
   cut to what the input can give. K from R5's step responses, about a third of what would close the gap in one game tick
   (30 a second), so the game's own momentum never makes it overshoot.
4. **The input**, two ways behind one call, the better one chosen by R5:
   - **the pad's right stick**: pad.cpp already answers the game's XInput for controller 0 (its left stick walks); the
     right stick goes in the same answer. The demand goes through the inverse of the seat's pad curve (the truck's;
     the Humvee's is straight). Not read for 500 ms (the known stall): the mouse instead.
   - **mouse moves** (`SendInput`, as mouse aim does): counts from the demand. Only with the game's window in front, as
     today.
   Either way the gain is learned as it goes: the camera's turn over the last half second against what was sent, kept
   within half and twice the row's figure, so a player's own vehicle sensitivity setting cannot break it. Said once a
   second in the log while it steers.
5. **Nothing is sent** with no hand on a handle and no stick turn (the gun holds still), in Seat, scripted, with the
   game's window behind (mouse), or with the camera stale. A demand's last value is never left standing: the right
   stick goes back to zero on any stop, as the walk's left stick does.

The drawn gun follows the hands as fast as the game turns it. If R5 finds the turret much slower than a quick sweep,
the hands (drawn on the handles in step 2) lag with it, which reads as weight; past 25 cm the hand lets go (4.3).

### 4.7 Fire

- The left mouse button is down while the trigger of a hand that holds a handle is pulled (either hand, both triggers
  one button), with input.cpp's usual hysteresis and its release guard (`StepRelease`: a release the game never got is
  sent again, after Swordbreaker's roof, where the game kept firing by itself). A trigger of a hand not on a handle does
  nothing. In Step 1, a hand counts as on its handle when its grip is squeezed (there is no drawn take yet).
- `FireButton`'s seat branch takes over from `hands::FireHand` in the seat; nothing of the held guns' rules (slide,
  tube, launcher) applies there.
- The overheat is the game's: it stops the rounds and shows its bar. The buzz a round (the recoil foundation) goes to
  each hand on a handle at the game's 550 a minute while the button is down, stopped by a copy of the game's heat
  (0.075 a round, 0.5 a second down, 1.2 s penalty: 2.6) until R8 finds the game's own firing state to read instead.
- Never the zoom: the left trigger sends no right button in the seat. If the game ever zooms anyway, the mod's render
  field of view must win (R7 checks).

### 4.8 Quiet in the seat (all states of the seat)

| What | Why | Where |
|---|---|---|
| The look angles: no writes, no search, no nudges, no mouse aim | they aim nothing here; a nudge in a vehicle is an unknown | aim.cpp, a seated flag as its scripted one |
| Holsters: no draws, no put-aways, no keys 1/2/4 | they would select vehicle weapons | holster.h, hands.cpp |
| Stance from the stick (X, Z) | locked by the level; X is countermeasures here | input.cpp `UpdateStance` |
| Jump (the stick's click, A) | nothing to jump | input.cpp |
| Walking keys and the left stick | the seat's throttle and yaw | input.cpp, pad.cpp |
| Grenades by hand, magazines, reloads by hand, the pouch | the soldier's own gear is not out | hands.cpp, pouch.cpp, ammo.cpp |
| Quick-time answers on the grips | the grips hold the handles; no prompt here | qtewatch.cpp: off unless a fight's prompt is open, as in play |
| The left trigger's right mouse button | the zoom | input.cpp |
| The held guns' recoil rows, the M320 | no held gun | recoilrun.cpp, m320.cpp |

The right stick's click (recenter when held) stays: in the seat a recenter puts the room's front at β and the head at the
eye.

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

- weapon.txt `mountlog N`: N seconds of one line a frame (the seat, the hands' targets, the game's angles, the error, what
  was sent and by which way, the learned gain).
- weapon.txt `mountsteer pad|mouse|auto`, `mountpush 0|1`, `mountseat 0` (the whole seat work off: today's behaviour).
- build\freehands.txt `test N` gains seat tests (scripted hands taking the handles, sweeping, firing), so the films need
  nobody in the headset, like the holsters' tests.
- No player-facing setting: the rows' numbers are measured, the gain learned.

### 4.13 Never break, players first, AMD

- Unknown seats, a failed proof, a row that doesn't match: today's behaviour, said once.
- Leaving the seat restores every rule; on-foot code paths are untouched when no seat is read (one boolean read a frame).
- Reads: the local player and its vehicle are heap objects, vtable-checked before every use; the game's camera rows are
  the ones camera.cpp already copies on the game's thread. Nothing the game mapped from the graphics card is read; no
  wait on the card. Only the right stick (or the mouse) is new output, and it is zero whenever the seat rules aren't
  steering.
- The frame cost: one 2x2 solve and a few vector sums a frame.

## 5. Build steps

**Step 1, *finish* (M, about a day after R1-R5):**
- `src\seatread.h` + `tools\seatreadtest.cpp` (made-up image as stancetest has): the reader and its proofs.
- `src\mountgun.h` (pure) + `tools\mountguntest.cpp`: the rows, the states, the handle law (4.4) with relative takes,
  the stick's orbit (4.5), the steering's demand and gain (4.6) against a simulated turret (rate input, the dump's
  damping, a 30 Hz tick, a tick of delay: converges without overshoot, saturates cleanly, sends zero when it must), the
  fire rule, the quiet list.
- `src\mountgun.cpp`: each frame's read, the body and eye to xr_runtime.cpp, headmove.h and camera.cpp, the demand to
  pad.cpp (right stick) or `SendInput`, the fire to input.cpp; the gates in aim.cpp, input.cpp, hands.cpp, holster.h,
  qtewatch.cpp.
- Hands: the game's own (4.3). A hand counts as on its handle while its grip is squeezed.

**Step 2, *full* (L):** the gloves drawn on the handles from the player's shoulders (skin.cpp's arm placement with a
handle as the target), takes within reach and lets-go by distance, the buzz a round, the hidden crosshair, `mountpush`,
the reach-and-grip entry with A15. Every offline test program, at Idle priority, before each
merge.

The work shares its first game session with the plan's B2 (Uprising) and Swordbreaker's end; the tank's roof .50 later
takes a row.

## 6. The runbook: what must be measured in the game

Rules for every round: the owner not playing and nobody in the headset (or the owner's word); the owner's live save
backed up first (`build\saves\user_live_<date>_<time>`, hashes kept) and put back after; weapon.txt in its hand-over
state after every film (`standin 0`); every compile and offline test elsewhere at Idle priority while the game runs;
memory tools read-only (`tools\dwords.ps1`, `fields.ps1`, `owner.ps1`, `findcode.ps1`, run in the 32-bit PowerShell);
loading a mission from Replay Missions moves the campaign's resume point: say so first. Swordbreaker's gun is reached from
the owner's checkpoint after the cellar fight (the bridge fight, then the meat market; the truck at (-610.4, 78.4,
-2091.8)); Uprising's needs the first run of that mission (B1) to reach the Staging Area.

**R1. Getting there, and the seat it gives.** Walk to the truck with whatever gun is in hand; press the use button
within 5 m. Log:
the "Views:", "Aim:", "Free hands:" and "Input:" lines; a screenshot each second. Settles: which seat E gives (gunner or
driver), how long the climb-in takes, the objective's success.

**R2. Today's DLL in the seat, nothing changed.** 10 s still; then the right hand turned left and right (the stand-in's
hand turned, as the aim films do); the right stick held left 2 s; the right trigger 1 s; the left trigger once.
`viewlog 10` through it. Settles section 3: the camera's kind (scripted or the player's), whether the look search nudges in the seat (if it
does, stop and note: step 1 must gate it first), whether anything turns the turret (mouse aim, the stick), whether it
fires, whether the left trigger zooms the headset's picture. **This decides whether Uprising's *finish* waits on step 1.**

**R3. Where the seat is in memory.** `dwords.ps1` on the local player (god.cpp's chain, `-Chain 1F80B58,30,BC -Start
0x3C0 -Length 0x80`; check how the tool takes a chain's last step first) on foot and in the seat; the words that change:
a pointer whose vtable's type (slot 0's class info, the way poseread names them) is the vehicle's, and a small integer
(the entry index, 1 expected). Then `findcode.ps1` on the image for the
code that reads those offsets, for the proofs. Check that `+0x3D8` (the soldier) stays the same in the seat and that
poseread's read of it still works.

**R4. The camera and the pivot.** With `viewlog`, the game camera's rows and place per frame while the turret is turned
by known mouse moves (a script's `SendInput`, the game in front): a full circle of yaw and the pitch from limit to limit.
Fit the circle and the arc: the ring's axis point and the pivot in the world, the camera's offsets (2.2's 0.61/0.20 and
0.98/0.27 expected), its forward against the barrel's line. Settles: rigid on the pitch body (Y from the camera each
frame) or animated (Y once at the climb-in's end).

**R5. How the turret answers.** Step responses, each held 1 s from rest, the camera's yaw and pitch per frame:
- mouse: 20, 50, 100 and 200 counts in one frame, each axis; and a steady 5 counts a frame;
- pad right stick (needs step 1's pad output behind a test word): 0.25, 0.5, 0.75 and 1.0 of the stick, each axis.
Settles: degrees per count, the rate at each stick (the truck's curve and the Humvee's straight line), the time to
speed and to stop, the most turn rate, the game tick's delay. From these: K, the curve's inverse, which way to steer by
default. Also: fire a burst at a wall with the turret at a known angle and check the hits are on the camera's line (the
server turns as the client does).

**R6. What the game draws in the seat.** `tail` for one frame in the seat, and the draw hunt if needed:
- the arm draws' keys (the glove draws 13755/15051 and their depth pass expected), the views they are drawn with
  (first-person, in the arms' space, or world), their bones;
- the gun's draws (the M2 and its shield, the truck's gun) and where their place comes from;
- **the handles**: the game's own palms (the gloves' roots) in the seat at four turret angles and three pitches, against
  the camera: each handle's point and grip frame in the pitch body's frame (the truck's left one against its IK point,
  2.2), and where the gunner's head is against the camera;
- the seat's HUD draws: the crosshair, the heat bar.

**R7. The scenes and the zoom.** The climb-in's camera (still, moving, scripted kind?), its length on Swordbreaker; one
press of the right mouse button in the seat, and what the headset's picture does; Swordbreaker's end: the truck's motion
from `viewlog`'s places, the frame the seat goes, the end scene; Uprising's rocket: fade, the move out, the get-up, the
Osprey.

**R8. The firing state** (step 2's buzz): the vehicle weapon's heat and its firing flag, by `fields.ps1`/`owner.ps1` on
its weapon object (from the vehicle's weapon component), checked against a 6 s burst to overheat.

**R9. Uprising's Humvee**, once B1 reaches the Staging Area: R1 to R7 on it, plus the shield: the M2's shield turns with
the ring about a line 0.18 m in front of the eye, so with the body held still and the gun turned 60 and 90 degrees, look
for armour passing through the head.

## 7. In-game checks after each step (nobody in the headset first, then the owner)

Step 1:
1. Swordbreaker, the truck: on with the use button; the climb-in as a scene, then the seat view still in the room while
   the turret moves; the head looks about freely; the horizon level.
2. Both grips squeezed and swept (the seat test's scripted hands): the gun follows both ways, up and down, no
   overshoot, no drift with the hands still; one hand alone; no hand: the gun holds.
3. The right stick turns the player and the gun together.
4. Either trigger fires only with its grip squeezed; the overheat stops it; no lone release left (the button up after).
5. Nothing stray: no stance key, no holster key, no zoom, no jump, no look writes (the log's lines).
6. The earthquake and pull-out, the end scene, the mission's end; a death on the gun and its checkpoint.
7. The owner: the feel of the handles on both shapes, the truck's post (`mountpush` both ways).
8. The never-break list on foot after the seat: aim follows the gun, the view still, the stick turn's sign, walking,
   shots landing, both eyes, 72 fps, the grip holding, a reload by hand.

Step 2: the gloves on the handles from the shoulders at full turn and pitch (no stretch, wrists bent), takes and
let-goes, the buzz stopping at the overheat, the crosshair gone, then Uprising's Humvee from the climb-in to the
Osprey.

## 8. Questions for the owner (theirs to answer; the rest is decided above)

1. On the truck, pushing the handles left swings the barrel right, as on a real post-mounted gun. Try it as is, or would
   they rather the gun went the way the hands go (it would no longer stay in the hands)?
2. Turning past the arms' reach: the right stick only, or also the body turning by itself at the edge (`mountpush`)?
3. Either trigger fires, or the right only?

## 9. Not verified

Everything in sections 2 and 3 is read from the dump and the code. The pivot, camera and weapon places are the data's
component transforms summed at rest; the handles, the seat's draws, the turret's response, the seat's place in memory,
the climb-ins' lengths and which seat E gives on the truck are not known until section 6 is run. Uprising's time on the
gun is the data's chain of events with one runner's path in it, not a measurement.

How it was read: the vehicles' and weapons' files fetched from the dump (`Vehicles/technicaltruck/*`,
`Vehicles/HumveeArmored/*`, `Vehicles/common/1pOpenGunner_CameraConfig`, the open gunner's pad curves, the three
`Mounted_*_Firing` files), the levels' and `Input/*` files from a local copy of it; three small readers kept with the
session's scratch files (`mounted_a6`): a vehicle's component tree with its offsets summed, a seat's input bindings per
concept, and a level's event graph (the earthquake area's in `eq_events.txt`).

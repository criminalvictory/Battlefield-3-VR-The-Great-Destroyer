# Changelog

Every upload to GitHub gets an entry here: what changed since the last one, and the stage the
project is at. Stages: pre-alpha, alpha, beta, release. A change of stage is also marked as a
GitHub release.

## 2026-11-13: a gun off the ground comes to the hand, and the D-pad's right is the magazine button (stage: pre-alpha, experimental)

Tried in the headset the night they were built: both work with real hands. Filmed once with scripted hands before that (six takes off the ground by both hands,
standing; crouched and prone filmed, not yet read frame by frame). The D-pad's film is still to make.

- **A gun lying on the ground comes to the hand** (`src/grasp.h`, `src/hands.cpp`, `src/freetune.cpp`, `src/input.cpp`): seated, the ground under a standing soldier is
  out of reach, and the take only answered within 25 cm of the gun. Now a gun at rest answers a fresh squeeze from a hand within 35 cm of it in any direction, or
  from a hand lowered beside the body (half a metre under the neck or more) up to 1.2 m above it and about half a metre around; it flies to the hand in 0.13 to
  0.25 s by its pistol grip, with the game's own weapon-handling sound (`tools/gamesounds.cpp` cuts the clip from the game's files). Either hand, every gun that can
  lie. Nothing is taken without a fresh squeeze by a hand the headset sees steadily; a gun in flight is caught as before, within 25 cm. The game's dropped guns get
  the same take when that feature is on (it stays off). `build\freehands.txt`: `magnet 0` is the old take exactly; `magnet` and `snap` with numbers set the reach
  and the speed; `lowsqueeze 1` asks for the squeeze to be made with the hand already low. Offline: rows 60 to 73 of `tools/grasptest.cpp` (92 checks), and the old take compared
  frame by frame over 46481 frames with the magnet off.
- **The D-pad's right is the magazine button with the rifle in the left hand** (`src/ejectkey.h`, `src/input.cpp`, `src/hands.cpp`): on the Steam Frame X is on the
  right controller, the other hand's once the left holds the rifle by its pistol grip. There the left controller's D-pad right now does all X does: a tap drops the
  magazine, held it is the game's own reload. X is unchanged, the D-pad's right does nothing with the rifle in the right hand, and controllers without a D-pad are
  as they were. `build\freehands.txt` `dpadeject 0` switches it off, `2` gives it to whichever hand holds the rifle.
- **Later that night: a gun taken off the ground is a gun like the player's own** (`src/ammo.cpp`, `src/pickrules.h`, `src/hands.cpp`, `src/recoilrun.cpp`,
  `src/input.cpp`, `src/pickups.cpp`): the game keeps a picked-up gun in whichever weapon slot has room, and the counts, the reload by hand, the trigger's
  stop, the recoil and the vibration only went by the first slot. Tried in the headset: a dead soldier's AN-94 reloaded by itself and had no recoil. Now the
  gun in the hands is followed into its slot, a reload the game starts by itself is undone, and a fired kept-back round is put back at once. Filmed on real
  dropped AN-94s with real rounds through the mod's own trigger (`weapon.txt` `filmtrigger`): fired to empty the trigger stops and nothing reloads, the recoil
  and the pulses asked for name the right gun, the magazine by hand and the lever work with the gun's own counts. Not filmed: a fresh drop fired with real
  rounds (the film's aim never settled with it), and the vibration as felt. The dropped guns stay off by default (`build\pickups.txt` `level 2` turns them on).
- **A dropped gun's own attachments on the ground: built, not yet seen** (`src/pickparts.cpp`, `src/pickpartsrules.h`): the game keeps a dead soldier's
  attachments in his dropped gun and draws only its bare body, so an optic appears once the gun is in the hand. Behind `build\pickups.txt` `parts N`, off by
  default: each drop's unlocks are read and their meshes made with the game's own calls. In its first film nothing leaked or crashed, and nothing was seen:
  the game's pickup renderer draws only each pickup's one body. Next: have that renderer draw the parts too.
- **Corrected, the next morning:** the night's film fired real rounds from one AN-94 only, the checkpoint's (in the game's second slot), not from a dropped
  one. The user's own try in the headset then did both: the checkpoint's AN-94 and a dead soldier's AN-94 taken by hand stopped at empty with no reload by
  itself, kicked at every burst and sent their vibration pulses ("Works right"). Still open: guns the mod has no hand hold for yet (the AKS-74u, SVD,
  shotguns) can still reload by themselves and have no recoil, and a picked-up gun's shots can land off where it points (being fixed).
- **The recoil's drift no longer switches itself off** (`src/recoilrun.cpp`, `tools/recoilruntest.cpp`, `tools/selftest.ps1`): a check meant to catch the
  game's own kick still in its data read the game camera's ordinary shake as that kick, and after about ten rounds turned the climb off for that gun until
  the next gun change. It now looks only for the camera climbing with the rounds, in strings of five or more fired with two hands, and switches the drift off
  only after three such strings in a row (and on again after three without). In the user's next play the AN-94's climb held through 54 seconds of firing.
  With nothing drawn, the hands' slot is now the game's own current weapon (`src/ammo.cpp`, `src/pickrules.h`).
- **Operation Swordbreaker** (the campaign's second mission) checked and played: the arms, the M4A1 by hand, its magazine and lever, recoil, the magnetic
  pickup and the D-pad's right all work as on Rock and a Hard Place. Not yet: the M4A1's holographic sight shows no reticle (the mod draws each sight's marks
  itself, and this one has none yet), the rooftop launcher can only be taken with its button, and the frame rate dips below 60 on the opening street.
- **README:** a Contributors section, naming DanInDev.
- **The holographic sight's reticle** (`src/skin.cpp`, `src/scopemarks.h`, `tools/scopemarks.ps1`, `src/gunbodies.h`, `tools/gunbodyfit.cpp`,
  `src/recoilrun.cpp`): the game draws the M4A1's EoTech reticle only while zoomed, which VR never does, so in the headset its glass was empty. The mod now
  draws it in the sight's glass itself, as it does the Kobra's: the red ring, its dot and four ticks, taken from the game's own zoomed part, at 1x, at
  infinity along the gun's line (it stays on the gun's line as the eye moves, and shows to the eye behind the glass). Its outline also joins the M4A1's
  shape on the ground and for the magnetic take. Filmed in both eyes with scripted hands; the user's first look in the headset came the same afternoon.
- **A picked-up gun's shots go where it points** (`src/aim.cpp`, `src/aimrules.h`, `src/skin.cpp`, `tools/aimrulestest.cpp`): with the enemies' guns the
  mod often hadn't found the game's look angles, so the shots went where the view looked rather than where the gun pointed. The search now compares each
  candidate with the camera as it walks, proves it as it is after its wait (no false "vehicle's turn"), remembers what worked for the soldier, and no longer
  tries a lost address again for 15 seconds after a take; mouse aim closes its last half degree; and a held gun with a scope or red dot fires along that
  sight's line, not 3 to 5 cm under it. `build\weapon.txt` `aimfind 0` and `heldshot 0` put back the old. Proven offline; on Swordbreaker the look was found
  within seconds of a hand holding the gun.

## 2026-11-12: a steady world when the head turns, a wider picture, every gun's recoil (stage: pre-alpha, experimental)

Tried in the headset: the steady world, the 106-degree picture, the recoil. The rest filmed with scripted hands or measured with a replayed head,
nobody in the headset. The dropped guns are built and switched off.

- **The world holds still when the head turns** (`src/xr_runtime.cpp`, `src/camera.cpp`, `src/headmove.cpp`, `src/skin.cpp`): each picture is handed to the headset
  under the head pose it was really drawn with (named by the view build most of its main-view uploads carried, not by its first), the pose follows the head faster
  the faster it turns, and the camera stands exactly where the head is. The log's "World turn" and "Edge" lines measure both in every session.
- **A wider picture at the same sharpness** (`src/config.cpp`, `src/views.cpp`, `src/scopeview.cpp`): with bf3vr.ini `[stereo] pixels_per_tangent` the angle follows
  `[window] size` (2400 pixels an eye is 106 degrees where 2160 is 100), `detail_fov` keeps the level of detail, `[world] shadowmap_resolution` keeps the shadows'
  edges, and `[world] motion_blur_passes=0` switches off the blur passes of a game whose blur amount is 0, which pays for most of the width. The black band at the far
  edges is smaller; covering the whole lens is for later. The vignette is held as it is at 100 degrees (`src/postpin.cpp`, `[stereo] pin_effects`).
- **Every gun's own recoil** (`src/recoil.h`, `src/recoilguns.h`, `src/recoilrun.cpp`, `src/hands.cpp`): the game's kick in the hands, harder in one hand; the
  launchers' kick is off until its timing is right.
- **Two crashes guarded** (`src/footsteps.cpp`, `src/framemem.cpp`): the game's 96-place animation list is kept from running over, and its memory for one frame's
  draws is 64 MB instead of 16 (`[world] frame_memory_mb`), with a meter in the log.
- **Three hand fixes, filmed:** the game's left arm whole on launcher-pose guns, hands in place when a gun is switched with the left hand kept, the hand's place
  at the handguard.
- **The game's dropped guns taken by hand: built, off** (`src/pickups.cpp`, `src/pickrules.h`, `[world] pickups`, off by default): the game hands the gun over,
  but it does not arrive in the hand yet.
- **Tests with nobody in the headset:** a head replay with its own judge (`src/headtest.cpp`, `tools/headfit.ps1`, `tools/headtrack.ps1`), pictures of two launches
  compared (`tools/cmpcentre.ps1`, `cmpshift.ps1`, `localshift.ps1`, `cmpbright.ps1`, `picdiff.ps1`), the self-test warning when the HUD's corner panels are
  missing. README: 72 Hz is the recommended rate.
- **Known:** the hands are one picture behind the gun at a weapon switch; with both controllers untracked a dropped magazine lies on until one is tracked again.
- **Later the same evening, a second upload:** the dropped guns' first run in the game was read and its causes corrected (`src/pickups.cpp`, `src/pickrules.h`,
  `src/hands.cpp`): free hands were asleep, then frozen, for the whole 1.5 s a take allows, so the gun never reached the hand; and the "round" that went off was a count
  left in the rifle, not a shot. Still off, not yet run again. The first upload's offline test of the take's rules failed (it took the new default of off): it passes again.
- **Late that night, a third upload: the sun's glare and the far mountains hold still** (`src/views.cpp`): the eyes are drawn from the game's own near plane (6 cm)
  again. Since 2026-11-05 the world had been drawn from 1 cm, for the eye pressed up to a sight. The depth buffer has 24 bits: at 1 cm one step of it is 6 m at a
  kilometre (1 m at the game's 6 cm), so the sun's glare flashed on and off and far mountains flickered as the head moved. Tried in the headset, 1 cm, 3 cm and
  6 cm by letter: flicker only at 1 cm, and nothing cutting through the sight at 6 cm. Measured with a replayed head, the two alternated within seconds: the glare
  steady in every burst at the game's plane and flashing in every burst at 1 cm; a far mountain's pixels changing from frame to frame, 0.7 to 0.9% against 4.8 to
  6.4%. `weapon.txt` `nearplane` still moves the plane for a test.

## 2026-11-11 (evening): free hands for every gun in the hands (stage: pre-alpha, experimental)

Filmed in the game with scripted hands on Rock and a Hard Place; not yet tried in the headset.

- **Every gun the mission puts in your hands now has free hands, not only the M416** (`src/gunprofile.h`, `src/hands.cpp`, `src/skin.cpp`): the AN-94, RPK-74M, AEK-971, AK-74M, G3A3, KH2002, M16A4, M4A1, M240, the Stinger and the SMAW. Each can be held by either hand on its pistol grip (the hand that picks it up is the trigger hand), by its handguard or front grip alone, dropped, thrown, left lying on the real ground and picked up again by either hand, with the M416's rules: nothing drops it by accident, and while no hand has it nothing fires. Filmed gun by gun, each through the same round as the M416's (dropped, taken up by the left, its trigger pulled, held by the right on its front alone, the pistol grip taken back, dropped again, taken up by the right, held by the left's front alone), a throw and the free right glove's poses: every moment in order on 11 of the 12, with nothing out of place in the log.
- **Each gun's grips are the game's own:** where the game's right hand holds it and where its left hand holds the front, read from the game's own hold at rest; the other hand's hold is that one mirrored. The left glove on a gun's pistol grip is the game's own right hand there, mirrored, learned the first time the gun is held (within 1 mm of where the game's hand rests on every gun).
- **Each gun falls and lies as itself** (`tools/gunbodyfit.cpp` writes `src/gunbodies.h`; `src/bodyfit.h`): its shape for the ground is fitted from its own first-person mesh, its sight or carry handle in it, with a keel along its bottom so a gun never stays standing on its magazine or bipod but tips onto its side. It lands with the game's own sounds: the AK family's polymer; the M16's, M4's, G3's and KH2002's metal; for the M240, the Stinger and the SMAW the game's own heavy metal knock.
- **A gun held in the left hand stays in the left through a weapon switch** and back (filmed with the AN-94 to the SMAW and back). A gun lying on the ground that the game switches away from (the Stinger after its last missile) goes with the switch instead of being left behind.
- **Whatever the other hand carries when the gun leaves your hands is never lost:** a magazine in the hand stays in it and falls when you let go; one on its way into the well falls where it is; the seated one stays in; a drawn lever springs home; the M240's lid stays as it was, its belt hanging (filmed thrown with its lid up: it flies and lies with it up); a rocket in the hand stays there, and the SMAW lies with its spent case on or bare as it was; the Stinger's aerial stays folded.
- **Guns the mod has never seen** (`src/gunlearn.cpp`, `tools/learntest.cpp`): the learner that will read an unknown gun's mesh, grips and body by itself is written and tested offline, not yet switched on in the game.
- **For testing:** `build\freehands.txt` "guns m416" puts every other gun back in the right hand only, as before; "twosided 1" draws a lying gun's faces both ways. The self-test runs the new offline tests and checks every gun's body against its mesh. Offline, all passing: `profiletest` (21), `bodytest` (118), `learntest` (131), `flaptest` (64), `heldmagtest` (122), `rockettest` (1925), `grasptest` (286), `m320test` (127). Self-test: 0 failed (one run in three trips on the view's camera following the game's eye for a moment when the look jumps 20 degrees at once: an old timing flake in code this upload doesn't touch). The old films (every held gun's reload by the scripted hand, the M320's reload, magazines on the ground, the rocket's reload, weapon switches) are as before.

Not done, or known: none of this has been tried in the headset. **The SMAW stays in the right hand after a switch from the M416** (putting the rifle away left its "seated in the car" sign on, with nothing to take it off): found by the films, fixed for the next upload. The M416 alone has its reload by either hand: on every other gun the magazine, lever and lid are still worked by the left hand only, and only with the gun in the right. Picking up an enemy's gun by hand isn't built yet.

## 2026-11-11: every M416 mechanic by either hand, and the campaign's weapons counted (stage: pre-alpha, experimental)

Filmed in the game with scripted hands on Rock and a Hard Place; not yet tried in the headset.

- **The right hand does everything the left does on the M416** (`src/reload.cpp`, `src/pouch.cpp`, `src/m320.cpp`, `src/hands.cpp`): whichever hand isn't on the pistol grip is the hand at work, so with the rifle in the left hand the right reloads it. The charging handle drawn by the right's trigger or grip, the bolt release slapped on the rifle's left side (where the M416's is: reach over or under the rifle to it), the seated magazine drawn out by hand and let fall, a fresh one taken at the belly, caught under the well and pushed home. The left's own way of doing each is unchanged, checked frame by frame against the old code (`tools/reloadtest.cpp`, `tools/magtest.cpp`).
- **The tactical reload:** hold the rifle by its handguard with the left alone and the right is free: magazine out, a fresh one in, the handle pulled, then the right takes the pistol grip back. A squeeze on the seated magazine takes the magazine, never the pistol grip by mistake.
- **The grenade launcher by either hand:** the right pinches a spent case by its rim, draws it, pushes it back home or pulls it out to fall, takes a 40 mm at the belly, carries it, puts it in along the barrel and slaps the barrel shut, each the left's own move flipped. The launcher's trigger is the trigger of whichever hand is on its grip: with the barrel shut and empty, its pull is the release that opens it. A round turned into line by the wrist while the hand was still drawing back could go in and come straight back out with the hand (one time in three for the right hand): it now goes on only with a push, and once on it comes back out only when the round itself is pulled out (18 of 18 home in a row). With the rifle in the left hand, a flick that shuts the launcher is the right hand's flick mirrored.
- **A magazine in the hand when the rifle drops** falls a frame after the rifle, with that hand's own movement. The hand opens as it goes (its fingers closed through the falling magazine for a frame or two) and stays open until you let go of the grip.
- **The free right glove collides with the rifle** as the left glove does: it rests on the gun's surface instead of going through it, its poses still the left's approved ones mirrored. A hand pushed straight through the receiver stays on the near side until it is 20 cm from it.
- **The campaign's weapons counted** (`docs/research/weapons-catalog.md`): the 12 missions load 58 hand-held weapons, 51 of which can reach your hands: 40 guns, 6 launchers, 2 grenades or mines and 3 melee or scripted items. 11 of the 40 are in the mod's hands today; 29 are new work, most of them combinations of mechanics already built (the AK family's blocks alone cover 11). Mission by mission, every gun's real mechanics and the game's own reload, each one's recoil from its data, and the order to build them in. There is no underbarrel shotgun and no GP-30 anywhere in the campaign; only two soldier types ever drop a gun.
- **For testing:** `build\freehands.txt` "test 4" to "test 9" run each mechanic with a scripted right hand after its left twin; the films show each right-hand step beside its left twin, flipped, with the rifle held up at the eyes for the magazine work so it is seen from below. `weapon.txt` "forceheadless 1" works every frame headless while the stand-in hand is on, so a film's head looks where the test says even with the headset awake on a desk. The films bring the game to the front before each round (the launcher's trigger is read only then). The log says when free hands go to sleep and why. The self-test runs a whole tactical reload by the right hand (13 of 13) and the new offline test, and it, like the films, refuses to run while the headset looks worn: focused since it last woke, or the head moving like a person's. Offline, all passing: `reloadtest` (72), `magtest` (74), `m320test` (127), `grasptest` (286), `twintest` (38), `collidetest`.

Not done, or known: none of this has been tried in the headset. Free hands and the either-hand mechanics are the M416's alone; the other guns and the launchers stay in the right hand. Picking up an enemy's gun by hand isn't built yet. Letting go of the pistol grip with the hand still eases the glove from the game's grip to your hand in 70 ms, with a roll of the wrist and a swing of the elbow (either hand): known, a fix for hand-overs is designed. Open questions: should a rifle magazine stay in the hand when the rifle drops, as a 40 mm does; the right glove at the game's width or the left's; a hold of the launcher's barrel for the right hand; the right's rim pinch on the case's other side if it clips; whether the left trigger hand's index should also slap the bolt release, as real left-handers do.

## 2026-11-06: the rifle in either hand, the grenade launcher loaded by hand, magazines on the real ground, a real picture in the scope (stage: pre-alpha, experimental)

Tried in the headset on Rock and a Hard Place, except where it says otherwise.

- **The rifle in either hand** (`src/grasp.h`, `src/gunbody.h`; the M416 for now): the grip button of the hand on its pistol grip now holds it. Let go, and it stays only in a hand on its handguard, held there alone and still aimed, though nothing fires; otherwise it falls. It lands with the game's own sound of a dropped gun, tips onto its side (it never stands on its magazine) and stays as long as you leave it: guns don't go after 30 s as magazines do. Thrown, it flies with your hand's own speed and spin. A fresh squeeze within 25 cm of it picks it up with either hand: it glides into that hand by its pistol grip in 0.12 s, and that hand's trigger fires it, the left trigger too. The other hand supports it on the handguard, and with the rifle held there alone the free hand can take its pistol grip again. "It works. I can pick it up with both hands. And throw it. It works wonderfully."
- **Nothing drops it by accident:** at a spawn, a checkpoint or a cutscene's end the rifle is in your right hand, and it only drops once that grip has been squeezed and let go; with the headset off or a controller asleep the hold freezes, and nothing changes hands in a scripted moment. While no hand has it, nothing fires and no weapon is switched to, and the aim eases to your head. A tap of X does nothing then, and X held is still the game's own pickup ("HOLD [R]"): a gun lying within 2.5 m comes into your right hand and your rifle goes where it lay (not tried in the headset yet). `build\freehands.txt` "off" puts the old right hand back.
- **The free right hand** has fingers of its own: relaxed, pointing or a fist, from the right controller's own grip, trigger and thumb, never the left's. Every pose is the left glove's approved one mirrored onto the right (`src/handpose.cpp`, checked by `tools/twintest.cpp`), never fitted again, and each was filmed beside its left twin. The right grip and thumb are now bound on the Index, Touch (Quest) and Frame controllers; a runtime that won't take them keeps the old bindings, and the rifle then simply never leaves the right hand.
- **A hold of its own on the grenade launcher under the rifle:** a squeeze near the M320's pistol grip takes it, apart from the handguard's hold, with the game's own left hand on that grip and the index finger fitted onto its trigger (the game's own hand rests 26 mm off it): "in place, and it's well gripped". Going back to the handguard no longer clicks and settles again. With the hand there, the left trigger fires a grenade and the right trigger still fires the rifle: the launcher is switched to for the moment of the shot and the rifle comes straight back, both switches instant (0.5 s each in the game's data). The grenade leaves 0.2 s after the pull, the rifle is back at 0.56 s. "It seems to be working just fine."
- **The M320 reloaded by hand** (`src/m320.cpp`, `tools/m320test.cpp`), as the real one and the game's own reload work: once it's fired, a pull of the left trigger puts the index on the barrel's release, and the barrel swings out to the left, 15 degrees about its front pin. The spent case slides out by itself with the muzzle raised past about 30 degrees, a jab of the gun throws it, or the hand pulls it out by its rim; it falls to the ground and lies there. A new round comes from the belly, held by its base end between the thumb and the index and middle fingertips (the pick of four fitted grips: `src/m320carry.h`, `tools/basegrip.cpp`). Bring its nose to the open end, in line, and push it: past half way it slides home by itself with a click. A slap or a push on the barrel's left side shuts it, and so does a flick of the wrist. Its sounds are cut from the game's own reload of it. "It's good. It's good. It feels really good." (Before that, both arms vanished for moments and a hand twisted away from the body: the reload's finger tables pointed at memory already given back. Fixed, and filmed in the game since.)
- **The flick that opened the M320 is gone** ("I would flick my gun and it would open"): a pull of the trigger on the release opens it, so a loaded launcher only opens by firing it. The flick still shuts it. Not tried in the headset yet.
- **The game's own M320 reload** (X held with the hand on its grip) now lets go of the old case as its hand pulls it out: it falls and lies on the ground, and the new round comes up from the hip. "The 40 mm casing shells do stay on the ground."
- **Magazines and cases fall for real** (`src/loose.cpp`, `tools/loosetest.cpp`), each a rigid body in the world. Out with the button, a magazine slides down its well with the gun (held sideways, it slides out sideways) and tips over the well's lip as it leaves; let go of or thrown, it keeps your hand's speed and spin and tumbles; knocked out by a fresh one, it flies. Each bounces, slides, rolls and settles on a flat side, drawn in the world for each eye with its own depth (the falling magazine had "a weird depth to it", "some other dimension"). It lies 30 s, then goes only while you aren't looking at it, never in view ("I don't want to flood the map full of artifacts"). A falling magazine no longer vanishes on landing. "It works really well ... so far the magazines look good." The SMAW's spent case goes the same way (not tried in the headset).
- **The game's own ground** (`src/ground.cpp`): what falls lands on the game's own collision, the terrain, buildings, crates and cars, found by the game's own ray cast on its own thread, 5 to 16 microseconds a ray. Every piece of the game's code it goes through is checked byte for byte first; anything not as expected leaves it on the soldier's feet instead. Seated or standing, a dropped thing finds the game's floor, not a guess below your eyes.
- **Landings have the game's own sounds:** a magazine's or the rifle's first landing is the game's own dropped gun, the smaller knocks after it rattly metal, or hollow plastic for the AK family's polymer magazines and the SMAW's case; the 40 mm case has its own casing's landing.
- **Throws:** a hard throw from behind the shoulder was taken for a tracking jump and held back, so the magazine "sometimes locks itself behind my shoulder". The controllers' own motion sensors now tell a real fast move from a glitch, and a move taken out that they confirm within 3 frames is put back. The rifle's throws go this way; a magazine's from behind the shoulder hasn't been tried again.
- **All ammo at the belly button** (`src/ammozone.h`), no longer over the left shoulder: magazines for any gun, the SMAW's rockets, the 40 mm rounds and the train pistol's magazines are taken with the left grip at your navel, a generous zone 36 cm across, 32 high and 24 deep that stops short of the gun's handguard and magazine well. The belly gives what's open: the launcher's barrel open, a 40 mm; the rifle's magazine out, a magazine; both, whichever was opened last. The shoulders are kept for holsters later (the primary on the right, the secondary on the left), the chest for grenades (left) and C4 and claymores (right).
- **A real picture in the scope** (`src/scopeview.cpp`): a second copy of the game's own world renderer draws the view along the sight's line, 720 by 720 pixels, only while a magnified sight is looked through, and the glass shows that instead of the eye's picture stretched. Every magnified sight the mod knows gets it, the ACOG, PSO-1, PKS-07, PK-A and PK-AS; the Kobra, unmagnified, keeps the plain lens. "That looks amazing." Its own smoke, sparks and fire came out as "paper squares", so it borrows the eye's particles: "That's way better now." It borrows the eye's terrain too: asked for a third view, the game's one terrain kept working 4.5 ms a frame harder for good. With its own sun shadows it costs about 1.8 ms of the graphics card's frame: 69 to 72 fps through the scope, 61 at the worst through explosions on film. `weapon.txt` "scopeview 0" brings back the old lens.
- **No clipping with the eye pressed to the sight:** the near plane is 1 cm instead of the game's 6 cm, for the world and the gun alike, so the rim, the glass and the receiver no longer cut through each other. "When I press my eye right up against the scope, I do not see any sort of clipping." `weapon.txt` "nearplane 0" puts the game's back.
- **Every reload sound from the game's own files** (`tools/gamesounds.cpp`): it reads the game's sound data and decodes each gun's own reload wave, cut into its parts: magazine out and in, lever, bolt, the M240's lid, box and belt, the Stinger's aerial, the M320's breech and rounds. The lever pulls recorded from the game's sound output last time sounded "really weird" (they were mostly the battle around) and are gone. The M16, M4 and M416 never pull their handle in the game, so they borrow the G36C's: "I did hear the lever pull sound ... That's a good touch." The AN-94's: "they sound great". The clips aren't in the repository: the tool makes them from the game's own files.
- **Crouch and prone on the right stick**, which every controller has, Quest's too: flick it down to crouch, flick it down and hold to go prone, flick it up to stand from either; each stays until you flick up. B no longer crouches; a sprint or a jump stands you up, as in the game; crouching for real still crouches the soldier from standing. **A short click of the right stick jumps** (held, it still recenters, with no jump; a click with the stick pushed over doesn't count), and A still jumps too. Neither judged in the headset yet.
- **Footsteps:** the game's own still play in VR, 2 to 3 a second at a full push of the stick, only quietly, as the game mixes them. Nothing changed; `weapon.txt` "steplog 1" counts them.
- **The AN-94 as uploaded last time**, levers over and under, its magazine by hand, the magnet and the knock, tried again: "this is perfect ... Keep this as a baseline for like most of the guns." New guns start from it; the M4 family keeps its own two-finger pull and bolt slap.
- **Tried and set aside:** the left eye at half rate to free frames for the scope (`weapon.txt` "halfeye 1", off): turning the head, "it was almost doubling". The mod's own casings that land and lie (`src/casings.*`, `tools/casingtest.cpp`): written, not built in, and put off ("We might not implement that at all").
- **For testing:** `build\freehands.txt` "test 1" to "test 3" run the hand-overs, the throw and the right glove's poses with scripted hands; `weapon.txt` "m320test 7" works the M320's reload by a scripted hand; "loose 0" and "m320byhand 0" bring back the old fall and the game's own M320 reload. Offline, all passing: `grasptest` (208 checks, every row also run with the hands swapped), `loosetest` (148), `twintest` (34), `m320test` (113). The self-test checks the right grip's bindings, runs the free hands round and the new offline tests, and now ends with the stand-in off (left on, it throws the gun at any tracking gap). Self-test: 0 failed, the free hands round 15 of 15, 71.9 fps.

Not done, or known: free hands are the M416's alone for now. Picking up an enemy's gun by hand and pulling the lever with the right hand were both tried, and neither is built yet, nor any reload by the right hand, nor the other guns or the launchers in either hand. With the rifle on the ground and no gun near, X held makes the game reload the rifle where it lies, and weapons can't be switched until a hand has it. The right glove doesn't collide with the rifle in the left hand. The M320's grip is the left hand's only. A loose thing is drawn only with its own gun: magazines vanish while another gun is out, and the SMAW's cases while the rifle is, back when it is. Every ground sounds like dirt for now (the ground's material isn't read yet). The train pistol's magazine still falls the old way.

## 2026-11-05: the first whole mission, every gun in it in the hands, scopes that magnify (stage: pre-alpha, experimental)

Tried by the user in the headset, except where it says otherwise.

- **Rock and a Hard Place plays from start to finish in VR:** the jet came down to the Stinger in the hands. "A complete playable mission."
- **Every gun of that mission in the hands** (`src/skin.cpp`, table in `docs/research/other-guns.md`): AN-94, RPK-74M, AEK-971, AK-74M, G3A3, KH2002, M16A4, M4A1, M240 and the FIM-92 Stinger, each on its own frame with its flash at the muzzle, casings at the port, its own collision shape and a hand fitted to its own magazine (`tools/magfit.cpp`, `src/magposes/`).
- **Magazines by hand on all of them** (`src/handmag.cpp`): taken over the left shoulder, a magnet at 10 cm and 0.15 s ("Yeah, that works"); guns with no button have the old one out by hand or knocked out with the new one, which takes real speed and pushes back when it doesn't ("I like the resistance"); guns with a button can also have it pulled by hand. Reloading works with the gun at the eye.
- **Grip or trigger, either one,** takes a magazine, a lever or a lid; the front grip stays the grip's.
- **Levers:** over and under the top on the side levers, a fist closed on the knob; the M4s' and M16's two-finger pull; the KH2002's index hooked round its fin, through the handle ("that's wonderful"); the G3's handle swings out as it's drawn. A pull now has a sound: the M240's and KH2002's own from their reloads, the KH2002's on the others for now (the clips aren't in the repo; cut by the lever's timing and not yet checked by ear).
- **The M16 and M4:** the port's flap opens and shuts with the lever and flicks open on each shot. The game only ever shows it open, so its mesh has only the one side: shut it was gone but for its edges, now it's drawn from both. Their bolt locks back, and a slap on the release sends it home, with the M416 too, only with the hand really on it. Their hand on the magazine is the M416's, thumb round it, a little higher up ("Yep, looking good").
- **The M240, all of it by hand** (`src/handmag.cpp`): the lid snaps up, the box comes off, a new box snaps in with its belt hanging, the belt's pinched with thumb and pointer and snaps into the feed, the lid shut, the lever. The belt stays there after a reload and gets shorter as it fires. "This is fantastic." The tape on the box stays on (it went with the rounds for a moment), links now go with the gun: neither filmed nor tried yet.
- **The Stinger's reload by hand** (`src/flap.cpp`): as in its own animation, the aerial folds after a shot, the left hand lifts it, it springs open and the next missile is ready. "I like this one a lot." It locks without the zoom and its missile leaves from the tube's end.
- **Scopes magnify inside the glass only** (`src/scopezoom.cpp`, `src/scopemarks.h`): on the game's own glass disc, inside its rim, with every line, number and range mark of each sight, for the ACOG, PSO-1, PKS-07, PK-A, PK-AS and Kobra. Everything in the hands is left out of the bit of picture the lens reads, so no part of the gun shows magnified in the glass. An edge-following filter (AMD's FSR 1 EASU, its MIT licence noted in the source) gives clean edges, but the picture's still soft: the lens stretches about 100 of the eye's pixels over about 340. Making it truly crisp is still open: the game can't draw a third view.
- **The gun isn't cut off near the eye any more:** what's in the hands is drawn nearer than the game's 6 cm near plane. It "sort of" still clips, much less than it did: nearer than that its parts lose their depth order.
- **Tracking gaps:** a jump of a controller in a single frame is taken out and eased back, and a gap of up to half a second holds the hand where it was (`src/input.cpp`). Not yet tried in the headset. What the user felt as the gun kicking away near the scope was the mod's own test hand left on: fixed.
- **A crash guard** for the game's particle job, which died once reading an emitter's transform from a page of its table that wasn't there. Not reproduced.
- **For testing:** `tools/heldmagtest.cpp` (offline, in the self-test), the gun's reload scripted with `weapon.txt heldtest`, and `forcehand` to film with the controllers awake.

Not done, or known: a falling magazine still vanishes on landing; most reload sounds are the M416's; the M9 has no collision; there's no real pick-up from a dead enemy by hand; the AN-94's overhand lever is the AEK's hand; the RPK's lever kick and the G3's handle haven't been filmed firing.

## 2026-11-04: the launcher loaded by hand, a rifle that goes where it points, switches that are instant (stage: pre-alpha, experimental)

All of it tried by the user in the headset on Rock and a Hard Place, except where it says otherwise. Only the Stinger against the jet is left in that mission.

- **Arms that never stretch:** held out of reach, the sleeve used to stretch across the view (1.08 m from shoulder to hand on a 0.56 m arm, "I look like Mr. Elastic"). Each bone now keeps its length, in the arm solver itself, for every weapon; its far end and the collar go where that is. The chest turns as a body holding a rifle does, the hand further out bringing its shoulder forward. A hold the arm truly can't reach, with the real hand far from it, lets the glove go back to the real hand and take the gun again by itself, the grip held all along. "Looks right." `tools/armtest.cpp` row 9 fails if a bone ever stretches.
- **Recentering is quicker:** the right stick's click held 1.5 s instead of 2.5, and a moment's slip of the thumb doesn't start the wait again. Tried in the headset.
- **A lean no longer creeps:** at some headings the head's offset crept a few millimetres a second sideways as the view's rows, square only to 0.015 degrees, were applied with no turn at all.
- **The graphics card's time in the log,** by part of the frame, once a second (`src/gputime.cpp`).
- **A steady frame rate:** 99% of seconds at 72 in the headset, nothing under it in a real fight but the odd frame; the earlier dips came from outside the mod.
- **Reloads by hand no longer die after the mission moves on:** the mod read the rifle's and the launcher's rounds at two fixed places (+0x248, +0x258 from the weapons component) that turned out not to belong to it: past its end, in whatever the game's memory pool had put next to it. Right at a mission's start and after most deaths, wrong after others, and then every reload was "only the picture": the rifle's magazine and the launcher's rocket both dead for the rest of the run. Now they're read from the soldier's own list of weapons by slot (`src/ammo.cpp`), the way the train's pistol was found. Both reloads by a scripted hand, before and after a death, on film: fine. In the headset: the user's report; to try.
- **All ammo from over the left shoulder:** magazines for any gun, on every level, are now taken where the rockets are (`src/ammozone.h`); the pelvis was awkward and is gone. Reach over the left shoulder and squeeze the left grip. X no longer puts a free magazine in with one out (`build\pouch.txt` "xinsert 1" brings that back); holding X is still the game's own reload. Tried by a scripted hand; the zone itself by the offline test.
- **The left View button does nothing now:** it lined the left glove up with a beep, and was pressed by accident some ten times, the hand lined up all over again each time. `build\pouch.txt` "lineup 1" brings it back.
- **A steadier head:** moving the head jittered, and the aim was off on and off. Measured frame by frame, three things moved the picture. A follower meant for vehicles stayed on on foot ("0 degrees off" is not 0), so the whole world stepped up to half a degree a frame as the gun moved: it went into the picture but not the shots. The game's camera wobble is now left out of the view's turn (`viewturn::PureRows`). The game's eye swings on a 7.73 cm arm, 1.3 mm a degree in steps: that's taken off the cameras (`headmove`). The head's turn and place are also smoothed a little, 6 and 12 Hz, with a small lead (`posefilter`, `placefilter`, `poselead` in `build\weapon.txt`), and the gun follows the picture's own head pose when the picture comes a frame older. On film, with the gun turning, the world moves 0.000 degrees and the cameras under a millimetre. In the headset: "pretty good", not gone. What's left waits for the end, with performance.
- **Pulling the left hand back no longer pushes the gun forward:** when the two hands' line came round past the gun, the steering faded and reversed. It now stays ahead of the gun and bends with the body (`steer::KeepAhead`, `steer::LimitLine`). On film only, by a stand-in hand.
- **Tried and set aside, the picture's black edges:** the picture's edge carried outwards (`edgefill`, off by default) was "worse than the black". A real 120-degree picture at the same sharpness holds 72 only with the game's 4x MSAA swapped for FXAA, and the leaves were too rough without it. Back at 100 degrees and 4x MSAA until the end. Past 2160 pixels high the game's HUD also goes off the picture, for whoever tries it next.

- **The rifle goes where it points:** without the zoom the M416's scatter is taken down to what it is zoomed, for free hands that never zoom. "riflespread 0" in `build\weapon.txt` puts the game's back.
- **The aim comes back after a switch:** the game keeps its look angles in five copies, and a weapon switch changes which it reads. All of them are written now, each checked before every write.
- **The launcher is reloaded by your hands** (`src/rocket.cpp`): the spent case stays on after the shot. Take it with the left grip, twist or pull it off and let it fall; reach over your shoulder for a new one; bring it to the launcher's back, push it on and twist it locked. Holding X with it empty is the game's own reload. Recorded sounds aren't part of this repository. "Feels right."
- **A real grip on the rocket:** the hand starts from where the game's own reload puts it, then the fingers and thumb are closed onto the rocket's real shape (8.9 cm across) by `tools/rocketfit.cpp`, the thumb the other way round. "Okay, it's perfect. You polished it."
- **The left hand on the launcher:** a squeeze near the front grip snaps the glove onto it, and it steers the launcher as it does the rifle.
- **Weapon switches are instant:** the arms are on the rifle from its first frame back from the launcher, instead of floating ahead of it for 5 s. On film: 3 frames (under 50 ms) with no gun while the game swaps them. "It is instant."
- **The rifle's magazine goes in more easily.**
- **The car ride at the mission's start:** seated, the game poses the M416's parts differently: the grenade launcher's tube hung out under the gun, turned, and slid back as you got out. They now stay where they are on foot (`docs/research/smaw.md`, "The rifle in the car"). "Looks right."
- **Bendable wrists on the gloved arms, both of them:** each forearm stays on the line from the elbow and only the hand bends at the wrist, as on The Great Destroyer. It's on by default, with the rifle and the launcher in hand. Each arm is now whole up to the shoulder, sleeve included (a forearm alone was an open tube you could look into). Every depth bone's twin in the colour draws was checked vertex by vertex: the passes agree within 0.1 mm. "Looks right."
- **The rifle's aim:** measured, three things were off. Its sights look 0.29 degrees below the barrel's line, so the rifle is raised that much. Its bullets started from the soldier's head, not the gun, so they now start on the sights' line at the muzzle. And the ACOG's chevron was a little shape 29 cm from the eye, 2 degrees off for each centimetre the eye is off the tube's axis: each eye now sees it on its own line along the sights, as with a real sight. On film, shots at a car 12 m off land on the chevron, within about 0.3 degrees before the recoil. In the headset: on the tip.
- **A code review of all this**, with the real findings fixed: the left hand's steer on the launcher was switched off by a setting's name being found inside another's; the arms' depth copies weren't placed from the launcher's data; a game paused at the wrong moment could set the arms' rest; memory the mod writes to is looked at before each write.
- **Not tried in the headset:** the left hand's steer since that fix; the game's own reload (on film only).
- **Still to do on this mission:** recoil for every gun, the Stinger, then picking up enemy guns from the floor. At the very end of the list: the 6DoF head when seated, the HUD's place and the launcher's count reading one high, this level's low-resolution look.

## 2026-11-02, evening: the SMAW in the hands, and the two BMPs beaten in VR (stage: pre-alpha, experimental)

On Rock and a Hard Place the rocket launcher now works in VR, and the user beat the mission's two BMPs with it in the headset. Only the Stinger against the jet is left in that mission.

- **The launcher in the hands:** the tube follows the right controller, with the game's own grip and reload kept. Tried in the headset and liked.
- **Picking a weapon:** left D-pad up for the launcher, down for the rifle, or Y held 0.7 s to swap. Only D-pad up has been seen working in the headset.
- **Free aim:** the launcher's spread is taken out of the game's data, the rocket starts at the tube's muzzle, and the tube is lined up with its iron sights. No zoom needed.
- **Not tried in the headset yet:** at the BMPs rockets fell short, so they keep the game's small upward tilt again. With a gun in hand, a lone jump of the game's aim no longer takes the aim from the hand for 1.5 s; this is meant to help the rifle "losing its aim", but that link is read from the log, not proven.
- **Still to do:** the rifle's own spread, reloading the launcher with real hands, a snap grip, and the Stinger. See `docs/research/smaw.md`.

## 2026-11-02: a new name on GitHub (stage: pre-alpha, experimental)

The project is now **Battlefield 3 VR: The Great Destroyer**, at
`github.com/criminalvictory/Battlefield-3-VR-The-Great-Destroyer` (GitHub's addresses can't have spaces). The old address still
works, it leads here. Nothing in the mod changed.

## 2026-11-02, overnight: 6DoF head, the muzzle flash and smoke at the barrel, casings out of the port (stage: pre-alpha, experimental)

Tried by the user in the headset the same morning, seated, on a Steam Frame. The flash, smoke and casings: "It looks really great."
Firing while leaning and looking away, the headset off and on, and the train's pistol all came out right. The head's movement:
"it's okay", to be polished at the very end. The real head's walking and standing crouch haven't been judged yet, from a chair.

### New
- **Your head moves the view**: lean, duck, peek. Up to 25 cm freely, then more and more gently to 32 cm, and past that the
  view stops going further so you can't put your head through a wall. Walk in your room and the soldier walks under
  your head. Crouch for real (30 cm down) and the game's soldier crouches, with your eyes staying where they are. Hold the
  right stick's click to set where "standing here" is. In a cutscene or a ride it moves a little (10 cm, 15 at most) and
  your body stays on the scene. `[head]` in `build\bf3vr.ini` turns each part off (`position`, `walk`, `crouch`).
- **The muzzle flash comes out of the barrel**, not the chest: the game's own fire and smoke, moved with the gun, on the
  M416 and the train's pistol. The flash comes only some of the time, as in the game itself.
- **Smoke and sparks** that go with a shot come from the drawn gun too, rifle and pistol: the plume along the barrel and the
  small puff at the ejection port. What's moved is a long shape round the barrel's line: all of it within 0.6 m of that
  line up to 2.2 m past the muzzle, then less and less. The dirt a bullet throws up where it lands stays where it lands,
  unless you shoot something within about 3 m: nothing in the game's data tells that dirt from the smoke.
- All of it follows your head: lean, crouch or look away from your aim, and it stays on the barrel, at the right depth in each
  eye, and when you run. When the shooting stops the smoke stays in the air as the gun moves on.

### Fixed
- **Casings fly out of the ejection port** in ordinary play. What was wrong: the mod often didn't know where the gun's own
  coordinates sit in the world, and then what it moved with the gun swung kilometres away or not at all (the flash too).
  It's now always worked out from the camera. They also stay with the gun further (0.45 m, gone by 1.3 m); at the old 0.2
  m and 0.8 m a casing, thrown at 4 m/s, was soon on its way back to the chest.
- **The train's pistol** has its casings moved too (a different mesh), and shots are told on it (its rounds weren't being read).
- **"I can't move"**: the game looks for controllers only once, 5.6 s after it starts, and never listens to one it didn't
  find then. The mod now hands it one at that moment, so the walking stick is read from the start. A Valve Index plugged in
  needs no dongle wait in `tools\launch_vr.ps1`.

### Careful with the headset coming off
- **Off and on again**: the view starts afresh where you are when the headset's back on, standing. Set down on a desk, nothing is
  walked and the game's crouch is let go. Too low a start (bending for a controller as the game first saw the headset) finds
  itself in 2 s.
- **Left Ctrl is never left held**: a real crouch holds it, as the B button does. The injector now stays until the game has gone
  and lets go of everything the mod could have held down, so a crash can't leave the desktop with Ctrl stuck.

### Not yet
- **Bullets still start at the soldier**, not your leaned head. Lean out past a door frame and shots can hit it; step further than
  25 cm and the soldier walks out with you. Starting them at the drawn muzzle is its own piece of research.
- **Guns the mod doesn't hold in your hand yet**: their flash stays where the game has it, off the barrel by as far as you lean.
- **While you shoot**, the smoke within reach of the barrel moves with the gun, old and new alike, rather than trailing behind it.
- **Playing seated**: leaning far walks the soldier, and rising and sitting down again can leave it crouched with the view low until
  you hold the right stick's click. A seated mode is on the list for the very end, with HUD placement.
- **Rock and a Hard Place looks low resolution** in its forest, though the other levels look great. The resolution hasn't changed
  (2160 per eye); the foliage and its shading are the suspects. Also for the very end.

### Looked into
- **Tracers and bullets**: only other soldiers' guns draw tracers. The M416 and the train's pistol draw none of their own
  and bullets aren't drawn otherwise, so there's nothing there to move.
- **Lens ghosts**: the flash's lens flare (the blobs across the view on some shots) is left as the game draws it: it belongs
  to the picture, not the gun. So is the light the flash throws on nearby things, at the game's own muzzle.

### For finding things
- `build\weapon.txt`: `flashquads`, `flashburst N` (N frames in a row from the next trigger pull, as pictures), `flashscan`,
  `flashsurvey ms` (everything near the gun that comes with firing), `sixlog`, `sixlean`, `headtest`, `movetest`; the census
  now names shaders by checksums that hold between runs.
- `standin 1 0 25` aims the stand-in hand 25 degrees right of the head, with nobody in the headset.

## 2026-11-01, evening: the desktop and Discord show the stream view (stage: pre-alpha, experimental)

Tried by the user, streaming the game on Discord: "it works".

### New
- **The game's window fits your monitor and shows the stream view**: borderless, 16:9 (2:1 when Home shows both
  eyes), the whole view. It was 4320x2160, more than twice the size of a 1920x1080 screen, so the desktop showed
  only its top left ("way too zoomed in ... way too cropped"). The headset's picture is untouched: the game is told
  its window is still the headset's size and draws at it, and Windows scales the picture into the window.
  `[window] fit=0` (or weapon.txt "windowfit 0") puts the big window back.
- **Discord streams the whole view.** Discord's Go Live copies the window as Windows shows it, and of a window off
  the screen's edges Windows kept only the part on the monitor up to date: the stream showed that part moving in a
  corner of an old, frozen picture. Now Discord gets the whole picture, current, at 16:9. OBS is unchanged.

### Fixed
- **Walking never gets stuck on the stick again.** In some runs the game stops reading the mod's controller stick
  (the user, The Great Destroyer: "i cannot move ... after fighting in the interrogation room"; twice before on
  2026-10-31). The mod notices within half a second and walks you on W/A/S/D instead, and goes back to the stick the
  moment the game reads it again (it's also told the devices changed, to look for its controller).
- With another window in front, the left stick no longer types W/A/S/D into it.
- In cutscenes, your body is moved to each eye's place only when the frame's eye pictures were counted; otherwise it
  stays as the game draws it, instead of possibly going to the wrong eye in a mission whose pictures are drawn
  another way (from the code review).

### Tools
- tools\resume.ps1 (and the desktop launcher with it) clicks RESUME CAMPAIGN in the fitted window; tools\selftest.ps1
  checks that the game still draws two `[window] size` squares for the headset.

## 2026-11-01, evening: your own body in cutscenes has depth (stage: pre-alpha, experimental)

Tried by the user in the headset, the Comrades drive: "it worked really well ... looked really good".

### New
- **In cutscenes and rides, your body is drawn for each eye**, so the hands, gun, knees and the jammer have their
  own depth. The game draws them from one camera for both eyes, flat against the scene around them, which felt
  "like looking cross-eyed", a split, when the jammer was raised to the face or the gun inspected up close. Each eye
  now gets its own view of the body, half the eye distance to its side, as the scene already had. On for every
  cutscene; weapon.txt "bodystereo 0" leaves the body as the game draws it.

### Fixed
- A body held by the cutscene's own camera no longer drifts by a frame's head turn while that camera holds still
  (from the code review).

### Known
- A dark layer around the arms, glove and gun in ordinary play is still unexplained (it isn't what caused this
  fix's first-try black holes, which were frames given the wrong eye).

## 2026-11-01, later in the afternoon: the body held through the whole Comrades drive (stage: pre-alpha, experimental)

Tried by the user in the headset: "stays put now".

### Fixed
- **The body in cutscenes and rides stays put through the whole drive of Comrades.** It had vanished for about ten
  seconds in the tunnel's dark stretch (looking down, no knees, no gun), and at the crash through the gate it let go of
  the head's turn and shook with the camera. In those shots the game draws the body from a place more than 5 m off
  the point the mod measured from, and the mod took it for something else and left it, while the rest of the body's
  drawing was turned, so the two no longer matched. The body is now known by the camera it's drawn from (the cutscene
  camera's exact angle, as the game made it), wherever it's placed, and each piece is turned by that camera's own turn.
  Replayed with the headset off, the body was held every second of the drive; offline tests: 33 of 33.

## 2026-11-01, afternoon: a stream view for OBS, a one-click launcher, cutscenes that follow the head, the pistol's magnetic reload (stage: pre-alpha, experimental)

Tried by the user: the stream view in OBS ("great work"), the Comrades drive and the interrogation with the head
turning the cutscene's camera ("super great"), the magnetic magazine ("goes in very smoothly") and slide, the
launcher. Not yet tried in the headset: the body held still in cutscenes (take 2), the stream view's steadying,
recentering by the stick.

### New
- **A stream view: OBS and the desktop window show one eye, in proportion.** In 3D the game's window holds both
  eyes side by side; once the headset has its frame, the mod draws over the window with the left eye's picture, 85
  degrees across, drawn 16:9 so OBS's "stretch to screen" shows it in proportion, and steadied while the headset
  is worn (small head shakes taken out, the horizon level). The headset's picture is untouched. The mod now runs
  before capture programs' hooks (OBS, Streamlabs, Discord), so they record the finished frame. Home switches it
  (saved); settings in [stereo] of bf3vr.ini (stream_view, stream_fov, stream_aspect, stream_fit, stream_steady,
  stream_tilt). README: "Streaming and recording".
- **A one-click launcher**: tools\launch_vr.ps1, with desktop shortcuts "BF3 VR" and "BF3 VR - Mission select".
  It starts SteamVR first (over the Steam Frame's dongle, not the home Wi-Fi: it waits for the dongle's link and
  restarts SteamVR if it took the Wi-Fi), the mod's injector, and BF3 through Steam; you click Battlelog's PLAY
  CAMPAIGN (the EA sign-in stays Battlelog's), and it resumes the campaign or stops at the menu.
- **Cutscenes and rides follow the head**: a scripted camera (the Comrades drive, the interrogation) is turned to
  where the head looks with the horizon level, so the picture fills the view with no black edges; the player's
  own body (arms, gun, legs) is turned with it, so it stays on the scene and only the head moves.
  weapon.txt "cutsceneturn 0" / "bodylock 0" put them back as before.
- **The pistol's reload, magnetic**: the magazine is caught from any direction within 7.5 cm and glides in; the
  slide is taken in three grips, picked by the way the hand comes, with the hand gliding on; the
  G2 carry (the index finger along the magazine's front). The reload sounds are the game's own (the M9's, and the
  M416's for the rifle), cut from its files and played centred; they aren't in this repository.
- **Recenter by holding the right stick's click** for 2.5 s (as F9).

### Fixed
- The stream view came out twice as wide with the headset idle (the user: "it looks stretched").
- tools\resume.ps1 kept clicking into a level that opens on a scripted ride (the Comrades drive) and reported a
  failed click; it now knows the level has started from the mod's switch to the headset's view.
- A steep scripted camera's level direction no longer swings with the least turn.

### Known
- The stream view crops out the subtitles and the HUD's corners (they sit at the eye's outer edge), and the pause
  menu; Home shows both eyes. Recorders other than OBS aren't tested; screen capture sees only part of the
  window, which is bigger than the monitor.
- The aim still takes a while to lock on after loads and cutscenes.

## 2026-11-01, night: The Great Destroyer: the pistol reloaded by hand, first try (stage: pre-alpha, experimental)

Tried in the headset by the user at the train: walking, racking the slide, X dropping the magazine, taking a
magazine at the belt and pushing it home, firing and the two-handed hold as before, and X held for the game's
reload all work. Grabbing the slide and getting the magazine in still take too precise a hand; they become
magnetic in the next round.

### New
- **The pistol's manual reload by hand, a rehearsal: the rounds don't change yet.** All of it is behind
  build\pistol.txt ("reload on", "slide 1", "mag 1"), off by default, so nothing changes without it.
  - The left trigger on the back of the slide grabs it. Pulled back its full 4.57 cm and let go, it springs
    home. The right trigger doesn't fire while the slide is held.
  - X drops the magazine out of the grip and it falls to the floor. The right trigger stays dead while the grip
    is empty.
  - The left grip at the front of the belt takes a magazine. The fingers open flat under the grip, the palm
    pushes it up, and it snaps home near the end ("seat N" cm; each try logs how close the palm came).
  - A magazine left part way in stays there. Let go anywhere else, it falls. X held is still the game's own
    reload.
  - Offline tests: 83 checks for the magazine, 60 for the slide.
- **The study behind it** (docs/research/pistol-manual-reload.md): the game's own reload measured from a
  recording, the pistol's ammo found in memory (read only), the hand poses, and the user's choices, including
  the G2 carry (the index finger along the magazine's front) for the next round.

### Fixed
- **The view keeps one body yaw** for walking and looking, so it no longer jumps when the look starts or stops.
- With the headset asleep the cutscene flag is still counted (the self-test's headless run).
- Cutscenes are kept where the hand faced again (the upside-down QTE report), and cameras the mod refuses log
  their up and forward.
- A long comment line in pistol.txt could have switched on the scripted test hand by itself.

### Known
- The slide's grab and the magazine's catch need a precise hand. Next: both magnetic, with animations, two more
  slide grips (fingers over the left side, and a pinch from behind), and the G2 carry.
- The pistol's magazine makes no sound yet.
- The upside-down QTE fix hasn't been checked in the headset yet.

## 2026-10-31, night: The Great Destroyer: no more teleporting view, a safe aim search, crash guards (stage: pre-alpha, experimental)

Built and pushed on request; not yet tried in the headset.

### Fixed
- **The view jumping ("teleporting"), walking or seated.** While the mod looked for the look angles,
  mouse aim paused for each candidate the search nudged. The picture then switched to the game camera's
  own direction and snapped back afterwards, undoing the stick's turn made in between: up to 112-180
  degrees in one frame. Mouse aim now keeps steering through a nudge. Only its corrections wait, and the
  stick's own turn is sent as it is. Checked offline against the recorded runs: the largest picture step
  away from the stick's turn went from 135-178 degrees to 0.
- **The crashes just after the interrogation (most likely).** To find the look angles, the aim search
  nudges values near the soldier by 0.03 and watches the camera. It nudged zeros too, which are often
  null pointers, and the game then read 0.03 as a pointer (bf3.exe+179AE2 reading 3CF5C28F: 0.03 exactly).
  It also put old values back after a nudge, even where the game had changed them meanwhile. Now only
  real, non-zero angles are nudged. Each nudge is written only over the value it read, and undone only
  where it is still there.

### New
- **Crash guards** for two crashes in the game's own code at that moment: a render step walking a freed
  list (bf3.exe+134E5F0, four times today) and a call through a freed node (bf3.exe+ADF021). Each is sent
  down its function's own way out, with the code checked first, and logged as "Crash guard".
- The crash log says whether a crash was on the render thread. The aim search logs how long it held
  mouse aim, and the view log marks the pictures turned from the game camera.

### Known
- The forearm still moves when only the wrist bends. A lock is designed but not built yet.
- The manual pistol reload is being studied.

## 2026-10-31, evening: The Great Destroyer: real wrists, the pistol in both hands, a smooth suppression blur (stage: pre-alpha, experimental)

Tried in the headset by the user: the wrists and forearms, the two-handed pistol, the fingers on
the grip and the suppression blur all passed. The in-game self-test passes, except the look-angle
check, which can't pass on the moving train.

### Fixed
- **The "candy wrapper" wrists.** The bare hands were placed by the thumb's last bone (45 and 19),
  not the palm (41 and 15). The whole hand followed the game's thumb: the wrist twisted thin and
  the left one sat crooked. They're placed by the palm now (the user: "a lot better, especially the
  wrists"). The twist offset that had hidden it is gone.
- **The wrung forearm with the palm up.** Each forearm now takes half the hand's roll at the elbow
  end, 80% in the middle and all of it at the wrist; before, it was nothing, half and 95%. The user:
  "a lot better ... this fixes everything". weapon.txt `bareshare E A B` changes the split.
- **The pistol was off the hand.** Its grip was taken at the magazine's place in the unassembled
  mesh, 13.6 cm off. The pistol and the hand holding it were drawn about 12 cm above and 7 cm behind
  the real hand. The grip is the seated magazine now, and the held hand is drawn where the real hand
  is.
- **Learning the game's hold on the pistol.** Draws are paired by frame, since the game records them
  on several threads. The hand taken is the one whose fist is on the grip, after the pistol's first
  second in hand, and both hands come from one settled moment. A nearer pair replaces a wrong first
  pick (not yet tried in the headset). If nothing is learned in 5 s, a hold fitted offline is used.
  `barehands pistol` learns it again; that switch did nothing before.
- **The square patches in the suppression blur.** They came from the mod's orange-marker remover, not
  the game. It took its "before" picture before the blur, then pasted the unblurred picture back on a
  32-pixel grid wherever the blur spread orange sparks or warm light. It now takes it after the blur
  and the damage pass, and only cuts where the HUD's own pixel is orange. Each eye also keeps its own
  blur: the right eye's blurred picture was drawn over the left eye too, which took away the depth
  under fire. The user: "it's smooth".
- **Free hands stuck off after a scripted moment.** The flag only cleared once the look angles were
  found, which took a minute and a half after the dumpster. It now clears at every cutscene, and
  after 1.5 s of quiet without the angles.
- The rifle-only systems no longer act on the pistol. The M416's handguard hold and steer, its
  collision and its charging handle had been running against an invisible M416 at the right hand.

### New
- **Two-handed pistol.** Bring the left hand to the pistol and squeeze the left grip: it goes into the
  game's own two-handed M9 hold. Let go of the grip to free it (the user: "fabulous"). While the right
  hand holds the pistol, the free left hand's fingers follow its controller.
- **Fingers on the grip.** In one hand, the right middle, ring and little fingers open 25 degrees. With
  both hands the middle eases to 8, because past about 12 degrees its tip comes out through the back of
  the game's left hand (weapon.txt `pistolfingers`, `pistolfingers2`). The game's own pose has them 13-17
  mm inside the grip; this halves the ring's and the little's.
- **Mouse aim.** While the look angles aren't found, the view is steered with the mouse, so aiming works
  at once after a load. After a cutscene it starts again from where the game left the view.
- **Stereo on at once** when the player's soldier is in; the resume script no longer presses F8.
- **Research tools.**
  - `reloadrec N` records the game's own pistol and arm bones; it caught the magazine's path and the
    slide's 4.5 cm travel.
  - `build\psdump.txt` saves every pixel shader's bytecode.
  - `pistoltest 1` puts the stand-in left hand on the pistol.
  - The crash log keeps the code round the crash and the stack's first words.
  - The camera watch says why it drops a camera.
  - The self-test reads the forearm lines case-sensitively, so the bare arms' own lines no longer fail
    it.

### Known
- **The view can jump ("teleporting").** It happens while the mod hunts for the look angles, after a
  cutscene and in the interrogation room: two ways of turning the picture disagree. The fix is being
  worked out.
- **The forearm moves when only the wrist bends.** A lock is being worked out.
- **An occasional crash** at bf3.exe+134E5F0, inside the game's own code (three times, all at scripted
  moments). The crash log now saves more to find it.
- **Only the M416 and the train pistol have hand support.** The shotgun picked up at the train's door
  falls back to the game's own hands.

Tried in the headset by the user, round by round; the offline tests pass. The in-game self-test
wasn't run (the user was in the headset).

### New
- **Free bare hands** after the interrogation (the user: free hands, but not in the opening and its
  QTEs): Blackburn's bare arms follow the controllers, lined up as the left glove of Rock and a Hard
  Place is (the bare arms are the gloves' rig: the right hand mirrored). On only once the game has left
  the view to the player for 3 s: no cutscene, no scripted turn, the aim found. The cuff round the right
  wrist goes with the forearm. The body, legs and sleeves and the game's depth-only copies (black arms
  and legs by the waist) are left out meanwhile. weapon.txt `barehands 0` leaves the game's own arms;
  `barehands learn 5` lines them up on the game's arms instead.
- **Wrists**: each forearm solved from its hand, a shoulder from the headset and an elbow (the same
  solver as the gloved left arm's): the hand bends at the wrist and the forearm stays on the arm's line,
  over a wider range than the gloves' (the user: "proper arm tracking behavior"). weapon.txt
  `barewrist 0` moves the forearm whole with the hand.
- Research switches: `pslog <pixel shader>` logs every draw using a shader (its code or this launch's
  pointer), with where its bones sit; `dump <count> <start>` now takes any draw by name.
  `tools\transcribe.ps1` roughly transcribes what's said in a recording.

### Known
- The chain and the open cuff hanging from the wrist are hidden for now: moved with the hand they
  jittered (set aside).
- Rock and a Hard Place's gloved arms don't have the wrist solver on both sides yet (to do there).

## 2026-10-31, afternoon: The Great Destroyer: no crash at the train, cutscenes you can look around in (stage: pre-alpha, experimental)

Tried by the user in The Great Destroyer. The in-game self-test wasn't run: on the moving train the
aim can't be found again (below); the offline tests pass.

### Fixed
- **The crash jumping onto the train** (four crashes, each at a different place in the game): after a
  cutscene or the jump into the train's section of the level, the mod kept writing your look angles
  where it had found them before, which by then held something else. While the game draws without the
  head's turn, nothing is written now, and the look angles are forgotten and looked for again after.
  The user then made it through the jump and through the train window.
- **Cutscenes turning the wrong way** (the user: "if I move it to the left, it goes to the right"):
  in the opening interrogation the game draws from its own camera, without the head's turn, but the
  picture went to the headset as if seen from the head. Such pictures are now handed over as drawn,
  kept still in the room, and the head looks about them. Black shows past the game's 100 degrees.
- **The game turning the view itself** (scripted moments, vehicles): the head's look angles were
  written over it every frame. Turned by the game more than 2 degrees in a frame, the mod lets it lead
  until it has left the view alone for 1.5 s. weapon.txt `scripted 0` writes through as before.

### New
- **A crash log**: a crash the game doesn't recover from is written to bf3vr.log ("CRASH?"): where,
  what it touched, the registers and the code addresses on the stack.

### Known
- On the moving train the aim isn't found again after the jump (the search only tests with the camera
  still): the view follows the head, the gun doesn't aim the game.

## 2026-10-31, midday: the left hand aims, the thumb wraps the magazine (stage: pre-alpha, experimental)

Both tried in the headset by the user and confirmed; the self-test passes.

### Changed
- **The left hand aims, the right hand levels** (the user: "the left hand needs to be the one that
  helps with the aiming… the right hand should be just the leveler"). With both hands on the rifle,
  the gun follows the line between the hands whenever that line points out ahead of the body (within
  70 degrees of its heading, fading out by 100; not more than 50-75 degrees down) and the hands are
  more than 3-8 cm apart. The right controller only takes over for a hand beside, below or behind the
  body, or turned back past the wrist. The easing, rate limit and coast from the morning stay. The
  first try made the gun point up when aiming far right: the hands came 9-12 cm apart there, and the
  old 10 cm limit handed the aim back to the right controller ("Fixed" after lowering it). weapon.txt
  `steer 35 65` returns to the morning's cone.
- **The magazine held round its back edge** (the user: "I just need the thumb to be really wrapping
  around that magazine"; they picked option A of three rendered grips, then "I love it" in the
  headset): the back edge in the palm, the thumb flat along the far side, angled forward, the fingers
  along the near side with their tips at the front edge. The thumb can now swing out and roll about
  its length (`handpose` thumbOut, thumbRoll).

## 2026-10-31, morning: the gun stays in your hands, no lens-flare ring, a steady grip (stage: pre-alpha, experimental)

From the user's headset session and videos this morning. The grip and wrist fixes were seen in the
headset ("both fixed"). The gun and the lens flare were then tried in the headset one test at a time,
and all six passed: two-handed aiming smooth with no jumps; the gun staying with the right hand while
the left wanders with the grip squeezed; settling back gently onto the handguard; no jump letting go;
the ring gone and the lens blobs kept; a relaxed left hand not grabbing or sticking (the grip reads
1.0 holding, about 0.2 let go).

### Fixed
- **The gun jumping to wrong places** (the user's video: the rifle lying across the body, swinging
  round at the face, dropping out of view). It happened only while the left hand "held" the handguard
  (the grip squeezed for 38 s): the gun then pointed from the right hand to the left wherever the left
  hand went, and switched in one jump. Now the left hand steers the gun only while the line between the
  hands is near where the right hand points it (fully within 35 degrees, not at all past 65) and the
  hands are more than 10-15 cm apart; the change eases in and out over a tenth of a second, a tracking
  jump of the left controller can't throw the gun across the view, a short loss of it changes nothing,
  and a hold taken on the way in follows the hand onto the handguard. The hold itself still lasts until
  the grip is let go. `src/steer.h`, with an offline test (`steertest.exe`, 19 checks, in the
  self-test). weapon.txt `steer 35 65` tries other angles; the log says once a second how far the hands
  strayed and what the left grip read, to tune them from.
- **The "crosshair" seen when hit** was the sun's lens flare: an aperture-shaped ring the game draws
  near the middle of the view when the sun is in sight (it came with the sun while being shot at). It's
  taken off; the dirty-lens blobs from the same flare stay (the user likes them). The game's shaders
  are now known by their code from one start to the next (`src/shaders.cpp`). weapon.txt `lensflare 1`
  keeps the ring.
- **A stretched, spiky glove while gripping**: the glove's blend onto the handguard (and the handle and
  magazine holds) turned about each joint, not about a point 1.2 m away.
- **Black patches on the left wrist**: the bare forearm's bones now move with the glove's own.
- **The camera following the right controller**: a stray F4 had flipped the aim's direction (saved).
  F2, F3, F4 and Scroll Lock now need Ctrl, and the log warns if the direction is flipped.

- **The game's own crosshair is gone** (the user: "I still see the crosshairs when I take damage"):
  four faint ticks round the middle of the view, hard to see in the mirror, caught in the headset's
  own pictures while being shot at. They are four HUD quads on a 128x64 compressed sheet nothing else
  in the HUD uses, now skipped (weapon.txt `crosshair 1` keeps them). Confirmed gone in the headset.
- **The left hand on the charging handle** refitted: the index and middle fingers open in a V round
  the handle's shaft, nothing through anything (checked independently). Confirmed in the headset
  ("came out perfect"), as was the right trigger finger.
- **The magazine in the left hand** refitted at the user's palm width: the thumb lies along its side
  instead of 11 mm inside it, the fingers round its front edge, the little finger under the base
  plate, and the hand closes on it the frame it's taken. Not yet seen in the headset.

### Notes
- The orange markers the user saw in a recording are only in recordings: OBS takes each frame before
  the mod takes them off. The headset's picture doesn't have them.
- weapon.txt `turnby 30` turns the view at once (finding the sun with nobody in the headset).

## 2026-10-31, overnight: a self-test, casings out of the port, the flap with the handle, a smooth glove, no crash in the aim search (stage: pre-alpha, experimental)

Since the last upload. Worked on overnight with nobody in the headset: a stand-in hand holds the
rifle while the controllers sleep, and a self-test checks what broke before after every build. The
flap was tested in the headset by the user before bed ("the flaps are great"); the rest by the
self-test and the mod's own pictures, not yet in the headset.

### New
- **Self-test** (`tools\selftest.ps1`), run after every build: the mod's parts came up, the look
  angles were found and held, the frame rate, the view staying still while the gun turns 20
  degrees, the game aiming where the gun points (its weapon camera against the yaw written), the
  stick turning the way the user confirmed, both eyes' HUD corners alike, no streaks, errors in the
  log, and the glove collision test. PASS/FAIL per line.
- **Glove collision test without the game** (`collidetest.exe`): 25 scripted hand paths at 72 Hz
  (resting, soft and hard slaps, sliding, turning the wrist, through the gun, tracking jumps)
  against the rifle's shape, checking penetration, passing through, jerks and felt slaps.
- **Stand-in hand** (weapon.txt `standin 1`): holds the rifle out in front while the right
  controller sleeps, so the gun, aim and effects can be tested with nobody in the headset; with the
  headset off the frame's work goes on headless. `standin 1 45` looks 45 degrees up (test shots into
  the sky only), more numbers turn the aim and move the hold. While the left controller sleeps too,
  a stand-in left hand rests under the handguard, palm up, to see the left glove and forearm.
- weapon.txt `hudskip 10-19 40`: those HUD draws of each frame left out, to tell which is which;
  `hudskiptex 64x32` those with a texture that size. `tail` now also logs each HUD draw's
  constants, textures and vertices.
- **The right glove's trigger finger pulls with the trigger**: the game's own grip curls the index
  further, up to 22 degrees with the trigger all the way in, in the gloves' colour pass and both
  depth passes. The right hand's finger bones were mapped from the game's meshes (`handpose`:
  `BendRightIndex`, checked offline with `handview hand=r`). weapon.txt `fingertest 1` shows it
  without firing. Not yet seen in the headset.
- **The enemies' orange markers are gone** (the user: they were the "crosshairs"; there is no centre
  crosshair in the game). Their draws can't be told from the blue squad and objective markers (one
  icon sheet, coloured inside the game's shader), so the picture is copied before the HUD is drawn,
  and at the end of the frame everything the HUD drew within about 40 pixels of a pixel it made more
  orange is put back as it was. The blue markers stay. It costs no measurable frame time (13.9 ms a
  frame with it and without). weapon.txt `orangemarkers 1` keeps them.
- **Spent casings come out of the gun's ejection port** (the user: "every time I shoot"): each
  casing is its own small draw whose world placement is in its instance data, so while firing that
  placement is copied on the graphics card and moved by a small compute shader the way the gun was
  moved from where the game holds it to the hand. Fully moved near the port, less as it flies off,
  not at all 0.8 m out, so casings lying on the ground stay where they fell. In the pictures the
  casings now start 3-4 cm from the port; before, they started about 20 cm off, where the game's
  own gun is. No measurable frame time (bursts at 15.4 and 15.7 ms a frame with it, 15.1 and 16.2
  without). weapon.txt `casings 0` leaves them where the game throws them; `casings 1 0.2 0.8` sets
  how near the port they're fully moved and how far out not at all. Not yet seen in the headset.
- **Magazines from a pouch at the pelvis, pushed into the M416 by hand** (the user: "physically
  picking up a magazine ... from the pelvis area and manually inserting it into the gun"). With the
  magazine out (X ejects it), or the bolt locked back on an empty one, a fresh squeeze of the left
  grip in front of the lower belly puts the gun's own magazine in the left glove. That's below the
  belly button and above the groin, placed from the headset, and nothing is drawn there.
  - Brought top first up under the magazine well, it's caught in a funnel below the well's mouth that
    straightens it as it's pushed. It slides only along the well, the glove with it.
  - It seats when pushed home (within 5 mm, two frames), and only then do the rounds go in, with the
    magazine's sound.
  - Let go of before that, it falls, or goes back in the pouch if let go there.
  - The hold in the glove was fitted against the meshes so no finger goes into the magazine.
  - Tested in the game with a scripted hand (pouch.txt `test 1`, `test 2`): taken, caught, seated,
    30 rounds in. Not yet in the headset.
  - X with the magazine out still puts one in for now; `xinsert 0` in build\pouch.txt turns that off.
    Holding X is still the game's reload.
  - Any other gun reloads with the game's own reload when the left hand squeezes at the pelvis
    (`anygun 0` turns that off).
  - `zone` in pouch.txt moves and sizes the pouch. Every squeeze near it is logged with where the hand
    was, to place it from the first session.
  - The design came from three independent designs and a judge; a rifle magazine never goes into the
    grenade launcher's firing state.
  - An offline test drives it with scripted hands at 72 Hz (`magtest.exe`, 50 checks, now in the
    self-test). It and a three-way review found 15 problems, all fixed:
    - a 6-degree turn of the wrist knocked the magazine off the well;
    - a one-frame tracking jump threw it out;
    - a one-frame loss of the right controller made it vanish;
    - it could be caught coming down through the receiver;
    - the other guns' pelvis reload could take the squeeze meant for the handguard;
    - a dropped magazine fell from rest instead of at the hand's speed;
    - a few smaller ones.
    These fixes are tested offline only: the game couldn't be restarted (Battlelog had signed out).
- `tools\launch.ps1` clicks Battlelog's LAUNCH CAMPAIGN when PLAY CAMPAIGN opens the campaign page.
- **The ACOG's reticle and red cable stand out more**: they're see-through, so they're drawn again on
  themselves (the reticle five more times, the cable three), the strongest of three levels the user
  compared in the headset. weapon.txt `scopeboost 12 2 129 1` tries other levels.
- weapon.txt `scopetrace`: three frames of the scope's draws logged, each with the bones it was
  drawn with, whether it moved with the gun, and once per part how it's drawn (blending, its pixel
  shader's constants and textures). `zoom 1` holds the game's zoom (to test the
  Stinger's lock-on, which only locks zoomed).
- Research notes in `docs\research\`: FrostbiteVR (what works there, what applies to BF3),
  the mission's weapons, a 6DoF head plan, ten ranked next steps.
- **Wrist movement, step 1 (the left hand)**: the left forearm is solved from the glove's hand and a
  shoulder estimated from the headset, so the glove bends at the wrist while the forearm keeps
  pointing at the elbow, and the forearm takes two thirds of the wrist's roll at the watch
  (`src/armik.h`; three designs and a judge). **Off until it has been seen in the headset**:
  `on` in build\arms.txt turns it on; no file, or `off`, draws the forearm exactly as before. `log 1`
  logs the wrist's readings once a second; `test move`, `test neutral`, `test bend` and `test drop`
  move, straighten, bend or remove the stand-in's left hand. Tested offline only (`armtest.exe`, 14
  checks, now in the self-test with an in-game row too); not yet run in the game.

### Changed
- **The ejection port's flap follows the charging handle**: pulled slowly it opens slowly, fully
  open 4 cm back, and it shuts with the handle as the spring takes it home.
- **The free left glove no longer jerks against the gun**: it was kept from a point 1.2 m from the
  hand, so small turns of the wrist threw it by centimetres, and its slide flipped direction on the
  bolt catch's edges. Now it's kept by the hand's middle, pushed out by all the spheres in the gun at
  once, and slides smoothly (turning the wrist while touching: 35 mm jerks before, about 1 mm now).
  A hard slap can't pass through the gun (its path is swept, stopping at the first touch).
- Muzzle flash research: its sprites were read back (`flashprobe`): flat quads placed through their
  own block of constants, not the view as tried before. Moving that block's first matrix with the
  gun (`flashmove 1`, off by default) applied only some of the time and the flash wasn't seen to
  move, so the flash still comes from where the game holds its gun.

### Fixed
- **The ACOG's dark lens circle, red cable and red dot floated in the sky**, moving with the gun
  (the user's video): they're drawn later in the frame than the rest of the gun, where the mod
  never learned where they sit on it, so they stayed where the game holds its own gun. They now
  take the scope body's place. Confirmed in the headset by the user ("it seems to work just fine
  now"). Two earlier attempts at it were taken out again: one moved world-space scope parts near
  the gun, which could have taken a squadmate's scope onto the player's gun; one kept a copy of
  every bone upload, which the scope never needed.
- **The hands slid about on the gun while sprinting**, and the gun could flip between two holds
  twice a second (the user's videos): the mod copied how the game's hands hold its gun every frame,
  sprint animation and all. Now it copies that only from the game's normal hold: not while
  sprinting (nor for a second after), and not while the game's hands are more than 3-4 cm off
  their usual grip (its other moves), unless both stay put elsewhere for 5 s of the game running (a
  pause or the rifle put away doesn't count; only the hand that's off takes the new hold). The
  first hold after loading is followed until both palms stay still for a second, and every draw of
  a frame copies the game's hands or keeps its own together. A log line every 10 s says how the
  hold was judged. Not yet tested with a sprint (the keyboard's didn't move the player there).
- **In some launches the right glove stayed at the game's hip instead of on the grip**: where the
  game's hands hold its gun was only learned when the gun was drawn before the arms in a view, and
  which comes first differs from launch to launch (in one tonight it learned only after a reload,
  2.5 minutes in). The arms' bones are now kept until the gun is drawn and learned then: in a launch
  where every view draws the arms first, the grip was learned 0.1 s after stereo started.
- **Black slivers could float between the hands** with the left hand off the gun (found by a code
  review): 15 small collar triangles mix the body and both arms, and the arms now fold into their
  forearms' middles while the body still folded between the wrists. The body folds onto the line
  between the arms' fold points again, where those triangles have no area.
- **The stand-in's left hand acted as a real one** (found by the review): with the headset off it
  could take a hold on the handguard or chamber a round with the controllers' last buttons. It's
  now only drawn. If the controllers failed to start, its first pose also stayed in the room.
- **Black spikes on the left forearm** below the watch (the user's picture): the bare forearm's far
  end was folded into the palm with an all-zero matrix, which made its stretched triangles black,
  and the depth passes' differently cut copy of them left more. The bare forearms now move whole
  with their hands, and the arms' hidden parts fold into the middle of the forearm, inside it.
  Checked with the stand-in's left hand; not yet seen in the headset.
- **Squeezing the right grip threw a grenade** (it was bound to the game's G): the right grip is for
  holding the gun; the grenade is unbound until it gets its own gesture.
- **The game could crash right after a load**: the look-angle search tried 24 of 163 matching
  places in memory when the game's yaw happened to be near zero. It now never tries when that many
  match, and turns the view a little to look again.
- The stand-in hand and the self-test: controllers aren't taken as tracked while input isn't
  focused (their old poses stayed).

### Known
- In the car ride at the mission's start the game turns the look itself (the seat's limits, the
  vehicle turning) and the mod writes its own back each frame: the picture follows the head, but
  the body doesn't turn with the vehicle yet, and turning the head turned the view the wrong way
  (the user). Needs a test in the headset before changing.
- The ammo counter reads one more than the gun holds.
- Looking along a forearm into its open elbow end shows a black disc (the depth pass's folded arm
  inside it); from the side nothing shows.
- To come (the user's list): the muzzle flash at the gun, wrist movement for the gloves, a pouch
  reload of their own for the other guns and the launchers.

## 2026-10-30, late night: aim, both hands on the gun, collisions, both eyes alike (stage: pre-alpha, experimental)

Since the last upload. The game aims where the gun points and the view stays put however the gun
moves, the left hand holds the gun, the free hand rests against the gun instead of passing through
it, and everything drawn over the picture is the same in both eyes. Tested in the headset by the
user ("they land there": the shots; the view no longer moves with the gun; the smooth turn the
right way) and by the mod's own pictures of both eyes in combat.

### New
- **Both hands on the gun**: squeeze the left grip near the handguard and the left hand holds it
  until the grip is let go (not on sway, not on a far swing). The gun points from the right hand to
  the left, and the game aims along it. The glove is the game's own left hand as its animation
  holds this gun (made for it, the grenade launcher included, so it doesn't clip), its fingers
  moving a little with the controller's, easing on and off the gun.
- **Collisions with the rifle**: the free left hand rests against the gun and slides along it
  instead of passing through, and slapping the bolt release counts when the glove touches it. The
  rifle's shape is a distance field built from its mesh dump by `gunshape.exe` (into
  `build\guns\m416.bin`; it's the game's model, so it isn't in the repository). The launcher, the
  stock and the scope aren't in it yet.
- **Walking with the left stick as an analog stick** (the game reads a virtual controller), where
  the head faces, not where the gun points.
- **Sharper picture**: 2160 pixels per eye.
- **God mode keeps the spare rounds up**, for testing the reloads over and over.
- Tools: `gunshape.exe` (the rifle's collision shape and the glove's collision spheres),
  `gripfit.exe` and `leverfit.exe` (the glove fitted on the gun from the mesh dumps),
  `tools\gunammo.ps1` (the gun's ammo state from outside), `tools\make.bat` (build only some
  targets, e.g. the tools while the game runs). weapon.txt: `tail` (one frame's draws onto the
  window, in order), `viewdiff`, `viewlog` (the view's turn each frame), `aimtest`.

### Fixed
- **Every shot froze the game**: the window is bigger than the monitor, so the mouse pointer sat on
  the taskbar's "Show desktop" corner and each fire click minimised everything. The pointer is now
  kept on the game's window.
- **The view moved with the gun, and the game aimed the other way**: BF3's look angle grows turning
  right, the headset's turning left. The old angle finder flipped it; the search that replaced it
  didn't. The flip is now built into the aim itself (the gun's turn and the stick's), the angle is
  set outright each frame, and the picture goes by the angle written.
- **Shots didn't go where the gun pointed**: recoil or a turn during the check that proves the
  look angles let a wrong pair through. The check now waits for a still camera, nudges both the
  yaw and the pitch each its own way, and needs the camera to follow both and come back.
- **The right eye smeared into streaks when suppressed** (seen as a blinding blur): BF3 drew the
  second eye as a split screen's right half, and its screen effects read the scene past its edge.
  The right eye is now drawn as the left one is and moved into place; the player's blood, damage
  tint and HUD go into both eyes.
- **Magazines held 59 or 88 rounds**: the game reloaded by itself on top of a hand reload. The
  game's count keeps one hidden round so it never runs dry, and the rifle's magazine is 30.
- The grip let go when the gun was swung far right or left, the free hand jittered against the
  gun, the gun shook aiming up past the game's pitch limit, and the reload sounds were off to the
  right: all fixed. The ejection port's cover swings shut between shots, and the black layer round
  the arms, glove and gun is gone.

### Known
- The ammo counter on the HUD shows one round more than the gun holds (the hidden round).
- The lever hand still clips the charging handle's shaft.
- The ACOG's reticle can float away from the scope.

## 2026-10-30, night: smooth turning and reloading by hand (stage: pre-alpha, experimental)

4 commits since the last upload. Turning is smooth, the rifle is reloaded by hand with the game's
own sounds, and the left glove keeps the user's line-up. Tested in the headset by the user
(turning: "super smooth"; the reloads, the cover and walking: "it seems okay").

### New
- **Reloading by hand** (the rifle): run dry, the bolt locks back and the gun won't fire. With a
  full magazine in, slap the bolt release on the left of the receiver (the flat-screen game's empty
  reload), or put the left hand on the charging handle, hold the left trigger, pull back and let
  go: the handle slides back with the hand and springs home. The ejection port's cover swings open
  on its hinge when the bolt moves. With free hands the left trigger works the handle instead of
  aiming down sights (the scope is looked through by holding the gun up to the eye).
- **The game's own reload sounds**: the magazine coming out, going in, and the bolt going home,
  recorded from the game's empty reload and played at the gun when the hands do the same. They
  follow bf3.exe's volume in Windows like the game. They aren't in the repository (they're the
  game's): `capture.exe` records them; the README says how.
- **The left glove's line-up** made on the test setup is built in, so a new install starts with
  the glove where the hand is.
- Tools: `sampler.exe` (where the game's threads spend their time, from outside), `capture.exe`
  (records what the PC plays, finds and cuts clips, lists app volumes), `playtest.exe` (plays a
  reload sound as the mod does), `anglescan.exe` (finds the look angles and pointer paths from
  outside the game).

### Fixed
- **Smooth turning**: every frame BF3 asked Windows for memory statistics, which took 11 ms with
  hundreds of processes running and dropped the game below the headset's 72 fps. The mod answers
  from a copy refreshed once a second: a steady 72 fps. The right stick's turn now goes into the
  look angle the mod writes each frame instead of the mouse, so each frame turns the same amount.
- **Walking backwards**: at a new checkpoint the pointer to the player's look angles led to other
  memory (the game's facing stopped following the head, so forward could walk backwards, and the
  mod wrote over a pointer there). The mod now finds the angles from the player's soldier at every
  life or checkpoint, writes only to an object of their type, and of those only to the one that
  keeps a small nudge.

### Known issues
- The ejection port's cover is slow between shots.
- The ACOG's reticle and lens tint sometimes vanish from the scope and float in the distance.
- A dark layer shows around the arms, glove and gun against bright light.
- The muzzle flash, its smoke and the casings come from where the game holds its own gun (the
  chest), not the drawn gun.
- The charging handle has no sound of its own (the M16's reloads never use it).
- The HUD is drawn once across both eyes (the minimap in the left eye, the ammo count in the right).

## 2026-10-30, evening: per-eye rendering and a real left hand (stage: pre-alpha, experimental)

2 commits since the last upload. The 3D is now made by the game itself, and the left glove has
working fingers. Tested in the headset by the user ("this is amazing... beautiful").

### New
- **Per-eye stereo** (the new default): BF3 draws both eyes itself every frame, from the same game
  state, each from its own camera half the eye distance from the middle of the head, so there is
  real depth everywhere and no halo. It uses a developer setting the retail game still has,
  `GameRenderSettings.SplitScreenTestViewCount`: set to 2, the game draws a second complete view
  beside the first. The mod hooks the function that builds the frame's views (found in BF3's
  decrypted code) and makes the two views the eyes, in a window twice as wide (3200×1600, each eye
  a square 1600×1600 half); each eye is shown its half. The approach FrostbiteVR takes for Bad
  Company 2, found already built into BF3. Depth 3D and the other methods stay on Scroll Lock.
- **The left glove's fingers** follow the controller: the left trigger curls the index finger, the
  grip the other three, the thumb on the stick folds the thumb in. All three make a fist; the
  grip alone, a pointing hand. Each finger bends at its own joints, measured from the glove's mesh;
  relaxed, the fingers are a little curved and apart. Three small cracks in the glove model that
  bending opened are closed.
- Tools: `handview.exe` (the glove's finger poses drawn without the game, with checks for
  clipping and the mesh's openings), `dumpimage.ps1` and `findcode.ps1` (BF3's decrypted code saved
  and searched, for `dumpbin` disassembly), `dwords.ps1` (memory as values), `alarm.ps1` (rings
  every speaker when a test is ready).

### Fixed
- In per-eye views the hands and gun looked giant: BF3 draws the first-person parts of both views
  from one camera with no eye offset, which reads as far away. The free hands and gun are now
  shifted per eye (the two views' first-person draws come in order, the left eye's first).
- In per-eye views the picture showed as a small square: the headset was told the game's own 55°
  for a 100° picture. It now gets the angle the views were really built with.

### Known issues
- Smooth turning with the stick stutters a little: about 70 fps against the headset's 72.
- The HUD is drawn once across both eyes (the minimap in the left eye, the ammo count in the right).
- With free hands off, the game's own arms and gun have no depth of their own.
- `[stereo] async_handover=1` silently stops the gun's FOV fix and free hands: keep it 0.
- A short piece of bare forearm shows under the left glove's cuff.

## 2026-10-30, later: free hands (stage: pre-alpha, experimental)

2 commits since the last upload. First of the milestones that make it feel like VR.

### New
- **Free hands** (Insert on/off, on by default): the gun is drawn in your right hand, pointing
  where the controller points and held where the game's own hand holds it, scope and stock
  included; the left glove follows the left controller, open when empty, closing as you squeeze
  the left grip; arms, sleeves and body are folded away. On the Steam Frame the left View button
  lines the glove up with your hand. Steady while walking and strafing (tested in the headset).
- `tools\videoframes.ps1`: frames from a screen recording, without ffmpeg.

### Fixed
- After the opening ride the mod could lose the game's camera: the game then drew its own 55°
  and the headset stretched it over 100° (zoomed, blurry, big gun and arms). The camera now stays
  found, and if it is ever lost the headset shows the angle the game really drew.
- A dropped magazine no longer follows you along the floor (it vanishes as it lands).

### Known issues
- Free hands know only the campaign's M16-type rifle; bullets leave from where the game holds
  its gun, not the drawn muzzle; no two-handed aiming yet.

## 2026-10-30: v0.1.0-pre-alpha (stage: pre-alpha, experimental)

First upload marked with a stage. 14 commits since the last upload.

### New
- **Depth 3D** (the default stereo): each game frame becomes both eyes, every pixel shifted by its
  distance from the game's depth buffer, so both eyes update every frame and head turns no longer
  double the picture. The gun and arms get their real depth back (the game draws them squeezed
  towards the eye and marks them in the stencil buffer). Tuning in `build\depth3d.txt`; Scroll
  Lock cycles depth 3D / hybrid / alternate eyes / mono / frozen.
- **Wide field of view** (100° by default, Delete / Page Up / Page Down): the campaign renders a
  fixed 55° whatever the menu says. The render and cull views are widened, and the gun and arms'
  own projection is rescaled so they keep their real size.
- **God mode** (End, on by default): keeps the health of the soldier in the game's own server,
  which decides deaths. Scripted deaths still happen.
- **Bodies stay** (`[world] corpse_seconds`, 600 on the test setup): dead soldiers lie as long as
  that instead of the game's 15 s.
- **Magazine eject** on X (the campaign's M16-type rifle): the magazine drops out of the gun,
  tumbles and vanishes at the floor; its rounds go back to the spare ones, one stays in the
  chamber. X again puts a new one in; holding X is the game's own reload. The gun doesn't fire
  with the magazine out or one round left.
- **Steam Frame controllers**: their own SteamVR profile is used, so the buttons are where the
  Frame has them (A, B, X, Y on the right hand). Before, SteamVR guessed and put crouch on X.
- **Quick start** in the README, and `tools\selfcheck.ps1`, which confirms from the log that every
  part of the mod came up.

### Changed
- The picture no longer sways with the hand: the view turn is read from each camera the game
  builds.
- Faster: the research hooks are off by default (about +11 fps), the scan for the gun's projection
  skips upload sizes that stopped matching (about 1 ms a frame), the desktop window no longer
  waits for the monitor.
- The hands test reads its direction from `build\hands.txt`.

### Removed
- The old health finders (god mode finds the health itself now).

### Tools added
Memory research for the campaign (run with the 32-bit PowerShell): find and set values
(`findints`, `pokeint`, `pokeabs`, `findfloat`, `findhud`), pointer paths (`pathto`, `findref`),
the game's type tables (`fields`, `owner`), the server's soldiers and bodies (`soldiers`,
`corpses`). Pictures: `eyebmp` (saved eyes), `weapontest`, `fpmap`, `stencilbox`, `huntstep`.

### Known issues
- The head's position doesn't move the view yet (no lean or duck), and the gun is fixed to the
  view: it only points where the controller points.
- Only the M16-type rifle's magazine is known; the SMAW and the grenade launcher make aiming go
  wrong.
- The crosshair stays in the middle of the view, and HUD text takes the depth of what's behind it.
- Around near things against far ones the 3D can show a soft halo.

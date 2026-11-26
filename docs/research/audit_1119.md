# The audit of the 2026-11-19 play: what this branch fixes, and the exact fixes for the landing branches

The audits read the owner's Swordbreaker play of 2026-11-19 (`build\logs\bf3vr_1119_userplay_holster.log`), the players' reports and
the code of main 85d59d2 (released as v0.1.14). Their High and Medium findings, and where each one stands. Nothing here has run in the
game: every change is checked by the offline programs (all of them, never playtest or xrtest) and needs the proof run at the end.

## Fixed on this branch (main 85d59d2 + 223a063, settled by the review of 2026-11-25; main 4943aca and 0704dc5 merged in)

| Finding | What changed | Where | Checked by |
|---|---|---|---|
| High: a single quick-time key awaited in play took the first fresh squeeze of either grip, and the hands took the same squeeze (a draw, a magazine, the handguard, a door, a gun off the floor) | One owner per squeeze. In play (no scripted moment, no fight's prompt) a hand that carries something never answers a key (`qte::In::holds`, from `hands::HoldsSomething` by `qte::HoldsForKey`: a magazine, round or case, the charging handle, the pistol's slide or magazine, a prompt's use, the gun in a hand not seen at a holster's place; settled 2026-11-25: the gun's own holds, its grip, front, handguard or a front grip, never count). A squeeze that presses, or is owed a press for the key-up gap (settled 2026-11-25), is the event's from its first frame till that grip is under 0.4 (`qte::OwnerStep`, `input::QteOwns`): the grasp, holsters, the use, the pouch, handmag, flap, rocket, the M320, the shotgun's shells, the charging handle, the pistol's hold and magazine and slide, Support and LauncherSupport take nothing with it. Each press in play is said ("QTE: the left grip's squeeze is the quick-time event's (in play)") | `src/qte.h`, `src/input.cpp` (QteOwnStep), `src/hands.cpp` (QteHand at every take), `src/hands.h` | qtetest 30a-30f, 30h-30j (29a-29h in 223a063: renumbered past main's own 29a and 29b at the merge) |
| Medium: answering a key in play with the gun's own grip dropped the gun | While a key is awaited in play the pistol-grip hand is disarmed (`grasp::Frame::keyAwaited`, as a scripted moment disarms it) and so is the front hand's let-go (`g_barrelArmed`): no let-go drops a gun while the prompt is up, for 10 s of a key at most (settled 2026-11-25: `qte::KeyKeepsGun`, a trigger left live in free play never keeps a gun undroppable; said as it passes); a squeeze past 0.6 once the key is gone arms it again. So either grip answers, the gun's own too, let go and squeezed afresh (README and START HERE, as 0.1.14 said it, with the gun kept in the hand) | `src/grasp.h`, `src/hands.cpp`, `src/qte.h`, `src/input.cpp`, README | grasptest 86a-86c (and mirrored; 85a-85c in 223a063, moved past main's own round 14 row 85 at the merge), qtetest 30b, 30k |
| Medium: a fist fight whose camera isn't scripted left every hold live | A fight's run (`qte::FightStep`: its prompt, or one within 6 s while a fight is still read; 0.25 s over a frame not read) is ORed into `hands::SetScripted` (xr_runtime.cpp), and both grips are the fight's (`QteOwns`). Said once a fight when it runs outside a scripted moment ("QTE fight: a fist fight runs outside a scripted moment"): the runbook's open item answers itself in the next log | `src/qte.h`, `src/qtewatch.*` (`Frame::fights`), `src/input.cpp`, `src/xr_runtime.cpp` | qtetest 30g |
| Medium: a prompt by reach next to a lying gun was decided by pickReach while the grasp reached further | One measure: `UseYields` measures the lying gun and each candidate by the grasp's own reaches for that hand (pickReach; the magnet's sphere for a settled hand and a gun at rest; its column for a hand reaching down) and the prompt's target by its capsule or point (`usepoint::TargetDistance`); the gun goes first only when nearer (`usepoint::GunFirst`, a tie the prompt's). A prompt that gives a gun whose thing lies off its point unmeasured (the AT4's own 1 m disc, no capsule yet): the lying gun goes first only for a hand practically on it, its box within 8 cm, and never by the magnet's column (`usepoint::LyingFirst`; the integration review of d32a39b, finding 2: pickReach and the column had taken a squeeze made on the tube with the rifle 5-15 cm beside it, or 35-60 cm over the rifle; a door with its own disc would keep pickReach) | `src/usepoint.h`, `src/hands.cpp` | at4test t13, t15 |
| Medium: a prompt by reach never yielded at the belly or the chests | `holster::AtBodyTake` (the belly's zone grown 5 cm, the chests' eggs grown 3 cm), from the neck point (`grasp::FromNeck`, `pouch::BodyFrame`), holsters on or off: such a squeeze is never the use's, but for a prompt that gives a gun while both hands are empty (nothing there takes a squeeze then: `usepoint::BodyFirst`; the integration review of d32a39b, finding 7). The use's own squeezes bar the grasp from their first frame (`spent`, as before); the other modules (Support, LauncherSupport, the pouch, handmag, the handle...) only once the use is asked or refused with it (`usepoint::Out::claimed`, `g_useOwnsBy`; settled 2026-11-25: before that a module's take wins and the use gives up, as usepoint.h always said: the handguard squeezed 20-35 cm out and brought to it, the belly's 0.4 s window) | `src/holster.h`, `src/grasp.h`, `src/hands.cpp`, `src/usepoint.h` | at4test t13, t14, t15 |
| High: the Mk11's second renderer cost the owner's floor (61.9 fps on average with it on, 31% of seconds under 60) | The frame-rate guard (`src/scopegov.h`), settled 2026-11-25: THE OWNER'S SWITCH, weapon.txt `scopegov 1` acts, `scopegov 0` (the default) only watches and says once a trip what it would do (never trade looks for frames without asking him: performance-last; the real picture through every scope: scopes-real-picture). Once a second, from the pace (frames over the second's own length measured between frames, at any refresh; whether they went to the headset), three seconds running under 60 frames a second (rounded) with the view on in at least half the frames: hold the view back 10 s (the lens magnifies the eye's own picture, as `scopeview 0` does), then back only once no magnified lens is looked through (at the next raise, never mid-aim); held again within 2 minutes of the last hold's END, twice as long, 60 s at most. No late-frame rule. Said as it's held back and let go ("Scope view: held back ...", or "... scopegov 1 would hold it back ..."). The lens turns the view on within 0.38 of its line as it always did, only for a lens no more than 1.35 times the eye's pixels per unit of slope in radius and in front of the eye; once on, only in front (an eyepiece pressed close keeps it). Replayed on the owner's 11-19 seconds: 3 trips (00:58:39, 01:13:37, 01:14:13), 38 of 178 scoped seconds held, 31 of them under 60 in the log (the 223a063 rule: 10 trips, 125 held, 7 trips on seconds at 60-65) | `src/scopegov.h`, `src/scopeview.*`, `src/xr_runtime.cpp`, `src/skin.cpp` (weapon.txt), `src/recoilrun.cpp` via `scopeview::Showing` | scopegovtest G1-G11 |
| Medium: a whole-memory scan at every swap of held-gun kind | Per kind since the last load: a swap back to a kind whose rows (each tagged with its kind now) still read as the game's or as written, or whose AT4 rocket start is still its own, starts no scan ("Held gun: ... still there: not looked for again"). A new kind, data gone, a load (the 15 s gap), ten minutes, a take of the kind: scanned as before. Every scan thread runs under `THREAD_MODE_BACKGROUND_BEGIN`, and one at a time (a draw that wants one while another runs keeps it pending and starts it the first frame after) | `src/skin.cpp` | compiles; the rule needs the game (proof run) |
| High: the README and START HERE said the AT4 could be taken with a gun in the other hand, and "squeeze afresh for each prompt" | README 41, 126, 127, 148, 150, 176, 187 and START HERE (`tools/package.ps1`): the AT4 needs both hands empty (drop the rifle or put it on a shoulder), then reach and hold the grip or hold X (Index: left A); doors and wires take a free hand with the rifle in the other; in a fight a grip already held counts; in play either grip, let go and squeezed again, the gun kept in the hand (settled 2026-11-25; 223a063 said an empty hand's only) | README.md, tools/package.ps1 | read |
| Medium: the report made 1 s after the mod went in held nothing of the play | The launch-time report opens with "MADE AS THE GAME STARTED, BEFORE ANY PLAY ..."; the launcher leaves a hidden watcher (`tools\report.ps1 -Quiet -AfterGame <bf3's id>`, Idle priority, reading only) that makes `BF3 VR report.txt` again when the game closes ("Made when the game closed") | tools/report.ps1, tools/launch_vr.ps1 | run against a stand-in process: the report made 3 s after it closed |
| Medium: Page Up / Page Down needed dev_keys=1 (a player fixed double vision with them) | Player's keys again (they only scale the picture, with the game's window in front). Their values go to `build\bf3vr.player.ini` too, read after bf3vr.ini and over it, never in a release's zip: an update unzipped over the folder keeps them. README: a double-vision entry, the keys in the players' table; START HERE: the keyboard line | src/xr_runtime.cpp, src/config.*, README.md, tools/package.ps1 | compiles; needs the game |
| Medium (aim, the part on main): the report's key lines didn't show the aim's state | `tools/report.ps1` key lines: the aim's state lines ("Aim: no look angles", "the look angles found", "the search took", mouse aim, the scripted moment), `Keys:`, the display FOV and the player's own view, this branch's QTE and scope-view lines; and bf3vr.player.ini's values | tools/report.ps1 | parsed |

### The review of 2026-11-25 (two reviewers of 223a063) and what the settle changed

Every High and Medium was checked in the code and found real; each is fixed above (the rows marked "settled 2026-11-25"):

- **Scope guard (review 2):** H1 the late rule tripped at 64 frames at 72 Hz (the owner calls 60-70 fine) and M2 it was reckoned against 72 Hz
  (82 at 90 Hz, every 144 Hz second): no late rule now, frames over the second's own length under 60, three seconds running. M3 the guard
  flapped 8, 16, 32, 60, 8 (calm measured from the hold's start, kCalm = kHoldMost): calm from the hold's end, 2 minutes, and a hold lets go
  only with no magnified lens looked through. M4 the fallback was unstated and on by default: weapon.txt `scopegov 0|1`, off (watching) by
  default as the owner's call, README (What works today, Low frame rate) and a CHANGELOG block. M5 the 0.2 turn-on changed every raise: 0.38
  again; the size cap only to turn on (an eyepiece pressed close keeps the view), in front for both.
- **Grips (review 1):** M1 the use's claim went to every module from the squeeze's first frame, so the handguard squeezed on the way in and the
  belly's window lost to a prompt by reach: the modules are barred only once the use is asked or refused (`Out::claimed`). M2 in play the gun's
  own grip could never answer (with the rifle in both hands no grip did): the gun's own holds never count (`qte::HoldsForKey`).

Lows fixed with them: review 1's 4 (a press owed for the key-up gap now the event's, and dropped for a hand that comes to hold something: qtetest
30j) and 7 (the gun kept in the hand 10 s of a key at most: 30k); review 2's 6 (recoil's "at the eye" while the guard holds the view back:
`scopeview::Showing` true for a magnified lens looked through), 9 in the README (the file named, delete it to go back), 12 (scopegovtest G1-G11:
72, 80, 90, 120, 144 Hz, a trip right after a 60 s hold, the release at a raise); and the words of grasp.h's keyAwaited and grasptest 86c (85c before the merge)
(review 1's 3: keyAwaited alone bars no take; the squeezes the key answers are busy).

Left as they are, for the owner or a later run (Low): review 1's 3 (in play a free hand's squeeze at the belly or a shoulder presses the key
rather than taking a magazine or drawing: the windows are short and rare; the README says so), 5 (the magnet's column against the AT4's
unmeasured disc: the AT4's capsule row settles it, see below), 6 (a fight's 6 s tail into play; it pulls against review 2's 10, which wants a
longer gap: both need a fight run outside a scripted moment, never seen yet: the proof run's line says); review 2's 7 (background-priority scans
and the spread mutex), 8 (the rescan skip and a picked-up gun of a scanned kind: no shot data seen on a pickup yet), 9's code (player.ini over a
changed `[window] size`), 10 (see 6), 11 (the hidden report watcher and Defender: speculative).

## For the merge step: the landing branches

### Bigger holsters, swing and let go (worktree-wf_edba319c-aab-1: 706a8e2, its merge b9eebc9, c6dabab)

DONE at this branch's merge of main 4943aca (2026-11-25), where they came merged already (5251532): `AtBodyTake` kept beside the swing's
`OutOf`, `AwaySpeed`, `TowardCos` and `SwungTo`; main's `KeptOut` grows the belly by kPutBackMore plus keepOut (7 cm at the default, 2 cm
more than AtBodyTake's) and the chests by keepOut (2 cm, 1 less), so `UseYields` asks `holster::AtBodyTake(fromNeck, g_holsterSet)`: the
plain test OR KeptOut, the use never taking a squeeze where the holsters keep the body out. The grep finds no new take site (the swing's
put-away and the any-speed draw go through holster.h, `HolsterHand`'s `h.busy = g.busy` carrying QteHand); `HoldsSomething` needs nothing
more (the gun's own holds count for nothing now, the review's finding 2; `g_holsterHold` still counts); holstertest 241 passes, HT6's lines
word for word. What follows is the plan as written before the merge:

1. `src/holster.h`: this branch adds `holster::AtBodyTake` just above `Nearest`. Keep it as it is. The swing branch's `KeptOut` takes the
   head, the chests and the belly out of every place, so holsters' `hr.place` never covers them there either: `UseYields`' body test is what
   keeps a door from taking a belly or chest squeeze. If the swing branch's belly or chest eggs are bigger than `ammozone::kBelly` grown by
   `kPutBackMore` and `kLeftChestEgg` grown by 0.03, make `AtBodyTake` test the branch's eggs instead (the same function, its eggs in place of
   these), so the two never disagree about where the body is. Do not add the head: a hand by the face at a door is the door's.
2. `src/hands.cpp`: every path by which the branch takes a gun with a squeeze (the any-speed draw by an empty hand's fresh squeeze, the swing's
   put-away, the grace) must read the hand's busy state that `FreeHands` builds: `h.busy = h.busy || QteHand(s);` at the top of the hand loop
   (it reaches `HolsterHand`'s `h.busy = g.busy`). After the merge: `grep -n "handFree =\|\.busy =\|fresh = true" src/hands.cpp` and give
   every new site `|| QteHand(s) || g_useOwnsBy[s]` (busy) or `&& !QteHand(s) && !g_useOwnsBy[s]` (handFree), as this branch does at each.
3. `hands::HoldsSomething` (the end of `hands::Update`): if the branch keeps a gun "in the hand" through a swing's grace or a hand not seen,
   that hand must count as holding it (the publish ORs `g_holsterHold[s]` already; add the branch's own keep flag if it has another).
4. holstertest HT6 reads hands.cpp's lines word for word: this branch changed none of them (UseYields' first rule, the gift's mark, the busy
   line `h.busy = h.busy || hr.busy[s] || uo.spent[s] || giftWay;` are as they were).

### Any grip, any lever (the right hand on the held guns' levers, boxes and shells; the diff the audit read is `scratchpad\audits\anygrip.diff`)

DONE at this branch's merge of main 0704dc5 (2026-11-25; any grip came merged as fae59cf, hosting as 0704dc5): the held magazine's frame
now feeds either hand (workhand::FedSide), and both its handFree lines carry the one-owner terms (the right's `!g_onLauncherBy[1] &&
!g_onGripBy[1] && !g_spentBy[1] && !QteHand(1) && !g_useOwnsBy[1]`, the left's as below); a lever, box or belt in the right is busy
(`g_busyWasBy[1]` ORs `heldWorkBy[1]`), so it counts for `qte::HoldsForKey`; shellload's and the M320's busy lines had both terms already;
grasptest keeps main's row 85 (round 14) and the branch's keyAwaited rows move to 86. Hosting needed nothing here. The plan as written before:

1. The left's `hf.handFree` in the held magazine's frame conflicts with this branch. Resolve to
   `hf.handFree = !g_onLauncher && !g_lining && !g_leftOnGrip && !g_leftSpent && !QteHand(0) && !g_useOwnsBy[0];`
2. Its new right-hand line becomes
   `hf.handFree = !g_onLauncherBy[1] && !g_supportingBy[1] && !g_onGripBy[1] && !g_spentBy[1] && pouch::HoldingSide() != 1 && !QteHand(1) && !g_useOwnsBy[1];`
3. Any new `busy` for the right in `src/shellload.cpp`'s or the M320's frame: `|| QteHand(1) || g_useOwnsBy[1]`.
4. `hands::HoldsSomething`: a right hand holding a lever, a box or a shell must count. If the branch's right-hand state isn't in
   `g_busyWasBy[1]` (busyBy, at `g_busyWasBy[1] = busyBy[1];`), add it to `qte::Carry`'s `item` in the publish at the end of `hands::Update`
   (settled 2026-11-25: the publish goes through `qte::HoldsForKey`; the gun's own holds never count, a carried thing always).
5. (Settled 2026-11-25) `g_useOwnsBy` now means the use's claim (asked or refused: `usepoint::Out::claimed`), not its spent squeeze: the lines
   above stay as written.

### The bridge M249's ACOG (worktree-wf_041ddbae-752-2, not committed at 16:02)

Merged into main (81c028b) before this branch's merge of 4943aca: nothing of it needed changing here.

1. Merge it after this branch. The frame-rate guard (`src/scopegov.h`) works on the lens (any sight with power above 1.5), not on a gun, so
   nothing in the ACOG's code needs to change for it; but it acts only with weapon.txt `scopegov 1` (the owner's call, off by default): by
   default the bridge ACOG's second renderer costs what the Mk11's does, and its "would hold it back" lines say how often it takes the overpass
   under 60. Keep the mount math as written (`lidride::OnMount`, `m249rows::kSightMount`), and the M416's ACOG bit for bit as it is.
2. Measure on the bridge with the scope up, nobody else running a compile: the "Frame timing" and "Pace" seconds, "Graphics card timing: up to
   the end of the view through a scope", the "Scope view: on" lens line, and whether "Scope view: held back" comes. The owner chooses the look
   from there (see the Mk11's settings below).

### The Saiga 12K and the PKP (worktree-wf_7055b337-795-1 and -2, not committed at 16:02)

1. Land them. Their rows in `ClearRifleSpread`'s `kGuns` get their kind on every scatter row by themselves (`r.kind = wanted` is set in the
   loop), so a swap back to them is verified, not rescanned. A kind whose scan finds no rows is scanned again at each swap to it, as before.
2. A PKP with a PK-A (2.95x is a row already) is covered by the frame-rate guard as it stands (when the owner switches it on: `scopegov 1`).
3. Check Swordbreaker's floor AK-74M: its draw keys against the Rock row (a census of the draw with it held on Swordbreaker; the keys in
   `skin.cpp`'s held-gun table). Not done offline: the data is the game's.
4. Before the next missions: a generic hold for any gun the mod doesn't know (`src/gunlearn.cpp`, written, not wired in): wire it so an
   unknown gun gets a held row from its draw (palms from its bones, its sights' line from the zoomed eye) instead of staying locked to the
   game's camera, the 3DoF look kilo and Notagameaddict reported. Every new mission brings unheld guns (docs/research/missions/PLAN.md).

### Aim after a checkpoint (worktree-wf_c3cd0d1c-7ef-1, not committed at 16:02)

Merged into main (4943aca, the look angles' reach) and so into this branch.

1. Land it with a test row that replays the 22:04 case of `build\logs\bf3vr_1118_m249ingame.log` (22:04:06-22:09:45: "no look angles yet"
   every 3 s for 5.7 minutes after LOAD LAST CHECKPOINT): the search must find the look angles within seconds of the new soldier, as the
   roof's deaths did (2-30 s).
2. The report already shows the aim's state lines (this branch's `tools/report.ps1`): no change needed there.

### The AT4's capsule

`kUseCapsules` (hands.cpp) has no row for `ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG`: where the tube lies was never measured (its stills and an
estimate only). Until it is, `usepoint::TargetDistance` gives the AT4's own disc no distance and the rifle beside it goes first only
for a hand practically on it, its box within 8 cm, never by the magnet's column (`usepoint::LyingFirst`: the integration review of d32a39b,
finding 2; within 25 cm and by the column the rifle took a squeeze made on the tube). Never the finder's estimate as the row: a capsule
replaces the 1 m disc in `usepoint::Reach` with 0.205 m round the tube, so a 20 cm error makes the AT4 unreachable. At the prompt, a census
or drawscan of the tube gives its two ends in the world: then the row
`{"ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG", {x0, y0, z0}, {x1, y1, z1}, 0.055f},` and the one-measure rule (at4test t13, t15) covers flow (b).

## The proof run before the next release (no code; nothing of it has run in the game)

- A fist fight with a grip held (Swordbreaker's cellar): each prompt answered; no "QTE fight: a fist fight runs outside a scripted moment"
  line if the camera is scripted (if it comes, the hands were held still through it: check nothing dropped).
- A single key in play, if a level shows one: the free hand's squeeze presses it ("QTE: the ... grip's squeeze is the quick-time event's (in
  play)"); the gun hand let go and squeezed again presses it too, the gun not dropped meanwhile; a hand holding a magazine presses nothing;
  nothing else is taken with that squeeze; past 10 s of the key, "QTE: a quick-time key awaited in play 10 s" and a let-go drops again.
- A prompt by reach (a door, the roof AT4) with the left squeezed on the way to the handguard: the handguard taken ("Hands: the left hand took
  hold of the handguard"), the use given up, the door not opened; the same squeeze held at the door 0.25 s: the use, nothing else with it.
- The roof AT4: refused with the rifle held (double buzz, the "Use: ... drop it or put it on a shoulder first" line); taken with the rifle
  dropped or holstered; the rifle dropped 10-15 cm beside the tube: the hand on the tube takes the AT4, and so does a hand coming down over the
  rifle from 35-60 cm (only a hand on the rifle, within 8 cm of it, takes the rifle); a hand resting at the belly with both hands empty: the AT4.
- Crouch after standing, a sprint and a death; the Mk11 handle; a magazine in the hand after a death (0.1.14's fixes, never run).
- The Mk11 in Swordbreaker's street: the seconds with the scope up ("Pace", "Graphics card timing"), the "Scope view: ... scopegov 1 would
  hold it back" lines (the default, watching), then the same street with `scopegov 1` in weapon.txt: "Scope view: held back", the softer
  picture for 10 s or more, back at the next raise, never mid-aim; the owner chooses the default from seeing both. Then
  the scope view's own settings one at a time, to let the owner choose the look: `scopeviewshadows 0` (read when the view is made: a level
  load after writing it), `scopeviewdetail` on either side of its 1.15 (the ratio its level of detail is picked by; which way is cheaper is
  what the measurement says), `scopeviewparticles 1` in weapon.txt, each measured the same way.
- Swaps M4A1 / Mk11 / M249 within a minute: "Held gun: ... still there: not looked for again" at each swap back; no "Rifle: its scatter's
  numbers found" more than once per kind after the level's start; shots still from the muzzle with each.
- Page Up and Page Down without dev_keys: "Display FOV" / "Render FOV" lines, `build\bf3vr.player.ini` written; after a restart "Config:
  the player's own view from bf3vr.player.ini".
- The launcher: after closing the game, `BF3 VR report.txt` begins "made ... by itself as the game closed".

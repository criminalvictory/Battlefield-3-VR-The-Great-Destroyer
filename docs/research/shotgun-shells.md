# M1014 (Benelli M4) reload by hand: design

Designed 2026-11-18 on top of the code as it stood at cf9a900. **Built so far: only the offline core of section 1**
(`src\shotgunammo.h` and its test `tools\shotgunammotest.cpp`, in the offline list of `CMakeLists.txt` and
`tools\selftest.ps1`). Nothing in the game runs it yet: `ammo.cpp`, `hands.cpp`, `input.cpp` and `skin.cpp` are
untouched, because the gun isn't measured (section 7). Section 1 below is the core as built, corrected where
building it showed the first design wrong or short, and again after an independent review of the core the same
day (the asking ended by a shot, X the only asking, the game's reload never written into while it runs, the
hidden round left in on a full tube, "from empty" judged once per reload; 1.3); sections 2 to 8 are still design
only. What is left is listed in section 9.

## 0. What the code has today, and two points to settle first

- **The M1014 is not a held gun yet.** It has no row in `kHeldKinds` (`src\skin.cpp`, ~8693), so it is drawn with
  the game's own hands and reload. It does have a recoil row (`src\recoilguns.h:314`: kShotgun, 220 rpm,
  magazine 5, 10 pellets, shape nullptr).
- **Onboarding as a held gun comes first:** a `kHeldKinds` row, the `guns\m1014.bin` shape, a `gunbodies.h` body,
  a `gunprofile.h` `kNames` row, and shape "m1014" in the recoil row.
- **Capacity (settled).** The task said 5 shells (7 extended) plus one chambered. The game's `MagazineCapacity`
  is 5 (7), and the catalog reads that as 4+1 (6+1), chamber included.
  - As designed: tube = `row->magazine - 1` (4, or 6 extended), plus the chamber, as `UpdatePistol` does for the
    M9. The HUD and the 32-shell pickup then stay true, and the game's own reload fills the gun completely.
  - The core still takes the tube's size as a number from its caller (the test runs tubes of 4, 5, 6 and 7), so
    nothing in it depends on this.
- **Catalog decisions owed (5.5 item 7), with my defaults:**
  - One shell per reach to the belly (as the M320's round); the hand module keeps a count so two can come later
    (the core's `CanTake` already takes the number of shells in the hands).
  - Both the port load and "tube then release" are allowed.
  - The bolt goes home by the release button or by a tug of the handle.

Models followed: `src\pistolammo.h` for counting, `src\m320.h/.cpp` for a round pushed in on a line by either
hand, `src\pistolslide.h` for the bolt handle, and `reload::Slapped` (`src\reload.h:65`) for the release button.

## 1. States and rules: the pure core `src\shotgunammo.h` (built)

Header only, no Windows, no game calls, no logs. Test: `tools\shotgunammotest.cpp` (94 checks; section 1.4).
The first design called it `tubeammo.h`; it is `shotgunammo.h`, namespace `shotgunammo`.

### 1.1 The gun as the mod counts it

```
enum class Chamber { kNone, kLive, kSpent /*a fired hull: the pump's, later*/ };
struct State {
    int tube;         // shells in the tube
    Chamber chamber;
    bool locked;      // the bolt held open
    bool port;        // a shell lying in the ejection port (only with the bolt open and the chamber empty)
    bool down;        // the hammer down on an empty chamber: no second click until the bolt is worked
};
Total(s) = (chamber == kLive) + tube      // the port's shell is not counted yet
```

- The tube's size is not in the state: every operation that needs it takes it (`capacity`).
- `down` is new against the first design: "the first pull clicks, later ones are silent" needs it.
- There is no `Action` enum yet. The pump's own operations go in beside these (section 8); `kSpent` is in the
  enum already and every operation treats a fired hull correctly (no shell of the count, nothing to fire, thrown
  out by the handle at no loss, no port load over it).
- Always true (the random runs check it): the bolt is never open on a live shell; a shell lies in the port only
  with the bolt open and the chamber empty; the hammer is never down with the bolt open or a live shell chambered;
  the tube is never over its size.

### 1.2 What the hands and the gun do

- **`FromCount(total)`:** chamber live if total >= 1, tube = total - 1, bolt open if total < 1. For a new gun, a
  checkpoint, the train's scripted pickup and the forced kit.
- **`Fire(s, shots)`:** each shot takes the chambered shell and feeds one from the tube. With the tube empty the
  bolt stays open (always: there is no "magazine out" case). It cocks the hammer.
- **`LoadTube(s, capacity, belly)`:** tube + 1. Refused when the tube is full (`kFull`: the shell stops at the
  lifter) or no shell is left at the belly (`kNoShell`). Allowed with the bolt home or open. It never chambers.
- **`PortLoad(s, belly)`:** only with the bolt open, the chamber empty and the port free; sets `port`. Refused
  with the bolt home (`kBoltHome`), a shell there already (`kTaken`), none left (`kNoShell`).
  - The port's shell is not counted until the bolt takes it: to the game it is still a spare shell. So nothing
    has to be undone if it never gets chambered (a checkpoint, the game's own reload).
  - `Belly(s, in)` is what a hand can still take: the game's spare shells, less what the gun holds that the game's
    count hasn't been told of yet, less the one in the port. So the port's shell can't be taken a second time.
  - `CanTake(s, capacity, belly, inHand)`: yes; none left; or "the tube is full" (and the port takes none).
- **`BoltHome(s)`** (the release button, or a tug of the handle off its hold): only with the bolt open.
  - a shell in the port is chambered (now it leaves the spare ones);
  - else one from the tube is chambered (the forgiving "tube then release");
  - else the bolt goes home on an empty chamber.
- **`Rack(s)`** (the handle drawn fully back from home and let go):
  - a live chambered shell is thrown out and lost from the count, as the pistol's (it falls as a loose live shell;
    the caller adds one to its `lost` tally);
  - then one is fed from the tube, or with the tube empty the bolt stays open;
  - with the bolt open already it is `BoltHome`.
  - **This is forgiving, not the real Benelli M4**, and marked so in the header, the same way "tube then release"
    is. On the real gun a shell leaves the tube only when the hammer falls or the cartridge drop lever is
    pressed: racking a cocked M4 throws the chambered shell out and closes on an empty chamber (that is how it
    is unloaded, and how a slug is swapped in through the port). Here racking always feeds from the tube, which
    is what most players expect of a shotgun. The owner chose forgiving rules for the pistol's slide too. The
    real rule, if it is ever wanted, is one input more (the hammer cocked or fallen, and the lever) and a
    `Rack` that feeds only when a shell has been let onto the carrier.
- **`OnPull(s, handleHeld)`**, and `Pulled`, which also lets the hammer fall:
  - `kFires`: chamber live, bolt home, handle not held;
  - `kNothing`: bolt open or handle held (out of battery, no click), or the hammer down already;
  - `kClick`: home on an empty chamber with the hammer cocked. Firing, the release and the handle cock it again.
  - The trigger never sends the bolt home: the pistol's `kSendsHome` is its own rule and is not copied.

### 1.3 The game's count: `See(CountIn) -> CountOut`

As `pistolammo::See`, with these rules:

- The game's count is kept at `1 + Total` (the hidden round), so the game never reloads by itself.
- `raw < last`: shots, through `Fire`.
  - A shot the game made with nothing chambered (the mouse passes no trigger's stop; the bolt open on shells in
    the tube) takes **a shell of the tube's**, not one from the belly: nothing loads by itself. (The pistol pays
    such a shot from the spare ones; that is its own rule.)
  - With nothing in the gun and the count at 0 it is the hidden round's: put back from the spare ones, at most
    `kMostPutBacks` times; then left at 0 for the game's own reload, which is given back when it has ended.
  - **A shot ends the asking** (`CountOut.shot`, set on every look that sees the count fall): that look treats
    the reload as not asked for whatever `asked` says, so the hidden round is back in the same look; the caller
    forgets its X and shell times, and X held through the shot asks again only by a fresh press. Before this, 2
    shells left, X tapped and both fired within the asking's 3 s left the count at 0 with the hidden round out,
    and the game reloaded the gun by itself, each kept shell extending the asking.
- Any other difference is settled to or from the spare ones, less `lost`.
- The spare ones short of the difference: the gun holds as many fewer (`adopted`), the tube's shells first. This
  never happens when a look is made right before and after each move of the hands, as `ammo.cpp` does for the
  pistol (the careful random run counts 0).
- `CountOut.hidden` and `CountOut.lost` are the caller's to keep once the write is made (or nothing was to write).

**The one real difference from the pistol: the game's own reload (X held).** It raises the count one shell per
loop, so the pistol's `FromCount(raw)` on a rise would count the hidden round as a shell. Built:

- **`asked` is X only.** The pistol also counts "a swap of the game's just now" as an asking; this gun must not:
  with the hidden round out the game reloads an empty shotgun by itself.
- While asked **and the tube has room**, the hidden round is **taken out** of the game's count (for nothing), so
  the game fills to its own size and plays its empty reload at 0. It goes back on top (for nothing) at the first
  look that isn't asked, that sees a shot, or that finds the tube full. With the tube full it is never taken out:
  the game sees a full gun and starts nothing (also with the bolt open on an empty chamber over a full tube: the
  hand presses the release).
- While asked (or the hidden round still out), a rise of n is n shells: all into the tube, the bolt left as it
  is; but in a reload that **began on an empty gun** (the game's empty reload), the first shell that finds no
  live one chambered goes into the chamber and the bolt home.
  - "Began on an empty gun" is the caller's latch, `CountIn.fromEmpty`: set when the asking begins with nothing
    in the gun, kept until the asking is over and the hidden round back in. It was judged per rise (the count at 0
    just before); a shell pushed up the tube by hand before the game's first then sent that first into the tube,
    the bolt left open, while the game played its empty reload. A rise from a count of 0 still counts as from
    empty by itself (the only shell racked out during the asking).
  - Corrected earlier: the first design said "chamber first if empty and locked". With shells in the tube and the
    bolt open the game plays its tactical reload (its count isn't 0), so those go into the tube; the hand then
    presses the release.
- A shell the hands left in the port goes back to the belly at the game's empty reload (it was never counted).
- The hidden round fired by the mouse and left owed (put back too often), then X pressed: the count is left at 0,
  and the first shell of the game's reload is that round's, paid by the game from the spare ones. No shell is
  made (the pistol's kept reload makes one each time). The count is then written back to 0 once, with the game's
  reload running (section 7 item 17).
- **Shells of the game's that aren't kept** (a rise not asked for: `undone`; shells the tube has no room for, the
  tube's size set below the game's: `refused`) stay in its count while it still rises: nothing at all is written
  (`waiting`; the hands' moves wait with it, and `Belly` stays on the safe side meanwhile). When the count has
  not risen for `kRisingMs` (`CountIn.sinceRise`, counted by the caller from `CountOut.rose`) they go back to
  the spare ones in one write (`back`). Before this each shell was undone or refused as it landed; a game that
  looks at its count each loop would then never finish (the reviewer's stand-in looped for ever; the old test's
  stand-in only reloaded when told, so it could not show it).
- `Asked(sinceX, sinceShell)`: X asks while held and 3 s after (`kAskedMs`: the empty reload's start is 1.93 s
  before its first shell); each kept shell keeps it asked 1.5 s more (`kRisingMs`: a loop is 0.50 s, the end
  0.80 s). Both from the catalog's waves, to be confirmed in the game (section 7 item 12). A shot forgets both.

### 1.4 The test

`tools\shotgunammotest.cpp`: a stand-in for the game's count and the mod's look each frame. The stand-in fires
while its count is above 0; its own reload, once begun, loads a shell a loop **for as long as its count is below
its size, looking at the count each loop**, begins by itself at 0, and is ended by a shot. Every loop of it in
T8, T9 and T15 is bounded, so a reload the mod keeps alive fails the test.

- T1 to T12 check every rule above by hand; T15 the review's cases (X tapped and the gun fired dry, a swap, a
  shell pushed up by hand before the game's first).
- T13 is 6000 random sessions run as `ammo.cpp` is to run it (a look, the hands' move, a look, the game's frame,
  a look; 125 ms a frame): X pressed and held, the asking by `Asked` on real times and forgotten at a shot, the
  game's reload begun by X, by a key nobody pressed, or by itself at 0. Checked after every frame: the counts
  agree (but for the game's own shells waiting to go back), no shell is made or lost, nothing is ever adopted,
  **the game's count is never at 0 with nobody asking** (X pressed and no shot since: the test keeps that apart
  from what it tells the mod), no shell is kept of a reload the game began by itself unasked, every reload of the
  game's ends and its shells are given back 1.5 s after.
- T14 is 6000 more run rough (moves with no look before, writes held off, the game's reload racing the hands):
  still no shell made or lost, except a shot the game fires from a shell the bolt had already thrown out, before
  its count was told, with nothing left to pay for it (3 in 1,200,000 frames; it can't happen with a look after
  each rack).
- Deliberate breaks of the header (27 tried, among them the seven the review found surviving: `Belly` without
  its `lost` term, the wrapper's owed term, `Adopt` leaving the hammer down, `hiddenFired` without `raw == 0`,
  `undone` set with `hiddenPaid` or with `refused`, `settled` always true) each fail at least one check, but
  for one that changes nothing the rules can reach (`Fire` cocking the hammer: it is never down on a live shell).

## 2. Hand interactions: new pure module `src\shellload.h/.cpp` (not built)

Frame, Result and Update as in `m320.h`. `hand[2]` and `gunHand` are as there: the hand at work is whichever does
not hold the gun. Test: `tools\shellloadtest.cpp`.

**Taking a shell**
- A fresh squeeze (grip or trigger) of the working hand inside the belly egg (`ammozone::Inside` from
  `pouch::BodyFrame`, measured as m320.cpp does) gives one shell.
- Conditions: M1014 in hand, hand not busy, and `shotgunammo::CanTake` says yes (a shell left at the belly beyond
  those in the hands and the port; room in the tube, or the bolt open with the port free).
- Nothing is counted until the shell is in the gun.
- Carried by its base (thumb under, index and middle on top, as `m320carry.h`), blending to a thumb-on-base push
  at the port.
- Let go at the belly: put back. Let go elsewhere: it falls (section 5) and costs nothing. Gun leaves the hands:
  it falls, never cancelled.
- Tube full and no room in the port: nothing taken, note "the tube is full".

**Loading port (under the receiver, on the centre line, so mirrored for either hand)**
- Enter: the shell's nose within ~3 cm of the port's line, from 3 cm outside the mouth to 2 cm inside, turned at
  most 45 degrees from nose-forward, and moving in. These are the M320's widened windows; no magnet.
- On the line it moves only as the hand pushes: up past the lifter, then forward along the tube.
- Lifter drawn pushed up if it is its own bone; otherwise not shown.
- Counted at the latch, about one shell length in (~6.5 cm): click, `LoadTube`, the shell hidden, the hand eases
  off.
- The last ~1.5 cm finishes by itself at 0.5 m/s ("a finish, not a magnet").
- Pulled back before the latch: it comes out into the hand once its nose is clear, and re-arms only after 6 cm
  away.
- One shell per push. Tube full at the mouth: it stops at the lifter with a knock and stays in the hand.

**Ejection port (the gun's right side, never mirrored), bolt held open only**
- The shell's middle within ~3.5 cm of the port's centre, within 60 degrees of the bore, nose forward: it drops
  onto the carrier, `PortLoad`, and is drawn lying in the port.
- The hand may come over the top or from the right. The right hand may do it while the left holds the fore-end.
- `CountOut.portBack` (the game's empty reload, or no spare shell left behind it): the shell in the port is
  hidden again.

**Bolt release button (under the port, right side)**
- `reload::Slapped` with a fingertip touch at the button's place, by any free hand, not for 0.3 s after a port
  drop by that same hand's squeeze.
- With the bolt open: `BoltHome`, the bolt springs home in ~70 ms. Bolt home: nothing.

**Bolt handle (right side, moves with the bolt)**
- Grabbed by a fresh trigger or grip press near the knob, over or under (handmag's two lever holds).
- Drawn back along -z at most its travel. Past ~85% and let go from home: `Rack`.
- From held open: a tug of ~1 cm further and let go: `BoltHome` (`Rack` does that by itself with the bolt open).
- Held back: out of battery, the trigger fires nothing (`OnPull`'s `handleHeld`).
- A lost palm coasts 0.2 s, as the slide.

**Firing**
- Semi-automatic is the game's own (`fltSingleFire`): each pull fires once.
- The last shot leaves the bolt open (`CountOut.lockedNow`). The game still has its hidden round and animates the
  bolt home, so the mod draws the bolt group back at `lockAt` while it's open, as the pistol's slide.
- Firing in the middle of a load is allowed (vanilla allows it; T5 checks the count); a shell on the port's line
  stays with the hand.

## 3. What the game's own reload does

From the catalog (waves; still to be recorded in the game):
- **Tactical:** start 0.57 s, loop 0.50 s per shell with a click ~0.2 s into each loop, end 0.80 s. The left hand
  pushes shells up one at a time.
- **Empty:** start 1.93 s. A shell is dropped into the ejection port at ~0.97 s, the release is pressed and the
  bolt slams home at ~1.11-1.18 s (the loudest sound), then the tube is filled. End 0.71 s.
- The handle is never pulled.
- Draws: body 13377/0 (bone 0 rear sight [inferred], 1 trigger, 2 bolt group, 3 frame), sight dots 324/13377,
  shell 450/13701, depth 14151/14151, extended tube 696/0.
- The train door is a scripted pickup (`PUS_player`): free hands off there, and the state is taken from the count
  afterwards (`FromCount`).

## 4. Sounds, buzz, log

**Sounds** (rule: from the game's files only; new rows in `g_cuts`, `tools\gamesounds.cpp`, shape "m1014", played
by name as the M320's are)

| Clip | Source | When |
| --- | --- | --- |
| `shell_in_m1014` | tactical wave, the loop's click | the latch |
| `port_in_m1014` | empty wave, ~0.97 s | the port drop |
| `bolt_m1014` | empty wave, ~1.11-1.18 s | bolt home, by button, tug or rack |
| `dry_m1014` | `trigger_click_wave` | a `kClick` pull |
| `shell_land_*` | the game's shotgun-shell landing wave | a loose shell landing |
| `lever_m1014` | none in its reload | handle drawn |

- The exact wave names and cut times are not in the docs; find them with gamesounds' listing.
- For the handle: try its deploy wave; otherwise a borrow needs the owner's yes (as the M416's T-handle was the
  G36C's).
- For a full tube's knock: no clip is known; `port_in` quietly, or silence.

**Buzz** (`input::Buzz(side, amplitude, seconds)`; values to tune)

| Event | Working hand | Gun hand |
| --- | --- | --- |
| shell taken | 0.15 / 20 ms | none |
| latch click | 0.5 / 30 ms | 0.2 / 20 ms |
| full knock | 0.3 / 40 ms | none |
| port drop | 0.3 / 25 ms | none |
| bolt home | 0.4 / 50 ms | 0.8 / 50 ms |
| bolt left open by the last shot | none | 0.6 / 40 ms after recoil's |
| dry click | none | 0.15 / 15 ms |

**Log** (prefix "Shotgun:")
- "its rounds by hand from now (slot N, the M1014): T in the tube, chamber ..., tube holds C, S spare; a hidden
  round ..."
- "a shell from the belly (left|right hand)"
- "squeezed at the belly: nothing (<note>)"
- "its nose into the loading port: pushed in by the hand"
- "a shell home in the tube: T of C, S spare"
- "the tube is full: the shell stops at the lifter"
- "pulled back out"
- "a shell dropped into the ejection port"
- "the release pressed: the bolt home, the port's shell chambered | one from the tube chambered | on an empty
  chamber"
- "the handle drawn N mm and let go: racked, a live shell thrown out (lost), ... | not far enough"
- "the last shot: the bolt held open"
- "the trigger didn't fire: <the bolt is held open | the handle is held | nothing in the chamber>"
- "let go of: it falls" and "put back at the belly"
- The count lines as "Pistol:" has them, from `CountOut`'s flags: the hidden round put back, taken out for the
  game's reload and back on top after it; the game's shells kept (how many, the first into the chamber) or
  undone; shells the tube had no room for; a shot on an empty chamber that took the tube's shell; the port's
  shell gone back; adopted.
- With `shelllog 1`, a state line per frame.

## 5. Loose shell

New `Spec` "12g" in `src\loose.cpp`, beside "m320":
- round about z, base at 0, mouth along +z;
- rings at 0 and 0.064 m, radius 0.0105 m (6.4 x 2.1 cm, the catalog's size);
- live ~0.045 kg (confirm against the load); bounce ~1.2; head ~0.5; not a casing.

Handling:
- Started with `QueueLoose(kLooseShell, ...)` -> `loose::ReleaseBy` from the hand, or `loose::Flick` out of the
  port for a racked-out live shell (`Bolt.threw`).
- Lies 30 s. Drawn with the gun's own shell draw (450/13701), so it vanishes while another gun is out; that is the
  known hosting gap.
- Spent hulls: leave the game's own effect in version 1. Later, a `k12g` kind (~0.010 kg) and an "m1014" emitter
  in `src\casings.cpp`.

## 6. Integration points (not built)

- **`src\ammo.cpp` / `ammo.h`**
  - `struct Tube` and `g_tube`, `UpdateTube()` and `StepTube()` beside `UpdatePistol`/`StepPistol`.
  - Found by the slot's row (shape "m1014"), followed by firing, weapon and blueprint.
  - `StepTube` keeps `hidden` and `lost` as `CountOut` gives them (once written), and three times: X let go
    and the last kept shell (for `shotgunammo::Asked`), and the last rise of the game's count (`CountOut.rose`,
    for `CountIn.sinceRise`).
  - **`asked` is X only**: `Asked(sinceX, sinceShell)` and nothing else. No swap of the game's (the pistol's
    `asked` has one; here it would let the game reload an empty gun by itself).
  - On `CountOut.shot` it forgets the X and shell times, and X still held asks again only by a fresh press.
  - It latches "the asking began on an empty gun" (`Total == 0` at the first asked look) for `CountIn.fromEmpty`,
    and drops it when the asking is over and the hidden round back in.
  - Met: `FromCount(raw)` with the hidden round out, and the first look puts it on top; a count above the tube and
    one (a checkpoint with the hidden round in it): hidden already in, as the pistol's.
  - `Firing()` must exclude `g_tube.firing` as it does the pistol's.
  - `Swapped()` forgets it, as it does the pistol (the gun, its times and its latch: it asks for nothing).
  - Capacity: `row->magazine - 1`, +2 when the extended tube is seen; cross-check with `Gun::fullest`.
  - API: `SetTubeByHand`, `TubeLive/Dry/Locked/Count/Spare/Belly`, `TubeLoadShell`, `TubePortLoad`,
    `TubeBoltHome`, `TubeRack`, `TubePull`: each a look, the move, a look, as the pistol's.
  - `HeldPress()` gets a tube branch for the incremental reload. The hidden round has to be out of the game's
    count before the game reads its reload key (X held is known a frame before R goes to the game, or R is held
    back a frame): otherwise a gun one short of full looks full to the game and it refuses.
- **`src\hands.cpp` / `hands.h`**
  - `TubeByHand(...)` beside `M320ByHand` (~2404): builds the frame, turns events into ammo calls, sounds, buzz,
    log and `QueueLoose`.
  - `hands::TubeDry()`, `TubeDryPull()`, `TubeBoltHeld()`.
  - Busy gates as the M320's (`g_freshBy`, the support hold, the collision glove).
  - `SetTubeByHand` each frame in `Update` (~5356, beside `SetPistolByHand`).
  - A scripted hand "shelltest N" for films with nobody in the headset.
- **`src\input.cpp`**
  - `FireButton` (~1584): a branch like `PistolDry`.
  - `FireBlocked` (~1538): its reasons.
- **`src\skin.cpp` / `skin.h`**
  - The `kHeldKinds` row, with `mag` null so handmag stays out.
  - A new `HeldTube` table: bolt bone, shell draw key, extended-tube key, the places from section 7.
  - `HandTargets`: `heldBoltBack` (or reuse `heldLeverBack`), the shell in the hand's pose, the port shell.
  - `skin::HeldTubeGun()`.
- **Other source files**
  - `src\loose.cpp`: the Spec.
  - `src\recoilguns.h`: shape "m1014".
  - `src\gunprofile.h`: `kNames`.
  - New `src\shellposes.h`: the fitted holds.
- **Tools and build**
  - `tools\gamesounds.cpp`: the cuts.
  - `CMakeLists.txt` and `tools\selftest.ps1`: `shotgunammotest` is in the offline list; `shellloadtest` to add.
- **Switch**
  - `build\shotgun.txt` "reload on/off" (or `weapon.txt` "tubebyhand 0"). Off: the game's own reload, hidden round
    out, as the pistol's give-back.
  - No other gun's path changes.
- **Open problem: a checkpoint can make a shell** (inherited from the pistol's plan).
  - The game saves its count with the hidden round in it. When the checkpoint is loaded the mod meets the gun
    anew: `FromCount(raw)` takes the hidden round for a real shell and the first look puts another on top. One
    shell made per load. Only a count above the tube and one gives it away ("hidden already in"); a restored
    count of 1 to capacity + 1 can't be told from a gun that really holds that many, without a mark that
    survives the checkpoint.
  - It also runs the other way: a checkpoint saved before the mod first met the gun has no hidden round in it,
    so "always take one off" loses a shell there.
  - Options, none chosen:
    1. Take the hidden round out before the game saves, back in after (needs the save's moment: a hook on the
       checkpoint, or its "saving" state if the game shows one). Exact, and the saves stay clean for a player
       who later removes the mod.
    2. A mark of the mod's own beside the save: the count it wrote and the spare ones, per slot, in a small file
       keyed by level and checkpoint; a restored pair that matches the mark has the hidden round in. Survives a
       restart of the game; wrong only if two checkpoints share a key.
    3. A mark in the process only: "the hidden round was in when this level's last checkpoint passed". Simple,
       but lost when the game is closed, so the first load of a session can still make (or lose) one.
    4. Carry the hidden round only when it is needed: the game's count the true one while the gun holds 2 or
       more, the hidden round on top only at 1 or 0 left. Most checkpoints then hold a true count; the doubt is
       left to counts of 1 and 2. To be weighed against a write of the count right after the shot that leaves
       one (does the game take it in time?).
    5. Err on the side of losing: a restored count is always taken as hidden round in when the mod has run
       before in this save. At worst one shell fewer after a load, never one more.
  - The pistol has the same problem; whichever is chosen should be built once for both.

## 7. To measure in the game later

Places are on the frame bone (3): x left, y up, z to the muzzle.

1. For the `kHeldKinds` row: confirm bone 3 is the frame, the sights' line and up, the muzzle, both palms at rest,
   and the depth pass's bone order.
2. The loading port: the mouth's centre, the line's direction, the lifter's travel (and whether it has a bone), and
   the latch depth.
3. The tube's mouth, and its end for the 5- and 7-shell tubes; whether 696/0 is on its own bone.
4. The ejection port: centre and size, and the shell's place lying on the carrier.
5. The release button's place.
6. The bolt group (bone 2): travel from a fire clip, the held-open position, the handle's knob, and whether the
   handle is in bone 2.
7. Where the game parks its shell draw at rest, and its bone, for hiding and re-posing.
8. Hand tables from recordings of both reloads: the pinch at the loading port at the loop's click, the hand at the
   port drop, the finger on the release.
9. Fitted by tool, not from the game: the carry (`tools\basegrip.cpp`) and the handle's over and under holds
   (`tools\magfit.cpp`).
10. Whether the game shows the bolt held back at count 0 (hidden round off, `weapon.txt` "empty").
11. Wave names and cut times, including the deploy wave and the shell-landing wave.
12. The count during the game's reload: one rise per loop, where it stops, its firing-state word while reloading,
    how long after the key the first shell lands (for `kAskedMs`, `kRisingMs`), whether its empty reload starts
    from a count of 0 only, and whether it starts on the key's press or while the key is held.
13. Whether writing a count above `MagazineCapacity` holds on this gun (it does on the M9): one above with a tube
    of `magazine - 1`, two above with the owner's 5/7.
14. The pickup's first count (5 or 7) and spare (32 on the train, 16 in the vault).
15. Whether the vault and gas-sluice pickups are bare.
16. Belly reach against the gun's place when seated: the egg must stay short of the loading port.
17. **Does the game's reload loop again when a shell is written back?** With its reload running, write its count
    one lower (and one higher) between two loops: does it load one more loop for it, stop where it meant to, or
    take the written count as it is? And does a shot end it? The core writes nothing while the count rises for
    this reason; two writes still land in a running reload: the hidden round taken out as X is pressed (before
    the first shell) and put back when the tube is full, and the count written back to 0 after the first shell
    has paid for an owed hidden round. If the game counts its loops at the start instead, the wait before
    shells are given back can go and `kRisingMs` only has to cover one loop.

## 8. How the 870's pump fits later

- **`shotgunammo.h`:** the same state, tube, port, belly and count. Its own operations beside the M1014's: a shot
  that leaves `Chamber::kSpent` and feeds nothing; `PumpBack()` throws out whatever is chambered (a spent hull, or
  a live shell as a loose one); `PumpForward()` feeds from the port's shell, else the tube. The bolt is never
  "held open"; the port load is allowed while the fore-end is back instead. `OnPull` gains `kNeedsPump`. `See`
  takes which gun it is for the shot's rule. `kSpent` is in the enum today and tested (T12).
- **`shellload`:** shared as it is. The loading port takes its own numbers.
- **New `pump.h`:** the support hand's fore-end hold becomes a slider on bone 2 (16 cm long, 0.32-0.48 m along the
  gun). Only full strokes count, and it still steers.
- **The game's own pump:** the 0.48 s bolt action after each shot is hidden or held; the trigger is stopped until
  the hand's stroke.
- **Sounds:** its own cuts (pump back 0.25-0.33 s, forward 0.50-0.57 s of its 0.87 s wave).
- Nothing of the M1014's release button or handle is reused for it.

## 9. What is left

1. The owner's decisions: one shell per reach, the borrowed handle sound if the gun has none, and how a
   checkpoint is kept from making a shell (section 6's open problem). The tube's size is settled (4/6).
2. The measurements of section 7, in the game.
3. Onboarding the M1014 as a held gun (section 0).
4. `shellload.h/.cpp` and its test (section 2), the fitted holds (`shellposes.h`).
5. The integration of section 6: `ammo.cpp`, `hands.cpp`, `input.cpp`, `skin.cpp`, the switch.
6. The loose shell (section 5), the sounds and buzz (section 4).
7. Films of both loads, then the headset test.

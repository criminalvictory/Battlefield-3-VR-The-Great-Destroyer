# Battlefield 3 VR: The Great Destroyer

A fan-made VR mod for the **Battlefield 3 campaign** on PC: the game in true 3D in your headset, your head moving the view, and your own hands holding, aiming and reloading the guns.
You need your own copy of Battlefield 3; the mod adds only its own code.

**This is a pre-alpha test build.** God mode is on by default, on purpose, and it isn't ready for a normal play-through yet: [read what that means](#pre-alpha-please-read) before you play.

**[Download it from the Releases page](https://github.com/criminalvictory/Battlefield-3-VR-The-Great-Destroyer/releases)**, then follow [Download and play](#download-and-play): unzip, double-click, play.

**Watch it:** [4-minute gameplay clip of Rock and a Hard Place, on X](https://x.com/PCVRGamer/status/2117429358998478896)

![The M416's muzzle flash at the end of the barrel, the picture for each eye side by side](pictures/rifle_flash.png)

**On this page:** [Pre-alpha](#pre-alpha-please-read) · [What works](#what-works-today) · [Download and play](#download-and-play) · [Controls](#controls) · [Mission by mission](#what-to-expect-mission-by-mission) · [Troubleshooting](#troubleshooting) · [Campaign only](#campaign-only) · [Credits](#contributors-and-credits) · [For developers](#for-developers)

## Pre-alpha: please read

This is a **test build**: an early version for people who want to try it and say what goes wrong. It isn't a finished way to play the campaign yet.

- **God mode is on by default, on purpose.** Enemy fire can't kill you, so you can try the guns, the reloads and the levels without replaying the same fight again and again. It's a choice for testing, not a cheat left in by mistake. **End** on the keyboard switches it off (and on again), and the choice is remembered (`[god] enabled` in `build\bf3vr.ini`). The game's own ways of failing a mission still happen, such as leaving the mission area or a scripted death.
- **Not for a normal play-through yet.** One mission, Rock and a Hard Place, plays from start to end in VR. The first two (Semper Fidelis, Operation Swordbreaker) have their guns in your hands and are being played now; most others not at all yet: see [mission by mission](#what-to-expect-mission-by-mission). Some guns are still held by the game's own hands.
- **Expect rough edges:** things that look or feel wrong, the odd crash, and settings that may need a change on your PC.
- **The game's graphics settings are set for VR at every start:** the ones the mod was tuned with (high quality, anti-aliasing on, motion blur off, in a window), listed in `build\gamesettings.txt`. Your monitor's resolution and refresh rate stay yours, and your old settings file is kept once as `PROF_SAVE_profile.before-bf3vr` next to it. To keep a setting of your own, change it in `gamesettings.txt`.
- **Holsters are by hand:** a gun goes onto a shoulder only when your hand puts it there: see [Controls](#controls). If they get in your way, `holsters 0` in `build\holsters.txt` switches them off. The weapon buttons switch guns either way.
- **Made on one PC:** Steam's copy of the game, a Steam Frame headset, an RTX 3080 graphics card, played seated. Players have since run it on Quest headsets (Steam Link, Virtual Desktop) and on AMD graphics cards; the Index's buttons are set up, but nobody has reported on them yet. Reports from every setup are very welcome: see [reporting a problem](#reporting-a-problem).

## What works today

- **True 3D:** the game draws a separate picture for each eye, 106 degrees wide.
- **Your head moves the view:** look around, lean, duck and peek. Cutscenes and rides stay in 3D: your body stays in the scene, your head stays free.
- **Your hands are your hands:** the gun is in the hand that holds it, pointing where you point it, and the other hand is free. Either hand can hold a gun, and both hands can hold a rifle. Let go of the grip and the gun drops; lower a hand towards a gun on the ground and squeeze, and it flies into that hand, an enemy's gun too.
- **Holsters on your body:** the guns you start a mission (or a checkpoint) with are on your body: your main gun on your right shoulder, the rocket launcher on your left (behind your neck when a second gun is on that shoulder). A gun goes onto a shoulder only when your hand puts it there: open your hand on your right shoulder and it's your main gun, on your left and it's your second one, whichever gun it is (a launcher always goes to the launcher's place). A gun you pick up off the ground stays in your hand until you put it on a shoulder or let go of it; a gun on a shoulder stays there until your hand takes it out, and a gun you drop is on no shoulder any more. Reach a shoulder with either hand and squeeze the grip to draw; the controller gives a light tick when your hand is where it can draw or put away. The game carries two guns besides the launcher and swaps a gun you take for one of them: if that one was on your shoulder, it stays on your shoulder while the game keeps it in the place where the new gun lay (you'll see it lying there). Reach that shoulder and squeeze within a couple of metres of that place (the game hands a gun over only within 2.5 m and in sight), and it comes back into your hand with its own rounds, the gun the game swapped for it going down there instead: hold your rifle on your shoulder, pick up a rocket launcher, fire it, throw it away, and draw your rifle again. Further away, a double buzz says to go back nearer (or take it off the floor there: by hand or with X held, it comes into your hand, still that shoulder's gun). At the launcher's place, the launcher you carry is drawn when the one in a pickup is out of reach. A gun the game takes off your shoulder with no pickup (Operation Swordbreaker's rooftop rocket launcher) stays on that shoulder until the game gives it back, after a death too. If the game swaps the gun in your hand for one you pick up, the gun from your hand goes down where the new one lay, on no shoulder. Put another gun on that shoulder meanwhile and it's that gun's shoulder now: the one in the pickup stays there on no shoulder (the log says so), so a shoulder is never blocked. The weapon buttons can't bring back a gun that's away in a pickup: they give the gun the game holds in that slot now (the launcher you picked up, say). One gun a shoulder: open your hand on a shoulder another gun you carry has and the gun stays in your hand (a double buzz), it never drops; the gun you put on a shoulder last keeps it after a death too. A gun you picked up and threw away goes back into the game's hands when you draw another; your own gun stays where you drop it. Only a dropped gun on the floor that would destroy the gun on your shoulder, take every gun you carry, hold more guns than the mod reads, or take the grenade launcher alone off the rifle on your shoulder isn't taken (a double buzz; the log names the gun): take that one out and drop it first. The level's own pickups are always taken (Rock and a Hard Place's Stinger for the jet): if one replaces a gun on your shoulder, that gun is gone, as the game does it (the log says so). The weapon buttons still work: a gun that came from a shoulder goes back there, and one that never was on a shoulder stays in the game's hands until a button brings it back.
- **The guns:** every gun of Rock and a Hard Place (the M416 with its M320 grenade launcher, the M16A4, M4A1, AN-94, AEK-971, AK-74M, RPK-74M, G3A3, KH2002 and M240, the SMAW and the Stinger); Operation Swordbreaker's M4A1 with its holographic sight, Mk11 with its scope, M249 and AT4; Semper Fidelis's pistol, M1014 shotgun, AKS-74u and UMP-45.
- **Reloading by hand on all of them, and no automatic reloads:** take a fresh magazine from your belly and push it into the gun. Guns with a magazine button drop the old one when you press it; guns without one (the AK family, the G3) have it pulled out by hand, or knocked out by the new one. Then work the lever or the charging handle, or slap the bolt release on the M4s and the M16, whose port cover opens and shuts, and on the Mk11. The M1014 shotgun takes its shells one by one, pushed up its loading port.
- **The machine guns' whole reload** (the M240, the M249): lift the lid, swap the ammo box, lay the belt in the feed, close the lid, pull the lever. The M240's belt gets shorter as you fire.
- **The launchers too:** pull the spent case off the SMAW and load a new rocket; open the M320 by pulling its trigger once it's empty, put in a 40 mm round and slap it shut; lift the Stinger's aerial after each shot for the next missile. The AT4 fires once: let go of it to throw it away.
- **Recoil and vibration:** each gun kicks with its own recoil (the rocket launchers not yet), and the controllers buzz with every shot. Fired with one hand, a rifle jumps all over the place, on purpose: use two hands.
- **Scopes that really magnify:** the zoomed view sits inside the scope's own glass, with its reticle and range marks, while the rest of your view stays normal. The Mk11's scope has the game's own mil-dot reticle. The Kobra red dot and the M4A1's holographic sight show their reticles. A magnified scope's own picture is a second view of the world (about 2.6 ms of the graphics card a frame while you look through it), and it stays on unless you ask otherwise: with `scopegov 1` on a line of its own in `build\weapon.txt`, three seconds under 60 frames a second with a scope's own picture up make the scope magnify your eye's own picture instead (softer) for 10 seconds or more, back at the first raise after. Without that line the log only says when it would have done so.
- **Effects from the gun you hold:** the muzzle flash and smoke come from its barrel, and the casings fly out of its ejection port.
- **Moving as the soldier:** walk with the stick the way you look; crouch, go prone and stand up with the right stick, or crouch for real.
- **Quick-time events and scripted prompts by hand:** when the game shows a key to press in a scripted fight, jump or climb, squeeze either grip. At a door, a wire or a gun the game hands you (Operation Swordbreaker's AT4, with both hands empty), reach for it and hold the grip.
- **The game's own sounds** for the reloads by hand, made from your own copy of the game the first time you start.
- **A flat view for streaming:** the monitor, OBS and Discord show a normal picture, not two eyes side by side.

![An M16 with its carrying handle held in VR, seen from its right side, a gloved hand on the grip and the other under the handguard](pictures/m16_top_handle.png)

**Not there yet:**

- Most missions haven't been played in VR yet. Where you play someone else (Dima in Comrades and Kaffarov, the tank and the jet), the arms and guns are still the game's own.
- Guns the mod doesn't know yet (the other shotguns, the SVD, SKS, AS Val, PKP, Javelin, RPG-7 and others) stay in the game's own hands: they may reload by themselves and have no recoil of the mod's.
- Hand grenades aren't on the controllers yet (G on the keyboard throws one the game's way). No knife and no ladders by hand.
- The HUD isn't placed for VR yet, and some black can show at the far edges of the view when you turn your head fast.
- The newest guns, the quick-time events on the grips and the prompts by hand were built from recordings of the game and haven't all been played through in the headset yet.

![Six frames of a pistol shot, the fireball coming from the muzzle](pictures/pistol_fireball.png)

## Download and play

### What you need

- **Battlefield 3 for PC**, from Steam or the EA app, installed and **started once the normal way** first (so it makes its settings and links your EA account). Only the single-player campaign is used.
- **Steam with SteamVR** (SteamVR is free in Steam's Store).
- **A headset that works with SteamVR**, with thumbstick controllers: Meta Quest 3 or Quest 2 (through Steam Link, Virtual Desktop, or Quest Link / Air Link), Valve Index, Steam Frame, and others that SteamVR runs.
- **A strong graphics card:** it's made on an RTX 3080 at 72 frames a second. A slower card can make the picture smaller: see [low frame rate](#troubleshooting).
- **Windows 10 or 11.** Nothing else to install: no Visual C++ runtime, nothing put in the game's folder.

### Install (once)

1. **Download** the zip (`BF3-VR-0.1.0-prealpha.zip`, or whatever the newest one is called) from the [Releases page](https://github.com/criminalvictory/Battlefield-3-VR-The-Great-Destroyer/releases).
2. **Unblock it:** right-click the zip, **Properties**, tick **Unblock** at the bottom, **OK**. Windows marks downloaded files; unblocking the zip before unzipping keeps it from stopping the mod's files one by one. (No Unblock box: nothing to do.)
3. **Unzip it:** right-click the zip, **Extract All...**, and pick a folder of your own, for example `C:\Games` (outside Documents and the desktop is best: Windows' Controlled folder access, when it's on, keeps programs from writing there, and the mod writes its logs and sounds into its own folder). Extract All makes a folder with another of the same name inside it: `Start BF3 VR.bat` is in the inner one. Keep the folder together, and don't start anything from inside the zip itself.
4. **If your antivirus complains:** the mod's `build\injector.exe` puts the mod (`build\bf3vr.dll`) into the running game. That's how many game mods work, but it's also something harmful programs do, and these files are new and unsigned, so some antivirus programs (Windows Security among them) may block them or move them to quarantine. Since November 2026 Windows Security does this to `injector.exe` the moment the zip is unzipped, calling it "Trojan:Script/Wacatac.C!ml": the "!ml" means a guess by its machine learning, and it's a false alarm. Everything in the zip is built from the code on this page. If the launcher says a file is missing or your antivirus removed it, let the mod's folder be (the launcher opens Windows Security's settings for you): **Windows Security, Virus & threat protection, Manage settings, Exclusions, Add or remove exclusions, Add an exclusion, Folder**, and pick the folder you unzipped the mod into. Then unzip the package into it again. Allow only that folder; never switch your antivirus off. (Avoid "Allow on device" in Protection history: it allows that whole detection name on your PC, not just this file. Another antivirus has an exceptions list of its own.) (Windows 11's Smart App Control is another matter: see [Troubleshooting](#troubleshooting).)

### Connect the headset

Every time, before you start: turn the headset on and connect it to SteamVR.

- **Meta Quest 3 or Quest 2**, one of these:
  - **Steam Link** (free, from the Meta Store on the headset): open it in the headset and pick your PC.
  - **Virtual Desktop**: connect to your PC; SteamVR streams through it (the launcher starts SteamVR if it isn't running).
    Or try **VDXR**, Virtual Desktop's own OpenXR runtime, without SteamVR (experimental, see below).
  - **Quest Link** (cable) or **Air Link** (Wi-Fi), through the Meta Quest Link app on the PC.

  With Quest Link or Air Link, Meta's OpenXR runtime is usually the active one on the PC. The mod asks for SteamVR's by itself, but to be sure: in SteamVR's **Settings**, **OpenXR** (in older versions under **Developer**), click **Set SteamVR as OpenXR Runtime**. (To give it back to Meta later for other games: the Meta Quest Link app, Settings, General, OpenXR Runtime.)
- **Valve Index**, **Steam Frame** and other SteamVR headsets: as you always do. With the Steam Frame's USB adapter plugged in, the launcher waits for its link and makes sure SteamVR streams over it.

**Virtual Desktop's VDXR (experimental, not tested by us yet):** VDXR is Virtual Desktop's own OpenXR runtime; it comes with the
Virtual Desktop Streamer and runs the game in the headset without SteamVR. To try it: in the Virtual Desktop Streamer on the PC,
set the OpenXR runtime to **VDXR** (or Automatic), connect Virtual Desktop on the headset to the PC, and start with
**`Start BF3 VR (VDXR).bat`** instead of `Start BF3 VR.bat`. That one doesn't start SteamVR (and closes it if it's running), and the
mod points the game at VDXR. `build\launch.log` says "VDXR: the game runs on Virtual Desktop's own OpenXR runtime" when it took.
If anything's wrong with it, send `build\launch.log` and `build\bf3vr.log`, and play with `Start BF3 VR.bat` (SteamVR) meanwhile. (The first player to try it, on a Quest with Virtual Desktop, had the game crash inside Virtual Desktop's own runtime about a minute into a level, while `Start BF3 VR.bat` worked on the same PC. If it crashes for you, double-click `Make a report.bat` right away, before starting again, so the crash's log is in the report.)

Set the headset to **72 Hz**, the rate the mod is made and tested at (in Steam Link's, Virtual Desktop's or the Meta Quest Link app's settings, or SteamVR's). The Index can't do 72: use 80 Hz, its lowest.

### Play

1. Double-click **`Start BF3 VR.bat`** in the unzipped folder. If Windows asks first ("Open File - Security Warning": **Run**; or "Windows protected your PC": **More info**, **Run anyway**), let it: that's the zip not being unblocked.
2. A window opens and says what it's doing. **Leave it open.** It checks your PC, sets the game to windowed if it was full screen (the mod needs a window; your old settings file is kept), makes the mod's sounds from your copy of the game the first time (about a minute, once), starts SteamVR, and starts Battlefield 3: Steam's copy through Steam, which opens Battlelog in your browser; the EA app's copy by opening Battlelog in your browser.
3. **In Battlelog, click PLAY CAMPAIGN.** Sign in with your EA account if it asks (the launcher never touches your account). If your browser asks whether to open the EA app, allow it. If PLAY CAMPAIGN opens the campaign page instead, click its **LAUNCH CAMPAIGN** button.
4. When the window says **"The mod is in"**, the game is starting with the mod.
   - **The first time:** in the game's menu click **START NEW CAMPAIGN** and pick a difficulty (or **RESUME CAMPAIGN** if you've played the campaign before).
   - **From then on** the window clicks RESUME CAMPAIGN for you.
5. **Put the headset on as the level starts:** the game switches to VR by itself (the menus before it are a flat screen). Sit or stand the way you'll play, look straight ahead and hold the right stick's click for 1.5 s to recenter (F9 on the keyboard does the same).

**The next time:** connect the headset, double-click `Start BF3 VR.bat`, click PLAY CAMPAIGN, put the headset on.

- **Another mission:** press Escape in the game, EXIT TO MENU, then REPLAY MISSIONS (it lists the missions you've finished). If the mouse misses in the menus while the VR view is on, use the arrow keys and Enter, or press F8 for the flat screen (and F8 again for VR).
- **A newer release:** unzip it into a new folder and start from there.
- **Uninstall:** delete the folder. The only change it makes outside it: a game set to full screen is set to windowed, its old settings file kept as `Documents\Battlefield 3\settings\PROF_SAVE_profile.before-bf3vr`. To undo that, put that file back in place of `PROF_SAVE_profile`, or set full screen again in the game's options.

## Controls

The controllers work only while the game's window is in front on your PC: if you clicked anything else, click the game's window once.

### The same on every controller

| | |
|---|---|
| Right trigger | Fire the gun in your right hand |
| Left trigger | Fire the gun in your left hand. With the hand on a charging handle, lever or pistol slide: grab it (pull back, let go). On the M320's grip: fire a grenade |
| Grip | Hold a gun. Keep squeezing: let go and it drops (unless the other hand holds it at the front) |
| The other grip, at the gun's front | Hold it with both hands: it then points from your back hand towards your front hand |
| Grip at your belly | A fresh magazine for the gun in your hands (or a rocket, or a 40 mm round) |
| Grip near a gun on the ground, or with your hand lowered towards it | The gun flies into that hand |
| Either grip, when the game shows a key to press (a quick-time event) | Presses the key the game is waiting for, whichever it is. In a scripted fight a grip you're already holding counts. In ordinary play, let go of either grip and squeeze it again, the hand holding your gun too: your gun isn't dropped while the game shows the key. A hand holding a magazine, a round or the charging handle presses nothing there. Keep squeezing where the game wants it pressed again and again (a climb) |
| An empty hand at a scripted prompt (a door, a wire, the AT4 on the roof), grip held | Reach for it (a light tick says your hand is there), then squeeze and hold the grip a moment: the game's own Use. At a door or a wire a gun in the other hand is fine. The AT4 needs both hands empty: drop your rifle or put it on a shoulder first (with a gun in a hand nothing happens, only a double buzz if your controllers vibrate) |
| Grip at a shoulder (or behind your neck) | Draw the gun kept there: your main gun on the right, a second gun or the launcher on the left. Either hand, either shoulder |
| Open your hand at a shoulder | Put the gun in it away on that shoulder (the right makes it your main gun, the left your second one; a launcher goes to the launcher's place). If another gun you carry has that shoulder, the gun stays in your hand |
| Left stick | Walk, the way your head faces; click: sprint |
| Right stick | Turn; flick down: crouch; flick down and hold: prone; flick up: stand up |
| Right stick's click | Short: jump; held 1.5 s: recenter the view |

There's no aim button: bring the gun up to your eye and look through its sights.

### The buttons

| | Quest 3 / Quest 2 | Valve Index | Steam Frame |
|---|---|---|---|
| **Magazine:** tap to drop it (guns with a magazine button); hold for the game's own reload, or when the game says **HOLD [R]** to pick something up | **X** (left) | **A** (left) | **X** (right); with the gun in your left hand also **D-pad right** |
| **Use:** the game's **E** (doors, prompts) | **Y** (left) | **B** (left) | **Y** (right) |
| **Switch weapon** (rifle, launcher) | hold **Y** 0.7 s | hold left **B** 0.7 s; or press a trackpad hard: left the launcher, right the rifle | hold **Y** 0.7 s; or **D-pad** up the launcher, down the rifle |
| **Jump:** the game's **Space** | **A** (right) | **A** (right) | **A** (right) |
| Free | **B** (right) | **B** (right) | **B** |

On the Quest, switch weapons by holding **Y**: the left menu button (≡) belongs to SteamVR's own menu.

**In scripted fights, jumps and climbs** (quick-time events) the game shows a key to press: squeeze either grip, and the mod presses the key the game is waiting for (E, F, Space, Z or a mouse button). In a fight a grip you're already holding answers each prompt the moment it appears (once a prompt), and a fresh squeeze works too; where the game wants a key hammered, keep the grip squeezed. In ordinary play (no cutscene), squeeze either grip afresh: let go and squeeze again, the hand holding your gun too. The gun isn't dropped while the game shows the key, and that squeeze takes nothing else (no magazine, no holster, no handguard). A hand holding a magazine, a round or the charging handle presses nothing there. If a prompt doesn't take the grip (the mod presses nothing when it can't read what the game waits for), the buttons still work: **Space** is A, **E** is Y (Index: left B), **HOLD [R]** is holding X (Index: left A); for any other key, press it on the keyboard. `qte 0` on a line of its own in `build\weapon.txt` leaves quick-time events to the buttons and keys alone.

**At a scripted prompt** (open a door, cut a wire, pick up the AT4 on Operation Swordbreaker's roof): reach your empty hand to where the prompt is and hold the grip. The button the game shows works too (Y for **E**, holding X for **HOLD [R]**; Index: left B and left A). Doors and wires take a free hand with the rifle in the other. **The AT4 needs both hands empty:** drop your rifle or put it on a shoulder first, then reach for the AT4 and hold the grip, or hold X (Index: left A). With a gun still in a hand neither the grip nor X takes it (a double buzz, if your controllers vibrate).

### The keyboard (with the game's window in front)

| Key | |
|---|---|
| F8 | The VR view, or a flat screen in the headset |
| F9 | Recenter |
| End | God mode off / on (remembered) |
| Home | What the monitor shows: one eye, or both side by side (remembered) |
| Insert | The mod's hands off / on (off: the game's own arms) |

The full list is under [Keys and settings](#keys-and-settings).

### By hand

- **Two hands:** with the gun in one hand, squeeze the other grip at its handguard. Let go of that grip to free the hand.
- **Drop and take back:** open your hand and the gun falls (or flies, if your hand was moving). To get it back, lower your hand towards it and squeeze.
- **Reload:** drop the old magazine (the magazine button, on guns that have one) or pull it out by hand (the AK family, the G3; pushing a new one against it knocks it out too). Squeeze at your belly for a full one, bring it under the magazine well top first and push it home. If the gun ran empty: put your hand on the charging handle or lever, hold that hand's trigger, pull back and let go; on the M4s and the M16 you can instead slap the bolt release, on the left behind the magazine well, with an open hand.
- **Pistol:** the other hand's trigger on the back of the slide grabs it: pull back, let go. The magazine button drops the magazine; a new one comes from your belly.
- **SMAW:** grab the spent case at the back, twist or pull it off, take a rocket from your belly and push it in until it locks.
- **M320 (under the M416):** squeeze at its own grip, under the barrel in front of the magazine: that hand's trigger fires a grenade, and the other trigger still fires the rifle. When it's empty, pull its trigger to open it, the spent case falls out, take a round from your belly, push it in and slap the barrel shut.
- **Stinger:** hold it on the jet until you hear the lock tone, then fire. Lift the aerial after each shot for the next missile.
- **M240 and M249:** lift the lid, swap the box (from your belly; the M249's comes off to the left, then down), pinch the belt with your thumb and finger and lay it in the feed, close the lid, pull the lever or handle.
- **M1014 (the shotgun):** squeeze at your belly for a shell, bring it under the gun and push it up the loading port, one shell at a time.
- **Mk11:** the magazine button drops the magazine; after a fresh one, slap the bolt catch or pull the T handle at the back.
- **AT4:** both hands empty first (drop the rifle or put it on a shoulder), then reach for it where it lies and hold the grip (or hold the magazine button at **HOLD [R]**). One shot; then open your hand to throw it away. Your rifle is on the shoulder you put it on, or where you dropped it.
- **Holsters:** open your hand at a shoulder to put a gun there, squeeze there to draw. A gun off the ground is never put on a shoulder for you.
- **Holding the magazine button** is always the game's own reload, if you'd rather.

## What to expect, mission by mission

A new campaign starts at the first mission, and the game's REPLAY MISSIONS lists only missions you've finished. If your copy has played the campaign before, you can pick any of those.

| # | Mission | With the mod so far |
|---|---|---|
| 1 | Semper Fidelis | Where a new campaign starts. The scripted jumps, climbs and fights take either grip (see [Controls](#controls)); then your bare hands and the pistol, with its slide and magazine by hand, the M1014 shotgun with its shells by hand, the AKS-74u and the UMP-45. The guns after the pistol are new in this build and not yet played through in the headset. |
| 2 | Operation Swordbreaker | Played: the M4A1 with its holographic sight, reloads by hand, recoil, enemy guns taken off the ground. New in this build: the Mk11 with its scope, the M249 with its belt by hand, and the AT4 on the hotel roof, taken with both hands empty (drop the rifle or put it on a shoulder first) by reaching for it and holding the grip, or by holding the magazine button when the game says HOLD [R]. The frame rate dips below 60 on the opening street. |
| 3 | Uprising | Not played in VR yet. |
| 4 | Going Hunting | Not played in VR yet (the jet's back seat). |
| 5 | Operation Guillotine | Not played in VR yet. |
| 6 | Comrades | The opening drive played: in 3D around you, your body in the car, your head free. You play Dima here: his arms and guns are still the game's own. |
| 7 | Thunder Run | Not played in VR yet (a tank). |
| 8 | Fear No Evil | Not played in VR yet (a tank). |
| 9 | Night Shift | Not played in VR yet. |
| 10 | Rock and a Hard Place | The one mission played from start to end in VR: the car ride, every gun of it in your hands, the SMAW against the BMPs and the Stinger against the jet. |
| 11 | Kaffarov | Not played in VR yet. |
| 12 | The Great Destroyer | Played as far as the train: the interrogation and its fight with the game's own hands (either grip at each key it shows), then your bare hands and the pistol on the train. |

## Troubleshooting

**First, make a report.** Double-click **`Make a report.bat`** in the mod's folder. It makes **`BF3 VR report.txt`** there, everything
needed to see what went wrong: your copy of the game (Steam's or the EA app's) and where it is, Steam, the EA app and Battlelog's
links to it, VR, the mod's files and whether an antivirus took one, what's running, the crashes Windows saw, the launcher's, the
injector's and the mod's logs, and Steam's and the EA app's own lines about starting the game. Your Windows user and PC names,
e-mail addresses, account ids and tokens are taken out. Send that one file when you ask for help. (The launcher also makes it each
time it stops on a problem; everything the launcher window says is also in `build\launcher.log`.)

**The launcher window stops with a message.** Its last lines say what's wrong and what to do (the game wasn't found, SteamVR isn't installed, a file is missing...). Press Enter to close it, fix that, and double-click again.

**The window says the mod's folders aren't there, or closes at once.** Unzip the whole zip first (Extract All) and start `Start BF3 VR.bat` in the folder that makes.

**The launcher can't find Battlefield 3.** It looks where Steam and the EA app install it. If yours is somewhere else, the window asks for its folder: paste the folder that has `bf3.exe` in it (for example `C:\Program Files\EA Games\Battlefield 3`) and press Enter. It remembers it for the next time (`build\gamefolder.txt`).

**The launcher says Battlefield 3 has no settings yet.** The game has never been started on this PC, and would open in full screen. Start it once the normal way (Battlelog's PLAY CAMPAIGN), set OPTIONS, VIDEO, FULLSCREEN to OFF, close it, and double-click again.

**Windows says Smart App Control blocked a file.** Smart App Control, in some Windows 11 installs, lets no unsigned program run, and this mod's programs are unsigned (they're built from the code on this page): the mod can't run while it's on.

**Nothing in the headset.**
- The VR view starts with the level; the menus before it show as a flat screen. F8 switches between the two.
- SteamVR must be running and showing your headset: connect the headset first, then double-click.
- Quest with Quest Link or Air Link: make SteamVR the OpenXR runtime (see [Connect the headset](#connect-the-headset)).
- Other programs that hook the game's picture (ReShade and the like: a `dxgi.dll`, `d3d11.dll`, `d3d9.dll` or `dinput8.dll` in the game's folder) can fight the mod: move them out. The launcher warns about them.
- Programs that draw over games (MSI Afterburner's RivaTuner overlay, the Discord or NVIDIA overlays) hook the game's picture too: if the headset stays dark or the game crashes as it starts, close them and try again.
- Keep the game windowed: in full screen the mod can't size its picture for the headset.

**The game started flat, without VR (the mod isn't in it).**
- Start through `Start BF3 VR.bat` every time, and click PLAY CAMPAIGN once its window says "The mod is waiting for the game". A game started any other way, or more than 10 minutes later, runs without the mod: close it and start again.
- A line starting with **REFUSED** means that game wasn't started as the campaign (multiplayer, co-op): the mod stays out on purpose. Close that game, then click PLAY CAMPAIGN. If it says that of a game you started with PLAY CAMPAIGN, please [report it](#reporting-a-problem) with `build\injector.log`.
- "Battlefield 3 is running as administrator": don't run the game or the EA app as administrator (in their Properties, Compatibility, untick "Run this program as an administrator"). Running the launcher as administrator too would also do, but then only with SteamVR set as the PC's OpenXR runtime.
- `build\injector.log` has everything the launcher's injector saw.
- **"The EA app failed to start Battlefield 3".** While it waits for the game, the launcher reads the EA app's own log, and says so when the EA app itself couldn't start the game (its "Game launch error", for example error 9, LaunchFailed): that's the EA app, not the mod. The launcher sets it right once by itself: it closes the EA app, starts the game again, and asks you to click PLAY CAMPAIGN again. If the EA app fails a second time, the launcher stops and makes [the report](#reporting-a-problem): restart the PC and try again; then, in the EA app's menu, Help, App recovery; then have the game's files checked (Steam: Verify integrity of game files; the EA app: Repair).
- **A leftover injector.** `injector.exe` waits hidden for the game. One left running from an earlier try (the launcher window closed before the game started, with a version before this fix), or from an older copy of the mod in another folder, would put its own, old mod into the next game. The launcher now stops every one of the mod's injectors, in any folder, before it starts its own, and says how many and from where; the injector also ends by itself when the launcher window is closed before the game starts, or after 30 minutes without the game. [The report](#reporting-a-problem) lists each under "Running now" with its folder. To end one by hand: Task Manager (Ctrl+Shift+Esc), Details, `injector.exe`, End task.

**The controllers do nothing.** Click the game's window once: the mod sends your buttons to the game only while its window is in front. Also check that the controllers are on and the headset sees them.

**The view is off to one side, too high or too low.** Sit or stand the way you play, look straight ahead, and hold the right stick's click for 1.5 s (or press F9).

**Double vision, or the world too big or too small** (some headsets, wide ones most of all). With the game's window in front, press **Page Up** or **Page Down** a step at a time until the world looks life-size and both eyes meet: they change the zoom of the headset's picture (with the wide view on, its angle), 5 degrees a step, and the mod remembers it in `build\bf3vr.player.ini`, over `build\bf3vr.ini`. An update unzipped over the old folder keeps it. To go back to the mod's own view (and to have a lower `size` under `[window]` narrow the view again), delete `build\bf3vr.player.ini` with the game closed.

**The soldier crouches or walks by itself when you only lean** (easy to do seated). With the game closed, open `build\bf3vr.ini` in Notepad and add these lines at its end:

```
[head]
crouch=0
walk=0
```

**Low frame rate or stutter.**
- Set the headset to 72 Hz (the Index: 80 Hz).
- With the game closed, lower `size` under `[window]` in `build\bf3vr.ini`. It's each eye's picture in pixels: 2400 by default, 106 degrees wide. At 2160 the view is 100 degrees wide, at 2000 about 95, at 1600 about 83: just as sharp, a little narrower, and much lighter for the graphics card.
- Lower the game's own graphics settings (OPTIONS, VIDEO) and close programs you don't need.
- Only with a magnified scope up (the Mk11's): `scopegov 1` on a line of its own in `build\weapon.txt` lets the mod hold the scope's own picture back while the frame rate stays under 60 (see [What works today](#what-works-today)); the log's "Scope view:" lines say when.

**No reload sounds.** The mod's sounds (reloads, magazines landing, holsters) are made from your copy of the game the first time you start (`build\sounds`, about 120 files); if the game was started some other way, the mod makes them as it starts (about a minute). A newer version's new sounds (a new gun's) are made the same way at the first start after you update the mod in its folder: only the ones not there yet, in a few seconds to half a minute, and the ones you have are left as they are. They play wherever the game's own sound plays. `build\launch.log` says how many there are and which output they play on ("the mod's sounds play on ..."). If they couldn't be made, `build\gamesounds.log` says why: delete the `build\sounds` folder and start again to make them anew. They follow Battlefield 3's volume in Windows' volume mixer. (To only check things and make the sounds without starting anything: type `cmd` in the folder's address bar, Enter, then `"Start BF3 VR.bat" -Check`.)

**A crash, or anything else.** Please report it.

**The controllers don't vibrate** (no buzz on a shot, at a holster or on a reload). SteamVR is using a controller binding for the game with no vibration in it, often one saved for another mod of Battlefield 3. In SteamVR's settings, open Controllers, Manage Controller Bindings, pick the game and choose its default binding (this mod's own), then start the game again. `build\launch.log` says so when it happens.

**It runs very slowly on an AMD graphics card.** Fixed in 0.1.4: the mod now notices AMD's slow-to-read graphics memory by itself and works around it. In `build\bf3vr.log`, the line starting with `Skin: the game's uploads` says which way it went ("read slowly" on AMD cards, "read fast" on NVIDIA). If it's still slow, send that log.

**OBS or Discord shows both eyes side by side.** Press **Home** with the game's window in front: it switches the monitor between one eye fitted to the window (for streaming and recording) and both eyes, and remembers your choice.

**A quick-time prompt doesn't take the grip, or the grips press something they shouldn't.** In ordinary play (no cutscene) a grip answers once it's let go and squeezed again; a hand holding a magazine, a round or the charging handle doesn't answer (put it away first). Otherwise press the key the game shows (Space is A, E is Y, the others on the keyboard), and please [report it](#reporting-a-problem) with the mission and the moment. `qte 0` on a line of its own in `build\weapon.txt` switches the grips' quick-time events off.

**My hand goes through a gun, or the slap on the bolt release does nothing.** The mod makes each gun's shape the first time you hold it (a few seconds, in the background): try again a little later. If it stays so, `build\bf3vr.log` says in one line why the shape couldn't be made: send [the report](#reporting-a-problem).

**I don't want the holsters.** Write `holsters 0` in `build\holsters.txt` (and `holsters 1` to have them back). The weapon buttons switch guns either way.

**I want my own graphics settings.** The launcher sets the game's graphics to the mod's every start; to keep one of yours, change its value in `build\gamesettings.txt` (or delete its line). Your old settings file is kept as `PROF_SAVE_profile.before-bf3vr` in `Documents\Battlefield 3\settings`.

### Reporting a problem

Double-click `Make a report.bat` and attach the `BF3 VR report.txt` it makes (it has the logs below, with private details taken out).
Open an issue on this page's [Issues](https://github.com/criminalvictory/Battlefield-3-VR-The-Great-Destroyer/issues) tab. Say which mission, what you did and what happened, which headset and how it's connected, and your graphics card, and attach these from the unzipped folder's `build` folder:

- `launch.log`: the launch report, short. Step by step, whether the VR launch worked: the mod in the campaign, the game drawing, the VR runtime and the headset (OpenXR), the VR session, the session started, the first picture in the headset, the headset's focus, and a level in the headset (its picture there, or black). Each step says OK or PROBLEM, with what to try; LAUNCH WATCH lines tell if the game stopped handing over new pictures (a long load or a freeze) and where it was waiting.
- `bf3vr.log`: the mod's whole log (the launch report's lines are in it too, starting with **LAUNCH**). The next game start replaces both, so copy them before you start again.
- `injector.log`: how the game was started and whether the mod went in.
- `gamesounds.log`, if the sounds failed.
- `guns\gunshape.log`, if `bf3vr.log` says a gun's shape couldn't be made.

The logs name folders on your PC, which can include your Windows user name: look them over before you post them.

## Campaign only

The mod is made for the single-player campaign, and it refuses everything else by itself:

- the injector reads how each new Battlefield 3 was started and puts the mod only into Battlelog's **PLAY CAMPAIGN** start. A multiplayer or co-op start, or one it can't read, gets nothing, and the launcher window shows a **REFUSED** line;
- the mod checks again as it loads, and leaves at once, without touching anything, if the game isn't the campaign.

Once in, the mod stays in that game until it closes. So **close the game before you play online**: never join a multiplayer or co-op game from Battlelog while the campaign you started with the mod is still open.

Never try to get round it: loading anything into the game on online servers can get your account banned. This project contains only its own code: it ships no EA files and none of the game's sounds (those are made on your PC from your own copy).

## Contributors and credits

- [DanInDev](https://github.com/DanInDev): ideas for the holsters (a buzz when a hand reaches a holster spot, and a check that the game really switched guns after a draw), shared from his own work on the same game engine.
- [FrostbiteVR](https://github.com/justiceofrip/FrostbiteVR): the per-eye picture is made the way FrostbiteVR renders Bad Company 2, the game itself drawing a second complete view for the other eye; this mod found the same in Battlefield 3's own code.
- The mod's programs are built with the [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK) (the Khronos Group, Apache 2.0, with JsonCpp) and [MinHook](https://github.com/TsudaKageyu/minhook) (Tsuda Kageyu, BSD 2-clause), and the scopes' upscaling filter is AMD's FidelityFX Super Resolution 1 (MIT). Their licences are in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt) and in the release's `licenses` folder.
- This is a fan project, not affiliated with or endorsed by Electronic Arts or DICE. Battlefield is a trademark of Electronic Arts.

---

## License

The mod's own code is under the [MIT License](LICENSE): use it, change it and share it, keeping the copyright line and the license text with it. The libraries it's built with keep their own licenses ([THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt)). Battlefield 3 is EA's: nothing of the game is in this repository.

## For developers

Everything below is about building the mod from source and how it works. Every change is logged in [CHANGELOG.md](CHANGELOG.md).

### Status

**Pre-alpha (experimental).** It becomes **alpha** when: the head's position moves the view, the gun sits in the right hand and
the left hand is free, every campaign weapon aims correctly, it installs from the download steps above
without the finder steps, and a whole mission plays through without a crash.

### Building

Needs Visual Studio with the C++ workload (it brings CMake and Ninja). Then run:

```
build.bat
```

This fetches the OpenXR SDK and MinHook and produces `build\bf3vr.dll`, `build\injector.exe` and the tools
(`build\gamesounds.exe` and `build\gunshape.exe` among them), with the C runtime built in. `tools\package.ps1` makes the release zip
from one commit: a clean build of it, the settings it's played with, checked for anything private.

### Running from source

`Start BF3 VR.bat` in the repository's root starts a build from source just as the release does (it uses
`build\`): SteamVR, the injector, the game, and the first start's sounds. By hand:

1. Start SteamVR.
2. Run `build\injector.exe`. It waits for the game, and puts the mod only into a campaign start.
3. Start the campaign from Battlelog. The mod loads a few seconds after `bf3.exe` starts, and switches to
   the headset's view by itself once the player's soldier is in a level.
4. Settings live in `build\bf3vr.ini` (created on first run); the log is `build\bf3vr.log`.

A fresh build has none of the settings files the release ships in its `build` folder (`bf3vr.ini`,
`weapon.txt`, `pickups.txt`, `recoil.txt`, `holsters.txt`, `gamesettings.txt`, `grip.txt`, `pistol.txt`, `aimroute.txt`,
`m320grip.bin`): without them the mod starts from its own defaults, which aren't how it's played. Copy
them from the release into your `build` folder.

The mod fixes up a Steam quirk on its own: games launched while SteamVR runs get the 64-bit
runtime path, which a 32-bit game can't load. And with another OpenXR runtime active (a Quest's
Meta runtime), it points the game at SteamVR's 32-bit one (`[general] prefer_steamvr=0` leaves the
OpenXR loader's own choice).

### What works, in detail

- **Per-eye stereo** (the default): BF3 draws both eyes itself, every frame, from the same game
  state, each from its own camera half your eye distance from the middle of your head: real depth
  everywhere and no halo. It uses a developer setting the retail game still has,
  `GameRenderSettings.SplitScreenTestViewCount`: set to 2 the game draws a second complete view
  beside the first. `src/views.cpp` hooks the function that builds the frame's views and makes them
  the two eyes (the left eye in the left half, the right eye in the right half of a window twice as
  wide), and each eye is shown its half. This is how FrostbiteVR renders Bad Company 2 (a native
  second view), found in BF3's own code. BF3 draws the gun and arms of both views from one camera,
  so the free hands and gun are shifted per eye by the mod (the game's own arms, with free hands
  off, have no depth of their own). Two full views hold the headset's 72 fps with about 6 ms to
  spare a frame (tested in the campaign's opening level).
- **A steady frame rate**: every frame, BF3 asks Windows for the PC's memory statistics
  (`GetPerformanceInfo`), and Windows counts every process and thread on the computer to answer:
  11 ms of the 13.9 ms a frame has at 72 Hz, with SteamVR, a browser and the usual apps open
  (380 processes, 7,800 threads). Made in 2011, it took microseconds then. The mod answers it from
  a copy refreshed once a second (`src/perfinfo.cpp`); with two views the frames no longer missed
  the headset and turning with the stick stopped stuttering.
- **Stereo 3D from depth** (Ctrl + Scroll Lock): every game frame becomes both eyes. Each pixel is shifted
  sideways by the eye's offset over its distance, read from BF3's own depth buffer, so both eyes
  update every frame and head turns no longer double the picture (alternate-eye rendering, one
  frame per eye in turn, did). The gun and arms get their real depth too: BF3 draws them squeezed
  towards the eye and marks them in the stencil buffer, so the mod un-squeezes exactly those
  pixels. Cheaper than per-eye views, but around near things against far ones it shows a soft halo.
  Ctrl + Scroll Lock cycles per-eye views / depth 3D / hybrid / alternate eyes / mono / frozen; tuning is
  in `build\depth3d.txt` (see below).
- **Head tracking**: the head turns the picture, including tilt.
- **Motion controller aiming**: the right controller aims; bullets, muzzle flash and the gun model
  follow it. Buttons and sticks are sent to the game as keyboard and mouse, except the smooth turn:
  in stereo the right stick's turn goes into the look angle the mod writes each frame, which the
  picture follows, so it turns in step with the frames (`[controllers] turn_degrees`, 266° a
  second at full deflection). As a mouse move once a frame it stuttered: BF3 reads the mouse on a
  rhythm of its own, and about 6 times a second a frame barely turned and the next turned double.
  In theater mode the stick still moves the mouse, a little every millisecond.
- **Square window**: in windowed mode the game draws square (default 1600×1600), which gives
  more view top to bottom and a sharper headset image; with per-eye views twice as wide (3200×1600:
  each eye a square half). That may be bigger than the monitor, so the window on the desktop is fitted
  to the monitor instead (borderless, in the shape of the picture it shows) while the game still draws
  at that size: see [Streaming and recording](#streaming-and-recording).
- **Floating screen** (theater) mode as an alternative to stereo.
- **Wide field of view** (on by default at 100°; Page Up / Page Down to tune, Delete on/off with the developer's keys on): the campaign
  renders a fixed 55° whatever the menu says, which in a headset looks like a square window. The
  mod widens the render, cull and first-person views each frame, and rescales the gun and arms'
  projection where it is uploaded to the graphics card, so they keep their real size. The
  headset image is shown at the same angle, so the world stays life-size.
- **Free hands** (Insert on/off, on by default): the gun is drawn in your right hand, pointing
  where the controller points, held where the game's own hand holds it, with its scope and stock;
  the left glove follows the left controller, its fingers following yours: the left trigger
  curls the index finger, the left grip the other three, and resting your thumb on the stick folds
  the thumb in (all three: a fist; the grip alone: pointing). Relaxed, the fingers are a little
  curved and apart. Each finger bends at its own joints (`src/handpose.cpp`), and three small
  cracks in the glove model that bending opens are filled. The arms, sleeves and body are folded
  away (floating hands). With "lineup 1" in `build\pouch.txt` (off, it was pressed by accident too
  often), the left View button on the Steam Frame lines the glove up: the first press holds it
  still, put your hand into it and press again (kept in `build\grip.txt`, with
  "spread 0.7" for how far apart the relaxed fingers are and "leftwidth 1.15" for the palm's
  width). Without one, the line-up made on the test setup's Steam Frame is used. BF3 draws the first-person arms in a space of their own whose origin jumps a metre at a
  time as you walk; each draw places the hands with the eye and axes of the view-projection it is
  drawn with, so they stay steady while moving.
- **Both hands on the rifle**: squeeze the left grip with the left hand at the handguard and the
  glove takes hold there (the game's own grip for this gun) until you let go of the grip. While it
  holds, the gun points from your right hand towards your left, and the game aims along it, as long
  as your left hand is where a handguard could be: with it beside, below or behind the right hand,
  or the hands closer than 10-15 cm, the right hand alone points the gun. The change eases in and
  out over about a tenth of a second, letting go too, and a sudden jump of the left controller's
  tracking can't throw the gun across the view. (Before, the rifle followed the left hand wherever
  it went while the grip was squeezed, and swung round in one jump.)
- **God mode** (End on/off, on by default): the campaign runs the game's own server inside
  `bf3.exe`, and the health that decides deaths is the server's copy of your soldier. The mod finds
  it every spawn and keeps health and maximum health at `[god] value` (100000), so hits are undone
  the next frame and the screen stays clean. Scripted deaths still happen (leaving the area of
  operation fails the mission).
- **Bodies stay** (`[world] corpse_seconds`, 600 here; 0 = the game's own 15 s): raises how long
  dead soldiers lie before the game removes them (tested: none of 12 removed in 3 minutes).
- **Magazine eject** (X): on guns with a magazine button (the M416, M16A4, M4A1, KH2002, the pistol)
  a tap drops the magazine out of the gun, and it falls to the ground. Its rounds go back to the spare
  ones and one stays in the chamber. Guns without a button (the AK family, the G3A3) have the magazine
  pulled out by hand, or knocked out by the new one. Holding X is the game's own reload. The gun
  doesn't fire with the magazine out or one round left (run dry, the game would reload by
  itself). The ammo is the count in the game's own server, which decides firing.
- **All ammo from the belly** (the rifle, the pistol, the launchers, the grenade launcher, any gun):
  with the magazine out, the bolt locked back on an empty one, or the grenade launcher open, bring
  the left hand to your belly and squeeze the left grip (nothing is drawn there). The gun's own
  magazine (or a 40 mm round) appears in the left glove. Bring it under the magazine well, top
  first: it's caught below the well's mouth, straightened as you push, slides only along the well,
  and seats when pushed home. Only then do its rounds go in. Let go of it on the way and it falls to
  the ground; let go of it at the belly and it goes back. With any other gun,
  squeezing there is the game's own reload. `build\pouch.txt` changes the place (`zone below forward
  reach`), turns it off (`pouch off`), lets a tap of X put a magazine in (`xinsert 1`), stops other guns
  from reloading there (`anygun 0`), and runs a scripted hand for pictures (`test 1`, `test 2 -0.05`).
- **Free hands** (every gun the mod has a hold for): the right grip button holds the rifle. Squeeze it once, and
  letting it go lets go of the rifle: it stays held by a hand on its handguard alone (still aimed,
  nothing fires) or it falls, or is thrown if the hand was moving. It lands with the game's own
  sound of a dropped gun and lies there until a hand takes it. Squeeze either grip near it and it
  comes into that hand by its pistol grip; that hand's trigger fires it (the left trigger with the
  rifle in the left hand). The free right hand relaxes, points and makes a fist from its own
  controller. With the rifle on the ground, holding X is the game's own pickup of a gun lying near.
  After the headset is off and on, or a cutscene, a hand must squeeze again before letting go
  drops anything. `build\freehands.txt` with `off` turns it all off (the rifle stays in the right
  hand as before).
- **No orange enemy markers**: the HUD's orange markers over enemies and targets are taken off at
  the end of each frame (they can't be told from the blue squad markers by their draws, so the
  picture under the HUD is kept and put back wherever the HUD made it more orange); the blue
  markers stay. `build\weapon.txt` `orangemarkers 1` keeps them.
- **The right glove's trigger finger** pulls with the trigger; **the ACOG's reticle and red cable**
  are drawn brighter (`scopeboost`).
- **Spent casings** come out of the drawn gun's ejection port on every shot, on the M416 and the
  train's pistol: each casing's world placement is moved on the graphics card with the gun near the
  port, less as it flies, not at all once it's 1.3 m out, so the ones on the ground stay where they
  fell (`build\weapon.txt` `casings 0` leaves them where the game throws them).
- **The muzzle flash and smoke** come from the drawn gun's barrel: the game's own, moved with the gun
  on the graphics card, whole quads at a time, those in a long shape round the line of the game's
  barrel, 0.6 m around it and 2.2 m past its muzzle (`build\weapon.txt` `flashquads 0` leaves them
  where the game has them). As in the game itself there's more smoke than fire, a flame now and then.
  Dirt thrown up where a bullet lands stays there, unless that's within about 3 m. When the shooting
  stops the smoke stays in the air as the gun moves on.
- **Your head moves the view** (6DoF): lean, duck and peek up to 25 cm freely and 32 cm at most; walk
  in your room and the soldier walks under your head; crouch for real and the game's soldier crouches
  with your eyes staying where they are; a small movement in cutscenes and rides. The right stick's
  click, held 2.5 s, sets where standing is. `[head]` in `build\bf3vr.ini`: `position`, `walk`,
  `crouch`, `lean`, `lean_most`, `follow_start`, `stick_full`.
- **Reloading by hand** (the rifle): run dry, the bolt locks back and the gun won't fire. With a
  full magazine in, either slap the bolt release on the left of the receiver, behind the magazine
  well, with the left hand (the flat-screen game's empty reload), or put the left hand on the
  charging handle at the back of the receiver's top, hold the left trigger, pull back at least
  4.5 cm and let go. The handle slides back with the hand and springs home. The ejection port's
  cover swings open on its hinge when the bolt moves (a shot, the bolt locked back, the handle
  drawn) and shuts again once it has been still for a moment.
- **The guns' collision shapes**: what keeps the free hand out of the gun and feels the slap on the
  bolt release is a distance field per gun (`build\guns\<name>.bin`). They're made from the game's
  own meshes, so none is in this repository or the release: the first time a gun is held, the mod
  reads its mesh back from the graphics card and `build\gunshape.exe` (from `tools/gunshape.cpp`)
  makes its shape in the background, a few seconds a gun, once (`src/shapemake.cpp`, the recipes
  in `src/shaperecipes.h`). Until then, or if it can't be made (`bf3vr.log` says why in one line),
  the hand goes through that gun and everything else works.
- **Reload sounds**: the magazines, bolts, levers and launchers sound as the game's own: the
  clips are cut out of the game's own sound files on the player's PC (`build\gamesounds.exe --mod`,
  from `tools/gamesounds.cpp`, run by the launcher at the first start into `build\sounds`; after an
  update in place `--missing` makes only the clips of its table that aren't there), so none
  of the game's audio is in this repository or the release. The mod plays them at the gun when the
  hands do the same; they follow bf3.exe's volume in Windows like the game.

Tested on a Steam Frame through SteamVR's OpenXR runtime with an RTX 3080.

### Known issues

- Per-eye views: the HUD is the game's own, drawn into both eyes at the screen's distance, not yet
  placed for VR. Keep `[stereo] async_handover=0`: handing frames to
  SteamVR on a thread of its own switches the graphics device to its locked functions, which go
  round the mod's draw hooks (the gun's FOV fix and free hands stop).
- Depth 3D: around near things against far ones (the gun, a soldier against the sky) the gap the
  other eye would see behind them is filled from the background beside it, which can look like a
  soft halo. `bias -1` / `bias 1` in `depth3d.txt` keeps one eye exactly as the game drew it and
  puts all the filling in the other; hybrid mode (Ctrl + Scroll Lock) alternates which eye is drawn.
- With the wide view, the sleeve near the shoulder can look stretched: that part of BF3's arm
  model is normally off screen.
- Free hands: guns the mod has no hold for yet get the game's own arms, and their flash and smoke
  stay where the game has them, off the barrel by as far as you lean. A short piece of bare forearm
  shows under the left glove's cuff.
- A dark layer shows around the arms, glove and gun, plainest against bright light (being looked
  into: one pass of the arms placed for the wrong eye is the suspect).
- Looking along a forearm into its open elbow end shows a black disc.
- The charging handle has no sound of its own yet (the M16's reloads never use it); letting it
  go plays the bolt going home.

Scenery no longer disappears when the controller aims away from where you look: the camera BF3
decides what to draw from is found once with the cull hunt (F10, then F6/F5: developer's keys) and turned with the
head like the render cameras.

### Streaming and recording

In 3D the game draws both eyes side by side, twice as wide as it is tall, at the headset's size
(`[window] size` per eye). Once the headset has its frame, the mod draws over that picture with one
view, the left eye's, so whatever records the game gets a normal-looking picture: 85° across, and
steadied while you wear the headset (small head shakes taken out, the horizon kept level). The
headset's own picture isn't touched. **Home** switches between this view and both eyes (saved; only
while the game is in front).

The game's window on the desktop fills the monitor, borderless, in the view's shape (16:9; 2:1 with
both eyes), and shows the same view: the game still draws at the headset's size, told its window is
that big, and Windows scales the picture into the window. `[window] fit=0` puts back the window as big
as the game draws, bigger than most monitors (the desktop then shows only its top left).

- **OBS and Streamlabs:** a *Game Capture* source, mode *Capture specific window*, window
  *[bf3.exe]: Battlefield 3*, then right-click it → *Transform* → *Stretch to screen* on a 16:9
  canvas. The view is drawn 16:9 into the 2:1 picture, so stretching it puts it back in proportion.
  The mod finishes each frame before any capture hook sees it, so no capture setting needs changing.
- **Discord** (*Go Live*, streaming Battlefield 3): Discord copies the window as Windows shows it, so
  it gets the whole view, 16:9. (With the window bigger than the screen, Windows kept only its part on
  the monitor up to date, and Discord showed that part moving in a corner of an old, frozen picture.)
- **Other recorders that hook the game** (Fraps, Bandicam's game mode) should get the same view. One
  that saves the game's own frame at its size records it 2:1: stretch it to 16:9, or set
  `stream_aspect=0` for a picture drawn in its own shape.
- **Screen, display or window capture** sees the window as it is on the monitor: the whole view.

`[stereo]` in `build\bf3vr.ini`: `stream_view` (1 = on), `stream_fov` (degrees across),
`stream_aspect` (the shape it's drawn for; 0 = the window's own), `stream_fit` (1 = the whole eye's
picture, black at its sides), `stream_steady` (1 = steadied), `stream_tilt` (degrees below where you
look). The subtitles and the HUD's corners sit at the eye's outer edge, so the cropped view leaves
them out; the pause menu is cropped too (Home shows both eyes).

### Keys and settings

The controllers' buttons are under [Controls](#controls) above. On the Steam Frame, A, B, X and Y are
all on the right controller; the mod uses SteamVR's own Frame controller profile
(`XR_VALVE_frame_controller_interaction`). On Touch-style controllers X and Y are on the left.

The mod's keys count only with the game's window in front, as the game's own keys do.

| Key | Action |
|---|---|
| F8 | Switch floating screen / stereo |
| F9 | Recenter |
| Insert | Free hands on / off (the game's own arms) |
| End | God mode on / off (saved) |
| Home | Stream view on / off: one eye in the window and OBS, or both eyes (saved) |
| Page Up / Page Down | With the wide view on: its angle, 5° a step. Otherwise: zoom of the headset image (saved; for double vision) |

The developer's keys below do nothing unless `build\bf3vr.ini` has `dev_keys=1` under `[general]` (read
at each press, no restart). They're off by default so that a stray key can't change a saved setting
or start a memory scan: F12 is also Steam's screenshot key.

| Key | Action |
|---|---|
| Delete | Wide field of view on / off (saved) |
| F1 | Flip which way the gun model follows the hand |
| Ctrl + F4 / Ctrl + F2 | Flip head yaw / pitch direction (saved; not needed: the direction is BF3's) |
| F7 | Swap eyes |
| F11 | How many frames old the head pose each eye image is shown with (0-3, default 1; saved) |
| F10, then F6 / F5 | The camera finder, then the cull hunt / saving the copy found |
| F12 | The skeleton finder |
| Ctrl + F3 | The look-angle finder |
| Ctrl + Scroll Lock | 3D method: depth 3D / hybrid (each frame from one eye, the other from its depth) / alternate eyes / mono / frozen |
| Pause | Skinning research hooks on / off (off by default: they cost frame rate) |

`build\hands.txt`: gloves palette numbers ("0-23" left hand, "24-54" right hand, or "all") and
"shift x y z" in metres. On foot the bones' axes are x right, y up, z back towards you.

`build\weapon.txt` (research on the rifle's three draws): "4 shift 0 -0.3 0" moves bones of the
colour pass ("hide" instead of the shift hides them), "pass 1: 0" / "pass 2: 4-5" names the bones
of its other draws (the magazine's rounds, the depth pass), "watch" logs which bones move, "dump"
writes the gun's mesh to `build\gun_*.bin` once, "floor 0.05" sets where a dropped magazine stops.

#### Depth 3D tuning: `build\depth3d.txt`

Read a few times a second while the game runs, e.g.
`strength 0.7 min 0.3 gun 2.4 mask 32 bias 0 fill 1 debug 0 show 0 hold 0`:

| Word | Meaning |
|---|---|
| strength | share of your real eye distance used (1 = full) |
| min | nearest distance in metres anything counts as |
| gun | the gun and arms' distances are multiplied by this (1 = off) |
| mask | stencil bits marking the gun and arms (BF3 uses 32; 0 = use "below" metres instead) |
| bias | -1 / 1: the left / right eye gets the game's own picture, the other eye all the shift |
| fill | gaps behind near things: 0 stretched, 1 mirrored, 2 mirrored and softer |
| debug | 1 distance in coloured bands, 2 distance as grey (white = 2 m), 3 the stencil |
| show | 1 = the desktop window shows the right eye |
| hold | 1 = freeze the picture (both eyes keep being made from it) to compare settings |
| save N | when N changes, both eyes are written to `build\eye_left.bmp` / `eye_right.bmp` |

`tools\eyebmp.ps1` turns the saved eyes into PNGs and reads distances or stencil values from them.

The hand's turn is now read from each camera the game builds, so the picture no longer sways as
the arm moves at any frame rate (the old fixed frame lag is only a fallback).

Finders and tests (needed once per setup; see the log for results):

| Key | Action |
|---|---|
| F10 | Camera finder (look around, turn and walk after the beeps) |
| F6 / F5 | Cull hunt: with the arm held out so scenery is missing, tries one camera copy every 3 s / F5 saves the one that brings it back |
| Ctrl + F3 | Look-angle finder |
| F12 | Gameplay skeleton finder (research: it turned out not to drive the drawn arms) |
| F15, F13 / F14 | Draw hunt, driven by `tools/drawhunt.ps1` (keys no keyboard has): hides half the skinned draws at a time until the one asked about is found |

`tools/selfcheck.ps1` confirms from the log, after a launch, that the hooks, OpenXR session, wide
view, god mode, bodies and the 3D's depth buffer all came up.
`tools/weapontest.ps1` tries weapon.txt settings in turn and puts the game's own picture for each
side by side (saved by the mod, so no screenshot shows an old frame). `tools/videoframes.ps1` pulls
frames out of a recording (Windows' own media components, no ffmpeg) into one sheet.
`handview.exe` (built with the mod, from `tools/handview.cpp`) draws the left glove from its mesh
dump in finger poses, each from four sides, without the game: `handview build out.bmp
0,1,1,1,1,0.7` (index, middle, ring, little, thumb, spread). It reports how deep fingers sink into
each other, whether the depth passes stand out of the glove, and with "a=1" the glove's pieces
and openings; "c=-1" culls back faces as the game does.
`sampler.exe` (built with the mod, from `tools/sampler.cpp`) shows where BF3's threads spend their
time, from outside the game: `sampler 5 10` pauses the 10 busiest a thousand times a second for
5 s and lists, per thread, the code it ran and the system calls it waited in, with the game code
behind them (it found the 11 ms memory statistics call). Thread CPU is read from cycle counts:
Windows' thread times are only charged at clock ticks and make bursty threads look idle.
`capture.exe` records what the PC plays (`capture record 7 take.wav`), shows how loud each 10 ms
was (`loud`), cuts mono clips (`cut take.wav 3.19 0.28 bolt.wav 4.17`, the gain undoing bf3.exe's
volume slider at the time) and lists each app's volume (`sessions`); `playtest.exe bolt` plays a
clip as the mod does. `anglescan.exe` finds the look angles from outside the game (pairs that
match the game camera; `test` nudges each), what points at an address (`refs`), pointer paths
from the soldier (`path`), and the level's ambient occlusion settings (`ao`).
`tools/drawhunt.ps1` presses the hunt keys and game keys (by scan code: the game ignores keys
sent by virtual-key code alone; `-Action fire` holds the mouse button across frames, `f16` is
the magazine button, `f17` turns as the right stick does for 3 s, with the turn's evenness frame
to frame in the log) and takes screenshots while BF3 is in front; `tools/say.ps1`
reads test instructions aloud. Memory tools for campaign research, run with the 32-bit
PowerShell: `peek.ps1`/`peekabs.ps1` (read), `poke.ps1`/`pokeabs.ps1` (write, refusing unless the
expected value is there), `watch.ps1`/`watchabs.ps1`/`treewatch.ps1`/`treediff.ps1` (watch values
change), `findref.ps1`/`findtext.ps1`/`findfloat.ps1` (search), `fields.ps1`/`owner.ps1` (read
BF3's own type tables), `soldiers.ps1`/`corpses.ps1` (the server's soldiers and their bodies),
`findints.ps1`/`pokeint.ps1` (find and set whole numbers such as the ammo), `pathto.ps1` (pointer
paths from an object to an address), `dwords.ps1` (memory as values, pointers into bf3.exe named).
Code research: `dumpimage.ps1` saves bf3.exe as it is in memory (its code is only decrypted there)
to `build\bf3_mem.exe`, which Visual Studio's `dumpbin /disasm /range:...` disassembles;
`findcode.ps1` lists where the code uses an address (a global, a function, a string), with the
instructions around it. `tools/alarm.ps1` rings on every speaker at once (with SteamVR running,
Windows' default output is the headset) to call the tester back when a test is ready.

`CAMERA_GUIDE.md` describes finding the camera by hand with Cheat Engine instead.

### How it works

- `src/d3d11_hook.cpp`: hooks `IDXGISwapChain::Present` to grab each finished frame.
- `src/xr_runtime.cpp`: OpenXR session on the game's own D3D11 device; submits frames as a
  floating quad or as per-eye projection views. `src/xrpick.h`: which OpenXR runtime the game uses
  (SteamVR's 32-bit one when another is active).
- `src/camera.cpp`: hardware write breakpoints on BF3's camera copies. Right after the game writes
  one, the mod shifts it per eye and turns it to the head.
- `src/aim.cpp`: writes the player's look angles, so the game aims where the controller points.
  It finds them itself from the player's soldier at each life or checkpoint, writes only to an
  object of their type, and of those only to the one that keeps a small nudge (the others are
  copies the game rebuilds every frame).
- `src/input.cpp`: OpenXR actions for the controllers, injected as keyboard and mouse.
- `src/finder.cpp`: in-game memory finders for the camera, look angles and settings.
- `src/window.cpp`: keeps the game drawing square (twice as wide in stereo), past the screen size
  limit, with its window fitted to the monitor and the game told it's still that size.
- `src/skin.cpp`: hooks the bone uploads and draws of skinned meshes; finds the hands' draws and
  draws them with changed bones.
- `src/views.cpp`: per-eye views: BF3's split-screen test turned on, its view builder hooked so
  the two views are the eyes.
- `src/depth3d.cpp`: finds BF3's depth buffer (by its clears) and makes both eyes from the frame
  and its depth in a pixel shader.
- `src/god.cpp`: keeps the health of the server's and the client's copy of the player's soldier.
- `src/bodies.cpp`: keeps dead soldiers' bodies longer.
- `src/ammo.cpp`: reads and sets the server's ammo count for the magazine eject (`skin.cpp` draws
  the magazine falling).
- `src/hands.cpp`: free hands: where the gun and the left glove go relative to the head, from the
  controllers (`skin.cpp` redraws the gun and arms there).
- `src/handpose.cpp`: the left glove's fingers, bent joint by joint (joints and finger directions
  measured from the mesh dump).
- `src/perfinfo.cpp`: answers the game's once-a-frame memory statistics call from a copy.
- `src/reload.cpp`: the rifle's bolt, charging handle, bolt release and ejection port cover, worked
  by the left hand.
- `src/sounds.cpp`: plays the reload sounds cut from the game's files (XAudio2, on a thread of its own).
- `src/effects.cpp`: the muzzle flash, its smoke and the casings moved to the drawn gun on the
  graphics card, and the census and scans that find what comes with a shot.
- `src/headmove.h`, `src/headmove.cpp`: the head's place in the room: lean, walk and crouch.
- `src/campaign.h`: the one rule for what counts as a campaign start, used by the injector and the DLL.
- `injector/main.cpp`: loads `bf3vr.dll` into `bf3.exe`, only into a campaign start.
- `tools/launch_vr.ps1` (run by `Start BF3 VR.bat`): the launcher; `tools/package.ps1`: the release.

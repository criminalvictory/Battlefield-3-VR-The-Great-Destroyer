The settings every release zip carries, as the mod is played: bf3vr.ini (the camera's and the look angles' places in the game, the
picture's size and angle, god mode on), grip.txt (the left glove's line-up), pistol.txt (the train pistol's slide and magazine by hand),
aimroute.txt (the route to the look angles) and m320grip.bin (the game's own left hand on the grenade launcher's grip).

tools\package.ps1 reads them with -SettingsFrom release\settings; the release build on GitHub's servers
(.github\workflows\release-build.yml) does so. Built on a PC, the packer reads them from build\ instead, as they are played there.

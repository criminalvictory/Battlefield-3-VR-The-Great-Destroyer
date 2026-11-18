cmake_minimum_required(VERSION 3.24)
project(bf3vr C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The C runtime built in (/MT), so the mod, the injector and the tools run on any PC without Microsoft's VC++
# redistributable installed. The OpenXR loader's own build asks for the DLL runtime for a static loader: set back below.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

include(FetchContent)

# OpenXR loader, linked statically into the mod DLL.
set(DYNAMIC_LOADER OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(openxr
    URL https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.1.38.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# MinHook for function hooking. Its own CMake is old, so build the sources directly.
FetchContent_Declare(minhook
    URL https://github.com/TsudaKageyu/minhook/archive/refs/tags/v1.3.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR do-not-add)

FetchContent_MakeAvailable(openxr minhook)
# Its CMake sets the DLL runtime on a static loader (src/loader/CMakeLists.txt): the static one, as everything else here.
set_target_properties(openxr_loader PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(minhook STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde32.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c)
target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)

add_library(bf3vr SHARED
    src/dllmain.cpp
    src/d3d11_hook.cpp
    src/xr_runtime.cpp
    src/config.cpp
    src/camera.cpp
    src/finder.cpp
    src/aim.cpp
    src/input.cpp
    src/qtewatch.cpp
    src/fov.cpp
    src/window.cpp
    src/skin.cpp
    src/depth3d.cpp
    src/god.cpp
    src/bodies.cpp
    src/ammo.cpp
    src/hands.cpp
    src/handpose.cpp
    src/freetune.cpp
    src/freeplaces.cpp
    src/views.cpp
    src/perfinfo.cpp
    src/gputime.cpp
    src/edgefill.cpp
    src/effects.cpp
    src/reload.cpp
    src/sounds.cpp
    src/pad.cpp
    src/gunshape.cpp
    src/shapemake.cpp
    src/collide.cpp
    src/pouch.cpp
    src/handmag.cpp
    src/flap.cpp
    src/scopezoom.cpp
    src/scopeview.cpp
    src/framemem.cpp
    src/footsteps.cpp
    src/loose.cpp
    src/ground.cpp
    src/pickups.cpp
    src/pickparts.cpp
    src/m320.cpp
    src/shellload.cpp
    src/rocket.cpp
    src/pistolmag.cpp
    src/pistolslide.cpp
    src/overlay.cpp
    src/shaders.cpp
    src/postpin.cpp
    src/streamview.cpp
    src/headmove.cpp
    src/recoilrun.cpp
    src/headtest.cpp
    src/log.cpp
    src/launchreport.cpp)
target_compile_definitions(bf3vr PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(bf3vr PRIVATE openxr_loader minhook d3d11 dxgi d3dcompiler xaudio2 ole32)

# Diagnostic: OpenXR instance creation outside the game.
add_executable(xrtest tools/xrtest.cpp)
target_compile_definitions(xrtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(xrtest PRIVATE openxr_loader)

# The left glove's finger poses drawn from a mesh dump, to shape them without the game.
add_executable(handview tools/handview.cpp src/handpose.cpp)
target_compile_definitions(handview PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The left glove fitted onto the rifle's charging handle from the mesh dumps, for hands.cpp.
add_executable(leverfit tools/leverfit.cpp src/handpose.cpp)
target_compile_definitions(leverfit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The left glove fitted round the rifle's handguard from the mesh dumps, for hands.cpp.
add_executable(gripfit tools/gripfit.cpp src/handpose.cpp)
target_compile_definitions(gripfit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The game's own hand on the launcher's case, from a recording of its reload and the mesh dumps.
add_executable(rocketgrip tools/rocketgrip.cpp src/handpose.cpp)
target_compile_definitions(rocketgrip PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The left glove fitted round the launcher's case joint by joint, thumb round the other way, for src/rocketposes.h.
add_executable(rocketfit tools/rocketfit.cpp src/handpose.cpp)
target_compile_definitions(rocketfit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The rifle's collision shape (a distance field) for the mod, from the gun's mesh dump.
add_executable(gunshape tools/gunshape.cpp src/handpose.cpp)
target_compile_definitions(gunshape PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The guns' shapes made on the player's PC (src/shapemake.cpp, src/shaperecipes.h): every recipe a gunshape command, its names against skin.cpp's
# held guns, and a mesh dump made into a shape by gunshape.exe as the mod does it (the dump written, the process, the .tmp, the rename, the load),
# with every way it can fail leaving no file and one line. Runs gunshape.exe from its own folder.
add_executable(shapemaketest tools/shapemaketest.cpp src/shapemake.cpp src/gunshape.cpp src/log.cpp)
target_compile_definitions(shapemaketest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(shapemaketest PRIVATE d3d11)
add_dependencies(shapemaketest gunshape)

# The left glove fitted round a gun's magazine, each part of each finger on its surface, for src/magposes.h.
add_executable(magfit tools/magfit.cpp src/handpose.cpp)
target_compile_definitions(magfit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The free left glove kept out of the rifle (src/collide.cpp) on scripted hand paths at 72 Hz, from
# the rifle's collision shape: penetration, passing through, jerks, jitter, slaps felt, time a frame.
add_executable(collidetest tools/collidetest.cpp src/collide.cpp src/gunshape.cpp src/handpose.cpp src/log.cpp)
target_compile_definitions(collidetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# A magazine from the pouch at the pelvis pushed into the rifle's well by the left hand (src/pouch.cpp)
# on scripted frames at 72 Hz: the zone, the squeeze, the body's heading, the funnel, the seat, letting go.
add_executable(magtest tools/magtest.cpp src/pouch.cpp)
target_compile_definitions(magtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(magtest PRIVATE OpenXR::headers)  # pouch.h's XrPosef: the headers only, no loader

# Any other gun's magazine and lever by the left hand (src/handmag.cpp) on scripted frames at 72 Hz: the magnet, the gun or the magazine, the knock, the lever.
add_executable(heldmagtest tools/heldmagtest.cpp src/handmag.cpp src/pouch.cpp)
target_compile_definitions(heldmagtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(heldmagtest PRIVATE OpenXR::headers)

# Reads the game's own sound assets out of its data files (catalogues, bundles, EBX, the audio chunks); writes only to build\gamesounds.
add_executable(gamesounds tools/gamesounds.cpp)
target_compile_definitions(gamesounds PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The launcher loaded by the left hand (src/rocket.cpp) on scripted frames at 72 Hz: the spent case, the shoulder, the
# catch at the launcher's back, the push and the seat, letting go, the launcher or the hand lost, the game's own reload.
add_executable(rockettest tools/rockettest.cpp src/rocket.cpp)
target_compile_definitions(rockettest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(rockettest PRIVATE OpenXR::headers)  # rocket.h's XR types: the headers only, no loader

# Loose things let go of (src/loose.cpp: magazines, cases, casings as small rigid bodies) on scripted frames: the eject, the fall, the bounce, a minute on the ground.
add_executable(loosetest tools/loosetest.cpp src/loose.cpp)
target_compile_definitions(loosetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The grenade launcher's reload by hand (src/m320.cpp: its barrel, the round in it, the hands on it) on scripted frames: the release, the spring, gravity, the belly, the magnet, the push shut.
add_executable(m320test tools/m320test.cpp src/m320.cpp)
target_compile_definitions(m320test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The train pistol's slide worked by the left hand (src/pistolslide.cpp, as hands.cpp runs it) on scripted
# frames at 72 Hz: the three grips and which the hand takes, the grab and its reach (the first headset
# test's misses), a press waiting for the hand, the hand gliding on and off, fresh presses, the ride back
# and its stops, the spring home, the slide stop, the trigger's rack, the hand or the pistol lost, the
# hand still coming in as it takes hold (from every side), tracking jumps, why a press missed.
add_executable(pistolslidetest tools/pistolslidetest.cpp src/pistolslide.cpp)
target_compile_definitions(pistolslidetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The train pistol's magazine by the left hand (src/pistolmag.cpp, as hands.cpp runs it) on scripted frames at
# 72 Hz: the take at the pelvis, the carry and the flat base, what's caught under the grip and what isn't, the
# ride up the raked line, the snap home and how near the palm came, parked and pushed on, let go and fallen, the
# hand or the pistol lost, the game's own reload, the carries, broken inputs. The design's reloadtest, ported.
add_executable(pistolmagtest tools/pistolmagtest.cpp src/pistolmag.cpp)
target_compile_definitions(pistolmagtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pistolmagtest PRIVATE OpenXR::headers)  # pistolmag.h's XrVector3f: the headers only

# The train pistol's rounds with its magazine by hand (src/pistolammo.h, header only, as ammo.cpp runs it) on a stand-in for the game's count:
# fired to empty and the slide on its stop, the trigger at an empty chamber, the magazine out (the chambered round stays) and a fresh one in (its
# rounds from the spare ones), the slide sent home, tugged or racked, the game's own reload undone or kept, the hidden round put back, a write held
# off a frame, random sessions with no round made or lost.
add_executable(pistolammotest tools/pistolammotest.cpp)
target_compile_definitions(pistolammotest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The M1014's shells by hand (src/shotgunammo.h, header only; nothing in the game runs it yet) on a stand-in for the game's count: fired to empty and
# the bolt held open, a shell at a time up the tube (its size the caller's), the port load and the release, the handle racked, the trigger at an empty
# chamber, the game's own reload undone or kept a shell a loop with the hidden round out, that round put back, a write held off a frame, random
# sessions with no shell made or lost.
add_executable(shotgunammotest tools/shotgunammotest.cpp)
target_compile_definitions(shotgunammotest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The M1014's shells by hand, what the hands do (src/shellload.cpp, as hands.cpp runs it) on scripted frames at 72 Hz: a shell from the belly, the loading port's
# way in pushed by the hand (no magnet) to the latch, pulled back out, a full tube's knock, the ejection port with the bolt held open, the release (arriving or
# poked, never hovered on), the handle racked or tugged, the bolt as drawn, either hand, the hand or the gun lost, numbers not finite.
add_executable(shellloadtest tools/shellloadtest.cpp src/shellload.cpp)
target_compile_definitions(shellloadtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The asking for the game's own reload of a tube shotgun (src/tubeask.h, header only, as ammo.cpp's StepTube keeps it around each look of shotgunammo.h) on a
# stand-in for the game's count and a clock: the key's 3 s, each kept shell's 1.5 s, a shot ending it (the key held through it asking again only pressed anew),
# the latch for a reload begun on an empty gun, the wait before the game's own shells are given back, random sessions with the game's count never at 0 unasked.
add_executable(tubeasktest tools/tubeasktest.cpp)
target_compile_definitions(tubeasktest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The forearm placed from its hand (src/armik.h, header only): the bind pose, rigid moves, the game's
# own rifle hold from the mesh dumps, the wrist's readings and range, the twist sweep, degenerate
# poses, the time a solve takes, and the stand-in's test hooks.
add_executable(armtest tools/armtest.cpp)
target_compile_definitions(armtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The left hand steering the gun while it holds the handguard (src/steer.h, header only) on scripted
# frames at 72 Hz: both hands swinging it, the left hand pivoting it, wandering off the handguard, the
# hands coming together, letting go, one-frame glitches, the line back while the last steer eases out,
# degenerate inputs, the time an update takes.
add_executable(steertest tools/steertest.cpp)
target_compile_definitions(steertest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Who holds the rifle (src/grasp.h, header only, as hands.cpp runs it) on scripted frames at 72 Hz, every row also run
# with the hands swapped and mirrored: the right hand holding it from a spawn, arming and letting go, the drop, the hold
# on the handguard alone, the pickup and the regrip and their glides, what never takes or drops it (the stand-in, a
# controller unread or untracked, a busy hand, a scripted moment, the game's reload), a new life and the returns, broken
# inputs, the films' timelines (src/freetest.h) and rounds 4-9 on their real places (src/freeplaces.cpp), a squeeze a
# module takes never regripping, the time.
add_executable(grasptest tools/grasptest.cpp src/freeplaces.cpp src/pouch.cpp src/m320.cpp src/handpose.cpp)
target_compile_definitions(grasptest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(grasptest PRIVATE OpenXR::headers)  # pouch.h's XrPosef: the headers only, no loader

# The rules of the game's dropped guns (src/pickrules.h, header only) without the game: the drawn pose where one lies, the countdowns by the
# policy, the view, a request's life, the seqlock, the world into the room, a take landed, the slot in hand, what a swap means to the hands and
# to ammo.
add_executable(picktest tools/picktest.cpp)
target_compile_definitions(picktest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The game's side of its dropped guns (src/pickups.cpp, compiled into the test with every call out of it stubbed) against fake game memory laid
# out as the game's: the checks of its code, the hook, the tick, the walk, a hand's take and each refusal, the counts handed over, the game's own
# take, faults, the policy, pickups.txt. Its pickups.txt is in a folder of its own under the temp folder, never beside the DLL.
add_executable(pickruntest tools/pickruntest.cpp)
target_compile_definitions(pickruntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pickruntest PRIVATE OpenXR::headers)  # hands.h's XR types: the headers only, no loader

# A dropped gun's attachments where it lies (src/pickpartsrules.h, and src/pickparts.cpp compiled into the test with every call out of it
# stubbed) on made-up memory laid out as the game's: the walk of an entry's unlocks, the entry the game draws, the sockets and their objects,
# a part's place, the key, the kinds, what each switch value draws, the parts' lives through the game's builder, setTransform and destructor.
add_executable(pickpartstest tools/pickpartstest.cpp)
target_compile_definitions(pickpartstest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The charging handle and the bolt release by either hand (src/reload.cpp) on scripted frames at 72 Hz, the game's calls
# stubbed: the left alone frame for frame as before (tools/reloadtest_old.inc), the right's grabs, one hand at a time, the
# slaps, a palm withdrawn, a held trigger, broken inputs.
add_executable(reloadtest tools/reloadtest.cpp src/reload.cpp)
target_compile_definitions(reloadtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The right glove as the left's mirror image (src/handpose.cpp): the twin bones, the two tables, the mirror, the right's
# poses and pivots.
add_executable(twintest tools/twintest.cpp src/handpose.cpp)
target_compile_definitions(twintest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Every gun's body when it's let go of, fitted from the game's own first-person meshes (the mesh dumps in build\) into src/gunbodies.h: its frame
# bone, its parked parts left out, its parts drawn apart and its sights added, its middle, at most 22 hull points with a keel; --check compares the
# header with the meshes (writing nothing), --pictures draws each body from four sides.
add_executable(gunbodyfit tools/gunbodyfit.cpp src/handpose.cpp)
target_compile_definitions(gunbodyfit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Every gun's body let go of (src/gunbodies.h, moved by src/loose.cpp) on scripted frames at 72 Hz, the M416's as the reference: dropped near upright
# and on either side, stood on end every way, thrown 200 ways, lying looked away from, the fit's own rules, the SMAW nose up with its case and
# without, the AN-94 with a sight on its side; at rest, none of its mesh's outermost vertices in the ground.
add_executable(bodytest tools/bodytest.cpp src/loose.cpp)
target_compile_definitions(bodytest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Recoil for every gun (src/recoil.h, header only, as recoilrun.cpp runs it) on scripted frames at 72 Hz with fixed seeds: one round and long
# strings of the M416 and others, the frame rates alike, the string's recovery, first shots, one hand against two, the pistol, the sight at the eye
# and the glass kept off it, the clears, the mirror, the aim's fold, nothing not finite, the buzz, the AN-94's pair, the slack, the time.
add_executable(recoiltest tools/recoiltest.cpp)
target_compile_definitions(recoiltest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Recoil's game side (src/recoilrun.cpp, compiled into the test with every call out of it stubbed) against fake game memory laid out as the game's
# data: a gun's kick taken out of its data and put back bit for bit, the checks before any write, a write refused, "off" and "game 1", recoil.txt's
# numbers, the shots and their guards, the grenade launcher's data checked, the eye at the sight and far off it, a switch, the clears. Its
# recoil.txt is in a folder of its own under the temp folder, never beside the DLL.
add_executable(recoilruntest tools/recoilruntest.cpp)
target_compile_definitions(recoilruntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(recoilruntest PRIVATE OpenXR::headers)  # hands.h's and input.h's XR types: the headers only, no loader

# The profile of whatever gun is in hand (src/gunprofile.h, header only, as hands.cpp makes it once a frame): the M416's row today's numbers bit
# for bit, every held gun's, the Stinger's and the SMAW's from their palms, "guns m416", what's left dormant, the names, the time, a change of gun.
add_executable(profiletest tools/profiletest.cpp)
target_compile_definitions(profiletest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Guns never seen before learned (src/gunlearn.cpp) from the mesh dumps and on a software graphics card: each one's frame bone, its parked parts, its
# body against gunbodyfit's, the buffers it reads and refuses, its depth pass, the size of a gun, a saved row, the draw it watches, its table read by
# many threads at once and the locks' order. The graphics card is found at run time: nothing more to link.
add_executable(learntest tools/learntest.cpp src/gunlearn.cpp src/log.cpp)
target_compile_definitions(learntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Pictures of each gun's holds by the other hand from the mesh dumps and the guns' recordings: the left glove on its pistol grip as the game's own
# right hand there mirrored, the right glove on its front grip as the game's left hold mirrored, the fingertips' distances to the gun's shape.
add_executable(holdpic tools/holdpic.cpp src/handpose.cpp)
target_compile_definitions(holdpic PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The Stinger's aerial folded and lifted by the left hand (src/flap.cpp) on scripted frames at 72 Hz: every way as before, and a lift let go of, nothing
# taken, when no hand holds the Stinger.
add_executable(flaptest tools/flaptest.cpp src/flap.cpp)
target_compile_definitions(flaptest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The view cameras' turn to the head (src/viewturn.h, header only, as camera.cpp TurnToHead runs it): the player's
# camera turned bit for bit as before (synthetic and recorded turns), a scripted camera (a cutscene's or a ride's, its
# rows' w lanes filled) turned about its own yaw with the horizon level and its w lanes kept, the camera check, the
# eye shift, mirrored rows, a forward near the vertical, a camera offered again, the w lanes' finding, the time; how
# far a 1-degree turn of a scripted camera turns its level yaw (the review's finding), the drive of Comrades as at
# 833abbd, the yaw going on from frame to frame (spins, pans, cuts, tumbling, a frame's cameras alike); the body's own
# views turned where they're uploaded (skin.cpp's body lock): the math, which uploads are turned, a normal picture's
# left bit for bit, the FOV fix after it as before, the time; and the review's findings: the body at the picture's
# field of view (the knees on the scene as the head turns), a mirrored camera's twin left alone, a frame's build (a
# normal picture drawn behind left bit for bit).
add_executable(viewturntest tools/viewturntest.cpp)
target_compile_definitions(viewturntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The look angles' search rules and a held gun's bullets' line (src/aimrules.h, header only, as aim.cpp and skin.cpp run them): the walk's test and the
# one at one instant, a walk against a turning look, the pass by the pitch alone, the vehicle's turn at the proof, mouse aim's step and its loop, what
# proved itself and waiting for it, a pitch limit learned and tried again on a simulated game, the held line, broken inputs, the time; each rule as it
# stood before this round checked bit for bit against its old expression.
add_executable(aimrulestest tools/aimrulestest.cpp)
target_compile_definitions(aimrulestest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The look angles' row swept from the copies written over (src/aimrules.h's SlotWords, SweepWanted, SweepTakes and SweepChoice, as aim.cpp's Search runs
# them) on a made-up piece of memory with a stand-in for the game's ticks and for the proof: the launch that never found them (three copies reached,
# the slot the game reads 0x300 to 0x880 past them) finds it by the sweep, nothing written but nudges on the look's slots, each undone; the launch
# that worked never sweeps; four a search, the tried left out, the reach's end, broken inputs, the time; the hold's 600 ms, the copies from the answers, a candidate
# of the pass not counted as tried, the round again after five searches, a new soldier, a find.
add_executable(aimsweeptest tools/aimsweeptest.cpp)
target_compile_definitions(aimsweeptest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Holsters (src/holster.h, header only, as hands.cpp runs it) on scripted frames at 72 Hz, with a stand-in for the grasp and the body as the
# pouch places it (src/pouch.cpp): the places against each other, the belly and the user's own squeezes, a slot to its place, putting away and
# drawing by either hand, the game's switch checked and its key pressed again, a hand not seen, the head's turns, a throw, the tick,
# holsters.txt, the lines, broken inputs, the time.
add_executable(holstertest tools/holstertest.cpp src/pouch.cpp)
target_compile_definitions(holstertest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(holstertest PRIVATE OpenXR::headers)  # pouch.h's XrPosef: the headers only, no loader

# input.cpp's and ammo.cpp's rules that need no game (src/inputrules.h) and the controllers' tracking record (src/tracking.h), header only, as input.cpp
# and ammo.cpp run them: a lone release of a mouse button owed to the game (Operation Swordbreaker's roof replayed), the fire button's sends said, a film
# round's real rounds, X held waiting for the game's reload (Rock and a Hard Place's rise, Swordbreaker's refusals), the record's states, changes, caps and
# squeezes, the places' words, the time.
add_executable(inputrulestest tools/inputrulestest.cpp)
target_compile_definitions(inputrulestest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The game's quick-time events on the grips (src/qte.h, header only, as input.cpp runs it) on scripted frames at 72 Hz: nothing held in ordinary play,
# a fresh squeeze of either grip in a scripted moment holding the six inputs till it's let go, a grip held as the moment began pressing nothing, everything
# let go the frame the moment ends, a flick's 120 ms, the game's turn of the look with and without a gun in hand, bf3 not in front, "qte 0", a grip not
# read, a clock going back, the time. Version 2 (qte.h's Step2 and src/qtelist.h, the reader of the game's list of live input triggers, on a memory
# image made up in the test): exactly the awaited keys, a tap's 70 ms and the mash every 130 ms, held keys, everything let go at every end, play's own
# Fire, Jump and Interact only in a scripted moment, the list not found or not read pressing nothing; the reader's entities, adds and removes not yet
# applied, wrong vtables, no manager, an entity listed again and again, bounds that make no sense; the signature's search; the train's three events.
add_executable(qtetest tools/qtetest.cpp)
target_compile_definitions(qtetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# A held gun's magazine's seat with bare arms (src/seatrest.h, header only, as skin.cpp runs it): with gloves nothing changes; the magazine's rest on its
# frame; the game's own reloads of the AKS-74u and the UMP-45 recorded on the train replayed (build\survey, SKIP without them): no seat is ever taken
# from a magazine on its way.
add_executable(seatresttest tools/seatresttest.cpp)
target_compile_definitions(seatresttest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Blackburn's bare arms by their draws (src/barestate.h, header only, as skin.cpp's draw hook asks it): handcuffed and with the cuffs off (the same arm
# mesh from another start, the body's draws moved) both taken; everywhere else the answer is the old table's; no draw recorded on a level with gloves
# (tools/barestatetest_draws.inc) is taken; the recordings themselves read (build\survey, SKIP without them).
add_executable(barestatetest tools/barestatetest.cpp)
target_compile_definitions(barestatetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Blackburn's bare arms with a gun in hand as the gloves under another numbering (src/barehold.h, header only, as skin.cpp's DrawBareArms runs it): the
# table there and back, the forearms from the arm solved from the hand as drawn (or whole with it), a hand folded away, the fitted hands (src/magposes) on
# the bare left hand; the train's recordings (build\survey, SKIP without them): the rig can learn the game's hold from the bare draw, and placed as the
# rig places the gloves the bare hands are on the held gun as the game has them on its own.
add_executable(bareholdtest tools/bareholdtest.cpp src/handpose.cpp)
target_compile_definitions(bareholdtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Read only: the running game's list of live input triggers through that same reader, from outside (ReadProcessMemory): the signature's finds, the
# place the manager's vtable picks, the list and the awaited keys, the game's input; "qtepeek 120" writes every change for two minutes.
add_executable(qtepeek tools/qtepeek.cpp)
target_compile_definitions(qtepeek PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The head's place moving the game's cameras (src/headmove.h, header only, as views.cpp runs it) on scripted heads and
# a simulated soldier: leaning, the limit, the neck deciding, walking under the head, walls, crouching without dropping
# twice, the turn about the head, cutscenes, tracking lost and found, the time.
add_executable(headmovetest tools/headmovetest.cpp)
target_compile_definitions(headmovetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The head replay and its meters (src/headtest.cpp, as xr_runtime.cpp runs it) with no game: the angles there and back and against the view turn
# (src/viewturn.h), the slide split into yaw and pitch, a sine, a burst, a track and its loop, the eyes on their pivot, the predictor, the ease in and
# out, headtest.txt, a frame line read back, the noise, the time. With "--synth <file>" it writes a made-up run for tools\headfit.ps1 -SelfTest;
# with "--track <file>" it says how the replay reads and plays a track file.
add_executable(headtesttest tools/headtesttest.cpp src/headtest.cpp)
target_compile_definitions(headtesttest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The frame's memory (src/framemem.h's logic and src/framemem.cpp, compiled into the test with every call out of it stubbed: MinHook, the game's
# allocator, the system's memory) against a made-up shader system, settings and block laid out as the game's: the sizes to try, the meter and the
# net, the game's code not as read, the hooks, the block made bigger at the first frame's end, the lines, no piece to be had, settings waiting to be
# taken, the block made anew, odd blocks, the crash record's line, the time.
add_executable(framememtest tools/framememtest.cpp)
target_compile_definitions(framememtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(framememtest PRIVATE ${minhook_SOURCE_DIR}/include)  # MinHook's header only: the test has a MinHook of its own

# Where the game's threads spend their time, sampled from outside the game.
add_executable(sampler tools/sampler.cpp)
target_compile_definitions(sampler PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The player's look angles found from outside the game (pairs matching the game camera).
add_executable(anglescan tools/anglescan.cpp)
target_compile_definitions(anglescan PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Records what the PC plays and cuts clips from it: the game's own reload sounds for the mod.
add_executable(capture tools/capture.cpp)
target_compile_definitions(capture PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(capture PRIVATE ole32)

# The mod's reload sounds played outside the game.
add_executable(playtest tools/playtest.cpp src/sounds.cpp)
target_compile_definitions(playtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(playtest PRIVATE xaudio2 ole32)

# The mod's own making of its sounds at its start (sounds::MakeMissing, as dllmain.cpp runs it) without the game and without a sound
# played: every clip there, a newer version's new ones missing after an update in place (only those made, the rest never touched),
# none at all, gamesounds.exe gone; with "--game <folder>" the real thing from the game's own files.
add_executable(soundsmaketest tools/soundsmaketest.cpp src/sounds.cpp)
target_compile_definitions(soundsmaketest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(soundsmaketest PRIVATE xaudio2 ole32)
add_dependencies(soundsmaketest gamesounds)

add_executable(injector injector/main.cpp)
target_compile_definitions(injector PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# The campaign-only rule (src/campaign.h, header only, as the injector and dllmain.cpp run it) without the game: Battlelog's
# campaign start against multiplayer and co-op starts and broken lines, the split against the system's own, a running process's
# command line read as the injector reads it (injector/cmdline.h: its 32-bit PEB, and by asking), the injector itself with
# "--injector <path>" (its --judge, nothing loaded), and SteamVR's 32-bit OpenXR runtime preferred (src/xrpick.h).
add_executable(campaigntest tools/campaigntest.cpp)
target_compile_definitions(campaigntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(campaigntest PRIVATE shell32)

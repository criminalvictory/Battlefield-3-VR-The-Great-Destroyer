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
    src/mountseat.cpp
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
    src/fpbatch.cpp
    src/loose.cpp
    src/gunhost.cpp
    src/ground.cpp
    src/pickups.cpp
    src/pickparts.cpp
    src/m320.cpp
    src/shellload.cpp
    src/rocket.cpp
    src/pistolmag.cpp
    src/pistolslide.cpp
    src/overlay.cpp
    src/markers.cpp
    src/skyrays.cpp
    src/shaders.cpp
    src/postpin.cpp
    src/streamview.cpp
    src/headmove.cpp
    src/recoilrun.cpp
    src/headtest.cpp
    src/log.cpp
    src/launchreport.cpp
    src/keybind.cpp)
target_compile_definitions(bf3vr PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(bf3vr PRIVATE openxr_loader minhook d3d11 dxgi d3dcompiler xaudio2 ole32)

# The commit the DLL is built from, named at every build (tools/buildinfo.cmake writes build\generated\buildinfo.h only when it changed): the DLL's
# version resource (src/bf3vr.rc) and its first log line carry it, and the launcher and the report read it back from the DLL (never git's latest
# commit: the source can be newer than the build).
find_package(Git QUIET)
set(BF3VR_GENERATED ${CMAKE_BINARY_DIR}/generated)
add_custom_target(buildinfo
    COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_SOURCE_DIR} -DOUT=${BF3VR_GENERATED}/buildinfo.h -DGIT=${GIT_EXECUTABLE}
            -P ${CMAKE_SOURCE_DIR}/tools/buildinfo.cmake
    BYPRODUCTS ${BF3VR_GENERATED}/buildinfo.h
    COMMENT "Naming the commit the mod is built from"
    VERBATIM)
target_sources(bf3vr PRIVATE src/bf3vr.rc)
target_include_directories(bf3vr PRIVATE ${BF3VR_GENERATED})
set_source_files_properties(src/bf3vr.rc src/dllmain.cpp PROPERTIES OBJECT_DEPENDS ${BF3VR_GENERATED}/buildinfo.h)
add_dependencies(bf3vr buildinfo)

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
add_executable(magfit tools/magfit.cpp src/handpose.cpp src/gunshape.cpp src/log.cpp)
target_compile_definitions(magfit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The way a box drawn apart (the M249's pouch) comes off and goes on in its gun's own reload, from that reload's recording, for src/magposes/<gun>_way.h
# (handmag::BoxWay).
add_executable(boxway tools/boxway.cpp)
target_compile_definitions(boxway PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

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

# The owner's rule of 2026-11-19 (src/workhand.h: while one hand holds a gun, by its pistol grip or its front alone, the other works every part it has,
# either hand in either role) through the very code hands.cpp runs, on scripted frames at 72 Hz: grasp.h, workhand.h, handmag.cpp by either hand (every
# held gun's lever over and under, its magazine or box, lid and belt; the right's the left's mirror image across the gun's middle; a hand changed;
# Claims against Update), shellload.cpp's handle; the owner's M249 and M240 end to end (the left lays the belt, holds the front, the right pulls the
# handle, takes its grip back and fires) and mirrored for a lefty, the PKP's too; every held gun's lever with the gun held by its front alone by either
# hand; the Saiga 12K's magazine by either hand with the other on its front (out, home, the flick, the handle).
add_executable(otherhandtest tools/otherhandtest.cpp src/handmag.cpp src/pouch.cpp src/shellload.cpp src/handpose.cpp)
target_compile_definitions(otherhandtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(otherhandtest PRIVATE OpenXR::headers)

# The fingers on a lever or a charging handle (weapon.txt heldtest 3 and 4's scripted hand, src/heldtest.h, through handmag.cpp and the fitted hands, the
# glove and fingers drawn as hands.cpp draws the left and the right) for every gun with a lever, over the top and from under it, either hand, every frame
# of the pull at 72 Hz: the glove a turn, each finger bone its own length, none stretched (the proof of 2026-11-25: the Mk11's from under drawn as rods);
# the fingers eased out from the table they were on as the hand lets go (handmag::EaseFingers, as hands.cpp has it: the review of 84654d7, a lever let go
# of had eased out from the magazine's). "leverfingerstest old" runs the wrist as it was, "leverfingerstest oldletgo" the let-go as it was: both fail.
add_executable(leverfingerstest tools/leverfingerstest.cpp src/handmag.cpp src/pouch.cpp src/handpose.cpp)
target_compile_definitions(leverfingerstest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(leverfingerstest PRIVATE OpenXR::headers)

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

# Hosting a gun that lies (src/hostrules.h, src/gunhost.cpp): the pure rules, and the graphics part on a software graphics card (WARP): a lying gun's
# draws kept as the mod draws them, then drawn after another gun's in each pass, the same pixels as the game's own, every state put back.
add_executable(hosttest tools/hosttest.cpp src/gunhost.cpp src/loose.cpp)
target_compile_definitions(hosttest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(hosttest PRIVATE d3d11 d3dcompiler)

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

# The view through a scope's frame-rate guard (src/scopegov.h, header only): seconds under the floor with the view on hold it back, longer each
# time soon after; the lens's own tests for turning the view on (its line, its size, in front of the eye); the view's own switch a dispatch at a time
# (back on soon after it went off with an eyepiece pressed close); weapon.txt's scopegov line taken away, or the file deleted and put back with its
# old write time (skin.cpp's reader, word for word).
add_executable(scopegovtest tools/scopegovtest.cpp)
target_compile_definitions(scopegovtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The scans of the game's memory for a gun's data (src/scanthread.h, as skin.cpp's four scans run it): a scan's thread in background mode through its
# walk alone, its own priorities again at Foreground and at its end; each of skin.cpp's scans' walks followed at once by Foreground, nothing logged,
# locked, added or written in background mode (word for word, run from the project's folder).
add_executable(scantest tools/scantest.cpp)
target_compile_definitions(scantest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

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
# numbers, the shots and their guards, the grenade launcher's data checked, the eye at the sight and far off it, a switch, the clears, a change of
# gun under way (the slot's gun by its name never touched under the drawn gun's). Its recoil.txt is in a folder of its own under the temp folder,
# never beside the DLL.
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

# A scripted scene's straight ahead (src/sceneview.h, header only, with src/viewturn.h's turn, as camera.cpp and xr_runtime.cpp run them): the head's
# yaw, the turn bit for bit as before at the room's own, the owner's breach of 2026-11-26 (its forward shown at his face from the first picture, the
# head about after it), the scene's end (no jump), a ride's one latch, the scenes' numbering (Comrades' drive, the interrogation's retries, Semper
# Fidelis' death and checkpoint, a recenter, the clock), nothing in play, mirrored cameras, the head's place, the time.
add_executable(sceneviewtest tools/sceneviewtest.cpp)
target_compile_definitions(sceneviewtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The look angles' search rules and a held gun's bullets' line (src/aimrules.h, header only, as aim.cpp and skin.cpp run them): the walk's test and the
# one at one instant, a walk against a turning look, the pass by the pitch alone, the vehicle's turn at the proof, mouse aim's step and its loop, what
# proved itself and waiting for it, a pitch limit learned and tried again on a simulated game, the held line, broken inputs, the time; each rule as it
# stood before this round checked bit for bit against its old expression; mouse aim's view held off level, straight ahead and the box where memory is
# full of pairs like the look (the logged searches "too many" and "straight ahead", a million random aims, its loop from a load's level view), the gun
# kept with what proved itself (the user's switch to the AT4 of 2026-11-19: no 15 s wait), the next search sooner after a loss, the gun a proof counts
# for taken as the proof began and ended (the user's switch to the Mk11 landing mid-search, 2026-11-19 01:09:12: the Mk11's, not the M4A1's); mouse
# aim's hold never while a round can fly and the box's side kept through a hand's tremor at level (the review of d32a39b: the bridge M249's burst on the
# gun's line, not 6 degrees off; no flip in 600 frames, 67 as it was).
add_executable(aimrulestest tools/aimrulestest.cpp)
target_compile_definitions(aimrulestest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The look angles' row swept from the copies written over (src/aimrules.h's SlotWords, SweepWanted, SweepTakes and SweepChoice, as aim.cpp's Search runs
# them) on a made-up piece of memory with a stand-in for the game's ticks and for the proof: the launch that never found them (three copies reached,
# the slot the game reads 0x300 to 0x880 past them) finds it by the sweep, nothing written but nudges on the look's slots, each undone; the launch
# that worked never sweeps; four a search, the tried left out, the reach's end, broken inputs, the time; the hold's 600 ms, the copies from the answers, a candidate
# of the pass not counted as tried, the round again after five searches, a new soldier, a find. And the reach (ReachKind, ReachScan, ReachSpans, ReachThrough,
# ReachChoice, ReachPointerAlone) on a sparse memory megabytes across with VirtualQuery's regions and allocations, a working set and a clock: the M249's
# checkpoint of 2026-11-18 (its look 0x1580 from the copies: nothing in 43 searches as it was, the first search now), the user's play of 2026-11-19 (the
# M4A1's look 1.9 MB away, the M249's 3.4 MB, two arenas), a new life about a look proved before, a look without the row's shape only where its copies lack
# it (the .50 cal run), crowds never nudged (every place counted, 17 of a kind kept), AMD's write-combined memory, threads' stacks and the rest never read,
# memory slower than RAM left, pages out of the working set never read, rings nearest first, 64 MB and 25 ms at most; and src/aimmemory.h (aim.cpp's
# reads of the game's memory) on this program's own: 32 MB's time, a page put out of the working set, stacks with and without a guard page; each piece
# timed by the reading thread's own cycles (two 15 ms stalls in a row never strike plain RAM: the review of d32a39b) and a strike kept for its life only.
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
# squeezes, the places' words, the time; Page Up and Page Down (xr_runtime.cpp's ViewKeyStep: a step only from a key held half a second with the headset
# on, the knocked keyboard of 2026-11-18 replayed, the session focused only while frames are begun, a gap in the frames, both keys at once) and the
# developer's keys' words in what ships (from the project's folder).
add_executable(inputrulestest tools/inputrulestest.cpp)
target_compile_definitions(inputrulestest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The right stick's crouch and prone (src/stance.h, header only, as input.cpp runs it) on the game's own pose (src/poseread.h, its reader, on a memory
# image made up in the test) against the game's rules as read in bf3.exe 1.6.0.0, at 72 Hz with the game's side at its 30: the owner's sequence of
# 2026-11-19 ("if I flick up, I crouch"), a death while prone, the RPG checkpoint's prone soldier, prone from a crouch, the game's own stand-ups, a
# change during a change, scripted locks, no room to stand, the head's crouch, a fight's prompt and a cutscene; each the old way (the belief alone:
# shown going wrong), with the read and without; the reader's checks; its proofs on a made-up image and against build\bf3_mem.exe (SKIP without it).
add_executable(stancetest tools/stancetest.cpp)
target_compile_definitions(stancetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The mounted gun (src/mountgun.h and src/seatread.h, header only, as src/mountseat.cpp runs them; docs/research/mounted_gun.md), without the game: its
# rows (every one unmeasured today, Ready naming the first missing for each need and why; a yes or no measured as no and a row half filled refused; the
# law's rows from the gun's rows alone; the game's invert flipping the pitch's sign), the seat's states (only a gun's seat told by measured rows is ever
# one; Rock's ride and every other seat as before; a read missed a moment held; a steering stop putting the seat back as before), the quiet list and the
# stick's routing as input.cpp runs it, the handle law with its takes kept in the holding frame (the hands' line, or the body's; both guns' shapes, the
# truck's post and the Humvee's ring; a lost hand's place moving nothing), the stick's orbit and its eye's speed, the target's smoothing, the steering
# against a simulated turret (rate input, momentum, a 30 Hz tick, its delay, the pad's dead zones and curve or the mouse's counts: converging without
# overshoot, saturating cleanly, nothing sent when it mustn't, a wrong sign big or small and no answer stopped whatever the push's size, a read that
# doesn't follow the turret stopped, a stop met short of its row found, no false stop with the rows off, the strength learned only in steady input), the
# fire rule, the seat's reader on a made-up memory image (its proofs refused while unmeasured); and the broken laws the reviews found, each shown failing.
add_executable(mountguntest tools/mountguntest.cpp)
target_compile_definitions(mountguntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The game's quick-time events on the grips (src/qte.h, header only, as input.cpp runs it) on scripted frames at 72 Hz: nothing held in ordinary play,
# a fresh squeeze of either grip in a scripted moment holding the six inputs till it's let go, a grip held as the moment began pressing nothing, everything
# let go the frame the moment ends, a flick's 120 ms, the game's turn of the look with and without a gun in hand, bf3 not in front, "qte 0", a grip not
# read, a clock going back, the time. Version 2 (qte.h's Step2 and src/qtelist.h, the reader of the game's list of live input triggers, on a memory
# image made up in the test): exactly the awaited keys, a tap's 70 ms and the mash every 130 ms, held keys, everything let go at every end, play's own
# Fire, Jump and Interact only in a scripted moment, the list not found or not read pressing nothing; the reader's entities, adds and removes not yet
# applied, wrong vtables, no manager, an entity listed again and again, bounds that make no sense; the signature's search; the train's three events.
add_executable(qtetest tools/qtetest.cpp)
target_compile_definitions(qtetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# A held gun's magazine's seat with bare arms (src/seatrest.h, header only, as skin.cpp runs it): with gloves and the palms known nothing changes, with
# gloves and no palms only a seat not known is taken at rest; the magazine's rest on its frame; the game's own reloads of the AKS-74u and the UMP-45
# recorded on the train replayed (build\survey, SKIP without them): no seat is ever taken from a magazine on its way; the Mk11's and the M4A1's under
# the gloves' rule: their seats taken once, the gun's, and none replaced in the reload of the gun empty.
add_executable(seatresttest tools/seatresttest.cpp)
target_compile_definitions(seatresttest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# A magazine in a hand drawn after a death, on every gun the hands reload (src/inputrules.h's stance at a new soldier and its HandsMovedByStance,
# src/heldpalms.h, src/seatrest.h's Take and Change; header only, as input.cpp and skin.cpp run them): the owner's Operation Swordbreaker of 2026-11-19
# replayed at 72 Hz from its log's lines (prone at a death, the next life's Mk11 with no magazine drawn in the hand), with the rules as they were (the log
# again, to the 10 s windows and each draw's palms) and as they are (every frame drawn); thirteen guns drawn the first time prone, sprinting, standing,
# again after another gun and after a death; what was right kept (bare arms, the game's own reload, the palms' rule, the seats kept); the game's own
# reload while the stance or a sprint moves its hands, cut by a change of gun or a death: the seat kept is the gun's own (its review's case).
add_executable(magseattest tools/magseattest.cpp)
target_compile_definitions(magseattest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The Mk11 Mod 0 and its rifle scope (Operation Swordbreaker) held to what was measured: skin.cpp's rows for it (copied) against its mesh dumps and its
# recordings (build\mesh_12456_2214_* and the rest, build\survey\MK11_*.bin; SKIP without them): its magazine, T handle, bolt catch, muzzle and port
# on the mesh; its scope's eyepiece, the zoomed eye on its line, the power, the lens big enough for the second renderer's picture; its reticle
# (scopemarks.h's kRifleScope) made again from the dumps, centred, its dots even; its depth pass's and parts' bones in every recorded frame; its own
# reload's magazine straight down the well; the hand at the bolt catch; its eleven rounds' kick as its recoil row has it.
add_executable(mk11test tools/mk11test.cpp src/handpose.cpp)
target_compile_definitions(mk11test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The AT4 (Operation Swordbreaker's M136) held to what was measured, without the game: its row in each table (recoil, body, shape, profile, skin.cpp's
# word for word); its mesh's bones and front ring; its sights (the rear aperture's ring, the front post's tip: their line 0.47 degrees under and 0.68
# right of its tube, the game's zoomed eye on it); the game's palms in its recording; the sights' turn as hands.cpp places it (bit for bit the raise for
# every other gun); a hand's use of a scripted prompt by reach and grip (src/usepoint.h); the quick-time guard with a launcher in a hand, one rule with
# the fist fights' guard (src/qtewatch.h's Bar); its holster's place beside the Mk11; the game's use's code against bf3_mem.exe; its rocket's start
# and the SMAW's each in its own firing data alone (src/at4shot.h, the roof's dump); the take's side effects on the held magazine (src/pickrules.h).
add_executable(at4test tools/at4test.cpp src/handpose.cpp)
target_compile_definitions(at4test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The handles drawn apart with the hand (src/handleruns.h, header only, as skin.cpp draws them: the Mk11's T handle, with no bone of its own) against the
# mesh dump they were read from (build\mesh_12456_2214_*, whose buffers hold its depth pass too; SKIP without it): its runs the T and its latch whole, on
# the frame alone, sharing no corner with the rest; the depth pass's runs the same triangles; the body less its runs and its runs every index once; the
# shaft's four faces made from the stem's own top, a closed box from the pulled stem home, lit as the top is.
add_executable(handleruntest tools/handleruntest.cpp)
target_compile_definitions(handleruntest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The first-person batches probe (weapon.txt "fpbatchlog 1"; src/fpbatchrules.h and the hook's stub src/fpbatchstub.h, header only, as
# fpbatch.cpp runs them), without the game: every place of the game's code it goes by against build\bf3_mem.exe (SKIP without it) and the
# calls' targets and returns made from them; a call read from a made-up frame, view and batch, and each way of it not being the batch
# camera's frame refused alone; the origin's pair check; views by name and kind; per-view uploads matched to a batch's camera (near,
# far, rows off, a camera of frames before; two kinds of batch with one camera shared, never the first's; only the cameras a batch's
# per-view block is made from, as the game tests flag 4 and the origin); the draws kept, shared ones apart; the books full; the log's
# lines; the names; the pages never read; the stub on a stand-in setter (the two calls noted with EBX, EBP and the stack, every other
# passed straight on, the registers and the stack kept); the time.
add_executable(fpbatchtest tools/fpbatchtest.cpp)
target_compile_definitions(fpbatchtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The M249 (Operation Swordbreaker), offline: skin.cpp's own rows for it (src/m249rows.h) against its mesh dumps (build\mesh_24291_2808_*, 2808_0,
# 987_27099, the gloves' 13755_15051), its shape (build\guns\m249.bin) and its recordings (build\survey\M249_*.bin, M240_*.bin), skipped without them;
# its fitted hands against its meshes; its belt reload by hand through handmag on its own table, its box along its own reload's way; what rides its cover.
add_executable(m249test tools/m249test.cpp src/handmag.cpp src/pouch.cpp src/handpose.cpp src/gunshape.cpp src/log.cpp)
target_compile_definitions(m249test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(m249test PRIVATE OpenXR::headers)

# The bridge M249's ACOG (the M416's own, on Operation Swordbreaker's overpass), offline: its row's numbers (src/acog.h), the feed cover's mount it rides
# (m249rows::kSightMount, lidride::OnMount), its marks (scopemarks::kAcog), its corners (gunbodies' kAttach) against today's dumps of its draws and its
# zoomed model, the 11-05 dumps kAcog was made from, the game's own reload with it on and the bare M249's recordings (build, build\survey; SKIP without
# them); its line: the lens along the gun's sights' line, its bullets' start on it, its chevron over it; whose its six draws are, frame by frame (the
# held gun's sight on whichever held gun carries it, the M249's or the M4A1's after Guillotine's bridge; the M416's own from its first frame back).
add_executable(m249acogtest tools/m249acogtest.cpp src/handpose.cpp)
target_compile_definitions(m249acogtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The Saiga 12K (Operation Swordbreaker), offline: skin.cpp's own rows for it (src/saiga12krows.h) against its body's mesh dump (build\mesh_14970_0_*),
# its magazine's own three draws (a mesh of its own: build\mesh_1176_0_*, 450_1176_*, 1626_1626_*, made by gamesounds --mesh) at its seat in the
# receiver's opening, its recordings (build\survey\SAIGA12K_*), its tables (its recoil row and kick.tsv's, its body, its shape's recipe, its sound cuts),
# its fitted hands against its meshes; skipped without them. Its reload by hand through handmag is heldmagtest's.
add_executable(saiga12ktest tools/saiga12ktest.cpp src/handpose.cpp src/loose.cpp)
target_compile_definitions(saiga12ktest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(saiga12ktest PRIVATE OpenXR::headers)

# The PKP Pecheneg (Operation Swordbreaker), offline: skin.cpp's own rows for it (src/pkprows.h) against its mesh dumps (build\mesh_23298_2310_*, 2310_0, the
# gloves' 13755_15051), its shape (build\guns\pkp.bin) and its recordings (build\survey\PKP_*.bin), skipped without them; its fitted hands (each its own
# reload's right hand mirrored onto the left) against its meshes and its shape; its belt reload by hand through handmag on its own table, its box along its
# own reload's way to the gun's right, its belt numbered from the box; its cover's four bones; its clips in the game's own sounds' table.
add_executable(pkptest tools/pkptest.cpp src/handmag.cpp src/pouch.cpp src/handpose.cpp src/gunshape.cpp src/log.cpp src/loose.cpp)
target_compile_definitions(pkptest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pkptest PRIVATE OpenXR::headers)

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

# Blackburn's arms whole up to the shoulders on the bare arms' levels (src/barestate.h's parts, src/barehold.h's sleeve tables, src/armik.h's solver and
# collar, header only, as skin.cpp's DrawBareSleeves and hands.cpp's PlaceBareForearms run them): the torso's and the strip's draws in both cuff states,
# none on a level with gloves, a sleeve only on an arm solved whole; with the mesh dump and the train's recordings (build, build\survey: SKIP without
# them) the torso, sleeves and strip posed from the game's own frames join the forearm at the elbow, no bone stretched, the arms' open end inside the sleeve.
add_executable(baresleevetest tools/baresleevetest.cpp)
target_compile_definitions(baresleevetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The player's own key bindings (src/keybind.h, header only, as keybind.cpp and input.cpp's SendKey use it) without the game: the game's default
# bindings as its settings file has them giving every one of today's keys, no file or no key bindings giving them too, a player's keys followed (the
# walking keys by their axes' direction, the arrows and the right Ctrl with their extended flag), the keys the mod never sends (its own, Escape,
# the F keys, Windows', media and power keys), an action on the mouse or the pad alone or unbound pressing nothing, the quick-time actions' own,
# broken lines, the log's words; this PC's own settings file read when it's there (SKIP without it; shared with the game's saves); the fights' guard
# by the key that goes (use moved onto F held up at an E prompt: src/qtefight.h's KeysOfQuickTime); the watch's looks (a read that fails or finds
# the file empty keeps the player's keys and reads again); the reload key's Windows key code by its scan code (ammo.cpp).
add_executable(keybindtest tools/keybindtest.cpp)
target_compile_definitions(keybindtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The game's pad read with its window in the background (src/globalpad.h, header only, as pad.cpp runs it) on a memory image made up in the test:
# its proofs (the field record, ClientSettings' type and vtable, the four places that read it), each broken alone, at another base too; a look at the
# setting (not kept yet, not a ClientSettings, not a bool, off, on); against build\bf3_mem.exe (SKIP without it): every proof borne out, the field
# among ClientSettings' own, the import the first three places call USER32's GetFocus.
add_executable(globalpadtest tools/globalpadtest.cpp)
target_compile_definitions(globalpadtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The other controllers' OpenXR profiles (src/xrprofiles.h, header only, as input.cpp suggests them): every path against the OpenXR specification's
# list for its profile and hand, each action's input of its type, the Touch's layout as input.cpp's own, every control there on each (no D-pad or
# View alone), the grips' buttons, the extensions; the Vive wands' right trackpad counted only clicked away from its middle, a turn held at its
# edge never a recenter.
add_executable(xrprofilestest tools/xrprofilestest.cpp)
target_compile_definitions(xrprofilestest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# The VR runtime's own recenter followed (src/spacechange.h, header only, as xr_runtime.cpp runs it) on scripted frames at 72 Hz: LOCAL's change
# recentered once its time has come, other spaces' left alone, a time never reached, a clock gone back, two changes, a runtime that keeps sending them,
# bf3vr.ini's follow_recenter=0 following none.
add_executable(spacechangetest tools/spacechangetest.cpp)
target_compile_definitions(spacechangetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Other programs in the game's folder (src/conflicts.h, header only, as launchreport.cpp runs it): the game's own folder giving nothing, ReShade,
# 3DMigoto, DXVK, ENB, Special K, SweetFX, an ASI loader, x360ce and any other proxy DLL each named with its DLLs, a program's files without a DLL
# giving nothing, each DLL named once, names in any case.
add_executable(conflictstest tools/conflictstest.cpp)
target_compile_definitions(conflictstest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

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

# Each eye's own passes without the game (src/eyefx.h and src/markermath.h, header only, as overlay.cpp, shaders.cpp, skyrays.cpp and markers.cpp run
# them): the HUD's depth in pixels and its place, the world markers' matrix taken apart and rebuilt for the head in both layouts, the sky's shaders known by
# their bytecode (with build\ps: all of the sky's and none else), the mod's sky vertex shader run on a software graphics card (WARP) against the rays
# worked out on the CPU, centred and off-centre, and the hooked code's first bytes against build\bf3_mem.exe (SKIP without them). And src/skyrays.cpp
# itself (the game's side stubbed) on WARP: which sky draws it takes (an eye's own picture only, the depth off) and the game's state put back after.
add_executable(eyefxtest tools/eyefxtest.cpp src/skyrays.cpp)
target_compile_definitions(eyefxtest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(eyefxtest PRIVATE d3d11 d3dcompiler)

# The overlay's routing (src/overlay.cpp, compiled into the test with the game's side stubbed) on a software graphics card (WARP), a made-up frame drawn
# through it: the eyes' pictures, the depth of field's composites with the window's and an eye's viewport, the first view's effects matched and alone,
# the HUD at infinity and at a depth, scaled, clipped to its eye, the pieces reading an eye's picture, the held effects; each picture read back.
add_executable(overlaytest tools/overlaytest.cpp src/overlay.cpp)
target_compile_definitions(overlaytest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(overlaytest PRIVATE d3d11 d3dcompiler dxgi)

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

# The DLL's own build read back (tools/buildinfo.cmake, src/bf3vr.rc): build\bf3vr.dll's version resource names a commit as the launcher and the report
# read it, and it is this build's (build\generated\buildinfo.h, written in the same build; SKIP for a DLL older than the tests, built while it was locked).
add_executable(buildinfotest tools/buildinfotest.cpp)
target_compile_definitions(buildinfotest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_include_directories(buildinfotest PRIVATE ${BF3VR_GENERATED})
target_link_libraries(buildinfotest PRIVATE version)
set_source_files_properties(tools/buildinfotest.cpp PROPERTIES OBJECT_DEPENDS ${BF3VR_GENERATED}/buildinfo.h)
add_dependencies(buildinfotest buildinfo)

# The player's report (tools/report.ps1) without the game: its pieces out of it by PowerShell's parser, run in Windows PowerShell on a made-up log under
# %TEMP% (never build\): every hosting line the mod writes (src\*.cpp's Log("Host: ...")) one of its key lines, the word elsewhere not, the key lines'
# section from a long log holding each hosting kind once with its count, and from a log of many kinds its first 50 and last 150. From the project's
# folder.
add_executable(reporttest tools/reporttest.cpp)
target_compile_definitions(reporttest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# The release's played settings (tools/package.ps1's bf3vr.ini) without packing: its DevKeysLine and ShippedIni out of it by PowerShell's parser, run in
# Windows PowerShell on made-up lines: the comment on dev_keys shipped as src\config.cpp's (a played bf3vr.ini's old line named Page Up / Page Down),
# the rest as before, release\settings\bf3vr.ini unchanged. From the project's folder.
add_executable(packagetest tools/packagetest.cpp)
target_compile_definitions(packagetest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

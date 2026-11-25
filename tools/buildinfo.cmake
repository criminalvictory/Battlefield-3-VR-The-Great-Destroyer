# Writes the build's own name (OUT: build\generated\buildinfo.h) at every build, run by CMakeLists.txt's buildinfo target: the commit the mod is built from
# (SOURCE: the source's folder), the commit's date, and whether the source had changes not committed. bf3vr.dll carries it in its version resource
# (src/bf3vr.rc: its ProductVersion) and its first log line, and the launcher and the report read it back from the DLL itself (tools\launch_vr.ps1,
# tools\report.ps1): git's latest commit is not the DLL's when the source was changed after the build (pulled, not built again). The file is written only
# when what it says changed, so nothing is built again for nothing. No git (a source zip): "not known". Nothing else: no path, no name (the package's
# -Forbid check reads every file).
#   cmake -DSOURCE=<folder> -DOUT=<file> [-DGIT=<git.exe>] -P tools/buildinfo.cmake
if(NOT GIT)
    set(GIT git)
endif()
set(hash "")
set(date "")
set(dirty 0)
execute_process(COMMAND "${GIT}" -C "${SOURCE}" rev-parse HEAD
                OUTPUT_VARIABLE hash ERROR_QUIET RESULT_VARIABLE failed OUTPUT_STRIP_TRAILING_WHITESPACE)
if(failed OR NOT hash MATCHES "^[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]+$")
    set(hash "")
endif()
if(hash)
    execute_process(COMMAND "${GIT}" -C "${SOURCE}" show -s --format=%cd --date=format:%Y-%m-%d HEAD
                    OUTPUT_VARIABLE date ERROR_QUIET RESULT_VARIABLE failed OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(failed OR NOT date MATCHES "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$")
        set(date "")
    endif()
    # (No optional locks: a build never writes git's index, whatever else runs git on this source.)
    execute_process(COMMAND "${GIT}" --no-optional-locks -C "${SOURCE}" status --porcelain --untracked-files=no
                    OUTPUT_VARIABLE changes ERROR_QUIET RESULT_VARIABLE failed OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT failed AND changes)
        set(dirty 1)
    endif()
endif()
if(hash)
    string(SUBSTRING "${hash}" 0 7 short)
    set(text "commit ${hash}")
    if(date)
        string(APPEND text " of ${date}")
    endif()
    if(dirty)
        string(APPEND text ", with changes not committed")
    endif()
else()
    set(short "")
    set(text "commit not known (no git where it was built)")
endif()
set(content "// Written by tools/buildinfo.cmake at every build (CMakeLists.txt's buildinfo target): never by hand, never committed.
#ifndef BF3VR_BUILDINFO_H
#define BF3VR_BUILDINFO_H
#define BF3VR_COMMIT \"${hash}\"
#define BF3VR_COMMIT_SHORT \"${short}\"
#define BF3VR_COMMIT_DATE \"${date}\"
#define BF3VR_COMMIT_DIRTY ${dirty}
#define BF3VR_BUILD_TEXT \"${text}\"
#endif
")
set(old "")
if(EXISTS "${OUT}")
    file(READ "${OUT}" old)
endif()
if(NOT old STREQUAL content)
    file(WRITE "${OUT}" "${content}")
endif()

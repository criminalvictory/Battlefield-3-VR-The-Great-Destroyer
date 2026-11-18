@echo off
rem Battlefield 3 VR (pre-alpha) through Virtual Desktop's own OpenXR runtime (VDXR) instead of SteamVR (experimental): for a
rem headset on Virtual Desktop. SteamVR isn't started (one running is closed). Otherwise the same as "Start BF3 VR.bat".
title BF3 VR launcher (VDXR)
if not exist "%~dp0tools\launch_vr.ps1" goto unzipped
if not exist "%~dp0build\injector.exe" goto unzipped
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\launch_vr.ps1" -VDXR %*
exit /b %errorlevel%
:unzipped
echo The mod's folders aren't beside this file. Unzip the whole package first (right-click the zip, Extract All),
echo then double-click "Start BF3 VR (VDXR).bat" in the folder that makes. If you did, your antivirus may have removed build\injector.exe:
echo allow it there and unzip again.
pause
exit /b 1

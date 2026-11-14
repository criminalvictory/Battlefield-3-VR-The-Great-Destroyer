@echo off
rem Battlefield 3 VR (pre-alpha): starts SteamVR, the mod and the game's campaign. Double-click it; README.md says the rest.
rem It runs tools\launch_vr.ps1 with PowerShell (built into Windows), allowed to run this once (-ExecutionPolicy Bypass).
title BF3 VR launcher
if not exist "%~dp0tools\launch_vr.ps1" goto unzipped
if not exist "%~dp0build\injector.exe" goto unzipped
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\launch_vr.ps1" %*
exit /b %errorlevel%
:unzipped
echo The mod's folders aren't beside this file. Unzip the whole package first (right-click the zip, Extract All),
echo then double-click "Start BF3 VR.bat" in the folder that makes. If you did, your antivirus may have removed build\injector.exe:
echo allow it there and unzip again.
pause
exit /b 1

@echo off
rem Battlefield 3 VR: makes "BF3 VR report.txt" in this folder, everything about this PC, the game and the mod's last start (which copy of
rem the game, the EA app, Steam, VR, the logs), private details taken out: the one file to send when the game doesn't start or something's
rem wrong. It starts nothing and changes nothing.
title BF3 VR report
if not exist "%~dp0tools\report.ps1" goto unzipped
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\report.ps1" %*
exit /b %errorlevel%
:unzipped
echo The mod's folders aren't beside this file. Unzip the whole package first (right-click the zip, Extract All),
echo then double-click "Make a report.bat" in the folder that makes.
pause
exit /b 1

@echo off
rem Your Custom Game: configure the CMake build tree with LaunchGame enabled and
rem open the generated solution. See debug-helper.bat for details.

set "GameAppId=70"
set "LauncherMod=valve"
set "GameDir=C:\Program Files (x86)\Steam\steamapps\common\Half-Life"

call "%~dp0debug-helper.bat"

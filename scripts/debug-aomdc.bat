@echo off
rem Afraid of Monsters: Director's Cut: configure the CMake build tree with LaunchGame enabled and
rem open the generated solution. See debug-helper.bat for details.

set "GameAppId=70"
set "LauncherMod=aomdc"
set "GameDir=D:\SteamLibrary\steamapps\common\Half-Life"

call "%~dp0debug-helper.bat"

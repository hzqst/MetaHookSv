echo off

set LauncherExe=svencoop.exe
set LauncherMod=svencoop

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 225840 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

copy "%~dp0Build\svencoop.exe" "%GameDir%\" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e
xcopy "%~dp0Build\svencoop_addon\" "%GameDir%\%LauncherMod%_addon\" /y /e

echo -----------------------------------------------------

echo Writing debug configuration...

call powershell -Command "(gc global.props) -replace '<MetaHookLaunchName>.*</MetaHookLaunchName>', '<MetaHookLaunchName>%LauncherExe%</MetaHookLaunchName>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookLaunchCommnand>.*</MetaHookLaunchCommnand>', '<MetaHookLaunchCommnand>-game %LauncherMod%</MetaHookLaunchCommnand>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookGameDirectory>.*</MetaHookGameDirectory>', '<MetaHookGameDirectory>%GameDir%\</MetaHookGameDirectory>' | Out-File global.props"

echo -----------------------------------------------------

echo All files copied to "%GameDir%"
pause
exit

:fail

echo Failed to locate GameInstallDir of Sv-en Coop, please make sure Steam is running and you have Sv-en Coop installed correctly.
pause
exit
echo off

set LauncherExe=svencoop.exe
set LauncherMod=svencoop

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 225840 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\svencoop.exe" "%GameDir%\" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e
xcopy "%~dp0Build\svencoop_addon\" "%GameDir%\%LauncherMod%_addon\" /y /e

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Sven Co-op, please make sure Steam is running and you have Sven Co-op installed correctly.
pause
exit
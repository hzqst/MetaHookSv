echo off

set LauncherExe=metahook.exe
set LauncherMod=gearbox

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 50 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\svencoop.exe" "%GameDir%\%LauncherExe%" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Half-Life : Opposing Force, please make sure Steam is running and you have Half-Life : Opposing Force installed correctly.
pause
exit
echo off

set LauncherExe=metahook.exe
set LauncherMod=valve

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 70 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\%LauncherExe%" "%GameDir%\%LauncherExe%" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Half-Life, please make sure Steam is running and you have Half-Life installed correctly.
pause
exit
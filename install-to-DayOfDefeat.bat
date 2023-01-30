echo off

if not exist "%~dp0Build\svencoop.exe" goto fail_nobuild

set LauncherExe=metahook.exe
set LauncherMod=dod
set FullGameName=Day of Defeat
set ShortGameName=DayOfDefeat

for /f "delims=" %%a in ('"%~dp0SteamAppsLocation/SteamAppsLocation" 30 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\svencoop.exe" "%GameDir%\%LauncherExe%" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop" "%GameDir%\%LauncherMod%" /y /e
xcopy "%~dp0Build\valve" "%GameDir%\%LauncherMod%" /y /e

if not exist "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y

del "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst"
del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst"
del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop_avx2.lst"

powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for %ShortGameName%.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-insecure -game %LauncherMod%\";$shortcut.Save();

echo -----------------------------------------------------

echo Make sure that you have all plugins you want in the plugins.lst

notepad "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst"

echo Done
echo Please launch game from shortcut "MetaHook for %ShortGameName%"
pause
exit

:fail

echo Failed to locate GameInstallDir of %FullGameName%, please make sure Steam is running and you have %FullGameName% installed correctly.
pause
exit

:fail_nobuild

echo Compiled binaries not found ! You have to download compiled zip from github release page or compile the sources by yourself before installing !!!
pause
exit
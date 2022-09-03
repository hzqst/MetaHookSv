echo off

if not exist "%~dp0Build\svencoop.exe" goto fail_nobuild

set LauncherExe=metahook.exe
set LauncherMod=dod

for /f "delims=" %%a in ('"%~dp0SteamAppsLocation/SteamAppsLocation" 30 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\svencoop.exe" "%GameDir%\%LauncherExe%" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop" "%GameDir%\%LauncherMod%" /y /e

copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y

powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for DayOfDefeat.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-game %LauncherMod%\";$shortcut.Save();

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Day of Defeat, please make sure Steam is running and you have Day of Defeat installed correctly.
pause
exit

:fail_nobuild

echo Compiled binaries not found ! You have to download compiled zip from github release page or compile the sources by yourself before installing !!!
pause
exit
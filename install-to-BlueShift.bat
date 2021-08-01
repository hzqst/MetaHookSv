echo off

set LauncherExe=metahook.exe
set LauncherMod=bshift

for /f "delims=" %%a in ('%~dp0SteamAppsLocation/SteamAppsLocation 130 InstallDir') do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\svencoop.exe" "%GameDir%\%LauncherExe%" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e

powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for BlueShift.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-game %LauncherMod%\";$shortcut.Save();

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Half-Life : Blue Shift, please make sure Steam is running and you have Half-Life : Blue Shift installed correctly.
pause
exit
echo off

set LauncherExe=svencoop.exe
set LauncherMod=svencoop

set PsCmdLine='call powershell -File %~dp0SteamAppsLocation/SteamAppsLocation.ps1 225840 InstallDir'
for /f "delims=" %%a in (%PsCmdLine%) do set GameDir=%%a

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%~dp0Build\svencoop.exe" "%GameDir%\" /y
copy "%~dp0Build\SDL2.dll" "%GameDir%\" /y
copy "%~dp0Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%~dp0Build\svencoop\" "%GameDir%\%LauncherMod%\" /y /e
xcopy "%~dp0Build\svencoop_addon\" "%GameDir%\%LauncherMod%_addon\" /y /e

powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for SvenCoop.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-game %LauncherMod%\";$shortcut.Save();

echo -----------------------------------------------------

echo done
pause
exit

:fail

echo Failed to locate GameInstallDir of Sven Co-op, please make sure Steam is running and you have Sven Co-op installed correctly.
pause
exit
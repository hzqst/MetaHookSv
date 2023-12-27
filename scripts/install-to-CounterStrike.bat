@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"tools

if not exist "%SolutionDir%Build\svencoop.exe" goto fail_nobuild

set LauncherExe=metahook.exe
set LauncherMod=cstrike
set FullGameName=Counter-Strike
set ShortGameName=CounterStrike
set GameAppId=10

for /f "delims=" %%a in ('"SteamAppsLocation" %GameAppId% InstallDir') do set OutputString=%%a

if %ERRORLEVEL% equ 0 (
    set GameDir=%OutputString%
)

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

copy "%SolutionDir%Build\svencoop.exe" "%GameDir%\%LauncherExe%" /y
copy "%SolutionDir%Build\FreeImage.dll" "%GameDir%\" /y
xcopy "%SolutionDir%Build\svencoop" "%GameDir%\%LauncherMod%" /y /e
xcopy "%SolutionDir%Build\cstrike_hd" "%GameDir%\cstrike_hd" /y /e
xcopy "%SolutionDir%Build\valve" "%GameDir%\%LauncherMod%" /y /e

if not exist "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y

del "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst"
del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst"
del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop_avx2.lst"

cd /d "%SolutionDir%"

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
@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"tools

if exist "%SolutionDir%Build\MetaHook.exe" goto start_install
if exist "%SolutionDir%Build\MetaHook_blob.exe" goto start_install

goto fail_nobuild

:start_install

set LauncherExe=metahook.exe
set LauncherMod=bshift
set FullGameName=Half-Life : Blue Shift
set ShortGameName=BlueShift
set GameAppId=130

for /f "delims=" %%a in ('"SteamAppsLocation" %GameAppId% InstallDir') do set OutputString=%%a

if %ERRORLEVEL% equ 0 (
    set GameDir=%OutputString%
)

if "%GameDir%"=="" goto fail

echo -----------------------------------------------------

echo Copying files...

set GameSDL2Path=%GameDir%\SDL2.dll
for /f "delims=" %%i in ('powershell.exe -Command "$filePath = '%GameSDL2Path%'; $fileVersion = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($filePath).FileVersion; Write-Output $fileVersion"') do set "GameSDL2_fileVersion=%%i"

if "%GameSDL2_fileVersion%"=="2, 0, 20, 0" (
    echo SDL2 version is "%GameSDL2_fileVersion%", no need to replace SDL2
    goto no_replace_sdl2
)

if "%GameSDL2_fileVersion%"=="2, 0, 16, 0" (
    echo SDL2 version is "%GameSDL2_fileVersion%", no need to replace SDL2
    goto no_replace_sdl2
)

echo SDL2 version is "%GameSDL2_fileVersion%", need to replace SDL2
copy "%SolutionDir%Build\SDL2.dll" "%GameDir%\" /y
goto no_replace_sdl2

:no_replace_sdl2

if exist "%SolutionDir%Build\MetaHook.exe" copy "%SolutionDir%Build\MetaHook.exe" "%GameDir%\%LauncherExe%" /y
if exist "%SolutionDir%Build\MetaHook_blob.exe" copy "%SolutionDir%Build\MetaHook_blob.exe" "%GameDir%\metahook_blob.exe" /y
xcopy "%SolutionDir%Build\svencoop" "%GameDir%\%LauncherMod%" /y /e
xcopy "%SolutionDir%Build\valve" "%GameDir%\%LauncherMod%" /y /e

if not exist "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y

if exist "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" del "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst"
if exist "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst" del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst"
if exist "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop_avx2.lst" del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop_avx2.lst"
if exist "%GameDir%\FreeImage.dll" del "%GameDir%\FreeImage.dll"

cd /d "%SolutionDir%"

powershell $shell = New-Object -ComObject WScript.Shell;$shortcut = $shell.CreateShortcut(\"MetaHook for %ShortGameName%.lnk\");$shortcut.TargetPath = \"%GameDir%\%LauncherExe%\";$shortcut.WorkingDirectory = \"%GameDir%\";$shortcut.Arguments = \"-insecure -game %LauncherMod%\";$shortcut.Save();

echo -----------------------------------------------------

echo Make sure that you have all plugins you want in the plugins.lst

notepad "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst"

echo Make sure that you have all library directories you want in the dllpaths.lst

notepad "%GameDir%\%LauncherMod%\metahook\configs\dllpaths.lst"

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
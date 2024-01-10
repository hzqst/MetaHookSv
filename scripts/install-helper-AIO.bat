@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

if exist "%SolutionDir%Build\MetaHook.exe" goto start_install
if exist "%SolutionDir%Build\MetaHook_blob.exe" goto start_install

echo Error: Compiled binaries not found ! You have to download compiled zip from github release page or compile the sources by yourself before installing !!!
pause
exit

:start_install

if not "%GameDir%"=="" (
    if exist "%GameDir%\" goto start_copy

    echo Error: The GameDir "%GameDir%" is not existing !!!
    pause
    exit
)

for /f "delims=" %%a in ('"%SolutionDir%\tools\SteamAppsLocation" %GameAppId% InstallDir') do set OutputString=%%a

if %ERRORLEVEL% equ 0 (
    set "GameDir=%OutputString%"
    goto start_copy
)

echo Error: Failed to locate GameInstallDir of %FullGameName%, please make sure Steam is running and you have %FullGameName% installed correctly.
pause
exit

:start_copy

echo -----------------------------------------------------

echo Copying files...

call "%SolutionDir%scripts\install-helper-CopySDL2.bat"
call "%SolutionDir%scripts\install-helper-CopyBuild.bat"
call "%SolutionDir%scripts\install-helper-CreateShortcut.bat"
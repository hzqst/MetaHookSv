@echo off
rem Shared helper for the scripts\debug-*.bat launchers.
rem
rem The caller sets:
rem   GameAppId    Steam app ID (required)
rem   LauncherMod  mod directory under the game root (optional; CLI default when omitted)
rem   GameDir      game root override (optional; Steam discovery when omitted)
rem
rem It configures the CMake build tree under <repo>\build\x86\Debug with the
rem LaunchGame debug workflow enabled and then opens the generated MetaHookSv.sln.
rem The tree leaf (Debug) selects the default install prefix install\x86\Debug.

if not defined GameAppId (
    echo Error: GameAppId is not defined.
    exit /b 1
)

for %%I in ("%~dp0..") do set "RepoRoot=%%~fI"
set "BuildDir=%RepoRoot%\build\x86\Debug"

rem Keep quotes inside the value so a GameDir containing spaces stays one argument.
set "DepArgs="
if defined LauncherMod set DepArgs=%DepArgs% "-DMETAHOOKSV_GAME_MOD=%LauncherMod%"
if defined GameDir set DepArgs=%DepArgs% "-DMETAHOOKSV_GAME_DIRECTORY=%GameDir%"

pushd "%RepoRoot%"
if errorlevel 1 exit /b 1

cmake -S . -B "%BuildDir%" -G "Visual Studio 17 2022" -A Win32 ^
    -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON ^
    -DMETAHOOKSV_GAME_APPID=%GameAppId% %DepArgs%
if errorlevel 1 (
    echo CMake configuration failed.
    popd
    exit /b 1
)

start "" "%BuildDir%\MetaHookSv.sln"
popd
exit /b 0

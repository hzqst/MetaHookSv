@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

if not "%GameDir%"=="" (

    if not exist "%GameDir%\" (
        echo Error: The GameDir "%GameDir%" is not existing !!!
        pause
        exit
    )

    if not exist "%GameDir%\%LauncherMod%" (
        echo Error: The ModDir "%GameDir%\%LauncherMod%" is not existing !!!
        pause
        exit
    )

    if not exist "%GameDir%\%LauncherMod%\liblist.gam" (
        echo Error: The ModDir "%GameDir%\%LauncherMod%" is not a valid Mod !!!
        pause
        exit
    )

    goto start_copy
)

echo %GameAppId% > "%SolutionDir%tools\steam_appid.txt"

for /f "delims=" %%a in ('"%SolutionDir%\tools\SteamAppsLocation" %GameAppId%') do set OutputString=%%a

if not "%OutputString%"=="" (
    set "GameDir=%OutputString%"
    goto start_copy
)

echo Error: Failed to locate GameInstallDir of %FullGameName%, please make sure Steam is running and you have %FullGameName% installed correctly.
pause
exit

:start_copy

echo -----------------------------------------------------
echo Writing MSVC Properties...

cd /d "%SolutionDir%tools"

copy global_template.props global.props /y

call powershell -Command "(gc global.props) -replace '<MetaHookLaunchName>.*</MetaHookLaunchName>', '<MetaHookLaunchName>%LauncherExe%</MetaHookLaunchName>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookLaunchCommnand>.*</MetaHookLaunchCommnand>', '<MetaHookLaunchCommnand>-game %LauncherMod%</MetaHookLaunchCommnand>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookGameDirectory>.*</MetaHookGameDirectory>', '<MetaHookGameDirectory>%GameDir%\</MetaHookGameDirectory>' | Out-File global.props"
call powershell -Command "(gc global.props) -replace '<MetaHookModName>.*</MetaHookModName>', '<MetaHookModName>%LauncherMod%</MetaHookModName>' | Out-File global.props"

echo -----------------------------------------------------
echo Done

@echo off

cd /d "%SolutionDir%"

tasklist | find /i "devenv.exe"

if "%errorlevel%"=="1" (goto ok1) else (goto ok2)

:ok1

for /f "usebackq tokens=*" %%i in (`tools\vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property productPath`) do (
  set productPath=%%i
)

if exist "%productPath%" (
    "%productPath%" MetaHook.sln
    exit
)

@echo You can open MetaHook.sln with Visual Studio IDE now
pause
exit

:ok2
@echo Please restart Visual Studio IDE to apply changes to the msvc properties
pause
exit

:fail
@echo Failed to locate GameInstallDir of %FullGameName%, please make sure Steam is running and you have %FullGameName% installed correctly.
pause
exit
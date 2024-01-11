@echo off

if exist "%SolutionDir%Build\MetaHook.exe" copy "%SolutionDir%Build\MetaHook.exe" "%GameDir%\%LauncherExe%" /y
if exist "%SolutionDir%Build\MetaHook_blob.exe" copy "%SolutionDir%Build\MetaHook_blob.exe" "%GameDir%\metahook_blob.exe" /y

mkdir "%GameDir%\%LauncherMod%\"
xcopy "%SolutionDir%Build\svencoop" "%GameDir%\%LauncherMod%" /y /e

if "%LauncherMod%"=="svencoop" (
    mkdir "%GameDir%\%LauncherMod%_addon\"
    xcopy "%SolutionDir%Build\svencoop_addon" "%GameDir%\%LauncherMod%_addon\" /y /e

    mkdir "%GameDir%\%LauncherMod%_schinese\"
    xcopy "%SolutionDir%Build\svencoop_schinese" "%GameDir%\%LauncherMod%_schinese\" /y /e

    mkdir "%GameDir%\platform\"
    xcopy "%SolutionDir%Build\platform" "%GameDir%\platform" /y /e

    if not exist "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y

) else (

    xcopy "%SolutionDir%Build\valve" "%GameDir%\%LauncherMod%" /y /e

    if not exist "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y
)

echo -------------------------------------------------------
echo Cleaning deprecated files from previous version of MetaHookSv...

if exist "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst" del "%GameDir%\%LauncherMod%\metahook\configs\plugins_goldsrc.lst"
if exist "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst" del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst"
if exist "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop_avx2.lst" del "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop_avx2.lst"
if exist "%GameDir%\FreeImage.dll" del "%GameDir%\FreeImage.dll"

echo -----------------------------------------------------
echo Make sure that all plugins you want has been added into the plugins.lst

notepad "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst"

echo -----------------------------------------------------
echo Make sure that all library directories required by plugins has been added into the dllpaths.lst

notepad "%GameDir%\%LauncherMod%\metahook\configs\dllpaths.lst"

echo -----------------------------------------------------
echo Done

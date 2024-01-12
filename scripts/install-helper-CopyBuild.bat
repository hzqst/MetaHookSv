@echo off

if exist "%SolutionDir%Build\MetaHook.exe" ( 
    copy "%SolutionDir%Build\MetaHook.exe" "%GameDir%\%LauncherExe%" /y
) else (
    if exist "%SolutionDir%Build\MetaHook_blob.exe" copy "%SolutionDir%Build\MetaHook_blob.exe" "%GameDir%\%LauncherExe%" /y
)

mkdir "%GameDir%\%LauncherMod%\"
xcopy "%SolutionDir%Build\svencoop" "%GameDir%\%LauncherMod%" /y /e

if "%LauncherMod%"=="svencoop" (

    mkdir "%GameDir%\%LauncherMod%_addon\"
    xcopy "%SolutionDir%Build\svencoop_addon" "%GameDir%\%LauncherMod%_addon\" /y /e

    mkdir "%GameDir%\%LauncherMod%_schinese\"
    xcopy "%SolutionDir%Build\svencoop_schinese" "%GameDir%\%LauncherMod%_schinese\" /y /e

    mkdir "%GameDir%\platform\"
    xcopy "%SolutionDir%Build\platform" "%GameDir%\platform" /y /e
    
    mkdir "%GameDir%\%LauncherMod%_hidpi\"
    xcopy "%SolutionDir%Build\svencoop_hidpi" "%GameDir%\%LauncherMod%_hidpi\" /y /e

    if not exist "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" copy "%GameDir%\%LauncherMod%\metahook\configs\plugins_svencoop.lst" "%GameDir%\%LauncherMod%\metahook\configs\plugins.lst" /y

) else (

    if "%GameSDL2_fileVersion%"=="2, 0, 20, 0" (

        mkdir "%GameDir%\%LauncherMod%_hidpi\"
        xcopy "%SolutionDir%Build\valve_hl25_hidpi" "%GameDir%\%LauncherMod%_hidpi\" /y /e

    ) else (

        mkdir "%GameDir%\%LauncherMod%_hidpi\"
        xcopy "%SolutionDir%Build\valve_hidpi" "%GameDir%\%LauncherMod%_hidpi\" /y /e

    )

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
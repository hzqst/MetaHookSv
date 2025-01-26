@echo off

set "GameSDL2Path=%GameDir%\SDL2.dll"

for /f "delims=" %%i in ('powershell.exe -Command "$filePath = '%GameSDL2Path%'; $fileVersion = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($filePath).FileVersion; Write-Output $fileVersion"') do set "GameSDL2_fileVersion=%%i"



if "%GameSDL2_fileVersion%"==" " (
    echo SDL2 version is empty, no need to replace SDL2
    goto no_replace_sdl2
)

if "%GameSDL2_fileVersion%"=="" (
    echo SDL2 version is empty, no need to replace SDL2
    goto no_replace_sdl2
)
if "%GameSDL2_fileVersion%"=="2, 30, 50, 0" (
    echo SDL2 version is "%GameSDL2_fileVersion%", no need to replace SDL2
    goto no_replace_sdl2
)
echo SDL2 version is "%GameSDL2_fileVersion%", need to replace SDL2
copy "%SolutionDir%Build\SDL2.dll" "%GameDir%\" /y

:no_replace_sdl2
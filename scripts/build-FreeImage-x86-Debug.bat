@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

for /f "usebackq tokens=*" %%i in (`tools\vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

cd /d "%SolutionDir%thirdparty\FreeImage_clone\"

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat"

    MSBuild.exe FreeImage.2017.sln "/target:FreeImage" /p:Configuration="Debug" /p:Platform="Win32"

    mkdir "%SolutionDir%thirdparty\install"
    mkdir "%SolutionDir%thirdparty\install\FreeImage"
    mkdir "%SolutionDir%thirdparty\install\FreeImage\x86"
    mkdir "%SolutionDir%thirdparty\install\FreeImage\x86\Debug"
    mkdir "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\include"
    mkdir "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\bin"
    mkdir "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\lib"

    copy "Dist\x32\FreeImage.h" "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\include\FreeImage.h" /y
    copy "Dist\x32\FreeImaged.dll" "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\bin\FreeImaged.dll" /y
    copy "Dist\x32\FreeImaged.lib" "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\lib\FreeImaged.lib" /y
)
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

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86

    MSBuild.exe MetaHook.sln /t:MetaHook /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:Renderer /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:CaptionMod /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:BulletPhysics /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:BulletPhysics /p:Configuration=Release_AVX2 /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:CommunicationDemo /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:DontFlushSoundCache /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:PrecacheManager /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:SCModelDownloader /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:SteamScreenshots /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe MetaHook.sln /t:StudioEvents /p:Configuration=Release /p:Platform="Win32"
)
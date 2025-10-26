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

    "%InstallDir%\Common7\Tools\vsdevcmd.bat"

    MSBuild.exe MetaHook.sln "/target:PluginLibs\UtilThreadTask" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:PluginLibs\UtilHTTPClient_SteamAPI" /p:Configuration="Release" /p:Platform="Win32" 

    MSBuild.exe MetaHook.sln "/target:PluginLibs\UtilHTTPClient_libcurl" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:PluginLibs\UtilAssetsIntegrity" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\Renderer" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\Renderer" /p:Configuration="Release_AVX2" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\BulletPhysics" /p:Configuration="Release" /p:Platform="Win32"
    
    MSBuild.exe MetaHook.sln "/target:Plugins\BulletPhysics" /p:Configuration="Release_AVX2" /p:Platform="Win32"
    
    MSBuild.exe MetaHook.sln "/target:Plugins\VGUI2Extension" /p:Configuration="Release" /p:Platform="Win32" 
    
    MSBuild.exe MetaHook.sln "/target:Plugins\CaptionMod" /p:Configuration="Release" /p:Platform="Win32"
    
    MSBuild.exe MetaHook.sln "/target:Plugins\PrecacheManager" /p:Configuration="Release" /p:Platform="Win32"
    
    MSBuild.exe MetaHook.sln "/target:Plugins\SCModelDownloader" /p:Configuration="Release" /p:Platform="Win32"
    
    MSBuild.exe MetaHook.sln "/target:Plugins\SteamScreenshots" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\StudioEvents" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\ThreadGuard" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\ResourceReplacer" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\HeapPatch" /p:Configuration="Release" /p:Platform="Win32"

    MSBuild.exe MetaHook.sln "/target:Plugins\SCCameraFix" /p:Configuration="Release" /p:Platform="Win32"
)
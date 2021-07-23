for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do (
  set InstallVsVersion=%%i
)

set ForcePlatformToolset=v141

if "%InstallVsVersion%"=="2019" set ForcePlatformToolset=v142

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86
    
    MSBuild.exe MetaHook.sln /t:BulletPhysics /p:Configuration=Release /p:Platform="Win32" /p:PlatformToolset=%ForcePlatformToolset%

    pause
)
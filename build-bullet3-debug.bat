call cmake -S "%~dp0bullet3" -B "%~dp0bullet3\build" -A Win32

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86
    
    if "%VisualStudioVersion%"=="16.0" (
      set force_platform_toolset=v142
    ) else (
      set force_platform_toolset=v141
    )

    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Collision /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Common /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Dynamics /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Geometry /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:BulletCollision /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:BulletDynamics /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:LinearMath /p:Configuration=Debug /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
)
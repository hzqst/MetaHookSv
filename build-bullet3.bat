call cmake -S "%~dp0bullet3" -B "%~dp0bullet3\build" -A Win32

cd /d "%~dp0"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86

    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Collision /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Common /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Dynamics /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:Bullet3Geometry /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:BulletCollision /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:BulletDynamics /p:Configuration=Release /p:Platform="Win32"
    MSBuild.exe "bullet3\build\BULLET_PHYSICS.sln" /t:LinearMath /p:Configuration=Release /p:Platform="Win32"
)
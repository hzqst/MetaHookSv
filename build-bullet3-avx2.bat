call cmake -S "%~dp0bullet3" -B "%~dp0bullet3\build" -A Win32

cd /d "%~dp0"

copy "bullet3\build\src\Bullet3Collision\Bullet3Collision.vcxproj" "bullet3\build\src\Bullet3Collision\Bullet3Collision.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\Bullet3Collision\Bullet3Collision.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\Bullet3Collision\Bullet3Collision.vcxproj.vcxproj"

copy "bullet3\build\src\Bullet3Common\Bullet3Common.vcxproj" "bullet3\build\src\Bullet3Common\Bullet3Common.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\Bullet3Common\Bullet3Common.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\Bullet3Common\Bullet3Common.vcxproj.vcxproj"

copy "bullet3\build\src\Bullet3Dynamics\Bullet3Dynamics.vcxproj" "bullet3\build\src\Bullet3Dynamics\Bullet3Dynamics.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\Bullet3Dynamics\Bullet3Dynamics.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\Bullet3Dynamics\Bullet3Dynamics.vcxproj.vcxproj"

copy "bullet3\build\src\Bullet3Geometry\Bullet3Geometry.vcxproj" "bullet3\build\src\Bullet3Geometry\Bullet3Geometry.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\Bullet3Geometry\Bullet3Geometry.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\Bullet3Geometry\Bullet3Geometry.vcxproj.vcxproj"

copy "bullet3\build\src\BulletCollision\BulletCollision.vcxproj" "bullet3\build\src\BulletCollision\BulletCollision.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\BulletCollision\BulletCollision.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\BulletCollision\BulletCollision.vcxproj.vcxproj"

copy "bullet3\build\src\BulletDynamics\BulletDynamics.vcxproj" "bullet3\build\src\BulletDynamics\BulletDynamics.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\BulletDynamics\BulletDynamics.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\BulletDynamics\BulletDynamics.vcxproj.vcxproj"

copy "bullet3\build\src\LinearMath\LinearMath.vcxproj" "bullet3\build\src\LinearMath\LinearMath.vcxproj.bak"
call powershell -Command "(gc bullet3\build\src\LinearMath\LinearMath.vcxproj) -replace '<EnableEnhancedInstructionSet>StreamingSIMDExtensions2</EnableEnhancedInstructionSet>', '<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>' | Out-File bullet3\build\src\LinearMath\LinearMath.vcxproj.vcxproj"

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

copy "bullet3\build\src\Bullet3Collision\Bullet3Collision.vcxproj.bak" "bullet3\build\src\Bullet3Collision\Bullet3Collision.vcxproj"
copy "bullet3\build\src\Bullet3Common\Bullet3Common.vcxproj.bak" "bullet3\build\src\Bullet3Common\Bullet3Common.vcxproj"
copy "bullet3\build\src\Bullet3Dynamics\Bullet3Dynamics.vcxproj.bak" "bullet3\build\src\Bullet3Dynamics\Bullet3Dynamics.vcxproj"
copy "bullet3\build\src\Bullet3Geometry\Bullet3Geometry.vcxproj.bak" "bullet3\build\src\Bullet3Geometry\Bullet3Geometry.vcxproj"
copy "bullet3\build\src\BulletCollision\BulletCollision.vcxproj.bak" "bullet3\build\src\BulletCollision\BulletCollision.vcxproj"
copy "bullet3\build\src\BulletDynamics\BulletDynamics.vcxproj.bak" "bullet3\build\src\BulletDynamics\BulletDynamics.vcxproj"
copy "bullet3\build\src\LinearMath\LinearMath.vcxproj.bak" "bullet3\build\src\LinearMath\LinearMath.vcxproj"
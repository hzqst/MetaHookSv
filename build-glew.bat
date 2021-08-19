xcopy "%~dp0glew\auto\src\" "%~dp0glew\src\" /y /e

call cmake -S "%~dp0glew\build\cmake" -B "%~dp0glew\build" -A Win32

cd /d "%~dp0"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86

    
)
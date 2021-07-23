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

    MSBuild.exe "Plugins\Renderer\Renderer.sln" /t:Renderer /p:Configuration=Release /p:Platform="Win32" /p:PlatformToolset=%force_platform_toolset%
)
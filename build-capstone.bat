copy "capstone\msvc\capstone_static\capstone_static.vcxproj" "capstone\msvc\capstone_static\capstone_static.vcxproj.bak"

powershell -Command "(gc capstone\msvc\capstone_static\capstone_static.vcxproj) -replace 'CAPSTONE_X86_ATT_DISABLE_NO;CAPSTONE_DIET_NO;CAPSTONE_X86_REDUCE_NO;CAPSTONE_HAS_ARM;CAPSTONE_HAS_ARM64;CAPSTONE_HAS_M68K;CAPSTONE_HAS_MIPS;CAPSTONE_HAS_POWERPC;CAPSTONE_HAS_SPARC;CAPSTONE_HAS_SYSZ;CAPSTONE_HAS_X86;CAPSTONE_HAS_XCORE', 'CAPSTONE_X86_ATT_DISABLE;CAPSTONE_DIET_NO;CAPSTONE_X86_REDUCE_NO;CAPSTONE_HAS_X86' | Out-File capstone\msvc\capstone_static\capstone_static.vcxproj"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86
    
    MSBuild.exe "capstone\msvc\capstone.sln" /t:capstone_static /p:Configuration=Release /p:Platform="Win32" /p:PlatformToolset=v141

    copy /y "capstone\msvc\capstone_static\capstone_static.vcxproj.bak" "capstone\msvc\capstone_static\capstone_static.vcxproj"
)
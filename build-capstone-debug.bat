cd /d "%~dp0"

copy "capstone\msvc\capstone_static\capstone_static.vcxproj" "capstone\msvc\capstone_static\capstone_static.vcxproj.bak"

call powershell -Command "(gc capstone\msvc\capstone_static\capstone_static.vcxproj) -replace 'CAPSTONE_X86_ATT_DISABLE_NO;CAPSTONE_DIET_NO;CAPSTONE_X86_REDUCE_NO;CAPSTONE_HAS_TRICORE;CAPSTONE_HAS_ARM;CAPSTONE_HAS_ARM64;CAPSTONE_HAS_BPF;CAPSTONE_HAS_EVM;CAPSTONE_HAS_M680X;CAPSTONE_HAS_M68K;CAPSTONE_HAS_MIPS;CAPSTONE_HAS_MOS65XX;CAPSTONE_HAS_POWERPC;CAPSTONE_HAS_RISCV;CAPSTONE_HAS_SPARC;CAPSTONE_HAS_SYSZ;CAPSTONE_HAS_TMS320C64X;CAPSTONE_HAS_WASM;CAPSTONE_HAS_X86;CAPSTONE_HAS_XCORE', 'CAPSTONE_X86_ATT_DISABLE;CAPSTONE_DIET_NO;CAPSTONE_X86_REDUCE_NO;CAPSTONE_HAS_X86' | Out-File capstone\msvc\capstone_static\capstone_static.vcxproj"

call powershell -Command "(gc capstone\msvc\capstone_static\capstone_static.vcxproj) -replace '<ConfigurationType>StaticLibrary</ConfigurationType>', '<ConfigurationType>StaticLibrary</ConfigurationType><PlatformToolset>$(DefaultPlatformToolset)</PlatformToolset>' | Out-File capstone\msvc\capstone_static\capstone_static.vcxproj"

call powershell -Command "(gc capstone\msvc\capstone_static\capstone_static.vcxproj) -replace '<Keyword>Win32Proj</Keyword>', '<Keyword>Win32Proj</Keyword><WindowsTargetPlatformVersion>$([Microsoft.Build.Utilities.ToolLocationHelper]::GetLatestSDKTargetPlatformVersion(\"Windows\", \"10.0\"))</WindowsTargetPlatformVersion>' | Out-File capstone\msvc\capstone_static\capstone_static.vcxproj"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86

    MSBuild.exe "capstone\msvc\capstone.sln" /t:capstone_static /p:Configuration=Debug /p:Platform="Win32"

    copy /y "capstone\msvc\capstone_static\capstone_static.vcxproj.bak" "capstone\msvc\capstone_static\capstone_static.vcxproj"
    del "capstone\msvc\capstone_static\capstone_static.vcxproj.bak"
)
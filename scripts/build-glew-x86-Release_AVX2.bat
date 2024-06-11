@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

call cmake -S "%SolutionDir%thirdparty\glew_fork" -B "%SolutionDir%thirdparty\build\glew\x86\Release_AVX2" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\glew\x86\Release_AVX2" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake"  -DONLY_LIBS=TRUE -Dglew-cmake_BUILD_SHARED=FALSE -DGLEW_MSVC_LTCG=TRUE

call cmake --build "%SolutionDir%thirdparty\build\glew\x86\Release_AVX2" --config Release --target install
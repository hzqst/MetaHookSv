@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

call cmake -S "%SolutionDir%thirdparty\SDL2-compat" -B "%SolutionDir%thirdparty\build\SDL2-compat\x86\Release" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\SDL2-compat\x86\Release" -DCMAKE_PREFIX_PATH="%SolutionDir%thirdparty\install\SDL3\x86\Release" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake"  -DSDL2COMPAT_TESTS=FALSE -DSDL2COMPAT_INSTALL_SDL3=TRUE

call cmake --build "%SolutionDir%thirdparty\build\SDL2-compat\x86\Release" --config Release --target install
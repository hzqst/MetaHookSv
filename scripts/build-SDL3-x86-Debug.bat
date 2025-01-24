@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

call cmake -S "%SolutionDir%thirdparty\SDL3" -B "%SolutionDir%thirdparty\build\SDL3\x86\Debug" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\SDL3\x86\Debug" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake" 

call cmake --build "%SolutionDir%thirdparty\build\SDL3\x86\Debug" --config Debug --target install
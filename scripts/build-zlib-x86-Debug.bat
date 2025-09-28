@echo off

setlocal

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

call cmake -G "Visual Studio 17 2022" -S "%SolutionDir%thirdparty\zlib-fork" -B "%SolutionDir%thirdparty\build\zlib\x86\Debug" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\zlib\x86\Debug" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake" -DZLIB_BUILD_TESTING=FALSE -DUSE_VCLTL=TRUE

call cmake --build "%SolutionDir%thirdparty\build\zlib\x86\Debug" --config Debug --target install

mkdir "%SolutionDir%install\"
mkdir "%SolutionDir%install\x86\"
mkdir "%SolutionDir%install\x86\Debug\"

endlocal
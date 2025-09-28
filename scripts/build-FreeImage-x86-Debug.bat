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

call cmake -G "Visual Studio 17 2022" -S "%SolutionDir%thirdparty\FreeImage_clone" -B "%SolutionDir%thirdparty\build\FreeImage\x86\Debug" -A Win32 -DCMAKE_PREFIX_PATH="%SolutionDir%thirdparty\install\zlib\x86\Debug" -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\FreeImage\x86\Debug" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake" -DUSE_VCLTL=TRUE

call cmake --build "%SolutionDir%thirdparty\build\FreeImage\x86\Debug" --config Debug --target install

mkdir "%SolutionDir%install\"
mkdir "%SolutionDir%install\x86\"
mkdir "%SolutionDir%install\x86\Debug\"
copy "%SolutionDir%thirdparty\install\FreeImage\x86\Debug\bin\FreeImaged.dll" "%SolutionDir%install\x86\Debug\FreeImaged.dll"


endlocal
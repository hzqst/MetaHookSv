@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

:: Check if SDL3 directory has been initialized
if not exist "%SolutionDir%thirdparty\SDL3_fork\CMakeLists.txt" if not exist "%SolutionDir%thirdparty\SDL3_fork\.git" (
    echo Initializing SDL3 submodule only...
    :: Initialize only the SDL3 submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\SDL3_fork"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

call cmake -G "Visual Studio 17 2022" -S "%SolutionDir%thirdparty\SDL3_fork" -B "%SolutionDir%thirdparty\build\SDL3\x86\Release" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\SDL3\x86\Release" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake"  -DCMAKE_POLICY_VERSION_MINIMUM=3.5

call cmake --build "%SolutionDir%thirdparty\build\SDL3\x86\Release" --config Release --target install
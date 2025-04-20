@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

:: Check if glew_fork directory has been initialized
if not exist "%SolutionDir%thirdparty\glew_fork\.git" (
    echo Initializing glew_fork submodule only...
    :: Initialize only the glew_fork submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\glew_fork"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

call cmake -S "%SolutionDir%thirdparty\glew_fork" -B "%SolutionDir%thirdparty\build\glew\x86\Debug" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\glew\x86\Debug" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake"  -DONLY_LIBS=TRUE -Dglew-cmake_BUILD_SHARED=FALSE

call cmake --build "%SolutionDir%thirdparty\build\glew\x86\Debug" --config Debug --target install
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

:: Check if ScopeExit directory has been initialized
if not exist "%SolutionDir%thirdparty\ScopeExit\.git" (
    echo Initializing ScopeExit submodule only...
    :: Initialize only the ScopeExit submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\ScopeExit"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

endlocal
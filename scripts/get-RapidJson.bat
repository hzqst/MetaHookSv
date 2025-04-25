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

:: Check if rapidjson directory has been initialized
if not exist "%SolutionDir%thirdparty\rapidjson\.git" (
    echo Initializing rapidjson submodule only...
    :: Initialize only the rapidjson submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\rapidjson"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

endlocal
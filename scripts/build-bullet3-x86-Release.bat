@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

:: Check if bullet3_fork directory has been initialized
if not exist "%SolutionDir%thirdparty\bullet3_fork\.git" (
    echo Initializing bullet3_fork submodule only...
    :: Initialize only the bullet3_fork submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\bullet3_fork"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

call cmake -G "Visual Studio 17 2022" -S "%SolutionDir%thirdparty\bullet3_fork" -B "%SolutionDir%thirdparty\build\bullet3\x86\Release" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\bullet3\x86\Release" -DUSE_MSVC_SSE2=FALSE -DUSE_MSVC_SSE=FALSE -DUSE_MSVC_AVX=FALSE -DUSE_MSVC_AVX2=FALSE -DBUILD_BULLET2_DEMOS=FALSE -DBUILD_BULLET3=TRUE -DBUILD_BULLET_ROBOTICS_EXTRA=FALSE -DBUILD_BULLET_ROBOTICS_GUI_EXTRA=FALSE -DBUILD_CLSOCKET=FALSE -DBUILD_CONVEX_DECOMPOSITION_EXTRA=FALSE -DBUILD_CPU_DEMO=FALSE -DBUILD_NET=FALSE -DBUILD_EXTRAS=FALSE -DBUILD_GIMPACTUTILS_EXTRA=FALSE -DBUILD_HACD_EXTRA=FALSE -DBUILD_INVERSE_DYNAMIC_EXTRA=FALSE -DBUILD_OBJ2SDF_EXTRA=FALSE -DBUILD_OPENGL3_DEMOS=FALSE -DBUILD_PYBULLET=FALSE -DBUILD_SERIALIZE_EXTRA=FALSE -DBUILD_SHARED_LIBS=FALSE -DBUILD_UNIT_TESTS=FALSE  -DINSTALL_LIBS=TRUE -DUSE_GLUT=FALSE -DUSE_GRAPHICAL_BENCHMARK=FALSE -DCMAKE_POLICY_VERSION_MINIMUM=3.5

call cmake --build "%SolutionDir%thirdparty\build\bullet3\x86\Release" --config Release --target install
@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

:: Check if curl directory has been initialized
if not exist "%SolutionDir%thirdparty\curl\.git" (
    echo Initializing curl submodule only...
    :: Initialize only the curl submodule without recursive initialization
    call git submodule update --init "%SolutionDir%thirdparty\curl"
    if errorlevel 1 (
        echo Error: git submodule initialization failed!
        exit /b 1
    )
    echo submodule initialization completed.
)

call cmake -G "Visual Studio 17 2022" -S "%SolutionDir%thirdparty\curl" -B "%SolutionDir%thirdparty\build\libcurl\x86\Debug" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\libcurl\x86\Debug" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake" -DBUILD_CURL_EXE=FALSE -DBUILD_LIBCURL_DOCS=FALSE -DBUILD_MISC_DOCS=FALSE -DCURL_ZLIB=FALSE -DCURL_ZSTD=FALSE -DCURL_BROTLI=FALSE -DENABLE_CURL_MANUAL=FALSE -DENABLE_THREADED_RESOLVER=FALSE -DPICKY_COMPILER=FALSE -DUSE_LIBIDN2=FALSE -DUSE_NGHTTP2=FALSE -DUSE_WIN32_IDN=TRUE -DCURL_USE_LIBPSL=FALSE -DUSE_LIBPSL=FALSE -DCURL_USE_LIBSSH2=FALSE -DUSE_LIBSSH2=FALSE -DCURL_USE_SCHANNEL=TRUE -DCURL_ENABLE_SSL=TRUE -DCMAKE_POLICY_VERSION_MINIMUM=3.5

call cmake --build "%SolutionDir%thirdparty\build\libcurl\x86\Debug" --config Debug --target install
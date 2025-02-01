@echo off

:: Check if SolutionDir is already set and non-empty
if not defined SolutionDir (
    :: Only set SolutionDir if it's not already set
    SET "SolutionDir=%~dp0.."
)

:: Ensure the path ends with a backslash
if not "%SolutionDir:~-1%"=="\" SET "SolutionDir=%SolutionDir%\"

cd /d "%SolutionDir%"

call cmake -S "%SolutionDir%thirdparty\curl" -B "%SolutionDir%thirdparty\build\libcurl\x86\Debug" -A Win32 -DCMAKE_INSTALL_PREFIX="%SolutionDir%thirdparty\install\libcurl\x86\Debug" -DCMAKE_TOOLCHAIN_FILE="%SolutionDir%tools\toolchain.cmake" -DBUILD_CURL_EXE=FALSE -DBUILD_LIBCURL_DOCS=FALSE -DBUILD_MISC_DOCS=FALSE -DCURL_ENABLE_SSL=FALSE -DCURL_ZLIB=FALSE -DCURL_ZSTD=FALSE -DCURL_BROTLI=FALSE -DENABLE_CURL_MANUAL=FALSE -DENABLE_THREADED_RESOLVER=FALSE -DPICKY_COMPILER=FALSE -DUSE_LIBIDN2=FALSE -DUSE_NGHTTP2=FALSE -DUSE_WIN32_IDN=TRUE -DCURL_USE_LIBPSL=FALSE -DUSE_LIBPSL=FALSE -DCURL_USE_LIBSSH2=FALSE -DUSE_LIBSSH2=FALSE

call cmake --build "%SolutionDir%thirdparty\build\libcurl\x86\Debug" --config Debug --target install
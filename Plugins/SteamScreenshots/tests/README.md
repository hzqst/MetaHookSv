# OpenGL capture tests

Standalone C++20 tests compile the production `gl_capture.cpp` against GLFW and
GLEW, without loading MetaHook, Steam or the game. `support/metahook.h` supplies
only the host's capture dimensions and console logging. Pixels are rendered into
a hidden GLFW window's back buffer and checked byte for byte after capture.

Requires CMake 3.20+, a C++20 compiler and a working desktop OpenGL 3.3 driver.
Context creation failures fail the tests rather than silently skipping them.

From the repository root on Windows:

```powershell
git submodule update --init thirdparty/glfw thirdparty/glew_fork
cmake -S Plugins/SteamScreenshots/tests -B intermediate/SteamScreenshotsTests -G "Visual Studio 17 2022" -A Win32
cmake --build intermediate/SteamScreenshotsTests --config Release
ctest --test-dir intermediate/SteamScreenshotsTests -C Release --output-on-failure
```

Coverage includes synchronous/asynchronous RGB readback, default and non-default
pixel packing, a pre-bound caller PBO, independent read/draw FBO bindings,
capture width/height changes, vertical flipping, and framebuffer capability gates.
The capture region changes inside a fixed-size hidden framebuffer to avoid
platform-specific hidden-window resize behavior.

The fence failure test temporarily replaces GLEW's wait entry point to inject
timeout/failure results; the PBO, readback, fence and subsequent recovery use real
OpenGL. Capability tests similarly override GLEW capability flags or the bind
entry point. They do not claim to emulate an entire legacy driver.

These tests do not validate the engine's hook ordering, swap timing, or Steam
screenshot submission; those require game integration testing.

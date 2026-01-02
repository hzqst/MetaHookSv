# MetaHookSv Project Overview

## Purpose
MetaHookSv is a client-side modding framework for GoldSrc engine based games, specifically designed for Sven Co-op and other GoldSrc games. It is a porting of the original MetaHook project to support SvEngine (GoldSrc engine modified by Sven Co-op Team).

## Tech Stack
- **Language**: C++ (C++20 standard)
- **Build System**: MSBuild (Visual Studio 2022, vc143 toolset)
- **Graphics API**: OpenGL 4.4 Core Profile
- **Platform**: Windows x86
- **IDE**: Visual Studio 2022

## Key Technologies
- **OpenGL**: GLEW for extension loading
- **Image Processing**: FreeImage library
- **Disassembly**: Capstone engine
- **Multimedia**: SDL2/SDL3
- **3D Models**: tinyobjloader for OBJ format
- **Physics**: Bullet Physics engine (for BulletPhysics plugin)
- **Source SDK**: tier0, tier1, vstdlib, mathlib

## Engine Compatibility
- GoldSrc_blob (buildnum 3248~4554) - Legacy encrypted format
- GoldSrc_legacy (< 6153) - Standard GoldSrc
- GoldSrc_new (8684+) - Modern GoldSrc
- SvEngine (8832+) - Sven Co-op modified engine
- GoldSrc_HL25 (≥9884) - Half-Life 25th Anniversary Update

## Project Structure
- **src/** - MetaHook loader executable source
- **Plugins/** - Plugin implementations (Renderer, BulletPhysics, CaptionMod, etc.)
- **PluginLibs/** - Shared utility libraries
- **include/** - Public interfaces and API headers
- **thirdparty/** - Third-party dependencies
- **Build/** - Output directory for built binaries
- **scripts/** - Build and debug batch scripts
- **docs/** - Documentation files

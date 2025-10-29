# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MetaHookSv is a client-side modding framework for GoldSrc engine based games, specifically designed for Sven Co-op and other GoldSrc games. It is a porting of the original MetaHook project to support SvEngine (GoldSrc engine modified by Sven Co-op Team).

## Build System

### Requirements
- Visual Studio 2022 with vc143 toolset
- CMake
- Git for Windows

### Build Commands

#### Build MetaHook Loader
```bash
scripts\build-MetaHook.bat
```
This builds the main MetaHook.exe (and MetaHook_blob.exe for legacy engines) loader executable.

#### Build All Plugins
```bash
scripts\build-Plugins.bat
```
This builds all plugin DLLs and utility libraries in the correct dependency order.

#### Debug Configuration
For debugging specific games, use the corresponding debug script:
- `scripts\debug-SvenCoop.bat` - Sets up debugging environment for Sven Co-op
- `scripts\debug-CounterStrike.bat` - Sets up debugging environment for Counter-Strike
- `scripts\debug-HalfLife.bat` - Sets up debugging environment for Half-Life
- And other game-specific scripts

### Visual Studio Solution
The main solution file is `MetaHook.sln` which contains all projects organized into logical folders:
- **MetaHook** - Main loader executable
- **Plugins** - All plugin projects
- **PluginLibs** - Utility libraries used by plugins
- **Tools** - Utility tools for installation and management
- **Libs** - Third-party library dependencies

### Build Configurations
- **Debug** - Debug builds with debugging information
- **Release** - Optimized release builds
- **Release_AVX2** - Release builds with AVX2 optimizations (for Renderer and BulletPhysics)
- **Release_blob** - Release builds for legacy blob engines

## Architecture

### Core Components

#### MetaHook Loader (`src/`)
The main executable that bootstraps the entire plugin system. Key files:
- `metahook.cpp` - Core MetaHook API implementation
- `launcher.cpp` - Game launcher and process management
- `LoadBlob.cpp` - Legacy blob engine loader support
- `LoadDllNotification.cpp` - DLL load monitoring system

#### Plugin System
Plugins are organized in the `Plugins/` directory, each implementing specific functionality:

**Core Plugins:**
- **VGUI2Extension** - VGUI2 modding framework, base for other UI plugins
- **Renderer** - Graphics enhancement engine with advanced rendering features
- **BulletPhysics** - Physics simulation using Bullet Physics engine
- **CaptionMod** - Subtitles, translations, and HiDPI support

**Utility Plugins:**
- **ResourceReplacer** - Runtime resource replacement system
- **ThreadGuard** - Thread management and cleanup
- **PrecacheManager** - Resource precaching management
- **SteamScreenshots** - Steam screenshot integration (Sven Co-op only)

#### Plugin Libraries (`PluginLibs/`)
Shared utility libraries:
- **UtilHTTPClient_SteamAPI** - HTTP client using Steam API
- **UtilHTTPClient_libcurl** - HTTP client using libcurl
- **UtilAssetsIntegrity** - Asset integrity verification
- **UtilThreadTask** - Threading utilities

#### Interface System (`include/Interface/`)
Defines plugin interfaces:
- `IPlugins.h` - Plugin management interface
- `IEngine.h` - Engine interface abstraction
- `IVGUI2Extension.h` - VGUI2 extension interface
- `IUtilHTTPClient.h` - HTTP client interface

### Engine Compatibility
MetaHookSv supports multiple engine types:
- **GoldSrc_blob** (buildnum 3248~4554) - Legacy encrypted format
- **GoldSrc_legacy** (< 6153) - Standard GoldSrc
- **GoldSrc_new** (8684+) - Modern GoldSrc
- **SvEngine** (8832+) - Sven Co-op modified engine
- **GoldSrc_HL25** (≥9884) - Half-Life 25th Anniversary Update

### API Architecture
The MetaHook API (`include/metahook.h`) provides:
- **Hook System** - Inline hooks, VFT hooks, IAT hooks
- **Memory Management** - Pattern searching, memory patching
- **Engine Integration** - Engine type detection, module management
- **Plugin Communication** - Inter-plugin interfaces and messaging
- **Misc** - Some other utils like ThreadPool and Cvar Registration...

## Development Workflow

### Adding New Plugins
1. Create new project in appropriate solution folder
2. Reference required PluginLibs dependencies
3. Implement plugin interface in `exportfuncs.cpp`
4. Add plugin to build scripts
5. Update plugin load list in configs

### Testing and Debugging
1. Use debug scripts to set up proper environment
2. Open `MetaHook.sln` in Visual Studio
3. Set target plugin as startup project
4. Use F5 to start debugging with proper game environment

### Plugin Development Patterns
- All plugins export standard entry points via `exportfuncs.h`
- Use MetaHook API for engine interaction and hooking
- Follow existing plugin structure and naming conventions
- Leverage PluginLibs for common functionality

## Important Notes

### Security and Safety
- This is a defensive security tool for game modification
- All hooks and patches are for legitimate game enhancement
- No malicious code or exploits should be introduced

### Engine Compatibility
- Check engine type using `g_pMetaHookAPI->GetEngineType()` before engine-specific operations
- Use blob-specific APIs for legacy blob engines
- Some features may not be available on all engine types

### Plugin Load Order
Plugin load order in `plugins.lst` is critical for proper dependency resolution and hook installation.

### Changelog rules

Changelog must follow the following format:

Example:

```

**changes**

[Renderer] Fix #741, which caused unexpected per-frame texture reloading

[CaptionMod] Fixed a crash in engine buildnum 4554

[MetahookInstaller] Fixed an issue with "EditPlugin" , when it failed to open the EditPluginDialog for custom game and error out "metahook not installed".

[SDL] Fix #740, which broke the IME input system.

**改动**

[Renderer] 修复 #741，该问题曾导致引擎每帧都会触发重新加载贴图的逻辑

[CaptionMod] 修复了一个在4554版本引擎下导致游戏崩溃的问题

[MetahookInstaller] 修复了 "编辑插件列表" 的一个问题 , 该问题曾导致当选择Custom Game时，编辑插件列表会报错提示没有安装metahook

[SDL] 修复 #740, which broke the IME input system.

```
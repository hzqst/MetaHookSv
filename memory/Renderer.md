---
title: Renderer
type: note
permalink: metahooksv/renderer
---

# Renderer Plugin Architecture

## Overview
The Renderer plugin is MetaHookSv's core graphics-enhancement plugin, providing modern graphics rendering for GoldSrc engine games. It implements an OpenGL rendering pipeline and supports a variety of advanced visual effects.

## Core Features

### Rendering Features
- **Modern OpenGL Rendering Pipeline** - A complete OpenGL-based rendering system
- **Deferred Lighting** - Efficient multi-light rendering
- **Ambient Occlusion (HBAO)** - Screen-space ambient occlusion
- **High Dynamic Range (HDR)** - HDR rendering and tone mapping
- **Real-Time Shadows** - Dynamic shadow casting and reception
- **Water Rendering** - Advanced water effects and refraction
- **Portal Rendering** - Portal effects
- **Post-Processing** - FXAA anti-aliasing, gamma correction, and more

### Model Rendering
- **Studio Model Rendering** - Rendering of `.mdl`-format models
- **Sprite Rendering** - Rendering of `.spr`-format models
- **World Surface Rendering** - Rendering of `.bsp`-format models

## Project Structure

### Core Entry Files
- `exportfuncs.cpp/h` - Plugin exported functions defining HUD interface entry points
- `plugins.cpp` - Plugin main logic and initialization
- `gl_hooks.cpp` - OpenGL function hooks and rendering-pipeline entry points

### Rendering Subsystems

#### Main Rendering Modules
- `gl_rmain.cpp` - Main rendering loop and scene management
- `gl_rmisc.cpp` `gl_draw.cpp` - Wrappers for OpenGL-related API calls
- `gl_studio.cpp` - Studio model rendering (characters/weapons)
- `gl_sprite.cpp` - Sprite rendering
- `gl_entity.cpp` - Management of rendering-specific data structures for entities
- `gl_water.cpp` - Water rendering
- `gl_rsurf.cpp` `gl_wsurf.cpp` - BSP terrain rendering; wsurf is for WorldSurface.
- `gl_light.cpp` - Dynamic lighting system
- `gl_shadow.cpp` - Shadow casting and reception
- `gl_portal.cpp` - Portal rendering
- `gl_shader.cpp` - Shader program management
- `gl_ringbuffer.cpp` - Ring buffer
- `gl_hud.cpp` - HUD element rendering
- `BaseUI.cpp` - Base UI components
- `EngineSurfaceHook.cpp` - Engine surface hooks

#### Tools and Utilities
- `gl_model.cpp` - Model loading and processing
- `gl_cvar.cpp` - Rendering-related console variables
- `VideoMode.cpp` - Video-mode management
- `mathlib2.cpp` - Math-library extensions
- `util.cpp` - General utility functions
- `zone.cpp` - Memory management

#### Platform and Game Specific
- `CounterStrike.cpp/h` - Counter-Strike-specific rendering adaptation
- `VGUI2ExtensionImport.cpp/h` - VGUI2 extension interface
- `GameUI.cpp` - Game UI adaptation

#### Thread and Task Management
- `LambdaThreadedTask.cpp/h` - Lambda task system
- `UtilThreadTask.cpp/h` - Thread-task utilities

#### Hashing and Messages
- `MurmurHash2.cpp/h` - MurmurHash2 implementation
- `parsemsg.cpp` - Message parsing

### Header Files

#### Core Headers
- `gl_local.h` - Internal state and global variable definitions **(most important)**
- `gl_common.h` - Common rendering definitions and macros
- `exportfuncs.h` - Exported-function interface definitions
- `privatehook.h` - Private hook definitions

#### Subsystem Headers
- `gl_shader.h` - Shader system
- `gl_model.h` - Model processing
- `gl_water.h` - Water rendering
- `gl_sprite.h` - Sprite rendering
- `gl_studio.h` - Studio model rendering
- `gl_hud.h` - HUD system
- `gl_shadow.h` - Shadow system
- `gl_light.h` - Lighting system
- `gl_wsurf.h` - Deformable surfaces
- `gl_portal.h` - Portal system
- `gl_entity.h` - Entity system
- `gl_ringbuffer.h` - Ring buffer
- `gl_draw.h` - 2D drawing
- `gl_cvar.h` - Console variables
- `qgl.h` - OpenGL wrapper functions; essentially only a reference to GLEW

#### Utility Headers
- `mathlib2.h` - Math-library extensions
- `plugins.h` - Plugin interface
- `zone.h` - Memory management
- `util.h` - General utilities
- `enginedef.h` - Engine definitions
- `bspfile.h` - BSP file format
- `modelgen.h` - Model generation
- `spritegn.h` - Sprite definitions

### Shader Resources

Shader files are located in `Build\svencoop\renderer\shader\`:

#### Post-Processing Shaders
- `pp_fxaa.frag.glsl` - FXAA anti-aliasing
- `hdr_brightpass.frag.glsl` - HDR bright-pass extraction
- `hdr_lumpass.frag.glsl` - HDR bloom
- `hdr_tonemap.frag.glsl` - HDR tone mapping
- `gamma_correction.frag.glsl` - Gamma correction
- `gaussian_blur_16x.frag.glsl` - Gaussian blur
- `down_sample.frag.glsl` - Downsampling

#### Geometry Shaders
- `studio_shader.geom.glsl` - Studio model geometry shader
- `wsurf_shader.geom.glsl` - WorldSurface geometry shader

#### Deferred Rendering
- `dlight_shader.vert.glsl/.frag.glsl` - Deferred lighting
- `dfinal_shader.frag.glsl` - Deferred-rendering final composition
- `blit_oitblend.frag.glsl` - OIT blending

#### Water and Portal
- `water_shader.vert.glsl/.frag.glsl` - Water rendering
- `portal_shader.vert.glsl/.frag.glsl` - Portal rendering

#### Common Shaders
- `fullscreenquad.vert.glsl` - Full-screen quadrilateral
- `fullscreentriangle.vert.glsl` - Full-screen triangle
- `pp_common.vert.glsl` - Common post-processing vertex shader

#### HUD and Debugging
- `hud_debug.vert.glsl/.frag.glsl` - HUD debugging shaders
- `drawfilledrect_shader.vert.glsl/.frag.glsl` - Filled rectangle
- `drawtexturedrect_shader.vert.glsl/.frag.glsl` - Textured rectangle

### Third-Party Dependencies

#### Static Libraries
- **GLEW** - OpenGL extension loading library
- **FreeImage** - Image-format support
- **Capstone** - Disassembly engine
- **SDL2/SDL3** - Cross-platform multimedia libraries
- **tinyobjloader** - OBJ model loader

#### Source SDK Components
- Complete tier0, tier1, and vstdlib systems
- Math library (mathlib)
- File-system interface
- Memory-management system

## Build Configuration

### Configuration Types
- **Debug** - Debug build with complete debugging information
- **Release** - Optimized build with standard optimizations
- **Release_AVX2** - AVX2-optimized build for high-performance rendering

### Key Compilation Settings
- **C++ Standard**: C++20
- **Runtime Library**: Multi-threaded (Release) / Multi-threaded Debug (Debug)
- **OpenGL**: Uses static linking for GLEW
- **Parallel Compilation**: Enables `/MP` multi-core compilation in Release mode

### Output Paths
- **Debug**: `output\Win32\Debug\renderer.dll`
- **Release**: `output\Win32\Release\renderer.dll`
- **Release_AVX2**: `output\Win32\Release_AVX2\renderer.dll`

### Deployment
After building, files are automatically copied to the game directory:
- Main plugin: `$(GameDir)/metahook/renderer/`
- Dependent DLL: `$(GameDir)/metahook/dlls/FreeImage/`

## Key Architecture

### Rendering Pipeline

1. **Main Rendering Loop** (`gl_rmain.cpp`)
   - Scene management and camera setup
   - Rendering-order control
   - Pass management

2. **Deferred Rendering** (`gl_rsurf.cpp`)
   - G-buffer generation
   - Geometry information storage
   - Multiple-render-target support

3. **Lighting Processing** (`gl_light.cpp`)
   - Dynamic light collection
   - Light-type classification
   - Lighting-calculation optimization

4. **Post-Processing** (`gl_shader.cpp`)
   - HDR pipeline
   - Anti-aliasing
   - Image effects

### Memory Management
- Uses the Source SDK memory-management system (tier0)
- Custom memory pool (`zone.cpp`)
- Resource lifetime management

### Thread Model
- Main-thread rendering
- Background-thread resource loading (`LambdaThreadedTask`)
- Asynchronous shader-compilation support

## Development Guide

### Key Development Files

#### When Modifying Rendering Logic
1. Review `gl_local.h` to understand global state
2. Add functionality in the appropriate subsystem file (for example, add a new lighting effect in `gl_light.cpp`)
3. Add required hooks in `gl_hooks.cpp`

#### When Adding New Shaders
1. Add `.glsl` files under `Build/svencoop/renderer/shader/`
2. Load and compile the shaders in `gl_shader.cpp`
3. Create the corresponding rendering functions

#### When Adding New CVars
1. Register console variables in `gl_cvar.cpp`
2. Add declarations in `gl_cvar.h`
3. Use them in the appropriate rendering modules

### Common Development Tasks

#### Add a New Rendering Pass
1. Add the pass call to the main rendering loop in `gl_rmain.cpp`
2. Implement the pass logic in the appropriate `.cpp` file
3. Add the required shaders in `gl_shader.cpp`

#### Modify the Lighting Model
1. Edit the lighting-calculation functions in `gl_light.cpp`
2. Update the corresponding shader files
3. Test different lighting parameters

#### Optimize Rendering Performance
1. Inspect GPU usage in `gl_ringbuffer.cpp`
2. Optimize model processing in `gl_model.cpp`
3. Tune using the performance CVars in `gl_cvar.cpp`

## Dependencies

### Dependent MetaHook Plugins
- **VGUI2Extension** - VGUI2 interface support (optional; no GUI menu is provided if VGUI2Extension is not loaded)

### Dependent PluginLibs
- No direct dependencies

### Dependent Game Engines
- All supported GoldSrc engine variants
- SvEngine (preferred support)

# Renderer Plugin Architecture

## Overview
Renderer is the core graphics enhancement plugin for MetaHookSv, providing modern OpenGL rendering capabilities to GoldSrc engine games.

## Key Features
- Modern OpenGL 4.4 Core Profile rendering pipeline
- Deferred Lighting with Blinn-Phong model
- HDR (High Dynamic Range) rendering
- HBAO (Horizon-Based Ambient Occlusion)
- Screen Space Reflection (SSR)
- Dynamic shadows
- Advanced water rendering with reflections/refractions
- Portal rendering
- Post-processing effects (FXAA, gamma correction)

## Core Files

### Entry Points
- **exportfuncs.cpp/h** - Plugin export functions, HUD interface entry points
- **plugins.cpp** - Plugin main logic and initialization
- **gl_hooks.cpp** - OpenGL function hooks and rendering pipeline entry

### Main Rendering Modules
- **gl_rmain.cpp** - Main rendering loop and scene management
- **gl_rmisc.cpp, gl_draw.cpp** - OpenGL API call wrappers
- **gl_studio.cpp** - Studio model rendering (characters/weapons)
- **gl_sprite.cpp** - Sprite rendering
- **gl_entity.cpp** - Entity rendering data structure management
- **gl_water.cpp** - Water surface rendering
- **gl_rsurf.cpp, gl_wsurf.cpp** - BSP terrain rendering (WorldSurface)
- **gl_light.cpp** - Dynamic lighting system
- **gl_shadow.cpp** - Shadow casting and receiving
- **gl_portal.cpp** - Portal rendering
- **gl_shader.cpp** - Shader program management

### Important Headers
- **gl_local.h** - Internal state and global variable definitions (MOST IMPORTANT)
- **gl_common.h** - Common rendering definitions and macros
- **gl_shader.h** - Shader system
- **gl_model.h** - Model processing

## Rendering Pipeline

1. **Main Rendering Loop** (gl_rmain.cpp)
   - Scene management and camera setup
   - Rendering order control
   - Pass management

2. **Deferred Rendering** (gl_rsurf.cpp)
   - G-Buffer generation
   - Geometry information storage
   - Multiple render target support

3. **Lighting Processing** (gl_light.cpp)
   - Dynamic light source collection
   - Light type classification
   - Lighting calculation optimization

4. **Post-Processing** (gl_shader.cpp)
   - HDR pipeline
   - Anti-aliasing
   - Image effects

## Shader Resources
Located in `Build/svencoop/renderer/shader/`:
- Post-processing: FXAA, HDR, gamma correction, blur
- Geometry: Studio model, WorldSurface
- Deferred rendering: Light pass, final composition
- Water and Portal shaders
- HUD and debug shaders

## Build Configurations
- **Debug** - Debug build with full debugging info
- **Release** - Optimized release build
- **Release_AVX2** - AVX2 optimized for high performance

## Dependencies
- **GLEW** - OpenGL extension loading
- **FreeImage** - Image format support
- **Capstone** - Disassembly engine
- **SDL2/SDL3** - Cross-platform multimedia
- **tinyobjloader** - OBJ model loader
- **Source SDK** - tier0, tier1, vstdlib, mathlib

## Development Guidelines

### Adding New Rendering Features
1. Check `gl_local.h` for global state
2. Add functionality in appropriate subsystem file
3. Add necessary hooks in `gl_hooks.cpp`

### Adding New Shaders
1. Add .glsl file in `Build/svencoop/renderer/shader/`
2. Load and compile shader in `gl_shader.cpp`
3. Create corresponding rendering function

### Adding New CVars
1. Register console variable in `gl_cvar.cpp`
2. Add declaration in `gl_cvar.h`
3. Use in appropriate rendering module

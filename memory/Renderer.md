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


## Player State Indexing (2026-09-12)

- Trigger: issue #865 exposed confusion between one-based player entity numbers and zero-based player-state slots; Renderer used a scanned address with the member offset and entity-number adjustment folded into it.
- Constraint: player entity numbers are `1..maxclients`, while `IEngineStudio.GetPlayerState` accepts slots `0..maxclients-1`. A legacy scanned address must not be treated as the raw frame-ring base.
- Implementation: `Plugins/Renderer/gl_rmain.cpp::R_GetPlayerState(playerIndex)` now matches BulletPhysics: reject negative slots, slots at or above `MAX_CLIENTS`, and slots at or above `GetMaxClients()` with `nullptr`, then delegate to `IEngineStudio.GetPlayerState`. The contract is documented in `gl_local.h`.
- Callers: `R_DrawStudioEntity` and `R_CreateLowerBodyModel` convert entity indices with `index - 1`; `R_EmitFlashlights` passes its zero-based loop index directly. Unavailable states skip the affected drawing/lighting work; attachment preparation restores `currententity` before returning.
- Dependencies: removed Renderer `cl_frames` / `size_of_frame` globals, declarations, scans, version constants, and mandatory lookup checks from `gl_hooks.cpp`. Keep `cl_parsecount` for network-state freshness checks. Studio API is saved by `HUD_GetStudioModelInterface` before rendering.
- Verification: Renderer `Release | Win32` and `Release_AVX2 | Win32` builds exited 0; DLL hashes match the corresponding Build copies; `git diff --check` passed. Build warnings occurred in untouched source. Game runtime behavior remains unverified; smoke-test player models, follower attachments, multiplayer flashlights, and lower-body rendering.
- Scope: Renderer player-state access only; other private-address discovery remains unchanged.

## Sven Client Core Profile Hooks (2026-09-26)

- Trigger: the legacy GL call-site patches missed client IAT reloads and other callers; upstream GoldSrc_VibeSignatures issues #254/#255 now publish the shader flag and the real clip-plane setter for both Windows clients.
- Constraint: Windows EnableShader/DisableShader are inlined into DrawPortals. They test `ClientPortalManager.m_bShadersAvailable` after calling InitShader. The old EnableClipPlane record named a different plane-calculation function; use the corrected gamedata entry, required on both 10257 and 8948.
- Implementation: client-only IAT hooks cover glEnable, glDisable, glTexEnvf, glColor4f, glBegin, glEnd and glNormal3f. ParticleDraw scopes `g_bIsSCClientParticleDrawing`; begin/end use the existing TriangleAPI bridge only within that scope. glNormal3f is a no-op. InitShader clears the published byte without calling the original; hooks are installed by LoadClient before client Initialize, so no legacy program is created. IAT and inline hooks are removed after client HUD_Shutdown.
- Portal layouts: the manager vector begin/end and source texture ID/width/height come from scalar gamedata. 8948 uses PortalSource texture symbols; 10257 uses ClientPortal texture symbols. The AngleVectors patch remains local and requires one match. Portal clear/copy now use client IAT hooks with a scoped rendering flag and target checks; unrelated client texture copies are forwarded.
- Files: `gl_scclient.cpp` contains the client compatibility handlers and layout readers; `gl_hooks.cpp` resolves symbols and manages hooks; `scripts/validate-gamedata.py` gates both client identities. No public MetaHook API change.
- Verification: Renderer Release and Release_AVX2 Win32 builds exit 0; the Renderer test runner includes real-handler tests for nested/throwing particle calls, GL primitive forwarding, shader byte writes, portal indexing and texture reads. Both original DLLs contain all seven imports and exactly one match for each retained portal patch. The full gamedata validator still reports the same 334 pre-existing virtualFunction-format errors as HEAD, with no added errors. Real Core-context gameplay and visual comparison remain unverified.
- Scope: Windows 10257/8948; DrawVR and DrawTest are excluded by user decision. Runtime acceptance still requires particles, portals/mirrors/monitors, fog/water, map transitions and video reinitialization on both versions.

## Sven Portal View IAT Scopes (2026-09-26)

- Trigger: replace the RenderPortals glClear/glCopyTexSubImage2D call-instruction patches with client import hooks and give portal drawing an explicit framebuffer/debug scope.
- Root cause / boundary: PushView and PopView wrap the entire visible-portal loop, not each portal. Windows 10257 calls them at 0x1004EBD7 / 0x1004F116; 8948 at 0x1009711E / 0x1009764A. Each iteration clears, renders, then copies. Copy source y is screen height minus texture height; it is not necessarily zero.
- Implementation: the existing ClientPortalManager_RenderPortals hook scopes g_bIsRenderingPortalViews and saves the scene FBO, the Renderer's binding bookkeeping and the viewport, restoring them through GL_BindFrameBuffer. Reusing this outer hook covers PushView/PopView and early exits without additional virtual-function hooks or gamedata requirements. SCClient_glClear starts a per-portal P_RenderPortals group, attaches its color/depth-stencil textures and immediately sets the current scene FBO. SCClient_glCopyTexSubImage2D skips only the full level-zero copy to the active portal texture, then restores state and ends the group. Other copies and non-portal clears are forwarded. RAII also cleans up a started target when the function returns before copying.
- Files: gl_scclient.cpp owns the handlers and scopes; gl_hooks.cpp installs/uninstalls both imports alongside the existing seven. The two legacy call-patch scanners and old gl_rmain.cpp handlers are removed.
- Verification: regression tests execute the real handlers with mocked GL and cover ordinary forwarding, two different portal textures, nonzero copy source y, unrelated copies, empty lists, early return, exception cleanup, viewport and scene/context restoration. Renderer Release and Release_AVX2 Win32 builds exited 0. Game runtime and visual verification remain outstanding.
- Scope: Sven Windows 10257/8948. No changes to DrawVR/DrawTest or upstream gamedata.

## Sven RenderPortals Stack Argument ABI (2026-09-26)

- Trigger: svencoop.dmp reported client.dll+0x4B4AD reading address 0x38 while called through Renderer::ClientPortalManager_RenderPortals.
- Root cause: the handler and trampoline pointer omitted the original ref_params_t* stack argument. Both Windows builds use thiscall: ECX is the manager, [ebp+8] is ref_params_t*, and the callee returns with ret 4 (10257 at 0x1004F12A; 8948 at 0x1009765E). The fastcall adapter must declare (void* self, int unusedEDX, ref_params_t* params), and explicitly forward params.
- Dump evidence: client base 0x4F6A0000; original caller at client+0x4ADAC pushes valid params 0x004FF4EC, whose movevars at +0xCC is 0x0C2EB4C8. The incorrectly declared trampoline instead receives 0x0644D885. The downstream routine loads EAX from [ESI+0xCC], obtains zero and faults on maxss xmm0,[eax+0x38]. The manager itself is valid. This happens before the per-portal clear/copy scope.
- Correct implementation: gl_portal.h, privatehook.h and gl_scclient.cpp now declare and forward ref_params_t*. Do not hide the failure with a null check or substitute global refdef state; preserve the original caller's pointer and callee stack cleanup.
- Verification: the actual handler is called in sc_client_tests through an x86 thiscall pointer, with a fake original that asserts pointer identity; the test also checks ESP before/after normal returns. This regression failed before the fix and passes after it, together with all Renderer tests. Release and Release_AVX2 Win32 builds exited 0. Replaying the game scenario remains unverified.
- Lesson / scope: a mock sharing a hook's incorrect prototype cannot validate the binary ABI. For new or modified client hooks, corroborate explicit stack arguments and ret cleanup from both binary versions and make the test exercise the caller's convention.

## Sven Portal Transform and Mode Gamedata (2026-09-26)

- Trigger: ClientPortal_GetPortalTransform / ClientPortal_GetPortalMode still selected hardcoded offsets using the engine build number despite available client layout scalars.
- Constraint: 10257 stores origin/angles directly at ClientPortal_origin_offset / ClientPortal_angles_offset, with ClientPortalSource_mode_offset. 8948 uses ClientPortal_entity_offset to reach cl_entity_t and ClientPortal_mode_offset; its PortalSource origin/angles describe a different object. Offset zero is valid.
- Implementation: Client_FillAddress_CoreProfile selects the entity layout by ClientPortal_entity_offset availability and queries all required fields through the existing fatal-error scalar loader. The readers now live in gl_scclient.cpp, use cached SCClientPortalLayout fields, and reject null portal/entity pointers without writing transform outputs. No engine-build fallback or gamedata changes.
- Verification: actual-reader tests cover both layouts, zero origin offset, relocated fields, mode values, and null entity/portal handling. All Renderer tests and Release / Release_AVX2 Win32 builds passed; gameplay rendering remains unverified.
- Scope: Sven client portal/monitor transform and mode reads only.

## Sven Portal Framebuffer Scope Simplification (2026-09-26)

- Trigger: SCClientFramebufferScope paired GL_PushFrameBuffer/GL_PopFrameBuffer, which save and restore the actual GL_READ_FRAMEBUFFER_BINDING and GL_DRAW_FRAMEBUFFER_BINDING, with its own GL_BindFrameBuffer restore.
- Change: both calls are removed (commit 371ba1fd). The scope still restores the scene FBO pointer, the Renderer's Rendering FBO and the viewport, and GL_BindFrameBuffer re-binds the Rendering FBO through GL_FRAMEBUFFER. After the scope the read and draw bindings therefore share the Rendering FBO's backbuffer; a distinct engine read/draw pair observed before the scope is no longer preserved.
- Decision: accepted by the maintainer as a simplification rather than a regression. This contradicts the guarantee recorded above in "Sven Portal View IAT Scopes" ("actual read/draw framebuffer bindings"), which is why that sentence was amended. Runtime rendering was not re-verified after the change.
- Tests: sc_client_tests.cpp pins parentFBO.s_hBackBufferFBO and asserts both bindings collapse onto it; the GL_PushFrameBuffer/GL_PopFrameBuffer mocks and the framebufferStack bookkeeping were deleted. The SCClientFramebufferScope comment in gl_scclient.cpp was updated to match.
- Verification: Plugins/Renderer/tests/run.ps1 runs all four suites green (studio_model_validation_tests, studio_model_load_tests, ringbuffer_tests, sc_client_tests). Core-context gameplay and visual comparison remain unverified.
- Scope: Sven Windows client portal framebuffer scope only.

---
title: L4D_Glow_Effect
type: note
permalink: metahooksv/l4-d-glow-effect
---

# L4D Glow Effect - Left 4 Dead-style Glow Effect Implementation

## Overview

The MetaHookSv Renderer plugin implements three Left 4 Dead-style glowing outline effects, enabled by setting an entity's `renderfx` property to specific values. These effects use the Stencil Buffer and post-processing techniques, and support both Studio Model (`.mdl`) and World Surface Model (`.bsp`) types.

## Effect Types

### 1. kRenderFxPostProcessGlow (30)
**Basic Glow Effect**

- The entity glows in the specified color with Bloom blur.
- Glow is shown only on the entity's visible portions.
- Does not show through walls.

### 2. kRenderFxPostProcessGlowWallHack (31)
**Wallhack Glow Effect**

- The entity glows in the specified color with Bloom blur.
- Glow is visible through walls and other obstructions.
- The full glow outline is always shown, regardless of whether the entity is occluded.

### 3. kRenderFxPostProcessGlowWallHackBehindWallOnly (32)
**Behind-Wall-Only Glow Effect**

- The entity glows in the specified color with Bloom blur.
- Only portions occluded by walls show glow.
- Visible portions do not glow; only occluded portions do.

## Rendering Flow

### Overall Rendering Pipeline

```
Render the scene normally
    ↓
While rendering entity bodies: collect Glow entities into lists
    ↓
After all transparent entities are rendered (`R_DrawTEntitiesOnList`), during `ClientDLL_DrawTransparentTriangles`, first call the original `ClientDLL_DrawTransparentTriangles`
    ↓
Draw Glow Stencil (mark occluded regions in the current scene without drawing color)
    ↓
Draw Glow Color (draw glow colors to a separate, newly cleared black texture)
    ↓
DownSample + Blur (apply blur post-processing to the texture above)
    ↓
Halo Add (composite the texture above onto the current scene)
```

### Phase 1: Collect Entities

In `StudioRenderModel_Template` (Studio Model) and `R_DrawWorldSurfaceModel` (World Surface), add the entity to the corresponding global list according to its `renderfx` property:

```cpp
// gl_studio.cpp / gl_wsurf.cpp
if (!R_IsRenderingGlowColor() && !R_IsRenderingGlowStencil() && !R_IsRenderingGlowStencilEnableDepthTest())
{
    if ((*currententity)->curstate.renderfx == kRenderFxPostProcessGlow)
    {
        g_PostProcessGlowColorEntities.emplace_back((*currententity));
    }
    else if ((*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHack)
    {
        g_PostProcessGlowStencilEntities.emplace_back((*currententity));
        g_PostProcessGlowColorEntities.emplace_back((*currententity));
    }
    else if ((*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly)
    {
        g_PostProcessGlowStencilEntities.emplace_back((*currententity));
        g_PostProcessGlowEnableDepthTestStencilEntities.emplace_back((*currententity));
        g_PostProcessGlowColorEntities.emplace_back((*currententity));
    }
}
```

**Three global lists:**
- `g_PostProcessGlowColorEntities` - Entities that need glow colors drawn
- `g_PostProcessGlowStencilEntities` - Entities that need Stencil markers drawn
- `g_PostProcessGlowEnableDepthTestStencilEntities` - Entities that need depth-tested Stencil markers drawn

### Phase 2: Draw Glow Stencil

`R_DrawGlowStencil()` runs after transparent-object rendering to mark regions in the Stencil Buffer.

```cpp
// gl_rmain.cpp
void R_DrawGlowStencil()
{
    // First Stencil Pass - mark wallhack regions
    if (g_PostProcessGlowStencilEntities.size() > 0)
    {
        r_draw_glowstencil = true;
        glColorMask(0, 0, 0, 0);  // Disable color writes

        for (auto ent : g_PostProcessGlowStencilEntities)
        {
            (*currententity) = ent;
            R_DrawCurrentEntity(true);
        }

        glColorMask(1, 1, 1, 1);
        r_draw_glowstencil = false;
    }

    // Second Stencil Pass - depth-tested marker
    if (g_PostProcessGlowEnableDepthTestStencilEntities.size() > 0)
    {
        r_draw_glowstencil_enabledepthtest = true;
        glColorMask(0, 0, 0, 0);

        for (auto ent : g_PostProcessGlowEnableDepthTestStencilEntities)
        {
            (*currententity) = ent;
            R_DrawCurrentEntity(true);
        }

        glColorMask(1, 1, 1, 1);
        r_draw_glowstencil_enabledepthtest = false;
    }
}
```

### Phase 3: Draw Glow Color

`R_DrawPostProcessGlow()` draws glow colors to a separate FBO.

```cpp
// gl_rmain.cpp
void R_DrawPostProcessGlow()
{
    if (g_PostProcessGlowColorEntities.empty())
        return;

    auto CurrentFBO = GL_GetCurrentSceneFBO();

    // Copy depth and Stencil to BackBufferFBO4
    GL_BlitFrameBufferToFrameBufferDepthStencil(CurrentFBO, &s_BackBufferFBO4);
    GL_BindFrameBuffer(&s_BackBufferFBO4);

    // Clear the color buffer
    vec4_t clearColor = { 0, 0, 0, 1 };
    GL_ClearColor(clearColor);

    r_draw_glowcolor = true;

    for (auto ent : g_PostProcessGlowColorEntities)
    {
        (*currententity) = ent;
        R_DrawCurrentEntity(true);
    }

    r_draw_glowcolor = false;

    // ... post-processing ...
}
```

### Phase 4: Post-processing (Bloom)

After glow colors are drawn, perform downsampling and Gaussian blur:

```cpp
// gl_rmain.cpp
R_DownSample(&s_BackBufferFBO4, nullptr, &s_DownSampleFBO[0], true, false); // 1 -> 1/4
R_DownSample(&s_DownSampleFBO[0], nullptr, &s_DownSampleFBO[1], true, false); // 1/4 -> 1/16
R_BlurPass(&s_DownSampleFBO[1], &s_BlurPassFBO[0][0], r_glow_bloomscale->value, false);
R_BlurPass(&s_BlurPassFBO[0][0], &s_BlurPassFBO[0][1], r_glow_bloomscale->value, true);
```

### Phase 5: Composite into the Scene

Use `R_CopyColorHaloAdd` to composite the blurred glow onto the scene:

```cpp
// gl_hud.cpp
void R_CopyColorHaloAdd(FBO_Container_t* src, FBO_Container_t* dst)
{
    GL_BindFrameBuffer(dst);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use the Stencil test to exclude regions marked NO_GLOW_BLUR
    GL_BeginStencilCompareNotEqual(STENCIL_MASK_NO_GLOW_BLUR, STENCIL_MASK_NO_GLOW_BLUR);

    GL_UseProgram(copy_color_halo_add.program);
    // ... draw fullscreen triangle ...

    GL_EndStencil();
    glDisable(GL_BLEND);
}
```

## Stencil Marker System

### Stencil Mask Definitions

```cpp
// gl_common.h
#define STENCIL_MASK_NO_GLOW_BLUR   0x8   // Exclude Bloom blur
#define STENCIL_MASK_NO_GLOW_COLOR  0x40  // Exclude color drawing
```

### Stencil Strategy per Effect

| Effect | Stencil Pass | Stencil Test During Color Drawing |
|------|-------------|------------------------|
| PostProcessGlow | None | None |
| PostProcessGlowWallHack | Mark `NO_GLOW_BLUR` | None |
| PostProcessGlowWallHackBehindWallOnly | Mark `NO_GLOW_BLUR` + `NO_GLOW_COLOR` (with depth test) | Draw only when `NO_GLOW_COLOR` is not equal |

## Shader Program State

### Studio Model (gl_studio.cpp)

```cpp
// Stencil Pass stage
R_ShouldDrawGlowStencilEnableDepthTest():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED | STUDIO_NF_DOUBLE_FACE | STUDIO_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHackBehindWallOnly():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED | STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED | STUDIO_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHack():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED | STUDIO_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencil():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED

// Color Pass stage
R_ShouldDrawGlowColor() || R_ShouldDrawGlowColorWallHack():
    StudioProgramState |= STUDIO_GLOW_COLOR_ENABLED

R_ShouldDrawGlowColorWallHackBehindWallOnly():
    StudioProgramState |= STUDIO_GLOW_COLOR_ENABLED | STUDIO_NF_DOUBLE_FACE
```

### World Surface (gl_wsurf.cpp)

```cpp
// Stencil Pass stage
R_ShouldDrawGlowStencilEnableDepthTest():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_COLOR_ENABLED | WSURF_DOUBLE_FACE_ENABLED | WSURF_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHackBehindWallOnly():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_COLOR_ENABLED | WSURF_STENCIL_NO_GLOW_BLUR_ENABLED | WSURF_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHack():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_BLUR_ENABLED | WSURF_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencil():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_BLUR_ENABLED

// Color Pass stage
R_ShouldDrawGlowColor() || R_ShouldDrawGlowColorWallHack():
    WSurfProgramState |= WSURF_GLOW_COLOR_ENABLED

R_ShouldDrawGlowColorWallHackBehindWallOnly():
    WSurfProgramState |= WSURF_GLOW_COLOR_ENABLED | WSURF_DOUBLE_FACE_ENABLED
```

### Program State Macro Definitions

```cpp
// gl_common.h

// World Surface
#define WSURF_GLOW_COLOR_ENABLED            0x80000000ull
#define WSURF_STENCIL_NO_GLOW_BLUR_ENABLED  0x100000000ull
#define WSURF_STENCIL_NO_GLOW_COLOR_ENABLED 0x200000000ull
#define WSURF_DOUBLE_FACE_ENABLED           0x400000000ull

// Studio Model
#define STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED     0x4000000000000ull
#define STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED    0x8000000000000ull
#define STUDIO_GLOW_COLOR_ENABLED               0x10000000000000ull
#define STUDIO_NF_DOUBLE_FACE                   0x10000  // enginedef.h
```

## Fragment Shader Implementation

### Glow Color Output

**Studio Model (studio_shader.frag.glsl):**
```glsl
#elif defined(GLOW_COLOR_ENABLED)

    #if defined(STUDIO_NF_MASKED)
        vec4 diffuseColor = SampleDiffuseTexture(v_texcoord);
        if(diffuseColor.a < 0.5)
            discard;
    #endif

    out_Diffuse = vec4(StudioUBO.r_color.xyz, 1.0);
```

**World Surface (wsurf_shader.frag.glsl):**
```glsl
#elif defined(GLOW_COLOR_ENABLED)

    #if defined(ALPHA_SOLID_ENABLED)
        vec4 diffuseColor = SampleDiffuseTexture(v_texcoord);
        if(diffuseColor.a < 0.5)
            discard;
    #endif

    out_Diffuse = vec4(EntityUBO.r_color.xyz, 1.0);
```

### Halo Add Shader (copy_color.frag.glsl)

```glsl
#if defined(HALO_ADD_ENABLED)
    // Calculate Alpha from luminance for blending
    float flLuminance = max( baseColor.r, max( baseColor.g, baseColor.b ) );
    baseColor.a = pow( flLuminance, 0.8f );
#endif

out_Color = baseColor;
```

## Predicate Function Reference

### Global State Variables

```cpp
// gl_rmain.cpp
bool r_draw_glowstencil = false;                    // Drawing Glow Stencil
bool r_draw_glowstencil_enabledepthtest = false;    // Drawing depth-tested Glow Stencil
bool r_draw_glowcolor = false;                      // Drawing Glow Color
```

### State Query Functions

```cpp
// Whether Glow Stencil rendering is in progress
bool R_IsRenderingGlowStencil() { return r_draw_glowstencil; }

// Whether depth-tested Glow Stencil rendering is in progress
bool R_IsRenderingGlowStencilEnableDepthTest() { return r_draw_glowstencil_enabledepthtest; }

// Whether Glow Color rendering is in progress
bool R_IsRenderingGlowColor() { return r_draw_glowcolor; }
```

### Draw-Condition Functions

```cpp
// Whether to draw Glow Stencil (basic version)
bool R_ShouldDrawGlowStencil()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencil() || 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlow;
}

// Whether to draw WallHack Stencil
bool R_ShouldDrawGlowStencilWallHack()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencil() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHack;
}

// Whether to draw BehindWallOnly Stencil
bool R_ShouldDrawGlowStencilWallHackBehindWallOnly()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencil() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly;
}

// Whether to draw depth-tested Stencil (for BehindWallOnly)
bool R_ShouldDrawGlowStencilEnableDepthTest()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencilEnableDepthTest() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly;
}

// Whether to draw Glow Color (basic version)
bool R_ShouldDrawGlowColor()
{
    return R_IsRenderingGlowColor() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlow;
}

// Whether to draw WallHack Glow Color
bool R_ShouldDrawGlowColorWallHack()
{
    return R_IsRenderingGlowColor() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHack;
}

// Whether to draw BehindWallOnly Glow Color
bool R_ShouldDrawGlowColorWallHackBehindWallOnly()
{
    return R_IsRenderingGlowColor() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly;
}
```

## Console Variables

| CVar | Default | Description |
|------|-------|------|
| `r_glow_bloomscale` | 0.5 | Controls the Glow effect's Bloom blur intensity, from 0.1 to 1.0 |

## Usage Example

Set an entity's glow effect in client code:

```cpp
// Set basic glow
entity->curstate.renderfx = kRenderFxPostProcessGlow;
entity->curstate.rendercolor.r = 255;  // Red component
entity->curstate.rendercolor.g = 0;    // Green component
entity->curstate.rendercolor.b = 0;    // Blue component

// Set wallhack glow
entity->curstate.renderfx = kRenderFxPostProcessGlowWallHack;

// Set behind-wall-only glow
entity->curstate.renderfx = kRenderFxPostProcessGlowWallHackBehindWallOnly;
```

## Technical Details

### Why Use Double-Sided Rendering (DOUBLE_FACE)

Double-sided rendering is required for `kRenderFxPostProcessGlowWallHackBehindWallOnly` and the depth-tested Stencil Pass because:
1. When the camera is inside an object, front-face culling makes the object invisible.
2. It ensures that Stencil is marked correctly from every viewing angle.

### Stencil Test Principles

- **Stencil Pass**: Disables color writes and only updates the Stencil Buffer.
- **Color Pass**: Determines whether to draw a pixel according to its Stencil value.
- The `GL_NOTEQUAL` test excludes marked regions.

### Bloom Post-processing Pipeline

1. **DownSample**: Reduce resolution to 1/16.
2. **Gaussian Blur**: Apply horizontal and vertical Gaussian blur passes.
3. **Halo Add**: Composite onto the scene with Alpha blending.

## Related Files

- `gl_rmain.cpp` - Main rendering loop and Glow rendering entry point
- `gl_studio.cpp` - Studio Model rendering
- `gl_wsurf.cpp` - World Surface rendering
- `gl_hud.cpp` - Post-processing functions
- `gl_common.h` - Program State and Stencil Mask definitions
- `enginedef.h` - `renderfx` constant definitions
- `studio_shader.frag.glsl` - Studio Model Fragment Shader
- `wsurf_shader.frag.glsl` - World Surface Fragment Shader
- `copy_color.frag.glsl` - Halo Add Shader

---

## Appendix: Detailed Rendering Flow per Effect

### kRenderFxPostProcessGlow (30) - Basic Glow Effect

#### Rendering Sequence

```
Render scene normally (collect entities)
    ↓
R_DrawPostProcessGlow (r_draw_glowcolor = true)
    ↓
Post-processing (DownSample + Blur)
    ↓
R_CopyColorHaloAdd (composite into scene)
```

#### Entity Collection Stage

Add the entity to:
- `g_PostProcessGlowColorEntities` ✓

#### Normal Scene Rendering Stage (Render Body)

The body is rendered normally and requires no special handling.

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | Normal rendering state |
| Stencil Ref | Normal value (determined by outline/flatshade, etc.) |
| Stencil Mask | `STENCIL_MASK_ALL` (opaque) or set as needed (transparent) |
| Depth Test | Enabled |
| Depth Write | Enabled |
| Cull Face | Enabled (GL_FRONT) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | Normal rendering state |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL` + other markers |
| Stencil Mask | `STENCIL_MASK_ALL` (opaque) or set as needed (transparent) |
| Depth Test | Enabled |
| Depth Write | Enabled |
| Cull Face | Enabled |

#### Stencil Marker Stage

**No Stencil marker stage is required** (`R_ShouldDrawGlowStencil()` returns true when `r_draw_glowstencil` is false, but this entity is not added to `g_PostProcessGlowStencilEntities`.)

#### Glow Color Drawing Stage (r_draw_glowcolor = true)

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | `STUDIO_GLOW_COLOR_ENABLED` |
| Stencil | No Stencil test |
| Depth Test | Enabled |
| Depth Write | Disabled (`glDepthMask(GL_FALSE)`) |
| Blend | Disabled |
| Cull Face | Enabled (GL_FRONT) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | `WSURF_GLOW_COLOR_ENABLED` |
| Stencil | No Stencil test |
| Depth Test | Enabled |
| Depth Write | Disabled |
| Blend | Disabled |
| Cull Face | Enabled |

#### Post-processing Composition Stage (R_CopyColorHaloAdd)

| Item | Value |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Blend | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` |

**Effect behavior:** Glow Blur is drawn whenever the `NO_GLOW_BLUR` bit is not marked in the Stencil. Because PostProcessGlow marks no Stencil bits, the Blur effect applies across the whole screen.

---

### kRenderFxPostProcessGlowWallHack (31) - Wallhack Glow Effect

#### Rendering Sequence

```
Render scene normally (collect entities)
    ↓
R_DrawGlowStencil (r_draw_glowstencil = true, depth test disabled)
    ↓
R_DrawPostProcessGlow (r_draw_glowcolor = true)
    ↓
Post-processing (DownSample + Blur)
    ↓
R_CopyColorHaloAdd (composite into scene, excluding marked regions)
```

#### Entity Collection Stage

Add the entity to:
- `g_PostProcessGlowStencilEntities` ✓
- `g_PostProcessGlowColorEntities` ✓

#### Normal Scene Rendering Stage (Render Body)

The body is rendered normally and requires no special handling. (Same as PostProcessGlow.)

#### Stencil Marker Stage (r_draw_glowstencil = true)

**Global state:**
- `glColorMask(0, 0, 0, 0)` - Disable color writes

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | `STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED \| STUDIO_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **Disabled** (`glDisable(GL_DEPTH_TEST)`) |
| Depth Write | Preserved |
| Blend | Disabled |
| Cull Face | Enabled (GL_FRONT) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | `WSURF_STENCIL_NO_GLOW_BLUR_ENABLED \| WSURF_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL \| STENCIL_MASK_NO_GLOW_BLUR` |
| Stencil Mask | Set as needed |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **Disabled** |
| Depth Write | Preserved |
| Blend | Disabled |
| Cull Face | Enabled |

**Effect behavior:** Disabling the depth test causes all entity pixels, whether occluded or not, to be marked `NO_GLOW_BLUR`.

#### Glow Color Drawing Stage (r_draw_glowcolor = true)

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | `STUDIO_GLOW_COLOR_ENABLED` |
| Stencil | No Stencil test |
| Depth Test | **Disabled** (`glDisable(GL_DEPTH_TEST)`) |
| Depth Write | Preserved |
| Blend | Disabled |
| Cull Face | Enabled (GL_FRONT) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | `WSURF_GLOW_COLOR_ENABLED` |
| Stencil | No Stencil test |
| Depth Test | **Disabled** |
| Depth Write | Preserved |
| Blend | Disabled |
| Cull Face | Enabled |

**Effect behavior:** With depth testing disabled, the full entity color is drawn, regardless of occlusion.

#### Post-processing Composition Stage (R_CopyColorHaloAdd)

| Item | Value |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Blend | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` |

**Effect behavior:** Because all entity pixels are marked `NO_GLOW_BLUR`, the `GL_NOTEQUAL` Stencil test excludes them. This means **Blur is not composited onto the entity itself**, but creates a glowing outline around it because Blur spreads into unmarked regions.

---

### kRenderFxPostProcessGlowWallHackBehindWallOnly (32) - Behind-Wall-Only Glow Effect

#### Rendering Sequence

```
Render scene normally (collect entities)
    ↓
R_DrawGlowStencil Pass 1 (r_draw_glowstencil = true, depth test disabled)
    ↓
R_DrawGlowStencil Pass 2 (r_draw_glowstencil_enabledepthtest = true, depth test enabled)
    ↓
R_DrawPostProcessGlow (r_draw_glowcolor = true, using the Stencil test)
    ↓
Post-processing (DownSample + Blur)
    ↓
R_CopyColorHaloAdd (composite into scene, excluding marked regions)
```

#### Entity Collection Stage

Add the entity to:
- `g_PostProcessGlowStencilEntities` ✓
- `g_PostProcessGlowEnableDepthTestStencilEntities` ✓
- `g_PostProcessGlowColorEntities` ✓

#### Normal Scene Rendering Stage (Render Body)

The body is rendered normally and requires no special handling. (Same as PostProcessGlow.)

#### Stencil Marker Stage Pass 1 (r_draw_glowstencil = true)

**Global state:**
- `glColorMask(0, 0, 0, 0)` - Disable color writes

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED \| STUDIO_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR \| STENCIL_MASK_NO_GLOW_COLOR` (0x48) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR \| STENCIL_MASK_NO_GLOW_COLOR` (0x48) |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **Disabled** (`glDisable(GL_DEPTH_TEST)`) |
| Depth Write | Preserved |
| Blend | Disabled |
| Cull Face | Enabled (GL_FRONT) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | `WSURF_STENCIL_NO_GLOW_COLOR_ENABLED \| WSURF_STENCIL_NO_GLOW_BLUR_ENABLED \| WSURF_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL \| STENCIL_MASK_NO_GLOW_BLUR \| STENCIL_MASK_NO_GLOW_COLOR` |
| Stencil Mask | Set as needed |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **Disabled** |
| Depth Write | Preserved |
| Blend | Disabled |
| Cull Face | Enabled |

**Effect behavior:** With depth testing disabled, mark **all pixels** of the entity, including occluded and visible portions, as `NO_GLOW_BLUR | NO_GLOW_COLOR`.

#### Stencil Marker Stage Pass 2 (r_draw_glowstencil_enabledepthtest = true)

**Global state:**
- `glColorMask(0, 0, 0, 0)` - Disable color writes

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE \| STUDIO_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **Enabled** |
| Depth Write | **Disabled** (`glDepthMask(GL_FALSE)`) |
| Blend | Disabled |
| Cull Face | **Disabled** (double-sided rendering) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | `WSURF_STENCIL_NO_GLOW_COLOR_ENABLED \| WSURF_DOUBLE_FACE_ENABLED \| WSURF_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL \| STENCIL_MASK_NO_GLOW_COLOR` |
| Stencil Mask | Set as needed |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **Enabled** |
| Depth Write | **Disabled** |
| Blend | Disabled |
| Cull Face | **Disabled** (double-sided rendering) |

**Effect behavior:** With depth testing enabled, only **visible pixels** pass the depth test and are marked `NO_GLOW_COLOR`. The `r_scale` used while drawing this Stencil matches the `r_scale` used later to draw GlowColor, so regions that are within the GlowColor area and not occluded by walls are marked `NO_GLOW_COLOR`; consequently, those regions are not drawn during the later GlowColor stage.

#### Glow Color Drawing Stage (r_draw_glowcolor = true)

**Studio Model:**
| Item | Value |
|------|-----|
| ProgramState | `STUDIO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE` |
| Stencil Func | **`GL_NOTEQUAL`** |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Depth Test | Enabled |
| Depth Func | **`GL_GEQUAL`** (reverse depth test) |
| Depth Write | **Disabled** (`glDepthMask(GL_FALSE)`) |
| Blend | Disabled |
| Cull Face | **Disabled** (double-sided rendering) |

**World Surface:**
| Item | Value |
|------|-----|
| ProgramState | `WSURF_GLOW_COLOR_ENABLED \| WSURF_DOUBLE_FACE_ENABLED` |
| Stencil Func | **`GL_NOTEQUAL`** |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Depth Test | Enabled |
| Depth Func | **`GL_GEQUAL`** |
| Depth Write | **Disabled** |
| Blend | Disabled |
| Cull Face | **Disabled** (double-sided rendering) |

**Effect behavior:**
1. **Stencil test `GL_NOTEQUAL`**: Only pixels whose `NO_GLOW_COLOR` bit is **not marked** pass the test. However, because Pass 1 and Pass 2 mark all/visible pixels, this must be understood together with depth.
2. **Depth Func `GL_GEQUAL`**: Only pixels whose depth is **greater than or equal to** the current depth-buffer value pass the depth test—namely, portions occluded by other objects.

**Actual effect:** Draws only entity pixels occluded by other objects (the behind-wall portion).

#### Post-processing Composition Stage (R_CopyColorHaloAdd)

| Item | Value |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Blend | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` |

**Effect behavior:** Because Pass 1 marks `NO_GLOW_BLUR` for all pixels, the Blur effect is not composited onto the entity itself, but forms a glowing outline around the edges of occluded portions.

---

### Summary Comparison Table

#### Entity Collection

| Effect | GlowColorEntities | GlowStencilEntities | GlowEnableDepthTestStencilEntities |
|------|-------------------|---------------------|-----------------------------------|
| PostProcessGlow | ✓ | - | - |
| PostProcessGlowWallHack | ✓ | ✓ | - |
| PostProcessGlowWallHackBehindWallOnly | ✓ | ✓ | ✓ |

#### Stencil Pass Configuration

| Effect | Pass | Depth Test | Stencil Ref | ProgramState (Studio) |
|------|------|------------|-------------|----------------------|
| PostProcessGlow | - | - | - | - |
| PostProcessGlowWallHack | 1 | Disabled | `NO_GLOW_BLUR` | `STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED` |
| PostProcessGlowWallHackBehindWallOnly | 1 | Disabled | `NO_GLOW_BLUR \| NO_GLOW_COLOR` | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED` |
| PostProcessGlowWallHackBehindWallOnly | 2 | Enabled | `NO_GLOW_COLOR` | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE` |

#### Glow Color Drawing Configuration

| Effect | Depth Test | Depth Func | Stencil Test | ProgramState (Studio) |
|------|------------|------------|--------------|----------------------|
| PostProcessGlow | Enabled | `GL_LESS` | None | `STUDIO_GLOW_COLOR_ENABLED` |
| PostProcessGlowWallHack | Disabled | - | None | `STUDIO_GLOW_COLOR_ENABLED` |
| PostProcessGlowWallHackBehindWallOnly | Enabled | `GL_GEQUAL` | `GL_NOTEQUAL, NO_GLOW_COLOR` | `STUDIO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE` |

#### Halo Add Stage (Shared)

| Item | Value |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Effect | Exclude regions marked `NO_GLOW_BLUR` |

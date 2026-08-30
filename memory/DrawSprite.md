---
title: DrawSprite
type: note
permalink: metahooksv/draw-sprite
---

# Renderer Plugin - Detailed Sprite Rendering Flow

## Overview

Sprites are 2D sprite objects in the GoldSrc engine, used to render particle effects, UI elements, visual effects, and more. The Renderer plugin implements a modern Sprite rendering system that supports frame interpolation, multiple blend modes, and advanced effects.

---

## Sprite Rendering Call Chain

### Complete Call Flow

```
R_RenderScene()
└── R_DrawEntitiesOnList()              // Opaque entity list
    └── R_DrawCurrentEntity(false)
        └── R_DrawSpriteEntity(false)
            └── R_DrawSpriteModel()
                └── R_DrawSpriteModelInterpFrames()

R_RenderScene()
└── R_DrawTransEntities()               // Transparent entity list
    └── R_DrawTEntitiesOnList()
        └── R_DrawCurrentEntity(true)
            └── R_DrawSpriteEntity(true)
                └── R_DrawSpriteModel()
                    └── R_DrawSpriteModelInterpFrames()
```

---

## Detailed Function Analysis

### 1. R_DrawCurrentEntity() - Entity Dispatcher
**Location**: `gl_rmain.cpp:2358-2401`

**Purpose**: dispatches to different rendering functions based on model type

```cpp
void R_DrawCurrentEntity(bool bTransparent) {
    // Check whether the entity should be rendered
    if (R_IsHidingEntity((*currententity)))
        return;
    
    // Calculate the blend value for transparent objects
    if (bTransparent) {
        (*r_blend) = CL_FxBlend((*currententity)) / 255.0;
    }
    
    // Dispatch based on model type
    switch ((*currententity)->model->type) {
        case mod_sprite:
            R_DrawSpriteEntity(bTransparent);
            break;
        case mod_brush:
            R_DrawBrushEntity(bTransparent);
            break;
        case mod_studio:
            R_DrawStudioEntity(bTransparent);
            break;
    }
}
```

---

### 2. R_DrawSpriteEntity() - Sprite Entity Preparation
**Location**: `gl_rmain.cpp:2198-2224`

**Purpose**: prepares position and blend parameters required for Sprite rendering

```cpp
void R_DrawSpriteEntity(bool bTransparent) {
    // Determine the Sprite position
    if ((*currententity)->curstate.body) {
        // Use the attachment-point position
        float* pAttachment = R_GetAttachmentPoint(...);
        VectorCopy(pAttachment, r_entorigin);
    } else {
        // Use the entity origin
        VectorCopy((*currententity)->origin, r_entorigin);
    }
    
    // Handle special blending for Glow render mode
    if (bTransparent && rendermode == kRenderGlow) {
        (*r_blend) *= R_GlowBlend((*currententity));
    }
    
    // Perform the actual draw
    if ((*r_blend) > 0) {
        R_DrawSpriteModel((*currententity));
    }
}
```

**Key points**:
- Supports attachment-point positioning (for attachment to other entities)
- Distance attenuation calculation for Glow mode
- Blend-value filtering (does not draw when blend <= 0)

---

### 3. R_DrawSpriteModel() - Sprite Model Rendering Entry Point
**Location**: `gl_sprite.cpp:769-802`

**Purpose**: obtains Sprite frames and prepares interpolation data

```cpp
void R_DrawSpriteModel(cl_entity_t *ent) {
    // Obtain Sprite data
    auto pSprite = (msprite_t *)ent->model->cache.data;
    auto pSpriteRenderData = R_GetSpriteRenderDataFromModel(ent->model);
    
    // Frame interpolation handling
    float lerp = 0;
    mspriteframe_t* frame = nullptr;
    mspriteframe_t* oldframe = nullptr;
    
    if (R_SpriteAllowLerping(ent, pSprite)) {
        // Enable frame interpolation
        R_GetSpriteFrameInterpolant(ent, pSprite, &frame, &oldframe, &lerp);
    } else {
        // No interpolation; use the current frame
        int frameIndex = (int)ent->curstate.frame;
        oldframe = frame = R_GetSpriteFrame(pSprite, frameIndex);
    }
    
    // Perform the actual render
    R_DrawSpriteModelInterpFrames(ent, pSpriteRenderData.get(), 
                                   pSprite, frame, oldframe, lerp);
}
```

**Key features**:
- **Frame interpolation** - smooth animation transitions
- **Frame selection** - chooses the correct frame based on entity state
- **Render-data caching** - avoids repeated loading

---

### 4. R_DrawSpriteModelInterpFrames() - Core Rendering Function
**Location**: `gl_sprite.cpp:464-767`

This is the core Sprite rendering function and contains complete render-pipeline setup.

#### 4.1 Render-Mode Setup

```cpp
void R_DrawSpriteModelInterpFrames(...) {
    program_state_t SpriteProgramState = 0;
    
    // Calculate color and blending
    colorVec color = { 0 };
    R_SpriteColor(&color, ent, (*r_blend) * 255);
    
    // Configure OpenGL state based on render mode
    switch (ent->curstate.rendermode) {
        case kRenderNormal:
            // Opaque rendering
            glDisable(GL_BLEND);
            break;
            
        case kRenderTransColor:
        case kRenderTransAlpha:
            // Alpha blending
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;
            break;
            
        case kRenderTransAdd:
            // Additive blending
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;
            break;
            
        case kRenderGlow:
            // Glow effect (without depth testing)
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;
            break;
    }
}
```

#### 4.2 Sprite-Type Handling

Sprites support five orientation types:

```cpp
int type = pSprite->type;

// SvEngine supports custom orientations
if (g_iEngineType == ENGINE_SVENGINE) {
    if (ent->curstate.effects & EF_SPRITE_CUSTOM_VP) {
        type = ent->curstate.sequence;
    }
}

// Force ORIENTED mode when a rotation angle is present
if (ent->angles[2] != 0 && type == SPR_VP_PARALLEL) {
    type = SPR_VP_PARALLEL_ORIENTED;
}

switch (type) {
    case SPR_VP_PARALLEL:
        // Parallel to the view plane; always faces the camera
        SpriteProgramState |= SPRITE_PARALLEL_ENABLED;
        break;
        
    case SPR_VP_PARALLEL_UPRIGHT:
        // Parallel to the view plane while remaining upright
        SpriteProgramState |= SPRITE_PARALLEL_UPRIGHT_ENABLED;
        break;
        
    case SPR_FACING_UPRIGHT:
        // Faces the camera while remaining upright
        SpriteProgramState |= SPRITE_FACING_UPRIGHT_ENABLED;
        break;
        
    case SPR_ORIENTED:
        // Fixed orientation; does not face the camera
        SpriteProgramState |= SPRITE_ORIENTED_ENABLED;
        break;
        
    case SPR_VP_PARALLEL_ORIENTED:
        // Parallel to the view plane, with rotation support
        SpriteProgramState |= SPRITE_PARALLEL_ORIENTED_ENABLED;
        break;
}
```

**Sprite type reference**:
- **SPR_VP_PARALLEL** - billboard mode; always faces the camera
- **SPR_VP_PARALLEL_UPRIGHT** - upright billboard; the Y axis remains upward
- **SPR_FACING_UPRIGHT** - faces the camera while remaining upright
- **SPR_ORIENTED** - fixed orientation, used for decals and similar objects
- **SPR_VP_PARALLEL_ORIENTED** - rotatable billboard

#### 4.3 Effect-Flag Setup

```cpp
// Alpha test (alpha clipping)
SpriteProgramState |= SPRITE_ALPHA_TEST_ENABLED;

// Water-surface clipping
if (R_IsRenderingWaterView()) {
    SpriteProgramState |= SPRITE_CLIP_ENABLED;
}

// G-Buffer rendering
if (R_IsRenderingGBuffer()) {
    SpriteProgramState |= SPRITE_GBUFFER_ENABLED;
}

// Fog
if (R_IsRenderingFog()) {
    if (r_fog_mode == GL_LINEAR)
        SpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP)
        SpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP2)
        SpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
}

// Gamma blending
if (R_IsRenderingGammaBlending()) {
    SpriteProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
}

// OIT blending (order-independent transparency)
if (r_draw_oitblend) {
    SpriteProgramState |= SPRITE_OIT_BLEND_ENABLED;
}

// Frame interpolation
if (frame != oldframe) {
    SpriteProgramState |= SPRITE_LERP_ENABLED;
}
```

#### 4.4 Shaders and Drawing

```cpp
// Select the shader program
sprite_program_t prog = { 0 };
R_UseSpriteProgram(SpriteProgramState, &prog);

// Set uniform variables
if (prog.in_up_down_left_right != -1)
    glUniform4f(prog.in_up_down_left_right, 
                frame->up, frame->down, frame->left, frame->right);

if (prog.in_color != -1)
    glUniform4f(prog.in_color, u_color[0], u_color[1], u_color[2], u_color[3]);

if (prog.in_origin != -1)
    glUniform3f(prog.in_origin, r_entorigin[0], r_entorigin[1], r_entorigin[2]);

if (prog.in_angles != -1)
    glUniform3f(prog.in_angles, ent->angles[0], ent->angles[1], ent->angles[2]);

if (prog.in_scale != -1)
    glUniform1f(prog.in_scale, scale);

if (prog.in_lerp != -1)
    glUniform1f(prog.in_lerp, lerp);

// Bind textures
GL_BindTextureUnit(0, GL_TEXTURE_2D, frame->gl_texturenum);

if (SpriteProgramState & SPRITE_LERP_ENABLED) {
    GL_BindTextureUnit(1, GL_TEXTURE_2D, oldframe->gl_texturenum);
}

// Draw a quad (two triangles)
const uint32_t indices[] = {0, 1, 2, 2, 3, 0};
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
```

---

## Data Structures

### CSpriteModelRenderData - Sprite Render Data
```cpp
class CSpriteModelRenderData {
public:
    int flags;                          // Effect flags (such as FMODEL_NOBLOOM)
    model_t* model;                     // Associated model
    std::vector<std::shared_ptr<CSpriteModelRenderMaterial>> vSpriteMaterials;
};
```

### CSpriteModelRenderMaterial - Sprite Material
```cpp
class CSpriteModelRenderMaterial {
public:
    std::string basetexture;            // Base texture name
    CGameModelRenderTexture textures[SPRITE_MAX_TEXTURE];
    mspriteframe_t replaceframe;        // Replacement frame data
};
```

### sprite_program_t - Sprite Shader Program
```cpp
typedef struct sprite_program_s {
    int program;                        // Shader program ID
    int in_up_down_left_right;         // Texture-coordinate uniform
    int in_color;                       // Color uniform
    int in_origin;                      // Position uniform
    int in_angles;                      // Angle uniform
    int in_scale;                       // Scale uniform
    int in_lerp;                        // Interpolation-factor uniform
} sprite_program_t;
```

---

## Render Modes in Detail

### kRenderNormal (0) - Opaque
- Blending disabled
- Writes to the depth buffer
- Standard lighting

### kRenderTransColor (1) - Color Transparency
- Alpha blending
- Does not write depth
- Uses `rendercolor` as the color

### kRenderTransAlpha (2) - Alpha Transparency
- Alpha blending
- Does not write depth
- Uses `renderamt` as the opacity

### kRenderTransAdd (4) - Additive Blending
- Additive blending (`GL_ONE`, `GL_ONE`)
- Does not write depth
- Used for glow effects

### kRenderGlow (3) - Glow
- Additive blending
- Depth testing disabled
- Distance attenuation
- Used for halo effects

---

## Program-State Flags

### Blend Modes
- `SPRITE_ALPHA_BLEND_ENABLED` - Alpha blending
- `SPRITE_ADDITIVE_BLEND_ENABLED` - Additive blending
- `SPRITE_GAMMA_BLEND_ENABLED` - Gamma-space blending

### Effects
- `SPRITE_ALPHA_TEST_ENABLED` - Alpha test
- `SPRITE_LERP_ENABLED` - Frame interpolation
- `SPRITE_CLIP_ENABLED` - Water-surface clipping
- `SPRITE_OIT_BLEND_ENABLED` - Order-independent transparency

### Fog
- `SPRITE_LINEAR_FOG_ENABLED` - Linear fog
- `SPRITE_EXP_FOG_ENABLED` - Exponential fog
- `SPRITE_EXP2_FOG_ENABLED` - Exponential-squared fog
- `SPRITE_LINEAR_FOG_SHIFT_ENABLED` - Fog offset

### Orientation Types
- `SPRITE_PARALLEL_ENABLED` - Parallel billboard
- `SPRITE_PARALLEL_UPRIGHT_ENABLED` - Upright parallel billboard
- `SPRITE_FACING_UPRIGHT_ENABLED` - Camera-facing upright
- `SPRITE_ORIENTED_ENABLED` - Fixed orientation
- `SPRITE_PARALLEL_ORIENTED_ENABLED` - Rotatable billboard

### Render Target
- `SPRITE_GBUFFER_ENABLED` - Writes to the G-Buffer

---

## Frame-Interpolation System

### R_SpriteAllowLerping() - Determines Whether Interpolation Is Allowed
```cpp
bool R_SpriteAllowLerping(cl_entity_t* ent, msprite_t* pSprite) {
    // Check the CVar setting
    if (!r_sprite_lerping->value)
        return false;
    
    // Check the render mode
    if (ent->curstate.rendermode != kRenderNormal &&
        ent->curstate.rendermode != kRenderTransAdd)
        return false;
    
    return true;
}
```

### R_GetSpriteFrameInterpolant() - Obtains Interpolated Frames
```cpp
void R_GetSpriteFrameInterpolant(cl_entity_t* ent, msprite_t* pSprite,
                                  mspriteframe_t** frame,
                                  mspriteframe_t** oldframe,
                                  float* lerp) {
    // Calculate the current and previous frames
    int currentFrame = (int)ent->curstate.frame;
    int lastFrame = (int)ent->latched.prevframe;
    
    // Calculate the interpolation factor
    *lerp = ent->curstate.framerate * (*cl_time - ent->latched.prevanimtime);
    *lerp = math_clamp(*lerp, 0.0f, 1.0f);
    
    *frame = R_GetSpriteFrame(pSprite, currentFrame);
    *oldframe = R_GetSpriteFrame(pSprite, lastFrame);
}
```

**Interpolation effects**:
- Smooth animation transitions
- Eliminates frame jumps
- Improves visual quality

---

## External File Support

Sprites support custom properties through `_external.txt` files:

### sprite_efx - Effect Flags
```
{
    "classname" "sprite_efx"
    "flags" "FMODEL_NOBLOOM"
}
```

### sprite_frame_texture - Frame Texture Replacement
```
{
    "classname" "sprite_frame_texture"
    "frame" "0"
    "replacetexture" "sprites/custom.png"
}
```

---

## Performance Optimization

### 1. Render-Data Cache
- `g_SpriteRenderDataCache` - caches Sprite render data
- Avoids repeatedly parsing external files
- Reduces memory allocations

### 2. Shader-Program Cache
- `g_SpriteProgramTable` - caches compiled shaders
- Fast lookup by program state
- Avoids repeated compilation

### 3. Batched Drawing
- Uses indexed drawing (`glDrawElements`)
- Reduces state changes
- Improves GPU utilization

### 4. Early Culling
- Blend-value check (blend <= 0)
- Frustum culling
- Distance culling (Glow mode)

---

## Shader System

### Sprite Shader Files
- `sprite_shader.vert.glsl` - vertex shader
- `sprite_shader.frag.glsl` - fragment shader

### Shader Variants
Different shader variants are generated from combinations of `SpriteProgramState` flags:
- Base variants: 2^5 = 32 orientation and blend combinations
- Effect variants: fog, interpolation, clipping, and more
- Total: hundreds of shader variants

### Dynamic Compilation
- Compiled on first use
- Compilation results cached
- Supports hot reloading

---

## Debugging and Diagnostics

### OpenGL Debug Group
```cpp
GL_BeginDebugGroupFormat("R_DrawSpriteModelInterpFrames - %s", 
                         ent->model->name);
// ... rendering code ...
GL_EndDebugGroup();
```

### Console Variables
- `r_sprite_lerping` - enables/disables frame interpolation
- `gl_spriteblend` - Sprite blend mode
- `r_drawentities` - enables/disables entity rendering

---

## Summary

Characteristics of the Sprite rendering system:

1. **Flexible render modes** - supports five blend modes
2. **Multiple orientation types** - five billboard modes
3. **Frame interpolation** - smooth animation transitions
4. **Advanced effects** - fog, OIT, and G-Buffer support
5. **External files** - customizable textures and properties
6. **Performance optimization** - caching, batched drawing, and early culling
7. **Shader system** - dynamic compilation and multi-variant support

The Sprite system is the foundation for particle effects, UI elements, and visual effects, providing high-quality visuals through a modern rendering pipeline.

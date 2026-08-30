---
title: DrawScene
type: note
permalink: metahooksv/draw-scene
---

# Renderer Plugin - 3D Scene Rendering Flow

## Entry function: R_RenderScene

Location: `Plugins/Renderer/gl_rmain.cpp`

### Complete rendering flow

```
R_RenderScene()
├── R_SetupFrame()              // Frame setup
├── R_SetupGL()                 // OpenGL state setup
├── R_SetFrustum()              // Frustum setup
├── R_MarkLeaves()              // Mark visible leaf nodes
├── R_BeginRenderGBuffer()      // Begin G-Buffer rendering (deferred rendering)
├── R_PrepareDrawWorld()        // Prepare world rendering
├── R_DrawWorld()               // Draw world geometry
├── R_DrawEntitiesOnList()      // Draw opaque entities
├── R_EndRenderOpaque()         // Finish opaque-object rendering
├── ClientDLL_DrawNormalTriangles()  // Client DLL draws normal triangles
└── R_DrawTransEntities()       // Draw transparent entities
```

---

## Detailed flow analysis

### 1. R_SetupFrame() - Frame setup
**Location**: `gl_rmain.cpp`

**Functionality**:
- Update RefDef (reference definition).
- Determine the BSP leaf node containing the current viewpoint.
- Configure fog effects (underwater fog, Sven Co-op fog, and user-defined fog).

**Key operations**:
```cpp
R_UpdateRefDef();
(*r_viewleaf) = Mod_PointInLeaf(r_origin, (*cl_worldmodel));
R_RenderWaterFog() / R_RenderSvenFog() / R_RenderUserFog();
```

---

### 2. R_SetupGL() - OpenGL state setup
**Functionality**: Configure OpenGL render state, the projection matrix, viewport, and more.

---

### 3. R_SetFrustum() - Frustum setup
**Functionality**: Calculate frustum planes for frustum culling.

---

### 4. R_MarkLeaves() - Mark visible leaf nodes
**Functionality**: Use PVS (Potentially Visible Set) to mark currently visible BSP leaf nodes.

---

### 5. R_BeginRenderGBuffer() - Begin G-Buffer rendering
**Location**: `gl_light.cpp`

**Functionality**: Initialize the G-Buffer for deferred rendering.

**Key operations**:
```cpp
r_draw_gbuffer = true;
GL_BindFrameBuffer(&s_GBufferFBO);
R_SetGBufferMask(GBUFFER_MASK_ALL);
GL_ClearColorDepthStencil(...);
```

**G-Buffer contents**:
- Position
- Normal
- Diffuse color
- Specular information
- Depth

---

### 6. R_PrepareDrawWorld() - Prepare world rendering
**Functionality**: Prepare the data and state required to render world geometry.

---

### 7. R_DrawWorld() - Draw world geometry
**Location**: `gl_wsurf.cpp`

**Functionality**: Draw the BSP world model.

**Rendering flow**:
```cpp
R_DrawWorld()
└── R_DrawWorldSurfaceModel(pModel, entity)
    ├── R_DrawWorldSurfaceLeafSky()      // Sky surfaces
    ├── R_DrawWorldSurfaceLeafStatic()   // Static surfaces
    ├── R_DrawWorldSurfaceLeafAnim()     // Animated surfaces
    └── R_DrawWorldSurfaceLeafShadow()   // Shadows
```

**Surface types**:
- **Sky** - Skybox surfaces
- **Static** - Statically lit surfaces
- **Anim** - Animated-texture surfaces
- **Shadow** - Shadow casting

---

### 8. R_DrawEntitiesOnList() - Draw opaque entities
**Location**: `gl_rmain.cpp`

**Functionality**: Iterate the visible-entity list and draw opaque entities.

**Entity classification**:
```cpp
for (int i = 0; i < (*cl_numvisedicts); ++i) {
    entity = cl_visedicts[i];
    
    if (rendermode != kRenderNormal) {
        R_AddTEntity(entity);  // Add to transparent-entity list
    }
    else if (model->type == mod_sprite && gl_spriteblend) {
        R_AddTEntity(entity);  // Sprite blending
    }
    else if (R_IsViewmodelAttachment(entity)) {
        R_AddViewModelAttachmentEntity(entity);  // View-model attachment
    }
    else {
        R_DrawCurrentEntity(false);  // Draw current entity
    }
}
```

**Entity types**:
- **Studio model** - Characters, weapons, and so on (.mdl)
- **Brush model** - Movable BSP models such as doors and elevators
- **Sprite** - 2D sprites (.spr)

---

### 9. R_EndRenderOpaque() - Finish opaque-object rendering
**Location**: `gl_rmain.cpp`

**Functionality**: Complete G-Buffer rendering and perform deferred-lighting calculations.

**Key operations**:
```cpp
r_draw_opaque = false;
if (R_IsRenderingGBuffer()) {
    R_EndRenderGBuffer(GL_GetCurrentSceneFBO());
}
```

**Deferred-lighting flow**:
1. The G-Buffer finishes storing geometry information.
2. The lighting pass calculates all dynamic lights.
3. The final color is composed into SceneFBO.

---

### 10. ClientDLL_DrawNormalTriangles() - Client rendering
**Functionality**: Call the client DLL's HUD_DrawNormalTriangles, allowing game code to draw custom geometry.

---

### 11. R_DrawTransEntities() - Draw transparent entities
**Location**: `gl_rmain.cpp`

**Functionality**: Draw all transparent objects.

**Two rendering modes**:

#### A. OIT (Order-Independent Transparency) mode
```cpp
if (g_bUseOITBlend) {
    R_ClearOITBuffer();
    r_draw_oitblend = true;
    R_DrawTEntitiesOnList(onlyClientDraw);
    ClientDLL_DrawTransparentTriangles();
    R_DrawParticles();
    R_BlendOITBuffer();
}
```

**OIT characteristics**:
- Independent of transparent-object order
- Stores transparent fragments in linked lists
- GPU sorting and blending
- Higher performance overhead

#### B. Traditional alpha-blending mode
```cpp
else {
    R_DrawTEntitiesOnList(onlyClientDraw);
    ClientDLL_DrawTransparentTriangles();
    R_DrawParticles();
}
```

**Traditional-mode characteristics**:
- Requires back-to-front sorting
- Standard alpha blending
- Better performance

**Transparent objects include**:
- Transparent entities (rendermode != kRenderNormal)
- Transparent triangles (client DLL)
- Particle systems

---

## Rendering-pipeline architecture

### Deferred rendering pipeline

```
[Geometry Pass]
    ↓
[G-Buffer]
├── Position Buffer
├── Normal Buffer
├── Diffuse Buffer
├── Specular Buffer
└── Depth Buffer
    ↓
[Lighting Pass]
├── Dynamic point lights
├── Spotlights
├── Directional lights
└── Ambient light
    ↓
[Composition]
    ↓
[Transparent Pass]
    ↓
[Post-Processing]
├── HDR
├── SSAO
├── SSR
├── FXAA
└── Gamma correction
```

---

## Key data structures

### refdef_t - Render definition
```cpp
typedef struct refdef_s {
    vrect_GoldSrc_t *vrect;      // Viewport rectangle
    vec3_t *vieworg;              // Viewpoint position
    vec3_t *viewangles;           // View angles
    color24 *ambientlight;        // Ambient light
    qboolean *onlyClientDraws;    // Client-only drawing flag
} refdef_t;
```

### Global render state
```cpp
extern refdef_t r_refdef;
extern float r_xfov, r_yfov;           // FOV
extern bool r_fog_enabled;              // Fog enabled
extern cl_entity_t* r_worldentity;      // World entity
extern model_t** cl_worldmodel;         // World model
```

---

## Performance optimization techniques

### 1. Frustum culling
- `R_SetFrustum()` calculates frustum planes.
- Cull objects outside the frustum.

### 2. PVS culling (Potentially Visible Set)
- `R_MarkLeaves()` uses BSP PVS data.
- Render only potentially visible leaf nodes.

### 3. Batched VBO drawing
- Use Vertex Buffer Objects.
- Reduce the number of draw calls.

### 4. Deferred rendering
- Reduce the cost of lighting calculations in multi-light scenes.
- Perform lighting calculations only on visible pixels.

### 5. Asynchronous asset loading
- Load models and textures on background threads.
- Avoid blocking the main thread.

---

## Debugging tools

### OpenGL debug groups
```cpp
GL_BeginDebugGroup("R_RenderScene");
// ... rendering code ...
GL_EndDebugGroup();
```

RenderDoc and similar tools can inspect the rendering calls in each debug group.

---

## Relevant console variables (CVars)

- `r_drawentities` - Whether to draw entities
- `r_drawworld` - Whether to draw the world
- `r_deferred_lighting` - Enable deferred lighting
- `gl_spriteblend` - Sprite blending mode
- `r_fog` - Fog settings

---

## Summary

The Renderer plugin's 3D scene rendering flow uses a modern deferred-rendering pipeline:

1. **Geometry Pass** - Write scene geometry information to the G-Buffer.
2. **Lighting Pass** - Calculate lighting in screen space.
3. **Transparent Pass** - Draw transparent objects.
4. **Post-Processing** - Apply various image effects.

This architecture supports many dynamic lights while maintaining good performance. With optimization techniques such as batched VBO drawing, frustum culling, and PVS culling, it can sustain high frame rates even in complex scenes.

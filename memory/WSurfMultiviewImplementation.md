---
title: WSurfMultiviewImplementation
type: note
permalink: metahooksv/wsurf-multiview-implementation
---

# WorldSurface Multiview Rendering Implementation

## Overview

This document describes the complete implementation of multiview rendering for the WorldSurface renderer. This feature renders multiple views in one draw call to support:
1. **Cubemap Shadow Mapping** - Render six views to a cubemap in a single call
2. **Cascaded Shadow Mapping (CSM)** - Render four cascaded shadows to a texture array in a single call

## Implementation Architecture

### 1. Program State Flag

A new shader state flag was added in `Plugins/Renderer/gl_wsurf.h`:

```cpp
#define WSURF_MULTIVIEW_ENABLED  0x20000000ull
```

This flag enables multiview-related macro definitions and the geometry shader during shader compilation.

### 2. C++ Code Changes

#### 2.1 Shader Compilation (gl_wsurf.cpp)

In `R_UseWSurfProgram`:
- When `state & WSURF_MULTIVIEW_ENABLED`, add the `WSURF_MULTIVIEW_ENABLED` macro definition.
- When multiview is enabled, specify the geometry shader path `wsurf_shader.geom.glsl`.

```cpp
if (state & WSURF_MULTIVIEW_ENABLED)
    defs << "#define WSURF_MULTIVIEW_ENABLED\n";

CCompileShaderArgs args;
args.vsfile = "renderer\\shader\\wsurf_shader.vert.glsl";
if (state & WSURF_MULTIVIEW_ENABLED)
    args.gsfile = "renderer\\shader\\wsurf_shader.geom.glsl";
args.fsfile = "renderer\\shader\\wsurf_shader.frag.glsl";
```

#### 2.2 Runtime Enablement

Multiview checks were added to the following draw functions:
- `R_DrawWorldSurfaceModelShadowProxyInternal`
- `R_DrawWorldSurfaceLeafShadow`
- `R_DrawWorldSurfaceLeafStatic`
- `R_DrawWorldSurfaceLeafAnim`
- `R_DrawWorldSurfaceLeafSky`

When `r_draw_multiview` is true, automatically add the `WSURF_MULTIVIEW_ENABLED` flag:

```cpp
if (r_draw_multiview)
{
    WSurfProgramState |= WSURF_MULTIVIEW_ENABLED;
}
```

### 3. Shader Implementation

#### 3.1 Vertex Shader (wsurf_shader.vert.glsl)

The vertex shader remains unchanged and continues outputting variables with the `v_` prefix:
- `v_worldpos`, `v_normal`, `v_tangent`, etc.
- These variables are received by the geometry shader.

#### 3.2 Geometry Shader (wsurf_shader.geom.glsl) - New

**Input configuration:**
```glsl
layout(triangles) in;

#ifdef WSURF_MULTIVIEW_ENABLED
    layout(triangle_strip, max_vertices = 18) out;  // 3 * 6 views
#else
    layout(triangle_strip, max_vertices = 3) out;
#endif
```

**Core logic:**

When multiview is enabled:
```glsl
#ifdef WSURF_MULTIVIEW_ENABLED
    int numViews = CameraUBO.numViews;
    
    for (int viewIdx = 0; viewIdx < numViews; ++viewIdx)
    {
        gl_Layer = viewIdx;  // Set the TextureArray layer
        
        for (int i = 0; i < 3; ++i)
        {
            // Transform using the matrix for the corresponding view
            vec4 worldPos = vec4(v_worldpos[i], 1.0);
            gl_Position = GetCameraProjMatrix(viewIdx) * 
                         GetCameraWorldMatrix(viewIdx) * worldPos;
            
            // Pass all attributes to the fragment shader
            g_worldpos = v_worldpos[i];
            g_normal = v_normal[i];
            // ... other attributes
            
            EmitVertex();
        }
        EndPrimitive();
    }
#endif
```

When multiview is disabled, the geometry shader simply passes through:
```glsl
#else
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        // Pass all attributes
        EmitVertex();
    }
    EndPrimitive();
#endif
```

#### 3.3 Fragment Shader (wsurf_shader.frag.glsl)

The input source is adapted through preprocessor macros:

```glsl
#ifdef WSURF_MULTIVIEW_ENABLED
    // g_-prefixed variables from the geometry shader
    #define v_worldpos g_worldpos
    #define v_normal g_normal
    // ... other variable mappings
    
    in vec3 g_worldpos;
    in vec3 g_normal;
    // ... other inputs
#else
    // v_-prefixed variables directly from the vertex shader
    in vec3 v_worldpos;
    in vec3 v_normal;
    // ... other inputs
#endif
```

This leaves the rest of the fragment shader unchanged, continuing to access variables through the `v_` prefix.

## Usage

### Prerequisites
- OpenGL 4.4+ Core Profile
- Geometry shader support
- TextureArray render-target support

### Enable Multiview Rendering

1. **Set CameraUBO:**
```cpp
camera_ubo_t CameraUBO{};

// Set transformation matrices for each view
 for (int i = 0; i < 6; ++i)  // For example: the six faces of a cubemap
{
    R_SetupCameraView(&CameraUBO.views[i]);
}

CameraUBO.numViews = 6;  // Or 4 for CSM

GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, 
                      sizeof(CameraUBO), &CameraUBO);
```

2. **Enable the multiview flag:**
```cpp
r_draw_multiview = true;
```

3. **Render the scene:**
```cpp
// Call the draw function normally
R_RenderScene();
```

4. **Restore state:**
```cpp
r_draw_multiview = false;
```

### Cubemap Shadow Example

```cpp
// Set views for the six cubemap faces
const vec3_t cubemapAngles[] = {
    {0, 0, 0},     // +X (right)
    {0, 180, 0},   // -X (left)
    {-90, 0, 0},   // +Y (up)
    {90, 0, 0},    // -Y (down)
    {0, 90, 0},    // +Z (forward)
    {0, -90, 0}    // -Z (backward)
};

camera_ubo_t CameraUBO{};
for (int i = 0; i < 6; ++i)
{
    VectorCopy(lightOrigin, (*r_refdef.vieworg));
    VectorCopy(cubemapAngles[i], (*r_refdef.viewangles));
    
    R_LoadIdentityForWorldMatrix();
    R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));
    R_SetupCameraView(&CameraUBO.views[i]);
}

CameraUBO.numViews = 6;
GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

r_draw_multiview = true;
R_RenderScene();  // Render six faces in one draw call
r_draw_multiview = false;
```

### CSM Optimization Example

The current CSM renders to four 2048x2048 regions on a 4096x4096 canvas. With multiview, it can be optimized as follows:

```cpp
// Create a 2048x2048 TextureArray (4 layers)
glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, 
             2048, 2048, 4, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

// Set views for the four CSM cascades
camera_ubo_t CameraUBO{};
for (int i = 0; i < 4; ++i)
{
    // Calculate the frustum for each cascade
    SetupCSMFrustum(i, &CameraUBO.views[i]);
}

CameraUBO.numViews = 4;
GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

r_draw_multiview = true;
R_RenderScene();  // Render four CSM layers in one draw call
r_draw_multiview = false;

// The geometry shader automatically sets gl_Layer and renders each view to its matching TextureArray layer
```

## Performance Optimization

### Advantages
1. **Fewer Draw Calls** - Reduce N draw calls to one
2. **Fewer State Changes** - No need to switch FBOs and viewports
3. **Lower CPU Overhead** - Rendering state needs to be set only once
4. **Improved GPU Efficiency** - Better batching and parallelization

### Notes
1. **Geometry Shader Overhead** - Adds some GPU overhead and is suitable for scenes with a moderate vertex count
2. **Maximum Vertex Output Limit** - Currently set to 18 vertices (3*6), suitable for cubemaps
3. **Memory Usage** - Geometry shaders consume more GPU cache

### Performance Comparison

**Traditional approach (drawing six cubemap faces):**
- 6 draw calls
- 6 FBO switches
- 6 viewport settings
- 6 state settings

**Multiview approach:**
- 1 draw call
- 1 state setting
- Geometry shader automatically distributes to six layers

Expected performance improvement: **30-60%** (depending on scene complexity)

## Extending to Other Shaders

The same pattern can be applied to:
- **StudioModel** - Add `STUDIO_MULTIVIEW_ENABLED`
- **Sprite** - Add `SPRITE_MULTIVIEW_ENABLED`
- **TriAPI** - Add `TRIAPI_MULTIVIEW_ENABLED`
- **Portal** - Add `PORTAL_MULTIVIEW_ENABLED`

Each requires:
1. Define a state flag in the header file.
2. Add the macro and geometry shader in the `R_Use*Program` function.
3. Create the corresponding `.geom.glsl` file.
4. Modify `.frag.glsl` to support g_-prefixed input.

## Debugging Recommendations

1. **Validate numViews** - Ensure that `CameraUBO.numViews` is set correctly.
2. **Check Matrices** - Use RenderDoc to inspect each view's transformation matrix.
3. **Validate Layers** - Confirm that `gl_Layer` is set to the corresponding TextureArray layer.
4. **Profile Performance** - Use GPU profiling tools to compare performance before and after multiview.

## Known Limitations

1. Geometry shader maximum output is 18 vertices (suitable for six views).
2. Requires OpenGL 4.3+ support.
3. Does not support MSAA TextureArray targets (requires additional handling).
4. Some mobile GPUs have limited geometry shader support.

## Future Optimization Directions

1. **Mesh Shader** - Replace geometry shaders with Mesh Shaders (requires OpenGL 4.6+)
2. **Multi-Draw Indirect** - Further optimize by combining with MDI
3. **Compute Culling** - Use Compute Shaders for multiview culling
4. **View Instancing** - Explore the ARB_shader_viewport_layer_array extension

## References

- OpenGL 4.4 Geometry Shader Specification
- NVIDIA Multi-View Rendering White Paper
- Cascaded Shadow Maps Implementation Guide

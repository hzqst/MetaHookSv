---
title: MultiviewDepthDebugging
type: note
permalink: metahooksv/multiview-depth-debugging
---

# Multiview Depth-Issue Debugging Guide

## Problem Description

After enabling `WSURF_MULTIVIEW_ENABLED` or `STUDIO_MULTIVIEW_ENABLED`, a RenderDoc frame capture shows incorrect geometry positions in the rendered depth buffer.

## Analysis of Possible Causes

### 1. Coordinate Transformation Flow Review

#### Vertex Shader (Geometry Shader Disabled)
```glsl
// Model space → world space
vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
v_worldpos = worldpos4.xyz;

// World space → clip space
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;
//           = projMatrix * viewMatrix * worldPos
```

#### Geometry Shader (Multiview Mode)
```glsl
// Receive world coordinates from the vertex shader
vec4 worldPos = vec4(v_worldpos[i], 1.0);

// Recalculate clip-space coordinates for every view
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

### 2. Diagnostic Checkpoints

Use RenderDoc for the following checks:

#### Checkpoint 1: Validate CameraUBO Data
```cpp
// Print worldMatrix and projMatrix after R_SetupCameraView
camera_ubo_t CameraUBO{};
for (int i = 0; i < numViews; ++i)
{
    R_SetupCameraView(&CameraUBO.views[i]);
    
    // Print debugging information
    gEngfuncs.Con_Printf("View %d worldMatrix:\n", i);
    for (int row = 0; row < 4; ++row)
    {
        gEngfuncs.Con_Printf("  [%f, %f, %f, %f]\n",
            CameraUBO.views[i].worldMatrix[row][0],
            CameraUBO.views[i].worldMatrix[row][1],
            CameraUBO.views[i].worldMatrix[row][2],
            CameraUBO.views[i].worldMatrix[row][3]);
    }
}
```

#### Checkpoint 2: Compare gl_Position Between Non-Multiview and Multiview

In RenderDoc:
1. Capture one non-multiview rendering frame (`r_draw_multiview = false`)
2. Capture one multiview rendering frame (`r_draw_multiview = true`)
3. Compare the following for the same vertex in both modes:
   - `v_worldpos` - should be identical
   - `gl_Position` - should be identical for view 0

#### Checkpoint 3: Inspect gl_Layer

Verify in RenderDoc:
- Whether `gl_Layer` is correctly set to 0, 1, 2... numViews-1
- Whether every layer's depth texture has content
- Whether each layer's content corresponds to the correct view

### 3. Common Problems

#### Problem A: Incorrect Matrix Order

**Incorrect form:**
```glsl
// ❌ Incorrect: the matrix order is reversed
gl_Position = GetCameraWorldMatrix(viewIdx) * GetCameraProjMatrix(viewIdx) * worldPos;
```

**Correct form:**
```glsl
// ✅ Correct: apply the view transformation before the projection transformation
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

#### Problem B: Misleading worldMatrix Naming

`CameraUBO.views[].worldMatrix` is actually a **view matrix** (world space → camera space), not a model-space-to-world-space transformation.

```cpp
// In R_SetupCameraView
memcpy(view->worldMatrix, r_world_matrix, sizeof(mat4));
// r_world_matrix is actually a view matrix!
```

#### Problem C: Missing EntityMatrix

Confirm that EntityMatrix has been applied in the vertex shader:
```glsl
vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
```

If `v_worldpos` received by the geometry shader does not include the EntityMatrix transformation, that is the problem.

#### Problem D: View Matrices Not Updated

When setting up multiview, ensure the following is called for every view:
```cpp
for (int i = 0; i < numViews; ++i)
{
    // Set the view direction
    VectorCopy(viewAngles[i], (*r_refdef.viewangles));
    VectorCopy(viewOrigin, (*r_refdef.vieworg));
    
    // Rebuild the view matrix
    R_LoadIdentityForWorldMatrix();
    R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));
    
    // Save to CameraUBO
    R_SetupCameraView(&CameraUBO.views[i]);
}
```

### 4. Debugging Code Examples

#### Add Debug Output in the Vertex Shader

```glsl
void main(void)
{
    // ... Normal calculations ...
    
    vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
    v_worldpos = worldpos4.xyz;
    
    gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;
    v_projpos = gl_Position;
    
    // Debug: encode world coordinates into color (for debugging only)
    #ifdef DEBUG_WORLDPOS
        v_debug_color = vec4(
            (v_worldpos.x + 1000.0) / 2000.0,  // Assume the scene is within [-1000, 1000]
            (v_worldpos.y + 1000.0) / 2000.0,
            (v_worldpos.z + 1000.0) / 2000.0,
            1.0
        );
    #endif
}
```

#### Add Debugging in the Geometry Shader

```glsl
#ifdef WSURF_MULTIVIEW_ENABLED
    for (int viewIdx = 0; viewIdx < numViews; ++viewIdx)
    {
        gl_Layer = viewIdx;
        
        for (int i = 0; i < 3; ++i)
        {
            vec4 worldPos = vec4(v_worldpos[i], 1.0);
            
            // Debug: print the world coordinates of the first triangle
            #ifdef DEBUG_MULTIVIEW
                if (gl_PrimitiveIDIn == 0 && i == 0)
                {
                    // GLSL cannot print this directly, but it can encode it into color
                    g_debug_info = vec4(float(viewIdx), worldPos.xyz);
                }
            #endif
            
            gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
            
            // ... Pass through other attributes ...
            
            EmitVertex();
        }
        EndPrimitive();
    }
#endif
```

#### Add Validation Code in C++

```cpp
// In gl_shadow.cpp, add this after setting CameraUBO
camera_ubo_t CameraUBO{};

for (int i = 0; i < 6; ++i)
{
    VectorCopy(args->origin, (*r_refdef.vieworg));
    VectorCopy(cubemapAngles[i], (*r_refdef.viewangles));
    
    R_LoadIdentityForWorldMatrix();
    R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));
    R_SetupCameraView(&CameraUBO.views[i]);
    
    // Debug: validate the view matrix
    auto& view = CameraUBO.views[i];
    gEngfuncs.Con_DPrintf("Cubemap face %d:\n", i);
    gEngfuncs.Con_DPrintf("  viewpos: [%f, %f, %f]\n", 
        view.viewpos[0], view.viewpos[1], view.viewpos[2]);
    gEngfuncs.Con_DPrintf("  vpn: [%f, %f, %f]\n",
        view.vpn[0], view.vpn[1], view.vpn[2]);
    
    // Validate the transformation of a test point
    vec3_t testPoint = {100.0f, 0.0f, 0.0f};
    vec4_t worldPoint = {testPoint[0], testPoint[1], testPoint[2], 1.0f};
    vec4_t viewPoint;
    
    // Manual matrix transformation
    for (int row = 0; row < 4; ++row)
    {
        viewPoint[row] = 0.0f;
        for (int col = 0; col < 4; ++col)
        {
            viewPoint[row] += view.worldMatrix[row][col] * worldPoint[col];
        }
    }
    
    gEngfuncs.Con_DPrintf("  Test point (100,0,0) in view space: [%f, %f, %f, %f]\n",
        viewPoint[0], viewPoint[1], viewPoint[2], viewPoint[3]);
}
```

### 5. RenderDoc Analysis Steps

1. **Capture a Frame** - Press F12 during the shadow pass

2. **Open Texture Viewer**
   - Switch to Depth/Stencil view
   - For a TextureArray, select different array slices to inspect

3. **Inspect the Draw Call**
   - Find the draw call for WorldSurface or StudioModel
   - Inspect `in_vertex` in Vertex Input and `v_worldpos` in vertex output
   - Confirm that `v_worldpos` is reasonable

4. **Inspect Geometry Shader Output**
   - Inspect the value of `gl_Layer`
   - Inspect the value of `gl_Position`
   - Compare whether `gl_Position` in different layers reflects different views

5. **Inspect the Uniform Buffer**
   - Expand CameraUBO
   - Inspect the worldMatrix and projMatrix of `views[0]`, `views[1]`, and so on
   - Validate the value of `numViews`

### 6. Known-Issue Troubleshooting

#### If the Depth Is Identical for All Views
→ `gl_Layer` may not be set correctly, or the geometry shader may not be invoked.

#### If Depth Values Are All 0 or 1
→ The projection matrix may be wrong; inspect near/far plane settings.

#### If Geometry Positions Are Offset
→ The view matrix may be wrong; inspect the `R_SetupPlayerViewWorldMatrix` call.

#### If Only the First View Is Correct
→ `viewIdx` may be used incorrectly in the loop, or CameraUBO may set only `views[0]`.

### 7. Quick Fix Attempts

If the issue is incorrect depth, try the following changes:

#### Change A: Ensure the Correct World Coordinates Are Used

In the geometry shader, confirm that v_worldpos is already the complete world coordinate:

```glsl
// In the multiview path
vec4 worldPos = vec4(v_worldpos[i], 1.0);

// Do not apply EntityMatrix again! v_worldpos already includes it
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

#### Change B: Manually Validate Matrices

Temporarily add a debug path that uses the matrices for view 0:

```glsl
#ifdef DEBUG_USE_VIEW0
    // Temporary: all views use the matrices for view 0
    gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldPos;
#else
    gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
#endif
```

If view 0 is correct, the matrices for the other views are faulty.

### 8. Expected Result

After correct implementation, RenderDoc should show:
- Depth from a different view in every TextureArray layer
- Geometry from different directions in all six Cubemap faces
- Geometry from different distance ranges in all four CSM layers
- Depth values within a reasonable range (not all 0 or 1)
- Geometry positions consistent with the scene's actual layout

## Summary

Most likely issues:
1. ✅ The view matrix in CameraUBO is set incorrectly
2. ✅ Matrix multiplication order in the geometry shader is wrong
3. ✅ `v_worldpos` is not the true world coordinate
4. ✅ Only the first view's matrices are set correctly

Use RenderDoc to investigate them one by one according to the steps above.

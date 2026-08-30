---
title: StudioMultiviewBugFix
type: note
permalink: metahooksv/studio-multiview-bug-fix
---

# Fixing StudioModel Multiview Geometry Distortion

## Problem Description

After enabling `STUDIO_MULTIVIEW_ENABLED`, RenderDoc shows that StudioModel vertices are transformed to incorrect coordinates, completely distorting the model. WorldSurface rendering remains normal.

## Root Cause

`studio_shader.vert.glsl` contains a **vertex-position inconsistency**:

### Problematic Code

```glsl
void main(void)
{
    // 1. Bone transformation produces outvert
    vec3 outvert = vec3(
        dot(vert, vertbone_matrix_0) + vertbone_matrix[0][3],
        dot(vert, vertbone_matrix_1) + vertbone_matrix[1][3],
        dot(vert, vertbone_matrix_2) + vertbone_matrix[2][3]
    );
    
    // 2. Assign it to v_worldpos
    v_worldpos = outvert;  // Line 78
    
    // 3. In certain rendering modes, v_worldpos is modified
    #if defined(OUTLINE_ENABLED)
        outvert = outvert + v_smoothnormal * StudioUBO.r_scale;
        v_worldpos = outvert;  // Line 106: v_worldpos is updated
    #elif defined(STUDIO_NF_CHROME)
        outvert = outvert + v_smoothnormal * StudioUBO.r_scale;
        v_worldpos = outvert;  // Line 111: v_worldpos is updated
    #endif
    
    // 4. gl_Position is calculated using outvert (rather than v_worldpos)
    gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(outvert, 1.0);  // ❌ Incorrect!
    
    v_projpos = gl_Position;
}
```

### Data Flow Analysis

**Normal case (without OUTLINE/CHROME):**
- `v_worldpos = outvert` ✅
- `gl_Position` uses `outvert` ✅
- The two are consistent, and the geometry shader receives the correct `v_worldpos`

**OUTLINE/CHROME modes:**
- `v_worldpos = outvert + offset` ✅ (modified position)
- `gl_Position` uses `outvert` ❌ (unmodified position)
- **Inconsistent!**

### Problem in Multiview

In the geometry shader's multiview path:

```glsl
// Receive v_worldpos from the vertex shader (may include an OUTLINE/CHROME offset)
vec4 worldPos = vec4(v_worldpos[i], 1.0);

// Recalculate gl_Position
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

If `v_worldpos` is inconsistent with the vertex position used to calculate `gl_Position` in the vertex shader, this causes:
1. **Non-multiview mode**: uses the vertex shader's `gl_Position` (based on `outvert`) → correct
2. **Multiview mode**: the geometry shader recalculates it (based on `v_worldpos`) → incorrect!

The result is a distorted model in multiview mode.

## Solution

**Modify line 187 of `studio_shader.vert.glsl`** to use `v_worldpos` instead of `outvert`:

```glsl
// ❌ Before the fix
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(outvert, 1.0);

// ✅ After the fix
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(v_worldpos, 1.0);
```

### Why Does This Fix Work?

1. **Ensures consistency**:
   - The vertex shader uses `v_worldpos` to calculate `gl_Position`
   - The geometry shader uses `v_worldpos` to recalculate `gl_Position`
   - Both use the same input data

2. **Correctly handles every mode**:
   - Regular mode: `v_worldpos = outvert`
   - OUTLINE mode: `v_worldpos = outvert + offset`
   - CHROME mode: `v_worldpos = outvert + offset`
   - In every mode, `gl_Position` is based on the final `v_worldpos`

3. **Compatible with non-multiview mode**:
   - The geometry shader's passthrough path directly passes through `gl_in[i].gl_Position`
   - This `gl_Position` now uses the correct `v_worldpos`

## Why Does WorldSurface Have No Issue?

Compare the WorldSurface vertex shader:

```glsl
vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
worldpos4.xyz += v_normal.xyz * EntityUBO.scale;  // Modify worldpos4
v_worldpos = worldpos4.xyz;                        // Synchronize with v_worldpos

// Calculate gl_Position using worldpos4 (consistent with v_worldpos)
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;  ✅
```

WorldSurface always keeps `v_worldpos` synchronized with the variable used to calculate `gl_Position`, so it has no issue.

## Verification Method

### 1. Compile and Test

Recompile the shaders and enable multiview rendering:

```cpp
r_draw_multiview = true;
r_draw_shadowview = true;
```

### 2. RenderDoc Validation

Inspect in RenderDoc:

**Before the fix:**
- StudioModel vertex positions are distorted
- The model shape is completely wrong
- Artifacts such as stretching and inversion may occur

**After the fix:**
- StudioModel vertex positions are correct
- The model shape is normal
- Depth is correct for every view

### 3. Comparative Testing

Test the following scenarios separately:
- ✅ Regular StudioModel (without OUTLINE/CHROME)
- ✅ StudioModel in OUTLINE mode
- ✅ StudioModel with CHROME material
- ✅ Multiview + Shadow rendering
- ✅ Cubemap shadow
- ✅ CSM shadow

## Technical Summary

### Nature of the Problem

**After introducing a geometry shader, the variable passed from the vertex shader to the geometry shader (such as `v_worldpos`) must be exactly consistent with the variable the vertex shader itself uses to calculate `gl_Position`.**

Otherwise:
- Non-multiview mode: uses the vertex shader's `gl_Position` → one position
- Multiview mode: the geometry shader recalculates using `v_worldpos` → another position
- Result: the same vertex has different positions in the two modes → distortion

### Best Practices

When designing a vertex shader that supports a geometry shader:

1. **Use a single variable**: the variable used to calculate `gl_Position` should match the variable passed to the geometry shader

```glsl
// ✅ Good design
v_worldpos = finalPosition;
gl_Position = projMatrix * viewMatrix * vec4(v_worldpos, 1.0);
```

2. **Avoid local variables**: do not use a variable local only to `main` when calculating `gl_Position`

```glsl
// ❌ Bad design
vec3 localPos = ...;
v_worldpos = localPos + offset1;
gl_Position = projMatrix * viewMatrix * vec4(localPos + offset2, 1.0);  // Inconsistent!
```

3. **Test every path**: ensure all shader branches (such as `#if defined`) correctly update `v_worldpos`

### Debugging Tips

If you encounter a similar issue:

1. **Compare variable values**: compare vertex output and geometry input in RenderDoc
2. **Inspect conditional compilation**: check whether every `#if defined` branch is handled correctly
3. **Test individually**: test multiview and non-multiview modes separately
4. **Use WorldSurface as a reference**: if WorldSurface works but StudioModel fails, the issue is in StudioModel-specific logic

## Related Files

- Modified file: `Build/svencoop/renderer/shader/studio_shader.vert.glsl`
- Related file: `Build/svencoop/renderer/shader/studio_shader.geom.glsl`
- Reference comparison: `Build/svencoop/renderer/shader/wsurf_shader.vert.glsl`

## Conclusion

By ensuring that the vertex shader calculates `gl_Position` using the same value as `v_worldpos`, the StudioModel geometry-distortion issue in multiview mode is resolved. This fix does not affect non-multiview mode and correctly handles every StudioModel rendering mode (regular, OUTLINE, CHROME, and so on).

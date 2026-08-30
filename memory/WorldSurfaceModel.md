---
title: WorldSurfaceModel
type: note
permalink: metahooksv/world-surface-model
---

# WorldSurfaceModel Geometry Data Organization and GPU Storage

## Overview

`WorldSurfaceModel` is the core data structure used to render world geometry. This module converts the engine's raw BSP model data into vertex-buffer and index-buffer formats suitable for modern GPU rendering.

Primary processing function: `R_GenerateWorldSurfaceWorldModel(model_t* mod)`

## Data Structures

### 1. CPU-Side Vertex Data Structures

#### brushvertex_t - Basic Vertex Data
```cpp
typedef struct brushvertex_s
{
    vec3_t  pos;                    // Vertex position (x, y, z)
    vec2_t  texcoord;               // Texture coordinates (u, v)
    vec2_t  lightmaptexcoord;       // Lightmap texture coordinates (u, v)
} brushvertex_t;
```

#### brushvertextbn_t - Tangent-Space Data
```cpp
typedef struct brushvertextbn_s
{
    vec3_t  normal;                 // Normal vector
    vec3_t  s_tangent;              // S tangent vector (texture U direction)
    vec3_t  t_tangent;              // T tangent vector (texture V direction)
} brushvertextbn_t;
```

#### brushinstancedata_t - Instance Data
```cpp
typedef struct brushinstancedata_s
{
    uint16_t packed_matId[2];       // [0]: diffuse texture material ID, [1]: lightmap texture index
    byte     styles[4];             // Light style array
    float    diffusescale;          // Diffuse scale (for the SURF_DRAWTILED flag)
} brushinstancedata_t;
```

### 2. CPU-Side Face Data Structure

#### CWorldSurfaceBrushFace - Face Description
```cpp
class CWorldSurfaceBrushFace
{
public:
    int      index;                 // Face index
    int      flags;                 // Face flags (SURF_*)
    vec3_t   normal;                // Face normal
    vec3_t   s_tangent;             // S tangent
    vec3_t   t_tangent;             // T tangent
    uint32_t poly_count;            // Polygon count
    uint32_t start_index;           // Forward index start position
    uint32_t index_count;           // Forward index count
    uint32_t reverse_start_index;   // Reverse index start position (for water surfaces)
    uint32_t reverse_index_count;   // Reverse index count
    uint32_t instance_index;        // Instance data index
    uint32_t instance_count;        // Instance count
};
```

### 3. GPU-Side Buffer Structures

#### CWorldSurfaceWorldModel - GPU Resource Container
```cpp
class CWorldSurfaceWorldModel
{
public:
    model_t* m_model;                           // Associated model pointer
    GLuint   hVBO[WSURF_VBO_MAX];               // Vertex buffer object array
    GLuint   hEBO;                              // Index buffer object
    GLuint   hVAO;                              // Vertex array object
    std::vector<CWorldSurfaceBrushFace> m_vFaceBuffer;  // Face information array
};
```

**VBO array index definitions:**
- `WSURF_VBO_VERTEX (0)`: Stores `brushvertex_t` data
- `WSURF_VBO_VERTEXTBN (1)`: Stores `brushvertextbn_t` data
- `WSURF_VBO_INSTANCE (2)`: Stores `brushinstancedata_t` data

## Data Processing Workflow

### Stage 1: Buffer Initialization

```cpp
std::vector<brushvertex_t> vVertexDataBuffer;
std::vector<brushvertextbn_t> vVertexTBNDataBuffer;
std::vector<brushinstancedata_t> vInstanceDataBuffer;
std::vector<uint32_t> vIndiceBuffer;

vVertexDataBuffer.reserve(mod->numvertexes);
vInstanceDataBuffer.reserve(mod->numsurfaces);
vIndiceBuffer.reserve(mod->numvertexes * 4);
```

Pre-allocate memory to improve performance and avoid dynamic reallocation.

### Stage 2: Iterate Over All Surfaces

Perform the following processing for every surface in the model:

#### 2.1 Extract Tangent-Space Basis Vectors

```cpp
for (int i = 0; i < mod->numsurfaces; i++)
{
    auto surf = R_GetWorldSurfaceByIndex(mod, i);
    auto pBrushFace = &pWorldModel->m_vFaceBuffer[i];
    
    // Extract tangent vectors from texture information
    VectorCopy(surf->texinfo->vecs[0], pBrushFace->s_tangent);
    VectorCopy(surf->texinfo->vecs[1], pBrushFace->t_tangent);
    VectorNormalize(pBrushFace->s_tangent);
    VectorNormalize(pBrushFace->t_tangent);
    
    // Extract the normal
    VectorCopy(surf->plane->normal, pBrushFace->normal);
    pBrushFace->index = i;
    pBrushFace->flags = surf->flags;
```

#### 2.2 Handle Back-Face Flag

```cpp
    // Reverse all vectors for a back face
    if (surf->flags & SURF_PLANEBACK)
    {
        VectorInverse(pBrushFace->normal);
        VectorInverse(pBrushFace->s_tangent);
        VectorInverse(pBrushFace->t_tangent);
    }
```

#### 2.3 Update Lightmap Texture Count

```cpp
    if (surf->lightmaptexturenum + 1 > g_WorldSurfaceRenderer.iNumLightmapTextures)
        g_WorldSurfaceRenderer.iNumLightmapTextures = surf->lightmaptexturenum + 1;
```

### Stage 3: Process Polygons

Each surface can contain one or more linked lists of polygons.

#### 3.1 Turbulent Surfaces (SURF_DRAWTURB) - Double-Sided Rendering

Turbulent surfaces, such as water, require geometry for both front and back faces:

**Front-Face Geometry Generation:**

```cpp
if (surf->flags & SURF_DRAWTURB)
{
    // Front-face rendering data
    uint32_t nBrushStartIndex = vIndiceBuffer.size();
    
    for (poly = surf->polys; poly; poly = poly->next)
    {
        uint32_t nPolyStartIndex = vVertexDataBuffer.size();
        
        // Extract polygon vertices
        for (int j = 0; j < poly->numverts; j++, v += VERTEXSIZE)
        {
            brushvertex_t tempVertexData;
            VectorCopy(v, tempVertexData.pos);
            tempVertexData.texcoord[0] = v[3];           // U
            tempVertexData.texcoord[1] = v[4];           // V
            tempVertexData.lightmaptexcoord[0] = v[5];   // Lightmap U
            tempVertexData.lightmaptexcoord[1] = v[6];   // Lightmap V
            
            brushvertextbn_t tempVertexTBNData;
            VectorCopy(pBrushFace->normal, tempVertexTBNData.normal);
            VectorCopy(pBrushFace->s_tangent, tempVertexTBNData.s_tangent);
            VectorCopy(pBrushFace->t_tangent, tempVertexTBNData.t_tangent);
            
            vVertexDataBuffer.emplace_back(tempVertexData);
            vVertexTBNDataBuffer.emplace_back(tempVertexTBNData);
        }
        
        // Triangulate: convert the polygon into a triangle list
        std::vector<uint32_t> vTriangleListIndices;
        R_PolygonToTriangleList(vPolyVertices, vTriangleListIndices);
        
        // Add indices
        for (size_t k = 0; k < vTriangleListIndices.size(); ++k)
        {
            vIndiceBuffer.emplace_back(nPolyStartIndex + vTriangleListIndices[k]);
        }
    }
    
    pBrushFace->start_index = nBrushStartIndex;
    pBrushFace->index_count = vIndiceBuffer.size() - nBrushStartIndex;
```

**Back-Face Geometry Generation:**

```cpp
    // Back-face rendering data (reverse index order)
    uint32_t nBrushStartIndex = vIndiceBuffer.size();
    
    for (poly = surf->polys; poly; poly = poly->next)
    {
        // ... Same vertex-data generation ...
        
        // Add indices in reverse order (reverses back-face culling)
        for (size_t k = 0; k < vTriangleListIndices.size(); ++k)
        {
            vIndiceBuffer.emplace_back(nPolyStartIndex + 
                vTriangleListIndices[vTriangleListIndices.size() - 1 - k]);
        }
    }
    
    pBrushFace->reverse_start_index = nBrushStartIndex;
    pBrushFace->reverse_index_count = vIndiceBuffer.size() - nBrushStartIndex;
}
```

#### 3.2 Regular Surfaces - Single-Sided Rendering

```cpp
else
{
    uint32_t nBrushStartIndex = vIndiceBuffer.size();
    
    for (poly = surf->polys; poly; poly = poly->next)
    {
        // ... Same vertex-data generation as for turbulent surfaces ...
        // Generate front-face indices only; reverse indices are unnecessary
    }
    
    pBrushFace->start_index = nBrushStartIndex;
    pBrushFace->index_count = vIndiceBuffer.size() - nBrushStartIndex;
}
```

### Stage 4: Generate Instance Data

Each surface has corresponding instance data for storing material and lighting information:

```cpp
pBrushFace->instance_index = vInstanceDataBuffer.size();

brushinstancedata_t tempInstanceData;

// Material ID: [0] = diffuse texture, [1] = lightmap index
tempInstanceData.packed_matId[0] = ptexture ? R_FindWorldMaterialId(ptexture->gl_texturenum) : 0;
tempInstanceData.packed_matId[1] = surf->lightmaptexturenum;

// Diffuse scale (for tiled textures)
tempInstanceData.diffusescale = (ptexture && (pBrushFace->flags & SURF_DRAWTILED)) 
                                ? 1.0f / ptexture->width : 0;

// Light styles
memcpy(&tempInstanceData.styles, surf->styles, sizeof(surf->styles));

vInstanceDataBuffer.emplace_back(tempInstanceData);
pBrushFace->instance_count = 1;
```

### Stage 5: Upload Data to the GPU

#### 5.1 Create and Upload the Index Buffer (EBO)

```cpp
pWorldModel->hEBO = GL_GenBuffer();
GL_UploadDataToEBOStaticDraw(pWorldModel->hEBO, 
                              sizeof(uint32_t) * vIndiceBuffer.size(), 
                              vIndiceBuffer.data());
```

#### 5.2 Create and Upload Vertex Buffers (VBOs)

**VBO[0] - Basic Vertex Data:**
```cpp
pWorldModel->hVBO[WSURF_VBO_VERTEX] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_VERTEX], 
                              sizeof(brushvertex_t) * vVertexDataBuffer.size(), 
                              vVertexDataBuffer.data());
```

**VBO[1] - Tangent-Space Data:**
```cpp
pWorldModel->hVBO[WSURF_VBO_VERTEXTBN] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_VERTEXTBN], 
                              sizeof(brushvertextbn_t) * vVertexTBNDataBuffer.size(), 
                              vVertexTBNDataBuffer.data());
```

**VBO[2] - Instance Data:**
```cpp
pWorldModel->hVBO[WSURF_VBO_INSTANCE] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_INSTANCE], 
                              sizeof(brushinstancedata_t) * vInstanceDataBuffer.size(), 
                              vInstanceDataBuffer.data());
```

### Stage 6: Configure the Vertex Array Object (VAO)

The VAO configuration defines vertex-attribute layouts and bindings.

```cpp
pWorldModel->hVAO = GL_GenVAO();

GL_BindStatesForVAO(pWorldModel->hVAO, [pWorldModel]() {

    // Bind the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pWorldModel->hEBO);
```

#### 6.1 Configure Basic Vertex Attributes

```cpp
    glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_VERTEX]);
    
    glEnableVertexAttribArray(WSURF_VA_POSITION);            // Position
    glEnableVertexAttribArray(WSURF_VA_TEXCOORD);            // Texture coordinates
    glEnableVertexAttribArray(WSURF_VA_LIGHTMAP_TEXCOORD);   // Lightmap coordinates
    
    glVertexAttribPointer(WSURF_VA_POSITION, 3, GL_FLOAT, false, 
                          sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
    glVertexAttribPointer(WSURF_VA_TEXCOORD, 2, GL_FLOAT, false, 
                          sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
    glVertexAttribPointer(WSURF_VA_LIGHTMAP_TEXCOORD, 2, GL_FLOAT, false, 
                          sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
```

**Vertex attribute index definitions:**
- `WSURF_VA_POSITION (0)`: vec3 - vertex position
- `WSURF_VA_TEXCOORD (2)`: vec2 - texture coordinates
- `WSURF_VA_LIGHTMAP_TEXCOORD (3)`: vec2 - lightmap coordinates

#### 6.2 Configure Tangent-Space Attributes

```cpp
    glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_VERTEXTBN]);
    
    glEnableVertexAttribArray(WSURF_VA_NORMAL);      // Normal
    glEnableVertexAttribArray(WSURF_VA_S_TANGENT);   // S tangent
    glEnableVertexAttribArray(WSURF_VA_T_TANGENT);   // T tangent
    
    glVertexAttribPointer(WSURF_VA_NORMAL, 3, GL_FLOAT, false, 
                          sizeof(brushvertextbn_t), OFFSET(brushvertextbn_t, normal));
    glVertexAttribPointer(WSURF_VA_S_TANGENT, 3, GL_FLOAT, false, 
                          sizeof(brushvertextbn_t), OFFSET(brushvertextbn_t, s_tangent));
    glVertexAttribPointer(WSURF_VA_T_TANGENT, 3, GL_FLOAT, false, 
                          sizeof(brushvertextbn_t), OFFSET(brushvertextbn_t, t_tangent));
```

**Vertex attribute index definitions:**
- `WSURF_VA_NORMAL (1)`: vec3 - normal vector
- `WSURF_VA_S_TANGENT (4)`: vec3 - S tangent vector
- `WSURF_VA_T_TANGENT (5)`: vec3 - T tangent vector

#### 6.3 Configure Instance Attributes

```cpp
    glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_INSTANCE]);
    
    glEnableVertexAttribArray(WSURF_VA_PACKED_MATID);    // Material ID
    glEnableVertexAttribArray(WSURF_VA_STYLES);          // Light styles
    glEnableVertexAttribArray(WSURF_VA_DIFFUSESCALE);    // Diffuse scale
    
    // Material ID (integer type)
    glVertexAttribIPointer(WSURF_VA_PACKED_MATID, 1, GL_UNSIGNED_INT, 
                           sizeof(brushinstancedata_t), 
                           OFFSET(brushinstancedata_t, packed_matId));
    glVertexAttribDivisor(WSURF_VA_PACKED_MATID, 1);  // Instanced rendering
    
    // Light styles (byte array)
    glVertexAttribIPointer(WSURF_VA_STYLES, 4, GL_UNSIGNED_BYTE, 
                           sizeof(brushinstancedata_t), 
                           OFFSET(brushinstancedata_t, styles));
    glVertexAttribDivisor(WSURF_VA_STYLES, 1);
    
    // Diffuse scale
    glVertexAttribPointer(WSURF_VA_DIFFUSESCALE, 1, GL_FLOAT, false, 
                          sizeof(brushinstancedata_t), 
                          OFFSET(brushinstancedata_t, diffusescale));
    glVertexAttribDivisor(WSURF_VA_DIFFUSESCALE, 1);
});
```

**Instance attribute index definitions:**
- `WSURF_VA_PACKED_MATID (6)`: uint - packed material ID
- `WSURF_VA_STYLES (7)`: uvec4 - light style array
- `WSURF_VA_DIFFUSESCALE (8)`: float - diffuse scale factor

**Instanced rendering notes:**
- `glVertexAttribDivisor(index, 1)` means this attribute is updated once per instance
- Allows instanced rendering with `glDrawElementsInstanced`
- Each surface uses different material and lighting parameters

## Memory Layout Diagram

```
CPU-side temporary buffers                 GPU-side buffers
┌──────────────────────┐                ┌──────────────────────┐
│  vVertexDataBuffer   │   =========>   │  hVBO[WSURF_VBO_     │
│  (brushvertex_t)     │                │       VERTEX]        │
│  - pos               │                │                      │
│  - texcoord          │                │  GL_ARRAY_BUFFER     │
│  - lightmaptexcoord  │                │  STATIC_DRAW         │
└──────────────────────┘                └──────────────────────┘

┌──────────────────────┐                ┌──────────────────────┐
│ vVertexTBNDataBuffer │   =========>   │  hVBO[WSURF_VBO_     │
│ (brushvertextbn_t)   │                │       VERTEXTBN]     │
│  - normal            │                │                      │
│  - s_tangent         │                │  GL_ARRAY_BUFFER     │
│  - t_tangent         │                │  STATIC_DRAW         │
└──────────────────────┘                └──────────────────────┘

┌──────────────────────┐                ┌──────────────────────┐
│ vInstanceDataBuffer  │   =========>   │  hVBO[WSURF_VBO_     │
│(brushinstancedata_t) │                │       INSTANCE]      │
│  - packed_matId      │                │                      │
│  - styles            │                │  GL_ARRAY_BUFFER     │
│  - diffusescale      │                │  STATIC_DRAW         │
└──────────────────────┘                └──────────────────────┘

┌──────────────────────┐                ┌──────────────────────┐
│   vIndiceBuffer      │   =========>   │       hEBO           │
│   (uint32_t)         │                │                      │
│   - triangle indices  │                │ GL_ELEMENT_ARRAY_    │
│                      │                │      BUFFER          │
└──────────────────────┘                └──────────────────────┘

                                        ┌──────────────────────┐
                                        │       hVAO           │
                                        │  (vertex array object)│
                                        │  Binds all buffers    │
                                        │  and vertex attributes│
                                        └──────────────────────┘
```

## Vertex Attribute Mapping Table

| Attribute Index | Attribute Name            | Type      | Data Source         | Purpose                  | Instanced |
|---------|--------------------------|----------|-------------------|-------------------------|--------|
| 0       | WSURF_VA_POSITION        | vec3     | brushvertex_t     | Vertex position          | ❌     |
| 1       | WSURF_VA_NORMAL          | vec3     | brushvertextbn_t  | Normal vector            | ❌     |
| 2       | WSURF_VA_TEXCOORD        | vec2     | brushvertex_t     | Texture coordinates      | ❌     |
| 3       | WSURF_VA_LIGHTMAP_TEXCOORD| vec2    | brushvertex_t     | Lightmap coordinates     | ❌     |
| 4       | WSURF_VA_S_TANGENT       | vec3     | brushvertextbn_t  | S tangent (normal map)   | ❌     |
| 5       | WSURF_VA_T_TANGENT       | vec3     | brushvertextbn_t  | T tangent (normal map)   | ❌     |
| 6       | WSURF_VA_PACKED_MATID    | uint     | brushinstancedata_t| Packed material ID (diffuse + lightmap) | ✅ |
| 7       | WSURF_VA_STYLES          | uvec4    | brushinstancedata_t| Light styles             | ✅     |
| 8       | WSURF_VA_DIFFUSESCALE    | float    | brushinstancedata_t| Tiled-texture scale      | ✅     |

## Key Technical Points

### 1. Separate Three-Buffer Storage Strategy

Vertex data is separated into three independent VBOs:
- **Advantages**:
  - Improves cache locality: shaders can access only the data they need
  - Easier updates: instance data can be updated independently without affecting geometry data
  - Supports instanced rendering: instance data uses `glVertexAttribDivisor`
- **Disadvantages**:
  - Requires more buffer objects
  - VAO configuration is slightly more complex

### 2. Polygon Triangulation

Use `R_PolygonToTriangleList` to convert any convex polygon into a triangle list:
- Raw BSP data uses polygon representations
- The GPU can render only triangles

### 3. Double-Sided Geometry for Turbulent Surfaces

Turbulent surfaces, such as water, must be visible from both sides:
- Generate two index sets (front and back)
- Implement back-face indices by reversing their order
- Supports viewing the water surface from underwater

### 4. Instanced Rendering Preparation

Although the current code has only one instance per surface, the architecture supports instancing:
- Uses `glVertexAttribDivisor(index, 1)`
- Can be extended to batch-render surfaces with identical geometry but different materials
- Reduces the number of draw calls

### 5. Static Data Optimization

Use the `GL_STATIC_DRAW` flag:
- Data is not modified after initialization
- The GPU can store data in faster video-memory regions
- Suitable for static scenes such as world geometry

### 6. Tangent-Space Calculation

Extract tangent vectors from the texture-coordinate system:
- `s_tangent` corresponds to the texture U direction
- `t_tangent` corresponds to the texture V direction
- Together with the normal, they form the TBN matrix for normal mapping

### 7. Material ID Packing

Use `uint16_t packed_matId[2]` to store two material indices:
- `[0]`: diffuse texture material ID
- `[1]`: lightmap texture index
- Saves storage space and reduces vertex-data size

## Rendering Workflow (Brief)

Although this document mainly focuses on data organization, the generated GPU resources are used roughly as follows:

1. Bind the VAO: `glBindVertexArray(pWorldModel->hVAO)`
2. Bind material textures and lightmaps
3. Set shader uniforms (MVP matrices and so on)
4. Draw calls:
   - Regular surfaces: `glDrawElementsBaseVertex(GL_TRIANGLES, count, GL_UNSIGNED_INT, start, base)`
   - Turbulent surface front faces: use `start_index` and `index_count`
   - Turbulent surface back faces: use `reverse_start_index` and `reverse_index_count`

## Performance Considerations

1. **Pre-allocate memory**: use `reserve()` to avoid dynamic reallocation
2. **Batch uploads**: upload all data at once to avoid multiple GPU transfers
3. **Index reuse**: reuse vertex data through the EBO
4. **Cache-friendly**: structure layouts account for alignment and access patterns
5. **Static storage**: use `STATIC_DRAW` to hint that the GPU should optimize the storage location

## Related Files

- **Implementation file**: `Plugins/Renderer/gl_wsurf.cpp`
- **Header files**: `Plugins/Renderer/gl_wsurf.h`, `Plugins/Renderer/gl_common.h`
- **Shaders**: `Build/svencoop/renderer/shader/wsurf_*.glsl` (inferred)

## Summary

The `R_GenerateWorldSurfaceWorldModel` function performs the complete conversion from BSP model data to a modern GPU rendering pipeline:

1. Iterate over all surfaces and extract geometry and texture information
2. Calculate tangent-space basis vectors in preparation for normal mapping
3. Triangulate polygons and generate the index buffer
4. Generate double-sided geometry for turbulent surfaces
5. Create instance data supporting multiple materials and light styles
6. Upload all data to separate GPU buffers
7. Configure the VAO to define the complete vertex-attribute layout

This design both maintains compatibility with the GoldSrc engine's BSP format and fully leverages modern OpenGL features for efficient world-geometry rendering.

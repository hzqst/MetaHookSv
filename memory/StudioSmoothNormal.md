---
title: StudioSmoothNormal
type: note
permalink: metahooksv/studio-smooth-normal
---

# Studio Model Smooth-Normal Generation Flow

This document describes the smooth-normal generation process for Studio Models (.mdl models) in the Renderer plugin.

## Overview

Smooth normals improve model rendering. Original GoldSrc Studio Model normals are calculated from face normals, which can produce noticeable hard edges between adjacent faces. By calculating smooth normals, rendering can use them for effects such as normal-expanded outlines.

## Data structures

### Vertex data structures

```cpp
// Base vertex data
class studiovertexbase_t
{
public:
    vec3_t  pos{};           // Vertex position
    vec3_t  normal{};        // Original normal
    vec2_t  texcoord{};      // Texture coordinates
    byte    packedbone[4]{}; // Bone indices (packedbone[0] for the vertex bone; packedbone[1] for the normal bone)
};

// TBN (tangent, bitangent, smooth normal) data
class studiovertextbn_t
{
public:
    vec3_t  tangent{};       // Tangent
    vec3_t  bitangent{};     // Bitangent
    vec3_t  smoothnormal{};  // Smooth normal
};
```

### Quantized vectors (for hash lookup)

```cpp
class CQuantizedVector
{
public:
    int64_t m_x{};           // Quantized X coordinate (original value × 1000)
    int64_t m_y{};           // Quantized Y coordinate
    int64_t m_z{};           // Quantized Z coordinate
    int m_boneindex{ -1 };   // Bone index
};
```

Quantized vectors convert floating-point coordinates to integers (with 0.001 precision), enabling fast hash-table lookups of vertices at the same position. They also include the bone index to ensure that same-position vertices on different bones are not incorrectly merged.

### Face-normal storage

```cpp
class CFaceNormalStorage
{
public:
    CVector3List weightedNormals;    // Weighted-normal list
    float normalTotalFactor{ 0 };    // Total weighting factor
    vec3_t averageNormal{};          // Calculated average normal
    
    CVector3List edges;              // Edge-vector list (for degenerate cases)
    vec3_t averageEdge{};            // Average edge vector
    bool bHasAverageEdge{ false };   // Whether to use the edge vector as the normal
};
```

## Complete flow

### 1. Entry function

Smooth normals are calculated in `R_PrepareSmoothNormalForRenderMesh`:

```
R_PrepareTBNForRenderSubmodel
    └── R_PrepareSmoothNormalForRenderMesh  ← Smooth-normal entry point
            ├── CalculateFaceNormalHashMap   ← Step 1: build the face-normal hash map
            ├── CalculateAverageNormal       ← Step 2: calculate average normals
            └── GetSmoothNormal              ← Step 3: obtain a smooth normal for every vertex
```

### 2. Step 1: Build the face-normal hash map

**Function**: `CalculateFaceNormalHashMap`

Traverse all triangles and collect weighted-normal information from each vertex's incident faces.

#### 2.1 Traverse triangles

```cpp
for (int i = 0; i < vIndicesBuffer.size(); i += 3)
{
    // Obtain the triangle's three vertex indices (counter-clockwise order)
    int idx0 = vIndicesBuffer[i + 2];
    int idx1 = vIndicesBuffer[i + 1];
    int idx2 = vIndicesBuffer[i + 0];
    
    // Obtain vertex positions, normals, and bone data
    // ...
}
```

#### 2.2 Calculate the face normal

```cpp
// Calculate two edge vectors
vec3_t edge1, edge2;
VectorSubtract(vTrianglePos[1], vTrianglePos[0], edge1);
VectorSubtract(vTrianglePos[2], vTrianglePos[0], edge2);

// The cross product yields the face normal (length = triangle area × 2)
vec3_t faceNormalWeighted;
CrossProduct(edge1, edge2, faceNormalWeighted);
float triangleArea = VectorLength(faceNormalWeighted);

// Normalize to obtain the unit face normal
vec3_t faceNormal;
VectorScale(faceNormalWeighted, 1.0f / triangleArea, faceNormal);
```

#### 2.3 Calculate vertex angle weights

For every triangle vertex, calculate the interior angle at that vertex as the weighting factor:

```cpp
// nextId obtains the index of the next vertex in the triangle
int nextId[4] = { 1, 2, 0, 1 };

for (int j = 0; j < 3; j++)
{
    // Calculate the two adjacent edges of the current vertex
    vec3_t edgeA, edgeB;
    VectorSubtract(vTrianglePos[nextId[j]], vTrianglePos[j], edgeA);
    VectorSubtract(vTrianglePos[nextId[j + 1]], vTrianglePos[j], edgeB);
    
    // Normalize the edge vectors
    vec3_t edgeANorm, edgeBNorm;
    VectorScale(edgeA, 1.0f / edgeALength, edgeANorm);
    VectorScale(edgeB, 1.0f / edgeBLength, edgeBNorm);
    
    // Calculate the vertex interior angle (radians)
    float dotProduct = DotProduct(edgeANorm, edgeBNorm);
    dotProduct = math_clamp(dotProduct, -1.0f, 1.0f);
    float angle = std::acos(dotProduct);
    
    // Combined weight = angle × area
    float combinedWeight = angle * triangleArea;
}
```

**Weighting algorithm**:

Uses **angle-area weighting**:
- **Angle weight**: the larger the interior angle at a vertex, the greater that face's contribution to the vertex normal.
- **Area weight**: larger triangles contribute more to the normal.
- Combined weight = angle × area

This method produces more natural smoothing than a simple average.

#### 2.4 Store in the hash table

```cpp
// Quantize the vertex position as the hash key
CQuantizedVector quantizedVertexPos(vTrianglePos[j], vertbones[j]);

// Calculate the weighted normal
vertex3f_t vWeightedNormal{};
VectorScale(faceNormal, combinedWeight, vWeightedNormal.v);

// Store in the hash table
auto it = FaceNormalHashMap.find(quantizedVertexPos);
if (it == FaceNormalHashMap.end())
{
    // Create a new entry
    faceNormalStorage = std::make_shared<CFaceNormalStorage>();
    faceNormalStorage->weightedNormals.emplace_back(vWeightedNormal);
    faceNormalStorage->normalTotalFactor += combinedWeight;
    // Also store edge vectors (for degenerate-case handling)
    faceNormalStorage->edges.emplace_back(negEdgeA);
    faceNormalStorage->edges.emplace_back(negEdgeB);
    FaceNormalHashMap[quantizedVertexPos] = faceNormalStorage;
}
else
{
    // Accumulate into the existing entry
    faceNormalStorage = it->second;
    faceNormalStorage->weightedNormals.emplace_back(vWeightedNormal);
    faceNormalStorage->normalTotalFactor += combinedWeight;
    faceNormalStorage->edges.emplace_back(negEdgeA);
    faceNormalStorage->edges.emplace_back(negEdgeB);
}
```

### 3. Step 2: Calculate average normals

**Function**: `CalculateAverageNormal`

Traverse every hash-table entry and calculate the average normal for that position.

```cpp
for (auto it = FaceNormalHashMap.begin(); it != FaceNormalHashMap.end(); it++)
{
    const auto& FaceNormalStorage = it->second;
    vec3_t averageNormal = { 0, 0, 0 };
    
    // Accumulate all weighted normals
    for (const auto& weightedNormal : FaceNormalStorage->weightedNormals)
    {
        VectorAdd(averageNormal, weightedNormal.v, averageNormal);
    }
    
    // Normalize
    float averageNormalLength = VectorLength(averageNormal);
    
    if (averageNormalLength < 0.001f)
    {
        // Degenerate case: a zero-thickness face (such as a double-sided face)
        // Use the average of edge vectors as the normal
        // ...
        FaceNormalStorage->bHasAverageEdge = true;
    }
    else
    {
        VectorScale(averageNormal, 1.0f / averageNormalLength, FaceNormalStorage->averageNormal);
    }
}
```

#### Degenerate-case handling

When the average-normal length approaches zero (typically on zero-thickness double-sided faces, where front and back normals cancel each other), use the average edge vector as the replacement normal:

```cpp
vec3_t averageEdge = { 0, 0, 0 };
for (const auto& edge : FaceNormalStorage->edges)
{
    VectorAdd(averageEdge, edge.v, averageEdge);
}
VectorScale(averageEdge, 1.0f / (float)FaceNormalStorage->edges.size(), averageEdge);

float averageEdgeLength = VectorLength(averageEdge);
if (averageEdgeLength > 0.001f)
{
    VectorScale(averageEdge, 1.0f / averageEdgeLength, FaceNormalStorage->averageEdge);
    FaceNormalStorage->bHasAverageEdge = true;
}
```

### 4. Step 3: Obtain the smooth normal for every vertex

**Function**: `GetSmoothNormal`

Look up the corresponding average normal from the hash table by vertex position, then blend it smoothly.

```cpp
void GetSmoothNormal(
    CStudioModelRenderData* pRenderData,
    const vec3_t VertexPos,
    const vec3_t VertexNorm,
    int vertbone,
    const CFaceNormalHashMap& FaceNormalHashMap,
    vec3_t outNormal)
{
    CQuantizedVector quantizedVertexPos(VertexPos, vertbone);
    auto it = FaceNormalHashMap.find(quantizedVertexPos);
    
    if (it != FaceNormalHashMap.end())
    {
        const auto& FaceNormalStorage = it->second;
        
        // Case 1: degenerate face; use the edge vector
        if (FaceNormalStorage->bHasAverageEdge)
        {
            VectorCopy(FaceNormalStorage->averageEdge, outNormal);
            return;
        }
        
        // Case 2: check the difference between the average and original normals
        float dotProduct = DotProduct(FaceNormalStorage->averageNormal, VertexNorm);
        
        // If the angle exceeds 60 degrees (cos(60°) = 0.5), the difference is too large
        if (dotProduct < 0.5f)
        {
            // Keep the original normal (preserve the hard-edge effect)
            VectorCopy(VertexNorm, outNormal);
            return;
        }
        
        // Case 3: use interpolation for a smooth transition
        float blendFactor = math_clamp((dotProduct - 0.5f) / 0.5f, 0.0f, 1.0f);
        vec3_t lerpResult;
        vec3_lerp(VertexNorm, FaceNormalStorage->averageNormal, blendFactor, lerpResult);
        VectorNormalize(lerpResult);
        VectorCopy(lerpResult, outNormal);
        return;
    }
    
    // Case 4: no corresponding entry found; use the original normal
    VectorCopy(VertexNorm, outNormal);
}
```

#### Smooth blending strategy

To avoid unnatural smoothing at hard edges, the algorithm uses an angle-based blending strategy:

| Condition | Handling |
|------|----------|
| Angle > 60° (dot < 0.5) | Keep the original normal and preserve the hard edge |
| Angle ∈ [0°, 60°] | Use interpolated blending, `blendFactor ∈ [0, 1]` |

Blending formula:
```
blendFactor = clamp((dotProduct - 0.5) / 0.5, 0, 1)
smoothNormal = lerp(originalNormal, averageNormal, blendFactor)
```

### 5. Apply to the vertex buffer

Finally, store the calculated smooth normals in `vVertexTBNBuffer`:

```cpp
for (size_t i = 0; i < vVertexTBNBuffer.size(); ++i)
{
    GetSmoothNormal(
        pRenderData,
        vVertexBaseBuffer[i].pos,
        vVertexBaseBuffer[i].normal,
        vVertexBaseBuffer[i].packedbone[0],
        FaceNormalHashMap,
        vVertexTBNBuffer[i].smoothnormal  // Output here
    );
}
```

## Flowchart

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Smooth-Normal Generation Flow                       │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Step 1: CalculateFaceNormalHashMap                                  │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Traverse all triangles                                           │  │
│  │   ├── Calculate face normals                                     │  │
│  │   ├── Calculate the angle weight of each vertex                  │  │
│  │   ├── Combined weight = angle × area                             │  │
│  │   └── Store weighted normals in the hash map (key = quantized position + bone index) │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Step 2: CalculateAverageNormal                                      │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Traverse every hash-map entry                                    │  │
│  │   ├── Accumulate all weighted normals                            │  │
│  │   ├── Normalize to obtain the average normal                     │  │
│  │   └── Handle degenerate cases (use edge vectors)                 │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Step 3: GetSmoothNormal (each vertex)                               │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Look up the hash map for the average normal                      │  │
│  │   ├── Degenerate case → use edge vector                          │  │
│  │   ├── Angle > 60° → keep original normal (hard edge)             │  │
│  │   ├── Angle ≤ 60° → interpolated blend                           │  │
│  │   └── Not found → use original normal                            │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Output: vVertexTBNBuffer[i].smoothnormal                            │
└─────────────────────────────────────────────────────────────────────┘
```

## GPU usage

After calculation, smooth-normal data is uploaded to the GPU through a VBO:

```cpp
// Upload TBN data to the VBO
m_pRenderData->hVBO[STUDIO_VBO_TBN] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(
    m_pRenderData->hVBO[STUDIO_VBO_TBN],
    m_vVertexTBNBuffer.size() * sizeof(studiovertextbn_t),
    m_vVertexTBNBuffer.data()
);

// Set vertex attributes
glVertexAttribPointer(
    STUDIO_VA_SMOOTHNORMAL,  // Attribute location
    3,                       // Component count
    GL_FLOAT,                // Data type
    false,                   // Whether to normalize
    sizeof(studiovertextbn_t),
    OFFSET(studiovertextbn_t, smoothnormal)
);
```

In shaders, smooth normals are used for:
- Normal-expanded outlines (Outline)
- Normal-expanded glow (Entity Glow Effects)

## Related files

- `Plugins/Renderer/gl_studio.cpp` - Smooth-normal calculation implementation
- `Plugins/Renderer/gl_studio.h` - Vertex data-structure definitions
- `Plugins/Renderer/mathlib2.h` - `CQuantizedVector` and hasher definitions

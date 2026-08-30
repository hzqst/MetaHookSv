---
title: DrawTriAPI
type: note
permalink: metahooksv/draw-tri-api
---

# Renderer Plugin - TriAPI Rendering Flow in Detail

## Overview

TriAPI (Triangle API) is an immediate-mode drawing API that the GoldSrc engine provides to client DLLs, allowing game code to draw custom geometry directly. The Renderer plugin intercepts and reimplements TriAPI, replacing the original immediate mode with batched VBO drawing in modern OpenGL.

---

## TriAPI Call Chain

### Complete Call Flow

```
R_RenderScene()
└── ClientDLL_DrawNormalTriangles()     // Opaque triangles
    └── gPrivateFuncs.ClientDLL_DrawNormalTriangles()
        └── [Client DLL code]
            └── gEngfuncs.pTriAPI->...

R_RenderScene()
└── R_DrawTransEntities()               // Transparent entities
    └── ClientDLL_DrawTransparentTriangles()
        └── gPrivateFuncs.ClientDLL_DrawTransparentTriangles()
            └── [Client DLL code]
                └── gEngfuncs.pTriAPI->...
                    ├── RenderMode()
                    ├── Begin()
                    ├── Color4f() / Color4ub()
                    ├── TexCoord2f()
                    ├── Vertex3f() / Vertex3fv()
                    └── End()
```

---

## TriAPI Interface Functions

### Core Rendering Flow

```cpp
// 1. Set render mode
gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);

// 2. Begin drawing
gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);

// 3. Submit vertices
gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
gEngfuncs.pTriAPI->Vertex3f(x, y, z);

// 4. Finish drawing
gEngfuncs.pTriAPI->End();
```

### TriAPI Function List

#### Rendering Control
- **RenderMode(int mode)** - Sets the render mode
- **Begin(int primitiveCode)** - Begins drawing a primitive
- **End()** - Finishes drawing and submits it

#### Vertex Attributes
- **Color4f(float r, float g, float b, float a)** - Sets color (floating point)
- **Color4ub(byte r, byte g, byte b, byte a)** - Sets color (byte)
- **TexCoord2f(float s, float t)** - Sets texture coordinates
- **Vertex3f(float x, float y, float z)** - Submits a vertex
- **Vertex3fv(float* v)** - Submits a vertex (array)

#### Other Features
- **Brightness(float brightness)** - Sets brightness
- **Color4fRendermode(float r, float g, float b, float a)** - Color with render mode
- **GetMatrix(int mode, float* matrix)** - Gets a matrix
- **BoxInPVS(float* mins, float* maxs)** - PVS visibility test
- **Fog(float* color, float start, float end, int enable)** - Fog control
- **FogParams(float density, int skybox)** - Fog parameters

---

## Detailed Function Analysis

### 1. triapi_RenderMode() - Set Render Mode
**Location**: `gl_rmain.cpp:1310-1314`

```cpp
void triapi_RenderMode(int mode) {
    gTriAPICommand.RenderMode = mode;
}
```

**Supported render modes**:
- `kRenderNormal` (0) - Opaque
- `kRenderTransColor` (1) - Color transparency
- `kRenderTransTexture` (2) - Texture transparency
- `kRenderGlow` (3) - Glow
- `kRenderTransAlpha` (4) - Alpha transparency
- `kRenderTransAdd` (5) - Additive blending

---

### 2. triapi_Begin() - Begin Drawing
**Location**: `gl_rmain.cpp:1315-1336`

```cpp
void triapi_Begin(int primitiveCode) {
    const int tri_GL_Modes[7] = {
        GL_TRIANGLES,       // TRI_TRIANGLES (0)
        GL_TRIANGLE_FAN,    // TRI_TRIANGLE_FAN (1)
        GL_QUADS,           // TRI_QUADS (2)
        GL_POLYGON,         // TRI_POLYGON (3)
        GL_LINES,           // TRI_LINES (4)
        GL_TRIANGLE_STRIP,  // TRI_TRIANGLE_STRIP (5)
        GL_QUAD_STRIP       // TRI_QUAD_STRIP (6)
    };
    
    gTriAPICommand.GLPrimitiveCode = tri_GL_Modes[primitiveCode];
    gTriAPICommand.DrawRenderMode = gTriAPICommand.RenderMode;
}
```

**Primitive types**:
- **TRI_TRIANGLES** - Independent triangles
- **TRI_TRIANGLE_FAN** - Triangle fan
- **TRI_QUADS** - Quadrilaterals
- **TRI_POLYGON** - Polygon
- **TRI_LINES** - Line segments
- **TRI_TRIANGLE_STRIP** - Triangle strip
- **TRI_QUAD_STRIP** - Quadrilateral strip

---

### 3. triapi_Color4f() / triapi_Color4ub() - Set Color
**Location**: `gl_rmain.cpp:1775-1825`

```cpp
void triapi_Color4f(float r, float g, float b, float a) {
    gTriAPICommand.DrawColor[0] = r;
    gTriAPICommand.DrawColor[1] = g;
    gTriAPICommand.DrawColor[2] = b;
    gTriAPICommand.DrawColor[3] = a;
    
    // Special handling for TransAlpha mode
    if (gTriAPICommand.RenderMode == kRenderTransAlpha) {
        // Premultiply color by alpha
    }
}

void triapi_Color4ub(byte r, byte g, byte b, byte a) {
    gTriAPICommand.DrawColor[0] = r / 255.0;
    gTriAPICommand.DrawColor[1] = g / 255.0;
    gTriAPICommand.DrawColor[2] = b / 255.0;
    gTriAPICommand.DrawColor[3] = a / 255.0;
}
```

---

### 4. triapi_TexCoord2f() - Set Texture Coordinates
**Location**: `gl_rmain.cpp:1876-1880`

```cpp
void triapi_TexCoord2f(float s, float t) {
    gTriAPICommand.TexCoord[0] = s;
    gTriAPICommand.TexCoord[1] = t;
}
```

---

### 5. triapi_Vertex3f() / triapi_Vertex3fv() - Submit Vertices
**Location**: `gl_rmain.cpp:1830-1874`

```cpp
void triapi_Vertex3f(float x, float y, float z) {
    vec3_t pos = { x, y, z };
    
    // Store positions for polygon triangulation
    gTriAPICommand.Positions.emplace_back(pos);
    
    // Build vertex data
    triapivertex_t vertex;
    VectorCopy(pos, vertex.pos);
    vertex.texcoord[0] = gTriAPICommand.TexCoord[0];
    vertex.texcoord[1] = gTriAPICommand.TexCoord[1];
    VectorCopy4(gTriAPICommand.DrawColor, vertex.color);
    
    gTriAPICommand.Vertices.push_back(vertex);
}

void triapi_Vertex3fv(float* v) {
    triapi_Vertex3f(v[0], v[1], v[2]);
}
```

---

### 6. triapi_End() - Finish Drawing and Submit
**Location**: `gl_rmain.cpp:1345-1773`

This is the core TriAPI function. It converts collected vertices into indexed triangles and submits them to the GPU.

#### 6.1 Convert Primitives to Triangles

```cpp
void triapi_End() {
    size_t n = gTriAPICommand.Vertices.size();
    
    // Generate indices according to primitive type
    if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLES) {
        // One triangle per three vertices
        for (size_t i = 0; i < n; i += 3) {
            if (i + 2 < n) {
                gTriAPICommand.Indices.push_back(i);
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i + 2);
            }
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLE_FAN) {
        // Fan: all triangles share the first vertex
        for (size_t i = 1; i < n - 1; ++i) {
            gTriAPICommand.Indices.push_back(0);
            gTriAPICommand.Indices.push_back(i);
            gTriAPICommand.Indices.push_back(i + 1);
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_QUADS) {
        // Convert each quadrilateral to two triangles
        for (size_t i = 0; i < n; i += 4) {
            if (i + 3 < n) {
                // Triangle 1: v0, v1, v2
                gTriAPICommand.Indices.push_back(i + 0);
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i + 2);
                // Triangle 2: v2, v3, v0
                gTriAPICommand.Indices.push_back(i + 2);
                gTriAPICommand.Indices.push_back(i + 3);
                gTriAPICommand.Indices.push_back(i + 0);
            }
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_POLYGON) {
        // Polygon triangulation (Ear Clipping algorithm)
        R_PolygonToTriangleList(gTriAPICommand.Positions, 
                                gTriAPICommand.Indices);
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLE_STRIP) {
        // Triangle strip: each new vertex forms a triangle with the preceding two vertices
        for (size_t i = 0; i < n - 2; ++i) {
            if (i % 2 == 0) {
                // Even: forward order
                gTriAPICommand.Indices.push_back(i);
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i + 2);
            } else {
                // Odd: reverse order (preserves winding order)
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i);
                gTriAPICommand.Indices.push_back(i + 2);
            }
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_QUAD_STRIP) {
        // Quadrilateral strip
        for (size_t i = 0; i + 3 < n; i += 2) {
            uint32_t v0 = i, v1 = i + 1, v2 = i + 2, v3 = i + 3;
            // Triangle 1: v0, v1, v3
            gTriAPICommand.Indices.push_back(v0);
            gTriAPICommand.Indices.push_back(v1);
            gTriAPICommand.Indices.push_back(v3);
            // Triangle 2: v0, v3, v2
            gTriAPICommand.Indices.push_back(v0);
            gTriAPICommand.Indices.push_back(v3);
            gTriAPICommand.Indices.push_back(v2);
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_LINES) {
        // Line segments: use vertex indices directly
        for (size_t i = 0; i < n; i++) {
            gTriAPICommand.Indices.push_back(i);
        }
    }
}
```

#### 6.2 VAO and Ring Buffer Initialization

```cpp
if (!gTriAPICommand.hVAO) {
    gTriAPICommand.hVAO = GL_GenVAO();
    
    // Create ring buffers
    if (!g_TriAPIVertexBuffer) {
        g_TriAPIVertexBuffer = GL_CreatePMBRingBuffer(
            "TriAPIVertexBuffer", 32 * 1024 * 1024, GL_ARRAY_BUFFER);
    }
    
    if (!g_TriAPIIndexBuffer) {
        g_TriAPIIndexBuffer = GL_CreatePMBRingBuffer(
            "TriAPIIndexBuffer", 8 * 1024 * 1024, GL_ELEMENT_ARRAY_BUFFER);
    }
    
    // Configure the VAO
    GL_BindStatesForVAO(gTriAPICommand.hVAO, [] {
        glBindBuffer(GL_ARRAY_BUFFER, g_TriAPIVertexBuffer->GetGLBufferObject());
        
        // Position
        glVertexAttribPointer(TRIAPI_VA_POSITION, 3, GL_FLOAT, false, 
                             sizeof(triapivertex_t), OFFSET(triapivertex_t, pos));
        glEnableVertexAttribArray(TRIAPI_VA_POSITION);
        
        // TexCoord
        glVertexAttribPointer(TRIAPI_VA_TEXCOORD, 2, GL_FLOAT, false, 
                             sizeof(triapivertex_t), OFFSET(triapivertex_t, texcoord));
        glEnableVertexAttribArray(TRIAPI_VA_TEXCOORD);
        
        // Color
        glVertexAttribPointer(TRIAPI_VA_COLOR, 4, GL_FLOAT, false, 
                             sizeof(triapivertex_t), OFFSET(triapivertex_t, color));
        glEnableVertexAttribArray(TRIAPI_VA_COLOR);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_TriAPIIndexBuffer->GetGLBufferObject());
    });
}
```

#### 6.3 Data Upload

```cpp
uint32_t verticesCount = gTriAPICommand.Vertices.size();
uint32_t indiceCount = gTriAPICommand.Indices.size();

size_t vertexDataSize = verticesCount * sizeof(triapivertex_t);
size_t indexDataSize = indiceCount * sizeof(uint32_t);

// Allocate space from the ring buffer
CPMBRingBufferAllocation vertexAllocation;
if (!g_TriAPIVertexBuffer->Allocate(vertexDataSize, vertexAllocation)) {
    gEngfuncs.Con_DPrintf("triapi_End: g_TriAPIVertexBuffer full!\n");
    return;
}

CPMBRingBufferAllocation indexAllocation;
if (!g_TriAPIIndexBuffer->Allocate(indexDataSize, indexAllocation)) {
    gEngfuncs.Con_DPrintf("triapi_End: g_TriAPIIndexBuffer full!\n");
    return;
}

// Copy data
memcpy(vertexAllocation.ptr, gTriAPICommand.Vertices.data(), vertexDataSize);
memcpy(indexAllocation.ptr, gTriAPICommand.Indices.data(), indexDataSize);

GLuint baseVertex = (GLuint)(vertexAllocation.offset / sizeof(triapivertex_t));
GLuint baseIndex = (GLuint)(indexAllocation.offset / sizeof(uint32_t));
```

#### 6.4 Rendering State Setup

```cpp
uint64_t ProgramState = 0;

switch (gTriAPICommand.DrawRenderMode) {
    case kRenderNormal:
        glDisable(GL_BLEND);
        break;
        
    case kRenderTransAdd:
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        ProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;
        break;
        
    case kRenderTransAlpha:
    case kRenderTransColor:
    case kRenderTransTexture:
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ProgramState |= SPRITE_ALPHA_BLEND_ENABLED;
        break;
}

// Fog
if (R_IsRenderingFog()) {
    if (r_fog_mode == GL_LINEAR)
        ProgramState |= SPRITE_LINEAR_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP)
        ProgramState |= SPRITE_EXP_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP2)
        ProgramState |= SPRITE_EXP2_FOG_ENABLED;
}

// Other effects
if (R_IsRenderingWaterView())
    ProgramState |= SPRITE_CLIP_ENABLED;
if (R_IsRenderingGammaBlending())
    ProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
if (r_draw_oitblend)
    ProgramState |= SPRITE_OIT_BLEND_ENABLED;
```

#### 6.5 Draw Call

```cpp
triapi_program_t prog{};
R_UseTriAPIProgram(ProgramState, &prog);

GL_BindVAO(gTriAPICommand.hVAO);

if (gTriAPICommand.GLPrimitiveCode == GL_LINES) {
    glDrawElementsBaseVertex(GL_LINES, indiceCount, GL_UNSIGNED_INT, 
                            BUFFER_OFFSET(baseIndex), baseVertex);
} else {
    glDrawElementsBaseVertex(GL_TRIANGLES, indiceCount, GL_UNSIGNED_INT, 
                            BUFFER_OFFSET(baseIndex), baseVertex);
}

GL_UseProgram(0);
GL_BindVAO(0);

// Restore state
glDisable(GL_BLEND);
glDepthMask(GL_TRUE);
```

---

## Data Structures

### CTriAPICommand - TriAPI Command Buffer
```cpp
class CTriAPICommand {
public:
    int GLPrimitiveCode;                    // OpenGL primitive type
    vec2_t TexCoord;                        // Current texture coordinates
    vec4_t DrawColor;                       // Current color
    std::vector<vertex3f_t> Positions;      // Position list (for polygon triangulation)
    std::vector<triapivertex_t> Vertices;   // Vertex list
    std::vector<uint32_t> Indices;          // Index list
    int RenderMode;                         // Render mode
    int DrawRenderMode;                     // Render mode at draw time
    GLuint hVAO;                            // VAO handle
};
```

### triapivertex_t - TriAPI Vertex Format
```cpp
typedef struct triapivertex_s {
    vec3_t pos;         // Position
    vec2_t texcoord;    // Texture coordinates
    vec4_t color;       // Color
} triapivertex_t;
```

---

## Ring Buffer System

### Why Use Ring Buffers?

Traditional immediate mode must create and destroy buffers for every draw, resulting in poor performance. Ring buffers provide:
1. **Persistent Mapping** - The buffer remains mapped to CPU memory.
2. **No Synchronization** - Uses offsets to avoid GPU/CPU conflicts.
3. **Efficient Reuse** - Cycles through one large buffer.

### Ring Buffer Sizes
- **Vertex buffer**: 32 MB
- **Index buffer**: 8 MB

### Frame Management
```cpp
void R_BeginFrame() {
    if (g_TriAPIVertexBuffer)
        g_TriAPIVertexBuffer->BeginFrame();
    if (g_TriAPIIndexBuffer)
        g_TriAPIIndexBuffer->BeginFrame();
}

void R_EndFrame() {
    if (g_TriAPIVertexBuffer)
        g_TriAPIVertexBuffer->EndFrame();
    if (g_TriAPIIndexBuffer)
        g_TriAPIIndexBuffer->EndFrame();
}
```

---

## Shader System

### R_UseTriAPIProgram() - Select TriAPI Shader
**Location**: `gl_sprite.cpp:170-246`

```cpp
void R_UseTriAPIProgram(program_state_t state, triapi_program_t* progOutput) {
    auto itor = g_TriAPIProgramTable.find(state);
    if (itor == g_TriAPIProgramTable.end()) {
        // Compile a new shader variant
        triapi_program_t prog;
        
        // Generate shader code according to state flags
        std::string defines;
        if (state & SPRITE_ALPHA_BLEND_ENABLED)
            defines += "#define ALPHA_BLEND\n";
        if (state & SPRITE_ADDITIVE_BLEND_ENABLED)
            defines += "#define ADDITIVE_BLEND\n";
        // ... additional flags
        
        // Compile shader
        prog.program = R_CompileShader(vertexShader, fragmentShader, defines);
        
        // Cache
        g_TriAPIProgramTable[state] = prog;
    }
    
    *progOutput = g_TriAPIProgramTable[state];
    GL_UseProgram(progOutput->program);
}
```

### Shader Files
- `triapi_shader.vert.glsl` - Vertex shader
- `triapi_shader.frag.glsl` - Fragment shader

---

## Usage Examples

### Example 1: Draw Particles
```cpp
void R_DrawParticles() {
    gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
    gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);
    
    for (particle_t* p = active_particles; p; p = p->next) {
        // Calculate quadrilateral vertices
        vec3_t up, right;
        VectorScale(vup, scale, up);
        VectorScale(vright, scale, right);
        
        // Vertex 1
        gEngfuncs.pTriAPI->Color4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
        gEngfuncs.pTriAPI->TexCoord2f(0, 0);
        gEngfuncs.pTriAPI->Vertex3fv(p->org);
        
        // Vertex 2
        gEngfuncs.pTriAPI->TexCoord2f(1, 0);
        gEngfuncs.pTriAPI->Vertex3f(p->org[0] + up[0], 
                                    p->org[1] + up[1], 
                                    p->org[2] + up[2]);
        
        // Vertex 3
        gEngfuncs.pTriAPI->TexCoord2f(0, 1);
        gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0], 
                                    p->org[1] + right[1], 
                                    p->org[2] + right[2]);
    }
    
    gEngfuncs.pTriAPI->End();
    gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
```

### Example 2: Draw a Wireframe
```cpp
void DrawWireframeBox(vec3_t mins, vec3_t maxs) {
    gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
    gEngfuncs.pTriAPI->Begin(TRI_LINES);
    gEngfuncs.pTriAPI->Color4f(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Bottom face
    gEngfuncs.pTriAPI->Vertex3f(mins[0], mins[1], mins[2]);
    gEngfuncs.pTriAPI->Vertex3f(maxs[0], mins[1], mins[2]);
    
    gEngfuncs.pTriAPI->Vertex3f(maxs[0], mins[1], mins[2]);
    gEngfuncs.pTriAPI->Vertex3f(maxs[0], maxs[1], mins[2]);
    
    // ... additional edges
    
    gEngfuncs.pTriAPI->End();
    gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
```

---

## Performance Optimization

### 1. Batched Drawing
- All TriAPI calls are submitted together at `End()`.
- Reduces the number of draw calls.
- Uses indexed drawing.

### 2. Ring Buffers
- Avoids creating/destroying buffers every frame.
- Persistent mapping reduces CPU overhead.
- Reuses memory cyclically.

### 3. Shader Cache
- Compiled shader variants are cached.
- Avoids repeated compilation.
- Enables fast lookup.

### 4. Primitive Conversion Optimization
- Converts all primitive types to triangles.
- Uses a unified drawing path.
- Uses a GPU-friendly data layout.

---

## Debugging and Diagnostics

### OpenGL Debug Group
```cpp
GL_BeginDebugGroup("triapi_End");
// ... drawing code ...
GL_EndDebugGroup();
```

### Buffer Overflow Detection
```cpp
if (!g_TriAPIVertexBuffer->Allocate(vertexDataSize, vertexAllocation)) {
    gEngfuncs.Con_DPrintf("triapi_End: g_TriAPIVertexBuffer full!\n");
    return;
}
```

---

## Differences from the Original TriAPI

### Original GoldSrc TriAPI
- Immediate mode (glBegin/glEnd)
- Submits to the GPU on every call
- Lower performance
- Does not support modern OpenGL

### Renderer Plugin TriAPI
- Batched mode (VBO)
- Collects all vertices and submits them together
- High performance
- Uses the modern OpenGL Core Profile
- Supports advanced effects (fog, OIT, etc.)

---

## Limitations and Notes

### 1. Buffer Size Limits
- Vertex buffer: 32 MB
- Index buffer: 8 MB
- Exceeding the limit prints a warning and skips drawing.

### 2. Primitive Type Limits
- Supports only seven standard primitive types.
- All primitives are ultimately converted to triangles or line segments.

### 3. State Management
- Colors and texture coordinates are "sticky".
- They must be set before each vertex.
- `RenderMode` is locked at `Begin`.

---

## Summary

Characteristics of the TriAPI rendering system:

1. **Immediate-Mode Interface** - Maintains compatibility with the original GoldSrc implementation
2. **Batched Rendering Implementation** - Uses modern VBO technology
3. **Ring Buffers** - Efficient memory management
4. **Primitive Conversion** - Uniformly converts primitives to triangles
5. **Shader System** - Supports multiple render modes and effects
6. **Performance Optimization** - Batched submission, caching, and indexed drawing
7. **Debugging Support** - OpenGL debug groups and overflow detection

TriAPI is the primary mechanism for client DLLs to draw custom geometry and is widely used for particle effects, debug visualization, HUD elements, and more. The Renderer plugin's modern implementation substantially improves performance while retaining complete API compatibility.

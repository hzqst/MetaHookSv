# Renderer Plugin - TriAPI绘制流程详解

## 概述

TriAPI (Triangle API) 是GoldSrc引擎提供给客户端DLL的即时模式绘图API，允许游戏代码直接绘制自定义几何体。Renderer插件拦截并重新实现了TriAPI，使用现代OpenGL的VBO批量绘制替代了原始的即时模式。

---

## TriAPI调用链

### 完整调用流程

```
R_RenderScene()
└── ClientDLL_DrawNormalTriangles()     // 不透明三角形
    └── gPrivateFuncs.ClientDLL_DrawNormalTriangles()
        └── [客户端DLL代码]
            └── gEngfuncs.pTriAPI->...

R_RenderScene()
└── R_DrawTransEntities()               // 透明实体
    └── ClientDLL_DrawTransparentTriangles()
        └── gPrivateFuncs.ClientDLL_DrawTransparentTriangles()
            └── [客户端DLL代码]
                └── gEngfuncs.pTriAPI->...
                    ├── RenderMode()
                    ├── Begin()
                    ├── Color4f() / Color4ub()
                    ├── TexCoord2f()
                    ├── Vertex3f() / Vertex3fv()
                    └── End()
```

---

## TriAPI接口函数

### 核心绘制流程

```cpp
// 1. 设置渲染模式
gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);

// 2. 开始绘制
gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);

// 3. 提交顶点
gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
gEngfuncs.pTriAPI->Vertex3f(x, y, z);

// 4. 结束绘制
gEngfuncs.pTriAPI->End();
```

### TriAPI函数列表

#### 渲染控制
- **RenderMode(int mode)** - 设置渲染模式
- **Begin(int primitiveCode)** - 开始绘制图元
- **End()** - 结束绘制并提交

#### 顶点属性
- **Color4f(float r, float g, float b, float a)** - 设置颜色 (浮点)
- **Color4ub(byte r, byte g, byte b, byte a)** - 设置颜色 (字节)
- **TexCoord2f(float s, float t)** - 设置纹理坐标
- **Vertex3f(float x, float y, float z)** - 提交顶点
- **Vertex3fv(float* v)** - 提交顶点 (数组)

#### 其他功能
- **Brightness(float brightness)** - 设置亮度
- **Color4fRendermode(float r, float g, float b, float a)** - 带渲染模式的颜色
- **GetMatrix(int mode, float* matrix)** - 获取矩阵
- **BoxInPVS(float* mins, float* maxs)** - PVS可见性测试
- **Fog(float* color, float start, float end, int enable)** - 雾效控制
- **FogParams(float density, int skybox)** - 雾效参数

---

## 详细函数分析

### 1. triapi_RenderMode() - 设置渲染模式
**位置**: `gl_rmain.cpp:1310-1314`

```cpp
void triapi_RenderMode(int mode) {
    gTriAPICommand.RenderMode = mode;
}
```

**支持的渲染模式**:
- `kRenderNormal` (0) - 不透明
- `kRenderTransColor` (1) - 颜色透明
- `kRenderTransTexture` (2) - 纹理透明
- `kRenderGlow` (3) - 发光
- `kRenderTransAlpha` (4) - Alpha透明
- `kRenderTransAdd` (5) - 加法混合

---

### 2. triapi_Begin() - 开始绘制
**位置**: `gl_rmain.cpp:1315-1336`

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

**图元类型**:
- **TRI_TRIANGLES** - 独立三角形
- **TRI_TRIANGLE_FAN** - 扇形三角形
- **TRI_QUADS** - 四边形
- **TRI_POLYGON** - 多边形
- **TRI_LINES** - 线段
- **TRI_TRIANGLE_STRIP** - 三角形带
- **TRI_QUAD_STRIP** - 四边形带

---

### 3. triapi_Color4f() / triapi_Color4ub() - 设置颜色
**位置**: `gl_rmain.cpp:1775-1825`

```cpp
void triapi_Color4f(float r, float g, float b, float a) {
    gTriAPICommand.DrawColor[0] = r;
    gTriAPICommand.DrawColor[1] = g;
    gTriAPICommand.DrawColor[2] = b;
    gTriAPICommand.DrawColor[3] = a;
    
    // TransAlpha模式特殊处理
    if (gTriAPICommand.RenderMode == kRenderTransAlpha) {
        // 颜色预乘Alpha
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

### 4. triapi_TexCoord2f() - 设置纹理坐标
**位置**: `gl_rmain.cpp:1876-1880`

```cpp
void triapi_TexCoord2f(float s, float t) {
    gTriAPICommand.TexCoord[0] = s;
    gTriAPICommand.TexCoord[1] = t;
}
```

---

### 5. triapi_Vertex3f() / triapi_Vertex3fv() - 提交顶点
**位置**: `gl_rmain.cpp:1830-1874`

```cpp
void triapi_Vertex3f(float x, float y, float z) {
    vec3_t pos = { x, y, z };
    
    // 保存位置用于多边形三角化
    gTriAPICommand.Positions.emplace_back(pos);
    
    // 构建顶点数据
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

### 6. triapi_End() - 结束绘制并提交
**位置**: `gl_rmain.cpp:1345-1773`

这是TriAPI最核心的函数，负责将收集的顶点转换为索引三角形并提交到GPU。

#### 6.1 图元转换为三角形

```cpp
void triapi_End() {
    size_t n = gTriAPICommand.Vertices.size();
    
    // 根据图元类型生成索引
    if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLES) {
        // 每3个顶点一个三角形
        for (size_t i = 0; i < n; i += 3) {
            if (i + 2 < n) {
                gTriAPICommand.Indices.push_back(i);
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i + 2);
            }
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLE_FAN) {
        // 扇形: 所有三角形共享第一个顶点
        for (size_t i = 1; i < n - 1; ++i) {
            gTriAPICommand.Indices.push_back(0);
            gTriAPICommand.Indices.push_back(i);
            gTriAPICommand.Indices.push_back(i + 1);
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_QUADS) {
        // 四边形转换为2个三角形
        for (size_t i = 0; i < n; i += 4) {
            if (i + 3 < n) {
                // 三角形1: v0, v1, v2
                gTriAPICommand.Indices.push_back(i + 0);
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i + 2);
                // 三角形2: v2, v3, v0
                gTriAPICommand.Indices.push_back(i + 2);
                gTriAPICommand.Indices.push_back(i + 3);
                gTriAPICommand.Indices.push_back(i + 0);
            }
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_POLYGON) {
        // 多边形三角化 (Ear Clipping算法)
        R_PolygonToTriangleList(gTriAPICommand.Positions, 
                                gTriAPICommand.Indices);
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_TRIANGLE_STRIP) {
        // 三角形带: 每个新顶点与前两个顶点组成三角形
        for (size_t i = 0; i < n - 2; ++i) {
            if (i % 2 == 0) {
                // 偶数: 正序
                gTriAPICommand.Indices.push_back(i);
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i + 2);
            } else {
                // 奇数: 反序 (保持绕序一致)
                gTriAPICommand.Indices.push_back(i + 1);
                gTriAPICommand.Indices.push_back(i);
                gTriAPICommand.Indices.push_back(i + 2);
            }
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_QUAD_STRIP) {
        // 四边形带
        for (size_t i = 0; i + 3 < n; i += 2) {
            uint32_t v0 = i, v1 = i + 1, v2 = i + 2, v3 = i + 3;
            // 三角形1: v0, v1, v3
            gTriAPICommand.Indices.push_back(v0);
            gTriAPICommand.Indices.push_back(v1);
            gTriAPICommand.Indices.push_back(v3);
            // 三角形2: v0, v3, v2
            gTriAPICommand.Indices.push_back(v0);
            gTriAPICommand.Indices.push_back(v3);
            gTriAPICommand.Indices.push_back(v2);
        }
    }
    else if (gTriAPICommand.GLPrimitiveCode == GL_LINES) {
        // 线段: 直接使用顶点索引
        for (size_t i = 0; i < n; i++) {
            gTriAPICommand.Indices.push_back(i);
        }
    }
}
```

#### 6.2 VAO和环形缓冲区初始化

```cpp
if (!gTriAPICommand.hVAO) {
    gTriAPICommand.hVAO = GL_GenVAO();
    
    // 创建环形缓冲区
    if (!g_TriAPIVertexBuffer) {
        g_TriAPIVertexBuffer = GL_CreatePMBRingBuffer(
            "TriAPIVertexBuffer", 32 * 1024 * 1024, GL_ARRAY_BUFFER);
    }
    
    if (!g_TriAPIIndexBuffer) {
        g_TriAPIIndexBuffer = GL_CreatePMBRingBuffer(
            "TriAPIIndexBuffer", 8 * 1024 * 1024, GL_ELEMENT_ARRAY_BUFFER);
    }
    
    // 配置VAO
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

#### 6.3 数据上传

```cpp
uint32_t verticesCount = gTriAPICommand.Vertices.size();
uint32_t indiceCount = gTriAPICommand.Indices.size();

size_t vertexDataSize = verticesCount * sizeof(triapivertex_t);
size_t indexDataSize = indiceCount * sizeof(uint32_t);

// 从环形缓冲区分配空间
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

// 拷贝数据
memcpy(vertexAllocation.ptr, gTriAPICommand.Vertices.data(), vertexDataSize);
memcpy(indexAllocation.ptr, gTriAPICommand.Indices.data(), indexDataSize);

GLuint baseVertex = (GLuint)(vertexAllocation.offset / sizeof(triapivertex_t));
GLuint baseIndex = (GLuint)(indexAllocation.offset / sizeof(uint32_t));
```

#### 6.4 渲染状态设置

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

// 雾效
if (R_IsRenderingFog()) {
    if (r_fog_mode == GL_LINEAR)
        ProgramState |= SPRITE_LINEAR_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP)
        ProgramState |= SPRITE_EXP_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP2)
        ProgramState |= SPRITE_EXP2_FOG_ENABLED;
}

// 其他特效
if (R_IsRenderingWaterView())
    ProgramState |= SPRITE_CLIP_ENABLED;
if (R_IsRenderingGammaBlending())
    ProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
if (r_draw_oitblend)
    ProgramState |= SPRITE_OIT_BLEND_ENABLED;
```

#### 6.5 绘制调用

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

// 恢复状态
glDisable(GL_BLEND);
glDepthMask(GL_TRUE);
```

---

## 数据结构

### CTriAPICommand - TriAPI命令缓冲区
```cpp
class CTriAPICommand {
public:
    int GLPrimitiveCode;                    // OpenGL图元类型
    vec2_t TexCoord;                        // 当前纹理坐标
    vec4_t DrawColor;                       // 当前颜色
    std::vector<vertex3f_t> Positions;      // 位置列表 (用于多边形三角化)
    std::vector<triapivertex_t> Vertices;   // 顶点列表
    std::vector<uint32_t> Indices;          // 索引列表
    int RenderMode;                         // 渲染模式
    int DrawRenderMode;                     // 绘制时的渲染模式
    GLuint hVAO;                            // VAO句柄
};
```

### triapivertex_t - TriAPI顶点格式
```cpp
typedef struct triapivertex_s {
    vec3_t pos;         // 位置
    vec2_t texcoord;    // 纹理坐标
    vec4_t color;       // 颜色
} triapivertex_t;
```

---

## 环形缓冲区系统

### 为什么使用环形缓冲区?

传统的即时模式每次绘制都需要创建和销毁缓冲区，性能很差。环形缓冲区允许:
1. **持久映射** - 缓冲区始终映射到CPU内存
2. **无需同步** - 使用偏移避免GPU/CPU冲突
3. **高效复用** - 循环使用同一块大缓冲区

### 环形缓冲区大小
- **顶点缓冲区**: 32 MB
- **索引缓冲区**: 8 MB

### 帧管理
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

## 着色器系统

### R_UseTriAPIProgram() - 选择TriAPI着色器
**位置**: `gl_sprite.cpp:170-246`

```cpp
void R_UseTriAPIProgram(program_state_t state, triapi_program_t* progOutput) {
    auto itor = g_TriAPIProgramTable.find(state);
    if (itor == g_TriAPIProgramTable.end()) {
        // 编译新的着色器变体
        triapi_program_t prog;
        
        // 根据状态标志生成着色器代码
        std::string defines;
        if (state & SPRITE_ALPHA_BLEND_ENABLED)
            defines += "#define ALPHA_BLEND\n";
        if (state & SPRITE_ADDITIVE_BLEND_ENABLED)
            defines += "#define ADDITIVE_BLEND\n";
        // ... 更多标志
        
        // 编译着色器
        prog.program = R_CompileShader(vertexShader, fragmentShader, defines);
        
        // 缓存
        g_TriAPIProgramTable[state] = prog;
    }
    
    *progOutput = g_TriAPIProgramTable[state];
    GL_UseProgram(progOutput->program);
}
```

### 着色器文件
- `triapi_shader.vert.glsl` - 顶点着色器
- `triapi_shader.frag.glsl` - 片段着色器

---

## 使用示例

### 示例1: 绘制粒子
```cpp
void R_DrawParticles() {
    gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
    gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);
    
    for (particle_t* p = active_particles; p; p = p->next) {
        // 计算四边形顶点
        vec3_t up, right;
        VectorScale(vup, scale, up);
        VectorScale(vright, scale, right);
        
        // 顶点1
        gEngfuncs.pTriAPI->Color4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
        gEngfuncs.pTriAPI->TexCoord2f(0, 0);
        gEngfuncs.pTriAPI->Vertex3fv(p->org);
        
        // 顶点2
        gEngfuncs.pTriAPI->TexCoord2f(1, 0);
        gEngfuncs.pTriAPI->Vertex3f(p->org[0] + up[0], 
                                    p->org[1] + up[1], 
                                    p->org[2] + up[2]);
        
        // 顶点3
        gEngfuncs.pTriAPI->TexCoord2f(0, 1);
        gEngfuncs.pTriAPI->Vertex3f(p->org[0] + right[0], 
                                    p->org[1] + right[1], 
                                    p->org[2] + right[2]);
    }
    
    gEngfuncs.pTriAPI->End();
    gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
```

### 示例2: 绘制线框
```cpp
void DrawWireframeBox(vec3_t mins, vec3_t maxs) {
    gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
    gEngfuncs.pTriAPI->Begin(TRI_LINES);
    gEngfuncs.pTriAPI->Color4f(1.0f, 0.0f, 0.0f, 1.0f);
    
    // 底面
    gEngfuncs.pTriAPI->Vertex3f(mins[0], mins[1], mins[2]);
    gEngfuncs.pTriAPI->Vertex3f(maxs[0], mins[1], mins[2]);
    
    gEngfuncs.pTriAPI->Vertex3f(maxs[0], mins[1], mins[2]);
    gEngfuncs.pTriAPI->Vertex3f(maxs[0], maxs[1], mins[2]);
    
    // ... 更多边
    
    gEngfuncs.pTriAPI->End();
    gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
```

---

## 性能优化

### 1. 批量绘制
- 所有TriAPI调用在`End()`时一次性提交
- 减少Draw Call数量
- 使用索引绘制

### 2. 环形缓冲区
- 避免每帧创建/销毁缓冲区
- 持久映射减少CPU开销
- 循环复用内存

### 3. 着色器缓存
- 编译的着色器变体被缓存
- 避免重复编译
- 快速查找

### 4. 图元转换优化
- 所有图元类型转换为三角形
- 统一的绘制路径
- GPU友好的数据布局

---

## 调试和诊断

### OpenGL调试组
```cpp
GL_BeginDebugGroup("triapi_End");
// ... 绘制代码 ...
GL_EndDebugGroup();
```

### 缓冲区溢出检测
```cpp
if (!g_TriAPIVertexBuffer->Allocate(vertexDataSize, vertexAllocation)) {
    gEngfuncs.Con_DPrintf("triapi_End: g_TriAPIVertexBuffer full!\n");
    return;
}
```

---

## 与原始TriAPI的区别

### 原始GoldSrc TriAPI
- 即时模式 (glBegin/glEnd)
- 每次调用都提交到GPU
- 性能较差
- 不支持现代OpenGL

### Renderer插件TriAPI
- 批量模式 (VBO)
- 收集所有顶点后一次性提交
- 高性能
- 使用现代OpenGL Core Profile
- 支持高级特效 (雾效、OIT等)

---

## 限制和注意事项

### 1. 缓冲区大小限制
- 顶点缓冲区: 32 MB
- 索引缓冲区: 8 MB
- 超出会打印警告并跳过绘制

### 2. 图元类型限制
- 只支持7种标准图元类型
- 所有图元最终转换为三角形或线段

### 3. 状态管理
- 颜色和纹理坐标是"粘性"的
- 需要在每个顶点前设置
- RenderMode在Begin时锁定

---

## 总结

TriAPI绘制系统的特点:

1. **即时模式接口** - 保持与原始GoldSrc兼容
2. **批量绘制实现** - 使用现代VBO技术
3. **环形缓冲区** - 高效的内存管理
4. **图元转换** - 统一转换为三角形
5. **着色器系统** - 支持多种渲染模式和特效
6. **性能优化** - 批量提交、缓存、索引绘制
7. **调试支持** - OpenGL调试组、溢出检测

TriAPI是客户端DLL绘制自定义几何体的主要方式，广泛用于粒子效果、调试可视化、HUD元素等。Renderer插件通过现代化的实现大幅提升了性能，同时保持了完全的API兼容性。

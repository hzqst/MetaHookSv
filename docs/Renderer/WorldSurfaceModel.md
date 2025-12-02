# WorldSurfaceModel 几何数据组织与GPU存储

## 概述

`WorldSurfaceModel` 是用于渲染世界几何体（World Geometry）的核心数据结构。该模块负责将引擎原始的 BSP 模型数据转换为适合现代 GPU 渲染的顶点缓冲区和索引缓冲区格式。

主要处理函数：`R_GenerateWorldSurfaceWorldModel(model_t* mod)`

## 数据结构

### 1. CPU 端顶点数据结构

#### brushvertex_t - 基础顶点数据
```cpp
typedef struct brushvertex_s
{
    vec3_t  pos;                    // 顶点位置 (x, y, z)
    vec2_t  texcoord;               // 纹理坐标 (u, v)
    vec2_t  lightmaptexcoord;       // 光照图纹理坐标 (u, v)
} brushvertex_t;
```

#### brushvertextbn_t - 切线空间数据
```cpp
typedef struct brushvertextbn_s
{
    vec3_t  normal;                 // 法线向量
    vec3_t  s_tangent;              // S 切线向量（纹理 U 方向）
    vec3_t  t_tangent;              // T 切线向量（纹理 V 方向）
} brushvertextbn_t;
```

#### brushinstancedata_t - 实例数据
```cpp
typedef struct brushinstancedata_s
{
    uint16_t packed_matId[2];       // [0]: 漫反射纹理材质ID, [1]: 光照图纹理索引
    byte     styles[4];             // 光照样式数组
    float    diffusescale;          // 漫反射缩放（用于 SURF_DRAWTILED 标志）
} brushinstancedata_t;
```

### 2. CPU 端面数据结构

#### CWorldSurfaceBrushFace - 面描述信息
```cpp
class CWorldSurfaceBrushFace
{
public:
    int      index;                 // 面索引
    int      flags;                 // 面标志 (SURF_*)
    vec3_t   normal;                // 面法线
    vec3_t   s_tangent;             // S 切线
    vec3_t   t_tangent;             // T 切线
    uint32_t poly_count;            // 多边形数量
    uint32_t start_index;           // 正向索引起始位置
    uint32_t index_count;           // 正向索引数量
    uint32_t reverse_start_index;   // 反向索引起始位置（用于水面）
    uint32_t reverse_index_count;   // 反向索引数量
    uint32_t instance_index;        // 实例数据索引
    uint32_t instance_count;        // 实例数量
};
```

### 3. GPU 端缓冲区结构

#### CWorldSurfaceWorldModel - GPU 资源容器
```cpp
class CWorldSurfaceWorldModel
{
public:
    model_t* m_model;                           // 关联的模型指针
    GLuint   hVBO[WSURF_VBO_MAX];               // 顶点缓冲区对象数组
    GLuint   hEBO;                              // 索引缓冲区对象
    GLuint   hVAO;                              // 顶点数组对象
    std::vector<CWorldSurfaceBrushFace> m_vFaceBuffer;  // 面信息数组
};
```

**VBO 数组索引定义：**
- `WSURF_VBO_VERTEX (0)`: 存储 `brushvertex_t` 数据
- `WSURF_VBO_VERTEXTBN (1)`: 存储 `brushvertextbn_t` 数据
- `WSURF_VBO_INSTANCE (2)`: 存储 `brushinstancedata_t` 数据

## 数据处理流程

### 阶段 1: 缓冲区初始化

```cpp
std::vector<brushvertex_t> vVertexDataBuffer;
std::vector<brushvertextbn_t> vVertexTBNDataBuffer;
std::vector<brushinstancedata_t> vInstanceDataBuffer;
std::vector<uint32_t> vIndiceBuffer;

vVertexDataBuffer.reserve(mod->numvertexes);
vInstanceDataBuffer.reserve(mod->numsurfaces);
vIndiceBuffer.reserve(mod->numvertexes * 4);
```

预先分配内存以提升性能，避免动态扩容。

### 阶段 2: 遍历所有表面 (Surfaces)

对模型中的每个表面执行以下处理：

#### 2.1 提取切线空间基向量

```cpp
for (int i = 0; i < mod->numsurfaces; i++)
{
    auto surf = R_GetWorldSurfaceByIndex(mod, i);
    auto pBrushFace = &pWorldModel->m_vFaceBuffer[i];
    
    // 从纹理信息中提取切线向量
    VectorCopy(surf->texinfo->vecs[0], pBrushFace->s_tangent);
    VectorCopy(surf->texinfo->vecs[1], pBrushFace->t_tangent);
    VectorNormalize(pBrushFace->s_tangent);
    VectorNormalize(pBrushFace->t_tangent);
    
    // 提取法线
    VectorCopy(surf->plane->normal, pBrushFace->normal);
    pBrushFace->index = i;
    pBrushFace->flags = surf->flags;
```

#### 2.2 处理背面标志

```cpp
    // 如果是背面，翻转所有向量
    if (surf->flags & SURF_PLANEBACK)
    {
        VectorInverse(pBrushFace->normal);
        VectorInverse(pBrushFace->s_tangent);
        VectorInverse(pBrushFace->t_tangent);
    }
```

#### 2.3 更新光照图纹理计数

```cpp
    if (surf->lightmaptexturenum + 1 > g_WorldSurfaceRenderer.iNumLightmapTextures)
        g_WorldSurfaceRenderer.iNumLightmapTextures = surf->lightmaptexturenum + 1;
```

### 阶段 3: 处理多边形 (Polygons)

每个表面可能包含一个或多个多边形链表。

#### 3.1 湍流表面 (SURF_DRAWTURB) - 双面渲染

湍流表面（如水面）需要生成正反两面的几何数据：

**正面几何数据生成：**

```cpp
if (surf->flags & SURF_DRAWTURB)
{
    // 正面渲染数据
    uint32_t nBrushStartIndex = vIndiceBuffer.size();
    
    for (poly = surf->polys; poly; poly = poly->next)
    {
        uint32_t nPolyStartIndex = vVertexDataBuffer.size();
        
        // 提取多边形顶点
        for (int j = 0; j < poly->numverts; j++, v += VERTEXSIZE)
        {
            brushvertex_t tempVertexData;
            VectorCopy(v, tempVertexData.pos);
            tempVertexData.texcoord[0] = v[3];           // U
            tempVertexData.texcoord[1] = v[4];           // V
            tempVertexData.lightmaptexcoord[0] = v[5];   // 光照图 U
            tempVertexData.lightmaptexcoord[1] = v[6];   // 光照图 V
            
            brushvertextbn_t tempVertexTBNData;
            VectorCopy(pBrushFace->normal, tempVertexTBNData.normal);
            VectorCopy(pBrushFace->s_tangent, tempVertexTBNData.s_tangent);
            VectorCopy(pBrushFace->t_tangent, tempVertexTBNData.t_tangent);
            
            vVertexDataBuffer.emplace_back(tempVertexData);
            vVertexTBNDataBuffer.emplace_back(tempVertexTBNData);
        }
        
        // 三角形化：将多边形转换为三角形列表
        std::vector<uint32_t> vTriangleListIndices;
        R_PolygonToTriangleList(vPolyVertices, vTriangleListIndices);
        
        // 添加索引
        for (size_t k = 0; k < vTriangleListIndices.size(); ++k)
        {
            vIndiceBuffer.emplace_back(nPolyStartIndex + vTriangleListIndices[k]);
        }
    }
    
    pBrushFace->start_index = nBrushStartIndex;
    pBrushFace->index_count = vIndiceBuffer.size() - nBrushStartIndex;
```

**反面几何数据生成：**

```cpp
    // 反面渲染数据（索引顺序反转）
    uint32_t nBrushStartIndex = vIndiceBuffer.size();
    
    for (poly = surf->polys; poly; poly = poly->next)
    {
        // ... 相同的顶点数据生成 ...
        
        // 反向添加索引（实现背面剔除翻转）
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

#### 3.2 普通表面 - 单面渲染

```cpp
else
{
    uint32_t nBrushStartIndex = vIndiceBuffer.size();
    
    for (poly = surf->polys; poly; poly = poly->next)
    {
        // ... 与湍流表面相同的顶点数据生成 ...
        // 但只生成正面索引，无需反向索引
    }
    
    pBrushFace->start_index = nBrushStartIndex;
    pBrushFace->index_count = vIndiceBuffer.size() - nBrushStartIndex;
}
```

### 阶段 4: 生成实例数据

每个表面都有对应的实例数据，用于存储材质和光照信息：

```cpp
pBrushFace->instance_index = vInstanceDataBuffer.size();

brushinstancedata_t tempInstanceData;

// 材质ID: [0] = 漫反射纹理, [1] = 光照图索引
tempInstanceData.packed_matId[0] = ptexture ? R_FindWorldMaterialId(ptexture->gl_texturenum) : 0;
tempInstanceData.packed_matId[1] = surf->lightmaptexturenum;

// 漫反射缩放（用于平铺纹理）
tempInstanceData.diffusescale = (ptexture && (pBrushFace->flags & SURF_DRAWTILED)) 
                                ? 1.0f / ptexture->width : 0;

// 光照样式
memcpy(&tempInstanceData.styles, surf->styles, sizeof(surf->styles));

vInstanceDataBuffer.emplace_back(tempInstanceData);
pBrushFace->instance_count = 1;
```

### 阶段 5: 上传数据到 GPU

#### 5.1 创建并上传索引缓冲区 (EBO)

```cpp
pWorldModel->hEBO = GL_GenBuffer();
GL_UploadDataToEBOStaticDraw(pWorldModel->hEBO, 
                              sizeof(uint32_t) * vIndiceBuffer.size(), 
                              vIndiceBuffer.data());
```

#### 5.2 创建并上传顶点缓冲区 (VBO)

**VBO[0] - 基础顶点数据：**
```cpp
pWorldModel->hVBO[WSURF_VBO_VERTEX] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_VERTEX], 
                              sizeof(brushvertex_t) * vVertexDataBuffer.size(), 
                              vVertexDataBuffer.data());
```

**VBO[1] - 切线空间数据：**
```cpp
pWorldModel->hVBO[WSURF_VBO_VERTEXTBN] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_VERTEXTBN], 
                              sizeof(brushvertextbn_t) * vVertexTBNDataBuffer.size(), 
                              vVertexTBNDataBuffer.data());
```

**VBO[2] - 实例数据：**
```cpp
pWorldModel->hVBO[WSURF_VBO_INSTANCE] = GL_GenBuffer();
GL_UploadDataToVBOStaticDraw(pWorldModel->hVBO[WSURF_VBO_INSTANCE], 
                              sizeof(brushinstancedata_t) * vInstanceDataBuffer.size(), 
                              vInstanceDataBuffer.data());
```

### 阶段 6: 配置顶点数组对象 (VAO)

VAO 配置定义了顶点属性的布局和绑定方式。

```cpp
pWorldModel->hVAO = GL_GenVAO();

GL_BindStatesForVAO(pWorldModel->hVAO, [pWorldModel]() {

    // 绑定索引缓冲区
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pWorldModel->hEBO);
```

#### 6.1 配置基础顶点属性

```cpp
    glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_VERTEX]);
    
    glEnableVertexAttribArray(WSURF_VA_POSITION);            // 位置
    glEnableVertexAttribArray(WSURF_VA_TEXCOORD);            // 纹理坐标
    glEnableVertexAttribArray(WSURF_VA_LIGHTMAP_TEXCOORD);   // 光照图坐标
    
    glVertexAttribPointer(WSURF_VA_POSITION, 3, GL_FLOAT, false, 
                          sizeof(brushvertex_t), OFFSET(brushvertex_t, pos));
    glVertexAttribPointer(WSURF_VA_TEXCOORD, 2, GL_FLOAT, false, 
                          sizeof(brushvertex_t), OFFSET(brushvertex_t, texcoord));
    glVertexAttribPointer(WSURF_VA_LIGHTMAP_TEXCOORD, 2, GL_FLOAT, false, 
                          sizeof(brushvertex_t), OFFSET(brushvertex_t, lightmaptexcoord));
```

**顶点属性索引定义：**
- `WSURF_VA_POSITION (0)`: vec3 - 顶点位置
- `WSURF_VA_TEXCOORD (2)`: vec2 - 纹理坐标
- `WSURF_VA_LIGHTMAP_TEXCOORD (3)`: vec2 - 光照图坐标

#### 6.2 配置切线空间属性

```cpp
    glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_VERTEXTBN]);
    
    glEnableVertexAttribArray(WSURF_VA_NORMAL);      // 法线
    glEnableVertexAttribArray(WSURF_VA_S_TANGENT);   // S 切线
    glEnableVertexAttribArray(WSURF_VA_T_TANGENT);   // T 切线
    
    glVertexAttribPointer(WSURF_VA_NORMAL, 3, GL_FLOAT, false, 
                          sizeof(brushvertextbn_t), OFFSET(brushvertextbn_t, normal));
    glVertexAttribPointer(WSURF_VA_S_TANGENT, 3, GL_FLOAT, false, 
                          sizeof(brushvertextbn_t), OFFSET(brushvertextbn_t, s_tangent));
    glVertexAttribPointer(WSURF_VA_T_TANGENT, 3, GL_FLOAT, false, 
                          sizeof(brushvertextbn_t), OFFSET(brushvertextbn_t, t_tangent));
```

**顶点属性索引定义：**
- `WSURF_VA_NORMAL (1)`: vec3 - 法线向量
- `WSURF_VA_S_TANGENT (4)`: vec3 - S 切线向量
- `WSURF_VA_T_TANGENT (5)`: vec3 - T 切线向量

#### 6.3 配置实例属性（Instance Attributes）

```cpp
    glBindBuffer(GL_ARRAY_BUFFER, pWorldModel->hVBO[WSURF_VBO_INSTANCE]);
    
    glEnableVertexAttribArray(WSURF_VA_PACKED_MATID);    // 材质ID
    glEnableVertexAttribArray(WSURF_VA_STYLES);          // 光照样式
    glEnableVertexAttribArray(WSURF_VA_DIFFUSESCALE);    // 漫反射缩放
    
    // 材质ID（整数类型）
    glVertexAttribIPointer(WSURF_VA_PACKED_MATID, 1, GL_UNSIGNED_INT, 
                           sizeof(brushinstancedata_t), 
                           OFFSET(brushinstancedata_t, packed_matId));
    glVertexAttribDivisor(WSURF_VA_PACKED_MATID, 1);  // 实例化渲染
    
    // 光照样式（字节数组）
    glVertexAttribIPointer(WSURF_VA_STYLES, 4, GL_UNSIGNED_BYTE, 
                           sizeof(brushinstancedata_t), 
                           OFFSET(brushinstancedata_t, styles));
    glVertexAttribDivisor(WSURF_VA_STYLES, 1);
    
    // 漫反射缩放
    glVertexAttribPointer(WSURF_VA_DIFFUSESCALE, 1, GL_FLOAT, false, 
                          sizeof(brushinstancedata_t), 
                          OFFSET(brushinstancedata_t, diffusescale));
    glVertexAttribDivisor(WSURF_VA_DIFFUSESCALE, 1);
});
```

**实例属性索引定义：**
- `WSURF_VA_PACKED_MATID (6)`: uint - 打包的材质ID
- `WSURF_VA_STYLES (7)`: uvec4 - 光照样式数组
- `WSURF_VA_DIFFUSESCALE (8)`: float - 漫反射缩放因子

**实例化渲染说明：**
- `glVertexAttribDivisor(index, 1)` 表示该属性每个实例更新一次
- 允许使用 `glDrawElementsInstanced` 进行实例化渲染
- 每个表面使用不同的材质和光照参数

## 内存布局图

```
CPU 端临时缓冲区                         GPU 端缓冲区
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
│   - 三角形索引        │                │ GL_ELEMENT_ARRAY_    │
│                      │                │      BUFFER          │
└──────────────────────┘                └──────────────────────┘

                                        ┌──────────────────────┐
                                        │       hVAO           │
                                        │  (顶点数组对象)       │
                                        │  绑定上述所有缓冲区   │
                                        │  和顶点属性配置      │
                                        └──────────────────────┘
```

## 顶点属性映射表

| 属性索引 | 属性名称                  | 类型      | 数据源              | 用途                     | 实例化 |
|---------|--------------------------|----------|-------------------|-------------------------|--------|
| 0       | WSURF_VA_POSITION        | vec3     | brushvertex_t     | 顶点位置                 | ❌     |
| 1       | WSURF_VA_NORMAL          | vec3     | brushvertextbn_t  | 法线向量                 | ❌     |
| 2       | WSURF_VA_TEXCOORD        | vec2     | brushvertex_t     | 纹理坐标                 | ❌     |
| 3       | WSURF_VA_LIGHTMAP_TEXCOORD| vec2    | brushvertex_t     | 光照图坐标               | ❌     |
| 4       | WSURF_VA_S_TANGENT       | vec3     | brushvertextbn_t  | S 切线（法线贴图）        | ❌     |
| 5       | WSURF_VA_T_TANGENT       | vec3     | brushvertextbn_t  | T 切线（法线贴图）        | ❌     |
| 6       | WSURF_VA_PACKED_MATID    | uint     | brushinstancedata_t| 材质ID（漫反射+光照图）   | ✅     |
| 7       | WSURF_VA_STYLES          | uvec4    | brushinstancedata_t| 光照样式                 | ✅     |
| 8       | WSURF_VA_DIFFUSESCALE    | float    | brushinstancedata_t| 平铺纹理缩放             | ✅     |

## 关键技术点

### 1. 三缓冲区分离存储策略

将顶点数据分为三个独立的 VBO：
- **优点**：
  - 提高缓存命中率：shader 可以只访问需要的数据
  - 便于更新：可以独立更新实例数据而不影响几何数据
  - 支持实例化渲染：实例数据使用 `glVertexAttribDivisor`
- **缺点**：
  - 需要更多的缓冲区对象
  - VAO 配置稍复杂

### 2. 多边形三角形化

使用 `R_PolygonToTriangleList` 将任意凸多边形转换为三角形列表：
- BSP 原始数据使用多边形表示
- GPU 只能渲染三角形

### 3. 湍流表面双面几何

水面等湍流表面需要从两侧可见：
- 生成两套索引数据（正面和反面）
- 反面索引通过反转顺序实现
- 支持水下观察水面

### 4. 实例化渲染准备

虽然当前代码每个表面只有一个实例，但架构支持实例化：
- 使用 `glVertexAttribDivisor(index, 1)`
- 可扩展为批量渲染相同几何体但不同材质的表面
- 减少 draw call 数量

### 5. 静态数据优化

使用 `GL_STATIC_DRAW` 标志：
- 数据在初始化后不再修改
- GPU 可以将数据存储在更快的显存区域
- 适合世界几何这种静态场景

### 6. 切线空间计算

从纹理坐标系统提取切线向量：
- `s_tangent` 对应纹理 U 方向
- `t_tangent` 对应纹理 V 方向
- 配合法线构成 TBN 矩阵，用于法线贴图

### 7. 材质ID打包

使用 `uint16_t packed_matId[2]` 存储两个材质索引：
- `[0]`: 漫反射纹理材质ID
- `[1]`: 光照图纹理索引
- 节省存储空间，减少顶点数据大小

## 渲染流程（简述）

虽然本文档主要关注数据组织，但生成的 GPU 资源的使用方式大致如下：

1. 绑定 VAO：`glBindVertexArray(pWorldModel->hVAO)`
2. 绑定材质纹理和光照图
3. 设置 shader uniform（MVP 矩阵等）
4. 绘制调用：
   - 普通表面：`glDrawElementsBaseVertex(GL_TRIANGLES, count, GL_UNSIGNED_INT, start, base)`
   - 湍流表面正面：使用 `start_index` 和 `index_count`
   - 湍流表面反面：使用 `reverse_start_index` 和 `reverse_index_count`

## 性能考虑

1. **预分配内存**：使用 `reserve()` 避免动态扩容
2. **批量上传**：一次性上传所有数据，避免多次 GPU 传输
3. **索引复用**：通过 EBO 复用顶点数据
4. **缓存友好**：结构体布局考虑了对齐和访问模式
5. **静态存储**：使用 `STATIC_DRAW` 提示 GPU 优化存储位置

## 相关文件

- **实现文件**：`Plugins/Renderer/gl_wsurf.cpp`
- **头文件**：`Plugins/Renderer/gl_wsurf.h`, `Plugins/Renderer/gl_common.h`
- **着色器**：`Build/svencoop/renderer/shader/wsurf_*.glsl`（推测）

## 总结

`R_GenerateWorldSurfaceWorldModel` 函数实现了从 BSP 模型数据到现代 GPU 渲染管线的完整转换：

1. 遍历所有表面，提取几何和纹理信息
2. 计算切线空间基向量，为法线贴图做准备
3. 将多边形三角形化，生成索引缓冲
4. 为湍流表面生成双面几何
5. 创建实例数据，支持多材质和光照样式
6. 将所有数据上传到 GPU 的不同缓冲区
7. 配置 VAO，定义完整的顶点属性布局

这种设计既保持了与 GoldSrc 引擎 BSP 格式的兼容性，又充分利用了现代 OpenGL 的特性，实现了高效的世界几何渲染。


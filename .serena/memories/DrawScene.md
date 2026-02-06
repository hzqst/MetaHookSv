# Renderer Plugin - 3D场景绘制流程

## 入口函数: R_RenderScene

位置: `Plugins/Renderer/gl_rmain.cpp`

### 完整渲染流程

```
R_RenderScene()
├── R_SetupFrame()              // 帧设置
├── R_SetupGL()                 // OpenGL状态设置
├── R_SetFrustum()              // 视锥体设置
├── R_MarkLeaves()              // 标记可见叶节点
├── R_BeginRenderGBuffer()      // 开始G-Buffer渲染 (延迟渲染)
├── R_PrepareDrawWorld()        // 准备世界渲染
├── R_DrawWorld()               // 绘制世界几何体
├── R_DrawEntitiesOnList()      // 绘制不透明实体
├── R_EndRenderOpaque()         // 结束不透明物体渲染
├── ClientDLL_DrawNormalTriangles()  // 客户端DLL绘制普通三角形
└── R_DrawTransEntities()       // 绘制透明实体
```

---

## 详细流程分析

### 1. R_SetupFrame() - 帧设置
**位置**: `gl_rmain.cpp`

**功能**:
- 更新RefDef (参考定义)
- 确定当前视点所在的BSP叶节点
- 设置雾效 (水下雾、Sven Co-op雾、用户自定义雾)

**关键操作**:
```cpp
R_UpdateRefDef();
(*r_viewleaf) = Mod_PointInLeaf(r_origin, (*cl_worldmodel));
R_RenderWaterFog() / R_RenderSvenFog() / R_RenderUserFog();
```

---

### 2. R_SetupGL() - OpenGL状态设置
**功能**: 设置OpenGL渲染状态、投影矩阵、视口等

---

### 3. R_SetFrustum() - 视锥体设置
**功能**: 计算视锥体平面，用于视锥体剔除

---

### 4. R_MarkLeaves() - 标记可见叶节点
**功能**: 使用PVS (Potentially Visible Set) 标记当前可见的BSP叶节点

---

### 5. R_BeginRenderGBuffer() - 开始G-Buffer渲染
**位置**: `gl_light.cpp`

**功能**: 初始化延迟渲染的G-Buffer

**关键操作**:
```cpp
r_draw_gbuffer = true;
GL_BindFrameBuffer(&s_GBufferFBO);
R_SetGBufferMask(GBUFFER_MASK_ALL);
GL_ClearColorDepthStencil(...);
```

**G-Buffer内容**:
- 位置 (Position)
- 法线 (Normal)
- 漫反射颜色 (Diffuse)
- 高光信息 (Specular)
- 深度 (Depth)

---

### 6. R_PrepareDrawWorld() - 准备世界渲染
**功能**: 准备世界几何体渲染所需的数据和状态

---

### 7. R_DrawWorld() - 绘制世界几何体
**位置**: `gl_wsurf.cpp`

**功能**: 绘制BSP世界模型

**渲染流程**:
```cpp
R_DrawWorld()
└── R_DrawWorldSurfaceModel(pModel, entity)
    ├── R_DrawWorldSurfaceLeafSky()      // 天空表面
    ├── R_DrawWorldSurfaceLeafStatic()   // 静态表面
    ├── R_DrawWorldSurfaceLeafAnim()     // 动画表面
    └── R_DrawWorldSurfaceLeafShadow()   // 阴影
```

**表面类型**:
- **Sky** - 天空盒表面
- **Static** - 静态光照表面
- **Anim** - 动画纹理表面
- **Shadow** - 阴影投射

---

### 8. R_DrawEntitiesOnList() - 绘制不透明实体
**位置**: `gl_rmain.cpp`

**功能**: 遍历可见实体列表，绘制不透明实体

**实体分类**:
```cpp
for (int i = 0; i < (*cl_numvisedicts); ++i) {
    entity = cl_visedicts[i];
    
    if (rendermode != kRenderNormal) {
        R_AddTEntity(entity);  // 添加到透明实体列表
    }
    else if (model->type == mod_sprite && gl_spriteblend) {
        R_AddTEntity(entity);  // Sprite混合
    }
    else if (R_IsViewmodelAttachment(entity)) {
        R_AddViewModelAttachmentEntity(entity);  // 视图模型附件
    }
    else {
        R_DrawCurrentEntity(false);  // 绘制当前实体
    }
}
```

**实体类型**:
- **Studio模型** - 角色、武器等 (.mdl)
- **Brush模型** - 门、电梯等可移动BSP模型
- **Sprite** - 2D精灵 (.spr)

---

### 9. R_EndRenderOpaque() - 结束不透明物体渲染
**位置**: `gl_rmain.cpp`

**功能**: 完成G-Buffer渲染，执行延迟光照计算

**关键操作**:
```cpp
r_draw_opaque = false;
if (R_IsRenderingGBuffer()) {
    R_EndRenderGBuffer(GL_GetCurrentSceneFBO());
}
```

**延迟光照流程**:
1. G-Buffer完成几何信息存储
2. 光照Pass计算所有动态光源
3. 合成最终颜色到SceneFBO

---

### 10. ClientDLL_DrawNormalTriangles() - 客户端绘制
**功能**: 调用客户端DLL的HUD_DrawNormalTriangles，允许游戏代码绘制自定义几何体

---

### 11. R_DrawTransEntities() - 绘制透明实体
**位置**: `gl_rmain.cpp`

**功能**: 绘制所有透明物体

**两种渲染模式**:

#### A. OIT (Order-Independent Transparency) 模式
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

**OIT特点**:
- 透明物体顺序无关
- 使用链表存储透明片段
- GPU排序和混合
- 性能开销较大

#### B. 传统Alpha混合模式
```cpp
else {
    R_DrawTEntitiesOnList(onlyClientDraw);
    ClientDLL_DrawTransparentTriangles();
    R_DrawParticles();
}
```

**传统模式特点**:
- 需要从后向前排序
- 标准Alpha混合
- 性能较好

**透明物体包括**:
- 透明实体 (rendermode != kRenderNormal)
- 透明三角形 (客户端DLL)
- 粒子系统

---

## 渲染管线架构

### 延迟渲染管线 (Deferred Rendering)

```
[几何Pass - Geometry Pass]
    ↓
[G-Buffer]
├── Position Buffer
├── Normal Buffer
├── Diffuse Buffer
├── Specular Buffer
└── Depth Buffer
    ↓
[光照Pass - Lighting Pass]
├── 动态点光源
├── 聚光灯
├── 方向光
└── 环境光
    ↓
[合成 - Composition]
    ↓
[透明Pass - Transparent Pass]
    ↓
[后期处理 - Post-Processing]
├── HDR
├── SSAO
├── SSR
├── FXAA
└── Gamma校正
```

---

## 关键数据结构

### refdef_t - 渲染定义
```cpp
typedef struct refdef_s {
    vrect_GoldSrc_t *vrect;      // 视口矩形
    vec3_t *vieworg;              // 视点位置
    vec3_t *viewangles;           // 视角
    color24 *ambientlight;        // 环境光
    qboolean *onlyClientDraws;    // 仅客户端绘制标志
} refdef_t;
```

### 全局渲染状态
```cpp
extern refdef_t r_refdef;
extern float r_xfov, r_yfov;           // FOV
extern bool r_fog_enabled;              // 雾效启用
extern cl_entity_t* r_worldentity;      // 世界实体
extern model_t** cl_worldmodel;         // 世界模型
```

---

## 性能优化技术

### 1. 视锥体剔除 (Frustum Culling)
- `R_SetFrustum()` 计算视锥体平面
- 剔除视锥体外的物体

### 2. PVS剔除 (Potentially Visible Set)
- `R_MarkLeaves()` 使用BSP的PVS数据
- 只渲染可能可见的叶节点

### 3. VBO批量绘制
- 使用Vertex Buffer Object
- 减少Draw Call数量

### 4. 延迟渲染
- 减少多光源场景的光照计算开销
- 光照计算只在可见像素上进行

### 5. 异步资源加载
- 后台线程加载模型和纹理
- 避免主线程阻塞

---

## 调试工具

### OpenGL调试组
```cpp
GL_BeginDebugGroup("R_RenderScene");
// ... 渲染代码 ...
GL_EndDebugGroup();
```

使用RenderDoc等工具可以查看每个调试组的渲染调用。

---

## 相关控制台变量 (CVars)

- `r_drawentities` - 是否绘制实体
- `r_drawworld` - 是否绘制世界
- `r_deferred_lighting` - 启用延迟光照
- `gl_spriteblend` - Sprite混合模式
- `r_fog` - 雾效设置

---

## 总结

Renderer插件的3D场景绘制流程采用现代化的延迟渲染管线:

1. **几何Pass** - 将场景几何信息写入G-Buffer
2. **光照Pass** - 在屏幕空间计算光照
3. **透明Pass** - 绘制透明物体
4. **后期处理** - 应用各种图像效果

这种架构支持大量动态光源，同时保持良好的性能。通过VBO批量绘制、视锥体剔除、PVS剔除等优化技术，即使在复杂场景中也能维持高帧率。

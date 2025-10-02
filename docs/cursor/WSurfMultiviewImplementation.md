# WorldSurface Multiview渲染实现文档

## 概述

本文档描述了为WorldSurface渲染器实现多视角(Multiview)渲染功能的完整方案。该功能允许在单次Draw调用中渲染多个视角，用于支持：
1. **Cubemap Shadow Mapping** - 单次渲染6个视角到Cubemap
2. **Cascaded Shadow Mapping (CSM)** - 单次渲染4个级联阴影到TextureArray

## 实现架构

### 1. Program State 标志位

在 `Plugins/Renderer/gl_wsurf.h` 中添加了新的着色器状态位：

```cpp
#define WSURF_MULTIVIEW_ENABLED  0x20000000ull
```

该标志位用于在编译着色器时启用multiview相关的宏定义和几何着色器。

### 2. C++代码修改

#### 2.1 着色器编译 (gl_wsurf.cpp)

在 `R_UseWSurfProgram` 函数中：
- 当 `state & WSURF_MULTIVIEW_ENABLED` 时，添加 `WSURF_MULTIVIEW_ENABLED` 宏定义
- 当启用multiview时，指定几何着色器文件路径 `wsurf_shader.geom.glsl`

```cpp
if (state & WSURF_MULTIVIEW_ENABLED)
    defs << "#define WSURF_MULTIVIEW_ENABLED\n";

CCompileShaderArgs args;
args.vsfile = "renderer\\shader\\wsurf_shader.vert.glsl";
if (state & WSURF_MULTIVIEW_ENABLED)
    args.gsfile = "renderer\\shader\\wsurf_shader.geom.glsl";
args.fsfile = "renderer\\shader\\wsurf_shader.frag.glsl";
```

#### 2.2 运行时启用

在以下绘制函数中添加了multiview检测：
- `R_DrawWorldSurfaceModelShadowProxyInternal`
- `R_DrawWorldSurfaceLeafShadow`
- `R_DrawWorldSurfaceLeafStatic`
- `R_DrawWorldSurfaceLeafAnim`
- `R_DrawWorldSurfaceLeafSky`

当 `r_draw_multiview` 为 true 时，自动添加 `WSURF_MULTIVIEW_ENABLED` 标志：

```cpp
if (r_draw_multiview)
{
    WSurfProgramState |= WSURF_MULTIVIEW_ENABLED;
}
```

### 3. 着色器实现

#### 3.1 Vertex Shader (wsurf_shader.vert.glsl)

顶点着色器保持不变，继续输出 `v_` 前缀的变量：
- `v_worldpos`, `v_normal`, `v_tangent` 等
- 这些变量会被几何着色器接收

#### 3.2 Geometry Shader (wsurf_shader.geom.glsl) - 新增

**输入配置：**
```glsl
layout(triangles) in;

#ifdef WSURF_MULTIVIEW_ENABLED
    layout(triangle_strip, max_vertices = 18) out;  // 3 * 6 views
#else
    layout(triangle_strip, max_vertices = 3) out;
#endif
```

**核心逻辑：**

启用multiview时：
```glsl
#ifdef WSURF_MULTIVIEW_ENABLED
    int numViews = CameraUBO.numViews;
    
    for (int viewIdx = 0; viewIdx < numViews; ++viewIdx)
    {
        gl_Layer = viewIdx;  // 设置TextureArray层
        
        for (int i = 0; i < 3; ++i)
        {
            // 使用对应视角的矩阵变换
            vec4 worldPos = vec4(v_worldpos[i], 1.0);
            gl_Position = GetCameraProjMatrix(viewIdx) * 
                         GetCameraWorldMatrix(viewIdx) * worldPos;
            
            // 传递所有属性到片段着色器
            g_worldpos = v_worldpos[i];
            g_normal = v_normal[i];
            // ... 其他属性
            
            EmitVertex();
        }
        EndPrimitive();
    }
#endif
```

未启用multiview时，几何着色器简单透传：
```glsl
#else
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        // 传递所有属性
        EmitVertex();
    }
    EndPrimitive();
#endif
```

#### 3.3 Fragment Shader (wsurf_shader.frag.glsl)

通过预处理器宏适配输入来源：

```glsl
#ifdef WSURF_MULTIVIEW_ENABLED
    // 来自几何着色器的 g_ 前缀变量
    #define v_worldpos g_worldpos
    #define v_normal g_normal
    // ... 其他变量映射
    
    in vec3 g_worldpos;
    in vec3 g_normal;
    // ... 其他输入
#else
    // 直接来自顶点着色器的 v_ 前缀变量
    in vec3 v_worldpos;
    in vec3 v_normal;
    // ... 其他输入
#endif
```

这样片段着色器的其余代码无需修改，继续使用 `v_` 前缀访问变量。

## 使用方法

### 前置条件
- OpenGL 4.4+ Core Profile
- 支持几何着色器
- 支持TextureArray渲染目标

### 启用Multiview渲染

1. **设置CameraUBO：**
```cpp
camera_ubo_t CameraUBO{};

// 为每个视角设置变换矩阵
for (int i = 0; i < 6; ++i)  // 例如：Cubemap的6个面
{
    R_SetupCameraView(&CameraUBO.views[i]);
}

CameraUBO.numViews = 6;  // 或者4用于CSM

GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, 
                      sizeof(CameraUBO), &CameraUBO);
```

2. **启用multiview标志：**
```cpp
r_draw_multiview = true;
```

3. **绘制场景：**
```cpp
// 正常调用绘制函数
R_RenderScene();
```

4. **恢复状态：**
```cpp
r_draw_multiview = false;
```

### Cubemap Shadow示例

```cpp
// 设置6个立方体贴图面的视角
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
R_RenderScene();  // 单次Draw渲染6个面
r_draw_multiview = false;
```

### CSM优化示例

当前CSM绘制到4096x4096画布的4个2048x2048区域。使用multiview后可以优化为：

```cpp
// 创建2048x2048的TextureArray (4层)
glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, 
             2048, 2048, 4, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

// 设置4个CSM级联的视角
camera_ubo_t CameraUBO{};
for (int i = 0; i < 4; ++i)
{
    // 计算每个级联的视锥体
    SetupCSMFrustum(i, &CameraUBO.views[i]);
}

CameraUBO.numViews = 4;
GL_UploadSubDataToUBO(g_WorldSurfaceRenderer.hCameraUBO, 0, sizeof(CameraUBO), &CameraUBO);

r_draw_multiview = true;
R_RenderScene();  // 单次Draw渲染4个CSM层
r_draw_multiview = false;

// 几何着色器会自动设置gl_Layer，将每个视角渲染到对应的TextureArray层
```

## 性能优化

### 优势
1. **减少Draw Call** - 从N次绘制优化为1次绘制
2. **减少状态切换** - 不需要切换FBO和Viewport
3. **减少CPU开销** - 只需要一次设置渲染状态
4. **提高GPU效率** - 更好的批处理和并行化

### 注意事项
1. **几何着色器开销** - 会增加一定的GPU开销，适合顶点数量适中的场景
2. **最大顶点输出限制** - 当前设置为18个顶点(3*6)，适用于Cubemap
3. **内存占用** - 几何着色器会占用更多GPU缓存

### 性能对比

**传统方式（绘制6个Cubemap面）：**
- 6次Draw Call
- 6次FBO切换
- 6次Viewport设置
- 6次状态设置

**Multiview方式：**
- 1次Draw Call
- 1次状态设置
- 几何着色器自动分发到6个层

预期性能提升：**30-60%** (取决于场景复杂度)

## 扩展到其他着色器

相同的模式可以应用到：
- **StudioModel** - 添加 `STUDIO_MULTIVIEW_ENABLED`
- **Sprite** - 添加 `SPRITE_MULTIVIEW_ENABLED`
- **TriAPI** - 添加 `TRIAPI_MULTIVIEW_ENABLED`
- **Portal** - 添加 `PORTAL_MULTIVIEW_ENABLED`

每个都需要：
1. 在头文件中定义状态位
2. 在R_Use*Program函数中添加宏和几何着色器
3. 创建对应的.geom.glsl文件
4. 修改.frag.glsl支持g_前缀输入

## 调试建议

1. **验证numViews** - 确保CameraUBO.numViews设置正确
2. **检查矩阵** - 使用RenderDoc查看每个视角的变换矩阵
3. **Layer验证** - 确认gl_Layer正确设置到对应的TextureArray层
4. **性能分析** - 使用GPU分析工具对比multiview前后的性能

## 已知限制

1. 几何着色器最大顶点输出为18 (适用于6视角)
2. 需要OpenGL 4.3+支持
3. 不支持MSAA的TextureArray (需要额外处理)
4. 某些移动GPU对几何着色器支持有限

## 后续优化方向

1. **Mesh Shader** - 使用Mesh Shader替代几何着色器 (需OpenGL 4.6+)
2. **Multi-Draw Indirect** - 结合MDI进一步优化
3. **Compute Culling** - 使用Compute Shader进行多视角剔除
4. **View Instancing** - 探索ARB_shader_viewport_layer_array扩展

## 参考资料

- OpenGL 4.4 Geometry Shader规范
- NVIDIA Multi-View Rendering白皮书
- Cascaded Shadow Maps实现指南


# Multiview深度问题调试指南

## 问题描述

启用 `WSURF_MULTIVIEW_ENABLED` 或 `STUDIO_MULTIVIEW_ENABLED` 后，通过RenderDoc抓帧发现渲染出来的深度中的几何位置不正确。

## 可能的原因分析

### 1. 坐标变换流程回顾

#### Vertex Shader (未启用几何着色器)
```glsl
// 模型空间 → 世界空间
vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
v_worldpos = worldpos4.xyz;

// 世界空间 → 裁剪空间
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;
//           = projMatrix * viewMatrix * worldPos
```

#### Geometry Shader (Multiview模式)
```glsl
// 从vertex shader接收世界坐标
vec4 worldPos = vec4(v_worldpos[i], 1.0);

// 为每个视角重新计算裁剪空间坐标
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

### 2. 问题诊断检查点

使用RenderDoc进行以下检查：

#### 检查点1：验证CameraUBO数据
```cpp
// 在R_SetupCameraView后，打印worldMatrix和projMatrix
camera_ubo_t CameraUBO{};
for (int i = 0; i < numViews; ++i)
{
    R_SetupCameraView(&CameraUBO.views[i]);
    
    // 打印调试信息
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

#### 检查点2：对比非multiview和multiview的gl_Position

在RenderDoc中：
1. 捕获一帧非multiview渲染（r_draw_multiview = false）
2. 捕获一帧multiview渲染（r_draw_multiview = true）
3. 对比同一个顶点在两种模式下的：
   - `v_worldpos` - 应该相同
   - `gl_Position` - 对于view 0应该相同

#### 检查点3：检查gl_Layer

在RenderDoc中验证：
- `gl_Layer` 是否正确设置为 0, 1, 2... numViews-1
- 每个layer的深度纹理是否都有内容
- 每个layer的内容是否对应正确的视角

### 3. 常见问题

#### 问题A：矩阵顺序错误

**错误写法：**
```glsl
// ❌ 错误：矩阵顺序反了
gl_Position = GetCameraWorldMatrix(viewIdx) * GetCameraProjMatrix(viewIdx) * worldPos;
```

**正确写法：**
```glsl
// ✅ 正确：先view变换，再projection变换
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

#### 问题B：worldMatrix命名误导

`CameraUBO.views[].worldMatrix` 实际上是 **view matrix**（世界空间→相机空间），不是模型空间→世界空间的变换。

```cpp
// 在R_SetupCameraView中
memcpy(view->worldMatrix, r_world_matrix, sizeof(mat4));
// r_world_matrix实际上是view matrix!
```

#### 问题C：EntityMatrix遗漏

确认vertex shader中已经应用了EntityMatrix：
```glsl
vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
```

如果几何着色器收到的`v_worldpos`不包含EntityMatrix变换，那就是问题所在。

#### 问题D：视角矩阵未更新

在设置multiview时，确保为每个视角都调用了：
```cpp
for (int i = 0; i < numViews; ++i)
{
    // 设置视角方向
    VectorCopy(viewAngles[i], (*r_refdef.viewangles));
    VectorCopy(viewOrigin, (*r_refdef.vieworg));
    
    // 重新构建view matrix
    R_LoadIdentityForWorldMatrix();
    R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));
    
    // 保存到CameraUBO
    R_SetupCameraView(&CameraUBO.views[i]);
}
```

### 4. 调试代码示例

#### 在Vertex Shader中添加调试输出

```glsl
void main(void)
{
    // ... 正常计算 ...
    
    vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
    v_worldpos = worldpos4.xyz;
    
    gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;
    v_projpos = gl_Position;
    
    // 调试：将世界坐标编码到颜色中（仅用于调试）
    #ifdef DEBUG_WORLDPOS
        v_debug_color = vec4(
            (v_worldpos.x + 1000.0) / 2000.0,  // 假设场景在[-1000, 1000]范围
            (v_worldpos.y + 1000.0) / 2000.0,
            (v_worldpos.z + 1000.0) / 2000.0,
            1.0
        );
    #endif
}
```

#### 在Geometry Shader中添加调试

```glsl
#ifdef WSURF_MULTIVIEW_ENABLED
    for (int viewIdx = 0; viewIdx < numViews; ++viewIdx)
    {
        gl_Layer = viewIdx;
        
        for (int i = 0; i < 3; ++i)
        {
            vec4 worldPos = vec4(v_worldpos[i], 1.0);
            
            // 调试：打印第一个三角形的世界坐标
            #ifdef DEBUG_MULTIVIEW
                if (gl_PrimitiveIDIn == 0 && i == 0)
                {
                    // 这在GLSL中无法直接打印，但可以编码到颜色
                    g_debug_info = vec4(float(viewIdx), worldPos.xyz);
                }
            #endif
            
            gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
            
            // ... 传递其他属性 ...
            
            EmitVertex();
        }
        EndPrimitive();
    }
#endif
```

#### 在C++中添加验证代码

```cpp
// 在gl_shadow.cpp中，设置CameraUBO后添加
camera_ubo_t CameraUBO{};

for (int i = 0; i < 6; ++i)
{
    VectorCopy(args->origin, (*r_refdef.vieworg));
    VectorCopy(cubemapAngles[i], (*r_refdef.viewangles));
    
    R_LoadIdentityForWorldMatrix();
    R_SetupPlayerViewWorldMatrix((*r_refdef.vieworg), (*r_refdef.viewangles));
    R_SetupCameraView(&CameraUBO.views[i]);
    
    // 调试：验证view matrix
    auto& view = CameraUBO.views[i];
    gEngfuncs.Con_DPrintf("Cubemap face %d:\n", i);
    gEngfuncs.Con_DPrintf("  viewpos: [%f, %f, %f]\n", 
        view.viewpos[0], view.viewpos[1], view.viewpos[2]);
    gEngfuncs.Con_DPrintf("  vpn: [%f, %f, %f]\n",
        view.vpn[0], view.vpn[1], view.vpn[2]);
    
    // 验证一个测试点的变换
    vec3_t testPoint = {100.0f, 0.0f, 0.0f};
    vec4_t worldPoint = {testPoint[0], testPoint[1], testPoint[2], 1.0f};
    vec4_t viewPoint;
    
    // 手动矩阵变换
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

### 5. RenderDoc分析步骤

1. **捕获帧** - 在shadow pass时按F12捕获

2. **查看Texture Viewer**
   - 切换到Depth/Stencil view
   - 如果是TextureArray，选择不同的array slice查看

3. **检查Draw Call**
   - 找到WorldSurface或StudioModel的draw call
   - 查看Vertex Input中的`in_vertex`和vertex output中的`v_worldpos`
   - 确认`v_worldpos`是否合理

4. **检查Geometry Shader Output**
   - 查看`gl_Layer`的值
   - 查看`gl_Position`的值
   - 对比不同层的`gl_Position`是否反映了不同的视角

5. **检查Uniform Buffer**
   - 展开CameraUBO
   - 查看`views[0]`, `views[1]`...的worldMatrix和projMatrix
   - 验证`numViews`的值

### 6. 已知问题排查

#### 如果所有视角的深度都一样
→ 可能`gl_Layer`没有正确设置，或者几何着色器没有被调用

#### 如果深度值都是0或1
→ 可能projection matrix有问题，检查near/far plane设置

#### 如果几何位置偏移
→ 可能view matrix有问题，检查R_SetupPlayerViewWorldMatrix的调用

#### 如果只有第一个视角正确
→ 可能循环中viewIdx使用错误，或者CameraUBO只设置了views[0]

### 7. 快速修复尝试

如果问题是深度不正确，尝试以下修改：

#### 修改A：确保使用正确的世界坐标

在geometry shader中，确认v_worldpos已经是完整的世界坐标：

```glsl
// 在multiview路径中
vec4 worldPos = vec4(v_worldpos[i], 1.0);

// 不要再应用EntityMatrix！因为v_worldpos已经包含了
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

#### 修改B：手动验证矩阵

临时添加debug路径，使用view 0的矩阵：

```glsl
#ifdef DEBUG_USE_VIEW0
    // 临时：所有视角都使用view 0的矩阵
    gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldPos;
#else
    gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
#endif
```

如果使用view 0正确，说明其他view的矩阵有问题。

### 8. 期望的结果

正确实现后，RenderDoc应该显示：
- 每个TextureArray层有不同视角的深度
- Cubemap的6个面应该看到不同方向的几何
- CSM的4个层应该看到不同距离范围的几何
- 深度值应该在合理范围内（不全是0或1）
- 几何位置应该与场景实际布局一致

## 总结

最可能的问题：
1. ✅ CameraUBO的view matrix设置不正确
2. ✅ 几何着色器中的矩阵乘法顺序错误
3. ✅ `v_worldpos`不是真正的世界坐标
4. ✅ 只有第一个view的矩阵被正确设置

请使用RenderDoc按照上述步骤逐一排查！


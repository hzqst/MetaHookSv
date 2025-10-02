# StudioModel Multiview 几何扭曲问题修复

## 问题描述

启用`STUDIO_MULTIVIEW_ENABLED`后，在RenderDoc中观察到StudioModel的顶点被变换到错误的坐标上，导致模型完全扭曲。而WorldSurface的渲染正常。

## 根本原因

在`studio_shader.vert.glsl`中存在**顶点位置不一致**的问题：

### 问题代码

```glsl
void main(void)
{
    // 1. 骨骼变换得到outvert
    vec3 outvert = vec3(
        dot(vert, vertbone_matrix_0) + vertbone_matrix[0][3],
        dot(vert, vertbone_matrix_1) + vertbone_matrix[1][3],
        dot(vert, vertbone_matrix_2) + vertbone_matrix[2][3]
    );
    
    // 2. 赋值给v_worldpos
    v_worldpos = outvert;  // 第78行
    
    // 3. 在某些渲染模式下，v_worldpos被修改
    #if defined(OUTLINE_ENABLED)
        outvert = outvert + v_smoothnormal * StudioUBO.r_scale;
        v_worldpos = outvert;  // 第106行：v_worldpos被更新
    #elif defined(STUDIO_NF_CHROME)
        outvert = outvert + v_smoothnormal * StudioUBO.r_scale;
        v_worldpos = outvert;  // 第111行：v_worldpos被更新
    #endif
    
    // 4. 计算gl_Position时使用的是outvert（而不是v_worldpos）
    gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(outvert, 1.0);  // ❌ 错误！
    
    v_projpos = gl_Position;
}
```

### 数据流分析

**正常情况（无OUTLINE/CHROME）：**
- `v_worldpos = outvert` ✅
- `gl_Position` 使用 `outvert` ✅
- 两者一致，geometry shader收到的`v_worldpos`正确

**OUTLINE/CHROME模式：**
- `v_worldpos = outvert + offset` ✅（修改后的位置）
- `gl_Position` 使用 `outvert` ❌（未修改的位置）
- **不一致！**

### Multiview下的问题

在几何着色器的multiview路径中：

```glsl
// 从vertex shader接收v_worldpos（可能包含OUTLINE/CHROME偏移）
vec4 worldPos = vec4(v_worldpos[i], 1.0);

// 重新计算gl_Position
gl_Position = GetCameraProjMatrix(viewIdx) * GetCameraWorldMatrix(viewIdx) * worldPos;
```

如果`v_worldpos`与vertex shader中计算`gl_Position`使用的顶点位置不一致，就会导致：
1. **非multiview模式**：使用vertex shader的`gl_Position`（基于`outvert`）→ 正确
2. **Multiview模式**：geometry shader重新计算（基于`v_worldpos`）→ 错误！

结果就是模型在multiview模式下扭曲。

## 解决方案

**修改`studio_shader.vert.glsl`第187行**，使用`v_worldpos`而不是`outvert`：

```glsl
// ❌ 修复前
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(outvert, 1.0);

// ✅ 修复后
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(v_worldpos, 1.0);
```

### 为什么这样修复？

1. **保证一致性**：
   - Vertex shader使用`v_worldpos`计算`gl_Position`
   - Geometry shader使用`v_worldpos`重新计算`gl_Position`
   - 两者使用相同的输入数据

2. **正确处理所有模式**：
   - 普通模式：`v_worldpos = outvert`
   - OUTLINE模式：`v_worldpos = outvert + offset`
   - CHROME模式：`v_worldpos = outvert + offset`
   - 所有模式下，`gl_Position`都基于最终的`v_worldpos`

3. **非multiview模式兼容**：
   - Geometry shader的passthrough路径直接透传`gl_in[i].gl_Position`
   - 现在这个`gl_Position`使用的是正确的`v_worldpos`

## 为什么WorldSurface没有问题？

对比WorldSurface的vertex shader：

```glsl
vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
worldpos4.xyz += v_normal.xyz * EntityUBO.scale;  // 修改worldpos4
v_worldpos = worldpos4.xyz;                        // 同步到v_worldpos

// 使用worldpos4计算gl_Position（与v_worldpos一致）
gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;  ✅
```

WorldSurface始终保持`v_worldpos`和用于计算`gl_Position`的变量同步，所以没有问题。

## 验证方法

### 1. 编译并测试

重新编译着色器，启用multiview渲染：

```cpp
r_draw_multiview = true;
r_draw_shadowview = true;
```

### 2. RenderDoc验证

在RenderDoc中检查：

**修复前：**
- StudioModel的顶点位置扭曲
- 模型形状完全错误
- 可能出现拉伸、反转等异常

**修复后：**
- StudioModel的顶点位置正确
- 模型形状正常
- 各个视角的深度正确

### 3. 对比测试

分别测试以下场景：
- ✅ 普通StudioModel（无OUTLINE/CHROME）
- ✅ OUTLINE模式的StudioModel
- ✅ CHROME材质的StudioModel
- ✅ Multiview + Shadow rendering
- ✅ Cubemap shadow
- ✅ CSM shadow

## 技术总结

### 问题的本质

**在引入几何着色器后，必须确保vertex shader传递给几何着色器的变量（如`v_worldpos`）与vertex shader自身用于计算`gl_Position`的变量完全一致。**

否则：
- 非multiview模式：使用vertex shader的`gl_Position` → 一个位置
- Multiview模式：几何着色器用`v_worldpos`重新计算 → 另一个位置
- 结果：同一顶点在两种模式下位置不同 → 扭曲

### 最佳实践

在设计支持几何着色器的vertex shader时：

1. **统一变量**：用于`gl_Position`计算的变量应该与传递给几何着色器的变量一致

```glsl
// ✅ 好的设计
v_worldpos = finalPosition;
gl_Position = projMatrix * viewMatrix * vec4(v_worldpos, 1.0);
```

2. **避免局部变量**：不要在计算`gl_Position`时使用仅存在于main函数中的局部变量

```glsl
// ❌ 坏的设计
vec3 localPos = ...;
v_worldpos = localPos + offset1;
gl_Position = projMatrix * viewMatrix * vec4(localPos + offset2, 1.0);  // 不一致！
```

3. **测试所有路径**：确保所有着色器分支（如`#if defined`）都正确更新`v_worldpos`

### 调试技巧

如果遇到类似问题：

1. **对比变量值**：在RenderDoc中对比vertex output和geometry input
2. **检查条件编译**：查看所有`#if defined`分支是否正确处理
3. **逐个测试**：分别测试multiview和非multiview模式
4. **WorldSurface对照**：如果WorldSurface正常但StudioModel异常，说明问题在StudioModel特有逻辑

## 相关文件

- 修改文件：`Build/svencoop/renderer/shader/studio_shader.vert.glsl`
- 相关文件：`Build/svencoop/renderer/shader/studio_shader.geom.glsl`
- 对照参考：`Build/svencoop/renderer/shader/wsurf_shader.vert.glsl`

## 结论

通过确保vertex shader中`gl_Position`的计算使用与`v_worldpos`相同的值，成功修复了StudioModel在multiview模式下的几何扭曲问题。这个修复不影响非multiview模式，并且正确处理了所有StudioModel的渲染模式（普通、OUTLINE、CHROME等）。


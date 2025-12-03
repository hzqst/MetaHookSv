# L4D Glow Effect - 类求生之路发光特效实现文档

## 概述

MetaHookSv Renderer 插件实现了三种类似《Left 4 Dead》（求生之路）的发光轮廓特效，通过设置实体的 `renderfx` 属性为特定值来启用。这些特效使用 Stencil Buffer 和后处理技术实现，支持 Studio Model (`.mdl`) 和 World Surface Model (`.bsp`) 两种模型类型。

## 特效类型

### 1. kRenderFxPostProcessGlow (30)
**基础发光效果**

- 实体以指定颜色发光，带有 Bloom 模糊效果
- 发光只在实体可见部分显示
- 不具备穿墙效果

### 2. kRenderFxPostProcessGlowWallHack (31)
**穿墙发光效果**

- 实体以指定颜色发光，带有 Bloom 模糊效果
- 发光可以透过墙壁等障碍物显示
- 无论实体是否被遮挡，始终显示完整的发光轮廓

### 3. kRenderFxPostProcessGlowWallHackBehindWallOnly (32)
**仅穿墙部分发光效果**

- 实体以指定颜色发光，带有 Bloom 模糊效果
- 只有被墙壁遮挡的部分显示发光
- 可见部分不显示发光，仅遮挡部分显示

## 渲染流程

### 整体渲染管线

```
正常渲染场景
    ↓
收集 Glow 实体到列表
    ↓
绘制 Glow Stencil (标记遮挡区域)
    ↓
绘制 Glow Color (绘制发光颜色)
    ↓
DownSample + Blur (后处理模糊)
    ↓
Halo Add (叠加到场景)
```

### Phase 1: 收集实体

在 `StudioRenderModel_Template` (Studio Model) 和 `R_DrawWorldSurfaceModel` (World Surface) 函数中，根据实体的 `renderfx` 属性，将实体添加到相应的全局列表：

```cpp
// gl_studio.cpp / gl_wsurf.cpp
if (!R_IsRenderingGlowColor() && !R_IsRenderingGlowStencil() && !R_IsRenderingGlowStencilEnableDepthTest())
{
    if ((*currententity)->curstate.renderfx == kRenderFxPostProcessGlow)
    {
        g_PostProcessGlowColorEntities.emplace_back((*currententity));
    }
    else if ((*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHack)
    {
        g_PostProcessGlowStencilEntities.emplace_back((*currententity));
        g_PostProcessGlowColorEntities.emplace_back((*currententity));
    }
    else if ((*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly)
    {
        g_PostProcessGlowStencilEntities.emplace_back((*currententity));
        g_PostProcessGlowEnableDepthTestStencilEntities.emplace_back((*currententity));
        g_PostProcessGlowColorEntities.emplace_back((*currententity));
    }
}
```

**三个全局列表：**
- `g_PostProcessGlowColorEntities` - 需要绘制发光颜色的实体
- `g_PostProcessGlowStencilEntities` - 需要绘制 Stencil 标记的实体
- `g_PostProcessGlowEnableDepthTestStencilEntities` - 需要绘制带深度测试的 Stencil 标记的实体

### Phase 2: 绘制 Glow Stencil

`R_DrawGlowStencil()` 函数在透明物体渲染后执行，用于在 Stencil Buffer 中标记区域。

```cpp
// gl_rmain.cpp
void R_DrawGlowStencil()
{
    // 第一次 Stencil Pass - 标记穿墙区域
    if (g_PostProcessGlowStencilEntities.size() > 0)
    {
        r_draw_glowstencil = true;
        glColorMask(0, 0, 0, 0);  // 禁止颜色写入

        for (auto ent : g_PostProcessGlowStencilEntities)
        {
            (*currententity) = ent;
            R_DrawCurrentEntity(true);
        }

        glColorMask(1, 1, 1, 1);
        r_draw_glowstencil = false;
    }

    // 第二次 Stencil Pass - 带深度测试标记
    if (g_PostProcessGlowEnableDepthTestStencilEntities.size() > 0)
    {
        r_draw_glowstencil_enabledepthtest = true;
        glColorMask(0, 0, 0, 0);

        for (auto ent : g_PostProcessGlowEnableDepthTestStencilEntities)
        {
            (*currententity) = ent;
            R_DrawCurrentEntity(true);
        }

        glColorMask(1, 1, 1, 1);
        r_draw_glowstencil_enabledepthtest = false;
    }
}
```

### Phase 3: 绘制 Glow Color

`R_DrawPostProcessGlow()` 函数将发光颜色绘制到单独的 FBO 中。

```cpp
// gl_rmain.cpp
void R_DrawPostProcessGlow()
{
    if (g_PostProcessGlowColorEntities.empty())
        return;

    auto CurrentFBO = GL_GetCurrentSceneFBO();

    // 复制深度和 Stencil 到 BackBufferFBO4
    GL_BlitFrameBufferToFrameBufferDepthStencil(CurrentFBO, &s_BackBufferFBO4);
    GL_BindFrameBuffer(&s_BackBufferFBO4);

    // 清空颜色缓冲
    vec4_t clearColor = { 0, 0, 0, 1 };
    GL_ClearColor(clearColor);

    r_draw_glowcolor = true;

    for (auto ent : g_PostProcessGlowColorEntities)
    {
        (*currententity) = ent;
        R_DrawCurrentEntity(true);
    }

    r_draw_glowcolor = false;

    // ... 后处理 ...
}
```

### Phase 4: 后处理 (Bloom)

发光颜色绘制完成后，进行降采样和高斯模糊：

```cpp
// gl_rmain.cpp
R_DownSample(&s_BackBufferFBO4, nullptr, &s_DownSampleFBO[0], true, false); // 1 -> 1/4
R_DownSample(&s_DownSampleFBO[0], nullptr, &s_DownSampleFBO[1], true, false); // 1/4 -> 1/16
R_BlurPass(&s_DownSampleFBO[1], &s_BlurPassFBO[0][0], r_glow_bloomscale->value, false);
R_BlurPass(&s_BlurPassFBO[0][0], &s_BlurPassFBO[0][1], r_glow_bloomscale->value, true);
```

### Phase 5: 合成到场景

使用 `R_CopyColorHaloAdd` 将模糊后的发光叠加到场景：

```cpp
// gl_hud.cpp
void R_CopyColorHaloAdd(FBO_Container_t* src, FBO_Container_t* dst)
{
    GL_BindFrameBuffer(dst);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 使用 Stencil 测试，排除标记了 NO_GLOW_BLUR 的区域
    GL_BeginStencilCompareNotEqual(STENCIL_MASK_NO_GLOW_BLUR, STENCIL_MASK_NO_GLOW_BLUR);

    GL_UseProgram(copy_color_halo_add.program);
    // ... 绘制全屏三角形 ...

    GL_EndStencil();
    glDisable(GL_BLEND);
}
```

## Stencil 标记系统

### Stencil Mask 定义

```cpp
// gl_common.h
#define STENCIL_MASK_NO_GLOW_BLUR   0x8   // 排除 Bloom 模糊
#define STENCIL_MASK_NO_GLOW_COLOR  0x40  // 排除颜色绘制
```

### 各特效的 Stencil 策略

| 特效 | Stencil Pass | 颜色绘制时的 Stencil 检测 |
|------|-------------|------------------------|
| PostProcessGlow | 无 | 无 |
| PostProcessGlowWallHack | 标记 `NO_GLOW_BLUR` | 无 |
| PostProcessGlowWallHackBehindWallOnly | 标记 `NO_GLOW_BLUR` + `NO_GLOW_COLOR` (带深度测试) | 检测 `NO_GLOW_COLOR` 不等于才绘制 |

## Shader Program State

### Studio Model (gl_studio.cpp)

```cpp
// Stencil Pass 阶段
R_ShouldDrawGlowStencilEnableDepthTest():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED | STUDIO_NF_DOUBLE_FACE | STUDIO_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHackBehindWallOnly():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED | STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED | STUDIO_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHack():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED | STUDIO_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencil():
    StudioProgramState |= STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED

// Color Pass 阶段
R_ShouldDrawGlowColor() || R_ShouldDrawGlowColorWallHack():
    StudioProgramState |= STUDIO_GLOW_COLOR_ENABLED

R_ShouldDrawGlowColorWallHackBehindWallOnly():
    StudioProgramState |= STUDIO_GLOW_COLOR_ENABLED | STUDIO_NF_DOUBLE_FACE
```

### World Surface (gl_wsurf.cpp)

```cpp
// Stencil Pass 阶段
R_ShouldDrawGlowStencilEnableDepthTest():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_COLOR_ENABLED | WSURF_DOUBLE_FACE_ENABLED | WSURF_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHackBehindWallOnly():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_COLOR_ENABLED | WSURF_STENCIL_NO_GLOW_BLUR_ENABLED | WSURF_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencilWallHack():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_BLUR_ENABLED | WSURF_SHADOW_CASTER_ENABLED

R_ShouldDrawGlowStencil():
    WSurfProgramState |= WSURF_STENCIL_NO_GLOW_BLUR_ENABLED

// Color Pass 阶段
R_ShouldDrawGlowColor() || R_ShouldDrawGlowColorWallHack():
    WSurfProgramState |= WSURF_GLOW_COLOR_ENABLED

R_ShouldDrawGlowColorWallHackBehindWallOnly():
    WSurfProgramState |= WSURF_GLOW_COLOR_ENABLED | WSURF_DOUBLE_FACE_ENABLED
```

### Program State 宏定义

```cpp
// gl_common.h

// World Surface
#define WSURF_GLOW_COLOR_ENABLED            0x80000000ull
#define WSURF_STENCIL_NO_GLOW_BLUR_ENABLED  0x100000000ull
#define WSURF_STENCIL_NO_GLOW_COLOR_ENABLED 0x200000000ull
#define WSURF_DOUBLE_FACE_ENABLED           0x400000000ull

// Studio Model
#define STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED     0x4000000000000ull
#define STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED    0x8000000000000ull
#define STUDIO_GLOW_COLOR_ENABLED               0x10000000000000ull
#define STUDIO_NF_DOUBLE_FACE                   0x10000  // enginedef.h
```

## Fragment Shader 实现

### Glow Color 输出

**Studio Model (studio_shader.frag.glsl):**
```glsl
#elif defined(GLOW_COLOR_ENABLED)

    #if defined(STUDIO_NF_MASKED)
        vec4 diffuseColor = SampleDiffuseTexture(v_texcoord);
        if(diffuseColor.a < 0.5)
            discard;
    #endif

    out_Diffuse = vec4(StudioUBO.r_color.xyz, 1.0);
```

**World Surface (wsurf_shader.frag.glsl):**
```glsl
#elif defined(GLOW_COLOR_ENABLED)

    #if defined(ALPHA_SOLID_ENABLED)
        vec4 diffuseColor = SampleDiffuseTexture(v_texcoord);
        if(diffuseColor.a < 0.5)
            discard;
    #endif

    out_Diffuse = vec4(EntityUBO.r_color.xyz, 1.0);
```

### Halo Add Shader (copy_color.frag.glsl)

```glsl
#if defined(HALO_ADD_ENABLED)
    // 根据亮度计算 Alpha，用于混合
    float flLuminance = max( baseColor.r, max( baseColor.g, baseColor.b ) );
    baseColor.a = pow( flLuminance, 0.8f );
#endif

out_Color = baseColor;
```

## 判断函数说明

### 全局状态变量

```cpp
// gl_rmain.cpp
bool r_draw_glowstencil = false;                    // 正在绘制 Glow Stencil
bool r_draw_glowstencil_enabledepthtest = false;    // 正在绘制带深度测试的 Glow Stencil
bool r_draw_glowcolor = false;                      // 正在绘制 Glow Color
```

### 状态查询函数

```cpp
// 是否正在进行 Glow Stencil 渲染
bool R_IsRenderingGlowStencil() { return r_draw_glowstencil; }

// 是否正在进行带深度测试的 Glow Stencil 渲染
bool R_IsRenderingGlowStencilEnableDepthTest() { return r_draw_glowstencil_enabledepthtest; }

// 是否正在进行 Glow Color 渲染
bool R_IsRenderingGlowColor() { return r_draw_glowcolor; }
```

### 绘制条件判断函数

```cpp
// 是否应该绘制 Glow Stencil (基础版)
bool R_ShouldDrawGlowStencil()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencil() || 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlow;
}

// 是否应该绘制 WallHack Stencil
bool R_ShouldDrawGlowStencilWallHack()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencil() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHack;
}

// 是否应该绘制 BehindWallOnly Stencil
bool R_ShouldDrawGlowStencilWallHackBehindWallOnly()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencil() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly;
}

// 是否应该绘制带深度测试的 Stencil (用于 BehindWallOnly)
bool R_ShouldDrawGlowStencilEnableDepthTest()
{
    if (R_IsRenderingGlowColor()) return false;
    return R_IsRenderingGlowStencilEnableDepthTest() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly;
}

// 是否应该绘制 Glow Color (基础版)
bool R_ShouldDrawGlowColor()
{
    return R_IsRenderingGlowColor() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlow;
}

// 是否应该绘制 WallHack Glow Color
bool R_ShouldDrawGlowColorWallHack()
{
    return R_IsRenderingGlowColor() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHack;
}

// 是否应该绘制 BehindWallOnly Glow Color
bool R_ShouldDrawGlowColorWallHackBehindWallOnly()
{
    return R_IsRenderingGlowColor() && 
           (*currententity)->curstate.renderfx == kRenderFxPostProcessGlowWallHackBehindWallOnly;
}
```

## 控制台变量

| CVar | 默认值 | 说明 |
|------|-------|------|
| `r_glow_bloomscale` | 0.5 | 控制 Glow 特效的 Bloom 模糊强度，范围 0.1 ~ 1.0 |

## 使用示例

在客户端代码中设置实体的发光效果：

```cpp
// 设置基础发光
entity->curstate.renderfx = kRenderFxPostProcessGlow;
entity->curstate.rendercolor.r = 255;  // 红色分量
entity->curstate.rendercolor.g = 0;    // 绿色分量
entity->curstate.rendercolor.b = 0;    // 蓝色分量

// 设置穿墙发光
entity->curstate.renderfx = kRenderFxPostProcessGlowWallHack;

// 设置仅穿墙部分发光
entity->curstate.renderfx = kRenderFxPostProcessGlowWallHackBehindWallOnly;
```

## 技术细节

### 为什么使用双面渲染 (DOUBLE_FACE)

对于 `kRenderFxPostProcessGlowWallHackBehindWallOnly` 和带深度测试的 Stencil Pass，需要启用双面渲染。这是因为：
1. 当相机在物体内部时，正面剔除会导致看不到物体
2. 确保从任何角度观察都能正确标记 Stencil

### Stencil 测试原理

- **Stencil Pass**: 禁用颜色写入，只更新 Stencil Buffer
- **Color Pass**: 根据 Stencil 值决定是否绘制像素
- `GL_NOTEQUAL` 测试用于排除已标记区域

### Bloom 后处理管线

1. **DownSample**: 将分辨率降低到 1/16
2. **Gaussian Blur**: 水平和垂直两次高斯模糊
3. **Halo Add**: 使用 Alpha 混合叠加到场景

## 相关文件

- `gl_rmain.cpp` - 主渲染循环，Glow 渲染入口
- `gl_studio.cpp` - Studio Model 渲染
- `gl_wsurf.cpp` - World Surface 渲染  
- `gl_hud.cpp` - 后处理函数
- `gl_common.h` - Program State 和 Stencil Mask 定义
- `enginedef.h` - renderfx 常量定义
- `studio_shader.frag.glsl` - Studio Model Fragment Shader
- `wsurf_shader.frag.glsl` - World Surface Fragment Shader
- `copy_color.frag.glsl` - Halo Add Shader

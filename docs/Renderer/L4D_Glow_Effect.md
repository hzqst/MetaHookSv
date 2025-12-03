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

---

## 附录：各特效详细渲染流程

### kRenderFxPostProcessGlow (30) - 基础发光效果

#### 渲染时序

```
场景正常渲染 (收集实体)
    ↓
R_DrawPostProcessGlow (r_draw_glowcolor = true)
    ↓
后处理 (DownSample + Blur)
    ↓
R_CopyColorHaloAdd (合成到场景)
```

#### 实体收集阶段

将实体添加到：
- `g_PostProcessGlowColorEntities` ✓

#### 场景正常渲染阶段 (渲染本体)

本体正常渲染，不需要特殊处理。

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | 正常渲染状态 |
| Stencil Ref | 正常值 (根据 outline/flatshade 等确定) |
| Stencil Mask | `STENCIL_MASK_ALL` (opaque) 或按需设置 (transparent) |
| Depth Test | 启用 |
| Depth Write | 启用 |
| Cull Face | 启用 (GL_FRONT) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | 正常渲染状态 |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL` + 其他标记 |
| Stencil Mask | `STENCIL_MASK_ALL` (opaque) 或按需设置 (transparent) |
| Depth Test | 启用 |
| Depth Write | 启用 |
| Cull Face | 启用 |

#### Stencil 标记阶段

**不需要 Stencil 标记阶段**（`R_ShouldDrawGlowStencil()` 在 `r_draw_glowstencil` 为 false 时返回 true，但该实体不会被加入 `g_PostProcessGlowStencilEntities`）

#### Glow Color 绘制阶段 (r_draw_glowcolor = true)

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | `STUDIO_GLOW_COLOR_ENABLED` |
| Stencil | 无 Stencil 测试 |
| Depth Test | 启用 |
| Depth Write | 禁用 (`glDepthMask(GL_FALSE)`) |
| Blend | 禁用 |
| Cull Face | 启用 (GL_FRONT) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | `WSURF_GLOW_COLOR_ENABLED` |
| Stencil | 无 Stencil 测试 |
| Depth Test | 启用 |
| Depth Write | 禁用 |
| Blend | 禁用 |
| Cull Face | 启用 |

#### 后处理合成阶段 (R_CopyColorHaloAdd)

| 项目 | 值 |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Blend | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` |

**效果说明：** 只要 Stencil 中的 `NO_GLOW_BLUR` 位未被标记，就绘制 Glow Blur。由于 PostProcessGlow 不标记任何 Stencil，所以全屏都会应用 Blur 效果。

---

### kRenderFxPostProcessGlowWallHack (31) - 穿墙发光效果

#### 渲染时序

```
场景正常渲染 (收集实体)
    ↓
R_DrawGlowStencil (r_draw_glowstencil = true, 禁用深度测试)
    ↓
R_DrawPostProcessGlow (r_draw_glowcolor = true)
    ↓
后处理 (DownSample + Blur)
    ↓
R_CopyColorHaloAdd (合成到场景, 排除已标记区域)
```

#### 实体收集阶段

将实体添加到：
- `g_PostProcessGlowStencilEntities` ✓
- `g_PostProcessGlowColorEntities` ✓

#### 场景正常渲染阶段 (渲染本体)

本体正常渲染，不需要特殊处理。（同 PostProcessGlow）

#### Stencil 标记阶段 (r_draw_glowstencil = true)

**全局状态：**
- `glColorMask(0, 0, 0, 0)` - 禁止颜色写入

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | `STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED \| STUDIO_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **禁用** (`glDisable(GL_DEPTH_TEST)`) |
| Depth Write | 保持 |
| Blend | 禁用 |
| Cull Face | 启用 (GL_FRONT) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | `WSURF_STENCIL_NO_GLOW_BLUR_ENABLED \| WSURF_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL \| STENCIL_MASK_NO_GLOW_BLUR` |
| Stencil Mask | 按需设置 |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **禁用** |
| Depth Write | 保持 |
| Blend | 禁用 |
| Cull Face | 启用 |

**效果说明：** 禁用深度测试意味着实体的所有像素（无论是否被遮挡）都会标记 `NO_GLOW_BLUR`。

#### Glow Color 绘制阶段 (r_draw_glowcolor = true)

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | `STUDIO_GLOW_COLOR_ENABLED` |
| Stencil | 无 Stencil 测试 |
| Depth Test | **禁用** (`glDisable(GL_DEPTH_TEST)`) |
| Depth Write | 保持 |
| Blend | 禁用 |
| Cull Face | 启用 (GL_FRONT) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | `WSURF_GLOW_COLOR_ENABLED` |
| Stencil | 无 Stencil 测试 |
| Depth Test | **禁用** |
| Depth Write | 保持 |
| Blend | 禁用 |
| Cull Face | 启用 |

**效果说明：** 禁用深度测试，绘制完整的实体颜色（无论是否被遮挡）。

#### 后处理合成阶段 (R_CopyColorHaloAdd)

| 项目 | 值 |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Blend | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` |

**效果说明：** 由于实体所有像素都已标记 `NO_GLOW_BLUR`，Stencil 测试 `GL_NOTEQUAL` 会排除这些区域。这意味着 **Blur 不会叠加在实体本身上**，但会在实体周围形成发光轮廓（因为 Blur 扩散到了未标记的区域）。

---

### kRenderFxPostProcessGlowWallHackBehindWallOnly (32) - 仅穿墙部分发光效果

#### 渲染时序

```
场景正常渲染 (收集实体)
    ↓
R_DrawGlowStencil Pass 1 (r_draw_glowstencil = true, 禁用深度测试)
    ↓
R_DrawGlowStencil Pass 2 (r_draw_glowstencil_enabledepthtest = true, 启用深度测试)
    ↓
R_DrawPostProcessGlow (r_draw_glowcolor = true, 使用 Stencil 测试)
    ↓
后处理 (DownSample + Blur)
    ↓
R_CopyColorHaloAdd (合成到场景, 排除已标记区域)
```

#### 实体收集阶段

将实体添加到：
- `g_PostProcessGlowStencilEntities` ✓
- `g_PostProcessGlowEnableDepthTestStencilEntities` ✓
- `g_PostProcessGlowColorEntities` ✓

#### 场景正常渲染阶段 (渲染本体)

本体正常渲染，不需要特殊处理。（同 PostProcessGlow）

#### Stencil 标记阶段 Pass 1 (r_draw_glowstencil = true)

**全局状态：**
- `glColorMask(0, 0, 0, 0)` - 禁止颜色写入

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED \| STUDIO_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR \| STENCIL_MASK_NO_GLOW_COLOR` (0x48) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR \| STENCIL_MASK_NO_GLOW_COLOR` (0x48) |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **禁用** (`glDisable(GL_DEPTH_TEST)`) |
| Depth Write | 保持 |
| Blend | 禁用 |
| Cull Face | 启用 (GL_FRONT) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | `WSURF_STENCIL_NO_GLOW_COLOR_ENABLED \| WSURF_STENCIL_NO_GLOW_BLUR_ENABLED \| WSURF_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL \| STENCIL_MASK_NO_GLOW_BLUR \| STENCIL_MASK_NO_GLOW_COLOR` |
| Stencil Mask | 按需设置 |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **禁用** |
| Depth Write | 保持 |
| Blend | 禁用 |
| Cull Face | 启用 |

**效果说明：** 禁用深度测试，标记实体的 **所有像素**（包括被遮挡和可见部分）为 `NO_GLOW_BLUR | NO_GLOW_COLOR`。

#### Stencil 标记阶段 Pass 2 (r_draw_glowstencil_enabledepthtest = true)

**全局状态：**
- `glColorMask(0, 0, 0, 0)` - 禁止颜色写入

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE \| STUDIO_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **启用** |
| Depth Write | **禁用** (`glDepthMask(GL_FALSE)`) |
| Blend | 禁用 |
| Cull Face | **禁用** (双面渲染) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | `WSURF_STENCIL_NO_GLOW_COLOR_ENABLED \| WSURF_DOUBLE_FACE_ENABLED \| WSURF_SHADOW_CASTER_ENABLED` |
| Stencil Func | `GL_ALWAYS` |
| Stencil Ref | `STENCIL_MASK_HAS_DECAL \| STENCIL_MASK_NO_GLOW_COLOR` |
| Stencil Mask | 按需设置 |
| Stencil Op | `GL_KEEP, GL_KEEP, GL_REPLACE` |
| Depth Test | **启用** |
| Depth Write | **禁用** |
| Blend | 禁用 |
| Cull Face | **禁用** (双面渲染) |

**效果说明：** 启用深度测试，只有 **可见像素** 才会通过深度测试并标记 `NO_GLOW_COLOR`。本次绘制Stencil时使用的r_scale与后面绘制GlowColor时使用的r_scale一致，所以“包含在GlowColor区域内”且未被墙体遮挡区域会被标记上`NO_GLOW_COLOR`，因此在后面GlowColor阶段这部分区域不会被绘制。

#### Glow Color 绘制阶段 (r_draw_glowcolor = true)

**Studio Model:**
| 项目 | 值 |
|------|-----|
| ProgramState | `STUDIO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE` |
| Stencil Func | **`GL_NOTEQUAL`** |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Depth Test | 启用 |
| Depth Func | **`GL_GEQUAL`** (反向深度测试) |
| Depth Write | **禁用** (`glDepthMask(GL_FALSE)`) |
| Blend | 禁用 |
| Cull Face | **禁用** (双面渲染) |

**World Surface:**
| 项目 | 值 |
|------|-----|
| ProgramState | `WSURF_GLOW_COLOR_ENABLED \| WSURF_DOUBLE_FACE_ENABLED` |
| Stencil Func | **`GL_NOTEQUAL`** |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_COLOR` (0x40) |
| Depth Test | 启用 |
| Depth Func | **`GL_GEQUAL`** |
| Depth Write | **禁用** |
| Blend | 禁用 |
| Cull Face | **禁用** (双面渲染) |

**效果说明：** 
1. **Stencil 测试 `GL_NOTEQUAL`**：只有 `NO_GLOW_COLOR` 位 **未被标记** 的像素才通过测试。但由于 Pass 1 和 Pass 2 都标记了所有/可见像素，这里需要结合 Depth 来理解。
2. **Depth Func `GL_GEQUAL`**：只有深度 **大于等于** 当前深度缓冲值的像素才通过深度测试，即被其他物体遮挡的部分。

**实际效果：** 只绘制被其他物体遮挡的实体像素（穿墙部分）。

#### 后处理合成阶段 (R_CopyColorHaloAdd)

| 项目 | 值 |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Blend | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` |

**效果说明：** 由于 Pass 1 标记了所有像素的 `NO_GLOW_BLUR` 位，Blur 效果不会叠加在实体本身上，但会在被遮挡部分的边缘形成发光轮廓。

---

### 总结对比表

#### 实体收集

| 特效 | GlowColorEntities | GlowStencilEntities | GlowEnableDepthTestStencilEntities |
|------|-------------------|---------------------|-----------------------------------|
| PostProcessGlow | ✓ | - | - |
| PostProcessGlowWallHack | ✓ | ✓ | - |
| PostProcessGlowWallHackBehindWallOnly | ✓ | ✓ | ✓ |

#### Stencil Pass 配置

| 特效 | Pass | Depth Test | Stencil Ref | ProgramState (Studio) |
|------|------|------------|-------------|----------------------|
| PostProcessGlow | - | - | - | - |
| PostProcessGlowWallHack | 1 | 禁用 | `NO_GLOW_BLUR` | `STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED` |
| PostProcessGlowWallHackBehindWallOnly | 1 | 禁用 | `NO_GLOW_BLUR \| NO_GLOW_COLOR` | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED` |
| PostProcessGlowWallHackBehindWallOnly | 2 | 启用 | `NO_GLOW_COLOR` | `STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE` |

#### Glow Color 绘制配置

| 特效 | Depth Test | Depth Func | Stencil Test | ProgramState (Studio) |
|------|------------|------------|--------------|----------------------|
| PostProcessGlow | 启用 | `GL_LESS` | 无 | `STUDIO_GLOW_COLOR_ENABLED` |
| PostProcessGlowWallHack | 禁用 | - | 无 | `STUDIO_GLOW_COLOR_ENABLED` |
| PostProcessGlowWallHackBehindWallOnly | 启用 | `GL_GEQUAL` | `GL_NOTEQUAL, NO_GLOW_COLOR` | `STUDIO_GLOW_COLOR_ENABLED \| STUDIO_NF_DOUBLE_FACE` |

#### Halo Add 阶段 (统一)

| 项目 | 值 |
|------|-----|
| Stencil Func | `GL_NOTEQUAL` |
| Stencil Ref | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| Stencil Mask | `STENCIL_MASK_NO_GLOW_BLUR` (0x8) |
| 效果 | 排除标记了 `NO_GLOW_BLUR` 的区域 |

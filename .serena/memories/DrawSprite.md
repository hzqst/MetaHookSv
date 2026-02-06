# Renderer Plugin - Sprite绘制流程详解

## 概述

Sprite是GoldSrc引擎中的2D精灵对象，用于渲染粒子效果、UI元素、特效等。Renderer插件实现了现代化的Sprite渲染系统，支持帧插值、多种混合模式和高级特效。

---

## Sprite绘制调用链

### 完整调用流程

```
R_RenderScene()
└── R_DrawEntitiesOnList()              // 不透明实体列表
    └── R_DrawCurrentEntity(false)
        └── R_DrawSpriteEntity(false)
            └── R_DrawSpriteModel()
                └── R_DrawSpriteModelInterpFrames()

R_RenderScene()
└── R_DrawTransEntities()               // 透明实体列表
    └── R_DrawTEntitiesOnList()
        └── R_DrawCurrentEntity(true)
            └── R_DrawSpriteEntity(true)
                └── R_DrawSpriteModel()
                    └── R_DrawSpriteModelInterpFrames()
```

---

## 详细函数分析

### 1. R_DrawCurrentEntity() - 实体分发器
**位置**: `gl_rmain.cpp:2358-2401`

**功能**: 根据模型类型分发到不同的渲染函数

```cpp
void R_DrawCurrentEntity(bool bTransparent) {
    // 检查是否应该绘制
    if (R_IsHidingEntity((*currententity)))
        return;
    
    // 透明物体计算混合值
    if (bTransparent) {
        (*r_blend) = CL_FxBlend((*currententity)) / 255.0;
    }
    
    // 根据模型类型分发
    switch ((*currententity)->model->type) {
        case mod_sprite:
            R_DrawSpriteEntity(bTransparent);
            break;
        case mod_brush:
            R_DrawBrushEntity(bTransparent);
            break;
        case mod_studio:
            R_DrawStudioEntity(bTransparent);
            break;
    }
}
```

---

### 2. R_DrawSpriteEntity() - Sprite实体准备
**位置**: `gl_rmain.cpp:2198-2224`

**功能**: 准备Sprite渲染所需的位置和混合参数

```cpp
void R_DrawSpriteEntity(bool bTransparent) {
    // 确定Sprite位置
    if ((*currententity)->curstate.body) {
        // 使用附着点位置
        float* pAttachment = R_GetAttachmentPoint(...);
        VectorCopy(pAttachment, r_entorigin);
    } else {
        // 使用实体原点
        VectorCopy((*currententity)->origin, r_entorigin);
    }
    
    // 处理Glow渲染模式的特殊混合
    if (bTransparent && rendermode == kRenderGlow) {
        (*r_blend) *= R_GlowBlend((*currententity));
    }
    
    // 调用实际绘制
    if ((*r_blend) > 0) {
        R_DrawSpriteModel((*currententity));
    }
}
```

**关键点**:
- 支持附着点定位 (用于附着到其他实体)
- Glow模式的距离衰减计算
- 混合值过滤 (blend <= 0 不绘制)

---

### 3. R_DrawSpriteModel() - Sprite模型绘制入口
**位置**: `gl_sprite.cpp:769-802`

**功能**: 获取Sprite帧并准备插值数据

```cpp
void R_DrawSpriteModel(cl_entity_t *ent) {
    // 获取Sprite数据
    auto pSprite = (msprite_t *)ent->model->cache.data;
    auto pSpriteRenderData = R_GetSpriteRenderDataFromModel(ent->model);
    
    // 帧插值处理
    float lerp = 0;
    mspriteframe_t* frame = nullptr;
    mspriteframe_t* oldframe = nullptr;
    
    if (R_SpriteAllowLerping(ent, pSprite)) {
        // 启用帧插值
        R_GetSpriteFrameInterpolant(ent, pSprite, &frame, &oldframe, &lerp);
    } else {
        // 不插值，使用当前帧
        int frameIndex = (int)ent->curstate.frame;
        oldframe = frame = R_GetSpriteFrame(pSprite, frameIndex);
    }
    
    // 调用实际渲染
    R_DrawSpriteModelInterpFrames(ent, pSpriteRenderData.get(), 
                                   pSprite, frame, oldframe, lerp);
}
```

**关键功能**:
- **帧插值** - 平滑的动画过渡
- **帧选择** - 根据实体状态选择正确的帧
- **渲染数据缓存** - 避免重复加载

---

### 4. R_DrawSpriteModelInterpFrames() - 核心渲染函数
**位置**: `gl_sprite.cpp:464-767`

这是Sprite渲染的核心函数，包含完整的渲染管线设置。

#### 4.1 渲染模式设置

```cpp
void R_DrawSpriteModelInterpFrames(...) {
    program_state_t SpriteProgramState = 0;
    
    // 计算颜色和混合
    colorVec color = { 0 };
    R_SpriteColor(&color, ent, (*r_blend) * 255);
    
    // 根据渲染模式设置OpenGL状态
    switch (ent->curstate.rendermode) {
        case kRenderNormal:
            // 不透明渲染
            glDisable(GL_BLEND);
            break;
            
        case kRenderTransColor:
        case kRenderTransAlpha:
            // Alpha混合
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            SpriteProgramState |= SPRITE_ALPHA_BLEND_ENABLED;
            break;
            
        case kRenderTransAdd:
            // 加法混合
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;
            break;
            
        case kRenderGlow:
            // 发光效果 (无深度测试)
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            SpriteProgramState |= SPRITE_ADDITIVE_BLEND_ENABLED;
            break;
    }
}
```

#### 4.2 Sprite类型处理

Sprite支持5种朝向类型:

```cpp
int type = pSprite->type;

// SvEngine支持自定义朝向
if (g_iEngineType == ENGINE_SVENGINE) {
    if (ent->curstate.effects & EF_SPRITE_CUSTOM_VP) {
        type = ent->curstate.sequence;
    }
}

// 有旋转角度时强制使用ORIENTED模式
if (ent->angles[2] != 0 && type == SPR_VP_PARALLEL) {
    type = SPR_VP_PARALLEL_ORIENTED;
}

switch (type) {
    case SPR_VP_PARALLEL:
        // 平行于视平面，始终面向相机
        SpriteProgramState |= SPRITE_PARALLEL_ENABLED;
        break;
        
    case SPR_VP_PARALLEL_UPRIGHT:
        // 平行于视平面，但保持垂直
        SpriteProgramState |= SPRITE_PARALLEL_UPRIGHT_ENABLED;
        break;
        
    case SPR_FACING_UPRIGHT:
        // 面向相机，但保持垂直
        SpriteProgramState |= SPRITE_FACING_UPRIGHT_ENABLED;
        break;
        
    case SPR_ORIENTED:
        // 固定朝向，不面向相机
        SpriteProgramState |= SPRITE_ORIENTED_ENABLED;
        break;
        
    case SPR_VP_PARALLEL_ORIENTED:
        // 平行于视平面，支持旋转
        SpriteProgramState |= SPRITE_PARALLEL_ORIENTED_ENABLED;
        break;
}
```

**Sprite类型说明**:
- **SPR_VP_PARALLEL** - 广告牌模式，始终面向相机
- **SPR_VP_PARALLEL_UPRIGHT** - 垂直广告牌，Y轴保持向上
- **SPR_FACING_UPRIGHT** - 面向相机但保持垂直
- **SPR_ORIENTED** - 固定朝向，用于贴花等
- **SPR_VP_PARALLEL_ORIENTED** - 支持旋转的广告牌

#### 4.3 特效标志设置

```cpp
// Alpha测试 (透明度裁剪)
SpriteProgramState |= SPRITE_ALPHA_TEST_ENABLED;

// 水面裁剪
if (R_IsRenderingWaterView()) {
    SpriteProgramState |= SPRITE_CLIP_ENABLED;
}

// G-Buffer渲染
if (R_IsRenderingGBuffer()) {
    SpriteProgramState |= SPRITE_GBUFFER_ENABLED;
}

// 雾效
if (R_IsRenderingFog()) {
    if (r_fog_mode == GL_LINEAR)
        SpriteProgramState |= SPRITE_LINEAR_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP)
        SpriteProgramState |= SPRITE_EXP_FOG_ENABLED;
    else if (r_fog_mode == GL_EXP2)
        SpriteProgramState |= SPRITE_EXP2_FOG_ENABLED;
}

// Gamma混合
if (R_IsRenderingGammaBlending()) {
    SpriteProgramState |= SPRITE_GAMMA_BLEND_ENABLED;
}

// OIT混合 (顺序无关透明度)
if (r_draw_oitblend) {
    SpriteProgramState |= SPRITE_OIT_BLEND_ENABLED;
}

// 帧插值
if (frame != oldframe) {
    SpriteProgramState |= SPRITE_LERP_ENABLED;
}
```

#### 4.4 着色器和绘制

```cpp
// 选择着色器程序
sprite_program_t prog = { 0 };
R_UseSpriteProgram(SpriteProgramState, &prog);

// 设置Uniform变量
if (prog.in_up_down_left_right != -1)
    glUniform4f(prog.in_up_down_left_right, 
                frame->up, frame->down, frame->left, frame->right);

if (prog.in_color != -1)
    glUniform4f(prog.in_color, u_color[0], u_color[1], u_color[2], u_color[3]);

if (prog.in_origin != -1)
    glUniform3f(prog.in_origin, r_entorigin[0], r_entorigin[1], r_entorigin[2]);

if (prog.in_angles != -1)
    glUniform3f(prog.in_angles, ent->angles[0], ent->angles[1], ent->angles[2]);

if (prog.in_scale != -1)
    glUniform1f(prog.in_scale, scale);

if (prog.in_lerp != -1)
    glUniform1f(prog.in_lerp, lerp);

// 绑定纹理
GL_BindTextureUnit(0, GL_TEXTURE_2D, frame->gl_texturenum);

if (SpriteProgramState & SPRITE_LERP_ENABLED) {
    GL_BindTextureUnit(1, GL_TEXTURE_2D, oldframe->gl_texturenum);
}

// 绘制四边形 (2个三角形)
const uint32_t indices[] = {0, 1, 2, 2, 3, 0};
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
```

---

## 数据结构

### CSpriteModelRenderData - Sprite渲染数据
```cpp
class CSpriteModelRenderData {
public:
    int flags;                          // 特效标志 (FMODEL_NOBLOOM等)
    model_t* model;                     // 关联的模型
    std::vector<std::shared_ptr<CSpriteModelRenderMaterial>> vSpriteMaterials;
};
```

### CSpriteModelRenderMaterial - Sprite材质
```cpp
class CSpriteModelRenderMaterial {
public:
    std::string basetexture;            // 基础纹理名
    CGameModelRenderTexture textures[SPRITE_MAX_TEXTURE];
    mspriteframe_t replaceframe;        // 替换帧数据
};
```

### sprite_program_t - Sprite着色器程序
```cpp
typedef struct sprite_program_s {
    int program;                        // 着色器程序ID
    int in_up_down_left_right;         // 纹理坐标Uniform
    int in_color;                       // 颜色Uniform
    int in_origin;                      // 位置Uniform
    int in_angles;                      // 角度Uniform
    int in_scale;                       // 缩放Uniform
    int in_lerp;                        // 插值系数Uniform
} sprite_program_t;
```

---

## 渲染模式详解

### kRenderNormal (0) - 不透明
- 禁用混合
- 写入深度缓冲
- 标准光照

### kRenderTransColor (1) - 颜色透明
- Alpha混合
- 不写入深度
- 使用rendercolor作为颜色

### kRenderTransAlpha (2) - Alpha透明
- Alpha混合
- 不写入深度
- 使用renderamt作为透明度

### kRenderTransAdd (4) - 加法混合
- 加法混合 (GL_ONE, GL_ONE)
- 不写入深度
- 用于发光效果

### kRenderGlow (3) - 发光
- 加法混合
- 禁用深度测试
- 距离衰减
- 用于光晕效果

---

## 程序状态标志

### 混合模式
- `SPRITE_ALPHA_BLEND_ENABLED` - Alpha混合
- `SPRITE_ADDITIVE_BLEND_ENABLED` - 加法混合
- `SPRITE_GAMMA_BLEND_ENABLED` - Gamma空间混合

### 特效
- `SPRITE_ALPHA_TEST_ENABLED` - Alpha测试
- `SPRITE_LERP_ENABLED` - 帧插值
- `SPRITE_CLIP_ENABLED` - 水面裁剪
- `SPRITE_OIT_BLEND_ENABLED` - 顺序无关透明度

### 雾效
- `SPRITE_LINEAR_FOG_ENABLED` - 线性雾
- `SPRITE_EXP_FOG_ENABLED` - 指数雾
- `SPRITE_EXP2_FOG_ENABLED` - 指数平方雾
- `SPRITE_LINEAR_FOG_SHIFT_ENABLED` - 雾效偏移

### 朝向类型
- `SPRITE_PARALLEL_ENABLED` - 平行广告牌
- `SPRITE_PARALLEL_UPRIGHT_ENABLED` - 垂直平行广告牌
- `SPRITE_FACING_UPRIGHT_ENABLED` - 面向垂直
- `SPRITE_ORIENTED_ENABLED` - 固定朝向
- `SPRITE_PARALLEL_ORIENTED_ENABLED` - 可旋转广告牌

### 渲染目标
- `SPRITE_GBUFFER_ENABLED` - 写入G-Buffer

---

## 帧插值系统

### R_SpriteAllowLerping() - 判断是否允许插值
```cpp
bool R_SpriteAllowLerping(cl_entity_t* ent, msprite_t* pSprite) {
    // 检查CVar设置
    if (!r_sprite_lerping->value)
        return false;
    
    // 检查渲染模式
    if (ent->curstate.rendermode != kRenderNormal &&
        ent->curstate.rendermode != kRenderTransAdd)
        return false;
    
    return true;
}
```

### R_GetSpriteFrameInterpolant() - 获取插值帧
```cpp
void R_GetSpriteFrameInterpolant(cl_entity_t* ent, msprite_t* pSprite,
                                  mspriteframe_t** frame,
                                  mspriteframe_t** oldframe,
                                  float* lerp) {
    // 计算当前帧和上一帧
    int currentFrame = (int)ent->curstate.frame;
    int lastFrame = (int)ent->latched.prevframe;
    
    // 计算插值系数
    *lerp = ent->curstate.framerate * (*cl_time - ent->latched.prevanimtime);
    *lerp = math_clamp(*lerp, 0.0f, 1.0f);
    
    *frame = R_GetSpriteFrame(pSprite, currentFrame);
    *oldframe = R_GetSpriteFrame(pSprite, lastFrame);
}
```

**插值效果**:
- 平滑的动画过渡
- 消除帧跳跃
- 提升视觉质量

---

## 外部文件支持

Sprite支持通过`_external.txt`文件自定义属性:

### sprite_efx - 特效标志
```
{
    "classname" "sprite_efx"
    "flags" "FMODEL_NOBLOOM"
}
```

### sprite_frame_texture - 帧纹理替换
```
{
    "classname" "sprite_frame_texture"
    "frame" "0"
    "replacetexture" "sprites/custom.png"
}
```

---

## 性能优化

### 1. 渲染数据缓存
- `g_SpriteRenderDataCache` - 缓存Sprite渲染数据
- 避免重复解析外部文件
- 减少内存分配

### 2. 着色器程序缓存
- `g_SpriteProgramTable` - 缓存编译的着色器
- 根据程序状态快速查找
- 避免重复编译

### 3. 批量绘制
- 使用索引绘制 (glDrawElements)
- 减少状态切换
- 优化GPU利用率

### 4. 早期剔除
- 混合值检查 (blend <= 0)
- 视锥体剔除
- 距离剔除 (Glow模式)

---

## 着色器系统

### Sprite着色器文件
- `sprite_shader.vert.glsl` - 顶点着色器
- `sprite_shader.frag.glsl` - 片段着色器

### 着色器变体
根据`SpriteProgramState`标志组合生成不同的着色器变体:
- 基础变体: 2^5 = 32种朝向和混合组合
- 特效变体: 雾效、插值、裁剪等
- 总计: 数百种着色器变体

### 动态编译
- 首次使用时编译
- 缓存编译结果
- 支持热重载

---

## 调试和诊断

### OpenGL调试组
```cpp
GL_BeginDebugGroupFormat("R_DrawSpriteModelInterpFrames - %s", 
                         ent->model->name);
// ... 渲染代码 ...
GL_EndDebugGroup();
```

### 控制台变量
- `r_sprite_lerping` - 启用/禁用帧插值
- `gl_spriteblend` - Sprite混合模式
- `r_drawentities` - 启用/禁用实体渲染

---

## 总结

Sprite渲染系统的特点:

1. **灵活的渲染模式** - 支持5种混合模式
2. **多种朝向类型** - 5种广告牌模式
3. **帧插值** - 平滑的动画过渡
4. **高级特效** - 雾效、OIT、G-Buffer支持
5. **外部文件** - 可自定义纹理和属性
6. **性能优化** - 缓存、批量绘制、早期剔除
7. **着色器系统** - 动态编译、多变体支持

Sprite系统是粒子效果、UI元素、特效的基础，通过现代化的渲染管线提供了高质量的视觉效果。

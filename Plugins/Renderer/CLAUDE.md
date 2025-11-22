# Renderer Plugin - MetaHookSv

## 项目概述

Renderer 插件是 MetaHookSv 的核心图形增强插件，为 GoldSrc 引擎游戏提供现代化的图形渲染功能。该插件实现了 OpenGL 渲染管线，支持多种高级图形特效。

## 核心功能

### 渲染特性
- **现代 OpenGL 渲染管线** - 基于 OpenGL 的完整渲染系统
- **延迟光照 (Deferred Lighting)** - 高效的多光源渲染
- **环境光遮蔽 (HBAO)** - Screen-space ambient occlusion
- **高动态范围 (HDR)** - HDR 渲染和色调映射
- **实时阴影** - 动态阴影投射和接收
- **水面渲染** - 高级水面特效和折射
- **Portal 渲染** - 传送门效果
- **后期处理** - FXAA 抗锯齿、Gamma 校正等

### 模型渲染
- **Studio Model 渲染** - .mdl格式模型的渲染
- **Sprite 渲染** - .spr格式的模型的渲染
- **World Surface 渲染** - .bsp格式的模型的渲染

## 项目结构

### 核心入口文件
- `exportfuncs.cpp/h` - 插件导出函数，定义 HUD 接口入口点
- `plugins.cpp` - 插件主逻辑和初始化
- `gl_hooks.cpp` - OpenGL 函数钩子和渲染管线入口

### 渲染子系统

#### 主要渲染模块
- `gl_rmain.cpp` - 主渲染循环和场景管理
- `gl_rmisc.cpp` `gl_draw.cpp` - OpenGL相关API调用封装
- `gl_studio.cpp` - Studio 模型渲染 (角色/武器)
- `gl_sprite.cpp` - Sprite 渲染
- `gl_entity.cpp` - 实体相关渲染专用数据结构管理
- `gl_water.cpp` - 水面渲染
- `gl_rsurf.cpp` `gl_wsurf.cpp` - BSP地形渲染，wsurf for WorldSurface.
- `gl_light.cpp` - 动态光照系统
- `gl_shadow.cpp` - 阴影投射和接收
- `gl_portal.cpp` - 传送门渲染
- `gl_shader.cpp` - 着色器程序管理
- `gl_ringbuffer.cpp` - 环形缓冲区
- `gl_hud.cpp` - HUD 元素渲染
- `BaseUI.cpp` - 基础 UI 组件
- `EngineSurfaceHook.cpp` - 引擎表面钩子

#### 工具和实用程序
- `gl_model.cpp` - 模型加载和处理
- `gl_cvar.cpp` - 渲染相关控制台变量
- `VideoMode.cpp` - 视频模式管理
- `mathlib2.cpp` - 数学库扩展
- `util.cpp` - 通用工具函数
- `zone.cpp` - 内存管理

#### 平台和游戏特定
- `CounterStrike.cpp/h` - Counter-Strike 特定渲染适配
- `VGUI2ExtensionImport.cpp/h` - VGUI2 扩展接口
- `GameUI.cpp` - 游戏 UI 适配

#### 线程和任务管理
- `LambdaThreadedTask.cpp/h` - Lambda 任务系统
- `UtilThreadTask.cpp/h` - 线程任务工具

#### 哈希和消息
- `MurmurHash2.cpp/h` - MurmurHash2 实现
- `parsemsg.cpp` - 消息解析

### 头文件 (Headers)

#### 核心头文件
- `gl_local.h` - 内部状态和全局变量定义 **(最重要)**
- `gl_common.h` - 通用渲染定义和宏
- `exportfuncs.h` - 导出函数接口定义
- `privatehook.h` - 私有钩子定义

#### 子系统头文件
- `gl_shader.h` - 着色器系统
- `gl_model.h` - 模型处理
- `gl_water.h` - 水面渲染
- `gl_sprite.h` - Sprite 渲染
- `gl_studio.h` - Studio 模型渲染
- `gl_hud.h` - HUD 系统
- `gl_shadow.h` - 阴影系统
- `gl_light.h` - 光照系统
- `gl_wsurf.h` - 可变形表面
- `gl_portal.h` - 传送门系统
- `gl_entity.h` - 实体系统
- `gl_ringbuffer.h` - 环形缓冲区
- `gl_draw.h` - 2D 绘制
- `gl_cvar.h` - 控制台变量
- `qgl.h` - OpenGL 包装函数,本质只是对glew的引用

#### 工具头文件
- `mathlib2.h` - 数学库扩展
- `plugins.h` - 插件接口
- `zone.h` - 内存管理
- `util.h` - 通用工具
- `enginedef.h` - 引擎定义
- `bspfile.h` - BSP 文件格式
- `modelgen.h` - 模型生成
- `spritegn.h` - Sprite 定义

### 着色器资源

着色器文件位于 `Build\svencoop\renderer\shader\` 目录：

#### 后处理着色器
- `pp_fxaa.frag.glsl` - FXAA 抗锯齿
- `hdr_brightpass.frag.glsl` - HDR 亮部提取
- `hdr_lumpass.frag.glsl` - HDR 光晕
- `hdr_tonemap.frag.glsl` - HDR 色调映射
- `gamma_correction.frag.glsl` - Gamma 校正
- `gaussian_blur_16x.frag.glsl` - 高斯模糊
- `down_sample.frag.glsl` - 降采样

#### 几何着色器
- `studio_shader.geom.glsl` - Studio 模型几何着色器
- `wsurf_shader.geom.glsl` - WorldSurface 几何着色器

#### 延迟渲染
- `dlight_shader.vert.glsl/.frag.glsl` - 延迟光照
- `dfinal_shader.frag.glsl` - 延迟渲染最终合成
- `blit_oitblend.frag.glsl` - OIT 混合

#### 水面和 Portal
- `water_shader.vert.glsl/.frag.glsl` - 水面渲染
- `portal_shader.vert.glsl/.frag.glsl` - 传送门渲染

#### 通用着色器
- `fullscreenquad.vert.glsl` - 全屏四边形
- `fullscreentriangle.vert.glsl` - 全屏三角形
- `pp_common.vert.glsl` - 通用后处理顶点着色器

#### HUD 和调试
- `hud_debug.vert.glsl/.frag.glsl` - HUD 调试着色器
- `drawfilledrect_shader.vert.glsl/.frag.glsl` - 填充矩形
- `drawtexturedrect_shader.vert.glsl/.frag.glsl` - 纹理矩形

### 第三方依赖

#### 静态库
- **GLEW** - OpenGL 扩展加载库
- **FreeImage** - 图像格式支持
- **Capstone** - 反汇编引擎
- **SDL2/SDL3** - 跨平台多媒体库
- **tinyobjloader** - OBJ 模型加载器

#### Source SDK 组件
- 完整的 tier0, tier1, vstdlib 系统
- 数学库 (mathlib)
- 文件系统接口
- 内存管理系统

## 构建配置

### 配置类型
- **Debug** - 调试版本，包含完整调试信息
- **Release** - 优化版本，标准优化
- **Release_AVX2** - AVX2 优化版本高性能渲染

### 关键编译设置
- **C++ 标准**: C++20
- **运行时库**: 多线程 (Release) / 多线程调试 (Debug)
- **OpenGL**: 使用 GLEW 静态链接
- **并行编译**: Release 模式启用 `/MP` 多核编译

### 输出路径
- **Debug**: `output\Win32\Debug\renderer.dll`
- **Release**: `output\Win32\Release\renderer.dll`
- **Release_AVX2**: `output\Win32\Release_AVX2\renderer.dll`

### 部署
构建后自动复制到游戏目录：
- 主插件: `$(GameDir)/metahook/renderer/`
- 依赖 DLL: `$(GameDir)/metahook/dlls/FreeImage/`

## 关键架构

### 渲染管线

1. **主渲染循环** (`gl_rmain.cpp`)
   - 场景管理和相机设置
   - 渲染顺序控制
   - Pass 管理

2. **延迟渲染** (`gl_rsurf.cpp`)
   - G-Buffer 生成
   - 几何信息存储
   - 多渲染目标支持

3. **光照处理** (`gl_light.cpp`)
   - 动态光源收集
   - 光源类型分类
   - 光照计算优化

4. **后期处理** (`gl_shader.cpp`)
   - HDR 管线
   - 抗锯齿
   - 图像效果

### 内存管理
- 使用 Source SDK 的内存管理系统 (tier0)
- 自定义内存池 (zone.cpp)
- 资源生命周期管理

### 线程模型
- 主线程渲染
- 后台线程资源加载 (LambdaThreadedTask)
- 异步着色器编译支持

## 开发指南

### 关键开发文件

#### 需要修改渲染逻辑时
1. 查看 `gl_local.h` 了解全局状态
2. 在相应的子系统文件中添加功能 (如 `gl_light.cpp` 添加新光照效果)
3. 在 `gl_hooks.cpp` 中添加必要的钩子

#### 需要添加新的着色器时
1. 在 `Build\svencoop\renderer\shader/` 目录添加 .glsl 文件
2. 在 `gl_shader.cpp` 中加载和编译着色器
3. 创建对应的渲染函数

#### 需要添加新的 CVars 时
1. 在 `gl_cvar.cpp` 中注册控制台变量
2. 在 `gl_cvar.h` 中添加声明
3. 在相应渲染模块中使用

### 常见开发任务

#### 添加新的渲染 Pass
1. 在 `gl_rmain.cpp` 的主渲染循环中添加 Pass 调用
2. 在相应的 .cpp 文件中实现 Pass 逻辑
3. 在 `gl_shader.cpp` 中添加所需的着色器

#### 修改光照模型
1. 编辑 `gl_light.cpp` 中的光照计算函数
2. 更新对应的着色器文件
3. 测试不同光照参数

#### 优化渲染性能
1. 检查 `gl_ringbuffer.cpp` 中的 GPU 使用情况
2. 优化 `gl_model.cpp` 中的模型处理
3. 使用 `gl_cvar.cpp` 中的性能 CVars 进行调优

## 依赖关系

### 依赖的 MetaHook 插件
- **VGUI2Extension** - VGUI2 接口支持 (可选，如果 没有加载VGUI2Extension则不提供GUI菜单)

### 依赖的 PluginLibs
- 无直接依赖

### 依赖的游戏引擎
- 所有支持的 GoldSrc 引擎变体
- SvEngine (优先支持)
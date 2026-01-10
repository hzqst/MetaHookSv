# BulletPhysics 插件速览（MetaHookSv）

## 位置与职责
- 目录：`Plugins/BulletPhysics/`
- 作用：在客户端侧为 GoldSrc/SvEngine 提供基于 **Bullet3** 的物理对象/布娃娃（ragdoll）模拟与调试 UI；通过 MetaHookSv 的插件接口接入并对引擎/客户端渲染与动画流程做必要 Hook。

## 目录结构（按功能分组）
- 插件入口与 Hook：`plugins.cpp/.h`、`exportfuncs.cpp/.h`、`privatehook.cpp/.h`
- 物理系统抽象层：`ClientPhysicManager.h`、`BasePhysicManager.cpp/.h`（大量配置/对象管理逻辑）
- Bullet3 具体实现：`BulletPhysicManager.cpp/.h`（世界、步进、碰撞形状/约束创建等）
- 物理对象与组件：
  - Object：`BaseStaticObject* / BaseDynamicObject* / BaseRagdollObject*` + `BulletStaticObject* / BulletDynamicObject* / BulletRagdollObject*`
  - RigidBody/Constraint：`BasePhysicRigidBody* / BasePhysicConstraint*` + `BulletPhysicRigidBody* / BulletPhysicConstraint*` 等
  - Behaviors：Barnacle/Gargantua/浮力/相机等（`Bullet*Behavior.*`）
- UI/调试器：`Viewport.*`、`PhysicDebugGUI.*`、大量 `*Page/*Panel/*Dialog`（编辑/检查/调试面板）
- VGUI2Extension 依赖接入：`VGUI2ExtensionImport.*`、`BaseUI.cpp`、`GameUI.cpp`、`ClientVGUI.cpp`
- 其他工具：`ClientEntityManager.*`（实体发射/inspect 状态管理）等

## 插件生命周期与核心入口
- `plugins.cpp`
  - `IPluginsV4::Init(...)`：保存 `g_pMetaHookAPI/g_pInterface/g_pMetaSave`。
  - `IPluginsV4::LoadEngine(cl_enginefunc_t*)`：
    - 初始化文件系统指针（`g_pInterface->FileSystem` / HL25 兼容字段）。
    - 记录引擎类型/BuildNum/模块区段信息（engine + mirror engine）。
    - 注册 DLL 加载通知回调 `DllLoadNotification`（用于 `vgui2.dll` 发现后初始化 KeyValuesSystem）。
    - 拷贝 `gEngfuncs`，做 `Engine_FillAddress(...)` + `Engine_InstallHook()`。
    - 初始化 VGUI2Extension（`VGUI2Extension_Init()`），并注册 BaseUI/GameUI/ClientVGUI 回调。
    - 创建全局物理管理器：`g_pClientPhysicManager = BulletPhysicManager_CreateInstance();`
    - 调用 `glewInit()`（调试绘制用 OpenGL/GLEW）。
  - `IPluginsV4::LoadClient(cl_exportfuncs_t*)`：
    - 保存原始 `gExportfuncs`，并替换 `HUD_Init/HUD_*`、`V_CalcRefdef` 等导出函数。
    - 记录 client dll（含 mirror）区段信息后执行 `Client_FillAddress(...)` 与 `Client_InstallHooks()`。
  - `IPluginsV4::Shutdown()`：注销 DLL 通知回调。
  - `IPluginsV4::ExitGame(int)`：销毁物理管理器、卸载 UI 回调与引擎 Hook，关闭 VGUI2Extension。

## 引擎/客户端 Hook（大方向）
- `privatehook.cpp`
  - `Engine_FillAddress(...)`：分项定位引擎内函数/全局变量（渲染、视图、临时实体、可见实体列表等）。
  - `Engine_InstallHook()`：安装inline hook： `R_NewMap` + `R_RenderView`（SvEngine 用 `R_RenderView_SvEngine` 分支）。
  - `Client_FillAddress(...)`：根据 game directory 识别 `dod/cstrike/czero/...`，设置 `g_bIsDayOfDefeat/g_bIsCounterStrike`，并补充部分地址。

## cl_exportfuncs 覆盖点与运行时行为
- `exportfuncs.cpp`
  - `HUD_Init()`：
    - 调用原始 `gExportfuncs.HUD_Init()`，随后 `ClientPhysicManager()->Init()`。
    - 注册/获取 cvar：`bv_debug_draw*`、`bv_simrate`、`bv_syncview`、`bv_force_updatebones`，以及 `sv_cheats/chase_active/...`。
    - 注册命令：`bv_open_debug_ui`、`bv_reload_all`、`bv_reload_objects`、`bv_reload_configs`、`bv_save_configs`。
    - 安装消息 Hook `ClCorpse`（若存在），并对 `efxapi_R_TempModel` 安装 inline hook。
    - 初始化 `g_pViewPort`（若存在）。
  - `HUD_GetStudioModelInterface(...)`：
    - 获取 `engine_studio_api_s` 并安装 Studio/Renderer 相关 Hook（`EngineStudio_*`、`ClientStudio_*`）。
    - 读取骨骼矩阵指针 `pbonetransform/plighttransform`，并缓存 `StudioCheckBBox` 等。
  - `HUD_CreateEntities()`：
    - 每帧枚举玩家与 client edicts（含校验 `messagenum/EF_NODRAW/modelindex` 等），标记实体“已发射”，并调用 `ClientPhysicManager()->CreatePhysicObjectForEntity(...)`。
  - `HUD_TempEntUpdate(...)`：
    - 遍历 temp entities，补建物理对象。
    - `ClientPhysicManager()->SetGravity(cl_gravity)`。
    - Sven 第三人称时用 `g_bIsUpdatingRefdef` 包裹强制更新视图（调用 `CAM_Think()` + `V_RenderView()`）。
    - `UpdateAllPhysicObjects(...)` 后 `StepSimulation(frametime)`。
  - `HUD_DrawTransparentTriangles()`：在 `AllowCheats()` 且 `bv_debug_draw` 打开时调用 `ClientPhysicManager()->DebugDraw()`。
  - `R_NewMap()`（被引擎 inline hook）：执行原始 `R_NewMap` 后触发 `ClientPhysicManager()->NewMap()`、`ClientEntityManager()->NewMap()`、`g_pViewPort->NewMap()`。
  - `V_CalcRefdef(ref_params_s*)`：在非暂停/非 intermission/非 portal 渲染等条件下，根据 `bv_syncview` 把相机与物理对象（spectator/本地玩家）同步。

## Bullet3 物理世界与物理模拟步进
- `BulletPhysicManager.cpp`
  - `CBulletPhysicManager::Init()`：创建 Bullet world（collision config/dispatcher/broadphase/solver/world），设置 debug drawer 与 overlap filter callback，初始重力为 0。
  - `CBulletPhysicManager::StepSimulation(double frametime)`：`stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate())`。
  - `CBulletPhysicManager::DebugDraw()`：根据 `bv_debug_draw_level_*` 与颜色 cvar 生成 `CPhysicDebugDrawContext`，按组件/对象过滤可视化并调用 `debugDrawWorld()`。
  - `BulletPhysicManager_CreateInstance()`：返回 `new CBulletPhysicManager()`。

## 调试 UI 与 VGUI2Extension
- 依赖：`VGUI2Extension.dll`（`VGUI2ExtensionImport.cpp` 通过 `Sys_GetFactory` 获取 `IVGUI2Extension` 与 DPI/VGUI* 接口，失败会 `Sys_Error`）。
- 典型交互：
  - `bv_open_debug_ui` -> `CViewport::OpenPhysicDebugGUI()` -> `PhysicDebugGUI`。
  - BaseUI/GameUI/ClientVGUI 通过 `IVGUI2Extension_*Callbacks` 注册回调，适配 UI 生命周期与输入/窗口过程。

## 构建与第三方依赖
- `Plugins/BulletPhysics/BulletPhysics.vcxproj`：
  - include：`$(Bullet3IncludeDirectory)`、`$(GLEWIncludeDirectory)`、`$(CapstoneIncludeDirectory)`、`$(ScopeExitIncludeDirectory)`、`$(TinyObjLoaderDirectory)`、`$(Chocobo1HashDirectory)` 等。
  - libs：`$(Bullet3LibrariesDirectory)`、`$(GLEWLibrariesDirectory)` + `$(Bullet3LibraryFiles)`、`$(GLEWLibraryFiles)`。
  - pre-build：执行 `$(Bullet3CheckRequirements)`（缺库时自动调用脚本编译 Bullet3）。
- 变量来源：`tools/global_common.props`
  - Bullet3：`thirdparty/install/bullet3/<Platform>/<Config>/...`，缺失时调用 `scripts/build-bullet3-<Platform>-<Config>.bat`。
  - GLEW：`thirdparty/install/glew/<Platform>/<Config>/...`，缺失时调用 `scripts/build-glew-<Platform>-<Config>.bat`。

## 常用 cvar/命令（调试工作流）
- 命令：`bv_open_debug_ui`、`bv_reload_all`、`bv_reload_objects`、`bv_reload_configs`、`bv_save_configs`
- cvar：`bv_debug_draw`、`bv_debug_draw_wallhack`、`bv_debug_draw_level_*`、`bv_debug_draw_*_color`、`bv_simrate`、`bv_syncview`、`bv_force_updatebones`

## 备注
- `AllowCheats()`：SvEngine 下走 `allow_cheats` 指针，其它引擎走 `sv_cheats` cvar。
- `ClientPhysicManager.h` 里声明了 `PhysXPhysicManager_CreateInstance()`，但在 `Plugins/BulletPhysics/` 目录内未找到实现；当前 `LoadEngine` 固定创建 Bullet 版本。
---
title: BulletPhysics
type: note
permalink: metahooksv/bullet-physics
---

﻿# BulletPhysics

## Overview
BulletPhysics 是 MetaHookSv 的客户端物理插件：它把 GoldSrc/SvEngine 的实体、Studio 模型与 BSP 世界映射到 Bullet3 物理世界，提供静态物体、动态物体和布娃娃（ragdoll）的构建、更新、渲染同步、调试绘制与配置编辑。插件通过 MetaHook 的插件接口、客户端导出函数替换、引擎/Studio inline hook 以及 VGUI2Extension 接入游戏运行时。

## Responsibilities
- 在 IPluginsV4 生命周期中初始化/销毁物理管理器，保存引擎、客户端、文件系统和 MetaHook 上下文。
- 枚举当前帧有效的玩家、网络实体和临时实体，按模型配置创建或移除物理对象。
- 维护按 modelindex 索引的物理配置，以及按实体/组件 ID 索引的运行时对象、刚体、约束和行为。
- 管理 Bullet 动力学世界、碰撞过滤、重力、固定步长模拟、射线检测和调试绘制。
- 将物理配置构建为 StaticObject、DynamicObject 或 RagdollObject，并按刚体 → 约束 → 行为的顺序组装组件。
- 通过 StudioSetupBones、StudioDrawModel/Player 和 StudioCheckBBox Hook，在动画骨骼与 Bullet 刚体之间执行双向同步。
- 提供 bv_* 控制台命令、VGUI2Extension 调试窗口、对象/组件检查选择和配置重载/保存能力。

## Involved Files & Symbols
- Plugins/BulletPhysics/plugins.cpp - IPluginsV4::Init、LoadEngine、LoadClient、ExitGame；插件生命周期和 Hook 安装顺序。
- Plugins/BulletPhysics/exportfuncs.cpp - HUD_Init、HUD_GetStudioModelInterface、HUD_CreateEntities、HUD_TempEntUpdate、V_CalcRefdef、HUD_DrawTransparentTriangles、HUD_Shutdown。
- Plugins/BulletPhysics/privatehook.cpp/.h - 引擎/客户端地址定位，R_NewMap、R_RenderView、Studio 绘制和 StudioSetupBones 的 inline/vtable Hook。
- Plugins/BulletPhysics/ClientPhysicManager.h - IClientPhysicManager、IPhysicObject、IPhysicComponent、刚体/约束/行为接口及帧上下文。
- Plugins/BulletPhysics/BasePhysicManager.h/.cpp - CBasePhysicManager；对象、组件、配置、资源缓存、地图切换和通用构建/更新流程。
- Plugins/BulletPhysics/ClientPhysicConfig.h - CClientPhysicObjectConfig、刚体/约束/行为/动画控制/碰撞形状配置及 modelindex 存储。
- Plugins/BulletPhysics/BulletPhysicManager.h/.cpp - CBulletPhysicManager；Bullet world、motion state、碰撞过滤、步进、射线检测和后端对象工厂。
- Plugins/BulletPhysics/BaseStaticObject.*、BaseDynamicObject.*、BaseRagdollObject.* - 与后端无关的对象生命周期、组件容器、动画活动状态和相机同步。
- Plugins/BulletPhysics/BulletStaticObject.*、BulletDynamicObject.*、BulletRagdollObject.* - Bullet 后端对象实现；创建 Bullet 刚体、约束和行为。
- Plugins/BulletPhysics/BulletPhysicRigidBody.*、BulletPhysicConstraint.*、BulletPhysicComponentBehavior.* - Bullet 组件适配层；封装 btRigidBody、btTypedConstraint、motion state 和行为。
- Plugins/BulletPhysics/ClientEntityManager.* - 实体类型/索引、玩家死亡状态、模型缩放、当前帧 emitted/可见状态和实体模型映射。
- Plugins/BulletPhysics/VGUI2ExtensionImport.*、Viewport.*、PhysicDebugGUI.*、PhysicEditorDialog.* - VGUI2Extension 接口、调试视口、检查/选择和配置编辑 UI。
- Plugins/BulletPhysics/PhysicUTIL.* - 配置序列化、模型完整性校验、类型/因子转换和物理工具函数。
- Plugins/BulletPhysics/BulletPhysics.vcxproj - Win32 DLL 构建、C++20、Bullet3/GLEW/Capstone/tinyobjloader 等依赖及预构建检查。
- docs/BulletPhysics.md、docs/BulletPhysicsCN.md - 功能、引擎兼容性和 legacy ragdoll 配置说明；Build/svencoop/bulletphysics/* - UI/本地化运行时资源。

## Architecture
BulletPhysics 采用“MetaHook 接入层 → 通用物理管理层 → Bullet 后端 → 对象/组件层”的分层结构。配置和运行时对象通过管理器关联，渲染 Hook 与物理步进在客户端主循环中汇合：

~~~mermaid
flowchart TD
    Loader["MetaHook loader / IPluginsV4"] --> Plugin["Plugins/BulletPhysics/plugins.cpp"]
    Plugin --> EngineHooks["Engine hooks / privatehook.cpp"]
    Plugin --> ExportHooks["Client exports / exportfuncs.cpp"]
    Plugin --> Manager["IClientPhysicManager"]
    Manager --> BaseManager["CBasePhysicManager"]
    BaseManager --> BulletManager["CBulletPhysicManager"]
    BulletManager --> BulletWorld["btDiscreteDynamicsWorld"]
    ExportHooks --> EntityFrame["HUD_CreateEntities / HUD_TempEntUpdate"]
    EntityFrame --> ObjectFactory["CreatePhysicObjectFromConfig"]
    ObjectFactory --> ModelConfig["modelindex based config"]
    ModelConfig --> PhysicObject["Static / Dynamic / Ragdoll object"]
    PhysicObject --> Components["RigidBody + Constraint + Behavior"]
    Components --> BulletWorld
    BulletWorld --> BoneSync["StudioSetupBones / BoneMatrix sync"]
    BoneSync --> Renderer["GoldSrc Studio renderer"]
    VGUI["VGUI2Extension / PhysicDebugGUI"] --> Manager
~~~

### 1. 插件接入与生命周期
- IPluginsV4::Init 只保存 metahook_api_t、mh_interface_t 和 mh_enginesave_t。
- LoadEngine 获取 FileSystem、引擎类型/build number、engine 与 mirror engine 区段；注册 DLL 加载通知；执行 Engine_FillAddress/Engine_InstallHook；初始化 VGUI2Extension 及 BaseUI/GameUI/ClientVGUI Hook；创建 g_pClientPhysicManager = BulletPhysicManager_CreateInstance；最后初始化 GLEW。
- LoadClient 保存原始 gExportfuncs，替换 HUD_Init、HUD_GetStudioModelInterface、HUD_CreateEntities、HUD_TempEntUpdate、HUD_AddEntity、HUD_DrawTransparentTriangles、HUD_Frame、HUD_Shutdown、V_CalcRefdef、HUD_PostRunCmd，然后执行客户端地址解析和 Hook 安装。
- HUD_Init 在原始初始化后调用 ClientPhysicManager()->Init，注册 bv_debug_draw*、bv_simrate、bv_syncview、bv_force_updatebones 等 cvar，以及 bv_open_debug_ui、bv_reload_*、bv_save_configs 命令，并安装 ClCorpse/临时模型 Hook。
- HUD_Shutdown 先调用原始函数，再关闭物理管理器、Studio Hook 和临时模型 Hook；ExitGame 负责销毁管理器、卸载 UI/引擎 Hook。
- R_NewMap 在原始换图完成后调用物理管理器、实体管理器和调试视口的 NewMap。

### 2. 通用管理器与数据模型
- IClientPhysicManager 定义生命周期、配置管理、物理对象管理、组件管理、世界操作、骨骼桥接、Debug/Inspect/Select 和资源缓存等公共契约。
- CBasePhysicManager 持有 m_physicObjects、m_physicComponents、m_physicConfigs、m_physicObjectConfigs、外部 OBJ/BSP 索引缓存以及调试选择状态；对象 ID 使用 PACK_PHYSIC_OBJECT_ID(entindex, modelindex)，避免实体编号复用时误取旧对象。
- NewMap 清除旧物理对象、BSP 生成配置和 BSP 索引缓存，重置组件/检查选择 ID，重新生成 brush 顶点/索引缓存，加载已知模型配置，并创建世界 brush 的静态物理对象。
- 配置按 modelindex 懒加载：Studio 模型优先读取 <model>_physics.txt，再兼容 <model>_ragdoll.txt；brush 模型从 BSP 生成三角网格静态配置。配置对象包含刚体、约束、行为，ragdoll 额外包含动画控制。
- CreatePhysicObjectForEntity 根据模型类型和实体类型分派 Studio/brush 创建；玩家、死亡玩家、CS/CS:CZ corpse 和临时实体会通过 ClientEntityManager 解析实际模型、玩家索引和缩放，必要时转移旧对象所有权。
- CreatePhysicObjectFromConfig 创建对应后端对象，加载碰撞形状的外部资源，调用 Build 成功后放入 m_physicObjects；构建失败则销毁临时对象。
- UpdateAllPhysicObjects 为每个对象创建 CPhysicObjectUpdateContext，传入当前重力；未在本帧 emitted 的实体会被标记释放，其余对象执行 Update。随后由 StepSimulation 推进 Bullet。

### 3. 对象、组件与后端分层
- CBaseStaticObject、CBaseDynamicObject、CBaseRagdollObject 负责通用对象状态、配置复制、Build/Rebuild、组件容器、生命周期更新、世界加入/移除和查询。
- DispatchBuildPhysicComponents 的顺序是：创建全部刚体；创建非 DeferredCreate 约束；创建行为；最后创建 DeferredCreate 约束。NonNative 组件只保留给上层行为/编辑逻辑，不直接创建 Bullet 原生对象。
- BulletStaticObject、BulletDynamicObject、BulletRagdollObject 实现后端工厂，将配置翻译为 Bullet 刚体、约束和行为；静态对象主要提供质量为零的碰撞体，dynamic/ragdoll 支持约束求解、外力、运动状态和行为。
- IPhysicComponent 的三类主要实现是 IPhysicRigidBody、IPhysicConstraint、IPhysicBehavior。组件通过 configId 和独立的 physicComponentId 同时关联配置和运行时实例，管理器负责全局注册与回收。
- CBulletPhysicManager 负责创建 btDefaultCollisionConfiguration、dispatcher、DBVT broadphase、sequential impulse solver 和自定义 CBulletDiscreteDynamicsWorld；对象/组件通过 Bullet collision group、user index/pointer 和 motion state 回接到 MetaHook 对象。

### 4. 帧时序、物理步进与查询
- HUD_CreateEntities 在原始客户端实体创建后枚举有效玩家和 client edict，标记实体 emitted 并调用 CreatePhysicObjectForEntity。
- HUD_TempEntUpdate 处理临时实体，更新重力，Sven Co-op 第三人称时临时驱动视图更新，然后调用 UpdateAllPhysicObjects 和 StepSimulation。
- CBulletPhysicManager::SetGravity 将 GoldSrc 重力转换到 Bullet 坐标系；StepSimulation 采用 stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate())。GetSimulationTickRate 由 bv_simrate 提供，通用层将其限制在 32–128。
- TraceLine 将 GoldSrc 射线转换到 Bullet，使用过滤组命中世界、静态/动态/ragdoll、约束或行为，并把命中的实体索引和组件 ID 转回 CPhysicTraceLineHitResult。
- HUD_DrawTransparentTriangles 在 AllowCheats 且开启调试 cvar 时调用 DebugDraw；调试上下文按对象/组件层级、颜色、inspect/selected 状态控制可视化。

### 5. Studio 骨骼与物理同步
- HUD_GetStudioModelInterface 缓存 engine_studio_api_s、gpStudioInterface、pbonetransform/plighttransform，解析并安装 EngineStudio/ClientStudio Hook。
- StudioDrawModel/Player 用 g_iRagdollRenderEntIndex 与 g_iRagdollRenderFlags 标记当前 SetupBones 所属实体；STUDIO_RAGDOLL_SETUP_BONES 只让引擎计算动画骨骼，STUDIO_RAGDOLL_UPDATE_BONES 用于把动画结果送入物理侧。
- StudioSetupBones_Template 的顺序是：先尝试物理对象 SetupBones（物理接管时跳过原始 SetupBones），再调用原始 SetupBones，最后尝试 SetupJiggleBones（kinematic/抖动骨骼同步）。
- ragdoll 构建时，SetupBonesForRagdoll 采样动画骨骼，BulletCreateMotionState 生成 CBulletBoneMotionState；动态刚体通过 setWorldTransform 更新 bone matrix，渲染 Hook 再将其写回 pbonetransform/plighttransform。kinematic 状态则反向把最新动画骨骼写入 motion state。
- V_CalcRefdef 在非暂停、非 intermission、非 portal 等条件下，把 spectator/本地玩家相机同步交给物理对象的 CalcRefDef；bv_syncview 决定同步级别。

### 6. 调试 UI 与配置编辑
- bv_open_debug_ui 进入 CViewport::OpenPhysicDebugGUI；CViewport 通过 VGUI2Extension 创建 CPhysicDebugGUI，并在 NewMap、每帧 inspect 更新和 UI 生命周期事件中转发状态。
- PhysicDebugGUI 提供实体/物理对象/刚体/约束/行为五类检查模式、选择颜色、编辑对话框、配置重载和保存；编辑后通常通过 RebuildPhysicObjectEx2 重新构建对象，同时保留可复用的组件 ID。
- VGUI2ExtensionImport 从 VGUI2Extension.dll 取得 IVGUI2Extension、DPI、Surface、Scheme 和 Input 接口，并通过 DLL 加载通知初始化 IKeyValuesSystem，供配置 KeyValues 和 UI 使用。

## Dependencies
- MetaHook API 与公共接口：include/metahook.h、include/Interface/；用于插件 ABI、地址解析、inline/vtable Hook、引擎类型和文件系统访问。
- GoldSrc/SvEngine 客户端与 Studio 接口：HLSDK 的 cl_dll、engine、studio、r_efx 结构，以及 engine_studio_api_s、cl_exportfuncs_t。
- Bullet3：btBulletDynamicsCommon 及 collision/ghost 组件；由 Bullet3IncludeDirectory、Bullet3LibrariesDirectory、Bullet3LibraryFiles 提供。
- VGUI2Extension：运行时 VGUI2Extension.dll；提供 IVGUI2Extension、DPI、Surface、Scheme、Input 和 KeyValuesSystem。
- SourceSDK/KeyValues/FileSystem：配置读写、模型文件访问和资源路径；HL25 兼容时使用 IFileSystem_HL25。
- GLEW：调试绘制的 OpenGL 扩展加载；HUD_Init 后执行 glewInit。
- Capstone：private hook 的指令扫描/反汇编地址定位。
- tinyobjloader、ScopeExit、Chocobo1Hash：OBJ 碰撞资源、作用域清理和哈希/完整性辅助；均在 BulletPhysics.vcxproj 的 include/build 依赖中声明。
- 运行时资源：*_physics.txt/*_ragdoll.txt、BSP brush 数据、外部 .obj 碰撞网格、Build/svencoop/bulletphysics/*.res 和本地化文本。

## Notes
- 该插件是基于全局指针、全局 Hook 和单一客户端物理管理器的运行时系统；源码未提供跨线程同步，物理对象、配置和渲染矩阵应遵循客户端 Hook 的主线程时序访问。
- Hook 的签名和 vtable 偏移按 engine/client build number 与 mirror module 解析；新增引擎版本时，privatehook.cpp、exportfuncs.cpp 和 enginedef.h 是高风险兼容边界。
- m_physicObjects 以实体索引为主键，而一个实体可能因模型切换、corpse 转移或临时实体复用而更换模型；跨帧访问应优先使用 packed physic object ID 校验 modelindex。
- 当前帧 emitted 状态决定对象是否存活；如果实体枚举/Hook 顺序改变，UpdateAllPhysicObjects 可能把对象误判为过期并释放。
- NewMap 会清除 BSP 来源对象、配置和索引缓存，再重新建立世界碰撞；地图切换和配置重载不能与正在执行的对象更新交错。
- bv_simrate 过低或过高会被强制限制到 32–128；Bullet 固定步长最多使用 4 个子步，过大的 frametime 仍可能带来模拟误差或 CPU 峰值。
- AllowCheats 是调试命令、调试绘制和配置保存的门禁；排查配置问题时需确认 sv_cheats/SvEngine 的 allow_cheats 路径。
- VGUI2Extension.dll 缺失时初始化函数直接返回；若 DLL 存在但接口工厂或所需接口缺失，则调用 Sys_Error。调试 UI 不是物理核心运行的唯一依赖，但配置编辑依赖它。
- ClientPhysicManager.h 仍声明 PhysXPhysicManager_CreateInstance，当前 LoadEngine 固定创建 Bullet 后端；不要据此假设存在可切换的 PhysX 实现。
- 配置加载/保存和 BoneMatrix 同步已有独立专题，分别参见 [[bulletphysics_physics_config]] 与 [[bulletphysics_bonematrix_physics_sync]]；插件接入细节参见 [[bulletphysics_plugin_overview]]，整体插件边界参见 [[plugin_system]]。

## Callers (optional)
- IPluginsV4::LoadEngine -> BulletPhysicManager_CreateInstance
- HUD_Init -> IClientPhysicManager::Init
- R_NewMap -> IClientPhysicManager::NewMap
- HUD_CreateEntities -> CreatePhysicObjectForEntity
- HUD_TempEntUpdate -> UpdateAllPhysicObjects -> StepSimulation
- HUD_GetStudioModelInterface -> Studio/ClientStudio Hook installation and bone matrix capture
- StudioSetupBones Hook -> SetupBones / SetupJiggleBones
- V_CalcRefdef -> IPhysicObject::CalcRefDef
- bv_open_debug_ui -> CViewport::OpenPhysicDebugGUI
- bv_reload_configs / bv_save_configs -> configuration lifecycle methods

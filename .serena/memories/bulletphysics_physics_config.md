# BulletPhysics 物理配置（physics config）

> 目标：描述 BulletPhysics 插件里 **物理配置的加载 / 保存 / 使用** 全流程（配置文件 → 内存结构 → 物理对象构建/重建 → 落盘）。

## 1) 配置的“单位”与存储位置

- **配置单位**：以 `model_t` 为粒度（模型/brushmodel/世界模型等），每个模型对应一个 `CClientPhysicObjectConfig`（静态/动态/布娃娃三选一）。
- **存储容器**：`CBasePhysicManager::m_physicObjectConfigs`（按 `modelindex` 索引的 `CClientPhysicObjectConfigStorage`）。
  - `Storage.state`：`PhysicConfigState_NotLoaded / Loaded / LoadedWithError`
  - `Storage.modelname`：记录模型名（用于保存时拼接文件名）
  - `Storage.pConfig`：配置对象（`shared_ptr`）
- **配置对象的子结构**：
  - `CClientPhysicObjectConfig` 包含 `RigidBodyConfigs / ConstraintConfigs / PhysicBehaviorConfigs`；若是 ragdoll 还包含 `AnimControlConfigs`。
  - 每个子配置（刚体/约束/行为/动画控制/碰撞形状）都有 `configId`，并通过 `ClientPhysicManager()->AddPhysicConfig(configId, ptr)` 注册到全局配置表，方便 UI / 运行时引用。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

## 2) 加载（Load）触发点

### 2.1 换图加载
- `CBasePhysicManager::NewMap()`：
  - 清理旧物理对象：`RemoveAllPhysicObjects(...)`
  - 清理 **BSP 生成** 的配置：`RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromBSP, 0)`
  - 生成 brush 的碰撞索引数据（BSP mesh）并缓存
  - 调用 `LoadPhysicObjectConfigs()` 预加载已知模型的配置
  - 创建世界 brush 的物理对象：`CreatePhysicObjectForBrushModel(..., *cl_worldmodel)`

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

### 2.2 运行时按需加载
- 当需要为某个实体创建物理对象时，会调用 `LoadPhysicObjectConfigForModel(mod)`；如果该 `modelindex` 仍是 `NotLoaded`，会在这里触发真正的“从文件/BSP”加载。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

## 3) 加载（Load）路径与优先级

### 3.1 Studio 模型：从文件加载（新格式优先）
- 入口：`CBasePhysicManager::LoadPhysicObjectConfigFromFiles(model_t *mod, Storage)`
- 文件名推导：
  1) `modelname = mod->name`，去掉扩展名（如 `.mdl`）
  2) 先尝试：`<modelname>_physics.txt`（**新格式**）
  3) 再尝试：`<modelname>_ragdoll.txt`（**旧格式**/legacy）
- 成功后：`OverwritePhysicObjectConfig(modelname, Storage, pConfig)`
  - `Storage.state = Loaded`
  - `pConfig->modelName = modelname`
  - `pConfig->shortName = V_FileBase(modelname)`

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

### 3.2 Brush 模型：从 BSP/资源生成配置
- 入口：`CBasePhysicManager::LoadPhysicObjectConfigFromBSP(mod, Storage)`
- 逻辑：
  - 从 brush 资源生成/加载三角网格索引数据
  - 构造一个 `CClientCollisionShapeConfig`（`PhysicShape_TriangleMesh`，`resourcePath` 指向 brush 资源）
  - 构造一个 `CClientRigidBodyConfig`（`mass=0`，指向上述 collision shape）
  - 构造一个 `CClientStaticObjectConfig`，并设置 `flags |= PhysicObjectFlag_FromBSP`
  - `OverwritePhysicObjectConfig(resourcePath, Storage, pStaticObjectConfig)`
- 注意：这种 **BSP 生成的 config** 不是“文件配置”，默认不会落盘。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

## 4) 新格式文件（KeyValues）结构（读/写一致）

### 4.1 读取
- `LoadPhysicObjectConfigFromNewFile(mod, filename)`：
  - `KeyValues("PhysicObjectConfig")` + `LoadFromFile(g_pFileSystem[_HL25], filename)`
  - 交给 `LoadPhysicObjectConfigFromKeyValues(mod, pKeyValues)`
- `LoadPhysicObjectConfigFromKeyValues` 根据 `type` 字段分派：
  - `"RagdollObject" / "StaticObject" / "DynamicObject"`
- `LoadPhysicObjectFlagsFromKeyValues`：无条件设置 `flags |= PhysicObjectFlag_FromConfig`，并读取：
  - `barnacle` / `gargantua` / `overrideStudioCheckBBOX`
- 完整性校验：`verifyBoneChunk`/`verifyModelFile` + 对应 `crc32...`；失败则返回 `nullptr`。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

### 4.2 写出（序列化）
- `SavePhysicObjectConfigToNewFile(filename, config)`：
  - `ConvertPhysicObjectConfigToKeyValues(config)`（根节点同为 `KeyValues("PhysicObjectConfig")`）
  - `KeyValues::SaveToFile(...)`
- `AddBaseConfigToKeyValues` 写入：
  - `type`（由 `UTIL_GetPhysicObjectConfigTypeName` 输出）
  - `barnacle/gargantua/overrideStudioCheckBBOX`（flags → KV）
- `AddVerifyStuffsFromKeyValues` 写入：
  - `verifyBoneChunk/verifyModelFile` 与 `crc32BoneChunk/crc32ModelFile`

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

### 4.3 碰撞形状（collisionShape）KV 关键字段
- 读取：`LoadCollisionShapeFromKeyValues` 支持字段：
  - `type`, `direction`, `origin`, `angles`, `size`, `resourcePath`, `compoundShapes`
- 写出：`AddCollisionShapeToKeyValues` 对应写回同名字段（只在非默认值时写部分字段）。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

## 5) 旧格式文件（legacy `_ragdoll.txt`）

- `LoadPhysicObjectConfigFromLegacyFile(filename)`：`COM_LoadFile` 读全文，然后 `LoadPhysicObjectConfigFromLegacyFileBuffer(buf)`
- legacy 是按 section 的文本格式（如 `[RigidBody]`, `[Constraint]` 等），逐行解析。
- legacy 产物是 `CClientRagdollObjectConfig`，并显式设置：
  - `flags |= PhysicObjectFlag_FromConfig`
  - `flags |= PhysicObjectFlag_OverrideStudioCheckBBox`

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`

## 6) 保存（Save）触发点与落盘规则

### 6.1 触发入口
- 控制台命令：`bv_save_configs` → `BV_SaveConfigs_f()`
- Debug UI：`CPhysicDebugGUI::SaveOpenPrompt()` → `SaveConfirm()` → `BV_SaveConfigs_f()`
- 上述入口都会先检查 `AllowCheats()`（通常受 `sv_cheats` 等约束）。

相关实现：`Plugins/BulletPhysics/exportfuncs.cpp`, `Plugins/BulletPhysics/PhysicDebugGUI.cpp`

### 6.2 保存内容与条件
- `CBasePhysicManager::SavePhysicObjectConfigs()` 遍历 `EngineGetNumKnownModel()`：
  - 仅对 **已加载** 的 studio 模型配置尝试保存
  - 调用 `SavePhysicObjectConfigToFile(mod->name, config)`
- `SavePhysicObjectConfigToFile` 的“硬门槛”：
  - 必须 `flags & PhysicObjectFlag_FromConfig`（即来自文件/或被 UI 标记成文件配置）
  - 必须 `UTIL_IsPhysicObjectConfigModified(...) == true`
- 文件名：`<mod->name 去扩展名>_physics.txt`（只写新格式，不回写 legacy）
- 写成功后：`UTIL_SetPhysicObjectConfigUnmodified` 递归清掉对象/子配置的 `configModified`。
- 写文件路径：优先尝试在 `GAMEDOWNLOAD` 目录写（会创建目录），失败再尝试默认路径。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`, `Plugins/BulletPhysics/PhysicUTIL.cpp`

## 7) Reload（重载）与编辑器创建新配置

### 7.1 重载配置
- 控制台命令：`bv_reload_configs` → `BV_ReloadConfigs_f()`：
  - `FreeAllIndexArrays(PhysicIndexArrayFlag_FromExternal, PhysicIndexArrayFlag_FromBSP)`（清掉外部 mesh，保留 BSP mesh）
  - `RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromConfig, 0)`（清掉文件配置）
  - `LoadPhysicObjectConfigs()` 重新加载

相关实现：`Plugins/BulletPhysics/exportfuncs.cpp`

### 7.2 Debug UI 创建空配置（让其可保存）
- `CPhysicDebugGUI::OnCreateStaticObject/DynamicObject/RagdollObject`：
  - 若该 `modelindex` 还没有 config，则 `CreateEmptyPhysicObjectConfigForModelIndex(modelindex, type)`
  - 随后显式 `pConfig->flags |= PhysicObjectFlag_FromConfig`，确保后续 `bv_save_configs` 能落盘。

相关实现：`Plugins/BulletPhysics/PhysicDebugGUI.cpp`

## 8) 运行时如何“使用”配置

- 实体走向：`CreatePhysicObjectForEntity` → `CreatePhysicObjectForStudioModel/BrushModel` → `CreatePhysicObjectFromConfig`。
- `CreatePhysicObjectFromConfig`：
  1) `LoadPhysicObjectConfigForModel(mod)` 拿到 config
  2) `LoadAdditionalResourcesForConfig(config)`：为 `collisionShape.resourcePath` 触发 mesh/index array 缓存加载
  3) 按 `config->type` 创建对应物理对象（Ragdoll/Dynamic/Static），并 `Build(CreationParam)`
- 编辑器修改后生效：常见路径是 `ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig)`，将新的 config 重新喂给对象的 `Rebuild()`。

相关实现：`Plugins/BulletPhysics/BasePhysicManager.cpp`


## KeyValues 配置格式（`*_physics.txt`，新格式）

> 该格式由 `Plugins/BulletPhysics/BasePhysicManager.cpp` 读写（`KeyValues("PhysicObjectConfig")`），用于描述 **Studio 模型**（`.mdl`）的物理对象配置。
>
> 文件名推导：`<modelname 去扩展名>_physics.txt`（新格式，优先）；旧格式为 `<modelname 去扩展名>_ragdoll.txt`（legacy）。

### 1) 顶层结构（总体示例）

```text
"PhysicObjectConfig"
{
    "type" "RagdollObject"

    "barnacle" "0"
    "gargantua" "0"
    "overrideStudioCheckBBOX" "0"

    "verifyBoneChunk" "0"
    "crc32BoneChunk" ""
    "verifyModelFile" "0"
    "crc32ModelFile" ""

    "rigidBodies"
    {
        "pelvis"
        {
            "boneindex" "0"
            "mass" "1"
            "collisionShape"
            {
                "type" "Capsule"
                "direction" "1"
                "size" "4 12 0"
            }
        }
    }

    "constraints"
    {
    }

    "physicBehaviors"
    {
    }

    "animControls"
    {
    }
}
```

### 2) 顶层参数说明（`"PhysicObjectConfig"` 直接子项）

- `type`：物理对象类型字符串；决定后续会读取哪些分组。
  - `StaticObject`：只读 `rigidBodies`
  - `DynamicObject`：读 `rigidBodies` + `constraints`
  - `RagdollObject`：读 `rigidBodies` + `constraints` + `physicBehaviors` + `animControls`
- `barnacle`：为该物理对象打上 “Barnacle 相关” 标记（`PhysicObjectFlag_Barnacle`）。
- `gargantua`：为该物理对象打上 “Gargantua 相关” 标记（`PhysicObjectFlag_Gargantua`）。
- `overrideStudioCheckBBOX`：启用对 Studio 模型 `StudioCheckBBox` 等检查的覆盖标记（`PhysicObjectFlag_OverrideStudioCheckBBox`）。
- `verifyBoneChunk`：是否对模型 bone chunk 做完整性校验（通过 `VerifyIntegrityForPhysicObjectConfig`）。
- `crc32BoneChunk`：bone chunk 的 CRC32 字符串（用于校验）。
- `verifyModelFile`：是否对模型文件整体做完整性校验（通过 `VerifyIntegrityForPhysicObjectConfig`）。
- `crc32ModelFile`：模型文件的 CRC32 字符串（用于校验）。
- `rigidBodies`：刚体配置表（子 Key 的 **名字** 就是刚体名，供 constraint/behavior 引用）。
- `constraints`：约束配置表（子 Key 的 **名字** 就是约束名，供 behavior 引用）。
- `physicBehaviors`：行为配置表（子 Key 的 **名字** 是行为名）。
- `animControls`：动画控制配置列表（子 Key 名不重要，读取时不会使用 name）。

### 3) `rigidBodies`（刚体）参数说明

`rigidBodies` 下每个子 Key 的 name 会作为 `CClientRigidBodyConfig::name`；其中字段：

- 状态/碰撞 flags（bool，存在且为 true 即生效）：
  - `alwaysDynamic` / `alwaysKinematic` / `alwaysStatic`
  - `invertStateOnIdle` / `invertStateOnDeath`
  - `invertStateOnCaughtByBarnacle` / `invertStateOnBarnaclePulling` / `invertStateOnBarnacleChewing`
  - `invertStateOnGargantuaBite`
  - `noCollisionToWorld` / `noCollisionToStaticObject` / `noCollisionToDynamicObject` / `noCollisionToRagdollObject`
- `debugDrawLevel`：调试绘制层级（用于 DebugDraw 过滤）。
- `boneindex`：该刚体绑定的骨骼索引；`-1` 表示不绑定。
- `origin`：局部偏移（字符串向量 `"x y z"`），解析为 `vec3_t`。
- `angles`：局部旋转欧拉角（字符串向量 `"pitch yaw roll"`）。
- `forward`：一个前向向量（字符串向量 `"x y z"`），用于部分约束/朝向计算。
- `isLegacyConfig`：标记该刚体是否由 legacy 格式迁移/兼容（0/1）。
- `pboneindex` / `pboneoffset`：legacy 兼容用的父骨信息与偏移。
- 物理参数（float）：
  - `mass`：质量（默认 `BULLET_DEFAULT_MASS`）。
  - `density`：密度（默认 `BULLET_DEFAULT_DENSENTY`）。
  - `linearFriction`：线性摩擦（默认 `BULLET_DEFAULT_LINEAR_FRICTION`）。
  - `rollingFriction`：滚动/角摩擦（默认 `BULLET_DEFAULT_ANGULAR_FRICTION`）。
  - `restitution`：弹性系数（默认 `BULLET_DEFAULT_RESTITUTION`）。
  - `ccdRadius`：CCD 半径（默认 0）。
  - `ccdThreshold`：CCD 阈值（默认 `BULLET_DEFAULT_CCD_THRESHOLD`）。
  - `linearSleepingThreshold` / `angularSleepingThreshold`：睡眠阈值（默认 `BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD / BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD`）。
  - `additionalDampingFactor` / `additionalLinearDampingThresholdSqr` / `additionalAngularDampingThresholdSqr`：额外阻尼相关参数（对应 Bullet 的额外阻尼设置）。
- `collisionShape`：碰撞形状子结构（见下一节）。

### 4) `collisionShape`（碰撞形状）参数说明

`collisionShape` 对应 `CClientCollisionShapeConfig`，支持字段：

- `type`：形状类型字符串（`None/Box/Sphere/Capsule/Cylinder/MultiSphere/TriangleMesh/Compound`）。
- `direction`：形状主轴方向（`0=X, 1=Y, 2=Z`；默认 `1`）。
- `origin`：局部偏移（字符串向量 `"x y z"`）。
- `angles`：局部旋转（字符串向量 `"x y z"`）。
- `size`：尺寸（字符串向量；支持 1/2/3 分量，解析顺序为 vec3→vec2→vec1）。
- `resourcePath`：外部资源路径（主要用于 `TriangleMesh` 之类从资源加载 mesh 的形状）。
- `compoundShapes`：仅对 `Compound` 有意义；为子形状列表（每个子 key 仍是一份 `collisionShape` 结构）。

### 5) `constraints`（约束）参数说明

`constraints` 下每个子 Key 的 name 会作为 `CClientConstraintConfig::name`；通用字段：

- `type`：约束类型字符串（`None/ConeTwist/Hinge/Point/Slider/Dof6/Dof6Spring/Fixed`）。
- `rigidbodyA` / `rigidbodyB`：引用刚体名（必须与 `rigidBodies` 下的子 key 名一致）。
- `originA` / `anglesA` / `originB` / `anglesB`：局部框架信息（字符串向量 `"x y z"`）。
- `forward`：辅助向量（字符串向量 `"x y z"`）。
- 约束 flags（bool，存在且为 true 即生效）：
  - `barnacle` / `gargantua`
  - `deactiveOnNormalActivity` / `deactiveOnDeathActivity`
  - `deactiveOnCaughtByBarnacleActivity` / `deactiveOnBarnaclePullingActivity` / `deactiveOnBarnacleChewingActivity`
  - `deactiveOnGargantuaBiteActivity`
  - `dontResetPoseOnErrorCorrection`
  - `DeferredCreate`
- 布尔配置（bool；未提供则使用默认值）：
  - `disableCollision`（默认 true）
  - `useGlobalJointFromA`（默认 true）
  - `useLinearReferenceFrameA`（默认 true）
  - `useLookAtOther`（默认 false）
  - `useGlobalJointOriginFromOther`（默认 false）
  - `useRigidBodyDistanceAsLinearLimit`（默认 false）
  - `useSeperateLocalFrame`（默认 false）
- `debugDrawLevel`：调试绘制层级（默认 `BULLET_DEFAULT_DEBUG_DRAW_LEVEL`）。
- `maxTolerantLinearError`：最大可容忍的线性误差（默认 `BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR`）。
- `isLegacyConfig`：legacy 兼容标记（默认 false）。
- `boneindexA` / `boneindexB`：legacy 兼容骨骼索引（默认 `-1`）。
- `offsetA` / `offsetB`：legacy 兼容偏移（字符串向量 `"x y z"`）。

`constraints/"<name>"/factors`：约束参数表（float；只对当前 `type` 的相关项生效；未设置时内部以 `NAN` 标记为“未提供”）。

- `ConeTwist`：`ConeTwistSwingSpanLimit1/ConeTwistSwingSpanLimit2/ConeTwistTwistSpanLimit/ConeTwistSoftness/ConeTwistBiasFactor/ConeTwistRelaxationFactor/LinearERP/LinearCFM/AngularERP/AngularCFM`
- `Hinge`：`HingeLowLimit/HingeHighLimit/HingeSoftness/HingeBiasFactor/HingeRelaxationFactor/AngularERP/AngularCFM/AngularStopERP/AngularStopCFM`
- `Point`：`AngularERP/AngularCFM`
- `Slider`：`SliderLowerLinearLimit/SliderUpperLinearLimit/SliderLowerAngularLimit/SliderUpperAngularLimit/LinearCFM/LinearStopERP/LinearStopCFM/AngularCFM/AngularStopERP/AngularStopCFM`
- `Dof6`：`Dof6LowerLinearLimitX/Y/Z/Dof6UpperLinearLimitX/Y/Z/Dof6LowerAngularLimitX/Y/Z/Dof6UpperAngularLimitX/Y/Z/LinearCFM/LinearStopERP/LinearStopCFM/AngularCFM/AngularStopERP/AngularStopCFM`
- `Dof6Spring`：在 `Dof6` 基础上增加
  - `Dof6SpringEnableLinearSpringX/Y/Z`、`Dof6SpringEnableAngularSpringX/Y/Z`
  - `Dof6SpringLinearStiffnessX/Y/Z`、`Dof6SpringAngularStiffnessX/Y/Z`
  - `Dof6SpringLinearDampingX/Y/Z`、`Dof6SpringAngularDampingX/Y/Z`
- `Fixed`：`LinearCFM/LinearStopERP/LinearStopCFM/AngularCFM/AngularStopERP/AngularStopCFM`

### 6) `physicBehaviors`（行为）参数说明

`physicBehaviors` 下每个子 Key 的 name 会作为 `CClientPhysicBehaviorConfig::name`；通用字段：

- `type`：行为类型字符串：
  - `None/BarnacleDragOnRigidBody/BarnacleDragOnConstraint/BarnacleChew/BarnacleConstraintLimitAdjustment/GargantuaDragOnConstraint/FirstPersonViewCamera/ThirdPersonViewCamera/SimpleBuoyancy/RigidBodyRelocation`
- `rigidbodyA` / `rigidbodyB`：引用刚体名（按行为类型决定是否使用）。
- `constraint`：引用约束名（按行为类型决定是否使用）。
- `barnacle` / `gargantua`：行为 flags（bool）。
- `origin` / `angles`：行为的局部位姿（字符串向量 `"x y z"`）。

`physicBehaviors/"<name>"/factors`：行为参数表（float；只对当前 `type` 的相关项生效；未设置时内部以 `NAN` 标记为“未提供”）。

- `BarnacleDragOnRigidBody`：`BarnacleDragMagnitude/BarnacleDragExtraHeight`
- `BarnacleDragOnConstraint`：`BarnacleDragMagnitude/BarnacleDragVelocity/BarnacleDragExtraHeight/BarnacleDragLimitAxis/BarnacleDragCalculateLimitFromActualPlayerOrigin/BarnacleDragUseServoMotor/BarnacleDragActivatedOnBarnaclePulling/BarnacleDragActivatedOnBarnacleChewing`
- `BarnacleChew`：`BarnacleChewMagnitude/BarnacleChewInterval`
- `BarnacleConstraintLimitAdjustment`：`BarnacleConstraintLimitAdjustmentExtraHeight/BarnacleConstraintLimitAdjustmentInterval/BarnacleConstraintLimitAdjustmentAxis`
- `GargantuaDragOnConstraint`：`BarnacleDragMagnitude/BarnacleDragVelocity/BarnacleDragExtraHeight/BarnacleDragLimitAxis/BarnacleDragUseServoMotor`
- `FirstPersonViewCamera` / `ThirdPersonViewCamera`：`CameraActivateOnIdle/CameraActivateOnDeath/CameraActivateOnCaughtByBarnacle/CameraSyncViewOrigin/CameraSyncViewAngles/CameraUseSimOrigin/CameraOriginalViewHeightStand/CameraOriginalViewHeightDuck/CameraMappedViewHeightStand/CameraMappedViewHeightDuck/CameraNewViewHeightDucking`
- `SimpleBuoyancy`：`SimpleBuoyancyMagnitude/SimpleBuoyancyLinearDamping/SimpleBuoyancyAngularDamping`

### 7) `animControls`（动画控制）参数说明（仅 `RagdollObject` 使用）

`animControls` 是一个“列表”（子 key 名无意义），每个子项字段：

- `sequence`：动作序列号（默认 -1）。
- `gaitsequence`：步态序列号（默认 -1）。
- `animframe`：动画帧（float，默认 0）。
- `activityType`：活动类型（int）。
- `flags`：标记位（int）。
- `controller_0..3`：controller 值（int，默认 -1）。
- `blending_0..3`：blending 值（int，默认 -1）。

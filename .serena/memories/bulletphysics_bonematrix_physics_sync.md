# BulletPhysics：渲染 BoneMatrix ↔ Bullet 刚体 的双向同步机制

> 目标：解释“渲染侧使用的 bonematrix（`pbonetransform`）如何同步到 Bullet 物理”，以及“Bullet 物理（刚体状态/属性）如何反向影响渲染使用的 bonematrix”。

## 关键全局指针/状态（桥梁）
- `Plugins/BulletPhysics/exportfuncs.cpp`：在 `HUD_GetStudioModelInterface(...)` 中从 `engine_studio_api_s` 取出并缓存：
  - `pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();`
  - `plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();`
- `Plugins/BulletPhysics/privatehook.h/.cpp`：定义/导出 `pbonetransform/plighttransform`、`pstudiohdr`、`currententity` 等，供渲染 Hook 与物理逻辑共用。
- `Plugins/BulletPhysics/exportfuncs.cpp`：用于“这次 SetupBones 属于哪个实体”的哨兵变量：
  - `g_iRagdollRenderEntIndex`、`g_iRagdollRenderFlags`。
  - 只有 `g_iRagdollRenderEntIndex > 0` 时，才会让 `ClientPhysicManager()->SetupBones/SetupJiggleBones` 介入。

## 渲染侧入口：Hook StudioSetupBones / StudioDraw*（驱动同步时机）
### 1) StudioSetupBones Hook（真正发生 bone 同步的地方）
- `Plugins/BulletPhysics/exportfuncs.cpp`：
  - `StudioSetupBones_Template(...)` 在每次引擎/客户端执行 `StudioSetupBones` 时构造 `CRagdollObjectSetupBoneContext`：
    - `Context.m_studiohdr = (*pstudiohdr)`
    - `Context.m_entindex = g_iRagdollRenderEntIndex`
    - `Context.m_flags = g_iRagdollRenderFlags`
  - 调用顺序是：
    1) 若 `g_iRagdollRenderEntIndex > 0 && ClientPhysicManager()->SetupBones(&Context)` 返回 true → **直接 return**（跳过原始 SetupBones）。
    2) 否则执行原始 `SetupBones`（动画计算写入 `pbonetransform`）。
    3) 若 `g_iRagdollRenderEntIndex > 0 && ClientPhysicManager()->SetupJiggleBones(&Context)` 返回 true → return。

### 2) StudioDrawModel / StudioDrawPlayer Hook（决定何时把 entindex/flags 填给 SetupBones）
- `Plugins/BulletPhysics/exportfuncs.cpp`：
  - `StudioDrawModel_Template(...)` / `StudioDrawPlayer_Template(...)` 对 ragdoll、更新骨骼等场景临时设置：
    - `g_iRagdollRenderEntIndex = entindex; g_iRagdollRenderFlags = flags;`
    - 调用原始 `StudioDraw*` 后清零。
  - 对两个特殊 flag：
    - `STUDIO_RAGDOLL_SETUP_BONES`：直接调用原始 draw 且 **不设置** `g_iRagdollRenderEntIndex`（让引擎纯动画算骨骼，用于“采样骨骼”）。
    - `STUDIO_RAGDOLL_UPDATE_BONES`：设置 `g_iRagdollRenderEntIndex/Flags`，但调用原始 draw 时传入 flags=0（用于“更新骨骼→同步到物理”，不走正常渲染路径）。
  - flag 定义：`Plugins/BulletPhysics/enginedef.h`（`STUDIO_RAGDOLL_SETUP_BONES 0x10`，`STUDIO_RAGDOLL_UPDATE_BONES 0x20`）。

## A. 渲染 BoneMatrix → 物理（矩阵→物理）
这条链路主要有两类：**构建时采样** 和 **运行时更新（kinematic 跟随骨骼）**。

### A1) 构建时：用动画骨骼初始化 Bullet 的 bone-based MotionState
- `Plugins/BulletPhysics/BaseRagdollObject.cpp`：`CBaseRagdollObject::Build(...)`（studio 模型）会先调用：
  - `ClientPhysicManager()->SetupBonesForRagdoll(...)` 或 `SetupBonesForRagdollEx(...)`（Idle anim override）。
- `Plugins/BulletPhysics/BasePhysicManager.cpp`：`CBasePhysicManager::SetupBonesForRagdoll*` 内部调用：
  - `(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL_SETUP_BONES)` 或 `StudioDrawPlayer(STUDIO_RAGDOLL_SETUP_BONES, ...)`。
- 由于 `STUDIO_RAGDOLL_SETUP_BONES` 分支不会设置 `g_iRagdollRenderEntIndex`，所以 `StudioSetupBones_Template` 不会调用物理侧 `SetupBones/SetupJiggleBones`：
  - **最终效果**：引擎用动画把 `(*pbonetransform)[i]` 计算好，作为“当前姿态骨骼快照”。
- 随后创建刚体时：
  - `Plugins/BulletPhysics/BulletPhysicManager.cpp`：`BulletCreateMotionState(...)` 会读取 `(*pbonetransform)[boneindex]` 生成 `btTransform bonematrix`（并 `TransformGoldSrcToBullet`），再用配置生成 `localTrans`（偏移矩阵/局部姿态），并返回：
    - `new CBulletBoneMotionState(pPhysicObject, bonematrix, localTrans)`
  - `Plugins/BulletPhysics/BulletPhysicManager.h`：`CBulletBoneMotionState::getWorldTransform` 固定采用：
    - `worldTrans = m_bonematrix * m_offsetmatrix`
  - `Plugins/BulletPhysics/BulletPhysicRigidBody.cpp`：`CBulletPhysicRigidBody` 构造 `btRigidBody` 后，将 motionstate 反向绑定内部刚体：
    - `pMotionState->SetInternalRigidBody(m_pInternalRigidBody);`

### A2) 运行时：更新 kinematic 刚体（骨骼驱动刚体）
- `Plugins/BulletPhysics/BaseRagdollObject.cpp`：`CBaseRagdollObject::Update(...)` 中，当对象不可见或 `bv_force_updatebones` 等条件满足，会置位：
  - `ObjectUpdateContext->m_bRigidbodyUpdateBonesRequired = true`
  - 然后调用 `UpdateBones(playerState)`。
- `UpdateBones` 实现：`CBaseRagdollObject::UpdateBones` → `ClientPhysicManager()->UpdateBonesForRagdoll(...)`。
- `Plugins/BulletPhysics/BasePhysicManager.cpp`：`CBasePhysicManager::UpdateBonesForRagdoll(...)` 触发：
  - `StudioDrawModel(STUDIO_RAGDOLL_UPDATE_BONES)` / `StudioDrawPlayer(STUDIO_RAGDOLL_UPDATE_BONES, ...)`。
- `STUDIO_RAGDOLL_UPDATE_BONES` 分支会设置 `g_iRagdollRenderEntIndex/Flags`，因此 `StudioSetupBones_Template` 会对该实体执行：
  1) 先尝试 `ClientPhysicManager()->SetupBones`（多数“非 OverrideAllBones”场景会返回 false，允许引擎算骨骼）。
  2) 引擎算完骨骼后，进入 `ClientPhysicManager()->SetupJiggleBones`。
- 在 bone-based 刚体上，`Plugins/BulletPhysics/BulletRagdollRigidBody.cpp` 与 `BulletDynamicRigidBody.cpp` 的 `SetupJiggleBones`（kinematic 分支）会：
  - `Matrix3x4ToTransform((*pbonetransform)[bone], pBoneMotionState->m_bonematrix)`
  - `TransformGoldSrcToBullet(pBoneMotionState->m_bonematrix)`
  - 这等价于把“渲染骨骼矩阵”写入 Bullet 的 bone motion state。
- 随后 Bullet 每帧对 kinematic 刚体的姿态读取，会通过 `CBulletBoneMotionState::getWorldTransform` 使用最新的 `m_bonematrix`，从而实现 **骨骼 → 刚体**。

## B. 物理（刚体）→ 渲染 BoneMatrix（物理→矩阵）
这条链路的核心是：**Bullet 更新刚体 world transform → motionstate 反推 bone matrix → 渲染时写回 `pbonetransform`**。

### B1) Bullet 将模拟结果写入 CBulletBoneMotionState::m_bonematrix
- `Plugins/BulletPhysics/BulletPhysicManager.h`：
  - `CBulletBoneMotionState::setWorldTransform(worldTrans)`：
    - `m_bonematrix = worldTrans * inverse(m_offsetmatrix)`
  - 对动态刚体，`stepSimulation` 后 Bullet 会调用 motionstate 的 `setWorldTransform` 来同步结果。

### B2) 渲染/骨骼计算阶段：把 m_bonematrix 写回 pbonetransform（供渲染使用）
- `Plugins/BulletPhysics/exportfuncs.cpp`：渲染路径进入 `StudioSetupBones_Template` 后，如果物理侧决定“本次由物理接管骨骼”，会走 `ClientPhysicManager()->SetupBones` 并返回 true → 直接跳过引擎原始 SetupBones。
- `Plugins/BulletPhysics/BasePhysicManager.cpp`：`CBasePhysicManager::SetupBones` 会把请求下发到 `IPhysicObject::SetupBones`。
- `Plugins/BulletPhysics/BaseRagdollObject.cpp`：当 `AnimControlFlag_OverrideAllBones` 打开时：
  - `CBaseRagdollObject::SetupBones` 遍历所有刚体组件调用 `IPhysicRigidBody::SetupBones`，最终返回 true。
- `Plugins/BulletPhysics/BulletRagdollRigidBody.cpp`：`CBulletRagdollRigidBody::SetupBones`（dynamic 分支）会：
  - 读取 `pBoneMotionState->m_bonematrix`
  - `TransformBulletToGoldSrc(...)` + `TransformToMatrix3x4(...)`
  - `memcpy((*pbonetransform)[bone], ...)` 且同步 `(*plighttransform)[bone]`
  - 标记 `Context->m_boneStates[bone] |= BoneState_BoneMatrixUpdated`
- `Plugins/BulletPhysics/BulletRagdollObject.cpp`：`CBulletRagdollObject::SetupBones` 在刚体更新“关键骨”后，会用 `m_BoneRelativeTransform`（构建时采样的相对矩阵）补齐非关键骨：
  - `merged = parent * m_BoneRelativeTransform[i]` 写回 `(*pbonetransform)[i]`

### B3) 刚体属性如何影响最终渲染 bonematrix
- **Kinematic/Dynamic 切换决定了同步方向**：
  - `Plugins/BulletPhysics/BulletRagdollRigidBody.cpp`：`CBulletRagdollRigidBody::Update(...)` 根据 activityType 与 rigidbody flags（如 `PhysicRigidBodyFlag_AlwaysKinematic/AlwaysDynamic/...InvertStateOn*`）切换 `btCollisionObject::CF_KINEMATIC_OBJECT`。
  - 切到 dynamic 时恢复质量/惯量：`setMassProps(m_mass, m_inertia)`，模拟结果会通过 `setWorldTransform` 影响 `m_bonematrix`，进而渲染骨骼姿态改变。
  - 切到 kinematic 时关闭重力/禁止休眠等：骨骼矩阵将通过 `getWorldTransform` 驱动物理姿态，物理不再反推骨骼。
- **质量/惯量/约束/外力**（`CBulletPhysicRigidBody` 的各种 `Apply*`、约束求解等）会改变动态刚体的 `worldTrans`，最终经 `CBulletBoneMotionState::setWorldTransform` 反映到 `pbonetransform`。

## C. 与 bonematrix 紧密相关的“实体原点/角度”回写（补充）
- 对非 bone-based 的刚体（entity-based motion state）：
  - `Plugins/BulletPhysics/BulletPhysicManager.cpp`：`CBulletEntityMotionState::setWorldTransform(...)` 会在刚体是动态时，枚举刚体组件并把 `GetGoldSrcOriginAngles(...)` 写回 `ent->origin/angles` 与 `ent->curstate.origin/angles`。
  - 这会影响模型整体变换（根节点），间接影响渲染骨骼最终在世界中的位置。

## 总结：最重要的时序图（脑内模型）
1) **构建/采样骨骼（矩阵→物理）**：`SetupBonesForRagdoll(STUDIO_RAGDOLL_SETUP_BONES)` → 引擎写 `pbonetransform` → `BulletCreateMotionState` 读取骨骼生成 `CBulletBoneMotionState(bone, offset)`。
2) **动态 ragdoll 渲染（物理→矩阵）**：Bullet `stepSimulation` → motionstate `setWorldTransform` 更新 `m_bonematrix` → 渲染进入 `StudioSetupBones_Template` → `SetupBones` 把 `m_bonematrix` 写回 `pbonetransform/plighttransform` → 渲染使用该矩阵。
3) **kinematic 跟随动画（矩阵→物理）**：需要更新时触发 `UpdateBonesForRagdoll(STUDIO_RAGDOLL_UPDATE_BONES)` → 引擎算骨骼 → `SetupJiggleBones` 把 `pbonetransform` 写入 `m_bonematrix` → 物理用 `getWorldTransform` 跟随骨骼。
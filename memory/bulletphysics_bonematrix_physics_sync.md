---
title: bulletphysics_bonematrix_physics_sync
type: note
permalink: metahooksv/bulletphysics-bonematrix-physics-sync
---

# BulletPhysics: Bidirectional Synchronization Between Render BoneMatrix and Bullet Rigid Bodies

> Goal: explain how the rendering-side bonematrix (`pbonetransform`) synchronizes to Bullet physics, and how Bullet physics (rigid-body state/properties) in turn affects the bonematrix used by rendering.

## Key Global Pointers/State (Bridge)
- `Plugins/BulletPhysics/exportfuncs.cpp`: retrieves and caches the following from `engine_studio_api_s` in `HUD_GetStudioModelInterface(...)`:
  - `pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();`
  - `plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();`
- `Plugins/BulletPhysics/privatehook.h/.cpp`: defines/exports `pbonetransform/plighttransform`, `pstudiohdr`, `currententity`, and so on, for use by both rendering Hooks and physics logic.
- `Plugins/BulletPhysics/exportfuncs.cpp`: sentinel variables used to determine "which entity this SetupBones belongs to":
  - `g_iRagdollRenderEntIndex`, `g_iRagdollRenderFlags`.
  - Only when `g_iRagdollRenderEntIndex > 0` will `ClientPhysicManager()->SetupBones/SetupJiggleBones` participate.

## Rendering-Side Entry Points: Hook StudioSetupBones / StudioDraw* (Driving Synchronization Timing)
### 1) StudioSetupBones Hook (Where Bone Synchronization Actually Occurs)
- `Plugins/BulletPhysics/exportfuncs.cpp`:
  - `StudioSetupBones_Template(...)` constructs a `CRagdollObjectSetupBoneContext` each time the engine/client executes `StudioSetupBones`:
    - `Context.m_studiohdr = (*pstudiohdr)`
    - `Context.m_entindex = g_iRagdollRenderEntIndex`
    - `Context.m_flags = g_iRagdollRenderFlags`
  - The call order is:
    1) If `g_iRagdollRenderEntIndex > 0 && ClientPhysicManager()->SetupBones(&Context)` returns true -> **return directly** (skip the original SetupBones).
    2) Otherwise execute the original `SetupBones` (animation calculation writes to `pbonetransform`).
    3) If `g_iRagdollRenderEntIndex > 0 && ClientPhysicManager()->SetupJiggleBones(&Context)` returns true -> return.

### 2) StudioDrawModel / StudioDrawPlayer Hook (Determines When entindex/flags Are Passed to SetupBones)
- `Plugins/BulletPhysics/exportfuncs.cpp`:
  - `StudioDrawModel_Template(...)` / `StudioDrawPlayer_Template(...)` temporarily set the following for scenarios such as ragdolls and bone updates:
    - `g_iRagdollRenderEntIndex = entindex; g_iRagdollRenderFlags = flags;`
    - Clear them after calling the original `StudioDraw*`.
  - For two special flags:
    - `STUDIO_RAGDOLL_SETUP_BONES`: directly calls the original draw and **does not set** `g_iRagdollRenderEntIndex` (letting the engine calculate bones from animation alone, for "bone sampling").
    - `STUDIO_RAGDOLL_UPDATE_BONES`: sets `g_iRagdollRenderEntIndex/Flags`, but calls the original draw with flags=0 (for "updating bones -> synchronizing to physics", without taking the normal rendering path).
  - Flag definitions: `Plugins/BulletPhysics/enginedef.h` (`STUDIO_RAGDOLL_SETUP_BONES 0x10`, `STUDIO_RAGDOLL_UPDATE_BONES 0x20`).

## A. Render BoneMatrix -> Physics (Matrix -> Physics)
This path has two primary categories: **construction-time sampling** and **runtime updates (kinematic following of bones)**.

### A1) At Construction Time: Initialize Bullet's Bone-Based MotionState from Animation Bones
- `Plugins/BulletPhysics/BaseRagdollObject.cpp`: `CBaseRagdollObject::Build(...)` (studio model) first calls:
  - `ClientPhysicManager()->SetupBonesForRagdoll(...)` or `SetupBonesForRagdollEx(...)` (Idle anim override).
- `Plugins/BulletPhysics/BasePhysicManager.cpp`: `CBasePhysicManager::SetupBonesForRagdoll*` internally calls:
  - `(*gpStudioInterface)->StudioDrawModel(STUDIO_RAGDOLL_SETUP_BONES)` or `StudioDrawPlayer(STUDIO_RAGDOLL_SETUP_BONES, ...)`.
- Because the `STUDIO_RAGDOLL_SETUP_BONES` branch does not set `g_iRagdollRenderEntIndex`, `StudioSetupBones_Template` does not call physics-side `SetupBones/SetupJiggleBones`:
  - **End result**: the engine calculates `(*pbonetransform)[i]` from animation, as a "current-pose bone snapshot".
- Subsequently, when creating rigid bodies:
  - `Plugins/BulletPhysics/BulletPhysicManager.cpp`: `BulletCreateMotionState(...)` reads `(*pbonetransform)[boneindex]` to generate `btTransform bonematrix` (and `TransformGoldSrcToBullet`), then uses configuration to generate `localTrans` (offset matrix/local pose), and returns:
    - `new CBulletBoneMotionState(pPhysicObject, bonematrix, localTrans)`
  - `Plugins/BulletPhysics/BulletPhysicManager.h`: `CBulletBoneMotionState::getWorldTransform` always uses:
    - `worldTrans = m_bonematrix * m_offsetmatrix`
  - `Plugins/BulletPhysics/BulletPhysicRigidBody.cpp`: after `CBulletPhysicRigidBody` constructs `btRigidBody`, it binds the motionstate back to the internal rigid body:
    - `pMotionState->SetInternalRigidBody(m_pInternalRigidBody);`

### A2) At Runtime: Update Kinematic Rigid Bodies (Bones Drive Rigid Bodies)
- `Plugins/BulletPhysics/BaseRagdollObject.cpp`: in `CBaseRagdollObject::Update(...)`, when the object is not visible or conditions such as `bv_force_updatebones` are satisfied, it sets:
  - `ObjectUpdateContext->m_bRigidbodyUpdateBonesRequired = true`
  - Then calls `UpdateBones(playerState)`.
- `UpdateBones` implementation: `CBaseRagdollObject::UpdateBones` -> `ClientPhysicManager()->UpdateBonesForRagdoll(...)`.
- `Plugins/BulletPhysics/BasePhysicManager.cpp`: `CBasePhysicManager::UpdateBonesForRagdoll(...)` triggers:
  - `StudioDrawModel(STUDIO_RAGDOLL_UPDATE_BONES)` / `StudioDrawPlayer(STUDIO_RAGDOLL_UPDATE_BONES, ...)`.
- The `STUDIO_RAGDOLL_UPDATE_BONES` branch sets `g_iRagdollRenderEntIndex/Flags`, so `StudioSetupBones_Template` performs the following for that entity:
  1) It first attempts `ClientPhysicManager()->SetupBones` (in most non-OverrideAllBones scenarios it returns false, allowing the engine to calculate bones).
  2) After the engine calculates bones, it enters `ClientPhysicManager()->SetupJiggleBones`.
- For bone-based rigid bodies, the `SetupJiggleBones` kinematic branches in `Plugins/BulletPhysics/BulletRagdollRigidBody.cpp` and `BulletDynamicRigidBody.cpp` perform:
  - `Matrix3x4ToTransform((*pbonetransform)[bone], pBoneMotionState->m_bonematrix)`
  - `TransformGoldSrcToBullet(pBoneMotionState->m_bonematrix)`
  - This is equivalent to writing the "render bone matrix" into Bullet's bone motion state.
- Subsequently, Bullet's per-frame pose reads for kinematic rigid bodies use the latest `m_bonematrix` through `CBulletBoneMotionState::getWorldTransform`, thereby implementing **bones -> rigid bodies**.

## B. Physics (Rigid Bodies) -> Render BoneMatrix (Physics -> Matrix)
The core of this path is: **Bullet updates the rigid body's world transform -> motionstate reversely derives the bone matrix -> writes it back to `pbonetransform` during rendering**.

### B1) Bullet Writes Simulation Results to CBulletBoneMotionState::m_bonematrix
- `Plugins/BulletPhysics/BulletPhysicManager.h`:
  - `CBulletBoneMotionState::setWorldTransform(worldTrans)`:
    - `m_bonematrix = worldTrans * inverse(m_offsetmatrix)`
  - For dynamic rigid bodies, Bullet calls the motionstate's `setWorldTransform` after `stepSimulation` to synchronize results.

### B2) During Rendering/Bone Calculation: Write m_bonematrix Back to pbonetransform (for Rendering)
- `Plugins/BulletPhysics/exportfuncs.cpp`: after the rendering path enters `StudioSetupBones_Template`, if the physics side decides "physics takes ownership of bones for this pass", it calls `ClientPhysicManager()->SetupBones` and returns true -> directly skips the engine's original SetupBones.
- `Plugins/BulletPhysics/BasePhysicManager.cpp`: `CBasePhysicManager::SetupBones` dispatches the request to `IPhysicObject::SetupBones`.
- `Plugins/BulletPhysics/BaseRagdollObject.cpp`: when `AnimControlFlag_OverrideAllBones` is enabled:
  - `CBaseRagdollObject::SetupBones` iterates over all rigid-body components and calls `IPhysicRigidBody::SetupBones`, ultimately returning true.
- `Plugins/BulletPhysics/BulletRagdollRigidBody.cpp`: `CBulletRagdollRigidBody::SetupBones` (dynamic branch) does the following:
  - Reads `pBoneMotionState->m_bonematrix`
  - `TransformBulletToGoldSrc(...)` + `TransformToMatrix3x4(...)`
  - `memcpy((*pbonetransform)[bone], ...)` and synchronizes `(*plighttransform)[bone]`
  - Marks `Context->m_boneStates[bone] |= BoneState_BoneMatrixUpdated`
- `Plugins/BulletPhysics/BulletRagdollObject.cpp`: after rigid bodies update "key bones", `CBulletRagdollObject::SetupBones` uses `m_BoneRelativeTransform` (the relative matrices sampled at construction) to fill in non-key bones:
  - `merged = parent * m_BoneRelativeTransform[i]` is written back to `(*pbonetransform)[i]`

### B3) How Rigid-Body Properties Affect the Final Rendered bonematrix
- **The Kinematic/Dynamic switch determines the synchronization direction**:
  - `Plugins/BulletPhysics/BulletRagdollRigidBody.cpp`: `CBulletRagdollRigidBody::Update(...)` switches `btCollisionObject::CF_KINEMATIC_OBJECT` based on activityType and rigidbody flags (such as `PhysicRigidBodyFlag_AlwaysKinematic/AlwaysDynamic/...InvertStateOn*`).
  - When switching to dynamic, it restores mass/inertia: `setMassProps(m_mass, m_inertia)`; simulation results affect `m_bonematrix` through `setWorldTransform`, which in turn changes the rendered bone pose.
  - When switching to kinematic, it disables gravity/prevents sleeping and so on: bone matrices drive the physics pose through `getWorldTransform`, and physics no longer reversely derives bones.
- **Mass/inertia/constraints/external forces** (the various `Apply*` methods of `CBulletPhysicRigidBody`, constraint solving, and so on) change a dynamic rigid body's `worldTrans`, ultimately reflecting in `pbonetransform` through `CBulletBoneMotionState::setWorldTransform`.

## C. Write-Back of Entity Origin/Angles Closely Related to bonematrix (Supplement)
- For non-bone-based rigid bodies (entity-based motion states):
  - `Plugins/BulletPhysics/BulletPhysicManager.cpp`: when a rigid body is dynamic, `CBulletEntityMotionState::setWorldTransform(...)` enumerates rigid-body components and writes `GetGoldSrcOriginAngles(...)` back to `ent->origin/angles` and `ent->curstate.origin/angles`.
  - This affects the model's overall transform (root node), indirectly affecting the final world-space positions of rendering bones.

## Summary: The Most Important Timing Diagram (Mental Model)
1) **Construction/bone sampling (matrix -> physics)**: `SetupBonesForRagdoll(STUDIO_RAGDOLL_SETUP_BONES)` -> engine writes `pbonetransform` -> `BulletCreateMotionState` reads bones to create `CBulletBoneMotionState(bone, offset)`.
2) **Dynamic ragdoll rendering (physics -> matrix)**: Bullet `stepSimulation` -> motionstate `setWorldTransform` updates `m_bonematrix` -> rendering enters `StudioSetupBones_Template` -> `SetupBones` writes `m_bonematrix` back to `pbonetransform/plighttransform` -> rendering uses the matrix.
3) **Kinematic following of animation (matrix -> physics)**: triggers `UpdateBonesForRagdoll(STUDIO_RAGDOLL_UPDATE_BONES)` when an update is needed -> engine calculates bones -> `SetupJiggleBones` writes `pbonetransform` into `m_bonematrix` -> physics follows bones using `getWorldTransform`.

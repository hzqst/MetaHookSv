---
title: bulletphysics_physics_config
type: note
permalink: metahooksv/bulletphysics-physics-config
---

# BulletPhysics Physics Configuration

> Goal: describe the complete **physics-configuration loading / saving / usage** flow in the BulletPhysics plugin (configuration file → in-memory structure → physics-object construction/reconstruction → persistence).

## 1) Configuration Unit and Storage Location

- **Configuration unit**: scoped to `model_t` (models, brushmodels, world models, and so on); each model maps to one `CClientPhysicObjectConfig` (static, dynamic, or ragdoll).
- **Storage container**: `CBasePhysicManager::m_physicObjectConfigs` (`CClientPhysicObjectConfigStorage` indexed by `modelindex`).
  - `Storage.state`: `PhysicConfigState_NotLoaded / Loaded / LoadedWithError`
  - `Storage.modelname`: stores the model name (used to construct the filename when saving)
  - `Storage.pConfig`: the configuration object (`shared_ptr`)
- **Configuration substructures**:
  - `CClientPhysicObjectConfig` contains `RigidBodyConfigs / ConstraintConfigs / PhysicBehaviorConfigs`; ragdolls additionally contain `AnimControlConfigs`.
  - Every subconfiguration (rigid body, constraint, behavior, animation control, collision shape) has a `configId` and is registered in the global configuration table through `ClientPhysicManager()->AddPhysicConfig(configId, ptr)` for convenient UI/runtime references.

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

## 2) Load Triggers

### 2.1 Map-Change Loading
- `CBasePhysicManager::NewMap()`:
  - Removes old physics objects: `RemoveAllPhysicObjects(...)`
  - Removes **BSP-generated** configurations: `RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromBSP, 0)`
  - Generates and caches collision index data (BSP meshes) for brushes
  - Calls `LoadPhysicObjectConfigs()` to preload configurations for known models
  - Creates the physics object for the world brush: `CreatePhysicObjectForBrushModel(..., *cl_worldmodel)`

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

### 2.2 Runtime On-Demand Loading
- When a physics object is needed for an entity, `LoadPhysicObjectConfigForModel(mod)` is called. If that `modelindex` is still `NotLoaded`, this triggers the actual load from a file or BSP.

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

## 3) Load Paths and Priority

### 3.1 Studio Models: File Loading (New Format First)
- Entry point: `CBasePhysicManager::LoadPhysicObjectConfigFromFiles(model_t *mod, Storage)`
- Filename derivation:
  1) `modelname = mod->name`, with its extension removed (for example, `.mdl`)
  2) First attempts: `<modelname>_physics.txt` (**new format**)
  3) Then attempts: `<modelname>_ragdoll.txt` (**old format**/legacy)
- On success: `OverwritePhysicObjectConfig(modelname, Storage, pConfig)`
  - `Storage.state = Loaded`
  - `pConfig->modelName = modelname`
  - `pConfig->shortName = V_FileBase(modelname)`

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

### 3.2 Brush Models: Generate Configuration from BSP/Resources
- Entry point: `CBasePhysicManager::LoadPhysicObjectConfigFromBSP(mod, Storage)`
- Logic:
  - Generates or loads triangle-mesh index data from the brush resource
  - Constructs a `CClientCollisionShapeConfig` (`PhysicShape_TriangleMesh`, with `resourcePath` pointing to the brush resource)
  - Constructs a `CClientRigidBodyConfig` (`mass=0`, pointing to the collision shape above)
  - Constructs a `CClientStaticObjectConfig` and sets `flags |= PhysicObjectFlag_FromBSP`
  - `OverwritePhysicObjectConfig(resourcePath, Storage, pStaticObjectConfig)`
- Note: this **BSP-generated config** is not a file configuration and is not persisted by default.

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

## 4) New-Format File (KeyValues) Structure (Consistent for Reading/Writing)

### 4.1 Reading
- `LoadPhysicObjectConfigFromNewFile(mod, filename)`:
  - `KeyValues("PhysicObjectConfig")` + `LoadFromFile(g_pFileSystem[_HL25], filename)`
  - Passes it to `LoadPhysicObjectConfigFromKeyValues(mod, pKeyValues)`
- `LoadPhysicObjectConfigFromKeyValues` dispatches based on the `type` field:
  - `"RagdollObject" / "StaticObject" / "DynamicObject"`
- `LoadPhysicObjectFlagsFromKeyValues`: unconditionally sets `flags |= PhysicObjectFlag_FromConfig` and reads:
  - `barnacle` / `gargantua` / `overrideStudioCheckBBOX`
- Integrity validation: `verifyBoneChunk`/`verifyModelFile` plus the corresponding `crc32...`; returns `nullptr` on failure.

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

### 4.2 Writing (Serialization)
- `SavePhysicObjectConfigToNewFile(filename, config)`:
  - `ConvertPhysicObjectConfigToKeyValues(config)` (the root node is likewise `KeyValues("PhysicObjectConfig")`)
  - `KeyValues::SaveToFile(...)`
- `AddBaseConfigToKeyValues` writes:
  - `type` (produced by `UTIL_GetPhysicObjectConfigTypeName`)
  - `barnacle/gargantua/overrideStudioCheckBBOX` (flags → KV)
- `AddVerifyStuffsFromKeyValues` writes:
  - `verifyBoneChunk/verifyModelFile` and `crc32BoneChunk/crc32ModelFile`

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

### 4.3 Key `collisionShape` Fields
- Reading: `LoadCollisionShapeFromKeyValues` supports the following fields:
  - `type`, `direction`, `origin`, `angles`, `size`, `resourcePath`, `compoundShapes`
- Writing: `AddCollisionShapeToKeyValues` writes the corresponding fields with the same names (some are written only when non-default).

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

## 5) Old-Format Files (Legacy `_ragdoll.txt`)

- `LoadPhysicObjectConfigFromLegacyFile(filename)`: reads the entire file with `COM_LoadFile`, then calls `LoadPhysicObjectConfigFromLegacyFileBuffer(buf)`
- Legacy is a section-based text format (such as `[RigidBody]` and `[Constraint]`) parsed line by line.
- The legacy result is a `CClientRagdollObjectConfig`, with the following flags set explicitly:
  - `flags |= PhysicObjectFlag_FromConfig`
  - `flags |= PhysicObjectFlag_OverrideStudioCheckBBox`

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`

## 6) Save Triggers and Persistence Rules

### 6.1 Entry Points
- Console command: `bv_save_configs` → `BV_SaveConfigs_f()`
- Debug UI: `CPhysicDebugGUI::SaveOpenPrompt()` → `SaveConfirm()` → `BV_SaveConfigs_f()`
- Both entry points first check `AllowCheats()` (typically governed by `sv_cheats` and similar constraints).

Related implementation: `Plugins/BulletPhysics/exportfuncs.cpp`, `Plugins/BulletPhysics/PhysicDebugGUI.cpp`

### 6.2 Save Content and Conditions
- `CBasePhysicManager::SavePhysicObjectConfigs()` iterates over `EngineGetNumKnownModel()`:
  - Attempts to save only configurations for **loaded** studio models
  - Calls `SavePhysicObjectConfigToFile(mod->name, config)`
- `SavePhysicObjectConfigToFile` has two strict prerequisites:
  - `flags & PhysicObjectFlag_FromConfig` must be set (the configuration originated from a file or was marked as a file configuration by the UI)
  - `UTIL_IsPhysicObjectConfigModified(...) == true` must hold
- Filename: `<mod->name without extension>_physics.txt` (writes only the new format; never writes back legacy)
- After a successful write, `UTIL_SetPhysicObjectConfigUnmodified` recursively clears `configModified` on the object and its subconfigurations.
- Write path: first tries the `GAMEDOWNLOAD` directory (creating it as needed), then falls back to the default path on failure.

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`, `Plugins/BulletPhysics/PhysicUTIL.cpp`

## 7) Reload and Creating New Configurations in the Editor

### 7.1 Reloading Configurations
- Console command: `bv_reload_configs` → `BV_ReloadConfigs_f()`:
  - `FreeAllIndexArrays(PhysicIndexArrayFlag_FromExternal, PhysicIndexArrayFlag_FromBSP)` (removes external meshes while retaining BSP meshes)
  - `RemoveAllPhysicObjectConfigs(PhysicObjectFlag_FromConfig, 0)` (removes file configurations)
  - Reloads via `LoadPhysicObjectConfigs()`

Related implementation: `Plugins/BulletPhysics/exportfuncs.cpp`

### 7.2 Debug UI: Create an Empty Configuration (to Make It Saveable)
- `CPhysicDebugGUI::OnCreateStaticObject/DynamicObject/RagdollObject`:
  - If the `modelindex` has no configuration yet, calls `CreateEmptyPhysicObjectConfigForModelIndex(modelindex, type)`
  - Then explicitly sets `pConfig->flags |= PhysicObjectFlag_FromConfig` so a later `bv_save_configs` can persist it.

Related implementation: `Plugins/BulletPhysics/PhysicDebugGUI.cpp`

## 8) How Configurations Are Used at Runtime

- Entity flow: `CreatePhysicObjectForEntity` → `CreatePhysicObjectForStudioModel/BrushModel` → `CreatePhysicObjectFromConfig`.
- `CreatePhysicObjectFromConfig`:
  1) Obtains the configuration via `LoadPhysicObjectConfigForModel(mod)`
  2) `LoadAdditionalResourcesForConfig(config)`: triggers mesh/index-array cache loading for `collisionShape.resourcePath`
  3) Creates the matching physics object (Ragdoll/Dynamic/Static) according to `config->type`, then calls `Build(CreationParam)`
- Applying editor changes: the common path is `ClientPhysicManager()->RebuildPhysicObjectEx2(pPhysicObject, pPhysicObjectConfig)`, which feeds the new configuration to the object's `Rebuild()`.

Related implementation: `Plugins/BulletPhysics/BasePhysicManager.cpp`


## KeyValues Configuration Format (`*_physics.txt`, New Format)

> This format is read and written by `Plugins/BulletPhysics/BasePhysicManager.cpp` (`KeyValues("PhysicObjectConfig")`) and describes physics-object configurations for **Studio models** (`.mdl`).
>
> Filename derivation: `<modelname without extension>_physics.txt` (new format, preferred); the old format is `<modelname without extension>_ragdoll.txt` (legacy).

### 1) Top-Level Structure (Overall Example)

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

### 2) Top-Level Parameter Reference (Direct Children of `"PhysicObjectConfig"`)

- `type`: physics-object type string; determines which groups are subsequently read.
  - `StaticObject`: reads only `rigidBodies`
  - `DynamicObject`: reads `rigidBodies` + `constraints`
  - `RagdollObject`: reads `rigidBodies` + `constraints` + `physicBehaviors` + `animControls`
- `barnacle`: marks the physics object as Barnacle-related (`PhysicObjectFlag_Barnacle`).
- `gargantua`: marks the physics object as Gargantua-related (`PhysicObjectFlag_Gargantua`).
- `overrideStudioCheckBBOX`: enables the flag that overrides checks such as `StudioCheckBBox` for Studio models (`PhysicObjectFlag_OverrideStudioCheckBBox`).
- `verifyBoneChunk`: whether to validate the integrity of the model bone chunk (through `VerifyIntegrityForPhysicObjectConfig`).
- `crc32BoneChunk`: CRC32 string for the bone chunk (used for validation).
- `verifyModelFile`: whether to validate the integrity of the entire model file (through `VerifyIntegrityForPhysicObjectConfig`).
- `crc32ModelFile`: CRC32 string for the model file (used for validation).
- `rigidBodies`: rigid-body configuration table (the **name** of each child key is the rigid-body name, referenced by constraints/behaviors).
- `constraints`: constraint configuration table (the **name** of each child key is the constraint name, referenced by behaviors).
- `physicBehaviors`: behavior configuration table (the **name** of each child key is the behavior name).
- `animControls`: animation-control configuration list (child-key names are insignificant and are not used when reading).

### 3) `rigidBodies` (Rigid Body) Parameter Reference

The name of each child key under `rigidBodies` becomes `CClientRigidBodyConfig::name`; fields are as follows:

- State/collision flags (bool; active when present and true):
  - `alwaysDynamic` / `alwaysKinematic` / `alwaysStatic`
  - `invertStateOnIdle` / `invertStateOnDeath`
  - `invertStateOnCaughtByBarnacle` / `invertStateOnBarnaclePulling` / `invertStateOnBarnacleChewing`
  - `invertStateOnGargantuaBite`
  - `noCollisionToWorld` / `noCollisionToStaticObject` / `noCollisionToDynamicObject` / `noCollisionToRagdollObject`
- `debugDrawLevel`: debug drawing level (used for DebugDraw filtering).
- `boneindex`: index of the bone bound to this rigid body; `-1` means unbound.
- `origin`: local offset (the string vector `"x y z"`), parsed as `vec3_t`.
- `angles`: local Euler rotation (the string vector `"pitch yaw roll"`).
- `forward`: a forward vector (the string vector `"x y z"`) used for certain constraint/orientation calculations.
- `isLegacyConfig`: indicates whether the rigid body was migrated from or is compatible with the legacy format (0/1).
- `pboneindex` / `pboneoffset`: parent-bone information and offset for legacy compatibility.
- Physics parameters (float):
  - `mass`: mass (default `BULLET_DEFAULT_MASS`).
  - `density`: density (default `BULLET_DEFAULT_DENSENTY`).
  - `linearFriction`: linear friction (default `BULLET_DEFAULT_LINEAR_FRICTION`).
  - `rollingFriction`: rolling/angular friction (default `BULLET_DEFAULT_ANGULAR_FRICTION`).
  - `restitution`: restitution coefficient (default `BULLET_DEFAULT_RESTITUTION`).
  - `ccdRadius`: CCD radius (default 0).
  - `ccdThreshold`: CCD threshold (default `BULLET_DEFAULT_CCD_THRESHOLD`).
  - `linearSleepingThreshold` / `angularSleepingThreshold`: sleeping thresholds (default `BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD / BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD`).
  - `additionalDampingFactor` / `additionalLinearDampingThresholdSqr` / `additionalAngularDampingThresholdSqr`: additional damping parameters (corresponding to Bullet's additional-damping settings).
- `collisionShape`: collision-shape substructure (see the next section).

### 4) `collisionShape` (Collision Shape) Parameter Reference

`collisionShape` maps to `CClientCollisionShapeConfig` and supports these fields:

- `type`: shape-type string (`None/Box/Sphere/Capsule/Cylinder/MultiSphere/TriangleMesh/Compound`).
- `direction`: shape primary-axis direction (`0=X, 1=Y, 2=Z`; default `1`).
- `origin`: local offset (the string vector `"x y z"`).
- `angles`: local rotation (the string vector `"x y z"`).
- `size`: dimensions (string vector; supports 1/2/3 components, parsed in vec3→vec2→vec1 order).
- `resourcePath`: external resource path (primarily for shapes such as `TriangleMesh` that load meshes from resources).
- `compoundShapes`: meaningful only for `Compound`; a list of child shapes (each child key is still a `collisionShape` structure).

### 5) `constraints` (Constraint) Parameter Reference

The name of each child key under `constraints` becomes `CClientConstraintConfig::name`; common fields are:

- `type`: constraint-type string (`None/ConeTwist/Hinge/Point/Slider/Dof6/Dof6Spring/Fixed`).
- `rigidbodyA` / `rigidbodyB`: referenced rigid-body names (must match child-key names under `rigidBodies`).
- `originA` / `anglesA` / `originB` / `anglesB`: local-frame information (the string vector `"x y z"`).
- `forward`: auxiliary vector (the string vector `"x y z"`).
- Constraint flags (bool; active when present and true):
  - `barnacle` / `gargantua`
  - `deactiveOnNormalActivity` / `deactiveOnDeathActivity`
  - `deactiveOnCaughtByBarnacleActivity` / `deactiveOnBarnaclePullingActivity` / `deactiveOnBarnacleChewingActivity`
  - `deactiveOnGargantuaBiteActivity`
  - `dontResetPoseOnErrorCorrection`
  - `DeferredCreate`
- Boolean configuration (bool; the default is used when omitted):
  - `disableCollision` (default true)
  - `useGlobalJointFromA` (default true)
  - `useLinearReferenceFrameA` (default true)
  - `useLookAtOther` (default false)
  - `useGlobalJointOriginFromOther` (default false)
  - `useRigidBodyDistanceAsLinearLimit` (default false)
  - `useSeperateLocalFrame` (default false)
- `debugDrawLevel`: debug drawing level (default `BULLET_DEFAULT_DEBUG_DRAW_LEVEL`).
- `maxTolerantLinearError`: maximum tolerable linear error (default `BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR`).
- `isLegacyConfig`: legacy-compatibility flag (default false).
- `boneindexA` / `boneindexB`: legacy-compatible bone indices (default `-1`).
- `offsetA` / `offsetB`: legacy-compatible offsets (the string vector `"x y z"`).

`constraints/"<name>"/factors`: constraint parameter table (float; only entries relevant to the current `type` take effect; omitted values are internally represented as `NAN`, meaning “not provided”).

- `ConeTwist`: `ConeTwistSwingSpanLimit1/ConeTwistSwingSpanLimit2/ConeTwistTwistSpanLimit/ConeTwistSoftness/ConeTwistBiasFactor/ConeTwistRelaxationFactor/LinearERP/LinearCFM/AngularERP/AngularCFM`
- `Hinge`: `HingeLowLimit/HingeHighLimit/HingeSoftness/HingeBiasFactor/HingeRelaxationFactor/AngularERP/AngularCFM/AngularStopERP/AngularStopCFM`
- `Point`: `AngularERP/AngularCFM`
- `Slider`: `SliderLowerLinearLimit/SliderUpperLinearLimit/SliderLowerAngularLimit/SliderUpperAngularLimit/LinearCFM/LinearStopERP/LinearStopCFM/AngularCFM/AngularStopERP/AngularStopCFM`
- `Dof6`: `Dof6LowerLinearLimitX/Y/Z/Dof6UpperLinearLimitX/Y/Z/Dof6LowerAngularLimitX/Y/Z/Dof6UpperAngularLimitX/Y/Z/LinearCFM/LinearStopERP/LinearStopCFM/AngularCFM/AngularStopERP/AngularStopCFM`
- `Dof6Spring`: adds the following on top of `Dof6`
  - `Dof6SpringEnableLinearSpringX/Y/Z`, `Dof6SpringEnableAngularSpringX/Y/Z`
  - `Dof6SpringLinearStiffnessX/Y/Z`, `Dof6SpringAngularStiffnessX/Y/Z`
  - `Dof6SpringLinearDampingX/Y/Z`, `Dof6SpringAngularDampingX/Y/Z`
- `Fixed`: `LinearCFM/LinearStopERP/LinearStopCFM/AngularCFM/AngularStopERP/AngularStopCFM`

### 6) `physicBehaviors` (Behavior) Parameter Reference

The name of each child key under `physicBehaviors` becomes `CClientPhysicBehaviorConfig::name`; common fields are:

- `type`: behavior-type string:
  - `None/BarnacleDragOnRigidBody/BarnacleDragOnConstraint/BarnacleChew/BarnacleConstraintLimitAdjustment/GargantuaDragOnConstraint/FirstPersonViewCamera/ThirdPersonViewCamera/SimpleBuoyancy/RigidBodyRelocation`
- `rigidbodyA` / `rigidbodyB`: referenced rigid-body names (whether used depends on the behavior type).
- `constraint`: referenced constraint name (whether used depends on the behavior type).
- `barnacle` / `gargantua`: behavior flags (bool).
- `origin` / `angles`: local pose of the behavior (the string vector `"x y z"`).

`physicBehaviors/"<name>"/factors`: behavior parameter table (float; only entries relevant to the current `type` take effect; omitted values are internally represented as `NAN`, meaning “not provided”).

- `BarnacleDragOnRigidBody`: `BarnacleDragMagnitude/BarnacleDragExtraHeight`
- `BarnacleDragOnConstraint`: `BarnacleDragMagnitude/BarnacleDragVelocity/BarnacleDragExtraHeight/BarnacleDragLimitAxis/BarnacleDragCalculateLimitFromActualPlayerOrigin/BarnacleDragUseServoMotor/BarnacleDragActivatedOnBarnaclePulling/BarnacleDragActivatedOnBarnacleChewing`
- `BarnacleChew`: `BarnacleChewMagnitude/BarnacleChewInterval`
- `BarnacleConstraintLimitAdjustment`: `BarnacleConstraintLimitAdjustmentExtraHeight/BarnacleConstraintLimitAdjustmentInterval/BarnacleConstraintLimitAdjustmentAxis`
- `GargantuaDragOnConstraint`: `BarnacleDragMagnitude/BarnacleDragVelocity/BarnacleDragExtraHeight/BarnacleDragLimitAxis/BarnacleDragUseServoMotor`
- `FirstPersonViewCamera` / `ThirdPersonViewCamera`: `CameraActivateOnIdle/CameraActivateOnDeath/CameraActivateOnCaughtByBarnacle/CameraSyncViewOrigin/CameraSyncViewAngles/CameraUseSimOrigin/CameraOriginalViewHeightStand/CameraOriginalViewHeightDuck/CameraMappedViewHeightStand/CameraMappedViewHeightDuck/CameraNewViewHeightDucking`
- `SimpleBuoyancy`: `SimpleBuoyancyMagnitude/SimpleBuoyancyLinearDamping/SimpleBuoyancyAngularDamping`

### 7) `animControls` (Animation Control) Parameter Reference (Used Only by `RagdollObject`)

`animControls` is a “list” (child-key names are insignificant); each item has these fields:

- `sequence`: animation sequence number (default -1).
- `gaitsequence`: gait sequence number (default -1).
- `animframe`: animation frame (float, default 0).
- `activityType`: activity type (int).
- `flags`: flag bits (int).
- `controller_0..3`: controller values (int, default -1).
- `blending_0..3`: blending values (int, default -1).

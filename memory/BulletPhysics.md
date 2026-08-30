---
title: BulletPhysics
type: note
permalink: metahooksv/bullet-physics
---

﻿# BulletPhysics

## Overview
BulletPhysics is MetaHookSv's client-side physics plugin. It maps GoldSrc/SvEngine entities, Studio models, and the BSP world into a Bullet3 physics world, providing construction, updates, render synchronization, debug drawing, and configuration editing for static objects, dynamic objects, and ragdolls. The plugin integrates with the game runtime through MetaHook's plugin interfaces, client export replacement, engine/Studio inline hooks, and VGUI2Extension.

## Responsibilities
- Initializes/destroys the physics manager during the IPluginsV4 lifecycle, retaining engine, client, filesystem, and MetaHook contexts.
- Enumerates players, network entities, and temporary entities active in the current frame, creating or removing physics objects according to model configuration.
- Maintains physics configurations indexed by modelindex and runtime objects, rigid bodies, constraints, and behaviors indexed by entity/component ID.
- Manages the Bullet dynamics world, collision filtering, gravity, fixed-timestep simulation, ray testing, and debug drawing.
- Builds physics configurations as StaticObject, DynamicObject, or RagdollObject, assembling components in rigid body → constraint → behavior order.
- Performs bidirectional synchronization between animated bones and Bullet rigid bodies through StudioSetupBones, StudioDrawModel/Player, and StudioCheckBBox hooks.
- Provides bv_* console commands, a VGUI2Extension debug window, object/component inspect-and-select functionality, and configuration reload/save capabilities.

## Involved Files & Symbols
- Plugins/BulletPhysics/plugins.cpp - IPluginsV4::Init, LoadEngine, LoadClient, ExitGame; plugin lifecycle and hook installation order.
- Plugins/BulletPhysics/exportfuncs.cpp - HUD_Init, HUD_GetStudioModelInterface, HUD_CreateEntities, HUD_TempEntUpdate, V_CalcRefdef, HUD_DrawTransparentTriangles, HUD_Shutdown.
- Plugins/BulletPhysics/privatehook.cpp/.h - engine/client address resolution; inline/vtable hooks for R_NewMap, R_RenderView, Studio rendering, and StudioSetupBones.
- Plugins/BulletPhysics/ClientPhysicManager.h - IClientPhysicManager, IPhysicObject, IPhysicComponent, rigid-body/constraint/behavior interfaces, and frame context.
- Plugins/BulletPhysics/BasePhysicManager.h/.cpp - CBasePhysicManager; objects, components, configurations, resource caches, map switching, and common build/update flow.
- Plugins/BulletPhysics/ClientPhysicConfig.h - CClientPhysicObjectConfig, rigid-body/constraint/behavior/animation-control/collision-shape configurations, and modelindex storage.
- Plugins/BulletPhysics/BulletPhysicManager.h/.cpp - CBulletPhysicManager; Bullet world, motion state, collision filtering, stepping, ray testing, and backend object factories.
- Plugins/BulletPhysics/BaseStaticObject.*, BaseDynamicObject.*, BaseRagdollObject.* - backend-agnostic object lifecycles, component containers, animation activity state, and camera synchronization.
- Plugins/BulletPhysics/BulletStaticObject.*, BulletDynamicObject.*, BulletRagdollObject.* - Bullet backend object implementations; create Bullet rigid bodies, constraints, and behaviors.
- Plugins/BulletPhysics/BulletPhysicRigidBody.*, BulletPhysicConstraint.*, BulletPhysicComponentBehavior.* - Bullet component adapter layer; wraps btRigidBody, btTypedConstraint, motion state, and behaviors.
- Plugins/BulletPhysics/ClientEntityManager.* - entity types/indices, player-death state, model scaling, current-frame emitted/visible state, and entity-model mapping.
- Plugins/BulletPhysics/VGUI2ExtensionImport.*, Viewport.*, PhysicDebugGUI.*, PhysicEditorDialog.* - VGUI2Extension interfaces, debug viewport, inspect/select, and configuration editing UI.
- Plugins/BulletPhysics/PhysicUTIL.* - configuration serialization, model-integrity validation, type/factor conversion, and physics utility functions.
- Plugins/BulletPhysics/BulletPhysics.vcxproj - Win32 DLL build, C++20, dependencies such as Bullet3/GLEW/Capstone/tinyobjloader, and prebuild checks.
- docs/BulletPhysics.md, docs/BulletPhysicsCN.md - functionality, engine compatibility, and legacy ragdoll configuration documentation; Build/svencoop/bulletphysics/* - UI/localization runtime resources.

## Architecture
BulletPhysics uses a layered structure: “MetaHook integration layer → common physics-management layer → Bullet backend → object/component layer.” Configurations and runtime objects are linked through the manager, while rendering hooks and physics stepping converge in the client main loop:

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

### 1. Plugin Integration and Lifecycle
- IPluginsV4::Init only retains metahook_api_t, mh_interface_t, and mh_enginesave_t.
- LoadEngine acquires FileSystem, engine type/build number, engine and mirror-engine segments; registers DLL load notifications; executes Engine_FillAddress/Engine_InstallHook; initializes VGUI2Extension and BaseUI/GameUI/ClientVGUI hooks; creates g_pClientPhysicManager = BulletPhysicManager_CreateInstance; and finally initializes GLEW.
- LoadClient preserves the original gExportfuncs, replaces HUD_Init, HUD_GetStudioModelInterface, HUD_CreateEntities, HUD_TempEntUpdate, HUD_AddEntity, HUD_DrawTransparentTriangles, HUD_Frame, HUD_Shutdown, V_CalcRefdef, and HUD_PostRunCmd, then performs client address resolution and hook installation.
- HUD_Init calls ClientPhysicManager()->Init after the original initialization, registers cvars such as bv_debug_draw*, bv_simrate, bv_syncview, and bv_force_updatebones, plus bv_open_debug_ui, bv_reload_*, and bv_save_configs commands, and installs ClCorpse/temporary-model hooks.
- HUD_Shutdown first calls the original function, then shuts down the physics manager, Studio hooks, and temporary-model hooks; ExitGame destroys the manager and unloads UI/engine hooks.
- R_NewMap calls NewMap on the physics manager, entity manager, and debug viewport after the original map change completes.

### 2. Common Manager and Data Model
- IClientPhysicManager defines public contracts for lifecycle, configuration management, physics-object management, component management, world operations, bone bridging, Debug/Inspect/Select, and resource caches.
- CBasePhysicManager holds m_physicObjects, m_physicComponents, m_physicConfigs, m_physicObjectConfigs, external OBJ/BSP index caches, and debug-selection state. Object IDs use PACK_PHYSIC_OBJECT_ID(entindex, modelindex) to avoid retrieving old objects when entity numbers are reused.
- NewMap clears old physics objects, BSP-generated configurations, and BSP index caches; resets component/inspect selection IDs; regenerates brush vertex/index caches; loads known model configurations; and creates the static physics object for world brushes.
- Configurations are lazily loaded by modelindex: Studio models first read <model>_physics.txt and then support <model>_ragdoll.txt; brush models generate triangle-mesh static configurations from BSP. Configuration objects include rigid bodies, constraints, and behaviors; ragdolls additionally include animation control.
- CreatePhysicObjectForEntity dispatches Studio/brush creation based on model and entity type. Players, dead players, CS/CS:CZ corpses, and temporary entities use ClientEntityManager to resolve their actual model, player index, and scale, transferring old-object ownership when necessary.
- CreatePhysicObjectFromConfig creates the corresponding backend object, loads collision-shape external resources, and inserts it into m_physicObjects after Build succeeds; it destroys the temporary object if construction fails.
- UpdateAllPhysicObjects creates CPhysicObjectUpdateContext for every object and passes current gravity. Entities not emitted this frame are marked for release, while the others execute Update. StepSimulation then advances Bullet.

### 3. Object, Component, and Backend Layers
- CBaseStaticObject, CBaseDynamicObject, and CBaseRagdollObject handle common object state, configuration copying, Build/Rebuild, component containers, lifecycle updates, world addition/removal, and queries.
- DispatchBuildPhysicComponents proceeds in this order: create all rigid bodies; create non-DeferredCreate constraints; create behaviors; finally create DeferredCreate constraints. NonNative components are retained only for upper-level behavior/editing logic and do not directly create Bullet native objects.
- BulletStaticObject, BulletDynamicObject, and BulletRagdollObject implement backend factories, translating configurations into Bullet rigid bodies, constraints, and behaviors. Static objects primarily provide zero-mass colliders; dynamic/ragdoll objects support constraint solving, external forces, motion state, and behavior.
- The three main IPhysicComponent implementations are IPhysicRigidBody, IPhysicConstraint, and IPhysicBehavior. Components link both configurations and runtime instances via configId and a separate physicComponentId; the manager handles global registration and reclamation.
- CBulletPhysicManager creates btDefaultCollisionConfiguration, dispatcher, DBVT broadphase, sequential impulse solver, and a custom CBulletDiscreteDynamicsWorld. Objects/components connect back to MetaHook objects through Bullet collision groups, user index/pointer, and motion state.

### 4. Frame Timing, Physics Stepping, and Queries
- HUD_CreateEntities enumerates active players and client edicts after original client entity creation, marks entities emitted, and calls CreatePhysicObjectForEntity.
- HUD_TempEntUpdate processes temporary entities, updates gravity, temporarily drives view updates in Sven Co-op third person, then calls UpdateAllPhysicObjects and StepSimulation.
- CBulletPhysicManager::SetGravity converts GoldSrc gravity to the Bullet coordinate system; StepSimulation uses stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate()). GetSimulationTickRate is supplied by bv_simrate, with the common layer clamping it to 32–128.
- TraceLine converts GoldSrc rays to Bullet, uses filter groups to hit world, static/dynamic/ragdoll, constraints, or behaviors, then converts the hit entity index and component ID back to CPhysicTraceLineHitResult.
- HUD_DrawTransparentTriangles calls DebugDraw when AllowCheats and a debug cvar are enabled; the debug context controls visualization by object/component level, color, and inspect/selected state.

### 5. Studio Bone and Physics Synchronization
- HUD_GetStudioModelInterface caches engine_studio_api_s, gpStudioInterface, and pbonetransform/plighttransform, then resolves and installs EngineStudio/ClientStudio hooks.
- StudioDrawModel/Player use g_iRagdollRenderEntIndex and g_iRagdollRenderFlags to mark the entity owning the current SetupBones. STUDIO_RAGDOLL_SETUP_BONES lets the engine calculate animated bones only; STUDIO_RAGDOLL_UPDATE_BONES sends animation results to the physics side.
- StudioSetupBones_Template proceeds by first attempting physics-object SetupBones (skipping the original SetupBones when physics takes over), then calling the original SetupBones, and finally attempting SetupJiggleBones (kinematic/jiggle-bone synchronization).
- When building a ragdoll, SetupBonesForRagdoll samples animated bones and BulletCreateMotionState generates CBulletBoneMotionState. Dynamic rigid bodies update the bone matrix through setWorldTransform, and render hooks write it back to pbonetransform/plighttransform. The kinematic state instead writes the latest animated bones back into motion state.
- V_CalcRefdef delegates spectator/local-player camera synchronization to the physics object's CalcRefDef when not paused, in intermission, portal view, and other excluded states; bv_syncview determines the synchronization level.

### 6. Debug UI and Configuration Editing
- bv_open_debug_ui enters CViewport::OpenPhysicDebugGUI. CViewport creates CPhysicDebugGUI through VGUI2Extension and forwards state during NewMap, per-frame inspect updates, and UI lifecycle events.
- PhysicDebugGUI provides five inspect modes for entities/physics objects/rigid bodies/constraints/behaviors, selection colors, editing dialogs, configuration reload, and saving. Edits normally rebuild objects through RebuildPhysicObjectEx2 while retaining reusable component IDs.
- VGUI2ExtensionImport obtains IVGUI2Extension, DPI, Surface, Scheme, and Input interfaces from VGUI2Extension.dll, and initializes IKeyValuesSystem through DLL load notifications for configuration KeyValues and UI usage.

## Dependencies
- MetaHook API and public interfaces: include/metahook.h, include/Interface/; used for plugin ABI, address resolution, inline/vtable hooks, engine type, and filesystem access.
- GoldSrc/SvEngine client and Studio interfaces: HLSDK cl_dll, engine, studio, and r_efx structures, plus engine_studio_api_s and cl_exportfuncs_t.
- Bullet3: btBulletDynamicsCommon and collision/ghost components; provided by Bullet3IncludeDirectory, Bullet3LibrariesDirectory, and Bullet3LibraryFiles.
- VGUI2Extension: runtime VGUI2Extension.dll; provides IVGUI2Extension, DPI, Surface, Scheme, Input, and KeyValuesSystem.
- SourceSDK/KeyValues/FileSystem: configuration I/O, model-file access, and resource paths; uses IFileSystem_HL25 for HL25 compatibility.
- GLEW: OpenGL extension loading for debug drawing; executes glewInit after HUD_Init.
- Capstone: instruction scanning/disassembly address resolution for private hooks.
- tinyobjloader, ScopeExit, Chocobo1Hash: OBJ collision resources, scope cleanup, and hashing/integrity helpers; all are declared as include/build dependencies in BulletPhysics.vcxproj.
- Runtime resources: *_physics.txt/*_ragdoll.txt, BSP brush data, external .obj collision meshes, Build/svencoop/bulletphysics/*.res, and localized text.

## Notes
- This plugin is a runtime system based on global pointers, global hooks, and one client physics manager. The source provides no cross-thread synchronization; physics objects, configurations, and render matrices should be accessed according to the client-hook main-thread ordering.
- Hook signatures and vtable offsets are resolved by engine/client build number and mirror module. When adding engine versions, privatehook.cpp, exportfuncs.cpp, and enginedef.h are high-risk compatibility boundaries.
- m_physicObjects uses entity index as its primary key, while an entity can change models due to model switching, corpse transfer, or temporary-entity reuse. Cross-frame access should prioritize the packed physics object ID to validate modelindex.
- The current-frame emitted state determines whether an object survives. If entity enumeration/hook order changes, UpdateAllPhysicObjects can mistakenly consider an object expired and release it.
- NewMap clears BSP-origin objects, configurations, and index caches before rebuilding world collision; map switching and configuration reload must not interleave with active object updates.
- Excessively low or high bv_simrate is forcibly clamped to 32–128. Bullet fixed stepping uses at most 4 substeps; excessive frametime can still cause simulation error or CPU spikes.
- AllowCheats gates debug commands, debug drawing, and configuration saving. When diagnosing configuration issues, confirm the sv_cheats/SvEngine allow_cheats path.
- If VGUI2Extension.dll is missing, the initialization function returns directly; if the DLL exists but its interface factory or a required interface is missing, it calls Sys_Error. The debug UI is not the sole dependency of core physics operation, but configuration editing depends on it.
- ClientPhysicManager.h still declares PhysXPhysicManager_CreateInstance, but LoadEngine currently always creates the Bullet backend; do not infer that a switchable PhysX implementation exists.
- Configuration load/save and BoneMatrix synchronization have separate dedicated topics: [[bulletphysics_physics_config]] and [[bulletphysics_bonematrix_physics_sync]]. Plugin integration details are in [[bulletphysics_plugin_overview]], and the overall plugin boundary is in [[plugin_system]].

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

---
title: BulletPhysics Plugin Overview
type: note
permalink: metahooksv/bulletphysics-plugin-overview
---

# BulletPhysics Plugin Overview (MetaHookSv)

## Location and Responsibilities
- Directory: `Plugins/BulletPhysics/`
- Purpose: Provides client-side physics object/ragdoll simulation and debug UI for GoldSrc/SvEngine based on **Bullet3**; integrates through the MetaHookSv plugin interface and applies the required hooks to engine/client rendering and animation flows.

## Directory Structure (Grouped by Function)
- Plugin entry points and hooks: `plugins.cpp/.h`, `exportfuncs.cpp/.h`, `privatehook.cpp/.h`
- Physics system abstraction layer: `ClientPhysicManager.h`, `BasePhysicManager.cpp/.h` (contains extensive configuration/object-management logic)
- Bullet3 implementation: `BulletPhysicManager.cpp/.h` (world creation, stepping, collision shape/constraint creation, and more)
- Physics objects and components:
  - Objects: `BaseStaticObject* / BaseDynamicObject* / BaseRagdollObject*` + `BulletStaticObject* / BulletDynamicObject* / BulletRagdollObject*`
  - Rigid bodies/constraints: `BasePhysicRigidBody* / BasePhysicConstraint*` + `BulletPhysicRigidBody* / BulletPhysicConstraint*`, etc.
  - Behaviors: Barnacle/Gargantua/buoyancy/camera, and others (`Bullet*Behavior.*`)
- UI/debugger: `Viewport.*`, `PhysicDebugGUI.*`, and many `*Page/*Panel/*Dialog` files (editing/inspection/debug panels)
- VGUI2Extension dependency integration: `VGUI2ExtensionImport.*`, `BaseUI.cpp`, `GameUI.cpp`, `ClientVGUI.cpp`
- Other utilities: `ClientEntityManager.*` (entity spawning/inspect-state management), etc.

## Plugin Lifecycle and Core Entry Points
- `plugins.cpp`
  - `IPluginsV4::Init(...)`: Stores `g_pMetaHookAPI/g_pInterface/g_pMetaSave`.
  - `IPluginsV4::LoadEngine(cl_enginefunc_t*)`:
    - Initializes file-system pointers (`g_pInterface->FileSystem` / HL25 compatibility field).
    - Records engine type, BuildNum, and module-section information (engine + mirror engine).
    - Registers the DLL load-notification callback `DllLoadNotification` (used to initialize KeyValuesSystem after discovering `vgui2.dll`).
    - Copies `gEngfuncs`, then calls `Engine_FillAddress(...)` + `Engine_InstallHook()`.
    - Initializes VGUI2Extension (`VGUI2Extension_Init()`) and registers BaseUI/GameUI/ClientVGUI callbacks.
    - Creates the global physics manager: `g_pClientPhysicManager = BulletPhysicManager_CreateInstance();`
    - Calls `glewInit()` (for OpenGL/GLEW debug drawing).
  - `IPluginsV4::LoadClient(cl_exportfuncs_t*)`:
    - Stores the original `gExportfuncs` and replaces exported functions such as `HUD_Init/HUD_*` and `V_CalcRefdef`.
    - Records client DLL (including mirror) section information, then executes `Client_FillAddress(...)` and `Client_InstallHooks()`.
  - `IPluginsV4::Shutdown()`: Unregisters the DLL notification callback.
  - `IPluginsV4::ExitGame(int)`: Destroys the physics manager, unloads UI callbacks and engine hooks, and shuts down VGUI2Extension.

## Engine/Client Hooks (Overview)
- `privatehook.cpp`
  - `Engine_FillAddress(...)`: Locates engine functions/global variables by category (rendering, view, temporary entities, visible entity list, and more).
  - `Engine_InstallHook()`: Installs inline hooks for `R_NewMap` + `R_RenderView` (SvEngine uses the `R_RenderView_SvEngine` branch).
  - `Client_FillAddress(...)`: Identifies `dod/cstrike/czero/...` based on the game directory, sets `g_bIsDayOfDefeat/g_bIsCounterStrike`, and resolves additional addresses.

## cl_exportfuncs Override Points and Runtime Behavior
- `exportfuncs.cpp`
  - `HUD_Init()`:
    - Calls the original `gExportfuncs.HUD_Init()`, then calls `ClientPhysicManager()->Init()`.
    - Registers/obtains cvars: `bv_debug_draw*`, `bv_simrate`, `bv_syncview`, `bv_force_updatebones`, and `sv_cheats/chase_active/...`.
    - Registers commands: `bv_open_debug_ui`, `bv_reload_all`, `bv_reload_objects`, `bv_reload_configs`, `bv_save_configs`.
    - Installs the `ClCorpse` message hook (if present) and an inline hook for `efxapi_R_TempModel`.
    - Initializes `g_pViewPort` (if present).
  - `HUD_GetStudioModelInterface(...)`:
    - Obtains `engine_studio_api_s` and installs Studio/Renderer-related hooks (`EngineStudio_*`, `ClientStudio_*`).
    - Reads bone-matrix pointers `pbonetransform/plighttransform` and caches `StudioCheckBBox`, among others.
  - `HUD_CreateEntities()`:
    - Enumerates players and client edicts every frame (including validation of `messagenum/EF_NODRAW/modelindex`, etc.), marks entities as "spawned," and calls `ClientPhysicManager()->CreatePhysicObjectForEntity(...)`.
  - `HUD_TempEntUpdate(...)`:
    - Traverses temporary entities and creates any missing physics objects.
    - Calls `ClientPhysicManager()->SetGravity(cl_gravity)`.
    - During Sven third-person mode, wraps a forced view update with `g_bIsUpdatingRefdef` (calling `CAM_Think()` + `V_RenderView()`).
    - Calls `UpdateAllPhysicObjects(...)`, then `StepSimulation(frametime)`.
  - `HUD_DrawTransparentTriangles()`: Calls `ClientPhysicManager()->DebugDraw()` when `AllowCheats()` is true and `bv_debug_draw` is enabled.
  - `R_NewMap()` (engine inline hook): After executing the original `R_NewMap`, triggers `ClientPhysicManager()->NewMap()`, `ClientEntityManager()->NewMap()`, and `g_pViewPort->NewMap()`.
  - `V_CalcRefdef(ref_params_s*)`: Under conditions such as not paused, not in intermission, and not portal rendering, synchronizes the camera with physics objects (spectator/local player) according to `bv_syncview`.

## Bullet3 Physics World and Simulation Stepping
- `BulletPhysicManager.cpp`
  - `CBulletPhysicManager::Init()`: Creates the Bullet world (collision config/dispatcher/broadphase/solver/world), sets the debug drawer and overlap filter callback, and initializes gravity to 0.
  - `CBulletPhysicManager::StepSimulation(double frametime)`: `stepSimulation(frametime, 4, 1.0f / GetSimulationTickRate())`.
  - `CBulletPhysicManager::DebugDraw()`: Creates `CPhysicDebugDrawContext` according to `bv_debug_draw_level_*` and color cvars, filters visualizations by component/object, and calls `debugDrawWorld()`.
  - `BulletPhysicManager_CreateInstance()`: Returns `new CBulletPhysicManager()`.

## Debug UI and VGUI2Extension
- Dependency: `VGUI2Extension.dll` (`VGUI2ExtensionImport.cpp` obtains `IVGUI2Extension` and DPI/VGUI* interfaces through `Sys_GetFactory`; failure calls `Sys_Error`).
- Typical interactions:
  - `bv_open_debug_ui` -> `CViewport::OpenPhysicDebugGUI()` -> `PhysicDebugGUI`.
  - BaseUI/GameUI/ClientVGUI register callbacks through `IVGUI2Extension_*Callbacks` to accommodate the UI lifecycle, input, and window procedure.

## Build and Third-Party Dependencies
- `Plugins/BulletPhysics/BulletPhysics.vcxproj`:
  - include: `$(Bullet3IncludeDirectory)`, `$(GLEWIncludeDirectory)`, `$(CapstoneIncludeDirectory)`, `$(ScopeExitIncludeDirectory)`, `$(TinyObjLoaderDirectory)`, `$(Chocobo1HashDirectory)`, and others.
  - libs: `$(Bullet3LibrariesDirectory)`, `$(GLEWLibrariesDirectory)` + `$(Bullet3LibraryFiles)`, `$(GLEWLibraryFiles)`.
  - pre-build: Runs `$(Bullet3CheckRequirements)` (automatically invokes a script to build Bullet3 when libraries are missing).
- Variable source: `tools/global_common.props`
  - Bullet3: `thirdparty/install/bullet3/<Platform>/<Config>/...`; invokes `scripts/build-bullet3-<Platform>-<Config>.bat` when missing.
  - GLEW: `thirdparty/install/glew/<Platform>/<Config>/...`; invokes `scripts/build-glew-<Platform>-<Config>.bat` when missing.

## Common cvars/Commands (Debug Workflow)
- Commands: `bv_open_debug_ui`, `bv_reload_all`, `bv_reload_objects`, `bv_reload_configs`, `bv_save_configs`
- cvars: `bv_debug_draw`, `bv_debug_draw_wallhack`, `bv_debug_draw_level_*`, `bv_debug_draw_*_color`, `bv_simrate`, `bv_syncview`, `bv_force_updatebones`

## Notes
- `AllowCheats()`: Uses the `allow_cheats` pointer under SvEngine; other engines use the `sv_cheats` cvar.
- `ClientPhysicManager.h` declares `PhysXPhysicManager_CreateInstance()`, but no implementation was found within `Plugins/BulletPhysics/`; `LoadEngine` currently always creates the Bullet version.

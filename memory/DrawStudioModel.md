---
title: DrawStudioModel
type: note
permalink: metahooksv/draw-studio-model
---

# Renderer Plugin - StudioModel (`.mdl`) Rendering Workflow (`Plugins/Renderer`)

> Objective: document the complete StudioModel path from entering `R_DrawCurrentEntity` to the final `glDrawElements`, and how Renderer implements multiple passes (Outline/HairShadow/GlowShell/deferred transparency).

---

## 0. Initialization Entry Point: Acquire the Studio Interface and Install Hooks

- Entry point: `Plugins/Renderer/exportfuncs.cpp:1828` `HUD_GetStudioModelInterface(version, ppinterface, pstudio)`
  - Preserves the original Engine Studio API:
    - `gPrivateFuncs.studioapi_GL_SetRenderMode / SetupRenderer / RestoreRenderer / StudioDynamicLight / StudioCheckBBox`
  - Resolves and installs engine-side Studio addresses and hooks: `EngineStudio_FillAddress(...)` → `EngineStudio_InstalHooks()`
  - Caches key pointers:
    - `pbonetransform = pstudio->StudioGetBoneTransform()`
    - `plighttransform = pstudio->StudioGetLightTransform()`
    - `rotationmatrix = pstudio->StudioGetRotationMatrix()`
  - Records the Studio interface pointer: `gpStudioInterface = ppinterface` (later entity rendering calls `StudioDrawModel/StudioDrawPlayer` through it)
  - Installs client-side Studio hooks: `ClientStudio_FillAddress(ppinterface)` → `ClientStudio_InstallHooks()`

> Conclusion: Renderer acquires `r_studio_interface_s` + `engine_studio_api_s` through the HUD interface and prepares to replace/intercept the Studio rendering pipeline at this stage.

---

## 1. Per-Frame Stage: Frame Lifecycle of the Studio Subsystem

- `Plugins/Renderer/gl_rmain.cpp:2939` `R_RenderFrameStart()`
  - Calls `R_StudioStartFrame()` (`Plugins/Renderer/gl_studio.cpp:4952`)
    - Clears the per-frame player-info storage markers.
    - `R_StudioClearAllBoneCaches()`: clears this frame's bone cache (preparing for multi-pass reuse).

- `Plugins/Renderer/gl_rmain.cpp:2988` `R_RenderEndFrame()`
  - Calls `R_StudioEndFrame()` (`Plugins/Renderer/gl_studio.cpp:4964`, currently empty)

---

## 2. Entity Rendering Entry Point: From Scene Entities to StudioDrawModel/Player

- Dispatch: `Plugins/Renderer/gl_rmain.cpp:2358` `R_DrawCurrentEntity(bool bTransparent)`
  - `model->type == mod_studio` → `R_DrawStudioEntity(bTransparent)`

- Studio entity handling: `Plugins/Renderer/gl_rmain.cpp:2242` `R_DrawStudioEntity(bool bTransparent)`
  - Player entities: `(*currententity)->player`
    - Trigger engine/client Studio rendering through `(*gpStudioInterface)->StudioDrawPlayer(...)` (possibly with `STUDIO_EVENTS`).
  - Non-player entities: `(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS)`.
  - Special case: `MOVETYPE_FOLLOW` (attachments/viewmodel attachments) temporarily switches `currententity` to draw the aiment, then draws itself.

> Key point: Renderer does not directly “manually traverse mdl meshes” here. It reuses the engine's Studio rendering call chain and takes over actual drawing at internal hook points.

---

## 3. StudioRenderModel: Multi-Pass Orchestration (Analysis/Deferral/Glow/Outline/Hair Shadow)

Renderer replaces “engine-side StudioRenderer RenderModel/RenderFinal” with custom logic:

- `Plugins/Renderer/gl_studio.cpp:3600+` `StudioRenderModel_Template(pfnRenderModel, pfnRenderFinal, ...)`
  - **ShadowView**: if `R_IsRenderingShadowView()`, directly calls `pfnRenderModel` once and returns (only the shadow-caster path runs).

  - **Deferred queue handling for the transparent Pass**: when `!r_draw_opaque`, checks the entity component `DeferredStudioPasses`:
    - If present, sets `currententity->curstate.renderfx = fx` and calls `pfnRenderModel` for each entry, then clears the queue and `return`s.

  - **Analysis Pass (key)**:
    - Clears: `r_draw_hashair/hasface/hasalpha/hasadditive/hasoutline = false`.
    - Sets: `r_draw_analyzingstudio = true`.
    - Calls `pfnRenderModel` once (this triggers `GLStudioDrawPoints`, but Analysis mode only gathers characteristics).
    - Ends: `r_draw_analyzingstudio = false`.
    - If a mesh did not draw outline but `R_StudioHasOutline()` returns true, additionally sets `r_draw_hasoutline = true`.

  - **Defer transparency-related meshes to the transparent Pass** (when currently in the opaque Pass and transparent materials are detected):
    - `r_draw_opaque && r_draw_hasalpha` → `DeferredStudioPasses += kRenderFxDrawAlphaMeshes`
    - `r_draw_opaque && r_draw_hasadditive` → `DeferredStudioPasses += kRenderFxDrawAdditiveMeshes`
    - Also sets: `r_draw_deferredtrans = true`.

  - **HairShadow Pass**: if `R_StudioHasHairShadow()`:
    - Binds and clears `s_BackBufferFBO2`.
    - Temporarily sets `renderfx = kRenderFxDrawShadowHair` and calls `pfnRenderModel`.
    - Restores the FBO.

  - **GlowShell/Outline/Normal and other combinations**:
    - `renderfx == kRenderFxGlowShell`: first DrawNormalPass (draws once with `renderfx=0`), then optionally OutlinePass, and finally handles GlowShell:
      - If currently `r_draw_opaque`: defer `kRenderFxDrawGlowShell` to the transparent Pass.
      - Otherwise, draw the GlowShell Pass immediately.
    - Otherwise:
      - Records PostProcessGlow-related entities in a list (for the post-processing stage).
      - Draws NormalPass.
      - If `r_draw_hasoutline`: draws OutlinePass (`renderfx = kRenderFxDrawOutline`).

  - If Outline was actually drawn this time, clears the marker with `GL_ClearStencil(STENCIL_MASK_HAS_OUTLINE)`.

> Conclusion: StudioModel multi-pass rendering is not performed in `R_DrawStudioEntity`; it is completed in the hooked `RenderModel` through **one analysis plus conditional multiple calls to the original RenderModel**.

---

## 4. GLStudioDrawPoints: Actual Mesh Rendering Takeover Point

The engine's original `GLStudioDrawPoints` is inline-hooked by Renderer:

- Hook location and installation: `Plugins/Renderer/gl_hooks.cpp` (for example, `Engine_FillAddress_R_GLStudioDrawPoints` + `Install_InlineHook(R_GLStudioDrawPoints)`).
- Takeover implementation: `Plugins/Renderer/gl_studio.cpp:3264` `R_GLStudioDrawPoints()`.

`R_GLStudioDrawPoints()` core logic:
1. Obtains the `CStudioModelRenderData` for the current `studiohdr`:
   - Fast path: `R_GetStudioRenderDataFromStudioHeaderFast(*pstudiohdr)` (`studiohdr->soundtable` is used as modelindex).
   - Slow path: `R_GetStudioRenderDataFromStudioHeaderSlow(*pstudiohdr)`: traverses known models, matches `mod->cache.data == studiohdr`, and calls `R_CreateStudioRenderData(mod, Mod_Extradata(mod))`.
2. If GPU resources (`pRenderData->hVAO`) are not ready (asynchronous loading is in progress), returns immediately and skips drawing.
3. If currently in ShadowView and `studiohdr->flags & FMODEL_NOSHADOW`, skips drawing.
4. If `r_draw_analyzingstudio`: calls only `R_StudioDrawSubmodel(...)` (the statistics path).
5. Otherwise:
   - `R_StudioDrawRenderDataBegin(pRenderData)` → `R_StudioDrawSubmodel(...)` → `R_StudioDrawRenderDataEnd()`

---

## 5. RenderDataBegin/Submodel/Mesh: From SubModel to glDrawElements

### 5.1 Begin: Upload Studio UBO + Bind VAO

- `Plugins/Renderer/gl_studio.cpp:2127` `R_StudioDrawRenderDataBegin(pRenderData)`
  - Assembles and uploads `studio_ubo_t`:
    - Color/opacity/scale: determined from `renderfx`/Glow/Outline/`rendermode`/`r_blend`.
    - Lighting parameters: `r_ambientlight` / `r_shadelight` / `r_plightvec`.
    - Legacy ELights (optional): `r_studio_legacy_elight`.
    - Bone matrices: `memcpy(StudioUBO.bonematrix, *pbonetransform, 128 * mat3x4)`.
    - LowerBody clipbone: generates the `r_clipbone` bitmask when `R_IsRenderingClippedLowerBody()`.
  - Calls `glBindBufferBase(...BINDING_POINT_STUDIO_UBO...)` after `GL_UploadSubDataToUBO`.
  - `GL_BindVAO(pRenderData->hVAO)`

### 5.2 Submodel: Find the VBO SubModel and Draw Each Mesh

- `Plugins/Renderer/gl_studio.cpp:3233` `R_StudioDrawSubmodel(studiohdr, submodel, pRenderData)`
  - Uses `submodel_byteoffset` to locate the corresponding `CStudioModelRenderSubModel` in `pRenderData->mSubmodels`.
  - Determines `ptexturehdr/ptexture/pskinref` through `R_StudioGetTextureHeaderSkinref(...)`.
  - Calls `R_StudioDrawSubmodel(pRenderData, pRenderSubmodel, ...)` to traverse meshes.

### 5.3 Mesh: Analysis and Drawing Paths

- `Plugins/Renderer/gl_studio.cpp:3143` `R_StudioDrawMesh(...)`
  - Generates `flags` from `mstudiotexture.flags` and current entity state (fullbright, glowshell, rendermode, Celshade switch, etc.).
  - `r_draw_analyzingstudio` → `R_StudioDrawMesh_AnalysisPass(...)`
    - Counts `r_draw_hasalpha / hasadditive / hasface / hashair` (for upper-level multi-pass orchestration).
  - Otherwise → `R_StudioDrawMesh_DrawPass(...)`

- `Plugins/Renderer/gl_studio.cpp:2378` `R_StudioDrawMesh_DrawPass(...)` (central implementation)
  - Combines `program_state_t StudioProgramState`:
    - ShadowView / GlowStencil / GlowColor / GlowShell / Outline / HairShadow
    - Transparency policy:
      - The `r_draw_opaque` stage skips `STUDIO_NF_ALPHA/ADDITIVE` meshes (and defers them to the transparent pass at the upper level).
      - The transparent pass enables `STUDIO_ALPHA_BLEND_ENABLED / STUDIO_ADDITIVE_BLEND_ENABLED`.
    - Legacy dynamic/entity lights: `r_studio_legacy_dlight / r_studio_legacy_elight`.
    - LowerBody near-plane clipping, multiview, linear depth, water/portal clipping, and more.
  - Selects and enables the shader variant: `R_UseStudioProgram(StudioProgramState, &prog)` (internally caches programs by state).
  - Binds texture units (diffuse/normal/parallax/specular/animated/stencil/shadow-diffuse, etc.) and sets many uniforms (celshade/outline/hair/packed skin, etc.).
  - `glDrawElements(GL_TRIANGLES, ...)`
  - Restore: unbinds textures and restores depth/blend/cull/stencil state.

---

## 6. RenderData Creation/Caching/Asynchronous Loading

### 6.1 During Model Loading: Optional Pre-Creation

- `Plugins/Renderer/gl_model.cpp:97` `Mod_LoadStudioModel(model_t* mod, void* buffer)`
  - Calls the original `gPrivateFuncs.Mod_LoadStudioModel`.
  - If `r_studio_lazy_load == 0`: immediately calls `R_CreateStudioRenderData(mod, studiohdr)`.
  - Otherwise: reloads only when RenderData for this model already exists in cache.

### 6.2 On First Draw: Slow-Path Creation

- `R_GLStudioDrawPoints()` → `R_GetStudioRenderDataFromStudioHeaderSlow()`
  - After finding `mod`, calls `R_CreateStudioRenderData(mod, Mod_Extradata(mod))`.

### 6.3 RenderData Contents and Asynchronous Tasks

- `Plugins/Renderer/gl_studio.cpp:4878` `R_CreateStudioRenderData(mod, studiohdr)`
  - Uses `R_CalcStudioHeaderHash` to determine whether old `CStudioModelRenderData` can be reused.
  - Loads TextureModel / external configuration (celshade/lowerbody/texture flags/replacement textures, etc.).
  - Starts asynchronous loading: `R_CreateStudioRenderDataAsyncLoadTask(...)`.

- `Plugins/Renderer/gl_studio.cpp:4830` `R_CreateStudioRenderDataAsyncLoadTask(...)`
  - `r_studio_parallel_load > 0`: background-thread task.
  - Otherwise: runs RunTask synchronously, then schedules `AsyncUploadResouce()` on the game thread.

> `R_GLStudioDrawPoints` directly returns when `pRenderData->hVAO` is not ready. Therefore, a model first appearing as “not displayed/delayed by one frame” is usually related to asynchronous upload timing.

---

## 7. BoneCache: Key Optimization Supporting Multiple Passes

- `StudioSetupBones_Template` (`Plugins/Renderer/gl_studio.cpp:3960`):
  - If `r_studio_bone_caches` is enabled and this is not a viewmodel/lowerbody:
    - Uses `CStudioBoneCacheHandle(soundtable, sequence, gaitsequence, frame, origin, angles)` as the key.
    - On a hit, directly memcpy's back to `*pbonetransform / *plighttransform`, avoiding repeated calculations.

- `StudioSaveBones_Template` (`Plugins/Renderer/gl_studio.cpp:3996`):
  - After calling the original SaveBones, stores the current bone matrices in `g_StudioBoneCacheManager[handle]`.

- `StudioMergeBones_Template` (`Plugins/Renderer/gl_studio.cpp:4033`):
  - Also reuses the cache (on a hit, directly copies matrices without merging again).

---

## 8. One-Sentence Overview (From Entity to GPU)

`R_DrawCurrentEntity` → `R_DrawStudioEntity` → `gpStudioInterface->StudioDrawModel/Player` → (hook) `StudioRenderModel_Template` (analysis/multiple passes/deferral) → (hook) `R_GLStudioDrawPoints` (called by submodel) → `R_StudioDrawRenderDataBegin` (UBO+VAO) → `R_StudioDrawSubmodel` → `R_StudioDrawMesh_DrawPass` (program_state selects shader + binds textures + `glDrawElements`) → `R_StudioDrawRenderDataEnd`

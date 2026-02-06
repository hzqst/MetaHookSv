# Renderer 插件 - StudioModel（.mdl）绘制流程（Plugins/Renderer）

> 目标：说明 StudioModel 从 `R_DrawCurrentEntity` 进入，到最终 `glDrawElements` 的完整链路；以及多 Pass（Outline/HairShadow/GlowShell/透明延后）如何在 Renderer 中实现。

---

## 0. 初始化入口：获取 Studio 接口并安装 Hook

- 入口：`Plugins/Renderer/exportfuncs.cpp:1828` `HUD_GetStudioModelInterface(version, ppinterface, pstudio)`
  - 保存原始 Engine Studio API：
    - `gPrivateFuncs.studioapi_GL_SetRenderMode / SetupRenderer / RestoreRenderer / StudioDynamicLight / StudioCheckBBox`
  - 解析并安装引擎侧 Studio 相关地址与 Hook：`EngineStudio_FillAddress(...)` → `EngineStudio_InstalHooks()`
  - 缓存关键指针：
    - `pbonetransform = pstudio->StudioGetBoneTransform()`
    - `plighttransform = pstudio->StudioGetLightTransform()`
    - `rotationmatrix = pstudio->StudioGetRotationMatrix()`
  - 记录 Studio 接口指针：`gpStudioInterface = ppinterface`（后续实体绘制通过它调用 `StudioDrawModel/StudioDrawPlayer`）
  - 安装客户端侧 Studio Hook：`ClientStudio_FillAddress(ppinterface)` → `ClientStudio_InstallHooks()`

> 结论：Renderer 通过 HUD 接口拿到 `r_studio_interface_s` + `engine_studio_api_s`，并在此阶段完成“替换/拦截 Studio 渲染管线”的准备。

---

## 1. 每帧阶段：Studio 子系统的 Frame 生命周期

- `Plugins/Renderer/gl_rmain.cpp:2939` `R_RenderFrameStart()`
  - 调用 `R_StudioStartFrame()`（`Plugins/Renderer/gl_studio.cpp:4952`）
    - 清理每帧的 player-info 存储标记
    - `R_StudioClearAllBoneCaches()`：清理本帧骨骼缓存（为多 Pass 复用做准备）

- `Plugins/Renderer/gl_rmain.cpp:2988` `R_RenderEndFrame()`
  - 调用 `R_StudioEndFrame()`（`Plugins/Renderer/gl_studio.cpp:4964`，当前为空）

---

## 2. 实体绘制入口：从场景实体到 StudioDrawModel/Player

- 分发：`Plugins/Renderer/gl_rmain.cpp:2358` `R_DrawCurrentEntity(bool bTransparent)`
  - `model->type == mod_studio` → `R_DrawStudioEntity(bTransparent)`

- Studio 实体处理：`Plugins/Renderer/gl_rmain.cpp:2242` `R_DrawStudioEntity(bool bTransparent)`
  - 玩家实体：`(*currententity)->player`
    - 通过 `(*gpStudioInterface)->StudioDrawPlayer(...)` 触发引擎/客户端 Studio 渲染（可能带 `STUDIO_EVENTS`）
  - 非玩家实体：`(*gpStudioInterface)->StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS)`
  - 特殊：`MOVETYPE_FOLLOW`（附件/视模附件）会临时切换 `currententity` 去绘制 aiment，再绘制自身。

> 关键点：Renderer 并不直接在这里“手动遍历 mdl 网格”；而是复用引擎的 Studio 渲染调用链，随后在内部 hook 点接管真正的绘制。

---

## 3. StudioRenderModel：多 Pass 编排（分析/延后/发光/描边/发丝阴影）

Renderer 将“引擎侧 StudioRenderer 的 RenderModel/RenderFinal”替换为自定义逻辑：

- `Plugins/Renderer/gl_studio.cpp:3600+` `StudioRenderModel_Template(pfnRenderModel, pfnRenderFinal, ...)`
  - **ShadowView**：如果 `R_IsRenderingShadowView()` 直接调用一次 `pfnRenderModel` 返回（只走阴影 caster 路径）。

  - **透明 Pass 的延后队列处理**：当 `!r_draw_opaque` 时，会检查实体组件 `DeferredStudioPasses`：
    - 若存在，逐个把 `currententity->curstate.renderfx = fx` 并调用 `pfnRenderModel` 绘制，再清空队列并 `return`。

  - **分析 Pass（关键）**：
    - 清零：`r_draw_hashair/hasface/hasalpha/hasadditive/hasoutline = false`
    - 置位：`r_draw_analyzingstudio = true`
    - 调用一次 `pfnRenderModel`（此时会触发 `GLStudioDrawPoints`，但在分析模式下只做特征统计）
    - 结束：`r_draw_analyzingstudio = false`
    - 如果 mesh 没画 outline 但 `R_StudioHasOutline()` 返回 true，则补记 `r_draw_hasoutline = true`

  - **把透明相关 mesh 延后到透明 Pass**（当前在不透明 Pass 且检测到透明材质时）：
    - `r_draw_opaque && r_draw_hasalpha` → `DeferredStudioPasses += kRenderFxDrawAlphaMeshes`
    - `r_draw_opaque && r_draw_hasadditive` → `DeferredStudioPasses += kRenderFxDrawAdditiveMeshes`
    - 同时置位：`r_draw_deferredtrans = true`

  - **HairShadow Pass**：如果 `R_StudioHasHairShadow()`：
    - 绑定 `s_BackBufferFBO2` 清理后
    - 临时设置 `renderfx = kRenderFxDrawShadowHair` 调用 `pfnRenderModel`
    - 再恢复 FBO

  - **GlowShell/Outline/Normal 等组合**：
    - `renderfx == kRenderFxGlowShell`：先 DrawNormalPass（把 `renderfx=0` 画一遍），然后可选 OutlinePass，再处理 GlowShell：
      - 若当前是 `r_draw_opaque`：把 `kRenderFxDrawGlowShell` 延后到透明 Pass
      - 否则立即画 GlowShell Pass
    - 否则：
      - 记录 PostProcessGlow 相关实体到列表（用于后处理阶段）
      - 画 NormalPass
      - 若 `r_draw_hasoutline`：画 OutlinePass（`renderfx = kRenderFxDrawOutline`）

  - 若本次确实绘制过 Outline：会 `GL_ClearStencil(STENCIL_MASK_HAS_OUTLINE)` 清理标记。

> 结论：StudioModel 的“多 Pass”不是在 `R_DrawStudioEntity` 中做，而是在 hook 的 `RenderModel` 内通过 **分析一次 + 条件多次调用原始 RenderModel** 来完成。

---

## 4. GLStudioDrawPoints：真正的网格绘制接管点

引擎原本的 `GLStudioDrawPoints` 被 inline hook 到 Renderer：

- Hook 定位与安装：`Plugins/Renderer/gl_hooks.cpp`（例如 `Engine_FillAddress_R_GLStudioDrawPoints` + `Install_InlineHook(R_GLStudioDrawPoints)`）
- 接管实现：`Plugins/Renderer/gl_studio.cpp:3264` `R_GLStudioDrawPoints()`

`R_GLStudioDrawPoints()` 核心逻辑：
1. 获取当前 `studiohdr` 对应的 `CStudioModelRenderData`：
   - 快速：`R_GetStudioRenderDataFromStudioHeaderFast(*pstudiohdr)`（`studiohdr->soundtable` 被用作 modelindex）
   - 慢速：`R_GetStudioRenderDataFromStudioHeaderSlow(*pstudiohdr)`：遍历已知模型，匹配 `mod->cache.data == studiohdr`，并 `R_CreateStudioRenderData(mod, Mod_Extradata(mod))`
2. 若 GPU 资源（`pRenderData->hVAO`）未就绪（异步加载中），直接返回跳过绘制。
3. 若当前在 ShadowView 且 `studiohdr->flags & FMODEL_NOSHADOW`，跳过。
4. 若 `r_draw_analyzingstudio`：仅调用 `R_StudioDrawSubmodel(...)`（走统计逻辑）
5. 否则：
   - `R_StudioDrawRenderDataBegin(pRenderData)` → `R_StudioDrawSubmodel(...)` → `R_StudioDrawRenderDataEnd()`

---

## 5. RenderDataBegin/Submodel/Mesh：从 SubModel 到 glDrawElements

### 5.1 Begin：上传 Studio UBO + 绑定 VAO

- `Plugins/Renderer/gl_studio.cpp:2127` `R_StudioDrawRenderDataBegin(pRenderData)`
  - 组装并上传 `studio_ubo_t`：
    - 颜色/透明度/缩放：根据 `renderfx`/Glow/Outline/`rendermode`/`r_blend` 决定
    - 光照参数：`r_ambientlight` / `r_shadelight` / `r_plightvec`
    - 旧式 ELights（可选）：`r_studio_legacy_elight`
    - 骨骼矩阵：`memcpy(StudioUBO.bonematrix, *pbonetransform, 128 * mat3x4)`
    - LowerBody clipbone：`R_IsRenderingClippedLowerBody()` 时生成 `r_clipbone` bitmask
  - `GL_UploadSubDataToUBO` 后 `glBindBufferBase(...BINDING_POINT_STUDIO_UBO...)`
  - `GL_BindVAO(pRenderData->hVAO)`

### 5.2 Submodel：找到 VBO SubModel 并逐 mesh 绘制

- `Plugins/Renderer/gl_studio.cpp:3233` `R_StudioDrawSubmodel(studiohdr, submodel, pRenderData)`
  - 用 `submodel_byteoffset` 在 `pRenderData->mSubmodels` 查找对应的 `CStudioModelRenderSubModel`
  - 通过 `R_StudioGetTextureHeaderSkinref(...)` 决定 `ptexturehdr/ptexture/pskinref`
  - 调用 `R_StudioDrawSubmodel(pRenderData, pRenderSubmodel, ...)` 遍历 mesh

### 5.3 Mesh：分析/绘制两条路径

- `Plugins/Renderer/gl_studio.cpp:3143` `R_StudioDrawMesh(...)`
  - 依据 `mstudiotexture.flags` 与当前实体状态（fullbright、glowshell、rendermode、celshade 开关等）生成 `flags`
  - `r_draw_analyzingstudio` → `R_StudioDrawMesh_AnalysisPass(...)`
    - 统计：`r_draw_hasalpha / hasadditive / hasface / hashair`（用于上层多 Pass 编排）
  - 否则 → `R_StudioDrawMesh_DrawPass(...)`

- `Plugins/Renderer/gl_studio.cpp:2378` `R_StudioDrawMesh_DrawPass(...)`（超核心）
  - 组合 `program_state_t StudioProgramState`：
    - ShadowView / GlowStencil / GlowColor / GlowShell / Outline / HairShadow
    - 透明策略：
      - `r_draw_opaque` 阶段会跳过 `STUDIO_NF_ALPHA/ADDITIVE` mesh（并在上层延后到透明 pass）
      - 透明 pass 才启用 `STUDIO_ALPHA_BLEND_ENABLED / STUDIO_ADDITIVE_BLEND_ENABLED`
    - Legacy 动态光/实体光：`r_studio_legacy_dlight / r_studio_legacy_elight`
    - LowerBody near-plane clip、multiview、linear-depth、water/portal clip 等
  - 选择并启用 shader 变体：`R_UseStudioProgram(StudioProgramState, &prog)`（内部按 state 缓存 program）
  - 绑定纹理单元（diffuse/normal/parallax/specular/animated/stencil/shadow-diffuse 等）并设置大量 uniform（celshade/outline/hair/packed skin 等）
  - `glDrawElements(GL_TRIANGLES, ...)`
  - Restore：解绑纹理、恢复 depth/blend/cull/stencil 状态

---

## 6. RenderData 的创建/缓存/异步加载

### 6.1 模型加载时：可选预创建

- `Plugins/Renderer/gl_model.cpp:97` `Mod_LoadStudioModel(model_t* mod, void* buffer)`
  - 调用原始 `gPrivateFuncs.Mod_LoadStudioModel`
  - 若 `r_studio_lazy_load == 0`：立即 `R_CreateStudioRenderData(mod, studiohdr)`
  - 否则：仅当缓存中已存在该模型的 RenderData 才 reload

### 6.2 首次绘制时：慢路径创建

- `R_GLStudioDrawPoints()` → `R_GetStudioRenderDataFromStudioHeaderSlow()`
  - 找到 `mod` 后调用 `R_CreateStudioRenderData(mod, Mod_Extradata(mod))`

### 6.3 RenderData 内容与异步任务

- `Plugins/Renderer/gl_studio.cpp:4878` `R_CreateStudioRenderData(mod, studiohdr)`
  - 使用 `R_CalcStudioHeaderHash` 判断是否可复用旧 `CStudioModelRenderData`
  - 载入：TextureModel / 外部配置（celshade/lowerbody/texture flags/替换贴图等）
  - 启动异步加载：`R_CreateStudioRenderDataAsyncLoadTask(...)`

- `Plugins/Renderer/gl_studio.cpp:4830` `R_CreateStudioRenderDataAsyncLoadTask(...)`
  - `r_studio_parallel_load > 0`：后台线程任务
  - 否则：同步 RunTask 后把 `AsyncUploadResouce()` 排到游戏线程执行

> `R_GLStudioDrawPoints` 会在 `pRenderData->hVAO` 未 ready 时直接 return，因此首次出现“模型不显示/延迟一帧出现”通常与异步上传时机有关。

---

## 7. BoneCache：支持多 Pass 的关键优化

- `StudioSetupBones_Template`（`Plugins/Renderer/gl_studio.cpp:3960`）：
  - 若开启 `r_studio_bone_caches` 且不是 viewmodel/lowerbody：
    - 以 `CStudioBoneCacheHandle(soundtable, sequence, gaitsequence, frame, origin, angles)` 为 key
    - 命中则直接 memcpy 回 `*pbonetransform / *plighttransform`，避免重复计算

- `StudioSaveBones_Template`（`Plugins/Renderer/gl_studio.cpp:3996`）：
  - 调用原始 SaveBones 后，把本次骨骼矩阵存入 `g_StudioBoneCacheManager[handle]`

- `StudioMergeBones_Template`（`Plugins/Renderer/gl_studio.cpp:4033`）：
  - 同样复用缓存（若命中则直接复制矩阵，不再 merge）

---

## 8. 一句话总览（从实体到 GPU）

`R_DrawCurrentEntity` → `R_DrawStudioEntity` → `gpStudioInterface->StudioDrawModel/Player` →（hook）`StudioRenderModel_Template`（分析/多 Pass/延后）→（hook）`R_GLStudioDrawPoints`（按 submodel 调用）→ `R_StudioDrawRenderDataBegin`（UBO+VAO）→ `R_StudioDrawSubmodel`→ `R_StudioDrawMesh_DrawPass`（program_state 选 shader + 绑定贴图 + `glDrawElements`）→ `R_StudioDrawRenderDataEnd`

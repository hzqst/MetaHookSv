---
title: renderer-privatevars
type: reference
permalink: metahooksv/privatevars/renderer-privatevars
tags:
- renderer
- private-vars
- private-funcs
- private-globals
- symbol-locating
- signature-scan
- disasm
- vtable
- reference
---

# Game-private symbols used by `Renderer`

This document inventories the unexported engine (`hw.dll`) and client (`client.dll`) functions, vtable-indexed virtuals, global data slots and patched call sites that `Plugins/Renderer` locates and consumes. Symbol names are the plugin's local `gPrivateFuncs.*` fields and extern global pointers; parenthetical names describe the inferred engine-side role rather than official debug-symbol names.

## Scope and shared resolution process

- The scope covers the engine OpenGL/context layer, the world/view render pipeline (render-view, scene, setup, world surfaces, skybox, sprites, particles, decals, lightmaps), the engine and client Studio renderers, the engine VGUI2 `EngineSurface` virtual table, the engine `CVideoMode` startup-graphics functions, the Sven Co-op client portal manager and CS/CZ client globals, plus the engine/client global data slots those paths read.
- **Gamedata-first (2026-09-14 migration).** Every symbol that the gamedata catalog provides is resolved exclusively through `g_pMetaHookAPI->ResolveGameSymbol` via the shared helper `GamedataResolvePtr(moduleBase, symbolName, kind)` (`Plugins/Renderer/plugins.h`, fatal `Sys_Error` on failure — same policy as `Sig_*NotFound`). The old signature/string/disasm locators and their `#define` catalogues for those symbols were deleted. Resolved from gamedata (see the "Gamedata-resolved symbols" section for the full list): 15 engine functions, 16 engine globals, 7 client globals, 7 client `virtualFunction` records and 7 engine `R_Studio*` functions. Everything not listed there is still located by scanning the engine/client images.
- Public MetaHook APIs, saved engine interfaces, and ordinary plugin state (`g_EngineDLLInfo`, `g_MirrorEngineDLLInfo`, `g_ClientDLLInfo`, `g_MirrorClientDLLInfo`, `g_iEngineType`, `g_dwEngineBuildnum`, `g_pFileSystem`) are excluded. So are symbols obtained from public interface tables — `gEngfuncs.*` (including `pTriAPI`, `pEfxAPI`, `pfnSetFilterMode`, `pfnSetFilterColor`, `pfnSetFilterBrightness`), `gExportfuncs`/`pExportFuncs`, and the `pstudio->*` `engine_studio_api_t` — even where they are copied into `gPrivateFuncs` or used as scan anchors; they are listed in the boundary section.
- **Locating is centralised.** Every locator lives in `Plugins/Renderer/gl_hooks.cpp` (`Engine_FillAddress_*` at lines 495–12329, dispatched by `Engine_FillAddress` at 12330; `Client_FillAddress_*` at 13212–14137), `Plugins/Renderer/EngineSurfaceHook.cpp` (`EngineSurface_FillAddress` at 1480) and `Plugins/Renderer/exportfuncs.cpp` (`EngineStudio_FillAddress` at 988, `ClientStudio_FillAddress` at 1767). `Plugins/Renderer/VideoMode.cpp` is a 27-line stub whose `VideoMode_FillAddress`/`InstallHooks`/`UninstallHooks` have empty bodies; the `CVideoMode_Common`/`CGame_DrawStartupVideo` symbols are actually resolved in `gl_hooks.cpp`. The remaining source files (`gl_rmain.cpp`, `gl_wsurf.cpp`, `gl_studio.cpp`, `gl_draw.cpp`, …) contain **no** locator primitives — they only define hook handler bodies and consume the extern slots.
- **Two module images.** Scans run on the scan image `DllInfo` (the mirror image `g_MirrorEngineDLLInfo` / `g_MirrorClientDLLInfo` when `ImageBase != 0`, otherwise the real image). Results are mapped to the real image with `ConvertDllInfoSpace(addr, Src, Target)` (`gl_hooks.cpp`), which returns `Target.ImageBase + (addr - Src.ImageBase)` when `addr` is inside `Src.ImageBase..+ImageSize`, else `nullptr`. Some locators use the older two-step `Convert_VA_to_RVA` / `VA_from_RVA(name, RealDllInfo)`. `Engine_FillAddress(mirror ? mirror : real, real)` / `Client_FillAddress(mirror ? mirror : real, real)` are the entry calls (`plugins.cpp:87`, `:146`). Gamedata resolves always use the **real** module base (`RealDllInfo.ImageBase`); the retained disasm passes that are rooted at a gamedata-resolved function (`GL_Init`, `GL_Bind`, `GL_SelectTexture`) therefore walk the real image directly and take operand displacements as real addresses without `ConvertDllInfoSpace`.
- **Vtable resolution.** The client `CGameStudioRenderer` virtuals are resolved by name from gamedata `virtualFunction` records (API 112); `GetVFunctionFromVFTable(vftable, index, DllInfo, RealDllInfo, OutputDllInfo)` (`gl_hooks.cpp`) remains only for the engine `EngineSurface` virtuals (still hooked directly by hardcoded index via `VFTHook`); MetaHook API 113 additionally exposes `MH_GAMESYMBOL_KIND_VTABLE` so a vtable record (`GameStudioRenderer`) can be resolved to its `.rdata` array address, though Renderer currently consumes no vtable record.
- **Locator primitives.** `Search_Pattern` (`.text`), `Search_Pattern_Data`/`_Rdata` (string anchors), `Search_Pattern_From[_Size]` (bounded scan from an already-resolved function), `Search_Pattern_NoWildCard*`; `\x2A` is a wildcard byte. `ReverseSearchFunctionBegin[Ex]` recovers a function prologue from a body match. `g_pMetaHookAPI->DisasmRanges` (Capstone) walks a bounded instruction window to extract call targets (`E8`/`FF 15` imm), absolute memory operands (`[imm32]`), struct-offset operands (`[reg+disp]`), and `push imm32`/`mov reg,imm` data-slot addresses. `GetCallAddress`/`GetNextCallAddr` read a relative call operand; `InlinePatchRedirectBranch` redirects a private call/jmp site (`E8`/`E9`).
- **Per-engine signatures.** Many `.text` signatures have `*_SIG_SVENGINE` / `_HL25` / `_NEW` / `_NEW2` / `_BLOB` / `_COMMON` variants selected by `g_iEngineType` (`ENGINE_SVENGINE`, `ENGINE_GOLDSRC_HL25`, `ENGINE_GOLDSRC`, `ENGINE_GOLDSRC_BLOB`); the `#define` catalogue is `gl_hooks.cpp:10-423`. Some symbols are inlined on certain engines (signature `""`) and are tracked by a plugin `*_inlined` flag instead.
- **Failure policy.** Missing required symbols call `Sig_FuncNotFound`/`Sig_NotFound`/`Sig_VarNotFound`/`Sig_AddrNotFound` → fatal `Sys_Error("Could not found: <name> … Engine buildnum: <n>")`. Several symbols are intentionally optional (engine-specific branches, `SCClientDLL001`-gated Sven client symbols, `CVideoMode`/`Draw_FillRGBABuf` on non-matching engines) and stay null without error.

## Gamedata-resolved symbols (2026-09-14 migration)

All of the following are resolved via `GamedataResolvePtr` (kind `FUNCTION` / `GLOBAL` / `VIRTUAL_FUNCTION`) against the **real** module base and are fatal when missing; their old signature-scan locators, sig `#define`s and thunk-disasm index derivations were deleted. The tables below override the "Resolution mechanism" cells of the inventory tables for these symbols.

**Engine functions (module `engine`, kind FUNCTION, 10/10 catalog versions):**

- `GL_Init`, `GL_Bind`, `GL_SelectTexture`, `GL_Set2D`, `GL_Finish2D`, `GL_BeginRendering`, `GL_EndRendering`, `Host_IsSinglePlayerGame` — the standalone locators `Engine_FillAddress_GL_Set2D/_GL_Finish2D/_GL_BeginRendering/_GL_EndRendering/_Host_IsSinglePlayerGame` were removed and the dispatch calls them inline; `GL_Init/_GL_Bind/_GL_SelectTexture` keep their wrapper functions because they still host retained disasm passes (below).
- `CGame_DrawStartupVideo` — HL25-only gate kept (`g_iEngineType == ENGINE_GOLDSRC_HL25`); other engines leave it null by design.
- `R_StudioDrawPlayer`, `R_StudioDrawModel`, `R_StudioRenderModel`, `R_StudioRenderFinal`, `R_StudioSetupBones`, `R_StudioMergeBones`, `R_StudioSaveBones` — the whole `ClientStudio_FillAddress_StudioDrawPlayer/_StudioDrawModel/_EngineStudioDrawPlayer` thunk-disasm machinery was deleted.

**Retained disasm roots (real-image):** `Engine_FillAddress_GL_Init` still extracts `gl_extensions` (when `!SDL_GL_GetProcAddress`), walking the gamedata-resolved function on the real image with `RealDllInfo` section bounds and assigning displacements verbatim (no `ConvertDllInfoSpace`). `_GL_Bind`'s `currenttexture` extraction was dropped by the 2026-09-18 follow-up (see below) and resolved from gamedata; `_GL_SelectTexture`'s `oldtarget` extraction was dropped by the 2026-09-19 one, which **deleted `oldtarget` entirely** (write-only slot) — `Engine_FillAddress_GL_SelectTexture` no longer exists.

**Engine globals (kind GLOBAL, 10/10):** `currententity`, `r_model`, `pstudiohdr`, `r_origin`, `g_ChromeOrigin` (deleted `EngineStudio_FillAddress_GetCurrentEntity/_SetRenderModel/_StudioSetHeader/_SetChromeOrigin`), `cl_viewentity` (deleted `_CL_ViewEntityVars`), `cl_max_edicts` + `cl_entities` (deleted `_CL_ReallocateDynamicData`), `cl_numvisedicts` + `cl_visedicts` (deleted `_VisEdicts`), `mod_known` + `mod_numknown` (deleted `_ModKnown`/`_Mod_NumKnown`), `gTempEnts` (deleted `_TempEntsVars`), `r_worldentity` + `cl_worldmodel` (replaced the candidate-disasm tail of `Engine_FillAddress_R_RenderView`), `cl_parsecount` (resolved at the top of `_R_DrawTEntitiesOnListVars`; the parsemod/parsecount disasm branches were removed while `r_blend`/`r_entorigin`/`ClientDLL_DrawTransparentTriangles` extraction remains).

**Client globals:** `g_iUser1` + `g_iUser2` (deleted `_CL_IsThirdPerson`; resolved unconditionally in `Client_FillAddress`), `g_PlayerExtraInfo` / `g_PlayerExtraInfo_CZDS` (deleted `_PlayerExtraInfo`; czeror picks the CZDS array), `g_bRenderingPortals_SCClient` + `g_ViewEntityIndex_SCClient` (deleted `_RenderingPortals`/`_ViewEntityIndex`; resolved inside the `SCClientDLL001` branch of `Client_FillAddress_SCClient`, the old `buildnum >= 10182` gate dropped), `g_pGameStudioRenderer`.

**Client virtualFunctions (kind VIRTUAL_FUNCTION, 14 versions):** `GameStudioRenderer_StudioDrawPlayer`, `_StudioDrawModel`, `_StudioRenderModel`, `_StudioRenderFinal`, `_StudioSetupBones`, `_StudioSaveBones`, `_StudioMergeBones`. The nine `GameStudioRenderer_*_vftable_index` fields were removed from `private_funcs_t`; `ClientStudio_FillAddress` is now a flat list of resolves and the final `Sys_Error` sanity check is gone (resolve failure is already fatal). The unused `GameStudioRenderer__StudioDrawPlayer` resolve and field were also removed: its BFS consumer no longer exists, and requiring it would fail on `czeror-8684` / `czeror-10210`, which publish no such symbol. `IPluginsV4::LoadEngine` requires MetaHook API 112 at runtime before using the gamedata APIs, matching the compile-time requirement.

## `Engine_FillAddress` dispatch order

`Engine_FillAddress` copies SDL2 exports (`GetProcAddress(GetModuleHandle("SDL2.dll"), …)`, excluded) and the public `gEngfuncs.pTriAPI->*` pointers, resolves `SvEngine_glewInit` from the engine export `_glewInit@0` (gated on the `SCEngineClient002` factory), then calls ~111 locators in a fixed order. Order is observable in the rare case two locators fill the same field. Highlights:

1. `EngineSurface_FillAddress`, `VideoMode_FillAddress` (stub).
2. Capability probes: `HasOfficialFBOSupport`, `HasOfficialGLTexAllocSupport`.
3. Context: `GL_Init`, `GL_SetMode`, `GL_Shutdown`, `GL_Bind`, `GL_SelectTexture`, `GL_LoadTexture2`, `R_CullBox`, `R_SetupFrame`.
4. View/scene: `R_SetupGL`, `R_RenderView`, `V_RenderView`, `R_RenderScene`, `R_NewMap`, `GL_LoadFilterTexture`, `GL_BuildLightmaps`, `R_BuildLightMap`, `R_AddDynamicLights`, `GL_Disable/EnableMultitexture`, `R_DrawSequentialPoly`, `R_TextureAnimation`, `R_DrawBrushModel`, `R_RecursiveWorldNode`, `R_DrawWorld`, `R_DrawViewModel`, `R_MarkLeaves`.
5. 2D: `GL_Set2D`, `GL_Finish2D`, `GL_BeginRendering`, `GL_EndRendering`, ~~`EmitWaterPolys`~~, `VID_UpdateWindowVars`, `Mod_PointInLeaf`, `R_DrawTEntitiesOnList`, `BuildGammaTable`.
6. Effects/Studio: `R_DrawParticles`, ~~`CL_AllocDlight`~~, ~~`CL_AllocElight`~~, `R_GLStudioDrawPoints`, `R_StudioLighting`, ~~`R_StudioChrome`~~, ~~`R_LightLambert`~~, `R_StudioSetupSkin`, `Host_ClearMemory`, `Cache_Alloc`, ~~`Draw_MiptexTexture`~~, `Draw_DecalTexture`, `R_GetSpriteFrame`, ~~`R_DrawSpriteModel`~~, ~~`R_LightStrength`~~, ~~`R_RotateForEntity`~~, `R_GlowBlend`, `SCR_BeginLoadingPlaque`, `Host_IsSinglePlayerGame`, `Mod_UnloadSpriteTextures`, `Mod_LoadSpriteModel`, `Mod_LoadSpriteFrame`, ~~`R_AddTEntity`~~, `Hunk_AllocName`.
7. Globals passes: `GL_EndRenderingVars`, `VisEdicts`, `R_AllocTransObjectsVars`, `R_RenderFinalFog`, `R_DrawTEntitiesOnListVars`, `R_RecursiveWorldNodeVars`, `R_LoadSkybox`, `GL_FilterMinMaxVars`, `ScrFov`, `RenderSceneVars`, `RenderSceneVars2`, `CL_IsDevOverviewModeVars`, `R_DecalInit`, `R_RenderDynamicLightmaps`, ~~`R_StudioChromeVars`~~, `CL_ViewEntityVars`, `CL_ReallocateDynamicData`, `TempEntsVars`, `WaterVars`, `ModKnown`, `Mod_NumKnown`, `Mod_LoadStudioModel`, `Mod_LoadBrushModel`, `Mod_LoadModel`, `BasePalette`, `SetFilterMode`, `SetFilterColor`, `SetFilterBrightness`, `MoveVars`, `MissingTexture`, `NoTexture`, `LegacyMultiTextureInit`, `PVSNode`.
8. VideoMode/draw: `DrawStartupGraphic`, `DrawStartupVideo`, `Draw_Frame`, `Draw_SpriteFrameHoles/Additive/Generic`, `Draw_FillRGBA/RGBABlend`, `Draw_FillRGBABuf`, `D_FillRect`, `Draw_Pic`.

## Engine-private functions

### OpenGL context, textures and 2D

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.GL_Init` (`GL_Init`) | `void (*)(void)` | `Engine_FillAddress_GL_Init`: `.text` `68 00 1F 00 00 FF` (`push 0x1F00; call/jmp`); `ReverseSearchFunctionBeginEx(+0x80)` accepts prologue `83 EC 14`/`55 8B EC 83 EC`/`90 68 00 1F 00 00`; `DisasmRanges(+0x120)` requires a `PUSH` string `"Failed to query GL vendor"` (`GL_VENDOR: %s`). | `Install_InlineHook(GL_Init)`; plugin wrapper calls original then `glewInit`/`SDL_InitGL`, resets `(*gl_extensions)`. |
| `gPrivateFuncs.GL_SetMode_SvEngine` / `GL_SetMode_GoldSrc` / `GL_SetModeLegacy` | `qboolean (*)(void*,HDC*,HGLRC*)` / `qboolean (*)(void*,HDC*,HGLRC*,int fD3D,const char* pszDriver,const char* pszCmdLine)` / same six-arg as `GL_SetMode_GoldSrc` | `Engine_FillAddress_GL_SetMode`: per-engine `GL_SETMODE_SIG_*`. SvEngine (true 3-arg ABI) → `GL_SetMode_SvEngine`; GOLDSRC_HL25 and GOLDSRC with `SDL_GL_GetProcAddress` → `GL_SetMode_GoldSrc` (SDL builds still keep the six-arg ABI; `fD3D`/`pszDriver`/`pszCmdLine` are forwarded to the private GL loader); GOLDSRC non-SDL and BLOB → `GL_SetModeLegacy`. | `Install_InlineHook(GL_SetMode_SvEngine)` / `(GL_SetMode_GoldSrc)` / `(GL_SetModeLegacy)` (+`GL_SelectPixelFormat` with legacy); wrappers in `gl_rmain.cpp` share `GL_SetMode_Internal`. |
| `gPrivateFuncs.GL_SelectPixelFormat` | `qboolean (*)(HDC)` | Same locator when legacy: `GL_SELECTPIXELFORMAT_SIG_BLOB` (`A1 … 56 85 C0 57 0F 85 … C7 05`). | `Install_InlineHook(GL_SelectPixelFormat)`; plugin stub returns `true`, never calls original. |
| `gPrivateFuncs.GL_Shutdown` + `Sys_ShutdownGame_call_GL_Shutdown` | `void (*)(void)` + call site | `Engine_FillAddress_GL_Shutdown`: per-engine `GL_SHUTDOWN_SIG_*`; VA advanced to the trailing `E8`; `GL_Shutdown = GetCallAddress(that)`, the call-site address stored separately. | **Not hooked**; `Engine_InstallHooks` → `InlinePatchRedirectBranch(Sys_ShutdownGame_call_GL_Shutdown, GL_Shutdown, NULL)`. |
| `gPrivateFuncs.GL_Bind` | `void (*)(int texnum)` | `Engine_FillAddress_GL_Bind`: per-engine `GL_BIND_SIG_*`. | `Install_InlineHook(GL_Bind)`; handler re-implements bind (original call commented out) and calls `GL_SelectTexture`. Also an equality anchor in the texture-alloc redirect. |
| `gPrivateFuncs.GL_SelectTexture` | `void (*)(GLenum)` | `Engine_FillAddress_GL_SelectTexture`: SVEngine/HL25 `Search_Pattern_From(GL_Bind, *_SIG_*)`; GoldSrc `_NEW`/`_NEW2`; BLOB `_BLOB`. | Not hooked; called by the plugin `GL_SelectTexture`. |
| `gPrivateFuncs.GL_LoadTexture2` | `int (*)(char* name,int type,int w,int h,byte* data,qboolean mipmap,int palType,byte* pPal,int filter)` | `Engine_FillAddress_GL_LoadTexture2`: SVEngine anchor `"NULL Texture\n"`, others `"Texture Overflow: MAX_GLTEXTURES"` → `68 <str> E8 [83 C4 04]`; `ReverseSearchFunctionBeginEx(+0x500)`; fallback `GL_LOADTEXTURE2_SIG_*`; re-based via `Convert_VA_to_RVA`/`VA_from_RVA`. | `Install_InlineHook(GL_LoadTexture2)`; also the scan start for `R_CullBox`. |
| `gPrivateFuncs.realloc_SvEngine` (`realloc`) | `void* (*)(void*, size_t)` | SVEngine only: within `+0x50` of the max-textures match, `51 E8 ?? ?? ?? ?? 83 C4 08` → `GetCallAddress(addr+1)`. | Grows the SvEngine texture array (`gl_draw.cpp:1355`). |
| `gPrivateFuncs.GL_UnloadTexture` | `void (*)(const char*)` | Derived inside `Engine_FillAddress_R_StudioSetupSkin`: an `E8` near `PUSH/MOV [reg+0x120]`. | Resolved only (hook declared but never installed). |
| `gPrivateFuncs.GL_UnloadTextures` | `void (*)(void)` | `Engine_FillAddress_R_NewMap`: last 5-byte `E8` before `RET` in the `R_NewMap` body. | `Install_InlineHook(GL_UnloadTextures)`; wrapper `gl_draw.cpp:661`. |
| `gPrivateFuncs.GL_LoadFilterTexture` | `void (*)(void)` | `Engine_FillAddress_GL_LoadFilterTexture`: sig-only, per-engine `GL_LOADFILTERTEXTURE_SIG_*`. | `Install_InlineHook(GL_LoadFilterTexture)`; wrapper `gl_draw.cpp:722`. |
| `gPrivateFuncs.GL_BuildLightmaps` | `void (*)(void)` | Call #4 of the four consecutive `E8` in `R_NewMap`; standalone `Engine_FillAddress_GL_BuildLightmaps` uses `GL_BUILDLIGHTMAPS_SIG_*`. | `Install_InlineHook(GL_BuildLightmaps)`; wrapper `gl_rsurf.cpp:365`. |
| `gPrivateFuncs.BuildGammaTable` | `void (*)(float gamma)` | `Engine_FillAddress_BuildGammaTable`: non-SvEngine `00 00 20 40 E8` (`2.0f; call`, target in `.text`); SvEngine no inline; fallback `BUILDGAMMATABLE_SIG_*`. | `Install_InlineHook(BuildGammaTable)`; wrapper fills `texgammatable`. |
| `gPrivateFuncs.GL_Set2D` / `GL_Finish2D` | `void (*)(void)` | `Engine_FillAddress_GL_Set2D`/`_GL_Finish2D`: sig-only per engine (`GL_SET2D_SIG_*` / `GL_FINISH2D_SIG_*`); HL25 `Set2D` adds `VA += 1`. | `Install_InlineHook(GL_Set2D)` / `(GL_Finish2D)`. |
| `gPrivateFuncs.GL_BeginRendering` / `GL_EndRendering` | `void (*)(int*,int*,int*,int*)` / `void (*)(void)` | `Engine_FillAddress_GL_BeginRendering` sig-only; `_GL_EndRendering` prefers `GL_ENDRENDERING_SIG_COMMON_GOLDSRC` + reverse-search, else per-engine (GoldSrc picks `_NEW` when `g_bHasOfficialFBOSupport` else `_BLOB`); requires `GL_BeginRendering`. | `Install_InlineHook(GL_BeginRendering)` / `(GL_EndRendering)`; handlers call the originals. |
| `gPrivateFuncs.LegacyMultiTextureInit` (`CheckMultiTextureExtensions` / `InitMultitexturing` / `DT_Initialize`) | `void (*)(void)` | `Engine_FillAddress_LegacyMultiTextureInit`: gamedata, first of the three the identity publishes. | `Install_InlineHook(LegacyMultiTextureInit)` (empty wrapper). |
| `gPrivateFuncs.SDL_InitGL` (engine SDL2 wrapper) | `void (*)(void)` | `Engine_FillAddress_GL_SetMode` BFS walk (GoldSrc/HL25 + `SDL_GL_GetProcAddress`): last direct `E8` before a `push 0x1F03` (`GL_EXTENSIONS`). | Called by the plugin's `GL_SetMode_SvEngine`/`GL_SetMode_GoldSrc` wrappers (via `GL_SetMode_Internal`). |
| `gPrivateFuncs.SvEngine_glewInit` (`_glewInit@0`) | `decltype(glewInit)*` | `GetProcAddress(GetEngineModule(), "_glewInit@0")`, gated on `SCEngineClient002`. | Called during engine GL init. |

### Render view / scene / frame

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.R_RenderView` / `R_RenderView_SvEngine` | `void (*)(void)` / `void (*)(int viewIdx)` | `Engine_FillAddress_R_RenderView`: string `"R_RenderView: NULL worldmodel"` → `75 2A 68 <str>`; `ReverseSearchFunctionBeginEx(+0x100)`; fallback `R_RENDERVIEW_SIG_*`. SvEngine stores `_SvEngine`, others `R_RenderView`. Also yields `c_*_polys`, `r_worldentity`, `cl_worldmodel`. | `Install_InlineHook` (engine-type branch); wrappers in `gl_rmain.cpp`. |
| `gPrivateFuncs.V_RenderView` | `void (*)(void)` | `Engine_FillAddress_V_RenderView`: `68 00 40 00 00 FF` (`push 4000h` glClear mask) → `DisasmRanges(+5,+0x120)` call to `R_RenderView` → `ReverseSearchFunctionBeginEx(+0x300)`; fallback `V_RENDERVIEW_SIG_*`. Also yields `cls_state`, `cls_signon`, `r_soundOrigin`, `r_playerViewportAngles`. | Wrapper `V_RenderView` calls original but is not hooked. |
| `gPrivateFuncs.R_RenderScene` | `void (*)(void)` | `Engine_FillAddress_R_RenderScene`: `DisasmRanges(R_RenderView,+0x500)` — if a callee is `R_SetupGL` the plugin sets `R_RenderScene_inlined`; else the callee that calls `R_SetupGL` is `R_RenderScene`; fallback `R_RENDERSCENE_SIG_*`. | Resolved only; search base for fog/render-scene var passes. |
| `gPrivateFuncs.R_SetupGL` | `void (*)(void)` | `Engine_FillAddress_R_SetupGL`: `68 E2 0B 00 00 FF … 68 C0 0B … 68 71 0B …` (glDisable/glEnable caps) + `ReverseSearchFunctionBeginEx(+0x600)`; fallback `R_SETUPGL_SIG_*` (buildnum-gated SVENGINE/HL25 variants). Also yields the matrices below. | Resolved only; anchor for `R_RenderScene`. |
| `gPrivateFuncs.R_SetupFrame` (+ `R_SetupFrame_inlined`) | `void (*)(void)` | `Engine_FillAddress_R_SetupFrame`: SVEngine/HL25 set `R_SetupFrame_inlined`; else `R_SETUPFRAME_SIG_NEW/_NEW2/_BLOB`. | Resolved only (plugin reimplements). |
| `gPrivateFuncs.R_ForceCVars` / `R_CheckVariables` / `R_AnimateLight` | `void (*)(qboolean mp)` / `void (*)(void)` / `void (*)(void)` | Same locator: `R_SETUPFRAME_CALL_SIG` `0F 9F C0 50 E8 … E8 … E8` gives call #1/#2/#3; `_SIG2` gives only `R_CheckVariables`/`R_AnimateLight` and sets `R_ForceCVars_inlined`. | `Install_InlineHook(R_ForceCVars)`; wrappers call the originals. |
| `gPrivateFuncs.R_NewMap` (+ `R_ClearParticles`,`R_DecalInit`,`V_InitLevel`) | `void (*)(void)` | `Engine_FillAddress_R_NewMap`: string `"Setting up renderer...\n"` → `68 <str> E8`; first `E8` target; fallback `R_NEWMAP_SIG_*`. `R_NEWMAP`-body four consecutive `E8` give `R_ClearParticles`/`R_DecalInit`/`V_InitLevel`/`GL_BuildLightmaps`. | `Install_InlineHook(R_NewMap)`; others resolved only. |
| `gPrivateFuncs.R_PolyBlend` / `V_FadeAlpha` | `void (*)(void)` / `float (*)(void)` | `Engine_FillAddress_R_PolyBlend`: per-engine `R_POLYBLEND_*`; `DisasmRanges(+0x100)` first `E8` → `V_FadeAlpha`. | Not hooked; plugin reimplements `R_PolyBlend`, calls `V_FadeAlpha`. |
| `gPrivateFuncs.S_ExtraUpdate` | `void (*)(void)` | `Engine_FillAddress_S_ExtraUpdate`: per-engine `S_EXTRAUPDATE_*`. | Resolved only; called by the plugin path. |
| `gPrivateFuncs.R_MarkLeaves` | `void (*)(void)` | Sig-only `R_MARKLEAVES_SIG_*`; also yields `r_viewleaf`, `r_oldviewleaf`. | Resolved only (plugin reimplements). |
| `gPrivateFuncs.R_DrawViewModel` | `void (*)(void)` | `Engine_FillAddress_R_DrawViewModel`: SVEngine inlined; else in `R_RenderView+0x1000`, three consecutive `E8` where call #2 = `R_PolyBlend`, call #3 = `S_ExtraUpdate` → call #1. Also yields `envmap`, `cl_stats`, `cl_weaponstarttime`, `cl_weaponsequence`, `cl_light_level`. | Resolved only (plugin reimplements). |
| `gPrivateFuncs.R_DrawParticles` / `R_FreeDeadParticles` / `R_TracerDraw` / `R_BeamDrawList` | `void (*)(void)` / `void (*)(particle_t**)` / `void (*)(void)` / `void (*)(void)` | `Engine_FillAddress_R_DrawParticles`: `83 C4 04 68 C0 0B 00 00` + `DisasmRanges(+0x100)` requiring `PUSH 0x2200/0x2300` and `PUSH 0x302/0x303` + reverse-search; fallback `R_DRAWPARTICLES_SIG_*`. `R_FreeDeadParticles` = `MOV ESI,[active_particles]` preceded by `E8`; `R_TracerDraw`/`R_BeamDrawList` from inline `R_TRACERDRAW_SIG` (`GetCallAddress(addr+6)`/`addr+11`). | Not hooked; plugin `R_DrawParticles` calls `R_FreeDeadParticles`/`R_TracerDraw`/`R_BeamDrawList`. |
| ~~`gPrivateFuncs.R_AddTEntity`~~ | `void (*)(cl_entity_t*)` | ~~`Engine_FillAddress_R_AddTEntity`: SVEngine string `"Can't add transparent entity. Too many"` + `50 68 <str> E8`; others `"AddTentity: Too many objects"` + `68 <str> E8`; `ReverseSearchFunctionBegin(+0x50)`.~~ | Deleted 2026-09-20 — never called, never hooked; the locator's own comment already said "engine's R_AddTEntity is not used by Renderer anymore" (see last section). |
| ~~`gPrivateFuncs.R_AddDynamicLights`~~ | `void (*)(msurface_t*)` | ~~BFS call-site walk rooted at `R_BuildLightMap` matching `PUSH reg; E8; 83 C4 04`; fallback `R_ADDDYNAMICLIGHTS_SIG_*`~~ | Deleted 2026-09-19 — never called, never hooked (see last section). |
| ~~`gPrivateFuncs.R_BuildLightMap`~~ | `void (*)(msurface_t*, byte*, int)` | ~~string `"Error: lightmap for texture %s too large"` → `68 <str> E8 83 C4 18` + `ReverseSearchFunctionBeginEx(+0x300)`; fallback `R_BUILDLIGHTMAP_SIG_*`~~ | Deleted 2026-09-19 — its only consumer was the `R_AddDynamicLights` BFS root (see last section). |
| ~~`gPrivateFuncs.R_RenderDynamicLightmaps`~~ | `void (*)(msurface_t*)` | ~~Found during the `R_DrawSequentialPoly` BFS (callee with trailing imm `0x14` and `PUSH 0x200`), and re-resolved by `Engine_FillAddress_R_RenderDynamicLightmaps` (per-engine `R_RENDERDYNAMICLIGHTMAPS_SIG_*`) while null. Also yields `d_lightstylevalue`, `lightmap_polys`, `lightmap_modified`.~~ | Deleted 2026-09-19 — never called; it anchored a BFS for two write-only globals (see last section). |
| `gPrivateFuncs.R_TextureAnimation` | `texture_t* (*)(msurface_t*)` | Sig-only `R_TEXTUREANIMATION_SIG_*`; also yields `rtable`. | Resolved only. |
| `gPrivateFuncs.R_DrawSequentialPoly` / `R_DrawSequentialPoly_HL25` | `void (*)(msurface_t*, int)` / `void (*)(msurface_t*, int, qboolean cleanUpShaderState)` | Sig-only `R_DRAWSEQUENTIALPOLY_SIG_*`; HL25 stores its own three-arg field (HL25 callers push a third 32-bit bool and the callee reads it to gate shader/program cleanup); also root of the lightmap/decal BFS (`lightmap_textures`, `lightmap_rectchange`, `lightmaps`, `gDecalSurfs`, `gDecalSurfCount`). The HL25 field is also the disassembly anchor for `R_RecursiveWorldNode` and `R_DrawWorld`. | Resolved only. |
| `gPrivateFuncs.R_RecursiveWorldNode` / `R_RecursiveWorldNode_HL25` | `void (*)(mnode_t*)` / `void (*)(mnode_t*, qboolean cleanUpShaderState)` | SvEngine sig; HL25 from `R_DrawSequentialPoly_HL25` (two-arg ABI, the callee propagates the second arg through recursion and into `R_DrawSequentialPoly`'s `cleanUpShaderState`); GoldSrc/BLOB from `R_DrawBrushModel`; fallback `R_RECURSIVEWORLDNODE_SIG_*`. Also yields `r_framecount`/`r_visframecount` (the Vars BFS anchors the per-engine field); `skychain`/`waterchain` were dropped from the same BFS on 2026-09-19 (see last section). | Called by the plugin wrapper (`gl_rsurf.cpp` `R_RecursiveWorldNode`); the HL25 branch forwards `true` (engine default, `gl_reduce_shader_changes == 0`). |
| `gPrivateFuncs.R_DrawWorld` | `void (*)(void)` | `Engine_FillAddress_R_DrawWorld`: `68 B8 0B 00 00 8D` + `DisasmRanges(+5)` needing `LEA [ebp/esp+disp]` + `6A 00` + `ReverseSearchFunctionBeginEx(+0x300)`; fallback `R_DRAWWORLD_SIG_*`. Also yields `modelorg`. | Resolved only (plugin reimplements). |
| `gPrivateFuncs.R_DrawBrushModel` | `void (*)(cl_entity_t*)` | Sig-only `R_DRAWBRUSHMODEL_SIG_*`. | Resolved only (base for `R_RecursiveWorldNode`/`R_DrawWorld`). |
| ~~`gPrivateFuncs.EmitWaterPolys`~~ | `void (*)(msurface_t*, int)` | ~~Sig-only `EMITWATERPOLYS_SIG_*`.~~ | Deleted 2026-09-21 — resolved, never called, never hooked (see last section). |
| `gPrivateFuncs.Mod_PointInLeaf` | `mleaf_t* (*)(vec3_t, model_t*)` | `Engine_FillAddress_Mod_PointInLeaf`: string `"Mod_PointInLeaf: bad model\0"` → `68 <str> E8 83 C4 04` + `ReverseSearchFunctionBeginEx(+0x100)`; fallback `MOD_POINTINLEAF_SIG_*`. | `Install_InlineHook(Mod_PointInLeaf)`. |
| `gPrivateFuncs.PVSNode` (`R_PVSNode`) | `mnode_t* (*)(mnode_t*, vec3_t, vec3_t)` | `Engine_FillAddress_PVSNode`: `FF B0 A4 00 00 00 E8 ?? ?? ?? ?? 83 C4 0C` → `GetCallAddress(addr+6)`; fallback `PVSNODE_COMMON_GOLDSRC` (`addr+8`). | `Install_InlineHook(PVSNode)`. |
| `gPrivateFuncs.VID_UpdateWindowVars` | `void (*)(RECT*, int, int)` | `Engine_FillAddress_VID_UpdateWindowVars`: SVEngine sig then `Search_Pattern_From_Size(+0x50,"50 E8")`; else per-engine `VID_UPDATEWINDOWVARS_SIG_*`. Also yields `window_rect`. | Resolved only. |
| `gPrivateFuncs.R_DrawTEntitiesOnList` | `void (*)(int onlyClientDraw)` | `Engine_FillAddress_R_DrawTEntitiesOnList`: string `"Non-sprite set to glow"` → `68 <str> E8 8B` + `ReverseSearchFunctionBeginEx(+0x500)`; fallback `R_DRAWTENTITIESONLIST_SIG_*`. Also yields `r_blend`, `cl_parsecount`, `cl_frames`, `size_of_frame`, `r_entorigin`. | Resolved only (plugin reimplements). |

### Skybox, models, cache and memory

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.R_LoadSkys` / `R_LoadSkyBox_SvEngine` (plus the deleted `R_LoadSkyboxInt_SvEngine`) | `void (*)(void)` / `void (*)(const char*)` / ~~`qboolean (*)(const char*)`~~ | Gamedata FUNCTION resolution for `R_LoadSkys` (non-SvEngine) / `R_LoadSkyBox_SvEngine` (SvEngine), 2026-09-17. ~~`Engine_FillAddress_R_LoadSkybox`'s `"SKY: "` reverse-search, the SvEngine `gSkyTexNumber` BFS and the anchored vars BFS~~ deleted 2026-09-19 together with `R_LoadSkyboxInt_SvEngine` / `gSkyTexNumber` / `r_loading_skybox` (no consumers) — see last section. | `Install_InlineHook(R_LoadSkys)` / `(R_LoadSkyBox_SvEngine)`. |
| `gPrivateFuncs.Mod_LoadStudioModel` | `void (*)(model_t*, void*)` | String `"bogus\0"` → `68 <str> ?? E8` + `ReverseSearchFunctionBeginEx(+0x50)`. | `Install_InlineHook(Mod_LoadStudioModel)`; wrapper calls original. |
| `gPrivateFuncs.Mod_LoadBrushModel` | `void (*)(model_t*, void*)` | String `"Mod_LoadBrushModel: %s has wrong version number"` → `68 <str> 6A 01 E8`/`68 <str> E8` + reverse-search. | Resolved only (hook declared but never installed). |
| `gPrivateFuncs.Mod_LoadModel` | `model_t* (*)(model_t*, qboolean, qboolean)` | ~~String `"Loading '%s'\n"` (SvEngine) / `"loading %s\n"` → `68 <str> E8 83 C4` + reverse-search.~~ Gamedata FUNCTION resolution (2026-09-18); the old pass also yielded the now-deleted `loadname` / `loadmodel`. | Resolved only. |
| `gPrivateFuncs.Mod_LoadSpriteModel` / `Mod_LoadSpriteFrame` / `Mod_UnloadSpriteTextures` | `void (*)(model_t*,void*)` / `void* (*)(void*,mspriteframe_t**,int)` / `void (*)(model_t*)` | `Engine_FillAddress_Mod_LoadSpriteModel`: SVEngine string `"Sprite \"%s\" has wrong version number"`, others `"Mod_LoadSpriteModel: Invalid # of frame"` → `68 <str> E8 … 83 C4` + reverse-search (+0x100 SvEngine / +0x300 others); fallback `MOD_LOADSPRITEMODEL_*`. `Mod_LoadSpriteFrame` derived: callee in `+0x240` starting `PUSH 0x300`. ~~`Mod_UnloadSpriteTextures` sig-only `MOD_UNLOADSPRITETEXTURES_*`.~~ `Mod_UnloadSpriteTextures` gamedata FUNCTION resolution on every identity (2026-09-19); `MOD_UNLOADSPRITETEXTURES_{BLOB,SVENGINE}` deleted. Also yields `gSpriteMipMap`. | `Install_InlineHook(Mod_LoadSpriteModel)` / `(Mod_UnloadSpriteTextures)`; `Mod_LoadSpriteFrame` resolved only. |
| `gPrivateFuncs.Cache_Alloc` | `void* (*)(cache_user_t*, int, const char*)` | String `"Cache_Alloc: already allocated"` → `68 <str> E8 83 C4 04` + `ReverseSearchFunctionBeginEx(+0x80)`. Also yields `cache_head`. | Wrapper `zone.cpp:10` forwards to it. |
| `gPrivateFuncs.Hunk_AllocName` | `void* (*)(int, const char*)` | String `"Hunk_Alloc: bad size: %i"`; SvEngine `68 <str> 0F AE E8 E8 … 83 C4 08`, others `68 <str> E8 … 83 C4 08` + reverse-search; `Convert_VA_to_RVA`. | Wrapper `zone.cpp:5` forwards to it. |
| `gPrivateFuncs.Host_ClearMemory` | `void (*)(qboolean)` | String `"Clearing memory\n"` → `68 <str> E8 83 C4 04` + `ReverseSearchFunctionBeginEx(+0x80)`. | `Install_InlineHook(Host_ClearMemory)`; wrapper calls `Mod_ClearModel` then original. Not restored on uninstall. |

### Dynamic lights, decals, Studio and sprites

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| ~~`gPrivateFuncs.CL_AllocDlight` / `CL_AllocElight`~~ | `dlight_t* (*)(int key)` | ~~`GamedataResolvePtr` FUNCTION.~~ Both fields deleted 2026-09-21 (second pass, see last section) — write-only once the `r_dlightactive` walk went, and the plugin reaches these APIs through `gEngfuncs.pEfxAPI`. Their locators are gone; `cl_dlights` / `cl_elights` are now resolved inline in `Engine_FillAddress`. |
| `gPrivateFuncs.R_GLStudioDrawPoints` | `void (*)(void)` | `Engine_FillAddress_R_GLStudioDrawPoints`: `75 2A 68 44 0B 00 00 FF 15 …` + single-instruction `MOV [mem],1` + `ReverseSearchFunctionBeginEx(+0x1000)` (four prologue forms) + `DisasmRanges(+0x100)` requiring `[reg+0x54]` and `[reg+0x60]`; fallback `R_GLSTUDIODRAWPOINTS_SIG_*`. | `Install_InlineHook(R_GLStudioDrawPoints)`; handler re-implements, never calls original. |
| `gPrivateFuncs.R_StudioLighting` | `void (*)(float* lv, int bone, int flags, vec3_t normal)` | Sig-only `R_STUDIOLIGHTING_SIG_*`; also yields `r_ambientlight`, `r_shadelight`, `r_blightvec`, `r_plightvec`, `lightgammatable` (BFS). | Resolved only. |
| ~~`gPrivateFuncs.R_StudioChrome`~~ | `void (*)(int* pchrome, int bone, vec3_t normal)` | ~~Sig-only `R_STUDIOCHROME_SIG_*`~~ | Deleted 2026-09-19 — resolved, never called; its only consumer was the `R_StudioChromeVars` GOLDSRC disasm base (see last section). |
| ~~`gPrivateFuncs.R_LightLambert`~~ | `void (*)(float (*light)[4], float* normal, float* src, float* lambert)` | ~~Sig-only `R_LIGHTLAMBERT_SIG_*`.~~ | Deleted 2026-09-21 — resolved, never called, never hooked (see last section). |
| `gPrivateFuncs.R_StudioSetupSkin` / `R_StudioGetSkin` | `void (*)(studiohdr_t*, int)` / `skin_t* (*)(int keynum, int index)` | `Engine_FillAddress_R_StudioSetupSkin`: string `"DM_Base.bmp"` → `68 <str> C7 44 24 …` + `ReverseSearchFunctionBeginEx(+0x300)`; `R_StudioGetSkin` = `E8` target in `+0x800` whose body contains `CMP reg,0xB`; `GL_UnloadTexture` from the same walk. Also yields `tmp_palette`. | Resolved only. |
| ~~`gPrivateFuncs.Draw_MiptexTexture`~~ | `void (*)(cachewad_t*, byte*)` | ~~String `"Draw_MiptexTexture: Bad cached wad %s\n"` → `68 <str> E8` + `ReverseSearchFunctionBeginEx(+0x80)`; fallback `DRAW_MIPTEXTEXTURE_SIG_*`. Also yielded `gfCustomBuild`, `szCustName`.~~ | Deleted 2026-09-21 with `gfCustomBuild` / `szCustName` and the plugin-side `Draw_MiptexTexture`: the hook had been disabled in the 2025-04 texture-pipeline replacement, so the whole apparatus was unreachable (see last section). |
| `gPrivateFuncs.Draw_DecalTexture` | `texture_t* (*)(int index)` | `GamedataResolvePtr("Draw_DecalTexture", FUNCTION)`. The former string scan (`"Failed to load custom decal for player"`), its `DRAW_DECALTEXTURE_SIG_*` fallbacks (macros no longer in the tree) and the `decal_wad` / `Draw_CustomCacheGet` / `Draw_CacheGet` BFS (removed 2026-09-21, see last section) are all gone. | Wrapper `gl_draw.cpp:1808` forwards to it, and `R_DrawDecals` (`gl_rsurf.cpp:1109`) calls that wrapper — live. |
| `gPrivateFuncs.R_GetSpriteFrame` | `mspriteframe_t* (*)(msprite_t*, int)` | String `"Sprite:  no pSprite!!!"` → `68 <str> E8 83 C4` + `ReverseSearchFunctionBeginEx(+0x120)`; fallback `R_GETSPRITEFRAME_SIG`/`_SIG2`. | `Install_InlineHook(R_GetSpriteFrame)`; handler calls original from `R_SpriteLoadExternalFile_FrameTexture`. |
| ~~`gPrivateFuncs.R_DrawSpriteModel`~~ | `void (*)(cl_entity_t*)` | ~~String `"R_DrawSpriteModel:  couldn"` → `68 <str> E8 83 C4` + `ReverseSearchFunctionBeginEx(+0x300)`; fallback `R_DRAWSRPITEMODEL_SIG_*`.~~ | Deleted 2026-09-21 — resolved, never called or hooked; the plugin's own `R_DrawSpriteModel` (`gl_sprite.cpp`) is untouched (see last section). |
| ~~`gPrivateFuncs.R_LightStrength` (+ `R_LightStrength_inlined`)~~ | `void (*)(int bone, float* vert, float (*light)[4])` | ~~SVEngine `R_LIGHTSTRENGTH_SIG_SVENGINE` (+10152); HL25 inlined; GoldSrc `_NEW`/`_NEW2`; BLOB `_BLOB`.~~ | Deleted 2026-09-21 — resolved, never called, never hooked (see last section). |
| ~~`gPrivateFuncs.R_RotateForEntity`~~ | `void (*)(float*, cl_entity_t*)` | ~~GoldSrc inline `R_ROTATEFORENTITY_GOLDSRC` → `GetCallAddress(addr+len-1)`; fallback SVENGINE/HL25/NEW.~~ | Deleted 2026-09-20 — never called, never hooked (see last section). |
| `gPrivateFuncs.R_GlowBlend` (+ `R_GlowBlend_inlined`) | `float (*)(cl_entity_t*)` | SVEngine/HL25 inlined; GoldSrc `R_GLOW_BLEND_SIG_NEW`/`_NEW2`; BLOB `_BLOB`. | Wrapper calls original when non-null. |
| `gPrivateFuncs.SCR_BeginLoadingPlaque` | `void (*)(qboolean reconnect)` | Single sig `SCR_BEGIN_LOADING_PLAQUE` (`6A 01 E8 … A1 … 83 C4 04 83 F8 03`); also yields `scr_drawloading`. | Resolved only. |
| `gPrivateFuncs.Host_IsSinglePlayerGame` | `qboolean (*)(void)` | String `"setpause;"` → `68 <str> E8 … 83 C4` + `ReverseSearchPattern(+0x50,"55 8B EC E8",4)`; the `E8` followed by `85 C0` is the target; fallback `HOST_IS_SINGLE_PLAYER_GAME_*`. | Wrapper `gl_rmain.cpp:824`. |
| ~~`gPrivateFuncs.R_AddTEntity`~~ | `void (*)(cl_entity_t*)` | ~~See render-view table.~~ | Deleted 2026-09-20 (see last section). |
| ~~`gPrivateFuncs.R_RenderFinalFog`~~ | `void (*)(void)` | ~~SvEngine `R_RenderFinalFog_VA` is the `push 0B60h` instruction inside the render-view body; HL25/GoldSrc `GetCallAddress(addr+9)`.~~ | Deleted 2026-09-21 — write-only field; the five user-fog globals now resolve from gamedata. |
| `gPrivateFuncs.Draw_Frame` / `Draw_SpriteFrameHoles[_SvEngine]` / `Draw_SpriteFrameAdditive[_SvEngine]` / `Draw_SpriteFrameGeneric[_SvEngine]` / `Draw_FillRGBA` / `Draw_FillRGBABlend` / `Draw_FillRGBABuf` / `D_FillRect` / `Draw_Pic` | 2D draw helpers | `Engine_FillAddress_*`: gamedata-only, engine-gated where the identity set differs; `Draw_Frame` also yields `giScissorTest`, `scissor_x/y/width/height`; `Draw_FillRGBABuf` is SvEngine-only, `D_FillRect` is non-SvEngine-only. | `Install_InlineHook` each; handlers in `gl_rmain.cpp`. |

### Engine Studio renderer (located in `exportfuncs.cpp`)

Entry point `EngineStudio_FillAddress(pstudio, DllInfo, RealDllInfo)` (`exportfuncs.cpp:988`) calls eleven locators, each using a public `pstudio-><Member>` as a real→scan anchor followed by `DisasmRanges` collecting `.data` operands. No scan fallbacks; missing symbols are fatal.

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.CL_FxBlend` (engine entity alpha-blend) | `int (*)(cl_entity_t*)` | `EngineStudio_FillAddress_StudioSetRenderamt` (529): anchor `pstudio->StudioSetRenderamt`; first 5-byte `E8` → target. | `Install_InlineHook(CL_FxBlend)`; wrapper `gl_rmain.cpp:1030`. |
| `currententity` | `cl_entity_t**` | `_GetCurrentEntity` (143): `pstudio->GetCurrentEntity`; `DisasmRanges(+0x10)` first `MOV EAX,[.data]`. | Current entity for every Studio pass. |
| `r_framecount` / `cl_time` / `cl_oldtime` | `int*` / `double*` / `double*` | `_GetTimes` (199): `pstudio->GetTimes`; `DisasmRanges(+0x50)` collects `.data` candidates — `candidates[0]` = `r_framecount`, first `FLD`/`MOVSD` = `cl_time`, second = `cl_oldtime`. | Frame counter and client time. |
| ~~`r_model`~~ | `model_t**` | ~~`_SetRenderModel` (309): `pstudio->SetRenderModel`; `DisasmRanges(+0x10)` first `MOV [.data],reg`~~ | Deleted 2026-09-19 — resolved, never read (see last section). |
| `pstudiohdr` | `studiohdr_t**` | `_StudioSetHeader` (358): `pstudio->StudioSetHeader`; `DisasmRanges(+0x10)` first `MOV [.data],reg`. | Studio bone/header paths. |
| `g_ForcedFaceFlags` | `int*` | `_SetForceFaceFlags` (409): `pstudio->SetForceFaceFlags`; `DisasmRanges(+0x10)` first `MOV [.data],reg`. | `R_IsRenderingChrome`. |
| `r_topcolor` / `r_bottomcolor` | `int*` / `int*` | `_StudioSetRemapColors` (462): `pstudio->StudioSetRemapColors`; `DisasmRanges(+0x50)` first two distinct `.data` stores. | Skin remap invalidation. |
| `r_blend` | `float*` | Same as `CL_FxBlend` locator: `DisasmRanges(+0x50)` first `FSTP [abs]` (`base==0`). A second independent resolution exists in `gl_hooks.cpp:8460`; both `if (!r_blend)`-guarded. | Studio blend. |
| ~~`pauxverts`/`auxverts`, `pvlightvalues`/`lightvalues`~~ | ~~glow-shell vertex/light arrays~~ | ~~`_SetupRenderer` (585): `pstudio->SetupRenderer`; `DisasmRanges(+0x50)` first/second `C7 05 [imm32],imm32` (`len 10`), slot at `+2`, array base at `+6`.~~ | Deleted 2026-09-20 — write-only, never read (see last section). |
| `pbodypart` / `psubmodel` | `mstudiobodyparts_t**` / `mstudiomodel_t**` | `_StudioSetupModel` (652): `pstudio->StudioSetupModel`; `DisasmRanges(+0x50)` first/second `MOV [reg+0],imm32` with imm in `.data`. | `psubmodel` used in `R_StudioDrawSubmodel`; `pbodypart` resolved only. |
| `r_origin` / ~~`g_ChromeOrigin`~~ | `float*` / ~~`float*`~~ | `_SetChromeOrigin` (715): `pstudio->SetChromeOrigin`; `DisasmRanges(+0x50)` collects `FLD`/`MOV`/`MOVQ`/`MOV imm` and `MOV`/`MOVQ`/`FSTP` store candidates; `qsort` ascending, lowest wins. | `r_origin` heavily used; ~~`g_ChromeOrigin` resolved only~~ → deleted 2026-09-19 (resolved, never read). |
| `r_colormix` | `float*` (3 floats) | `_StudioSetupLighting` (870): `pstudio->StudioSetupLighting`; `DisasmRanges(+0x200)` arms after `AND reg,0xFF00`, collects `.data` stores, accepts last/first three 4-byte-consecutive. | Studio UBO colour. |

## Engine-private global variables

### View, matrices, refdef and scene state (`gl_hooks.cpp`)

| Local symbol / inferred game object | Declaration / type | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `r_world_matrix` / `r_projection_matrix` / `gWorldToScreen` / `gScreenToWorld` | `float*` | `Engine_FillAddress_R_SetupGL`: `r_world_matrix` from `68 <m> 68 A6 0B 00 00`; `r_projection_matrix` from `68 <m> 68 A7 0B 00 00`; `gWorldToScreen`/`gScreenToWorld` from `68 <a> 68 <b> E8 … 83 C4` (`+6`/`+1`). | Matrix stack, screen/world transforms. |
| `vertical_fov_SvEngine` | `qboolean*` | `R_SetupGL` (SvEngine): `50 FF 15 … 83 3D <slot> 00` → `*(addr+9)`. | `r_vertical_fov` default. |
| `r_refdef` (`r_refdef_SvEngine` / `r_refdef_GoldSrc`) | `refdef_t` / engine-layout pointers | `Engine_FillAddress_RenderSceneVars`: on `E8 … 68 <refdef> … 83`, the pushed imm at `address-4` is the engine refdef; SvEngine vs GoldSrc layout selected by engine type. | `R_GetRefDef()` and view/refdef consumers. |
| `gDevOverview` | `overviewInfo_t*` | `Engine_FillAddress_RenderSceneVars2`: `DisasmRanges(CL_SetDevOverView,+0x300)`; first `PUSH 0x30` after instruction 100 arms, then up to 6 `FLD`/`MOVSS [.data]` candidates; `qsort` ascending, lowest wins. | Camera/zoom math. |
| `cl_waterlevel` | `int*` | `Engine_FillAddress_RenderSceneVars2`: `CMP [.data],2` or `MOV reg,[.data]` then `CMP reg,2` within `+0x30`. | Fog / camera. |
| `cl_waterlevel` also set by `WaterVars`; `gWaterColor` / `cshift_water` | `colorVec*` / `cshift_t*` | `Engine_FillAddress_WaterVars`: HL25 `GWATERCOLOR_SIG_HL25` (`addr+4`), else `GWATERCOLOR_SIG` (`addr+2`); `cshift_water = gWaterColor_VA + 12`. | Water/fog colour. |
| `cl_simorg` | `vec_t*` | `Engine_FillAddress_CL_SimOrgVars`: per-engine patterns embedding the `[esi/edi+0B48h]` destination offsets; source slot at `addr+2`/`+4`. | Simulated origin. |
| `cl_viewentity` | `int*` | `Engine_FillAddress_CL_ViewEntityVars`: SvEngine `CL_VIEWENTITY_SIG_SVENGINE` ptr at `+10`; GoldSrc `A1 <disp> 48 3B ?` + `DisasmRanges(+0x100)` requiring `CMP [.data],0x200`, ptr at `+1`. | Third-person camera / entity lookup. |
| `scrfov` | `float*` | `Engine_FillAddress_ScrFov`: SvEngine `D9 05 <scrfov> D9 5C 24 1C …`; others `C7 05 <a> 00 00 16 43` (150.0f) then `C7 05 <scrfov> 00 00 20 41` (10.0f). | Viewmodel FOV. |
| `g_bUserFogOn` / `g_UserFogColor` / `g_UserFogDensity` / `g_UserFogStart` / `g_UserFogEnd` | `int*` / `float*` | `Engine_FillAddress_R_RenderFinalFog`: gamedata GLOBAL `g_bUserFogOn`, `flFinalFogColor`, `flFogDensity`, `flFogStart`, `flFogEnd`. | User-fog upload. |
| `cls_state` / `cls_signon` / `scr_drawloading` | `cactive_t*` / `int*` / `qboolean*` | `cls_state`/`cls_signon` from `V_RenderView` (`CMP [.data],5`/`,2`); `scr_drawloading` from `SCR_BeginLoadingPlaque` (`MOV [.data],1`). | Client state. |
| `r_soundOrigin` / `r_playerViewportAngles` | `vec_t*` | `V_RenderView`: zeroed-register stores plus `FLDZ`+`FST[P]` candidates; if six candidates, `qsort` → `[0]` and `[3]`. | `r_playerViewportAngles` used; `r_soundOrigin` resolved only. |
| `frustum` / `vpn` / `vup` / `vright` | `mplane_t*` / `vec_t*` | `Engine_FillAddress_R_CullBox`: `MOV ESI,imm(.data)` → `frustum`; then `68 <frustum> 68 … 68 … E8` pattern gives `vpn`/`vup`; `68 <vpn> 68 … 68 <frustum+0x28>` gives `vright`. | `R_SetFrustum` culling. |
| `envmap` / `cl_stats` / `cl_weaponstarttime` / `cl_weaponsequence` / `cl_light_level` | `int*` / `float*` | `Engine_FillAddress_R_DrawViewModel`: per-engine patterns, operand offsets differ by engine type. | Viewmodel/env-map selection. |
| `pmovevars` | `movevars_t*` | `Engine_FillAddress_MoveVars`: SvEngine `56 8B 74 24 08 6A 2C 56 E8 … D9 05`; others `E8 <MSG_ReadFloat> D9 1D <gravity> …`. | `zmax`, `skyName`. |
| `r_framecount` / `r_visframecount` | `int*` | `Engine_FillAddress_R_RecursiveWorldNodeVars`: `MOV reg,[reg+0]` (or `[reg+4]`) then `MOV/CMP reg,[.data]`. | Frame/leaf counters. |
| ~~`skychain` / `waterchain`~~ | `msurface_t**` | ~~Same walk: `TEST reg8,imm` `imm==4` → `skychain`, `imm==0x10` → `waterchain`.~~ | Deleted 2026-09-19 — write-only (see last section). |
| `r_viewleaf` / `r_oldviewleaf` | `mleaf_t**` | `Engine_FillAddress_R_MarkLeaves`: `MOV ECX,[.data]` / `MOV [.data],ECX`. | PVS tracking. |
| `r_entorigin` / `r_blend` / `cl_parsecount` / `cl_frames` / `size_of_frame` | `vec_t*` / `float*` / `int*` / `void*` / `int` | `Engine_FillAddress_R_DrawTEntitiesOnListVars`: `r_blend` after fog-disable; `cl_parsecount` = `MOV EAX,[abs]` whose live value is `63`; `cl_frames` = `LEA` within `+20`; `size_of_frame` = `IMUL imm 0x4000..0xF000`; `r_entorigin` after `MOVSX [reg+0x2E8]`. Defaults `size_of_frame=0x42B8` for buildnum ≤ 8684. | `R_GetPlayerState` and sprite attachment origin. |
| `rtable` | `int (*)[20][20]` | `Engine_FillAddress_R_TextureAnimation`: `MOV ESI,imm(.data)`. | Animated-texture random table. |
| `modelorg` | `vec_t*` | `Engine_FillAddress_R_DrawWorld`: `DisasmRanges(+0x130)` `MOV`/`MOVSS`/`FSTP [.data]` candidates, `qsort`, consecutive-run heuristic. | Resolved only. |
| `window_rect` | `RECT*` | `Engine_FillAddress_VID_UpdateWindowVars`: HL25 `MOVUPS [abs],xmm`; else `MOV [abs],reg` within `+0x40`. | `GL_EndRendering` destination rect. |
| `pmainwindow` | `void**` | `Engine_FillAddress_EngineSurface_pushMakeCurrent` (see EngineSurface). | Main window handle. |

### Textures, particles, sprites, temp-entities, edicts and models

| Local symbol / inferred game object | Declaration / type | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gl_extensions` | `const char**` | `Engine_FillAddress_GL_Init` (and `GL_SetMode` BFS): `push 0x1F03` (`GL_EXTENSIONS`) then `MOV [.data],EAX`. | GL extension reset. |
| `vid_d3d` / `currenttexture` / ~~`oldtarget`~~ | `float*` / `int*` | `GL_SetMode` BFS (`MOV [.data],0x3F800000`); `currenttexture` now gamedata GLOBAL. | `currenttexture` mirrored by the plugin; `vid_d3d` resolved only. `oldtarget` was **deleted 2026-09-19** (write-only since `a5bfd6a5`). |
| `gltextures` / `gltextures_SvEngine` / `maxgltextures_SvEngine` / `peakgltextures_SvEngine` / `numgltextures` / `allocated_textures` / `gHostSpawnCount` | texture-array bookkeeping | `Engine_FillAddress_GL_LoadTexture2` `DisasmRanges` (non-SvEngine: `MOV reg,[.data]`/`MOV reg,imm` after a self-`XOR`; SvEngine: `8B 15 … 8B 1D` for `gltextures_SvEngine`, `6B C1 54 89 0D` for `maxgltextures`, `03 35 … 3B 15` for `peakgltextures`, `66 8B …` for `gHostSpawnCount`). `allocated_textures` chosen from the trailing `MOV [.data],reg` when `!g_bHasOfficialGLTexAllocSupport`. | Texture enumeration/unload/growth. |
| `particletexture` / `active_particles` | `int*` / `particle_t**` | `Engine_FillAddress_R_DrawParticles`: first `PUSH [.data]` or `MOV reg,[.data]` not followed by `33 C5`/`33 C4`; `active_particles` = the `MOV ESI,[.data]` preceding `E8` that anchors `R_FreeDeadParticles`. | Particle rendering. |
| `gTempEnts` | `TEMPENTITY*` | `Engine_FillAddress_TempEntsVars`: SvEngine `68 00 E0 5F 00 6A 00 68 <gTempEnts> A3`, others `68 30 68 17 00 6A 00 68 <gTempEnts> E8`; ptr at `addr+8`. | Temp-entity index lookup. |
| `cl_dlights` / ~~`r_dlightactive`~~ / `cl_elights` | `dlight_t*` / `int*` / `dlight_t*` | GLOBAL `GamedataResolvePtr`, resolved inline in `Engine_FillAddress` since 2026-09-21. `r_dlightactive` was the last catalog-uncovered one, walked from `CL_AllocDlight` (after `PUSH 0x28`, a `PUSH imm(.data)` / `MOV reg,[.data]` / `OR [.data],1`) — deleted 2026-09-21, it had no reader. | Dynamic-light rendering. `cl_dlights` (`gl_light.cpp:1786`, `gl_rmain.cpp:5305`, `gl_studio.cpp:3517-3543`) and `cl_elights` (`gl_studio.cpp:2222`) are read. |
| ~~`decal_wad`~~ / ~~`gfCustomBuild`~~ / ~~`szCustName`~~ | `cachewad_t**` / `qboolean*` / `char (*)[10]` | ~~`Engine_FillAddress_Draw_DecalTexture` BFS~~ / ~~`_Draw_MiptexTexture` `DisasmRanges(+0x500)`~~. | Custom-WAD texture lookup. All three deleted 2026-09-21 (see last section). |
| ~~`gSkyTexNumber` / `r_loading_skybox`~~ | `int*` / `int*` | ~~`Engine_FillAddress_R_LoadSkybox`: `MOV reg,imm(.data)` validated by `CMP [reg],reg`/`PUSH [reg+disp]`; `MOV eax,[.data]` or `CMP [.data],0`~~ | Deleted 2026-09-19 — write-only, no consumers (see last section). |
| `giScissorTest` / `scissor_x` / `scissor_y` / `scissor_width` / `scissor_height` | `qboolean*` / `int*` | `Engine_FillAddress_Draw_Frame`: `MOV reg,[.data]`+`TEST`, or `CMP [.data],0`, or `CMP [.data],xor_reg`; four `PUSH/MOV [.data]` candidates `qsort`ed. | `giScissorTest` used; scissor rect resolved only. |
| `mod_known` / `mod_numknown` | `model_t*` / `int*` | `Engine_FillAddress_ModKnown`: `.text` `B8 9D 82 97 53 81 E9`, ptr at `+7`; `_Mod_NumKnown`: string `"Cached models:\n"` → `57 68 <str> E8` + `DisasmRanges(+0x50)`. | Model index/count. |
| ~~`loadname` / `loadmodel`~~ | ~~`char (*)[64]` / `model_t**`~~ | **Deleted (2026-09-18).** The locator (`PUSH imm(.data)` near the `"loading %s"` printf; then first `MOV [.data],reg`) and the extern/definitions were removed: both slots were written but never read anywhere in the plugin. The upstream catalog does publish them (engine GLOBAL, 11/11). | — |
| `cl_max_edicts` / `cl_entities` | `int*` / `cl_entity_t**` | `Engine_FillAddress_CL_ReallocateDynamicData`: string `"CL_Reallocate cl_entities\n"` → `68 <str> E8` + `ReverseSearchFunctionBeginEx(+0x100)`; `MOV reg,[.data]`+`83 C4 04` or `IMUL reg,reg,[.data],imm`; `cl_entities` = first `MOV [.data],EAX` after the call. | Edict ranges. |
| `cl_numvisedicts` / `cl_visedicts` | `int*` / `cl_entity_t**` | `Engine_FillAddress_VisEdicts`: `.text` `8B 0D <slot> 81 F9 00 ?,00 00`; `DisasmRanges(+0x150)` `MOV [disp+ecx*4],reg` → array base. | Visible-entity list. |
| `host_basepal` | `word**` | `Engine_FillAddress_BasePalette`: `68 <"palette.lmp"> 68 00 08 00 00 E8 … 83 C4 08 A3 <slot>`. | Palette lookup. |
| `r_missingtexture` / `r_notexture_mip` | `texture_t**` | `Engine_FillAddress_MissingTexture`/`_NoTexture`: strings `"**missing**"` / `"**empty**"` → `6A 00 68 <str> E8 …`; first `MOV reg,[.data]` after the call. | Fallback textures. |
| `cache_head` | `cache_system_t*` | `Engine_FillAddress_Cache_Alloc` `DisasmRanges(+0x500)` `CMP reg,imm(.data)`. | LRU list. |
| `gSpriteMipMap` | `int*` | `Engine_FillAddress_Mod_LoadSpriteFrame` `DisasmRanges(+0x300)` `CMP [.data],0` / `MOV reg,[.data]`+`TEST`. | Sprite mipmap enable. |
| `detTexSupported` | `bool*` | `Engine_FillAddress_LegacyMultiTextureInit`: gamedata `GLOBAL`. | Detail-texture flag; forced false in the plugin's GL init. |

### Filter, move and texture-mode globals (`gl_hooks.cpp`)

| Local symbol / inferred game object | Declaration / type | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gl_filter_min` / `gl_filter_max` | `int*` | `Engine_FillAddress_GL_FilterMinMaxVars`: per-engine `GL_FILTER_SIG_*` embedding the `fild gl_filter_min` site and the two `glTexParameterf` slots; `gl_filter_max` read at `addr + Sig_Length(pattern)`. | Texture filtering. |
| `filterMode` / `filterColorRed` / `filterColorGreen` / `filterColorBlue` / `filterBrightness` | `int*` / `float*` | `Engine_FillAddress_SetFilterMode`/`_SetFilterColor`/`_SetFilterBrightness`: anchor = public `gEngfuncs.pfnSetFilterMode`/`pfnSetFilterColor`/`pfnSetFilterBrightness` mapped real→scan; `DisasmRanges(+0x50)` first store (colour: up to 3 candidates `FSTP`/`MOVSS`/`MOV`, `qsort` ascending → R/G/B). | Screen filter. |

### VideoMode offsets (`gl_hooks.cpp`)

| Local symbol / inferred game object | Declaration / type | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic` | `void (__fastcall*)(void*, int, void*)` | `Engine_FillAddress_DrawStartupGraphic`: per-engine `DRAWSTARTUPGRAPHIC_SVENGINE`/`_HL25`/`_NEW`/`_NEW2`/`_BLOB` `Search_Pattern`. | `Install_InlineHook`; wrapper reads the offsets below. |
| `offset_CVideoMode_Common_m_ImageID_Size` (+ derived `_m_ImageID`, `_m_iBaseResX`, `_m_iBaseResY`) | `int` fields | `DisasmRanges(DrawStartupGraphic,+0x100)`: `CMP [reg+disp],reg` / `MOV reg,[reg+disp]`+`TEST` / `CMP [reg+disp],0` with `disp` in `0x100..0x400`; `m_ImageID = _Size - sizeof(CUtlMemory<bimage_t>)`, `_m_iBaseResX = _Size+8`, `_m_iBaseResY = _Size+12`. | Startup-graphic wrapper. |
| `gPrivateFuncs.CGame_DrawStartupVideo` | `void (__fastcall*)(void*, int, const char*, void*)` | `Engine_FillAddress_DrawStartupVideo`: HL25 only, `DRAWSTARTUPVIDEO_HL25`; other engines leave null. | `Install_InlineHook` (install unconditional). |

### Sven Co-op client and CS/CZ globals (`Client_FillAddress_*`, `gl_hooks.cpp`)

| Local symbol / inferred game object | Declaration / type | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `g_bRenderingPortals_SCClient` | `bool*` | `Client_FillAddress_RenderingPortals`: `6A 00 6A 00 6A 00 8B ? FF 50 ?` + `DisasmRanges(+0x80)` `MOV [.data],1`. | Portal-pass gating. |
| `g_iWaterLevel` | `int*` | `Client_FillAddress_WaterLevel`: `A3 <slot> 83 …` ptr at `addr+1`. | `V_CalcRefdef` fog gate. |
| `g_iFogColor_SCClient` / `g_iStartDist_SCClient` / `g_iEndDist_SCClient` | `float*` | `Client_FillAddress_FogParams`: `68 01 26 00 00 68 65 0B 00 00` (`GL_LINEAR`,`GL_FOG`) + `DisasmRanges(+0x300)` `MOVSS xmm,[.data]` candidates; requires ≥5 with last three 4-byte-adjacent; assigns `[0]`/`[3]`/`[4]`. | Sven fog params. |
| `g_ViewEntityIndex_SCClient` | `int*` | `Client_FillAddress_ViewEntityIndex` (buildnum ≥ 10182): `FF 15 … 85 C0 ? ? 8B 00 ? 05` + `DisasmRanges(+0x80)` `CMP reg,[.data]`. | Studio view-entity save/restore. |
| `g_iUser1` / `g_iUser2` | `int*` | `Client_FillAddress_CL_IsThirdPerson`: anchor = client `CL_IsThirdPerson` (from `pExportFuncs`/`GetProcAddress`); `DisasmRanges(+0x100)` up to 16 `.data` candidates; last two accepted when adjacent. | Spectator resolution. |
| `g_PlayerExtraInfo` / `g_PlayerExtraInfo_CZDS` | `extra_player_info_t (*)[65]` / `extra_player_info_czds_t (*)[65]` | `Client_FillAddress_PlayerExtraInfo` (CS `cstrike`/`czero`/`czeror` only): `.text` three consecutive 16-bit stores `66 89 ? ? ? ? ?` → `DisasmRanges(+0x100)` up to 4 `.data` 16-bit candidates; `qsort`; verify `playerclass`/`teamnumber` adjacency, base = `Candidates[last] - offsetof(teamnumber)`. HL25 fallback pattern adds a wildcard per store. | `CounterStrike_IsVIP` / `CounterStrike_GetTeamNumber`. |

## Engine-private call sites and branch redirects

| Symbol | Patch kind | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `Sys_ShutdownGame_call_GL_Shutdown` | `InlinePatchRedirectBranch` → plugin `GL_Shutdown` | `Engine_FillAddress_GL_Shutdown` (call site stored during resolution). | Engine shutdown re-enters the plugin handler. |
| `GL_SetMode_call_qwglCreateContext` | `InlinePatchRedirectBranch` → `CoreProfile_qwglCreateContext` | `Engine_FillAddress_GL_SetMode` BFS: `MOV reg,[reg]` then within `+3` a `PUSH reg` whose next bytes are `FF 15`. | Non-SvEngine GL context creation. |
| `R_ResetLatched` engine call site | `InlinePatchRedirectBranch` → `R_ResetLatched_Patched` | `R_PatchResetLatched`: `.text` `6A 01 ? ? ? 08 03 00 00` + `DisasmRanges(+0x50)` waits for a `MOV [reg+0x308]` then the next `E8`; `GetCallAddress`; no-op on HL25. | `cl_fixmodelinterpolationartifacts` fix; original kept in `gPrivateFuncs.R_ResetLatched`. |
| `mov eax, ds:allocated_textures` sites | 5-byte rewrite to `call GL_RedirectedGenTexture` | `R_RedirectEngineLegacyOpenGLTextureAllocation`: per-hit `DisasmRanges(+0x100)` accepts a `.data` write-back `MOV` or an `E8` target equal to `GL_Bind`; no-op when `g_bHasOfficialGLTexAllocSupport`. | Legacy texture allocation through the plugin `GL_GenTexture`. |
| engine imports (`opengl32.dll`/`SDL2.dll`/`kernel32.dll!GetProcAddress`) | `IATHook` / `BlobIATHook` | `R_RedirectEngineLegacyOpenGLCallAPI`, branch by engine type (SvEngine vs SDL2 vs blob vs other). | Core-profile GL wrappers (`CoreProfile_*`). |
| Sven client legacy GL call sites (`glTexEnvf`, `glBegin`, `glEnd`, `glColor4f`, `glEnable`, `glDisable`, `glCopyTexSubImage2D`, `glClear`) | `InlinePatchRedirectBranch` each | `R_RedirectClientLegacyOpenGLCall`, per-site byte patterns; `glDisable(GL_CLIPPLANE0)` instead rewrites the `mov esi, ds:glDisable` operand to `g_glDisable_ClipPlane`. | Sven client core-profile routing. |
| Sven client `AngleVectors` call in the portal path | `InlinePatchRedirectBranch` → `ClientPortalManager_AngleVectors` | `C7 82 E8 00 00 00 01 00 00 00 50 8D 42 18 50 51 E8`. | Sets `g_pCurrentClientPortal = a1 - 12`. |

## Sven Co-op client portal manager (`Client_FillAddress_SCClient`)

Gated on the `SCClientDLL001` client factory; on non-Sven clients all fields stay null and the hooks are skipped.

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.ClientPortalManager_DrawPortalSurface` | `void (__fastcall*)(void*, int, void*, msurface_t*, GLuint)` | `Client_FillAddress_ClientPortalManager_GetOriginalSurfaceTexture_DrawPortalSurface`: `6A 01 6A 01 6A 01 6A 01 FF 15 … 68 E1 0D 00 00` + `DisasmRanges(+4,+0x50)` first `E8` = `GetOriginalSurfaceTexture`; `ReverseSearchFunctionBeginEx(pFound,+0x600)` predicate `83 EC` = `DrawPortalSurface`. | `Install_InlineHook(DrawPortalSurface)`; handler does not call original. |
| `gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture` | `mtexinfo_t* (__fastcall*)(void*, int, msurface_t*)` | Same locator (first `E8`). | Called by `R_GetPortalSurfaceModel`. |
| `gPrivateFuncs.ClientPortalManager_EnableClipPlane` | `void (__fastcall*)(void*, int, int, vec3_t, vec3_t, vec3_t)` | `Client_FillAddress_ClientPortalManager_EnableClipPlane`: prologue with security cookie `83 EC ? A1 … 33 C4 ? 44 24 … F3 0F`. | `Install_InlineHook(EnableClipPlane)`. |
| `gPrivateFuncs.ClientPortalManager_RenderPortals` | `void (__fastcall*)(void*, int)` | `Client_FillAddress_ClientPortalManager_RenderPoratals`: SEH prologue `55 8B EC 6A FF 68 … 64 A1 00 00 00 00 50 83 EC 6C …`. | `Install_InlineHook(RenderPortals)`; records `g_pClientPortalManager`. |
| `gPrivateFuncs.ClientPortalManager_ResetAll` | `void (__fastcall*)(void*, int)` | `Client_FillAddress_ClientPortalManager_ResetAll`: `C7 45 ? FF FF FF FF A3 ? ? ? ? E8 ? ? ? ? 8B 0D` → `GetCallAddress(addr+12)`. | **Hook commented out**; wrapper unreferenced. |
| `gPrivateFuncs.UpdatePlayerPitch` | `void (__cdecl*)(cl_entity_t*, float)` | `Client_FillAddress_UpdatePlayerPitch`: `FF 73 40 E8 ? ? ? ? 83 C4 08 80 3D ? ? ? ? 00` → `GetCallAddress(addr+3)`. | `Install_InlineHook(UpdatePlayerPitch)`; handler `gl_studio.cpp:5064`. |
| `gPrivateFuncs.SCClientDLL_glewInit` (`_glewInit@0`) | `decltype(glewInit)*` | `GetProcAddress(GetClientModule(), "_glewInit@0")`. | Called at the end of `Client_InstallHooks`. |
| Private `ClientPortal` / `ClientPortalManager` struct offsets | literals in `gl_portal.cpp` | Build-num gated: `origin`/`angles` at `+0`/`+12` (≥10000) else entity at `+0x70` (≥8948); `mode` at `+0x40` (≥10000) / `+0x28` (≥8948); texture id/w/h at `+204`/`+208`/`+212`; manager vector begin/end at `+140`. | `ClientPortal_GetPortalTransform` / `_Mode` / `_GetIndex`. |

## Engine `EngineSurface` (`EngineSurfaceHook.cpp`)

The engine VGUI2 surface (`EngineSurface007` → `IEngineSurface`/`IEngineSurface_HL25`, and `VGUI_Surface026`) is obtained from `g_pMetaHookAPI->GetEngineFactory()`; the plugin then hooks methods by hardcoded vtable index with `VFTHook(obj, 0, index, wrapper, &original)`. `EngineSurface_FillAddress` resolves the function pointers only to validate indices and to drive two disasm roots. `EngineSurface_InstallHooks` installs 19 engine-surface hooks (+`VGUI_Surface026::DrawSetTexture` at index 27); every wrapper replaces the engine method and never calls the saved original, except `BaseUISurface_DrawSetTexture`.

| Local symbol / inferred engine symbol | Signature of field | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.enginesurface_pushMakeCurrent` | `void (__fastcall*)(void*, int, int* insets, int* absExtents, int* clipRect, bool translateToScreenSpace)` | `GetVFunctionFromVFTable(vftable, index=1, …)`; also the scan root for `pmainwindow`/`g_bScissor`/`g_ScissorRect`. | VFTHook index 1; wrapper replaces engine method. |
| `enginesurface_popMakeCurrent` / `drawFilledRect` / `drawOutlinedRect` / `drawLine` / `drawPolyLine` / `drawTexturedPolygon` / `drawSetTextureRGBA` / `drawSetTexture` / `drawTexturedRect` / `drawTexturedRectAdd` / `createNewTextureID` / `drawPrintCharAdd` / `drawSetTextureFile` / `drawGetTextureSize` / `isTextureIDValid` / `drawSetSubTextureRGBA` / `drawFlushText` / `drawSetTextureBGRA` / `drawUpdateRegionTextureBGRA` | surface virtuals | Hardcoded `index_enginesurface_*`; HL25 uses a higher-index layout than GoldSrc/SvEngine. `drawTexturedRectAdd` is hooked on HL25 only; `drawTexturedPolygon` is hooked by index but its field is never populated. `drawFlushText` is the scan root for `g_iVertexBufferEntriesUsed`/`g_VertexBuffer`. | VFTHook per index; wrappers reimplement via `R_Draw*`. |
| `BaseUISurface_DrawSetTexture` (`VGUI_Surface026::DrawSetTexture`) | `void (__fastcall*)(void*, int, int textureId)` | `engineFactory("VGUI_Surface026")`; hardcoded `index_BaseUISurface_DrawSetTexture = 27`. | `VFTHook`; the only wrapper that calls the saved original. |
| `offset_enginesurface_drawColor` / `drawTextColor` | `int` | Hardcoded by engine type (`4`/`20` SvEngine vs `8`/`24` others) in `EngineSurface_FillAddress`, not disassembled. | Draw/text colour member reads. |
| `g_iVertexBufferEntriesUsed` / `g_VertexBuffer` | `int*` / `EngineSurfaceVertexBuffer_t (*)[256]` | `Engine_FillAddress_EngineSurface_drawFlushText`: `DisasmRanges` mirror body `+0x150`; `CMP [abs],imm` or `MOV reg,[abs]`+`CMP reg,imm`; then first `PUSH imm32(.data)` = `g_VertexBuffer`. | Engine vertex-buffer flush. |
| `g_bScissor` / `g_ScissorRect` | `bool*` / `RECT*` | `Engine_FillAddress_EngineSurface_pushMakeCurrent`: `DisasmRanges` mirror body `+0x500`; `MOV [.data],1` = `g_bScissor`; up to 4 successive `MOV [.data],reg`, `qsort`, lowest = `g_ScissorRect`. | Scissor clipping. |
| `pmainwindow` | `void**` | Same function: `MOV reg,[.data]` within first 35 instructions, validated by `MOV reg,[candReg]`/`PUSH [candReg]` within `+6`. | `Sys_GetMainWindow()`. |

## Client `CGameStudioRenderer` virtual table (`ClientStudio_FillAddress`)

`g_pGameStudioRenderer` is the client singleton; the plugin reads `vftable = *(PVOID**)g_pGameStudioRenderer` and resolves virtuals by index with `GetVFunctionFromVFTable`. The index is derived by disassembling the client-supplied `(*ppinterface)->StudioDrawPlayer`/`StudioDrawModel` thunk (a leading `E9` jmp is skipped). This pass scans the **client** image; `ClientStudio_FillAddress_EngineStudioDrawPlayer` scans the **engine** image for the engine `R_Studio*` functions from the same thunks.

| Local symbol / inferred engine symbol | Vtable index | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer` | default `3` | `DisasmRanges(client StudioDrawPlayer,+0x200)`: `CALL [reg+disp]` `disp 8..0x200` → `index=disp/4`; or `CALL imm` matching `vftable[i]` for `i=1..3`. | Hooked; → `StudioDrawPlayer_Template`. |
| `gPrivateFuncs.GameStudioRenderer_StudioDrawModel` | default `2` | `DisasmRanges(thunk,+0x80)` same detection. | Resolved only. |
| `gPrivateFuncs.GameStudioRenderer_StudioRenderModel` | unused | `GamedataResolvePtr(clientBase, "GameStudioRenderer_StudioRenderModel", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION)`. | Hooked; → `StudioRenderModel_Template`. |
| `gPrivateFuncs.GameStudioRenderer_StudioRenderFinal` | default `RenderModel+1` | `DisasmRanges(StudioRenderModel,+0x100)` `CALL [reg+disp]` `disp` just above `RenderModel*4`. | Hooked. |
| `gPrivateFuncs.GameStudioRenderer_StudioCalcAttachments` | anchor | BFS over vtable entries `4..9`; match on push of `"Too many attachments on %s\n"` or member offsets `0xD4`/`0xD8`. | Index anchor only. |
| `gPrivateFuncs.GameStudioRenderer_StudioSetupBones` / `_StudioSaveBones` / `_StudioMergeBones` | `CalcAttachments -1 / +1 / +2` | `GetVFunctionFromVFTable`. | Hooked. |
| `gPrivateFuncs.R_StudioDrawPlayer` / `R_StudioDrawModel` | engine image | `(*ppinterface)->StudioDrawPlayer`/`StudioDrawModel` mapped real→engine-scan, `E9` skipped, mapped scan→real. | `R_StudioDrawPlayer` hooked → `StudioDrawPlayer_Template`; `R_StudioDrawModel` resolved only. |
| `gPrivateFuncs.R_StudioRenderModel` | engine image | `Search_Pattern("50 E8 ? ? ? ? 83 C4 10 E8 ? ? ? ? E8 ? ? ? ? 8B")` → `GetCallAddress(addr+9)`. | Hooked; second half of the `ClientStudio_FillAddress` success gate. |
| `gPrivateFuncs.R_StudioRenderFinal` | engine image | `DisasmRanges(R_StudioRenderModel,+0x80)` first 5-byte `E8`. | Hooked. |
| `gPrivateFuncs.R_StudioSetupBones` | engine image | string `"Bip01 Spine\0"` → `68 <str> ?? E8 … 83 C4 08 85 C0` + `ReverseSearchFunctionBeginEx(+0x1000)`. | Hooked; discriminator for `R_StudioSaveBones`. |
| `gPrivateFuncs.R_StudioMergeBones` | engine image | `Search_Pattern_From_Size(EngineStudioDrawModelThunk,+0x250,"83 B8 08 03 00 00 0C")` + `DisasmRanges(+0x80)` first `E8`. | Hooked. |
| `gPrivateFuncs.R_StudioSaveBones` | engine image | Second distinct `E8` target in the same walk. | Hooked. |
| ~~`g_pGameStudioRenderer`~~ | client `.data` singleton | ~~`DisasmRanges(client StudioDrawPlayer,+0x200)` first `MOV ECX,imm(.data)`~~ | Deleted 2026-09-19 — the vtable is now resolved through gamedata `VIRTUAL_FUNCTION` records (see last section). |

## Boundary: public-API-derived pointers (not engine-private)

Stored in `gPrivateFuncs` but sourced from public interfaces, so excluded from the private inventory:

- `triapi_*` (15) — copied from `gEngfuncs.pTriAPI->*`, reassigned back in `Engine_InstallHooks`.
- `studioapi_GL_SetRenderMode` / `_SetupRenderer` / `_RestoreRenderer` / `_StudioDynamicLight` / `_StudioCheckBBox` — `pstudio->*`, hooked by `EngineStudio_InstalHooks`.
- SDL2 exports (`SDL_GetWindowPosition`, `SDL_GL_SetAttribute`, `SDL_GetWindowSize`, `SDL_GL_SwapWindow`, `SDL_GL_GetProcAddress`, `SDL_CreateWindow`, `SDL_GL_ExtensionSupported`) — plain `GetProcAddress`.
- `pbonetransform` / `plighttransform` / `rotationmatrix` (from `pstudio->StudioGetBoneTransform`/`GetLightTransform`/`GetRotationMatrix`), `r_smodels_total` / `r_amodels_drawn` (`pstudio->GetModelCounters`), `cl_viewent` (`gEngfuncs.GetViewModel`), `cl_sprite_white` / `cl_sprite_shell` (`IEngineStudio.Mod_ForName`), `cl_minmodels` / `cl_min_t` / `cl_min_ct` (public cvars).

## Hooks installed

- **Engine** (`Engine_InstallHooks`, `gl_hooks.cpp:12594`): `GL_Init`, `GL_SetMode_SvEngine`/`GL_SetMode_GoldSrc`/`GL_SetModeLegacy`(+`GL_SelectPixelFormat` with legacy), `GL_Bind`, `GL_LoadTexture2`, `GL_UnloadTextures`, `GL_LoadFilterTexture`, `GL_BuildLightmaps`, `GL_Set2D`, `GL_Finish2D`, `GL_BeginRendering`, `GL_EndRendering`, `R_RenderView`/`R_RenderView_SvEngine`, `R_NewMap`, `R_CullBox`, `R_ForceCVars`, `Mod_PointInLeaf`, `R_GLStudioDrawPoints`, `R_GetSpriteFrame`, `Mod_LoadStudioModel`, `Mod_LoadSpriteModel`, `Mod_UnloadSpriteTextures`, `BuildGammaTable`, `Host_ClearMemory`, `LegacyMultiTextureInit`, `PVSNode`, `R_LoadSkys`/`R_LoadSkyBox_SvEngine`, `CVideoMode_Common_DrawStartupGraphic`, `CGame_DrawStartupVideo`, `Draw_Frame`, `Draw_SpriteFrame*[_SvEngine]`, `Draw_FillRGBA`/`Draw_FillRGBABlend`, `Draw_FillRGBABuf`, `D_FillRect`, `Draw_Pic`, plus the `Sys_ShutdownGame_call_GL_Shutdown` branch redirect.
- **Client** (`Client_InstallHooks`, `gl_hooks.cpp:14081`): `ClientPortalManager_DrawPortalSurface`, `_EnableClipPlane`, `_RenderPortals`, `UpdatePlayerPitch`; `SCClientDLL_glewInit()` invoked. `ClientPortalManager_ResetAll` hook is commented out.
- **Studio** (`EngineStudio_InstalHooks` / `ClientStudio_InstallHooks`, `exportfuncs.cpp`): `CL_FxBlend`, the five `studioapi_*`, and the engine/client `R_Studio*`/`GameStudioRenderer_*` set. Note `EngineStudio_InstalHooks` runs before `ClientStudio_FillAddress`, so the engine Studio render hooks are effectively installed by the guarded `ClientStudio_InstallHooks` calls.
- **EngineSurface** (`EngineSurface_InstallHooks`): 19 `enginesurface_*` VFTHooks + `VGUI_Surface026::DrawSetTexture`. `EngineSurface_UninstallHooks` is empty — no surface hook is ever restored.

## Dependencies

- `Plugins/Renderer/plugins.cpp` — provides the real/mirror module bases and drives `Engine_FillAddress` / `Client_FillAddress` / install-uninstall.
- MetaHook APIs `SearchPattern`, `ReverseSearchFunctionBegin[Ex]`, `DisasmRanges`, `GetNextCallAddr`, `GetVFunctionFromVFTable`, `InlineHook`, `InlinePatchRedirectBranch`, `IATHook`, `BlobIATHook`, `VFTHook`, `GetEngineFactory`, `GetClientFactory`, `GetSectionByName`, `GetModuleCRC64`, `SysError`.
- Capstone (`cs_insn`, `X86_INS_*`) for `DisasmRanges`.

## Notes

- **Dead / resolved-only fields.** Many `gPrivateFuncs` fields are located but never hooked or called (the multitexture pair and its globals were removed on 2026-09-19 — see the last section): `R_SetupGL`, `R_RenderScene`, `R_SetupFrame`, `R_PolyBlend` (reimplemented), `S_ExtraUpdate`, `GL_SelectTexture`, `R_TextureAnimation`, `R_DrawSequentialPoly[_HL25]`, `R_DrawBrushModel`, `R_DrawWorld` (reimplemented), `R_DrawViewModel`, `R_MarkLeaves`, ~~`EmitWaterPolys`~~, `VID_UpdateWindowVars`, `R_DrawTEntitiesOnList`, `R_ClearParticles`, `V_InitLevel`, ~~`R_BuildLightMap`~~, ~~`R_AddDynamicLights`~~, `R_RenderDynamicLightmaps`, `R_DrawParticles` (reimplemented), ~~`CL_AllocDlight`/`CL_AllocElight`~~, `R_StudioLighting`, ~~`R_StudioChrome`~~, ~~`R_LightLambert`~~, `R_StudioSetupSkin`, `R_StudioGetSkin`, `GL_UnloadTexture`, ~~`Draw_MiptexTexture`~~, `Draw_DecalTexture`, ~~`Draw_CustomCacheGet`/`Draw_CacheGet`~~, ~~`R_DrawSpriteModel`~~, `Mod_LoadSpriteFrame`, `SCR_BeginLoadingPlaque`, `R_LightStrength`, ~~`R_RotateForEntity`~~, ~~`R_AddTEntity`~~, ~~`R_RenderFinalFog`~~, `Mod_LoadBrushModel`, `Mod_LoadModel`, `ClientPortalManager_ResetAll` (hook commented), `GameStudioRenderer_StudioDrawModel`, `R_StudioDrawModel`, and many `*Vars` globals (`cls_state`, `cls_signon`, `r_soundOrigin`, `lightmap_textures`, `lightmap_rectchange`, `gDecalSurfs`, `modelorg`, `vid_d3d`, `g_ChromeOrigin`, ~~`gSkyTexNumber`~~, ~~`r_loading_skybox`~~, `lightmap_polys`, `lightmap_modified`, ~~`chrome`~~, ~~`chromeage`~~, scissor rect, `pmainwindow` consumers).
- **Inlined-function flags.** `R_ForceCVars_inlined`, `R_SetupFrame_inlined`, `R_RenderScene_inlined`, `R_GlowBlend_inlined` indicate the engine inlined the target; the plugin then uses call-site-sensitive logic instead of a direct hook.
- **Duplicate resolution sites.** `r_blend` is resolved both by `Engine_FillAddress_R_DrawTEntitiesOnListVars` (gl_hooks) and `EngineStudio_FillAddress_StudioSetRenderamt` (exportfuncs); `R_RenderDynamicLightmaps` by the `R_DrawSequentialPoly` BFS and its own locator; `r_framecount` by `_GetTimes` and a shadowing local in `gl_hooks.cpp:8744`. Both `if (!field)`-guarded, so first wins.
- **Hook/uninstall asymmetry.** `Host_ClearMemory` is installed but never unhooked; `ClientPortalManager_DrawPortalSurface`'s hook is installed but `EngineSurface_UninstallHooks` is empty; `GameStudioRenderer_StudioDrawPlayer` is installed but not uninstalled.
- **Build-num gates.** `g_ViewEntityIndex_SCClient` requires buildnum ≥ 10182; `size_of_frame` defaults to `0x42B8` for buildnum ≤ 8684; `R_SetupGL`/`R_LoadSkybox` pick signatures by buildnum thresholds (10152, 9899).
- **`g_bHasOfficialFBOSupport` / `g_bHasOfficialGLTexAllocSupport`** are capability flags, not addresses: the former from the presence of the string `"FBO backbuffer rendering disabled"`, the latter from whether a `0x16A8`-based texture-alloc pattern exists. They select signatures and gate the legacy texture-allocation redirect.

## Issue #873 migration (2026-09-17): 76 more catalog-backed symbols

Baseline `26b17bd0`. All 76 dependencies the published catalog already covered
(71 FUNCTION, 1 GLOBAL, 4 PATCH) now resolve exclusively through
`ResolveGameSymbol`; their signature / string / `DisasmRanges` locators, sig
`#define`s and thunk derivations were deleted.

**Resolution rules used**

- Standalone locators were removed and the dispatch inlines
  `GamedataResolvePtr(RealDllInfo.ImageBase, "<canonical>", kind)`; the 46
  already migrated entries (`e7e818a0`) were left untouched.
- A new `GamedataResolvePtrIfAvailable` (`plugins.h`) powers conditionally
  required symbols; it consults `IsGameSymbolAvailable` and returns `nullptr`
  when the current identity publishes no record, so an explicitly isolated
  legacy branch can stay.
- Retained variable-extraction disasm passes are now rooted at the
  gamedata-resolved **real** address and walk the real image
  (`ctx = { RealDllInfo, RealDllInfo }`); operand displacements are assigned
  verbatim with no `ConvertDllInfoSpace`.
- PATCH records are the target instruction address: `GL_SetMode_call_qwglCreateContext`
  and `Sys_ShutdownGame_to_GL_Shutdown_callsite_0` resolve directly;
  `R_PatchResetLatched` redirects every numbered
  `CL_LinkPacketEntities_to_R_ResetLatched_callsite_N` (both `_0` and `_1`) and
  resolves `R_ResetLatched` itself. HL25 keeps its skip.

**Identity / ABI branches (all preserved)**

| Group | Coverage | Rationale |
| --- | --- | --- |
| `ALL` (11 identities) | most engine functions/globals + patches, incl. `Draw_FillRGBA` / `Draw_FillRGBABlend` | base symbol is published everywhere |
| non-SvEngine | `D_FillRect`, base `Draw_SpriteFrame*`, `R_LoadSkys`, `GL_SetMode_call_qwglCreateContext` | SvEngine uses a variant or the interface does not exist there |
| `E8` (cof + 8 hl) | `GL_SelectPixelFormat`, `GlowBlend` | inlined on HL25 and SvEngine |
| SvEngine only | `Draw_SpriteFrame*_SvEngine`, `Draw_FillRGBABuf`, `R_LoadSkyBox_SvEngine`, `allow_cheats` | variant symbols |
| hl-10210 only | `CGame_DrawStartupVideo` | HL25-only startup video |
| hl-10210 + hl-6153/8684 | `GL_SetMode` (SvEngine ABI split into `_SvEngine`/`_GoldSrc`) | SDL / six-arg ABI |
| cof + hl-3248..4554 | `GL_SetModeLegacy` | non-SDL legacy ABI; CoF now reaches the legacy branch |
| hl-10210 + hl-6153/8684 | `SDL_InitGL` | SDL builds only |
| all but svencoop-10257 | `R_RenderScene` | 10257 inlines it (`IsGameSymbolAvailable` gate) |
| all but svencoop-8948 | `R_DrawViewModel` | 8948 has no record and stays inlined |
| svencoop only | client portal functions/globals | `SCClientDLL001` factory |
| svencoop-10257 only | `ClientPortalManager_EnableClipPlane`, `g_ViewEntityIndex_SCClient` | 8948 publishes neither; `g_ViewEntityIndex_SCClient` is resolved with `GamedataResolvePtrIfAvailable` and its consumers are already null-guarded |

`R_GlowBlend` was renamed to the engine's real name `GlowBlend` everywhere
(field, locator, wrapper, consumer); the `R_GLOW_BLEND_*` sig macros and the
now-unused `R_ForceCVars_inlined` / `GlowBlend_inlined` fields were removed.

**Follow-up (same issue, 2026-09-17): `modelorg` and `active_particles`**

Both are published as engine GLOBALs for all 11 identities and the retained
heuristics for them were unreliable after the root switched to the real image,
so they were migrated as well (78 symbols total): `Engine_FillAddress_R_DrawWorld`
and `Engine_FillAddress_R_DrawParticles` now resolve them through
`ResolveGameSymbol` and only `particletexture` (catalog-uncovered) is still
scanned. A sweep for every catalog symbol that is still assigned outside a
`GamedataResolve*` call (matching bare-symbol assignments against the whole
`records[]` name set) now reports no remaining locator, so this class of
"catalog-covered but still scanned" symbol is exhausted.

**Root-consistency fix and removal (2026-09-18): `loadname` / `loadmodel`**

The retained `Mod_LoadModel` pass had kept a mirror-based root
(`ConvertDllInfoSpace(gPrivateFuncs.Mod_LoadModel, RealDllInfo, DllInfo)`) while
the `"Loading '%s'\n"` / `"loading %s\n"` anchor it searched for was already read
from the real image, so the embedded absolute address never matched and
`Mod_LoadModel_PushString` resolution would `Sys_Error` whenever the mirror
engine image existed. That was first fixed by rooting both passes on the resolved
real address, then the whole pass was removed: the two slots are written but
never read anywhere in the plugin, so `loadname` / `loadmodel` (extern in
`gl_local.h`, definitions in `gl_rmain.cpp`), the printf anchor,
`Mod_LoadModel_VA`, both `DisasmRanges` walks and the standalone
`Engine_FillAddress_Mod_LoadModel` were all deleted; `gPrivateFuncs.Mod_LoadModel`
is now an inline `ResolveGameSymbol` in the dispatch (the upstream catalog does
publish both as engine GLOBALs on 11/11 identities, release
`2026-09-17T15:27:45Z`, but nothing consumes them). A sweep for
`ConvertDllInfoSpace(gPrivateFuncs.*, RealDllInfo, DllInfo)` confirms the
remaining back-conversions all belong to **unmigrated** locators that keep the
scan image consistently for both root and pattern.

**Residual scanning (intentionally kept, catalog-uncovered)**

`R_SetupFrame`; `R_ClearParticles` / `R_DecalInit` / `V_InitLevel` (callees),
~~`GL_UnloadTextures`~~ (wrong — it was already published; see the 2026-09-19
section), ~~`R_LoadSkyboxInt_SvEngine`~~, `realloc_SvEngine`,
`particletexture`, the `gl_extensions` / `vid_d3d` / texture-array / fog /
scissor / viewmodel / sky / view-leaf / lightmap variable slots,
`CL_FxBlend`'s sibling `r_blend`, and every symbol outside the list
(EngineSurface virtuals, portal/DrawNormalTriangles scans,
~~`R_AddTEntity`~~, `R_TextureAnimation`, `R_LightStrength`, `R_Studio*` engine vars, etc.).

**Release gate**: `scripts/validate-gamedata.py` now carries a Renderer consumer
gate (`RENDERER_*` tables + `validate_renderer`) covering the 46 + 76 required
symbols per engine family / identity with kind **and module** checks; snapshot
records now retain their owning module. Behaviour tests live in
`scripts/tests/test_gamedata_contract.py::RendererGateTests`.

**Not verified**: in-game smoke tests on SvEngine / HL25 / GoldSrc / Blob / CoF
were not run from this environment; only `Release|Win32` builds of `MetaHook`
and `Renderer` plus the catalog gate and contract tests were executed.

## Issue #873 follow-up (2026-09-18): 19 newly published symbols

Baseline `f8c9c9c0` (merge of PR #874), branch `dev`. Upstream
`GoldSrc_VibeSignatures` added the records in `929deb15` and `b09de35d`
(release `2026-09-17T15:27:45Z` onwards); everything the catalog newly covers is
now resolved through `ResolveGameSymbol` and the corresponding signature /
`DisasmRanges` locators were deleted.

**Engine functions (kind FUNCTION, 11/11 identities): 2**

- `CL_AllocDlight` — the efx-API derivation
  (`gEngfuncs.pEfxAPI->CL_AllocDlight` + `ReverseSearchFunctionBeginEx`) and the
  `CL_ALLOCDLIGHT_SIG_{BLOB,NEW2,NEW,HL25,SVENGINE}` fallbacks are gone.
- `CL_AllocElight` — same for `CL_ALLOCELIGHT_SIG_*`.

(Correction 2026-09-21: both `gPrivateFuncs` fields were later shown to be write-only — the
plugin uses `gEngfuncs.pEfxAPI->CL_AllocDlight` — and were deleted, along with the two locators;
`cl_dlights` / `cl_elights` moved to inline `GamedataResolvePtr` calls in `Engine_FillAddress`.
See the last section.)

**Engine globals (kind GLOBAL): 17 slots / 18 record names**

`currenttexture` (was the retained `GL_Bind` disasm), `c_brush_polys`,
the alias-poly counter (`c_alias_polys` on the 9 non-SvEngine identities,
`c_model_polys` on `svencoop-10257`/`8948` — same slot, renamed engine-side),
`envmap`, `cl_stats`, `cl_weaponstarttime`, `cl_weaponsequence`,
`cl_light_level` (all five replaced the three-branch
`Engine_FillAddress_R_DrawViewModel` scan), `cl_dlights`, `cl_elights`,
`cache_head` (its 0x500-byte CMP-shape pass deleted), `cl_waterlevel`
(its three heuristics and the `std::map` candidate tracking in
`_RenderSceneVars2` deleted, `#include <map>` dropped), `cl_simorg`
(`Engine_FillAddress_CL_SimOrgVars` and `CL_SIMORG_SIG*` deleted, now inline in
the dispatch), `cshift_water` (the `+12` derivation dropped),
`detTexSupported`, `cl_time` + `cl_oldtime` (the `FLD` / `MOVSD` branches and
the `candidate_count == 5` block of `EngineStudio_FillAddress_GetTimes`
deleted).

`studioapi_GetTimes` is published too but intentionally unused — the public
`pstudio->GetTimes` already yields that address.

**Behaviour fix: `DT_Initialize` on the blob builds**

`ce9396e3` upstream dropped the `DT_Initialize` registration for `hl-3248` /
`3266` / `3329` / `3647`, so the plugin's `GamedataResolvePtr` started to
`Sys_Error` at startup on all four. The interim fix was
`GamedataResolvePtrIfAvailable` plus a guarded install; it was superseded the
same day by the `LegacyMultiTextureInit` retarget (next section), which gives
every identity a hook target again.

**Retained scans (still catalog-uncovered)**

- ~~`r_dlightactive`~~ (deleted 2026-09-21 — see last section: it was walked from the
  gamedata-resolved `CL_AllocDlight` body, anchor `push 0x28` / first `MOV reg,[mem]`
  within 8 instructions / `OR [mem],1` fallback, but nothing ever read it).
- `r_framecount` — first engine global the studio `GetTimes` thunk reads; the
  two MOV-candidate collectors in `EngineStudio_FillAddress_GetTimes` are kept.
- `gWaterColor` — both `GWATERCOLOR_SIG_HL25` / `GWATERCOLOR_SIG` branches kept
  in `Engine_FillAddress_WaterVars`, read from the `V_CalcRefdef` water-cshift
  call site.
- `particletexture`, plus `ClientDLL_DrawNormalTriangles` / `gDevOverview`,
  which shared the deleted `cl_waterlevel` scan in `_RenderSceneVars2` — that
  pass still runs in `DllInfo` space (it was never switched to the real image by
  #873) and its exit condition is now
  `if (gPrivateFuncs.ClientDLL_DrawNormalTriangles) return TRUE;`.

**Consumer gate**

`scripts/validate-gamedata.py`: `CL_AllocDlight` / `CL_AllocElight` added to
`RENDERER_ENGINE_ALL_FUNCTIONS` (both entries became stale on 2026-09-21 — the plugin no longer
resolves those two functions, so both were removed from the gate on 2026-09-21; the
`cl_dlights` / `cl_elights` GLOBAL entries stay); the 16 all-identity globals added to
`RENDERER_ENGINE_ALL_GLOBALS`; `c_model_polys` added to
`RENDERER_SVENGINE_GLOBALS`; new `RENDERER_ENGINE_NON_SVENGINE_GLOBALS`
(`c_alias_polys`); `DT_Initialize` moved out of the ALL group (see the next
section for the final grouping).
`scripts/tests/test_gamedata_contract.py::RendererGateTests` gained
`test_gate_alias_poly_counter_follows_engine_family`.

**Verified**: contract tests pass; `validate-gamedata.py` passes over the
21 synced snapshots; `Renderer` builds `Release|Win32` with no new warnings.
**Not verified**: in-game smoke tests on any engine family.

## `LegacyMultiTextureInit` retarget (2026-09-18)

Upstream `a41a675c` published `CheckMultiTextureExtensions` (HL / CoF / blob)
and `InitMultitexturing` (SvEngine). `gPrivateFuncs.DT_Initialize` was renamed
to `gPrivateFuncs.LegacyMultiTextureInit` and now holds whichever of the three
the running engine publishes, resolved in this order by
`Engine_FillAddress_LegacyMultiTextureInit` (each via
`GamedataResolvePtrIfAvailable`, fatal only if all three miss):

1. `CheckMultiTextureExtensions`
2. `InitMultitexturing`
3. `DT_Initialize`

The hook body (`LegacyMultiTextureInit` in `gl_rmain.cpp`, formerly
`DT_Initialize`) stays empty: the point is to suppress the engine's
fixed-function init — `glEnable(GL_TEXTURE_2D)`,
`glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB)`,
`glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f)` — which is invalid under the
Core profile context created by the `GL_SetMode_call_qwglCreateContext` patch.
`Install_InlineHook(LegacyMultiTextureInit)` is unconditional again.

**Why one hook is enough (verified against the binaries, not inferred)**

Disassembling `bin/<tag>/engine/hw{,.decrypt}.dll` at the recorded RVAs:

| Windows identity | probe record | `DT_Initialize` record | relationship |
| --- | --- | --- | --- |
| cof-5936, hl-4554, hl-6153, hl-8684 | `CheckMultiTextureExtensions` | published | the probe **calls** `DT_Initialize`, and that is its **only** `E8` xref in `.text` — hooking the probe covers both |
| hl-3248 / 3266 / 3329 / 3647 | `CheckMultiTextureExtensions` | none | see below |
| hl-10210, svencoop-10257, svencoop-8948 | inlined into `GL_Init` (linux-only record) | published | `GL_Init` calls `DT_Initialize` directly (twice on SvEngine); hooking the function covers every call site |

`InitMultitexturing` is linux-only on both SvEngine tags, so no Windows snapshot
carries it today; it is kept in the candidate list for forward compatibility.

**Correction to the upstream blob story.** `ce9396e3` says `DT_Initialize` is
"inlined into `CheckMultiTextureExtensions`" on the blob builds. It is not.
Each blob `hw.decrypt.dll` contains a standalone, `0x8d`-byte `DT_Initialize`
(hl-3248 `0x35590`, hl-3266 `0x35570`, hl-3329 `0x35220`, hl-3647 `0x35390`) —
same size and same `push <cvar>; call Cvar_RegisterVariable` ×2 prologue as the
hl-6153 / hl-8684 bodies, ending in `mov byte ptr [detTexSupported], 1`. It has
**zero `E8` xrefs**: it is dead code that never executes, and
`CheckMultiTextureExtensions` does not call it (the probe body carries no
`push 8573h` at all). Consequences:

- The pre-#873 `ReverseSearchFunctionBeginEx` locator *did* find the right
  function on blob (prologue rule `Candidate[0] == 0x68 && Candidate[5] == 0xE8`
  matches `0x35590`); it just hooked a function nobody calls.
- The blob `DT_Initialize` gamedata record published before `ce9396e3` pointed
  at `CheckMultiTextureExtensions` (`0x18d` bytes, FPU-compare prologue), so
  between #873 and the retarget the blob builds were neutering the multitexture
  probe instead. The retarget makes that deliberate and uniform.

**Consumer gate (final grouping)**

`RENDERER_INLINED_MTEX_PROBE_GAMES` = hl-10210, svencoop-10257, svencoop-8948 →
require `DT_Initialize`. `RENDERER_MTEX_PROBE_GAMES` = the other 8 → require
`CheckMultiTextureExtensions`. `DT_Initialize` is no longer required on
cof-5936 / hl-4554 / hl-6153 / hl-8684: it is published there but the plugin
resolves the probe first and never consumes it. Tests:
`test_gate_requires_exactly_one_multitexture_init_per_identity`,
`test_gate_flags_missing_multitexture_probe`,
`test_gate_flags_missing_dt_initialize_where_probe_is_inlined`.

**Behaviour delta.** On cof-5936 / hl-4554 / hl-6153 / hl-8684 the multitexture
probe itself is now neutered as well, so `gl_mtexable` stays 0 and
`qglMTexCoord2fARB` / `qglActiveTextureARB` stay null. Safe for the plugin:
`gl_mtexable` / `mtexenabled` are resolved but never read, the plugin's
`GL_EnableMultitexture` / `GL_DisableMultitexture` wrappers
(`gl_draw.cpp:622-630`) have no callers, and the engine's own
`GL_EnableMultitexture` gates on `gl_mtexable` before touching the ARB entry
points.

**Verified**: 50/50 contract tests; `validate-gamedata.py` over the 21 snapshots
re-synced at `2026-09-18T10:58:44Z` (the release carrying `a41a675c`); `Renderer`
`Release|Win32` builds with no new warnings. **Not verified**: in-game smoke
tests on any engine family.

## `Mod_UnloadSpriteTextures` full gamedata migration (2026-09-19)

`Engine_FillAddress_Mod_UnloadSpriteTextures` no longer has an engine branch: it
is one unconditional `GamedataResolvePtr(..., "Mod_UnloadSpriteTextures",
MH_GAMESYMBOL_KIND_FUNCTION)` for every identity. The SvEngine
`Search_Pattern(MOD_UNLOADSPRITETEXTURES_SVENGINE, DllInfo)` +
`ConvertDllInfoSpace` + `Sig_FuncNotFound` branch is gone, and both
`MOD_UNLOADSPRITETEXTURES_BLOB` (already dead) and
`MOD_UNLOADSPRITETEXTURES_SVENGINE` were deleted from the `#define` catalogue.

**Catalog coverage (why the branch was removable).** All 11 engine identities
publish an engine-module `function` record for `Mod_UnloadSpriteTextures` —
Windows on all 11, plus Linux on hl-8684, hl-10210, svencoop-8948 and
svencoop-10257. The old comment ("SvEngine publishes no catalog record for
`Mod_UnloadSpriteTextures`, `Draw_FillRGBA`, `Draw_FillRGBABlend` or
`D_FillRect`") was stale for this one symbol only; it then covered just the
other three and moved above `Engine_FillAddress_Draw_FillRGBA`. That remaining
comment is gone too — see the 2D fill migration below.

**`SPR_Shutdown` is not an extra hook site.** `Mod_UnloadSpriteTextures` is
*not* inlined into `SPR_Shutdown` on any supported identity, so the existing
`Install_InlineHook(Mod_UnloadSpriteTextures)` already covers the HUD sprite
list that `ClientDLL_Shutdown` → `SPR_Shutdown` walks. Disassembling
`bin/<tag>/engine/hw.dll` at the recorded `SPR_Shutdown` RVA shows exactly the
call shape of the reference `SPR_Shutdown` (`HLND2T engine/cl_draw.c:128`) — one
`E8` to the recorded `Mod_UnloadSpriteTextures` RVA plus two `E8`s to the same
`Mem_Free` helper:

| Windows identity | `SPR_Shutdown` | `Mod_UnloadSpriteTextures` | calls it |
| --- | --- | --- | --- |
| cof-5936 | `0x29391` (`0xcd`) | `0x661ab` | yes |
| hl-4554 | `0x1e680` (`0x84`) | `0x4e380` | yes |
| hl-6153 | `0x11500` (`0x84`) | `0x41790` | yes |
| hl-8684 | `0x117e0` (`0x84`) | `0x42890` | yes |
| hl-10210 | `0x19d140` (`0x8f`) | `0x2417b0` | yes |
| svencoop-8948 | `0x21250` (`0x8f`) | `0x9e20` | yes |
| svencoop-10257 | `0x212c0` (`0x8f`) | `0x9c80` | yes |

hl-3248 / 3266 / 3329 / 3647 ship an encrypted `hw.dll`, so they were not
byte-checked here; upstream's `[[SPR_Shutdown locator]]` /
`[[Mod_UnloadSpriteTextures locator]]` make that caller edge a fail-closed
discovery invariant (the producer requires one caller that calls the target
once, calls one free helper twice and walks a 12-byte `SPRITELIST`) and report
15/15 validated engine/platform targets, blob builds included.

**Pitfall worth remembering.** `CL_Disconnect` calls
`SPR_Shutdown_NoModelFree`, which clears `gSpriteList` / `gSpriteCount` but
deliberately never calls `Mod_UnloadSpriteTextures`. Sprite textures are not
leaked on disconnect: the `model_t`s stay in `mod_known` and the plugin's
`Host_ClearMemory` / `GL_UnloadTextures` hooks flush them on the next map load.
Adding a `SPR_Shutdown` hook would not change that path either.

**Consumer gate.** `Mod_UnloadSpriteTextures` moved from
`RENDERER_ENGINE_E8_FUNCTIONS` (cof + 8 hl) into `RENDERER_ENGINE_ALL_FUNCTIONS`
(all 11), so the gate now fails if any identity drops the record.
`RENDERER_ENGINE_E8_FUNCTIONS` keeps only `GL_SelectPixelFormat` and
`GlowBlend`, which really are inlined on HL25 / SvEngine. Test:
`test_gate_requires_mod_unloadspritetextures_on_every_identity`.

**Verified**: 51/51 contract tests; `validate-gamedata.py` over the 21 packaged
snapshots; `Renderer` `Release|Win32` builds with no new warnings. **Not
verified**: in-game smoke tests on any engine family.

## 2D fill migration: `Draw_FillRGBABuf` replaces `NET_DrawRect` (2026-09-19)

The last three legacy signature scans in `Engine_FillAddress_*` are gone.
Upstream GSV PRs #149 and #150 (issues #147 / #148) changed what the catalog
publishes for SvEngine, and this is the consumer-side adoption.

**`Draw_FillRGBA` / `Draw_FillRGBABlend` are now `ALL` (11 identities).** The old
comment claimed SvEngine publishes no record for them. The real cause was
different: SvEngine keeps `cl_enginefuncs` slots 11 and 130, but those entries
only *forward* to the drawing body — a direct `JMP` thunk on Windows, an
eight-int cdecl wrapper (plus PLT/GOT on 8948) on Linux. The upstream finder
previously validated GL behaviour at the forwarding entry and therefore emitted
nothing. It now resolves through the thunk, so all four SvEngine targets publish
the real bodies and both `Engine_FillAddress_Draw_FillRGBA` and
`Engine_FillAddress_Draw_FillRGBABlend` are one unconditional
`GamedataResolvePtr`. `DRAW_FILLEDRGBA_SVENGINE` and
`DRAW_FILLEDRGBABLEND_SVENGINE` were deleted.

| Body RVA | 10257 Win | 10257 Linux | 8948 Win | 8948 Linux |
| --- | --- | --- | --- | --- |
| `Draw_FillRGBA` | `0x4f970` | `0x127f70` | `0x4f6d0` | `0x174a60` |
| `Draw_FillRGBABlend` | `0x4faa0` | `0x1280c0` | `0x4f800` | `0x174bb0` |

**`NET_DrawRect` was the wrong name and is retired without an alias.** The
address the old `D_FILLRECT_SVENGINE` / `NET_DRAWRECT_SVENGINE` byte pattern hit
is a *buffered* rectangle function taking eight integers
`(x, y, w, h, r, g, b, a)`; it appends four 24-byte vertices to a 1024-entry
buffer and flushes with `glDrawArrays(GL_QUADS, ...)` +
`glBlendFunc(GL_SRC_ALPHA, GL_ONE)`. Linux 8948 carries the real symbol
`_Z16Draw_FillRGBABufiiiiiiii`, so the catalog now publishes it as
`Draw_FillRGBABuf` on all four SvEngine targets and **removed** the
`NET_DrawRect` records. `gPrivateFuncs.NET_DrawRect`, its hook slot and the
`gl_rmain.cpp` handler were renamed accordingly; the handler body is unchanged
because the ABI and the additive blend state already matched.

| `Draw_FillRGBABuf` | 10257 Win `0x51600` | 10257 Linux `0x12a590` | 8948 Win `0x513b0` | 8948 Linux `0x177080` |

**The earlier `/OPT:ICF` theory was withdrawn upstream.** Two byte-identical
patterns did *not* prove the linker folded `NET_DrawRect` with `D_FillRect` —
there was only ever one body, and it never had `D_FillRect`'s two-pointer shape.

**`D_FillRect` is now non-SvEngine-only.** SvEngine has no body with the legacy
`(vrect_t*, unsigned char*)` interface: its connection-message rectangle calls
the eight-int `Draw_FillRGBABlend(x, y, w, h, 0, 0, 0, 255)` instead, which this
plugin already hooks separately. `Engine_FillAddress_D_FillRect` returns early on
`ENGINE_SVENGINE` and `D_FILLRECT_SVENGINE` was deleted, so the SvEngine build no
longer installs a two-pointer hook on an eight-int function. That mismatch
existed before this change (both legacy patterns resolved to the same buffered
body), so this removes a real latent ABI bug, not a newly introduced one.

**Consumer gate.** `Draw_FillRGBA` / `Draw_FillRGBABlend` moved from
`RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS` into `RENDERER_ENGINE_ALL_FUNCTIONS`;
`NET_DrawRect` was replaced by `Draw_FillRGBABuf` in
`RENDERER_ENGINE_SVENGINE_FUNCTIONS`; `D_FillRect` stays non-SvEngine. Tests:
`test_gate_requires_rgba_fill_bodies_on_every_identity`,
`test_gate_requires_draw_fillrgbabuf_on_svengine_only`,
`test_gate_no_longer_references_the_retired_net_drawrect_name`,
`test_gate_skips_d_fillrect_on_svengine`.

**Verified**: 88 contract tests pass (4 new); `validate-gamedata.py` passes over
the 21 packaged snapshots, which were re-synced from the upstream index on
2026-09-19 and carry the new records; `Renderer` `Release|Win32` builds with no
new warnings. **Not verified**: in-game smoke tests on any engine family — in
particular nobody has confirmed on a running SvEngine client that the netgraph
rectangle still draws through the renamed hook.

## Lightmap / filter / multitexture migration (2026-09-19): 19 more catalog-backed symbols

Upstream `GoldSrc_VibeSignatures` PRs #153 / #155 / #157 (commits `e2836de`,
`856a283`, `c7a87fb`, release `2026-09-19T10:17:32Z`) published a new batch of
renderer private symbols. Every one of them is now resolved through
`ResolveGameSymbol` and the corresponding signature / `DisasmRanges` locators
were deleted.

**Engine functions (kind FUNCTION)**

| Symbol | Coverage | Note |
| --- | --- | --- |
| `R_TextureAnimation` | 11/11 | `Engine_FillAddress_R_TextureAnimation` deleted, now inline in the dispatch together with `rtable`; `R_TEXTUREANIMATION_SIG_{BLOB,NEW,NEW2,HL25,SVENGINE}` removed |
| ~~`R_RenderDynamicLightmaps`~~ | — | **Withdrawn 2026-09-19**: the field it fed was never called, so the gamedata resolve and the gate entry were deleted outright. The function that also resolved `d_lightstylevalue` survives as `Engine_FillAddress_LightstyleVars`. See the dead-code section at the end. |
| ~~`GL_EnableMultitexture`~~ | — | **Withdrawn the same day**: the symbol has no consumer at all, so it was deleted outright instead of wired to gamedata. See the dead-code section at the end. |

**Engine globals (kind GLOBAL, 11/11 identities): 16**

`frustum` (was the `mov esi, offset frustum` walk in
`Engine_FillAddress_R_CullBox`; the `vpn` / `vup` / `vright` pattern chain that
is keyed off the resolved `frustum` address is unchanged and still
catalog-uncovered), `rtable`,
`lightmaps`, `lightmap_textures`, `lightmap_rectchange`, `gDecalSurfs`,
`gDecalSurfCount`, `d_lightstylevalue`, `filterMode`, `filterColorRed`,
`filterColorGreen`, `filterColorBlue`, `filterBrightness`.

**Locators deleted**

- `Engine_FillAddress_GL_SelectTexture` — the whole function is gone; the
  dispatch now resolves `GL_SelectTexture` (FUNCTION) inline and `oldtarget` was
  deleted (see the dead-code section). This retires the last of the three
  "retained disasm roots" from the #873 migration (`GL_Init` still extracts
  `gl_extensions`).
- `Engine_FillAddress_R_DrawSequentialPoly`'s 230-line BFS (`std::set` code /
  branch tracking, `mov [reg+0x38]` lightmap anchor, decal-surface register
  heuristic, and the nested `imm 0x14` + `push 0x200` probe that identified
  `R_RenderDynamicLightmaps`) — replaced by five `GamedataResolvePtr` calls. The
  engine-type split that stores the HL25 three-argument field is kept.
- `Engine_FillAddress_SetFilterMode` / `_SetFilterColor` / `_SetFilterBrightness`
  — all three deleted. They anchored on the public `gEngfuncs.pfnSetFilter*`
  entry points and disassembled the first store(s), with per-engine
  `FSTP` / `MOVSS` / `MOV` branches and a `qsort` to order R/G/B. The five slots
  are now five inline resolves in the dispatch. The upstream `SetFilterMode` /
  `SetFilterColor` / `SetFilterBrightness` (9 identities) and `*_I`
  (2 SvEngine) function records exist but the plugin consumes none of them — it
  only ever wanted the globals, and it calls the filter setters through the
  public engine interface.
- `Engine_FillAddress_GL_EnableMultitexture` / `_GL_DisableMultitexture` —
  deleted outright rather than migrated, see the dead-code section.
- `Engine_FillAddress_R_RenderDynamicLightmaps`'s `cmp al, 0FFh` +
  `mov reg, d_lightstylevalue[reg*4]` pair. The retained pass keeps only
  `lightmap_polys` / `lightmap_modified` and is now rooted on the
  gamedata-resolved real address with `ctx = { RealDllInfo, RealDllInfo }`
  (it previously converted back into `DllInfo` space).

**Stale-claim correction: `GL_UnloadTextures` was never catalog-uncovered**

The #873 notes listed `GL_UnloadTextures` under "residual scanning
(catalog-uncovered)". That was wrong: upstream `9c22ab4` (2026-09-13, before the
#873 migration) publishes it as an engine FUNCTION on all 11 identities, so the
earlier "no remaining catalog-covered locator" sweep missed it — its assignment
came from a plain local (`ctx.candidateE8_VA`), not from a `<name>_VA`-shaped
one, so the name-matching sweep did not flag it. The heuristic in
`Engine_FillAddress_R_NewMap` (walk `R_NewMap` for 0x500 bytes, keep the last
5-byte `E8` seen before the first `RET`) is deleted and replaced by a
`GamedataResolvePtr`. The `R_ClearParticles` / `R_DecalInit` / `V_InitLevel`
four-`E8` pattern in the same function stays — those three really are
catalog-uncovered.

**Residual scanning after this pass** (unchanged from the #873 list except for
the entries above): `R_SetupFrame`, `R_ClearParticles` / `R_DecalInit` /
`V_InitLevel`, ~~`R_LoadSkyboxInt_SvEngine`~~, `realloc_SvEngine`,
`particletexture`, ~~`r_dlightactive`~~, `r_framecount`, `gWaterColor`,
`vpn` / `vup` / `vright`, `lightmap_polys` / `lightmap_modified`,
`gl_extensions` / `vid_d3d` / texture-array / fog / scissor / viewmodel / sky /
view-leaf slots, the EngineSurface virtuals and the portal / DrawNormalTriangles
scans.

**Consumer gate**

`scripts/validate-gamedata.py`: `R_RenderDynamicLightmaps`, `R_TextureAnimation`
and `GL_UnloadTextures` added to `RENDERER_ENGINE_ALL_FUNCTIONS`; the 16 globals
added to `RENDERER_ENGINE_ALL_GLOBALS` (minus the three retired below). New tests in
`scripts/tests/test_gamedata_contract.py::RendererGateTests`:
`test_gate_requires_lightmap_and_decal_symbols_on_every_identity`,
`test_gate_requires_screen_filter_globals_on_every_identity`,
`test_gate_requires_multitexture_globals_on_every_identity`,
`test_gate_requires_gl_enablemultitexture_on_non_svengine_only`.

**Verified**: 59 contract tests (55 → 59, 4 new) and the full
`scripts/tests` suite (92 passed / 2 skipped) pass; `validate-gamedata.py`
passes over the 21 snapshots re-synced from the upstream index at
`2026-09-19T10:17:32Z`; `Renderer` builds `Release|Win32` with `gl_hooks.cpp`
recompiled from scratch and no new warnings. **Not verified**: in-game smoke
tests on any engine family — in particular nobody has confirmed that the
screen-filter globals read correctly now that they come from the catalog instead
of the `pfnSetFilter*` disassembly.

## Dead-code removal (2026-09-19): the multitexture pair, `gl_mtexable` / `mtexenabled`, `oldtarget`

Follow-up to the section above, in direct response to the review question
*"are `GL_EnableMultitexture` / `GL_DisableMultitexture` actually used?"* —
**they are not**, and neither are the three globals that travelled with them.
Wiring `GL_EnableMultitexture` to gamedata in the previous pass was the wrong
call: it created a release-gate dependency on a record with no consumer. All of
it is deleted instead.

**Evidence (whole repo, excluding `thirdparty/`)**

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `GL_EnableMultitexture` / `GL_DisableMultitexture` (plugin wrappers) | definition `gl_draw.cpp:622-630`, declaration `gl_local.h:516-517` | **zero callers** |
| `gPrivateFuncs.GL_EnableMultitexture` / `.GL_DisableMultitexture` | field decl `privatehook.h:49-50`, the two locators, and the dead wrappers | never `Install_InlineHook`-ed |
| `gl_mtexable` / `mtexenabled` | definition `gl_rmain.cpp:132-133`, extern `gl_local.h:233/235`, the locator assignment | **never read** |
| `oldtarget` | definition `gl_draw.cpp:16`, extern `gl_draw.h:13`, the resolve | **never read** |

**When each died (git `-S` archaeology, not inference)**

- `69cfa251` "Fix portal." (2025-10-06) deleted `GL_PushDrawState` /
  `GL_PopDrawState` from `gl_rmisc.cpp`. That pair held the only read of
  `*mtexenabled` (`saved.mtex = *mtexenabled`) and the only two calls to the
  wrappers (`if (saved.mtex && !(*mtexenabled)) GL_EnableMultitexture(); else
  ...`). Before it, `gl_hud.cpp` / `gl_light.cpp` carried 7–10 call sites going
  back to 2021 — this was live fixed-function state-stack code that the Core
  Profile rewrite retired.
- `a5bfd6a5` "fix #610" (2025-06-02) removed the last `glActiveTexture((*oldtarget))`
  reads from `gl_studio.cpp` (4 sites, still present at `de20e644`, 2025-03-03).

**Second reason the multitexture pair could never do anything useful.** Since
the `LegacyMultiTextureInit` retarget (2026-09-18) the plugin neuters
`CheckMultiTextureExtensions` on cof-5936 / hl-4554 / hl-6153 / hl-8684, so
`gl_mtexable` is pinned at 0 there; the engine's own `GL_EnableMultitexture`
opens with `cmp gl_mtexable, 0` and returns immediately. Even restoring a caller
would have been a no-op on those four identities.

**Deleted**

- `Engine_FillAddress_GL_DisableMultitexture` (35 lines) and
  `Engine_FillAddress_GL_EnableMultitexture` (26 lines), plus both dispatch calls.
  Nothing else used them as an anchor: the Disable locator was rooted on
  `R_NewMap` and the Enable locator on Disable, and both chains ended there.
- All five sig macros: `GL_DISABLEMULTITEXTURE_SIG_{BLOB,NEW,HL25,SVENGINE}` and
  `GL_ENABLEMULTITEXTURE_SIG_SVENGINE`.
- The two `private_funcs_t` fields, the two wrappers and their declarations.
- `gl_mtexable` / `mtexenabled` / `oldtarget`: definitions, externs and resolves.

**Consumer gate reverted accordingly.** `GL_EnableMultitexture` removed from
`RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS`; `gl_mtexable` / `mtexenabled` /
`oldtarget` removed from `RENDERER_ENGINE_ALL_GLOBALS`. The net gamedata
adoption for 2026-09-19 is therefore **16 symbols, not 19**: 2 engine functions
(`R_TextureAnimation`, `R_RenderDynamicLightmaps`) + 13 engine globals + the
`GL_UnloadTextures` correction. `test_gate_requires_multitexture_globals_on_every_identity`
and `test_gate_requires_gl_enablemultitexture_on_non_svengine_only` were replaced
by `test_gate_ignores_symbols_the_renderer_no_longer_consumes`, which asserts all
five retired names stay out of every `RENDERER_*` table — a regression guard
against re-adding a gate entry for something nobody consumes.

**Stale comment fixed.** The two `DisasmRanges` predicates in
`Engine_FillAddress_R_MarkLeaves` carried a copy-pasted
`//01D57970 83 3D ... cmp gl_mtexable, 0` comment while actually matching
`mov ecx, r_viewleaf` / `mov r_oldviewleaf, ecx`. Corrected to the real
instruction shapes.

**Rule worth keeping.** Before migrating a newly published symbol to gamedata,
check that the plugin actually *reads* it. `loadname` / `loadmodel` (2026-09-18)
and these five are the same failure mode: upstream publishing a record is not
evidence that the consumer needs it, and adding it to the gate converts dead
code into a release-blocking dependency.

**Verified**: `Renderer` `Release|Win32` builds clean — the 9 remaining warnings
are byte-identical to the pre-change baseline; `validate-gamedata.py` passes over
the 21 snapshots; 91 passed / 2 skipped across `scripts/tests` (25 Renderer gate
tests). **Not verified**: in-game smoke tests on any engine family.

## Dead-code removal (2026-09-19): `R_AddDynamicLights` and its `R_BuildLightMap` anchor

Direct follow-up to the section above, in answer to the review question *"is
`R_AddDynamicLights` also unused at the semantic level?"* — **it is, in both of
its forms**, and deleting it exposed a second consumer-less resolve behind it.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| plugin stub `R_AddDynamicLights` | definition `gl_rsurf.cpp:88`, declaration `gl_wsurf.h:346` | **zero callers** — body was `//All moved to shader` |
| `gPrivateFuncs.R_AddDynamicLights` | field `privatehook.h:77`, locator `Engine_FillAddress_R_AddDynamicLights` (`gl_hooks.cpp:1444-1566`), `g_phook_R_AddDynamicLights` | never called (`git log -S 'gPrivateFuncs.R_AddDynamicLights('` empty across all history), never `Install_InlineHook`-ed |
| `gPrivateFuncs.R_BuildLightMap` | field `privatehook.h:76`, locator `Engine_FillAddress_R_BuildLightMap` (`gl_hooks.cpp:1379-1442`), `g_phook_R_BuildLightMap` | only remaining reader was the `R_AddDynamicLights` BFS root (`gl_hooks.cpp:1450`); never called (`git log -S 'gPrivateFuncs.R_BuildLightMap('` empty) |

**When the plugin stub died.** `f8607c28` "Rewrite GL_BuildLightmaps"
(2023-02-23) commented out its last caller (`// R_AddDynamicLights(psurf);`); the
body had already become a no-op once lighting moved into `R_BuildLightMap`, which
the plugin reimplements itself (`gl_rsurf.cpp:124`, still live, called from
`R_BuildSurfaceLightmap` at `gl_rsurf.cpp:268`). The engine's own `R_BuildLightMap`
address was therefore never needed by the plugin at all.

**Deleted**

- Plugin stub `R_AddDynamicLights` and its `gl_wsurf.h` declaration.
- `Engine_FillAddress_R_BuildLightMap` (64 lines) and
  `Engine_FillAddress_R_AddDynamicLights` (~123 lines), plus both dispatch calls.
- Both `private_funcs_t` fields and both `g_phook_*` variables (unused file-scope
  statics; MSVC emits no warning for these, so they had gone unnoticed).
- All four `R_BUILDLIGHTMAP_SIG_{BLOB,NEW,HL25,SVENGINE}` and all four
  `R_ADDDYNAMICLIGHTS_SIG_{SVENGINE,HL25,NEW,BLOB}` macros.

**Also removed in the same pass.** The plugin-local `R_RenderDynamicLightmaps`
stub (empty body `//All moved to shader`, decl `gl_wsurf.h:346`) had zero callers
too; it was deleted as well. It was a *different* symbol from the live
`gPrivateFuncs.R_RenderDynamicLightmaps`, which stays: that field is resolved by
`Engine_FillAddress_R_RenderDynamicLightmaps` and consumed as the
`lightmap_polys` / `lightmap_modified` disasm root at `gl_hooks.cpp:5018`. The
plugin-local `R_BuildLightMap` (`gl_rsurf.cpp:124`) is a live reimplementation —
kept.

**Rule, third instance.** Same failure mode as `loadname` / `loadmodel`
(2026-09-18) and the multitexture pair: the plugin carried a full sig-scan
locator (8 macros + 2 resolver functions) for a symbol nothing read. The
`R_BuildLightMap` case adds a corollary: when resolver B exists *only* as the
anchor for resolver A, deleting A must delete B too, or the anchor becomes the
next piece of dead code.

**Verified**: `Renderer` `Release|Win32` builds clean (same 9 pre-existing
warnings; one shifted by -4 lines); `validate-gamedata.py` passes over 21
snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests across
`scripts/tests`. **Not verified**: in-game smoke tests on any engine family.

## Dead-code removal (2026-09-19): the sky-var mirror and `R_LoadSkyboxInt_SvEngine`

The catalog re-sync of `2026-09-19T12:42:08Z` newly published `gSkyTexNumber`
(engine GLOBAL, 11/11), `gLoadSky` (engine GLOBAL, 11/11) and
`R_LoadSkyboxInt_SvEngine` (engine FUNCTION, 2/11 — the two SvEngine identities).
All three were **deleted rather than migrated**: the plugin has no reader for any
of them. (`gLoadSky` is the engine's real name for the local `r_loading_skybox`.)

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gSkyTexNumber` | def `gl_wsurf.cpp:30`, extern `gl_local.h:303`, SvEngine BFS in `Engine_FillAddress_R_LoadSkybox` | **write-only** |
| `r_loading_skybox` (= engine `gLoadSky`) | def `gl_wsurf.cpp:31`, extern `gl_local.h:111`, vars BFS anchored on `R_LoadSkyBox_SvEngine`/`R_LoadSkys` | **write-only**; `DrawSkybox.md` already stated it "does not further participate in the local skybox flow" |
| `gPrivateFuncs.R_LoadSkyboxInt_SvEngine` | field `privatehook.h:105`, `"SKY: "` search + `ReverseSearchFunctionBeginEx(0x600)` | write-only; its sole purpose was to root the `gSkyTexNumber` BFS |

**Deleted**

- The entire `Engine_FillAddress_R_LoadSkybox` disasm body (185 → 11 lines):
  `"SKY: "` data/`.rdata` string search, the `75 ?? 68 <str>` push-site probe,
  the `ReverseSearchFunctionBeginEx` walk, and both `DisasmRanges` BFS blocks
  (SvEngine `gSkyTexNumber` extractor over `0x100`, vars BFS over `0x50`). The
  function is now two `GamedataResolvePtr` calls: `R_LoadSkyBox_SvEngine` on
  SvEngine, `R_LoadSkys` otherwise. The `R_LoadSkyBox_SvEngine_VA` / `R_LoadSkys_VA`
  locals and the two stale commented-out local declarations went with it.
- `private_funcs_t::R_LoadSkyboxInt_SvEngine`, plus the `gSkyTexNumber` /
  `r_loading_skybox` globals (`gl_wsurf.cpp`) and both `gl_local.h` externs.

**Not added to the gate.** `R_LoadSkyboxInt_SvEngine` is published but nothing
consumes it, so it is deliberately not wired to gamedata and not added to any
`RENDERER_*` table — the same rule as the multitexture pair.

**Rule, fourth instance.** `loadname` / `loadmodel` (2026-09-18), the multitexture
pair, `R_BuildLightMap` / `R_AddDynamicLights`, and now these three are all the
same failure mode: a published record is not evidence that the consumer needs it.
A freshly synced catalog is a reason to *re-check* the locator, not a reason to
wire it up.

**Verified**: `Renderer` `Release|Win32` builds clean (same 9 warnings);
`validate-gamedata.py` passes over 21 snapshots / 5 engine families; 91 passed /
2 skipped / 26 subtests across `scripts/tests`. **Not verified**: in-game smoke
tests on any engine family.

## Dead-variable sweep (2026-09-19): 50 resolved-but-unread engine mirrors

Systematic audit of every Renderer `extern` engine mirror and every Renderer
file-scope global: for each symbol, count assignments against real reads,
treating `decltype(x)` / `sizeof(x)` as unevaluated operands (not reads) and
`Install_InlineHook(Name)` / `Uninstall_Hook(Name)` as macro uses of
`g_phook_Name`. 50 symbols were assigned (or merely declared) and never read.

**Methodology caveats worth keeping.** (1) `Install_InlineHook(fn)` is a
token-paste macro (`g_phook_##fn`), so `grep g_phook_X` cannot prove a hook is
unused — grep `Install_InlineHook(X)` instead. (2) Classify *per plugin*: a
repo-wide scan counts another plugin's same-named global as a read. Renderer's
`g_iUser1` / `g_iUser2` looked live only because BulletPhysics and SCCameraFix
have their own `g_iUser1` / `g_iUser2` that are read. (3) `X = (decltype(X))expr;`
is not a self-read; without this an entire class of write-only fields hides.

**Engine cvar mirrors fetched via `gEngfuncs.pfnGetCvarPointer` then never dereferenced (27).**
`gl_affinemodels`, `gl_finish`, `gl_flashblend`, `gl_flipmatrix`, `gl_fog`,
`gl_lightholes`, `gl_max_size`, `gl_monolights`, `gl_nocolors`, `gl_overdraw`,
`gl_picmip`, `gl_playermip`, `gl_polyblend`, `gl_reporttjunctions`,
`gl_round_down`, `gl_smoothmodels`, `gl_texsort`, `gl_wateramp`, `gl_zmax`,
`r_bmodelhighfrac`, `r_bmodelinterp`, `r_decals`, `r_dynamic`, `r_mirroralpha`,
`r_mmx`, `r_wadtextures`, `r_wateralpha`. `ati_npatch`, `ati_subdiv` and
`gl_watersides` were declared but never even resolved. Definitions, `gl_local.h`
externs and the `R_InitCvars` resolve lines are all deleted.

**Other engine / plugin globals (17).** `DM_RemapSkin`, `pDM_RemapSkin`,
`r_remapindex`, `g_NormalIndex` — their definitions sat inside `#if 0`, so only
the stale `gl_studio.h` externs and the disabled definitions remained.
`currenttexid` (`gl_draw.h`), `gRenderMode`, `s_BlendBufferFBO` (`gl_local.h`),
`pheader` (`enginedef.h`; BulletPhysics keeps its own `pheader`) and
`g_OITBlendObjects[512]` + `g_iNumOITBlendObjects` (`gl_wsurf.h`) were extern-only
declarations with no definition at all. `g_bIsHL1MMOD` was set to `true` for the
HL1MMod game dir and never read, so the whole
`if (!stricmp(gEngfuncs.pfnGetGameDirectory(), "HL1MMod")) { ... }` block is gone.
`g_pGameStudioRenderer`, `r_screenaspect` and `NUM_MRT` complete the set.

**`gPrivateFuncs` fields declared but never assigned nor called (6).**
`BuildGlowShellVerts`, `CL_IsThirdPerson`, `GL_Upload16`, `R_DecalMPoly`,
`R_DecalShootInternal`, `R_DrawDecals`. Note the same-named live entities stay:
the client-export `gExportfuncs.CL_IsThirdPerson` and the plugin's own
`R_DrawDecals(cl_entity_t*)` — different symbols from the deleted struct fields.
(Correction 2026-09-21: there never was a plugin-side `R_DecalShootInternal`
definition either; the orphaned `gl_local.h` declaration was deleted later.)

**Gate change.** `g_pGameStudioRenderer` left `RENDERER_CLIENT_STUDIO_GLOBALS`,
which is now `("g_iUser1", "g_iUser2")`. (BulletPhysics had already dropped the
same symbol on 2026-09-12 for the same reason.)

**Still open / deliberately not touched.** The 12 `gPrivateFuncs.triapi_*`
originals saved at `gl_hooks.cpp:6093-6107` (the plugin's triAPI wrappers
reimplement everything and only `triapi_Fog` / `triapi_FogParams` /
`triapi_SpriteTexture` call the saved originals). The five guard-only globals
whose only "use" is `Sig_VarNotFound` (`r_soundOrigin`, `scissor_x`, `scissor_y`,
`scissor_width`, `scissor_height`). And `g_StudioRendererRendermode` (`static`,
write-only). All three groups were reported but excluded from this sweep.

**Verified**: `Renderer` `Release|Win32` builds with the same 9 pre-existing
warnings and 0 errors; `validate-gamedata.py` passes over 21 snapshots / 5 engine
families; 91 passed / 2 skipped / 26 subtests. **Not verified**: in-game smoke
tests.

## Dead-variable sweep, follow-up (2026-09-19): 7 gated-but-dead globals + `DM_PlayerState`

Second pass over the same audit's overflow: these were already *required by the
release gate* yet never read by the plugin.

| Symbol | Gate table | Note |
| --- | --- | --- |
| `gDecalSurfs`, `lightmap_rectchange`, `lightmap_textures` | `RENDERER_ENGINE_ALL_GLOBALS` | added by `144d10c7`, whose "16 symbols with consumers" claim did not hold for these three |
| `g_ChromeOrigin`, `r_model` | `RENDERER_ENGINE_ALL_GLOBALS` | resolved at the top of `EngineStudio_FillAddress`, never dereferenced |
| `g_iUser1`, `g_iUser2` | `RENDERER_CLIENT_STUDIO_GLOBALS` | resolved at the top of `Client_FillAddress`, never read |

All seven left their gate tables; `RENDERER_CLIENT_STUDIO_GLOBALS` is now `()`.
`test_gate_requires_lightmap_and_decal_symbols_on_every_identity` lost the three
lightmap/decal names accordingly.

Also deleted `DM_PlayerState`: Renderer's `gl_studio.h` extern plus the leftover
`#if 0//unused` definition. The `DM_PlayerState` entry at `validate-gamedata.py:49`
belongs to SCModelDownloader's `COMMON_REQUIRED` set and is untouched.

Definitions, externs and the `GamedataResolvePtr` / `EngineStudio_FillAddress`
resolve lines for all eight symbols are gone.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-19): `R_StudioChromeVars` and the `R_StudioChrome` chain

Review question: does anything `Engine_FillAddress_R_StudioChromeVars` locates
have a real consumer? No — and its anchor did not either, so the whole chain is
gone.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `chrome` | def `gl_studio.cpp:55`, extern `gl_studio.h:329`, three `chrome = ConvertDllInfoSpace(...)` writes in the locator | **write-only** |
| `chromeage` | def `gl_studio.cpp:56`, extern `gl_studio.h:328`, three writes in the locator | **write-only** |
| `gPrivateFuncs.R_StudioChrome` | field `privatehook.h:217`, `Engine_FillAddress_R_StudioChrome` (sig-only), read **only** as the `Engine_FillAddress_R_StudioChromeVars` GOLDSRC disasm base | never called, never hooked |

**Deleted**

- `Engine_FillAddress_R_StudioChromeVars` (68 lines) and
  `Engine_FillAddress_R_StudioChrome` (34 lines), plus both dispatch calls.
- The `chrome` / `chromeage` globals and their `gl_studio.h` externs.
- Six function-local `#define`s (`CHROMEAGE_SIG_SVENGINE`, `CHROME_SIG_SVENGINE`,
  `CHROMEAGE_SIG_HL25`, `CHROME_SIG_HL25`, `CHROMEAGE_SIG`, `CHROME_SIG_NEW`) went
  with the `R_StudioChromeVars` body; five file-scope `R_STUDIOCHROME_SIG_*`
  macros and the `private_funcs_t::R_StudioChrome` field went with the other
  locator.

`gPrivateFuncs.R_GLStudioDrawPoints` stays: it is gamedata-resolved,
`Install_InlineHook`-ed, and also used as a disasm base by another resolver.

**Corollary, second instance.** When resolver B exists only as an anchor for
resolver A, deleting A must delete B — the same shape as `R_BuildLightMap`
anchoring `R_AddDynamicLights`. Here `R_StudioChrome` was kept alive solely to
feed `R_StudioChromeVars`'s GOLDSRC branch.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-19): the `R_RenderDynamicLightmaps` anchor and its BFS globals

Review question: does `Engine_FillAddress_R_RenderDynamicLightmaps` locate anything
with a real consumer? It located four things; three were dead, so the function was
slimmed to its one live resolve rather than deleted.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.R_RenderDynamicLightmaps` | field `privatehook.h:66`, resolve + `_VA` in the locator | never called, never hooked |
| `lightmap_polys` | def `gl_rsurf.cpp:10`, extern `gl_wsurf.h:306`, `DisasmRanges` BFS write | **write-only** |
| `lightmap_modified` | def `gl_rsurf.cpp:9`, extern `gl_wsurf.h:305`, BFS write | **write-only** |
| `d_lightstylevalue` | resolve `gl_hooks.cpp:4804`, reads `gl_rsurf.cpp:567` + `gl_wsurf.cpp:5218` | **live** |

**Deleted**

- `gPrivateFuncs.R_RenderDynamicLightmaps` field and its `GamedataResolvePtr` call.
- The `R_RenderDynamicLightmaps_SearchContext` struct, the `DisasmRanges` lambda
  that walked the function for `lightmap_polys` / `lightmap_modified`, the
  `R_RenderDynamicLightmaps_VA` alias, and both `Sig_VarNotFound` calls.
- The `lightmap_polys` / `lightmap_modified` globals and their `gl_wsurf.h` externs.
- `R_RenderDynamicLightmaps` from `RENDERER_ENGINE_ALL_FUNCTIONS`
  (`validate-gamedata.py`) and from the `test_gamedata_contract.py` tuple;
  `d_lightstylevalue` stays gated.

**Renamed.** `Engine_FillAddress_R_RenderDynamicLightmaps` →
`Engine_FillAddress_LightstyleVars`, body reduced to the single
`d_lightstylevalue` resolve. The original name no longer described what it does.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-20): `skychain` / `waterchain` and the `test_cl` machinery

Review question: can `skychain` and `waterchain` be cleaned up? Yes — they were
write-only. The other two globals the same BFS finds are live.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `skychain` | def `gl_rsurf.cpp:4`, extern `gl_local.h:294`, two writes in `Engine_FillAddress_R_RecursiveWorldNodeVars`, its own early-exit check, `Sig_VarNotFound` | **write-only** |
| `waterchain` | def `gl_rsurf.cpp:5`, extern `gl_local.h:295`, same shape | **write-only** |
| `r_framecount` | reads `gl_rmain.cpp:2988/3951/4532/4554/4557`, `gl_water.cpp:1102` | **live** |
| `r_visframecount` | reads `gl_rmain.cpp:1199/4532/4554/4557` | **live** |

The two chains were only ever read back by the locator's own
`if (r_visframecount && r_framecount && skychain && waterchain) return TRUE;`
early exit — a locator-internal mechanism, not a consumer.

**Deleted**

- The chain writes and the `TEST reg8,imm` / `MOV reg,[abs]` branches that
  produced them; `ctx->test_cl_flag` / `ctx->test_cl_instcount` existed solely to
  distinguish the two and went with them.
- The early exit is now `if (r_visframecount && r_framecount)`.
- Both `Sig_VarNotFound` calls, the two globals and their `gl_local.h` externs.
- The two entries in the locator's doc-comment listing the globals it finds.

No gate change: neither symbol is in gamedata or any `RENDERER_*` table.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-20): the engine-side `R_AddTEntity` locator

Review question: is `gPrivateFuncs.R_AddTEntity` used? No — and the locator's own
comment already said so (`//though engine's R_AddTEntity is not used by Renderer
anymore`, gl_hooks.cpp:3206).

**Name collision, third instance.** The plugin has its *own* `R_AddTEntity` — a
live reimplementation at gl_rmain.cpp:2146, declared at gl_local.h:432 and called
from gl_rmain.cpp:4584/4593/4611. Only the engine field is dead; the plugin
function stays.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.R_AddTEntity` | field `privatehook.h:42`, guard + assignment in `Engine_FillAddress_R_AddTEntity`, `Sig_FuncNotFound` | never called, never hooked |
| plugin `R_AddTEntity` | def `gl_rmain.cpp:2146`, decl `gl_local.h:432`, calls `4584/4593/4611` | **live** |

**Deleted**

- `Engine_FillAddress_R_AddTEntity` (49 lines) and its dispatch call.
- The `private_funcs_t::R_AddTEntity` field.

**Memory correction.** The `gPrivateFuncs.R_AddTEntity` table row claimed the
locator "Also yields `transObjects`/`maxTransObjs`"; it never did. Those three
globals (`transObjects`, `maxTransObjs`, `numTransObjs`) are resolved by
`Engine_FillAddress_R_AllocTransObjectsVars` and are all live (read/written at
gl_rmain.cpp:1969/2105/2134/2164/2172-2174/2196-2207). That resolver and its
globals are untouched.

No gate change: `R_AddTEntity` is not in gamedata or any `RENDERER_*` table.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-20): the engine-side `R_RotateForEntity` locator

Review question: is `gPrivateFuncs.R_RotateForEntity` used? No — it was write-only.

**Name collision, fourth instance.** The plugin has its *own* `R_RotateForEntity` — a
live reimplementation at gl_rmain.cpp:856, declared at gl_local.h:419 and called from
gl_wsurf.cpp:5048 and gl_water.cpp:275. The arg order is mirrored (`(cl_entity_t* e,
float out[4][4])` for the plugin vs `(float* origin, cl_entity_t* ent)` for the engine
field), which is why those two call sites look like consumers of the field at a glance.
Only the engine field is dead.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.R_RotateForEntity` | field `privatehook.h:64`, guard + 5 assignments in `Engine_FillAddress_R_RotateForEntity`, `Sig_FuncNotFound` | never called, never hooked |
| plugin `R_RotateForEntity` | def `gl_rmain.cpp:856`, decl `gl_local.h:419`, calls `gl_wsurf.cpp:5048`, `gl_water.cpp:275` | **live** |

**Deleted**

- `Engine_FillAddress_R_RotateForEntity` (48 lines) and its dispatch call.
- The `private_funcs_t::R_RotateForEntity` field.
- The five `R_ROTATEFORENTITY_*` signature macros (`_SVENGINE`, `_HL25`, `_NEW`, `_BLOB`,
  `_GOLDSRC`). `_BLOB` was dead even before this change — the BLOB branch searched `_NEW`.

**Behaviour change worth recording.** The locator ended in
`Sig_FuncNotFound(R_RotateForEntity)`, i.e. a pattern miss on any engine build was a fatal
`Sys_Error` during plugin init — a required-symbol failure mode guarding a value no code
ever read. Removing the locator removes that failure mode. (The unchecked `Search_Pattern`
results in the SvEngine/HL25/GoldSrc/BLOB branches were harmless: the null check in
`ConvertDllInfoSpace` rejects address 0, so the guard still fired.)

No gate change: `R_RotateForEntity` is not in gamedata (0 matching records across the 21
snapshots) or any `RENDERER_*` table.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-20): the write-only `_SetupRenderer` locator

Review question: are `pauxverts`/`auxverts` and `pvlightvalues`/`lightvalues` used? No — all
four were write-only, and the locator that populated them existed only to fill them.

**Not a name collision.** Unlike `R_RotateForEntity`/`R_AddTEntity`, there is no plugin-side
homonym here. The four globals were plugin-owned (`gl_studio.cpp`) mirrors of the engine's
`.data` slots — `pauxverts` held the address of the engine's `auxverts` pointer slot, `auxverts`
the array base (likewise for `pvlightvalues`/`lightvalues`) — populated from the disassembly of
`pstudio->SetupRenderer` and read nowhere in the repository.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `pauxverts` / `auxverts` / `pvlightvalues` / `lightvalues` | defs `gl_studio.cpp:48-51`, decls `gl_studio.h:321-324`, assignments + `Sig_VarNotFound` in `EngineStudio_FillAddress_SetupRenderer` | never read anywhere |
| `EngineStudio_FillAddress_SetupRenderer` | def `exportfuncs.cpp:344`, dispatch `exportfuncs.cpp:603` | sole purpose was the four assignments |
| `gPrivateFuncs.studioapi_SetupRenderer` | field `privatehook.h:217`, `pstudio->SetupRenderer` at `exportfuncs.cpp:714`, hook `exportfuncs.cpp:611` | **live** — a separate engine-side entry point, untouched |

**Deleted**

- `EngineStudio_FillAddress_SetupRenderer` (66 lines, including its `pstudio->SetupRenderer`
  anchor validation) and its dispatch call. The function had been reduced to a single
  `DisasmRanges(SetupRenderer, 0x50)` pass looking for two `C7 05 [imm32],imm32` instructions.
- The four globals: definitions in `gl_studio.cpp`, `extern` declarations in `gl_studio.h`.
  The engine's own `auxverts`/`lightvalues` are untouched — only the plugin's never-read
  mirrors are gone, so no rendering path changes. `auxvert_t` / `MAXSTUDIOVERTS` come from the
  HLSDK headers and stay.

**Behaviour change worth recording.** The locator ended in four `Sig_VarNotFound` calls, so a
`C7 05 [imm32],imm32` miss within the first `0x50` bytes of `SetupRenderer` — any engine build
whose compiler selected different instructions — was a fatal `Sys_Error` during plugin init,
guarding four values no code ever read. `EngineStudio_FillAddress` is invoked unconditionally
from `HUD_GetStudioModelInterface`, so that failure mode was live on every engine family.
Removing the locator removes it. The anchor validation it also performed was dropped with it;
the same pointer is still consumed at `exportfuncs.cpp:714` and hooked at `exportfuncs.cpp:611`.

No gate change: none of the four symbols is in gamedata (0 matching records across the 21
snapshots) or any `RENDERER_*` table.

**Verified**: build 0 errors, same 9 pre-existing warnings; `validate-gamedata.py`
passes over 21 snapshots / 5 engine families; 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-21): the `R_LightStrength` var scan and `locallight` / `numlights`

Review question: are `locallight` / `numlights` used? No — both were write-only engine
mirrors, and the locator that populated them had no other purpose.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `locallight` (`dlight_t *(*)[3]`) / `numlights` (`int*`) | defs `gl_studio.cpp:63-64`, decls `gl_studio.h:335-336`, assigned (never dereferenced) in `Engine_FillAddress_R_LightStrengthVars`, `Sig_VarNotFound` at its end | never read anywhere |
| `Engine_FillAddress_R_LightStrengthVars` | def `gl_hooks.cpp:3914`, dispatch `gl_hooks.cpp:5167` | sole purpose was the two assignments |
| `gPrivateFuncs.R_LightStrength` (+ `R_LightStrength_inlined`) | field `privatehook.h:207`/`255`, locator `Engine_FillAddress_R_LightStrength` (`gl_hooks.cpp:2958`) | left in place — see below |

**Nothing dereferenced them.** The only reads were the resolver's own `if (!locallight && …)`
guards, an early-`return TRUE` once both were set, and the two closing `Sig_VarNotFound` calls —
i.e. they worked as loop state and as a load-time assertion, never as data sources. No
`locallight[i][j]`, no `*numlights` existed.

**Not a name collision — a different, live dlight path.** The engine-internal `locallight`
(per-bone `dlight_t*` slot array) / `numlights` mirror what vanilla `R_LightStrength` feeds into
`R_StudioSetupLighting`. The plugin's studio dlight code instead walks `cl_dlights` /
`cl_elights` (defs `gl_rsurf.cpp:8-9`; both come from `GamedataResolvePtr`):
`gl_studio.cpp:2220-2222` (elight loop), `gl_studio.cpp:3513-3545` (`r_studio_legacy_dlight 0`),
`gl_studio.cpp:2558`/`2563`, `gl_rmain.cpp:5305`, `gl_light.cpp:1786`. The vanilla feed was
superseded by the `r_studio_legacy_dlight` 1/2 shader path, which is why the two mirrors were
left orphaned. (`r_dlightactive` was wrongly grouped with those live readers here; it had no
reader at all and was deleted on 2026-09-21 — see the last section.)

**Deleted**

- `Engine_FillAddress_R_LightStrengthVars` (384 lines) and its dispatch call — 387 lines total.
  Both of its paths produced nothing but the two globals: a `DisasmRanges(R_LightStrength, 0x500)`
  pass keyed on a zeroed `ebp` slot and a `XOR reg,reg`, and a `R_GLStudioDrawPoints`-rooted BFS
  (1000 instructions, depth 16, branch fan-out) for the inlined builds.
- The two globals: definitions in `gl_studio.cpp`, `extern` declarations in `gl_studio.h`.
  `R_GLStudioDrawPoints` keeps its own hook and its other callers; only its use as this BFS root
  is gone. No gamedata record exists for either symbol (0 matches in the packaged catalog), so
  no resolution gate changes. The engine's own slots are untouched.

**Behaviour change worth recording.** The deleted function ended in `Sig_VarNotFound(locallight)`
/ `Sig_VarNotFound(numlights)`, so a miss in either scan — plausible on any build whose compiler
picked different instructions or where the BFS exceeded its budget — was a fatal `Sys_Error`
during plugin init, guarding two values no code read. Removing the locator removes that failure
mode.

**Follow-up in the same session.** `gPrivateFuncs.R_LightStrength` / `R_LightStrength_inlined`
and the locator that filled them (`Engine_FillAddress_R_LightStrength` plus its six
`R_LIGHTSTRENGTH_SIG_*` defines) lost their last reader with this deletion, so they were swept in
the next section.

**Verified**: build 0 errors / same 9 pre-existing warnings (Renderer.dll regenerated);
`validate-gamedata.py` passes (21 snapshots, 5 engine families); `pytest scripts/tests`
91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-21): the write-only `R_LightStrength` locator

Follow-up to the section above. Once `Engine_FillAddress_R_LightStrengthVars` was gone,
`gPrivateFuncs.R_LightStrength` had no reader left, so the locator that filled it became
write-only by the same standard applied to `_SetupRenderer` / `R_RotateForEntity`.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.R_LightStrength` (+ `R_LightStrength_inlined`) | field `privatehook.h:207`/`255`; assigned in `Engine_FillAddress_R_LightStrength`; its only outside reader was the deleted var scan | write-only |
| `Engine_FillAddress_R_LightStrength` | def `gl_hooks.cpp:2958`, dispatch `gl_hooks.cpp:4712` | sole purpose was the two fields |
| six `R_LIGHTSTRENGTH_SIG_*` defines | `gl_hooks.cpp:118-123` | used only by the locator (`_SIG_HL25` was already dead — an empty string, never referenced) |

**Deleted**: the 36-line locator and its dispatch call; the six signature defines; both struct
fields. The `_inlined` flag was set only in the HL25 branch and read only at the locator's own
tail (`if (gPrivateFuncs.R_LightStrength_inlined) return;`), so it gated nothing.

**Behaviour change worth recording.** The locator ended in `Sig_FuncNotFound(R_LightStrength)`, a
fatal `Sys_Error` whenever the sig missed on SVEngine / GoldSrc / BLOB — removed with it. Worth
noting the SVEngine patterns locate the *inlined call site*, not a function entry, and the deleted
var scan was the only thing that ever consumed that address; nothing dereferenced or called it.

**Untouched**: `R_SetupFrame_inlined` / `R_RenderScene_inlined` and their locators,
`R_GLStudioDrawPoints` (still hooked), and the `Sig_*` machinery itself. No gamedata record exists
for `R_LightStrength` (0 matches in the packaged catalog), so no resolution gate changes.

**Verified**: build 0 errors / same 9 pre-existing warnings (Renderer.dll regenerated);
`validate-gamedata.py` passes (21 snapshots, 5 engine families); `pytest scripts/tests`
91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game smoke tests.

## Dead-code removal (2026-09-21): the `R_DrawSpriteModel` locator

Review question: is `gPrivateFuncs.R_DrawSpriteModel` used? No — write-only, same class as the
two sweeps above.

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.R_DrawSpriteModel` | field `privatehook.h:78`; assigned in `Engine_FillAddress_R_DrawSpriteModel`; no reader anywhere (no hook, no `g_phook_*`) | write-only |
| `Engine_FillAddress_R_DrawSpriteModel` | def `gl_hooks.cpp:2880`, dispatch `gl_hooks.cpp:4666` | sole purpose was the field |
| four `R_DRAWSRPITEMODEL_SIG_*` macros | `gl_hooks.cpp:113-116` | used only by the locator |

**Name collision (unlike the two sweeps above).** The plugin has its own
`R_DrawSpriteModel(cl_entity_t*)` — definition `gl_sprite.cpp:798`, declaration `gl_local.h:424`,
call site `gl_rmain.cpp:2243` (its T-entity sprite path) — with exactly the same signature as the
engine field. The two are distinct entities: the plugin's own function is a self-contained
reimplementation that never needed the engine address, and it is untouched.

**Locator shape.** String anchor `"R_DrawSpriteModel:  couldn"` (data, then rdata) →
`68 <str> E8 … 83 C4` → `ReverseSearchFunctionBeginEx(0x300)` accepting three prologues
(`55 8B EC`, `83 EC ?? A1`, `83 EC ?? 50..57`) → four per-family sig fallbacks, ending in a fatal
`Sig_FuncNotFound`. All of that is gone.

**Catalog nuance.** Unlike `R_RotateForEntity` / `R_LightStrength`, this symbol *is* published:
`engine/R_DrawSpriteModel`, kind `function`, Windows + Linux sigs, in every snapshot checked
(hl-8684 `func_rva 0x43d70`, hl-10210 `0x242cb0`, svencoop-10257 `0x532c0`). Publishing is not the
same as needing it — per the `cl_funcs_pDrawTransparentTriangles` precedent, the field was deleted
and **not** added to the `RENDERER_*` gate; if a consumer ever appears, resolution is one
`GamedataResolvePtr` line.

**Verified**: Renderer rebuilds Release|Win32 with 0 errors and the same 9 pre-existing warnings;
`validate-gamedata.py` passes (21 snapshots, 5 engine families); `pytest scripts/tests`
91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game sprite rendering smoke test.

## Dead-code removal (2026-09-21): the `decal_wad` / `Draw_CacheGet` / `Draw_CustomCacheGet` locator

Review question: can the three symbols produced by the second half of `Engine_FillAddress_Draw_DecalTexture`
go? Yes — same write-only class as the sweeps above, with one property the earlier ones lacked (no catalog record).

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.Draw_CustomCacheGet` | field `privatehook.h:69`; guards + assignment inside the locator; no reader, no hook | write-only |
| `gPrivateFuncs.Draw_CacheGet` | field `privatehook.h:70`; same | write-only |
| `decal_wad` | definition `gl_draw.cpp:16`, extern `gl_draw.h:14`; read only by the locator's own early-exit guards, written only by its BFS | write-only |
| `Draw_CustomCacheGet` / `Draw_CacheGet` (plugin-side) | declarations `gl_local.h:589-590` with no definition and no call site | stale declarations |

**Locator shape (deleted).** After the `GamedataResolvePtr("Draw_DecalTexture")` line, a BFS over the engine
function's disassembly (`max_insts 300`, `max_depth 16`, following `jmp`/`jcc` targets): `decal_wad` was taken
from any absolute data-section operand (`push` or `mov reg, moffs`), `Draw_CustomCacheGet` from a `call`
preceded by exactly 4 pushes and followed by `83 C4 10`, `Draw_CacheGet` from a `call` preceded by 2 pushes and
followed by `83 C4 08`. It ended in a fatal `Sig_VarNotFound(decal_wad)` plus two commented-out
`Sig_FuncNotFound`. All of that is gone, along with the two `private_funcs_t` fields, the two stale
declarations and the global.

**What deliberately stayed.** `gPrivateFuncs.Draw_DecalTexture` is genuinely consumed: the plugin's own
`Draw_DecalTexture` wrapper (`gl_draw.cpp:1808`) forwards to it, and that wrapper is called from `R_DrawDecals`
(`gl_rsurf.cpp:1109`). So the locator survives as a 6-line gamedata-only resolver, matching the shape of
`Engine_FillAddress_Cache_Alloc`.

**Catalog.** None of the three has a record (0 matches across all 21 snapshots), unlike `R_DrawSpriteModel`.
Deleting the locator therefore removes the only acquisition path; the pre-change locator is recoverable from
git history if a custom-decal feature is ever built.

**Adjacent finding — the miptex / custom-WAD apparatus is unwired (open item, deliberately not acted on).**
`gfCustomBuild` / `szCustName` (from `Engine_FillAddress_Draw_MiptexTexture`) are read only inside the plugin's
own `Draw_MiptexTexture` (`gl_draw.cpp:1815`), and nothing calls or installs it: `g_phook_Draw_MiptexTexture`
(`gl_hooks.cpp:180`) is declared and never used, `pfnCacheBuild` (`enginedef.h:52`) is never assigned, and
`gPrivateFuncs.Draw_MiptexTexture` has no reader. History: `Install_InlineHook(Draw_MiptexTexture);` was live in
2022 (`ffa35764`), commented out by 2025-04-21 (`5b8f88cd`) and the comment itself deleted 2025-09-28
(`c0da8d21`) during the texture-pipeline replacement (`0fc96e32` "replace texture") — the hook was disabled on
purpose and its locator plus helpers were left behind. Removing the rest is a separate decision, because it
would also drop the plugin's `Draw_MiptexTexture` implementation.

**Verified**: Renderer rebuilds Release|Win32 with 0 errors and the same 9 pre-existing warnings
(gl_light, gl_rsurf, gl_studio, gl_wsurf); `validate-gamedata.py` passes (21 snapshots, 5 engine families);
`pytest scripts/tests` 91 passed / 2 skipped / 26 subtests.
**Not verified**: in-game decal rendering smoke test.

## Dead-code sweep (2026-09-21, second pass): the miptex apparatus, `r_dlightactive`, `R_LightLambert`, `EmitWaterPolys`

Seven symbols were requested and all seven confirmed dead. This pass also closes the open item
recorded in the previous section (the unreachable custom-WAD miptex pipeline).

| Symbol | Every reference before this change | Verdict |
| --- | --- | --- |
| `gPrivateFuncs.Draw_MiptexTexture` | locator only (guard, assignments, `Sig_FuncNotFound`); no reader, no hook | write-only |
| `Draw_MiptexTexture` (plugin, `gl_draw.cpp:1815`) | its own definition only; never called, address never taken, never installed | unreachable |
| `g_phook_Draw_MiptexTexture` (`gl_hooks.cpp:180`) | declared, never used anywhere | dead variable |
| `gfCustomBuild` / `szCustName` | definitions `gl_draw.cpp:16-17`, externs `gl_draw.h:14-15`; read only inside the unreachable `Draw_MiptexTexture` | write-only |
| `gPrivateFuncs.R_LightLambert` | locator only + field `privatehook.h:207` | write-only |
| `gPrivateFuncs.EmitWaterPolys` | locator only + field `privatehook.h:58` | write-only |
| `r_dlightactive` | definition `gl_rsurf.cpp:10`, extern `gl_wsurf.h:308`, walk inside `Engine_FillAddress_CL_AllocDlight` | write-only (see note) |
| `R_DecalShootInternal` | `gl_local.h:551` declaration, no definition, no call site | orphaned declaration |

**Deleted**: three whole locators (`Engine_FillAddress_Draw_MiptexTexture`, `_R_LightLambert`,
`_EmitWaterPolys`) with their dispatch calls, the `EMITWATERPOLYS_SIG_*` (5), `R_LIGHTLAMBERT_SIG_*`
(6) and `DRAW_MIPTEXTEXTURE_SIG_*` (3) macro families, the `DisasmRanges` walk inside
`Engine_FillAddress_CL_AllocDlight`, the plugin-side `Draw_MiptexTexture` implementation, the
`gfCustomBuild` / `szCustName` globals, the three `private_funcs_t` fields, `gl_local.h`'s two
orphaned declarations, `r_dlightactive` (both definition and extern), `g_phook_Draw_MiptexTexture`,
the commented-out `//Uninstall_Hook(Draw_MiptexTexture);`, and a stale
`//xref string "Failed to load custom decal for player"` comment left over from the decal-string
scan removed earlier.

**`r_dlightactive` was never a dlight data source.** `cl_dlights` and `cl_elights` are read by the
live paths (`gl_light.cpp:1786`, `gl_rmain.cpp:5305`, `gl_studio.cpp:2222`, `gl_studio.cpp:3517-3543`);
`r_dlightactive` — the *count* of active dlights — was only ever assigned, from a walk over the
gamedata-resolved `CL_AllocDlight` body (`push 0x28` = the `memset(cl_dlights, 0, 0x28)` size, then
the first `MOV reg,[mem]` within 8 instructions, `OR [mem],1` as fallback). The earlier note that
grouped it with the live readers is corrected above.

**Kept on purpose.** `Engine_FillAddress_CL_AllocDlight` survives as a resolver for
`gPrivateFuncs.CL_AllocDlight` + `cl_dlights` and `Engine_FillAddress_CL_AllocElight` for
`CL_AllocElight` + `cl_elights`; both are now pure two-line `GamedataResolvePtr` bodies. The
`Convert_VA_to_RVA` helper macro stays (used by `r_viewleaf`, `VID_UpdateWindowVars`,
`window_rect`, `transObjects` and more). `gPrivateFuncs.CL_AllocDlight` / `CL_AllocElight` are
themselves still write-only and are the next candidates.

**Verified**: Renderer rebuilds Release|Win32 with 0 errors and the same 9 pre-existing warnings
(gl_light, gl_rsurf, gl_studio, gl_wsurf); `validate-gamedata.py` passes (21 snapshots, 5 engine
families); `pytest scripts/tests` 91 passed / 2 skipped / 26 subtests; a repo-wide grep for all
seven symbols plus the three removed macro families returns nothing; every touched file still has
CRLF endings.
**Not verified**: in-game smoke test (water surfaces, legacy dlight path, decal rendering).

### Follow-up of the same pass: the two `CL_Alloc*` fields

`gPrivateFuncs.CL_AllocDlight` / `CL_AllocElight` turned write-only the moment the `r_dlightactive`
walk was removed above — their only remaining references were the locators themselves. The plugin
never calls them: it goes through `gEngfuncs.pEfxAPI->CL_AllocDlight` (`gl_rmain.cpp:5212`, `:5349`).

Deleted: the two `private_funcs_t` fields, both `Engine_FillAddress_*` locators and their dispatch
calls. `cl_dlights` / `cl_elights` (the live half of those locators) are now two inline
`GamedataResolvePtr` GLOBAL calls at the same point in `Engine_FillAddress`, so the resolution order
is unchanged and the misleadingly named locators are gone.

**Gate follow-up (done on 2026-09-21).** `scripts/validate-gamedata.py` listed `CL_AllocDlight` /
`CL_AllocElight` in `RENDERER_ENGINE_ALL_FUNCTIONS`. `_renderer_check` treats that tuple as
required, so the two entries demanded catalog records nothing consumes — the same "publishing is
not the same as needing" situation as `R_DrawSpriteModel` and
`cl_funcs_pDrawTransparentTriangles`, from the other direction. Both were removed; no test pinned
them, and `validate-gamedata.py` plus the suite stayed green. A re-audit of the tuple against
`Plugins/Renderer` then found no other entry without a consumer (68 functions, 52 globals, 1 patch,
all still referenced), so the gate is consistent again.

**Verified**: Renderer rebuilds Release|Win32 with 0 errors and the same 9 pre-existing warnings
(gl_light, gl_rsurf, gl_studio, gl_wsurf); `validate-gamedata.py` passes (21 snapshots, 5 engine
families); `pytest scripts/tests` 91 passed / 2 skipped / 26 subtests; the only surviving
`CL_AllocDlight` mentions are the two live `gEngfuncs.pEfxAPI` call sites; CRLF endings intact.
**Not verified**: in-game dynamic-light smoke test.

## Gamedata migration (2026-09-21): `gDecalPool` / `gDecalCache`

`Engine_FillAddress_R_DecalInit` still scanned `"\x68\x00\xC0\x01\x00\x6A\x00"` (`push 0x1C000; push 0`) and walked the first `0x50` bytes of `R_DecalInit` for a data-segment `PUSH imm` (`gDecalPool`) and `MOV eax, imm` (`gDecalCache`). Both engine GLOBALs are published on every identity, and both have live readers in `gl_rsurf.cpp` (`EngineGetDecalByIndex` / `R_DecalIndex` / `R_DecalVertsNoclip`). The locator is now two `GamedataResolvePtr` calls; the names join `RENDERER_ENGINE_ALL_GLOBALS`. The engine `R_DecalInit` function itself is still catalog-uncovered and is not a plugin consumer.

## Gamedata migration (2026-09-21): user-fog globals

`Engine_FillAddress_R_RenderFinalFog` still located the five user-fog slots by walking engine code: SvEngine searched `R_RenderView[_SvEngine]` for `CMP [g_bUserFogOn],0` plus `push 0B60h/801h/B65h` (the function is inlined, no catalog FUNCTION record), GoldSrc/HL25 resolved `R_RenderFinalFog` from gamedata then pattern-scanned the render-view body for `g_bUserFogOn`, and a second `DisasmRanges(+0x100)` bound `g_UserFogDensity/Color/Start/End` to the `PUSH` of `GL_FOG_DENSITY`/`_COLOR`/`_START`/`_END`.

All five are published as engine GLOBALs on every identity (`g_bUserFogOn`, `flFinalFogColor`, `flFogDensity`, `flFogStart`, `flFogEnd`) and all five have live readers (`gl_rmain.cpp` `R_RenderUserFog` / fog-enable tests). The locator is now five `GamedataResolvePtr` calls; the catalog names join `RENDERER_ENGINE_ALL_GLOBALS`.

`gPrivateFuncs.R_RenderFinalFog` was write-only (never called, never hooked). After the globals no longer need it as a disasm root it is deleted, and `R_RenderFinalFog` leaves `RENDERER_ENGINE_NON_SVENGINE_FUNCTIONS`. Catalog records `g_bFogSkybox` / `R_FogParams` / `R_RenderFog` have no plugin consumer and stay ungated.

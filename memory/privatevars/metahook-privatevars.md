---
title: metahook-privatevars
type: reference
permalink: metahook-privatevars
tags:
- metahook
- private-vars
- symbol-locating
- reference
---

# Game-private symbols used by `metahook.cpp`

This document inventories the unexported private functions, private data slots, and private call sites that `src/metahook.cpp` consumes from the game engine image. Symbol names use the locally held variables in the code; names in parentheses indicate inferred game-side meanings based on use, rather than official debug-symbol names from the upstream binary.

> **gamedata migration status (2026-09-08, issue #850)**: every private engine symbol consumed by `src/metahook.cpp` is now resolved from the local gamedata catalog. There is no signature, string, cross-reference, or function-body scan left for these symbols, and `MH_DisasmRanges` is no longer used to locate them. Engine version confirmation and engine-family classification are also gamedata-only (`MH_GetModuleCRC64` → `GameData::GetGameVersion` → gameVersion prefix/build-number rule); `build_number` is still resolved for existing consumers but no longer participates in version or family decisions.
>
> Entry points: `MH_LoadEngine_ResolveSymbol` (required/optional resolve + shared failure diagnostic), `MH_IsGameSymbolAvailable` (existence probe), `MH_LoadEngine_DetermineEngineType`, `MH_LoadEngine_FindCvarDirectSet`, `MH_LoadEngine_PatchCvarCallbacks`, `MH_LoadEngine_FindLoadBlobClient`. Catalog initialization, module-source registration, and mirror-alias registration happen in `MH_LoadEngine` (`GameData::Initialize` / `RegisterModuleFileSource` / `RegisterMirrorAlias`).

## Scope and shared resolution process

- The scope includes game-internal objects whose addresses are stored in global/local variables or used as comparison, hook, or patch targets.
- It excludes MetaHook's own symbols such as `MH_*`, local state such as `g_ManagedCvarCallbackList`, and APIs obtained from the public `cl_enginefunc_t` table (for example, `Cvar_Set` and `GetFirstCmdFunctionHandle`).
- All resolution uses the real engine base `g_dwEngineBase` (the blob image base for blob engines). `ResolveGameSymbol` already returns the real VA, so there is no mirror/real conversion and no `SearchDllInfo` parameter. `MH_LoadEngine_ResolveGlobalOperand` is the one place that still derives an instruction operand, using the record's `signatureRva + instructionOffset + operandOffset`.
- A required-symbol failure aborts loading through `MH_SysError`; every required item below therefore has a runtime gate. Failures share one diagnostic (`MH_LoadEngine_ReportSymbolFailure`): symbol name, module path when available, CRC64 when available, and `MH_GetGameSymbolStatusString`. The variables are cleared by `MH_ResetAllVars`.

## Private functions

| Local symbol / inferred game symbol | Kind | Resolution | Subsequent use |
| --- | --- | --- | --- |
| `g_pfnbuild_number` (`buildnumber`) | function | `ResolveGameSymbol("build_number", FUNCTION)` | Forwarded through `MH_GetEngineVersion`. No longer used to identify the engine version or family. |
| `g_pfnSys_Error` (`Sys_Error`) | function | `ResolveGameSymbol("Sys_Error", FUNCTION)` | `MH_SysError` forwards the formatted fatal error to the engine. |
| `ClientDLL_Init` (in-engine client DLL initialization routine) | `cl_funcs` global metadata | `cl_funcs.signatureRva` plus `instructionOffset + operandOffset` points to the absolute operand of `call [cl_funcs.pInitFunc]`. | `MH_LoadEngine_ResolveGlobalOperand` obtains the call operand and verifies it still refers to the resolved `cl_funcs`; it does not alter the direct-global contract of `ResolveGameSymbol`. |
| `g_pfnClientDLL_HudInit` (in-engine HUD initialization routine) | function | `ResolveGameSymbol("ClientDLL_HudInit", FUNCTION)` | The proxy begins a hook transaction before calling the original and commits it afterward; `MH_InlineHook` installs the inline hook. The transaction covers `HUD_Init`, Studio interface initialization, and the subsequent `cl_righthand` query. |
| `g_pfnCvar_DirectSet` (`Cvar_DirectSet`) | function | `ResolveGameSymbol("Cvar_DirectSet", FUNCTION)`; required for every engine family. | Called by `MH_Cvar_DirectSet`; on the managed cvar path, `Cvar_Set` call sites are redirected to it. |
| `g_pfnNLoadBlob` (`NLoadBlob`) | function | Required for GoldSrc / Blob / HL25 / CoF. For SvEngine it may be absent, but only together with `FreeBlob`. | `MH_InlineHook` installs `MH_NLoadBlob`. |
| `g_pfnFreeBlob` (`FreeBlob`) | function | Same requiredness rule as `NLoadBlob` (pairwise on SvEngine). | `MH_InlineHook` installs `MH_FreeBlobProxy`. |

## Private variables and internal tables

| Local symbol / inferred game object | Kind | Resolution | Subsequent use |
| --- | --- | --- | --- |
| `g_pEngineFuncs` (the formal game-side object `cl_enginefuncs`) | global | `ResolveGameSymbol("cl_enginefuncs", GLOBAL)` returns the address of the embedded `cl_enginefunc_t` object, without a second dereference. | Copied to `gMetaSave.pEngineFuncs` and passed directly to `LoadEngine` of V3/V4 plugins. |
| `g_pExportFuncs` (the formal game-side object `cl_funcs`) | global | `ResolveGameSymbol("cl_funcs", GLOBAL)` returns the address of the embedded client export table; its ABI layout corresponds to `cl_exportfuncs_t`. | `ClientDLL_Initialize` copies it after the engine fills the table, passes it to plugins, and calls the original `Initialize`; initialization takeover writes to a separately calculated call operand, not the start address of the `cl_funcs` global. |
| `g_pClientDLLInitializeOperand` (the absolute operand of `call [cl_funcs.pInitFunc]`) | derived | `MH_LoadEngine_ResolveGlobalOperand` calculates it as `moduleBase + signatureRva + instructionOffset + operandOffset` and verifies that the operand currently points to direct-global `cl_funcs`. | `MH_WriteDWORD` redirects this operand to MetaHook's wrapper slot; this preserves the original ordering in which the client table is filled and immediately called. |
| `g_phClientModule` (`HMODULE *`, the game's client DLL module-handle slot) | global | `ResolveGameSymbol("g_phClientModule", GLOBAL)` | Controls client mirror loading; `MH_GetClientModule` exposes the module handle. |
| `g_pClientFactory` (`CreateInterfaceFn *` slot) | global | `ResolveGameSymbol("g_pClientFactory", GLOBAL)` | Dereferenced and returned by the client factory getter. |
| `videomode` (video-mode object pointer slot) | global | `ResolveGameSymbol("videomode", GLOBAL)` | Dereferenced as `IVideoMode` / `IVideoMode_HL25` to retrieve, return, and save the video mode. |
| `gClientUserMsgs` (`usermsg_t **`, user-message linked-list head slot) | global | `ResolveGameSymbol("gClientUserMsgs", GLOBAL)` | Dereferenced to walk/hook user messages. |
| `cl_parsefuncs` (`svc_func_t *`, SVC parse-function table base) | global | `ResolveGameSymbol("cl_parsefuncs", GLOBAL)` | Returned as the table base and queried or replaced by opcode/name. |
| `cvar_hooks` (`cvar_callback_entry_t **`, cvar callback linked-list head slot) | global | Native path: `ResolveGameSymbol("cvar_hooks", GLOBAL)` when the catalog has it. Managed path: MetaHook's own `g_ManagedCvarCallbackList`. | Walked/inserted by the cvar callback hooks; cleared on shutdown. The managed branch is not a game variable. |

## cvar callback branch: native list or managed call-site redirect

`MH_LoadEngine_PatchCvarCallbacks` tries the native list first:

- `IsGameSymbolAvailable("cvar_hooks") == OK`: resolve it as GLOBAL and use it as the engine's `cvar_callback_entry_t**` head. No `Cvar_Set` branch redirect is installed.
- `SYMBOL_NOT_FOUND`: managed path. Starting at index 0, build `Cvar_Set_to_Cvar_DirectSet_callsite_<N>`, probe with `IsGameSymbolAvailable`, and stop at the first absent index. Index 0 absent is a hard failure; any status other than `OK`/`SYMBOL_NOT_FOUND` aborts with that symbol's status. Each present call site is resolved as `PATCH` and redirected with `MH_InlinePatchRedirectBranch` (E8 and E9 forms are preserved). After the loop, `cvar_hooks = &g_ManagedCvarCallbackList`.
- Any other status from the `cvar_hooks` probe aborts with the shared symbol diagnostic.

## Engine version and family determination

`MH_LoadEngine_DetermineEngineType` runs before `build_number` is resolved:

1. `MH_GetModuleCRC64(g_dwEngineBase)` obtains the module identity (file-backed for ordinary PE and for blob engines registered with `RegisterModuleFileSource`).
2. `GameData::GetGameVersion(crc64, &gameVersion)` looks up the catalog entry.
3. `MH_LoadEngine_EngineTypeFromGameVersion` maps the prefix and build number: `svencoop-` → `ENGINE_SVENGINE`; `cof-` → `ENGINE_GOLDSRC_COF`; `hl-<N>` with `N > 9000` → `ENGINE_GOLDSRC_HL25`; other `hl-*` → `ENGINE_GOLDSRC`. Unrecognised prefixes (`cstrike-*`, `czero-*`, `czeror-*`, …) and unparsable build numbers are rejected.
4. CRC failure, a missing catalog entry, or an unrecognised version aborts with `Unable to determine engine version`. Blob engines keep the `ENGINE_GOLDSRC_BLOB` value set by `MH_LoadEngine` and are not reclassified by the table.

`MH_GetEngineTypeName` exposes the matching `engineTypeNames` entry, including `GoldSrc_CoF`.

## Objects derived from private slots but not independently scanned

- `g_pExportFuncs` directly points to `cl_funcs`; it is not produced by a second dereference from a machine-instruction operand. After the engine populates client exports, `ClientDLL_Initialize` copies it, passes it to plugins, and calls the original `Initialize`.
- `gMetaSave.pEngineFuncs` is copied from direct-global `cl_enginefuncs` and then used as the public `cl_enginefunc_t` table. Therefore, `Cmd_GetCmdBase` is still not an independently resolved private function.

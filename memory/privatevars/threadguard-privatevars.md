---
title: threadguard-privatevars
type: reference
permalink: metahooksv/privatevars/threadguard-privatevars
tags:
- threadguard
- private-vars
- symbol-locating
- reference
---

# Game-private symbols used by `ThreadGuard`

This document inventories the unexported game symbol that `Plugins/ThreadGuard` locates in the engine image and consumes: the engine module's global `IEngine*` slot (`engine`). The plugin resolves **no** game-private function, patches no engine instruction, and installs no engine-side hook. Like [[scmodeldownloader-privatevars]], it is not gamedata-based.

## Scope and shared resolution process
- The scope covers the single game-private global slot `engine` (`IEngine**`) located by `Engine_FillAddress` (`Plugins/ThreadGuard/privatehook.cpp`).
- Out of scope: the `kernel32` `CreateThread` / `WaitForSingleObject` / `Sleep` IAT hooks and the `FreeLibrary` IAT hooks (`Plugins/ThreadGuard/ThreadManager.cpp` and `privatehook.cpp`), the `_restart` command hook (`EngineCommand_InstallHook` via the public `FindCmd` / `HookCmd`), and the export-table work - none of these touch an engine-private address. `IEngine` itself is the public engine interface declared in `include/Interface/IEngine.h`.
- Public MetaHook APIs, saved engine interfaces, and ordinary plugin state (for example `g_EngineDLLInfo`, `g_MirrorEngineDLLInfo`, `g_iEngineType`, `g_dwEngineBuildnum`) are excluded.
- **No gamedata**: there is no `ResolveGameSymbol` / `IsGameSymbolAvailable` call, no gamedata symbol diagnostic, and no `METAHOOK_API_VERSION` `static_assert` in this plugin. `scripts/validate-gamedata.py` gates no ThreadGuard symbol.
- Scan space: `IPluginsV4::LoadEngine` calls `Engine_FillAddress(g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo)`, so the string/pattern searches run in the mirror engine when present and the located operand is converted `DllInfo` -> `RealDllInfo` through the plugin-local `ConvertDllInfoSpace`.
- Failure is fatal but late: a missed string anchor leaves `engine` null silently, and only the trailing `if (!engine) Sys_Error("CEngine not found")` reports it. There is no per-stage diagnostic and no alternative locator.

## Private global variables

| Local symbol / inferred game object | Declaration location | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `engine` (address of the engine module's global `IEngine*` slot, i.e. `IEngine**` as stored; the `IEngine` vtable is the public one from `include/Interface/IEngine.h`) | `Plugins/ThreadGuard/privatehook.cpp` (file-scope `IEngine** engine = NULL;`) | `Engine_FillAddress`: string anchor `"Sys_InitArgv( OrigCmd )"` -> `push <str>; call` pattern -> bounded `DisasmRanges` extraction of the first `.data` `MOV reg, [disp32]`; the operand is converted to `RealDllInfo` space. Fatal on failure. | `GetEngineDLLState()` dereferences it once (`(*engine)->GetState()`) and compares the result against `DLL_CLOSE` / `DLL_RESTART`; consumed by `Engine_WaitForShutdown`, `ServerDLL_WaitForShutdown`, and `ServerBrowser_WaitForShutdown`. |

## Resolution chain

1. **String anchor.** `Search_Pattern_Data("Sys_InitArgv( OrigCmd )", DllInfo)` scans `.data` (23 bytes, terminator excluded); on miss it retries `Search_Pattern_Rdata` over `.rdata`. If both miss, the whole block is skipped and only the final `Sys_Error` fires.
2. **Push/call pattern.** A 20-byte pattern `68 imm32 E8 rel32 68 imm32 E8 rel32` has the string address written into both `push` immediates, so both pushes must reference the same string; `Search_Pattern(pattern, DllInfo)` scans `.text` for it.
3. **Build-4554 fallback.** If the 20-byte pattern misses, a 5-byte `68 imm32` (`push <str>`) pattern is searched instead; the source comment records why (in build 4554 the bytes after the push are `and ecx, 3; rep movsb`, not a call). Added by commit `d7398591`.
4. **Operand extraction.** `DisasmRanges(Sys_InitArgv_PushString_VA, 0x50, cb, 0, ctx)` starts at the push and scans forward at most 0x50 bytes; the callback accepts the first `MOV reg, [disp32]` with `mem.base == 0` whose `disp` lies inside `DllInfo.DataBase..+DataSize`, converts it to `RealDllInfo` space and stores it in `engine`, then returns TRUE. The window scan also stops at `0xCC` or `RET`. No branch is followed.
5. **Validation.** `if (!engine) Sys_Error("CEngine not found")`.

## Architecture

```mermaid
flowchart TD
    A["IPluginsV4::LoadEngine"] --> B["Engine_FillAddress(DllInfo, RealDllInfo)"]
    B --> C["Search_Pattern_Data/Rdata: \"Sys_InitArgv( OrigCmd )\""]
    C --> D["push str; call pattern (20B) -> .text"]
    D --> E["fallback: push str only (5B, build 4554)"]
    E --> F["DisasmRanges(push, 0x50): first MOV reg,[.data disp32]"]
    F --> G["ConvertDllInfoSpace -> engine (IEngine**)"]
    G --> H["GetEngineDLLState: (*engine)->GetState()"]
    H --> I["DLL_CLOSE / DLL_RESTART gates thread shutdown wait"]
    J["ExitGame / NewFreeLibrary_Engine / NewFreeLibrary_GameUI"] --> K["*_WaitForShutdown"]
    K --> H
```

## Plugin-owned state

| Local state | Source and meaning | Use |
| --- | --- | --- |
| `gPrivateFuncs` (`private_funcs_t`, single `empty` field) | `Plugins/ThreadGuard/privatehook.cpp`, zero-initialized; never assigned anywhere. | Dead scaffolding - this plugin resolves no private function. |
| `g_ThreadManager_Engine` / `_GameUI` / `_ServerBrowser` / `_ServerDLL` | One `CThreadManager` per tracked module, created in `Engine_InstallHook` / `GameUI_InstallHook` / `ServerBrowser_InstallHook` / `ServerDLL_InstallHook`. | Owns the kernel32 IAT hooks and the tracked thread handles; not engine-private addresses. |

## Dependencies
- `Plugins/ThreadGuard/plugins.cpp` - builds `g_EngineDLLInfo` / `g_MirrorEngineDLLInfo`, calls `Engine_FillAddress`, registers `DllLoadNotification`, and calls `Engine_WaitForShutdown` from `ExitGame`.
- `Plugins/ThreadGuard/privatehook.cpp` - performs the location and consumes `engine` in `GetEngineDLLState`.
- MetaHook APIs `SearchPattern`, `DisasmRanges`, `GetSectionByName`, `GetEngineBase` / `GetEngineSize`, `GetMirrorEngineBase` / `GetMirrorEngineSize`, `GetEngineType`, `GetEngineBuildnum`, `SysError`, `IATHook` / `BlobIATHook` / `UnHook`, `RegisterLoadDllNotificationCallback`, `GetGameDirectory`, `GetEngineModule` / `GetBlobEngineModule`, `ModuleHasImport` / `ModuleHasImportEx`, `FindCmd` / `HookCmd`.
- Capstone `cs_insn` fields (`id`, `detail->x86.op_count`, `operands[].type`, `operands[].mem.{base,disp}`) inside the walk callback.
- Public `include/Interface/IEngine.h` - the `GetState` vtable slot (index 3, after `Load` / `Unload` / `SetState`) the plugin calls.

## Notes
- The located value is the **address of the global `IEngine*` slot**, not the `IEngine*` itself; that is why the plugin stores `IEngine**` and dereferences exactly once in `GetEngineDLLState`. The code guards `engine != NULL` but assumes `*engine` is non-null.
- The in-source comment on the extraction branch (`A1 40 77 7B 02 mov eax, gl_backbuffer_fbo`) is a stale copy-paste: the accepted operand is the engine pointer slot, not `gl_backbuffer_fbo`.
- The 0x50-byte window is a flat forward scan with no branch following, and it accepts the first `.data` global read after the anchor. The link to `engine` is therefore by engine layout, not by a verified reference - a reordering of the code after the anchor would silently pick a different global.
- The chain is build-sensitive: the string anchor, the 20-byte push/call pattern, and the 4554 push-only fallback are all layout-dependent. A change in any of them makes the location fail and the plugin aborts with `CEngine not found` (this is exactly what commit `d7398591` fixed for build 4554).
- `Engine_InstallHook` / `Engine_UninstallHook` are declared without parameters in `Plugins/ThreadGuard/privatehook.h` but defined with `(HMODULE, BlobHandle_t)` in `privatehook.cpp`; the no-arg declarations are unused. The actual installation happens in `DllLoadNotification` -> `Engine_InstallHook(ctx->hModule, ctx->hBlob)`, which only installs kernel32 IAT hooks (plus the `FreeLibrary` IAT hook under the `svencoop` directory) - no engine instruction is patched.
- `GetVFunctionFromVFTable` (`Plugins/ThreadGuard/privatehook.cpp`) is declared and defined but has no caller - dead helper. `ConvertDllInfoSpace` is used only for the single conversion above.

## Callers
- `IPluginsV4::LoadEngine` (`Plugins/ThreadGuard/plugins.cpp`) calls `Engine_FillAddress`.
- `DllLoadNotification` (registered in `LoadEngine`) drives `Engine_InstallHook` / `Engine_UninstallHook` on engine and target-DLL load/unload.
- `IPluginsV4::ExitGame`, `NewFreeLibrary_Engine`, and `NewFreeLibrary_GameUI` reach `GetEngineDLLState()`, the only consumer of the located `engine` slot.

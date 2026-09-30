---
title: threadguard-privatevars
type: reference
permalink: metahooksv/privatevars/threadguard-privatevars
tags:
- threadguard
- private-vars
- gamedata
- symbol-locating
- reference
---

# Game-private symbols used by `ThreadGuard`

This document inventories the unexported game symbol that `Plugins/ThreadGuard` locates in the engine image and consumes: the engine module's global `IEngine*` slot (`eng`). The plugin resolves **no** game-private function, patches no engine instruction, and installs no engine-side hook. Since issue #855 (2026-09-09) the location is **gamedata-only**; the former `"Sys_InitArgv( OrigCmd )"` string anchor, push/call pattern, build-4554 fallback and bounded disassembly scan are deleted.

> **Rename (2026-09-30):** the catalog symbol is **`eng`**, not `engine` — upstream renamed it in the `2026-09-30T06:25:26Z` release and confirmed it is the global's real name. The plugin file-scope variable and the `scripts/validate-gamedata.py` `COMMON_REQUIRED` entry were renamed to match, so the plugin alias is again identical to the catalog name. All `engine` spellings below have been updated in place.

## Scope and shared resolution process
- The scope covers the single game-private global slot `eng` (`IEngine**`) located by `Engine_FillAddress` (`Plugins/ThreadGuard/privatehook.cpp`).
- Out of scope: the `kernel32` `CreateThread` / `WaitForSingleObject` / `Sleep` IAT hooks and the `FreeLibrary` IAT hooks (`Plugins/ThreadGuard/ThreadManager.cpp` and `privatehook.cpp`), the `_restart` command hook (`EngineCommand_InstallHook` via the public `FindCmd` / `HookCmd`), and the export-table work - none of these touch an engine-private address. `IEngine` itself is the public engine interface declared in `include/Interface/IEngine.h`.
- Public MetaHook APIs, saved engine interfaces, and ordinary plugin state (for example `g_EngineDLLInfo`, `g_iEngineType`, `g_dwEngineBuildnum`) are excluded.
- **gamedata-only**: `Engine_FillAddress(void)` calls the shared `GamedataResolvePtr(g_EngineDLLInfo.ImageBase, "eng", MH_GAMESYMBOL_KIND_GLOBAL)` helper (`plugins.h`, itself a thin `ResolveGameSymbol` wrapper) against the **real** engine module base (`g_EngineDLLInfo.ImageBase` = `GetEngineBase()`), so the mirror image and the plugin-local `ConvertDllInfoSpace` are no longer involved. `privatehook.cpp` carries `static_assert(METAHOOK_API_VERSION >= 109, ...)`; `scripts/validate-gamedata.py` gates `eng` as a common required global.
- Failure is fatal and specific: a non-`MH_GAMESYMBOL_OK` status prints `Could not resolve gamedata symbol: eng` with buildnum and `GetGameSymbolStatusString`, then aborts via `Sys_Error`. The old silent-null + trailing `Sys_Error("CEngine not found")` path is gone.

## Private global variables

| Local symbol / inferred game symbol | Declaration location | Resolution mechanism | Subsequent use |
| --- | --- | --- | --- |
| `eng` (address of the engine module's global `IEngine*` slot, i.e. `IEngine**` as stored; the `IEngine` vtable is the public one from `include/Interface/IEngine.h`) | `Plugins/ThreadGuard/privatehook.cpp` (file-scope `IEngine** eng = NULL;`) | `Engine_FillAddress`: `GamedataResolvePtr(..., "eng", MH_GAMESYMBOL_KIND_GLOBAL)`; the gamedata `gv_va` is the slot address itself (the recorded anchor instruction is the store into that slot), so no dereference is applied. | `GetEngineDLLState()` dereferences it once (`(*eng)->GetState()`) and compares the result against `DLL_CLOSE` / `DLL_RESTART`; consumed by `Engine_WaitForShutdown`, `ServerDLL_WaitForShutdown`, and `ServerBrowser_WaitForShutdown`. |

## Resolution chain

1. **Resolve.** `Engine_FillAddress` (called from `IPluginsV4::LoadEngine` after `g_EngineDLLInfo.ImageBase` is set) resolves the GLOBAL `eng` with expected kind `MH_GAMESYMBOL_KIND_GLOBAL`.
2. **Diagnose.** On failure, report plugin-scoped `Sys_Error` with symbol name and `g_dwEngineBuildnum`; then return (the process aborts).
3. **Store.** `eng = (decltype(eng))address;` - the value is the **slot address**, not the `IEngine*` object.
4. **Consume.** `GetEngineDLLState()` performs the single dereference `(*eng)->GetState()`.

## Architecture

```mermaid
flowchart TD
    A["IPluginsV4::LoadEngine"] --> B["Engine_FillAddress()"]
    B --> C["GamedataResolvePtr(real base, 'eng', GLOBAL)"]
    C -->|OK| D["eng = slot address (IEngine**)"]
    C -->|fail| E["Sys_Error: symbol/buildnum/status"]
    D --> F["GetEngineDLLState: (*eng)->GetState()"]
    F --> G["DLL_CLOSE / DLL_RESTART gates thread shutdown wait"]
    H["ExitGame / NewFreeLibrary_Engine / NewFreeLibrary_GameUI"] --> I["*_WaitForShutdown"]
    I --> F
```

## Plugin-owned state

| Local state | Source and meaning | Use |
| --- | --- | --- |
| `g_ThreadManager_Engine` / `_GameUI` / `_ServerBrowser` / `_ServerDLL` | One `CThreadManager` per tracked module, created in `Engine_InstallHook` / `GameUI_InstallHook` / `ServerBrowser_InstallHook` / `ServerDLL_InstallHook`. | Owns the kernel32 IAT hooks and the tracked thread handles; not engine-private addresses. |

## Dependencies
- `Plugins/ThreadGuard/plugins.cpp` - sets `g_EngineDLLInfo.ImageBase` (only that field is populated now), calls `Engine_FillAddress()`, registers `DllLoadNotification`, and calls `Engine_WaitForShutdown` from `ExitGame`.
- `Plugins/ThreadGuard/privatehook.cpp` - performs the resolution and consumes `eng` in `GetEngineDLLState`.
- MetaHook APIs `ResolveGameSymbol` / `GetModuleCRC64` / `GetGameSymbolStatusString` (gamedata), `IATHook` / `BlobIATHook` / `UnHook`, `RegisterLoadDllNotificationCallback`, `GetGameDirectory`, `GetEngineModule` / `GetBlobEngineModule`, `ModuleHasImport` / `ModuleHasImportEx`, `FindCmd` / `HookCmd`.
- Public `include/Interface/IEngine.h` - the `GetState` vtable slot (index 3, after `Load` / `Unload` / `SetState`) the plugin calls.

## Notes
- The located value is the **address of the global `IEngine*` slot**, not the `IEngine*` itself; that is why the plugin stores `IEngine**` and dereferences exactly once in `GetEngineDLLState`. The code guards `engine != NULL` but assumes `*engine` is non-null.
- `Engine_FillAddress` signature changed from `(const mh_dll_info_t&, const mh_dll_info_t&)` to `(void)`; `g_MirrorEngineDLLInfo`, the `.text`/`.data`/`.rdata` section setup and `GetSectionByName` usage were removed from `plugins.cpp`, and `plugins.h` no longer declares `g_MirrorEngineDLLInfo`.
- Deleted by #855: `private_funcs_t` / `gPrivateFuncs` dead scaffolding, `ConvertDllInfoSpace`, `GetVFunctionFromVFTable`, the `Sys_InitArgv_SearchContext` + 0x50-byte `DisasmRanges` window, the `MOV reg, [disp32]` operand filter, the build-4554 `push`-only fallback, the `#include <capstone.h>` dependency, and the misleading `gl_backbuffer_fbo` comment. The no-argument `Engine_InstallHook()` / `Engine_UninstallHook()` declarations were removed from `privatehook.h` (the parameterized definitions used by `DllLoadNotification` remain).
- Thread management behaviour is unchanged: `DLL_CLOSE` / `DLL_RESTART` gating, exit/restart waits, `FreeLibrary` interception, DLL load notifications and IAT / BlobIAT hooks all keep their previous semantics.
- Verified 2026-09-09: `ThreadGuard.dll` (Release | Win32) contains no `Sys_InitArgv( OrigCmd )` / `CEngine not found` / `gl_backbuffer` strings; `eng` (then spelled `engine`) is the only gamedata symbol it resolves.

## Callers
- `IPluginsV4::LoadEngine` (`Plugins/ThreadGuard/plugins.cpp`) calls `Engine_FillAddress()`.
- `DllLoadNotification` (registered in `LoadEngine`) drives `Engine_InstallHook` / `Engine_UninstallHook` on engine and target-DLL load/unload.
- `IPluginsV4::ExitGame`, `NewFreeLibrary_Engine`, and `NewFreeLibrary_GameUI` reach `GetEngineDLLState()`, the only consumer of the located `eng` slot.

Related: [[game-data]] [[thread-guard]] [[private-symbols-disasm-workflow]]

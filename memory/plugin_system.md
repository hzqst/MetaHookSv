---
title: plugin_system
type: note
permalink: metahooksv/plugin-system
---

# MetaHookSv: Plugin system and development workflow

## Key directories
- Plugin directory: `Plugins/XXXXX`, for example `Plugins/Renderer`
- Plugin documentation: `docs/XXXXX.md`, for example `docs/Renderer.md`, `docs/RendererCN.md`

### Core plugins (examples)
- `VGUI2Extension`: a VGUI2 modding framework and the foundation for many UI-related plugins
- `Renderer`: graphics-enhanced rendering engine
- `BulletPhysics`: Bullet Physics-based client-side simulation supporting physical effects such as ragdolls.
- `CaptionMod`: subtitles/translation/HiDPI support

### Utility plugins (examples)
- `HeapPatch`: addresses GoldSrc engine fixed-heap resources being too small
- `ResourceReplacer`: runtime asset replacement
- `ThreadGuard`: thread management and cleanup
- `PrecacheManager`: asset precache management
- `SteamScreenshots`: Steam screenshot integration (Sven Co-op only)

## PluginLibs (`PluginLibs/`)
- `UtilHTTPClient_SteamAPI`: Steam API-based HTTP client
- `UtilHTTPClient_libcurl`: libcurl-based HTTP client
- `UtilAssetsIntegrity`: asset-integrity validation
- `UtilThreadTask`: thread utilities

## Common workflow for adding a plugin
1. Create a new project in the VS solution and place it under Plugins/XXXXXX.
2. Reference required `PluginLibs` dependencies (optional).
3. Complete the plugin implementation (other plugins can serve as references).
4. Add the plugin to build script `scripts/build-Plugins.bat`.
5. Update the plugin load list in configuration, `plugins.lst`.

## Common development patterns/conventions
- In `plugins.cpp!IPluginsV4::LoadClient`, replace `pExportFunc->` function pointers to take over exports from `client.dll`, for example: `pExportFunc->HUD_Init = HUD_Init;`. This requires implementing our own `HUD_Init` to take over the client's `HUD_Init`.
- Use the MetaHook API to interact with the engine/install inline hooks or vtable hooks.
- Follow the structures and naming of existing plugins.

## Testing and debugging
- Use `scripts\debug-*.bat` to set up the debugging environment.
- Open `MetaHook.sln`, set the target plugin as the startup project, and debug with F5.

## Safety and boundaries
- This project is for legitimate game enhancement/modding (positioned as a defensive security tool).
- Do not introduce malicious code or vulnerability-exploitation logic.

## Compatibility
- Some features are unavailable on different engine types; determine the engine type before performing engine-specific operations.

## Plugin load order
- The load order in `plugins.lst` is critical to dependencies and hook installation.

## Engine compatibility

MetaHookSv supports multiple GoldSrc / SvEngine variants. Common types and version ranges are:
- `GoldSrc_blob` (buildnum 3248 ~ 4554): legacy encrypted format
- `GoldSrc_legacy` (< 6153)
- `GoldSrc_new` (8684+): Half-Life Pre-25th Anniversary
- `SvEngine` (8832+): Sven Co-op modified engine
- `GoldSrc_HL25` (>= 9884): Half-Life 25th Anniversary Update

Notes:
- Before implementing engine-specific logic, first use `g_pMetaHookAPI->GetEngineType()` to determine the engine type.
- Legacy blob engines (non-standard PE files loaded directly into memory in binary form) require blob-specific APIs.

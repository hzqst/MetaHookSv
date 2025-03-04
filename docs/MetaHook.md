# ABI Compatibility

The `g_pMetaHookAPI` interface exported by MetaHookSv (V4) is fully compatible with plugins from the MetaHook (V2) era at the ABI level, so there are no compatibility issues when loading plugins from the MetaHook (V2) period. The only compatibility consideration is between different plugins.

# New Features of MetaHookSv (V4) Compared to MetaHook (V2)
### Automatic Engine Type Detection

Automatically identifies four types of engines: SvEngine, GoldSrc_HL25, GoldSrc, and GoldSrc_Blob.
- SvEngine: A modified GoldSrc branch created by the Sven Co-op team.
- GoldSrc_HL25: The latest GoldSrc after the "Half-Life 25th Anniversary Update," with an engine build number greater than or equal to 9884.
- GoldSrc_Blob: An engine with a build number less than 4554, using a non-standard PE format for encryption.
- GoldSrc: GoldSrc with a build number between 4554 and 8684.

You can use `g_pMetaHookAPI->GetEngineType` or `g_pMetaHookAPI->GetEngineTypeName` to obtain the corresponding engine type.

### API: Reverse Search Function Header

You can use `g_pMetaHookAPI->ReverseSearchFunctionBegin` or `g_pMetaHookAPI->ReverseSearchFunctionBeginEx` to search backward for a function header from a specific address.
- `g_pMetaHookAPI->ReverseSearchFunctionBegin` will traverse the entire function starting from the current byte using a disassembly engine when it encounters a previous byte of CC, 90, or C3. If it does not return to the previous byte's position during traversal, the current byte will be determined as the function header. This function may fail to retrieve the function header if it encounters an abnormal function ending like E9 (JMP) without padding bytes, where the entire function body is directly connected to the next function body. In such cases, `g_pMetaHookAPI->ReverseSearchFunctionBeginEx` should be used.
- `g_pMetaHookAPI->ReverseSearchFunctionBeginEx` requires a callback that returns TRUE for a specific pattern to stop the search and confirm the function header.

### API: Disassembly Engine

The disassembly engine uses Capstone 4.0.2.
- `g_pMetaHookAPI->DisasmSingleInstruction`: Disassembles a single instruction.
- `g_pMetaHookAPI->DisasmRanges`: Disassembles instructions within a specific range.

### API: Reverse Search Pattern

`g_pMetaHookAPI->ReverseSearchPattern` functions similarly to `g_pMetaHookAPI->SearchPattern`, with the only difference being that it searches backward from pStartSearch instead of downward.

### API: Client Module Operations (client.dll)

- `g_pMetaHookAPI->GetClientModule`: Retrieves the client module. Only non-blob loaded client.dll can obtain an HMODULE; it will return NULL for blob-loaded client.dll.
- `g_pMetaHookAPI->GetClientBase`: Retrieves the client base address. Supports both non-blob and blob loaded client.dll.
- `g_pMetaHookAPI->GetClientSize`: Retrieves the size of the client module. Supports both non-blob and blob loaded client.dll.
- `g_pMetaHookAPI->GetClientFactory`: Retrieves the client interface factory (i.e., `CreateInterface` in `interface.cpp`). Supports both non-blob and blob loaded client.dll.

### API: Request Information from Other Plugins

- `g_pMetaHookAPI->QueryPluginInfo`: Retrieves information about all loaded plugins. The retrieval method is:

```cpp
mh_plugininfo_t info;
if(g_pMetaHookAPI->GetPluginInfo("PluginName.dll", &info)) // "PluginName.dll" is case-insensitive
{
}
```

- `g_pMetaHookAPI->GetPluginInfo`: Queries a specific plugin by name, ignoring suffixes like _SSE or _AVX.

```cpp
mh_plugininfo_t info;
if(g_pMetaHookAPI->GetPluginInfo("PluginName.dll", &info)) // "PluginName.dll" is case-insensitive
{
}
```

### API: HookUser Msg

- `g_pMetaHookAPI->HookUser Msg`: Similar to the HookUser Msg implementation in some previous plugins, now exported by g_pMetaHookAPI.

### API: HookCvarCallback, RegisterCvarCallback

Cvar callbacks are a feature added by Valve in build number 6153 of the GoldSrc engine, used to execute specific callbacks when a cvar is modified. Valve only uses this feature on `gl_texturemode`.
- `g_pMetaHookAPI->HookCvarCallback`: Provides the ability to hook a modification callback for a specific cvar.
- `g_pMetaHookAPI->RegisterCvarCallback`: Provides the ability to register a modification callback for a specific cvar. If HookCvarCallback fails, you can register it yourself (if no one has registered a callback for that cvar, the hook will fail).

### API: HookCmd

- `g_pMetaHookAPI->HookCmd`: Similar to the HookCmd implementation in some previous plugins, now exported by g_pMetaHookAPI.

### API: SysError

- `g_pMetaHookAPI->SysError`: Similar to SysErrorEx in some previous plugins (popup error and exit the game), now exported by g_pMetaHookAPI.

### API: IsDebuggerPresent

- `g_pMetaHookAPI->IsDebuggerPresent`: Checks if a debugger is present.

### API: Blob Module Operations

- `g_pMetaHookAPI->GetBlobEngineModule`: Retrieves the module handle for the Blob format engine. This module handle cannot be mixed with HMODULE and can only be used to operate on Blob modules.
- `g_pMetaHookAPI->GetBlobClientModule`: Retrieves the module handle for the Blob format client.dll. This module handle cannot be mixed with HMODULE and can only be used to operate on Blob modules.
- `g_pMetaHookAPI->GetBlobModuleImageBase`: Gets the base address of the module from the Blob module handle.
- `g_pMetaHookAPI->GetBlobModuleImageSize`: Gets the size of the module from the Blob module handle.
- `g_pMetaHookAPI->GetBlobSectionByName`: Searches for a specific section in the Blob module corresponding to the Blob module handle. Only supports ".text\0\0\0" and ".data\0\0\0" (as Blob format modules generally only have these two sections holding valid information).
- `g_pMetaHookAPI->BlobLoaderFindBlobByImageBase`: Queries the Blob handle of the module from the base address of the Blob module.
- `g_pMetaHookAPI->BlobLoaderFindBlobByVirtualAddress`: Queries the Blob handle of the module from a specific address within the Blob module. As long as the address is within the range of ImageBase to ImageBase + ImageSize of the Blob module, the corresponding Blob module can be queried.

### API: DLL Load Callback Operations

- `g_pMetaHookAPI->RegisterLoadDllNotificationCallback`: Registers a callback for DLL load/unload notifications. Modules loaded/unloaded via LoadLibrary, import table, or Blob methods will execute your registered callback. You can use `(ctx->flags & LOAD_DLL_NOTIFICATION_IS_BLOB)` to determine if it is a Blob load/unload. You can use `(ctx->flags & LOAD_DLL_NOTIFICATION_IS_ENGINE)` to determine if the loaded/unloaded module is the engine. You can use `(ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)` to determine if the loaded/unloaded module is the client client.dll. You can use `(ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)` to determine if it is a load. You can use `(ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)` to determine if it is an unload.
- `g_pMetaHookAPI->UnregisterLoadDllNotificationCallback`: Allows you to unregister a previously registered DLL load/unload callback. Should be unregistered in `IPlugins::Shutdown`.

### API: Import Table Operations

- `g_pMetaHookAPI->ModuleHasImport`: Queries whether a specific HMODULE module imports a certain DLL.
- `g_pMetaHookAPI->ModuleHasImportEx`: Queries whether a specific HMODULE module imports a certain function.
- `g_pMetaHookAPI->BlobHasImport`: Queries whether a specific Blob module imports a certain DLL.
- `g_pMetaHookAPI->BlobHasImportEx`: Queries whether a specific Blob module imports a certain function.
- `g_pMetaHookAPI->BlobIATHook`: Similar to `g_pMetaHookAPI->IATHook`, with the only difference being that the first parameter is a Blob module handle instead of HMODULE.

### API: Get Current Game Directory

- `g_pMetaHookAPI->GetGameDirectory()`: Similar to `gEngfuncs.GetGameDirectory()`, but can be called before engine initialization (calling `gEngfuncs.GetGameDirectory()` before engine initialization will only return an empty string).

### API: Virtual Table Hook

- `g_pMetaHookAPI->VFTHookEx`: Similar to `g_pMetaHookAPI->VFTHook`, but does not require providing the object address, allowing direct hooking of a specific table entry when only the virtual table address is available.

### API: Patch Method Redirect Call/Jmp

- `g_pMetaHookAPI->InlinePatchRedirectBranch`: Redirects call/jmp instructions to a new function using a memory patch, affecting only the patched instruction.

### API: Mirror-DLL

Mirror-DLL is a memory module without executable permissions, and loaded without executing DLL entry point, only undergoing relocation fixes. The code segment (.text) and data segment (.rdata, .data) contents of the Mirror-DLL remain consistent with the state of the target DLL when it was first loaded.

Mirror-DLL provides a clean environment for plugins to search for signatures. When a plugin searches for signatures from a Mirror-DLL instead of the original DLL, it will not fail even if the target module has been hooked or patched by other third-party modules (like HLAE).

Since modules loaded in Blob format do not support relocation, Blob Engine and Blob Client do not provide corresponding Mirror-DLL support.

- `g_pMetaHookAPI->GetMirrorEngineBase`: Gets the base address of the engine loaded in Mirror-DLL form. Returns 0 for Blob Engine.
- `g_pMetaHookAPI->GetMirrorEngineSize`: Gets the size of the engine module loaded in Mirror-DLL form. Returns 0 for Blob Engine.
- `g_pMetaHookAPI->GetMirrorClientBase`: Gets the base address of the client.dll module loaded in Mirror-DLL form. Returns 0 for Blob Engine.
- `g_pMetaHookAPI->GetMirrorClientSize`: Gets the size of the client.dll module loaded in Mirror-DLL form. Returns 0 for Blob Engine.
- `g_pMetaHookAPI->LoadMirrorDLL_Std`: Loads a specific module in Mirror-DLL form. Internally uses fopen to open the DLL file.
- `g_pMetaHookAPI->LoadMirrorDLL_FileSystem`: Loads a specific module in Mirror-DLL form. Internally uses IFileSystem to open the DLL file.
- `g_pMetaHookAPI->FreeMirrorDLL`: Releases the module loaded in Mirror-DLL form.
- `g_pMetaHookAPI->GetMirrorDLLBase`: Obtains the base address of the module loaded in Mirror-DLL form.
- `g_pMetaHookAPI->GetMirrorDLLSize`: Obtains the size of the module loaded in Mirror-DLL form.

### Automatic Detection and Loading of SSE / SSE2 / AVX / AVX2 Versions of Plugins

1. The MetaHook launcher will always load the plugins listed in `\(ModDirectory)\metahook\configs\plugins.lst` in order from top to bottom. Lines with a semicolon ";" before the plugin name will be ignored.
2. When started in debug mode, automatically load (PluginName).dll.
3. If the file name from step (2) does not exist, and if the AVX2 instruction set is supported, automatically load (PluginName)_AVX2.dll.
4. If the file names from steps (2) and (3) do not exist, and if the AVX instruction set is supported and step (2) fails, automatically load (PluginName)_AVX.dll.
5. If the file names from steps (2), (3), and (4) do not exist, and if the SSE2 instruction set is supported and step (3) fails, automatically load (PluginName)_SSE2.dll.
6. If the file names from steps (2), (3), (4), and (5) do not exist, and if the SSE instruction set is supported and step (4) fails, automatically load (PluginName)_SSE.dll.
7. If the file names from steps (3), (4), (5), and (6) do not exist, automatically load (PluginName).dll.
8. If all the above steps fail, a popup will indicate that the plugin failed to load.

### Illegal Virtual Table Hook Check

A new launch parameter `-metahook_check_vfthook` has been added. This parameter will block any illegal calls to `g_pMetaHookAPI->MH_VFTHook`. Some calls to MH_VFTHook that arise from insufficient checks by plugin authors may attempt to hook addresses that exceed the actual virtual table range, potentially causing random crashes in the game. This launch parameter is specifically designed to address such situations and is not typically needed for normal use.

# Behavioral Changes of MetaHookSv (V4) Compared to MetaHook (V2)
### API Behavior Change: g_pMetaHookAPI->GetVideoMode

This API is used to obtain information related to VideoMode, such as game resolution, color depth, and whether it is in windowed mode. In MetaHook (V2), this API only retrieved information from the registry, resulting in inaccurate information with little reference value. In MetaHookSv (V4), if the `IVideoMode` interface is available in the engine, it will first attempt to use the `IVideoMode` interface to obtain relevant information; if that fails, it will fallback to the previous method of retrieval.

### API Behavior Change: g_pMetaHookAPI->GetEngineBase

In the MetaHook (V2) version, `g_pMetaHookAPI->GetEngineBase()` incorrectly returned 0x1D01000 instead of 0x1D00000 for BLOB encrypted versions of the engine (e.g., 3266). However, 0x1D01000 is actually the starting address of the code segment, not the engine base address. As a result, some plugins that relied on the V2 API would hard-code an incorrect offset based on this erroneous engine base (these plugins would use the correct offset of -0x1000 to counteract the V2 API's incorrect behavior, but this workaround could lead to an incorrect final offset if it encountered an API that returned the correct result). If a plugin uses the new `IPlugins` interface (`METAHOOK_PLUGIN_API_VERSION003` or `METAHOOK_PLUGIN_API_VERSION004`), it will return the correct 0x1D00000 for BLOB encrypted versions of the engine by default. Otherwise, the plugin will still receive the "incorrect" return value for the Blob engine as it did in the MetaHook (V2) era.

### Automatic Ignoring of Duplicate Plugins

If a plugin appears twice in the loading list or is loaded by another DLL in some other way, MetaHookSv will not load that plugin again. 
* Duplicate loading may lead to situations where a plugin replaces function pointers and ends up calling itself, resulting in infinite recursion.

### Support for Hook "Transactions"

During the engine's call to all plugins' `LoadEngine`, a "transaction" will be opened for all `InlineHook`, `VFTHook`, and `IATHook` requests. The hooks will only take effect after all plugins' `LoadEngine` interface calls have completed. This allows different plugins to simultaneously `SearchPattern` and hook the same function, avoiding conflicts where the previous plugin's early hook modifies the engine code, causing the subsequent plugin's pattern search to fail.

The transaction opening timing includes: during the engine's calls to all plugins' `LoadEngine` and `LoadClient`, during the engine's call to the client's `HUD_GetStudioModelInterface`, and during the DllLoadNotification period.

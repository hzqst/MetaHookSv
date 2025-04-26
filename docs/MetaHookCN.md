# ABI兼容性

MetaHookSv (V4) 导出的 `g_pMetaHookAPI` 接口从ABI层面完全兼容 MetaHook (V2) 时期的插件，所以加载 MetaHook (V2) 时期的插件也没有任何兼容性问题。唯一需要考虑的是与其他插件之间的兼容性。

# MetaHookSv (V4) 相比 MetaHook (V2) 的新功能

### 自动探测引擎类型

自动识别SvEngine、GoldSrc_HL25、GoldSrc、GoldSrc_Blob四种类型的引擎。

SvEngine：Sven Co-op团队自己魔改的GoldSrc分支

GoldSrc_HL25："Half-Life 25周年更新"后的最新GoldSrc，引擎buildnum大于等于9884

GoldSrc_Blob：引擎buildnum小于4554，使用非标准PE格式加密的引擎

GoldSrc：引擎buildnum在4554~8684之间的GoldSrc

可以用`g_pMetaHookAPI->GetEngineType`或`g_pMetaHookAPI->GetEngineTypeName`来获取对应的引擎类型。

### API：反向搜索函数头

可以用`g_pMetaHookAPI->ReverseSearchFunctionBegin`或`g_pMetaHookAPI->ReverseSearchFunctionBeginEx`来从某个特定的地址往回搜索函数头

`g_pMetaHookAPI->ReverseSearchFunctionBegin` 在碰到前一个字节为CC、90、C3的情况下，会用反汇编引擎从当前字节开始遍历整个函数，如果遍历过程中最终没有返回到前一个字节的位置，则当前字节会被确定为函数头。该函数在碰到前一个函数结尾为E9 (JMP)等非正常结尾，并且中间没有Padding字节，整个函数体直接跟下一个函数体连着的情况下会无法正常获取函数头。这种情况就需要用到`g_pMetaHookAPI->ReverseSearchFunctionBeginEx`。

`g_pMetaHookAPI->ReverseSearchFunctionBeginEx` 需要提供在回调中对特定的Pattern返回TRUE才会停止搜索并确定为函数头。

### API：反汇编引擎

反汇编引擎使用的是capstone 4.0.2

`g_pMetaHookAPI->DisasmSingleInstruction` ：反汇编单条指令

`g_pMetaHookAPI->DisasmRanges` ：反汇编某个区域内的指令

### API：反向搜索特征码

`g_pMetaHookAPI->ReverseSearchPattern`

同 `g_pMetaHookAPI->SearchPattern`，唯一的区别是从pStartSearch开始往前搜索而不是往下

### API：对客户端模块(client.dll)相关的操作

`g_pMetaHookAPI->GetClientModule` ：获取客户端模块。仅非blob方式加载的client.dll才能获取到HMODULE，对于以blob方式加载的client.dll会返回NULL。

`g_pMetaHookAPI->GetClientBase` ：获取客户端基址。同时支持非blob方式和blob方式加载的client.dll。

`g_pMetaHookAPI->GetClientSize` ：获取客户端模块大小。同时支持非blob方式和blob方式加载的client.dll。

`g_pMetaHookAPI->GetClientFactory` ：获取客户端的接口工厂（即`interface.cpp`中的`CreateInterface`）。同时支持非blob方式和blob方式加载的client.dll。

### API：请求其他插件的信息
 
`g_pMetaHookAPI->QueryPluginInfo` ：可以获取所有加载的插件的信息，获取方式：

```
mh_plugininfo_t info;
if(g_pMetaHookAPI->GetPluginInfo("PluginName.dll", &info))//"PluginName.dll" is case-insensitive
{

}
```

`g_pMetaHookAPI->GetPluginInfo` ：根据名字查询某个插件，插件名不考虑_SSE _AVX等后缀。

```
mh_plugininfo_t info;
if(g_pMetaHookAPI->GetPluginInfo("PluginName.dll", &info))//"PluginName.dll" is case-insensitive
{

}
```

### API：HookUserMsg

`g_pMetaHookAPI->HookUserMsg` ：同以前某些插件实现里的HookUserMsg，只是现在被g_pMetaHookAPI导出了。

### API: HookCvarCallback、RegisterCvarCallback

cvar回调是Valve在buildnum 6153 GoldSrc引擎中添加的功能，用于在cvar被修改时执行特定回调，Valve自己只在`gl_texturemode`上使用该功能。

`g_pMetaHookAPI->HookCvarCallback` ：提供了hook某个cvar的修改回调的功能。

`g_pMetaHookAPI->RegisterCvarCallback` ：提供了注册某个cvar的修改回调的功能。在HookCvarCallback失败的情况下可以自行注册。（没有人注册过该cvar的回调就会hook失败）

### API：HookCmd

`g_pMetaHookAPI->HookCmd` ：同以前某些插件实现里的HookCmd，只是现在被g_pMetaHookAPI导出了。

### API：SysError

`g_pMetaHookAPI->SysError` ：同以前某些插件实现里的SysErrorEx（弹窗报错并退出游戏），只是现在被g_pMetaHookAPI导出了。

### API：IsDebuggerPresent

`g_pMetaHookAPI->IsDebuggerPresent` ：检测调试器是否存在

### API：Blob模块相关操作

`g_pMetaHookAPI->GetBlobEngineModule` ：获取Blob格式引擎的模块句柄，该模块句柄不能与HMODULE混用，仅能用于操作Blob模块。

`g_pMetaHookAPI->GetBlobClientModule` ：获取Blob格式client.dll的模块句柄，该模块句柄不能与HMODULE混用，仅能用于操作Blob模块。

`g_pMetaHookAPI->GetBlobModuleImageBase` ：从Blob模块句柄获取该模块的基址。

`g_pMetaHookAPI->GetBlobModuleImageSize` ：从Blob模块句柄获取该模块的大小。

`g_pMetaHookAPI->GetBlobSectionByName` ：从Blob模块句柄对应的Blob模块中寻找特定节。仅支持".text\0\0\0"和".data\0\0\0"。（因为Blob格式的模块一般只有这两个节持有有效信息）

`g_pMetaHookAPI->BlobLoaderFindBlobByImageBase` ：从Blob模块的基址反向查询该模块的Blob句柄。

`g_pMetaHookAPI->BlobLoaderFindBlobByVirtualAddress` ：从Blob模块中的某一个地址反向查询该模块的Blob句柄。只要该地址在Blob模块的ImageBase~ImageBase+ImageSize区间即可查询到对应的Blob模块。

### API：DLL加载回调相关操作

`g_pMetaHookAPI->RegisterLoadDllNotificationCallback` ：注册DLL加载卸载回调。

以LoadLibrary方式、导入表方式或Blob方式加载卸载的模块都会执行你注册的回调。

可以用`(ctx->flags & LOAD_DLL_NOTIFICATION_IS_BLOB)`来判断是否是Blob方式加载/卸载。

可以用`(ctx->flags & LOAD_DLL_NOTIFICATION_IS_ENGINE)`来判断该加载/卸载的模块是否是引擎。

可以用`(ctx->flags & LOAD_DLL_NOTIFICATION_IS_CLIENT)`来判断该加载/卸载的模块是否是客户端client.dll。

可以用`(ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)`来判断是否是加载。

可以用`(ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)`来判断是否是卸载。

`g_pMetaHookAPI->UnregisterLoadDllNotificationCallback` ：可以反注册之前注册的DLL加载卸载回调。应在`IPlugins::Shutdown`中反注册。

### API：导入表操作

`g_pMetaHookAPI->ModuleHasImport` ：可以查询某个HMODULE模块是否导入了某个dll。

`g_pMetaHookAPI->ModuleHasImportEx` ：可以查询某个HMODULE模块是否导入了某个函数。

`g_pMetaHookAPI->BlobHasImport` ：可以查询某个Blob模块是否导入了某个dll。

`g_pMetaHookAPI->BlobHasImportEx` ：可以查询某个Blob模块是否导入了某个函数。

`g_pMetaHookAPI->BlobIATHook` ：同`g_pMetaHookAPI->IATHook`，唯一的区别是第一个参数从HMODULE变成了Blob模块句柄。

### API：获取当前游戏目录

`g_pMetaHookAPI->GetGameDirectory()`：同`gEngfuncs.GetGameDirectory()`，但是可以在引擎初始化之前调用。（引擎初始化之前调用`gEngfuncs.GetGameDirectory()`只能得到空字符串）

### API：虚表hook

`g_pMetaHookAPI->VFTHookEx` ：同`g_pMetaHookAPI->VFTHook`，但是无需提供对象地址，用于只有虚表地址得情况下直接hook虚表的特定表项。

### API：Patch方式重定向call/jmp

`g_pMetaHookAPI->InlinePatchRedirectBranch` ：以内存补丁的方式重定向call/jmp指令到新的函数上，只由被patch的那条指令会受该hook影响。

### API: Mirror-DLL

Mirror-DLL 是以内存模块形式加载的DLL，不包含可执行权限，没有执行过DLL入口点，只经过重定位修正。Mirror-DLL中的代码段(.text)和数据段(.rdata .data)内容与目标DLL刚加载时的状态保持一致。

Mirror-DLL 用于给插件提供一个干净的搜索特征码的环境。当插件从Mirror-DLL而非原始模块中搜索特征码时，即使目标模块已经被其他第三方模块hook或patch过，也不会导致特征码搜索失败。

由于Blob形式加载的模块不支持重定位，所以Blob Engine和Blob Client不提供对应的Mirror-DLL支持。

`g_pMetaHookAPI->GetMirrorEngineBase` : 获取以Mirror-DLL形式加载的引擎基地址。对于Blob Engine返回0

`g_pMetaHookAPI->GetMirrorEngineSize` : 获取以Mirror-DLL形式加载的引擎模块大小。对于Blob Engine返回0

`g_pMetaHookAPI->GetMirrorClientBase` : 获取以Mirror-DLL形式加载的client.dll模块基地址。对于Blob Engine返回0

`g_pMetaHookAPI->GetMirrorClientSize` : 获取以Mirror-DLL形式加载的client.dll模块大小。对于Blob Engine返回0

`g_pMetaHookAPI->LoadMirrorDLL_Std` : 以Mirror-DLL形式加载特定模块。内部使用fopen打开DLL文件。

`g_pMetaHookAPI->LoadMirrorDLL_FileSystem` : 以Mirror-DLL形式加载特定模块。内部使用IFileSystem打开DLL文件。

`g_pMetaHookAPI->FreeMirrorDLL` : 释放以Mirror-DLL形式加载的模块。

`g_pMetaHookAPI->GetMirrorDLLBase` : 获得以Mirror-DLL形式加载的模块的基地址。

`g_pMetaHookAPI->GetMirrorDLLSize` : 获得以Mirror-DLL形式加载的模块的大小。

### API：线程池相关操作

#### `g_pMetaHookAPI->GetGlobalThreadPool`

获取全局线程池的句柄。全局线程池在MetaHook初始化时自动创建，适合大多数通用异步任务使用。返回值为线程池句柄（`ThreadPoolHandle_t`）。

#### `g_pMetaHookAPI->CreateThreadPool`

创建一个新的线程池。参数为最小线程数和最大线程数，返回新线程池的句柄。适合需要独立线程池的场景。

```cpp
ThreadPoolHandle_t hPool = g_pMetaHookAPI->CreateThreadPool(2, 8);
```

#### `g_pMetaHookAPI->CreateWorkItem`

在指定线程池下创建一个工作任务。参数为线程池句柄、回调函数和上下文指针。回调函数签名为 `bool (*fnThreadWorkItemCallback)(void* ctx)`，返回 `true` 表示任务完成后立即释放该工作任务。在这种情况下，你无需调用DeleteWorkItem删除该工作任务。如果返回`false`则需要你手动调用DeleteWorkItem删除该工作任务。

```cpp
ThreadWorkItemHandle_t hWorkItem = g_pMetaHookAPI->CreateWorkItem(hPool, MyCallback, myContext);
```

#### `g_pMetaHookAPI->QueueWorkItem`

将工作任务加入线程池队列，等待线程池中的线程调度执行。

```cpp
g_pMetaHookAPI->QueueWorkItem(hPool, hWorkItem);
```

#### `g_pMetaHookAPI->WaitForWorkItemToComplete`

阻塞当前线程，直到指定的工作任务执行完成。

```cpp
g_pMetaHookAPI->WaitForWorkItemToComplete(hWorkItem);
```

#### `g_pMetaHookAPI->DeleteThreadPool`

销毁指定的线程池及其所有资源。注意：参数为线程池句柄，销毁后该线程池不可再用。

```cpp
g_pMetaHookAPI->DeleteThreadPool(hPool);
```

#### `g_pMetaHookAPI->DeleteWorkItem`

销毁指定的工作任务。通常在工作任务完成后调用以释放资源。

```cpp
g_pMetaHookAPI->DeleteWorkItem(hWorkItem);
```

### 自动检测并加载 SSE / SSE2 / AVX / AVX2 版本的插件

1. MetaHook启动器总是会以从上到下的顺序加载 `\(ModDirectory)\metahook\configs\plugins.lst` 中列出的插件。当插件名前面存在引号";"时该行会被忽略。

2. 当以调试模式启动时自动加载(PluginName).dll

3. 当(2)的文件名不存在，且当支持AVX2指令集时自动加载(PluginName)_AVX2.dll

4. 当(2) (3)的文件名不存在，且当支持AVX指令集时且(2)失败时自动加载(PluginName)_AVX.dll

5. 当(2) (3) (4)的文件名不存在，当支持SSE2指令集时且(3)失败时自动加载(PluginName)_SSE2.dll

6. 当(2) (3) (4) (5)的文件名不存在，当支持SSE指令集时且(4)失败时自动加载(PluginName)_SSE.dll

7. 当(3) (4) (5) (6)的文件名均不存在时自动加载(PluginName).dll

8. 如果上述步骤全部失败，则弹窗提示该插件加载失败。

### 非法虚表hook检查

新增启动项参数 `-metahook_check_vfthook` ，该启动项将屏蔽任何非法的 `g_pMetaHookAPI->MH_VFTHook` 调用。

有些来自插件作者检查不严格而产生的MH_VFTHook调用会对某些超出真实虚表范围的地址进行hook，这可能导致游戏随机崩溃等问题。该启动项专门用于解决这种情况，正常情况下不需要使用。

# MetaHookSv (V4) 相比 MetaHook (V2) 的行为变化

### API行为改变：g_pMetaHookAPI->GetVideoMode

该API用于获取VideoMode相关信息，如游戏分辨率、色深、是否窗口模式等信息。

在MetaHook (V2)中该API只会从注册表中获取信息，得到的信息很不准确，基本上没有参考价值

MetaHookSv (V4)在引擎中的`IVideoMode`接口可用的情况下，会先尝试使用`IVideoMode`接口获取相关信息，如果失败才会fallback到以前的获取方式。

### API行为改变：g_pMetaHookAPI->GetEngineBase

MetaHook (V2)版本的 `g_pMetaHookAPI->GetEngineBase()` 对BLOB加密版本的引擎(如3266)会错误返回 0x1D01000 而非 0x1D00000。

然而实际上 0x1D01000 是代码段起始地址而非引擎基址。因此某些依赖V2版本API的插件会依赖错误的引擎基址而硬编码一个错误的偏移（这些插件使用正确偏移-0x1000来达到抵消V2API错误行为的目的，但是这种抵消手法如果遇上返回正确结果的API就会导致算出的最终偏移比正常的大0x1000）。

如果插件使用新的`IPlugins`接口（`METAHOOK_PLUGIN_API_VERSION003`或`METAHOOK_PLUGIN_API_VERSION004`）则默认对BLOB加密版本的引擎返回正确的0x1D00000。否则该插件仍会像MetaHook (V2)时期那样对Blob引擎获取到“错误”的返回值。

### 自动忽略重复插件

如果一个插件在加载列表里出现了两次，或者被别的dll以其他方式加载进来了，MetaHookSv不会重复加载该插件。

* 重复加载可能会导致插件替换函数指针后出现自己调用自己的情况，引发无限递归。

### Hook的"事务"支持

引擎调用所有插件的 `LoadEngine` 期间会对所有 `InlineHook`, `VFTHook` 和 `IATHook` 请求开启"事务"。

直到所有插件的 `LoadEngine` 接口调用结束才会让hook真正生效

这样就可以允许不同插件同时 `SearchPattern` 和 hook 同一个函数，避免了因为前一个插件提前hook修改了引擎代码导致后一个插件搜索特征码失败等插件之间互相冲突的问题。

事务开启时机：引擎调用所有插件的`LoadEngine`和`LoadClient`期间、引擎调用客户端的 `HUD_GetStudioModelInterface` 期间以及DllLoadNotification期间。
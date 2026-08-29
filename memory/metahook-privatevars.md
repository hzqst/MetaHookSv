---
title: metahook-privatevars
type: reference
permalink: metahooksv/metahook-privatevars
tags:
- metahook
- private-vars
- symbol-locating
- reference
---

# `metahook.cpp` 使用的游戏私有符号

本文统计 `src/metahook.cpp` 在当前版本中从游戏引擎映像定位、并在后续逻辑中消费的未导出私有函数、私有数据槽与私有调用点。符号名称以代码中的本地持有变量为准；括号内是根据用途推定的游戏侧含义，并非上游二进制的正式调试符号名。

相关：[[meta-hook]] [[project-overview]]

> **gamedata 迁移状态（2026-08-29）**：以下 10 个「最终消费」符号已改为由本地 gamedata catalog 经 `ResolveGameSymbol` 解析，不再使用镜像映像 + 签名/字符串/交叉引用扫描：
> `build_number`、`Sys_Error`、`ClientDLL_HudInit`、`g_ppEngfuncs`、`g_ppExportFuncs`、`g_phClientModule`、`g_pClientFactory`、`videomode`、`gClientUserMsgs`、`cl_parsefuncs`。
> 解析入口见 `src/metahook.cpp` 的 `MH_LoadEngine_ResolveSymbol`；catalog 初始化、模块来源注册与镜像别名注册见 `MH_LoadEngine` 中 `GameData::Initialize` / `RegisterModuleFileSource` / `RegisterMirrorAlias` 调用。
>
> cvar 分支（`cvar_hooks` / `Cvar_Set` / `Cvar_DirectSet`）与 blob client 的 `FreeBlob` 仍沿用旧扫描，原因是发布 gamedata 尚缺这些符号（`scripts/validate-gamedata.py` 会作为发布门禁阻断）。下表仍以旧定位方式为主，待数据补齐后再更新。

## 范围与共通定位过程

- 统计范围包括：地址被保存到全局/局部变量，或作为反汇编起点、比较对象、hook/patch 目标使用的游戏内部对象。
- 不包括：`MH_*` 等 MetaHook 自身符号、`g_ManagedCvarCallbackList` 等本地状态，以及从公开 `cl_enginefunc_t` 表取得的 API（例如 `Cvar_Set`、`GetFirstCmdFunctionHandle`）。
- 大多数定位在镜像映像 `DllInfo`（优先使用 `g_hMirrorEngine`）中完成，再经 `ConvertDllInfoSpace` 按 RVA 映射回实际加载映像 `RealDllInfo`；转换实现见 `src/metahook.cpp:1268-1279`，所有定位例程的调度见 `src/metahook.cpp:2265-2276`。
- 定位失败会通过 `MH_SysError` 中止加载；这意味着下表的强制项均有运行时门禁。各变量还会在 `MH_ResetAllVars` 中清零（`src/metahook.cpp:1211-1241`）。

## 私有函数

| 本地符号 / 推定游戏符号 | 声明位置 | 定位代码范围与原理 | 后续使用 |
| --- | --- | --- | --- |
| `g_pfnbuild_number`（`buildnumber`） | `src/metahook.cpp:103` | `src/metahook.cpp:1282-1317`：先在 `.text` 匹配 `E8` 调用序列并取后继 call 目标；失败时在 `.rdata/.data` 查找 `"Exe build: "`，再以其 `push` 交叉引用取 call 目标。 | `src/metahook.cpp:1333-1336` 用返回值识别 HL25；`src/metahook.cpp:3769-3772` 由 `MH_GetEngineVersion` 转发。 |
| `g_pfnSys_Error`（`Sys_Error`） | `src/metahook.cpp:214` | `src/metahook.cpp:1349-1387`：按 SvEngine/GoldSrc 选择 "Couldn't/could not link client…Initialize" 错误字符串；在 `.text` 匹配 `push <string>; call`，取该 call 的目的地址。 | `src/metahook.cpp:236-245`，`MH_SysError` 将格式化后的致命错误转发到引擎。 |
| `g_pfnClientDLL_Init`（引擎内 client DLL 初始化例程） | `src/metahook.cpp:104` | `src/metahook.cpp:1389-1497`：以 `"ScreenShake"` 的 `push` 交叉引用为锚点，使用三种函数序言谓词向前恢复函数入口；随后以该区域反汇编寻找 `ppEngfuncs` 与 `ppExportFuncs`。 | 自身只作定位成功门禁（`src/metahook.cpp:1481-1485`），不直接调用；其成功定位是提取两个私有函数表槽的前提。 |
| `g_pfnClientDLL_HudInit`（引擎内 HUD 初始化例程） | `src/metahook.cpp:105` | `src/metahook.cpp:1503-1626`：以 `"cl_righthand"` 的 `push` 交叉引用为锚点，通过 `MH_ReverseSearchFunctionBeginEx` 和 HudInit 专用入口谓词恢复拥有函数入口；谓词覆盖常见的 `mov eax,[pHudInitFunc]` 无序言形式与 CoF 的 EBP frame 形式。 | `src/metahook.cpp:1135-1165` 的代理在调用原函数前开启 hook transaction、返回后提交；`src/metahook.cpp:2304-2305` 安装 inline hook。事务因此覆盖 `HUD_Init`、Studio interface 初始化和随后的 `cl_righthand` 查询。 |
| `DispatchDirectUserMsg`（临时局部，直接用户消息分派函数） | `src/metahook.cpp:1809` | `src/metahook.cpp:1794-1843`：由 `"HudText"` 的 `push` 交叉引用取得紧随其后的 call 目标。 | 不保存为全局，也不 hook；仅作为 `MH_DisasmRanges` 的起点，在该函数起始后的 `0x50` 字节内提取 `gClientUserMsgs`。 |
| `g_pfnCvar_DirectSet`（`Cvar_DirectSet`） | `src/metahook.cpp:105` | `src/metahook.cpp:1894-1979`：在数据段查找 `"***PROTECTED***"`，遍历该字符串的 `.text` `push` 引用，并以多种序言谓词反向恢复函数入口。 | `src/metahook.cpp:299-315` 调用；`src/metahook.cpp:2050-2068` 在 `Cvar_Set` 中识别指向它的分支并重定向至 `MH_Cvar_DirectSet`。 |
| `g_pfnLoadBlobFile`（`LoadBlobFile`） | `src/metahook.cpp:106` | `src/metahook.cpp:2097-2132`：仅 GoldSrc/Blob/HL25 分支；在 `.text` 找特征 `85 BC 32 7A FF`，并验证其前 `0x50` 字节中存在 `6A 00 6A 01 6A 00`，再向前恢复函数入口。 | `src/metahook.cpp:2395-2398` 安装 inline hook，替换为 `MH_LoadBlobFile`。 |
| `g_pfnFreeBlob`（`FreeBlob` / 注释中的 `UnloadBlob`） | `src/metahook.cpp:107` | `src/metahook.cpp:2134-2172`：从三套 `.text` `push; call` 特征取得 call 目标；后两套将已定位的 `g_phClientModule` 映射回扫描空间并写入签名作二次约束。 | `src/metahook.cpp:2405-2408` 安装 inline hook，替换为 `MH_FreeBlobProxy`。 |

## 私有变量与内部表

| 本地符号 / 推定游戏对象 | 声明位置 | 定位代码范围与原理 | 后续使用 |
| --- | --- | --- | --- |
| `g_ppEngfuncs`（`cl_enginefunc_t **` 槽） | `src/metahook.cpp:129` | `src/metahook.cpp:1454-1475`：在已确定的 `ClientDLL_Init` 附近反汇编，匹配 `6A 07 68 <imm32>`，将立即数操作数位置 `address + 3` 映射为真实槽地址。 | `src/metahook.cpp:1499-1501` 解引用并复制引擎函数表；`src/metahook.cpp:2420-2423` 解引用后传给 V3/V4 插件。 |
| `g_ppExportFuncs`（`cl_exportfuncs_t **` 槽） | `src/metahook.cpp:128` | `src/metahook.cpp:1454-1475`：同一反汇编窗口中匹配 `FF 15 <imm32>`，将 `address + 2` 的立即数操作数位置映射为真实槽地址。 | `src/metahook.cpp:2368-2373` 解引用得到原导出表，并覆写其中的 `Initialize` 槽。 |
| `pfnHUDInit`（临时局部；client `HUD_Init` 回调指针槽） | `src/metahook.cpp:1547` | `src/metahook.cpp:1503-1600`：从 `"cl_righthand"` 的 `.text` 交叉引用反向匹配 `A1 <imm32>; test eax`；`A1` 的立即数是该私有全局槽地址。 | `src/metahook.cpp:1575-1578` 仅作为排除条件，防止将 HUD 初始化回调槽误判为 `g_phClientModule`。该地址不会长期保存。 |
| `g_phClientModule`（`HMODULE *`，游戏的 client DLL 模块句柄槽） | `src/metahook.cpp:112` | `src/metahook.cpp:1503-1626`：沿同一 `"cl_righthand"` 锚点在 `HUD_Init` 邻域反汇编，提取位于 `.data` 的绝对内存操作数并排除 `pfnHUDInit`；fallback 匹配 `A1 <imm32>; push; call`。 | `src/metahook.cpp:1172-1181` 控制 client 镜像加载；`src/metahook.cpp:2176-2193` 约束 `FreeBlob` 签名；由 `MH_GetClientModule` 对外返回模块句柄。 |
| `g_pClientFactory`（`CreateInterfaceFn *` 槽） | `src/metahook.cpp:114` | `src/metahook.cpp:1598-1641`：查找 `"VClientVGUI001"`，由其 `.text` 引用向前匹配 `A1 <imm32>; test` 或 `83 3D <imm32>,0`，解引用操作数位置得到 factory 槽。 | `src/metahook.cpp:3721-3726` 解引用并返回 client factory。 |
| `videomode`（视频模式对象指针槽） | `src/metahook.cpp:101` | `src/metahook.cpp:1643-1792`：以 `"-fullscreen"` 为锚点（非 SvEngine 先尝试 `"-gl"`）；在后续 `0x400` 字节反汇编，匹配对 `.data` 绝对地址写零/寄存器的 `mov`，并要求后 15 条指令内出现 `ret`。 | `src/metahook.cpp:3528-3592` 解引用为 `IVideoMode` / `IVideoMode_HL25`，读取、返回及保存视频模式。 |
| `gClientUserMsgs`（`usermsg_t **`，用户消息链表头槽） | `src/metahook.cpp:99` | `src/metahook.cpp:1794-1853`：先解析临时私有函数 `DispatchDirectUserMsg`，再在该函数起始后的 `0x50` 字节中匹配 `mov reg,[absolute]`，并验证绝对地址位于模块映像范围。 | `src/metahook.cpp:424-455` 解引用并遍历/Hook 用户消息。 |
| `cl_parsefuncs`（`svc_func_t *`，SVC 解析函数表基址） | `src/metahook.cpp:102` | `src/metahook.cpp:1855-1892`：在 `.data` 循环查找含 opcode 序号的表布局特征；再验证第二字段指向 `.data/.rdata` 且字符串严格为 `"svc_bad"`。 | `src/metahook.cpp:503-564` 返回表基址，并按 opcode/名称查询或替换解析函数。 |
| `cvar_callbacks`（`cvar_callback_entry_t **`，cvar 回调链表头槽） | `src/metahook.cpp:96` | `src/metahook.cpp:1981-2094`：把公开 `Cvar_Set` 映射回扫描空间，在其前 `0x150` 字节中匹配 `mov eax,[absolute]` 以抽取链表头；若失败，改写对私有 `Cvar_DirectSet` 的调用，并退化至本地 `g_ManagedCvarCallbackList`。 | 真实游戏槽由 `src/metahook.cpp:299-419` 遍历/插入；`src/metahook.cpp:2659-2662` 在退出时清空。退化分支不是游戏变量。 |
## 从私有槽派生、但不独立扫描的对象

- `g_pExportFuncs`（`cl_exportfuncs_t *`，声明 `src/metahook.cpp:127`）：从私有 `g_ppExportFuncs` 解引用得到（`src/metahook.cpp:2368-2373`），之后由 `ClientDLL_Initialize` 复制、传给插件并调用原 `Initialize`（`src/metahook.cpp:1166-1207`）。它不是另一条签名扫描结果。
- `gMetaSave.pEngineFuncs`：从私有 `g_ppEngfuncs` 解引用并复制（`src/metahook.cpp:1499-1501`），随后作为公开 `cl_enginefunc_t` 表使用。因此 `Cmd_GetCmdBase` 也不属于独立定位的私有函数。

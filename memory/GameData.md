---
title: GameData
type: note
permalink: metahooksv/game-data
tags:
- metahook
- gamedata
- symbol-catalog
- api-109
- api-110
- api-111
- api-112
- crc64
---

# GameData

## Overview

GameData 是 launcher 与 V3/V4 插件共享的本地游戏符号目录（catalog）组件：从 `<game>\<mod>\metahook\gamedata\index.json` 读取并冻结一份只读符号表，按 `(moduleCRC64, symbolName)` 查询符号元数据，并通过 `moduleBase` 懒计算模块原始文件的 CRC-64/XZ。公共接口以 MetaHook API 109 暴露（`metahook_api_t` 尾部的 6 个函数槽），API 110 追加 `MH_GAMESYMBOL_KIND_PATCH` 与 `IsGameSymbolAvailable` 槽位，API 111 追加 `MH_GAMESYMBOL_KIND_SCALAR` 与 `QueryGameSymbolScalar` 槽位，API 112 追加 `MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION`（不新增函数槽位，经 `ResolveGameSymbol` / `QueryGameSymbol` 消费；均不改动旧槽位偏移）。

## Responsibilities

- 构建并冻结只读 catalog（`GameData::Initialize`），初始化后不可重载，所有返回指针在进程退出前有效。
- 校验 index（schema v4）与每个 snapshot（dataset schema v5 / `source.snapshotSchemaVersion` v8 / `source.analysisOutputContractVersion` v3）的路径安全、size、SHA-256（`Chocobo1::SHA2_256`）。旧代际（dataset schema 4 / source contract 7）被明确拒绝并记为 diagnostic。
- 只提取 `platform == "windows"` 的记录，将 `function` / `global` / `patch` / `scalar` / `virtualFunction` payload 规范化为 `GameSymbolRecord`；单 snapshot 失败隔离为 diagnostics，不破坏整个 catalog。`patch` 记录正规化为 `MH_GAMESYMBOL_KIND_PATCH`，地址取 `patch_rva`，不把 signature 当函数长度。`scalar` 记录正规化为 `MH_GAMESYMBOL_KIND_SCALAR`，只保留 uint32 `scalar_value`（地址字段全 0，不加 image base、不解引用）。
- 将 signature 文本编译为 `(bytes, mask, legacyPattern)` 三态。
- 按 `moduleBase` 管理 `ModuleIdentity`（普通 PE / Blob 文件 / None），懒计算并缓存模块 CRC-64/XZ，处理 mirror alias 与 unload 失效。
- 实现公共 API：`MH_GetModuleCRC64`、`MH_QueryGameSymbol`、`MH_QueryGameSymbolByCRC64`、`MH_ResolveGameSymbol`、`MH_SearchPatternMasked`、`MH_GetGameSymbolStatusString`，API 110 的 `MH_IsGameSymbolAvailable`（仅返回状态码：存在 `OK`、不存在 `SYMBOL_NOT_FOUND`、其它失败保留原状态；不返回地址），以及 API 111 的 `MH_QueryGameSymbolScalar`（按 `moduleBase` + 名字返回 uint32；非 scalar kind 返回 `KIND_MISMATCH`）。
- 提供 launcher 内部 getter `GameData::GetGameVersion(moduleCRC64, &gameVersion)`（不进入 `metahook_api_t`），用于按引擎模块 CRC 反查 catalog 中的 gameVersion。
- 提供 `GameData::RegisterModuleFileSource`（Blob engine）与 `RegisterMirrorAlias` 供 launcher 注册模块来源。

## Involved Files & Symbols

- `src/GameData.h` — `GameData` namespace 声明（`Initialize`/`QueryByCRC64`/`QueryScalarByCRC64`/`GetGameVersion`/`GetModuleCRC64`/`RegisterModuleFileSource`/`RegisterMirrorAlias`/`InvalidateModule`/`ResetModuleIdentities`）与公共 `MH_*` 入口。
- `src/GameData.cpp` — 实现 `GameDataCatalog` 构建（`Initialize`/`LoadSnapshot`/`ValidateIndex`）、签名解析 `ParseSignature`、payload 规范化 `NormalizeFunction`/`NormalizeGlobal`/`NormalizePatch`/`NormalizeScalar`、模块哈希状态机 `GetModuleCRC64`/`ComputeCrc64FromFile`、公共 API 与 `MH_GetGameSymbolStatusString`。
- `include/metahook.h` — `METAHOOK_API_VERSION 112`、`mh_gamesymbol_kind_t`（含 `MH_GAMESYMBOL_KIND_PATCH` / `MH_GAMESYMBOL_KIND_SCALAR` / `MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION`）/`mh_gamesymbol_status_t`/`mh_pattern_t`/`mh_gamesymbol_t`、`metahook_api_t` 新增的 8 个函数槽（第 6 个为 `IsGameSymbolAvailable`，第 8 个为 `QueryGameSymbolScalar`）。
- `src/metahook.cpp` — launcher 集成：`MH_LoadEngine_FormatModuleIdentity` / `MH_LoadEngine_ReportSymbolFailure` / `MH_LoadEngine_ResolveSymbol` / `MH_LoadEngine_ResolveGlobalOperand`、`MH_LoadEngine_DetermineEngineType` / `MH_LoadEngine_FindCvarDirectSet` / `MH_LoadEngine_PatchCvarCallbacks` / `MH_LoadEngine_FindLoadBlobClient`（均 gamedata-only），以及 `MH_LoadEngine` 中 catalog 初始化与来源注册。
- `src/LoadDllNotification.cpp` / `.h` — DLL 加/卸载通知中调用 `GameData::InvalidateModule`（第 144、212 行）与 `MH_IsInLdrCriticalRegion`（第 66 行）。
- `scripts/sync-gamedata.py` — Pre-build 从固定 HTTPS index 下载、校验并事务式替换打包目录。
- `scripts/validate-gamedata.py` — 发布门禁：校验 index/snapshot、期望符号、冲突与 required profiles。
- `tools/global_common.props` — `GameSymbolsIndexUrl`、gamedata 目录、统一 `GameDataSyncCommand`。
- `src/MetaHook.vcxproj` / `.filters` — 加入 `GameData.cpp` 与 rapidjson / Chocobo1Hash include 路径。
- `.gitignore` — 忽略 `Build/svencoop/metahook/gamedata/`。
- `Build\svencoop\metahook\gamedata\index.json` 及 `<snapshot>.json` — 打包数据。

## Architecture

### 架构约定：可信上游与 gamedata-only 定位（2026-09-08 确认）

- [constraint] 始终信任上游 gamedata 提供的数据正确。复用现有查询、解析和发布门禁；后续实现与审查不因假设上游提供错误 RVA、错误长度或跨 snapshot 元数据冲突而新增防御体系。既有校验和错误状态处理继续保留。
- [decision] 能由 gamedata 定位的符号，最终地址统一通过 `MH_LoadEngine_ResolveSymbol` / `MH_ResolveGameSymbol` 获取；插件使用对应的 `g_pMetaHookAPI->ResolveGameSymbol`。不保留 signature、字符串或反查定位 fallback。
- **触发信号 / 适用范围**：launcher、插件及共享代码新增或迁移 gamedata 消费逻辑，以及相关 spec、代码审查；这是长期架构约定，不限于 #850。
- **根因 / 约束**：上游负责符号数据的正确性，消费端复用统一查询与地址解析职责，避免重复定位及额外防御体系。
- **正确做法**：元数据查询和存在性判断不替代最终地址 Resolve；缺少必需数据按既有失败诊断处理，补数由上游完成。对于已提供地址的 call-site 也直接消费 gamedata，不再通过函数体 disasm 重新定位。
- **验证方式**：审查最终地址来源、确认旧定位 fallback 已移除，运行现有发布门禁及与行为直接相关的定向验证；不为上述假设的错误上游数据新增防御性测试体系。
- **决策来源**：用户在 [issue #850](https://github.com/hzqst/MetaHookSv/issues/850) 修订讨论中明确要求将这两条提升为 Basic Memory 架构约定。规则已确认不代表 #850 的代码已实现。

### 组件数据流

```mermaid
flowchart TD
    A["src/metahook.cpp MH_LoadEngine"] --> B["GameData::Initialize(gamedataRoot)"]
    B --> C["Read + validate index.json (schema v4)"]
    C --> D["Per version entry: LoadSnapshot"]
    D --> E["Verify size + SHA-256 (SHA2_256)"]
    E --> F["Parse snapshot JSON (dataset v5 / source v8 / analysis v3)"]
    F --> G["Normalize 'windows' records -> GameSymbolRecord"]
    G --> H["Build crc64 -> ModuleCatalog -> symbolName map"]
    H --> I["Freeze catalog, available=true"]

    J["MH_GetModuleCRC64(moduleBase)"] --> K["GetOrCreateModuleIdentity"]
    K --> L{"source type? PeFile / BlobFile / None"}
    L -->|"PeFile / BlobFile"| M["ComputeCrc64FromFile (CRC_64_XZ) + cache Ready/Failed"]
    L -->|"None"| N["MH_GAMESYMBOL_MODULE_PATH_UNAVAILABLE"]

    O["MH_QueryGameSymbolByCRC64(crc64, name)"] --> P["QueryByCRC64"]
    P --> Q["Resolve ModuleCatalog by crc64, then symbolName"]
    Q --> R["FillSymbol -> mh_gamesymbol_t"]
    R --> S["MH_GAMESYMBOL_OK"]

    T["MH_ResolveGameSymbol(moduleBase, name, kind)"] --> U["QueryGameSymbol + GetModuleCRC64"]
    U --> V{"kind match? rva in bounds? no overflow?"}
    V -->|"yes"| W["return moduleBase + rva"]
    V -->|"no"| X["KIND_MISMATCH / RVA_OUT_OF_RANGE"]
```

## Dependencies
- `thirdparty/rapidjson`（submodule）— 解析 index.json 与 snapshot JSON。
- `thirdparty/Chocobo1Hash`（submodule，gitlink `f455b0e`）— `Chocobo1::CRC_64_XZ`（模块身份哈希，refl poly `0xC96C5795D7870F42`）与 `Chocobo1::SHA2_256`（snapshot 完整性校验）。需在 `<windows.h>` 之前包含以免 `min/max` 宏污染。
- Windows API — `GetModuleFileNameW` / `CreateFileW` / `_wstat64` 用于普通 PE 来源发现与文件流式哈希；loader lock 判定 `MH_IsInLdrCriticalRegion`。
- 数据资源 — 打包 `Build\svencoop\metahook\gamedata\`（gitignore），运行时 `<game>\<mod>\metahook\gamedata\`。
## Notes

- catalog 构建后冻结，不重载；返回给插件的 `signature.text` / `bytes` / `mask` / `legacyPattern` 指向稳定 heap-owned 存储，有效至进程退出。
- 模块哈希懒计算且缓存：首个查询把 `Uncomputed -> Computing` 并在锁外做文件 I/O，后续线程在 cv 上等待；读取期间文件变化（size/mtime）返回 `MODULE_HASH_FAILED`。哈希 I/O 不持有全局 catalog 锁。
- loader 临界区内只用无锁 `g_pendingModuleInvalidations[64]` 与 `g_resetModuleIdentitiesPending` 排队失效，溢出时保守全量失效，并在下一个安全查询点 `DrainPendingModuleInvalidations`；`MH_LoadEngine` 开头的 `GameData::ResetModuleIdentities` 是每 engine session 的兜底。要求 lock-free 原子（`static_assert ATOMIC_POINTER_LOCK_FREE == 2`）。
- signature 只接受空格分隔的两位十六进制字节与 `??` wildcard；`bytes + mask` 为权威无损表示（`mask[i]==0` 通配），`legacyPattern` 仅在签名不含字面量 `0x2A` 字节时生成，否则为 `NULL`。
- 符号名查找区分大小写；键为 `(moduleCRC64, symbolName)`。完全相同的重复记录去重，内容冲突标记 `CATALOG_CONFLICT`；未类型化 kind 标记 `unsupportedKind` 查询返回 `UNSUPPORTED_KIND`。
- `patch` 记录只消费 `patch_rva`（`symbolSize`/`signatureRva`/指令字段均为 0），不按 signature 搜索内存，也不改写长度；`MH_ResolveGameSymbol` 现在接受 `FUNCTION`/`GLOBAL`/`PATCH` 三种 expected kind。
- `scalar` 记录只保留 uint32 `scalar_value`，地址字段全 0；`QueryGameSymbolScalar`（`MH_QueryGameSymbolScalar` / `QueryScalarByCRC64`）返回 `KIND_MISMATCH` 当名字对应非 scalar kind，`ResolveGameSymbol` 因 `expectedKind` 只能为 `FUNCTION`/`GLOBAL`/`PATCH`/`VIRTUAL_FUNCTION` 而拒绝 scalar。
- `virtualFunction` 记录（API 112）由 `NormalizeVirtualFunction` 规范化为 `MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION`：`rva = func_rva`、`symbolSize = func_size`、signature 取 `vfunc_sig`；`vfunc_index` / `vtable_name` 仅校验不入库。`ResolveGameSymbol` 接受该 kind 并返回 `moduleBase + rva`，语义等同 FUNCTION；无需新增函数槽位。`vtable` 记录仍为 unsupported kind。经 `QueryGameSymbol` 查询 scalar 时 `kind == MH_GAMESYMBOL_KIND_SCALAR`、地址字段全 0。
- `IsGameSymbolAvailable` 是纯存在性查询（`moduleBase` + 精确名字），不返回地址、不做 expected kind 检查；`OK`/`SYMBOL_NOT_FOUND` 之外的失败保留原状态码。连续编号的 call-site 由调用方自行拼接名字并循环探测，API 不做枚举。
- `cbSize` 契约：调用方先置 `cbSize = sizeof(mh_gamesymbol_t)`，过小返回 `OUTPUT_TOO_SMALL`；失败时输出字段清零但保留 `cbSize`。
- `ResolveGameSymbol` 只接受 `FUNCTION`/`GLOBAL`，并做 rva 与 `rva + symbolSize` 的溢出和映像边界检查；不校验内存 signature，也不做跨版本扫描。
- 迁移历史（规则见上文“架构约定”）：ResourceReplacer（计划见 `docs/plans/resource-replacer-gamedata-migration-plan.md`，2026-09-06 已实施——4 符号 gamedata-only、旧扫描全删、`COMMON_REQUIRED` 门禁已纳入，详见 [[resource-replacer-privatevars]]）；HeapPatch（2026-09-07 已实施——`Sys_InitMemory` gamedata-only、旧扫描全删、`COMMON_REQUIRED` 门禁已纳入，函数体内 heap-limit immediate 仍走有界 disasm，详见 [[heap-patch-privatevars]]）；launcher `src/metahook.cpp`（issue #850，2026-09-08 已实施——引擎族改为 catalog gameVersion → 前缀/版本阈值规则，`Cvar_DirectSet`、`cvar_hooks`、`Cvar_Set_to_Cvar_DirectSet_callsite_N`、`NLoadBlob`、`FreeBlob` 全部 gamedata-only，旧字符串/反查/pattern 扫描与 `MH_DisasmRanges` 定位全部删除，详见 [[metahook-privatevars]]）；HeapPatch / ResourceReplacer / PrecacheManager 三个插件（issue #853，2026-09-09 已实施——`Sys_InitMemory_HeapLimitPatches_N`、`S_LoadSound_to_FS_Open_callsite_N`、`Mod_LoadModel_to_FS_Open_callsite_N` 与 `cl_resourcesonhand` 全部 gamedata-only；`DisasmRanges` 控制流遍历、`"rb"` / `#GameUI_PrecachingResources` 字符串搜索、push pattern、`FindFSOpenCallSites`、`ConvertDllInfoSpace` 与镜像空间准备全部删除，详见 [[heap-patch-privatevars]] [[resource-replacer-privatevars]] [[precache-manager-privatevars]]）；ThreadGuard / SCModelDownloader 两个插件（issue #855，2026-09-09 已实施——ThreadGuard 的 `engine` GLOBAL 与 SCModelDownloader 的 `R_StudioDrawPlayer` / `studioapi_SetupPlayerModel` / `Host_IsSinglePlayerGame` FUNCTION、`DM_PlayerState` / `cl_players_model` GLOBAL 全部 gamedata-only；SCModelDownloader 改为 inline hook 两个 caller 并重建触发谓词，`R_StudioChangePlayerModel` FUNCTION 与 call-site PATCH 依赖、签名搜索、CFG 遍历、`ConvertDllInfoSpace` 与镜像空间准备全部删除，详见 [[threadguard-privatevars]] [[scmodeldownloader-privatevars]]）。仅对 gamedata 尚未提供且功能本体需要的指令信息保留相应操作；不能将 call-site 一概视为 gamedata 无法表达，已提供地址的符号必须直接 Resolve。PATCH 记录语义是目标**指令地址**：需要立即数位置时在真实地址上单条反汇编取 `encoding.imm_offset`，不新增立即数地址符号，也不按 `patch_sig` 重新定位。
- 上游契约升级（2026-09-08，schema 7 / dataset schema 4）：每个 module/platform 增加必需布尔 `isBlob`（仅通过完整 Metahook blob 解密/重建/校验的 Windows 二进制为 true；非 Windows 必须 false），并移除旧 `path`。MetaHook 只消费 `binaries.*.windows.crc64`，不读取 `isBlob`（遵循上文“信任上游”约定）。
- 上游契约升级（2026-09-12，dataset schema 5 / source snapshot contract 8 / analysis output contract 3，issue #865 前置）：新增 `kind: scalar`（payload 仅 `scalar_name` + uint32 `scalar_value`）。`src/GameData.cpp` 常量切到 `kSnapshotSchemaVersion 5` / `kSnapshotContractVersion 8` / `kAnalysisOutputContractVersion 3`，新增 `NormalizeScalar` 与公共 `QueryGameSymbolScalar`（API 111）；不再接受 schema 4 / contract 7 数据（逐 snapshot 记为 `unsupported schemaVersion`）。`scripts/sync-gamedata.py` 的 `SUPPORTED_SNAPSHOT_SCHEMA_VERSION 5` / `SUPPORTED_SNAPSHOT_CONTRACT_VERSION 8` / `SUPPORTED_ANALYSIS_OUTPUT_CONTRACT_VERSION 3`、`scripts/validate-gamedata.py` 的 `REQUIRED_SCALARS`（`size_of_frame` 属于 engine module，10 个受支持 engine 版本均覆盖）与 `isBlob` 布尔校验同步升级；行为测试见 `scripts/tests/test_gamedata_contract.py`。`gv_sig_allow_across_function_boundary` 现已在 `NormalizeGlobal` 中解析进 `flags`。`mh_gamesymbol_t` 布局与 `MetahookAPIVersion` 旧槽位不变；scalar 只经新槽位（第 8 个函数槽）获取，避免破坏旧插件 ABI。index 仍为 schema 4。
- 上游契约升级（2026-09-12，#865 消费端）：新增 `kind: virtualFunction` 记录（payload：`func_rva`/`func_size`/`vfunc_sig`/`vfunc_index`/`vtable_name`），供 client `CGameStudioRenderer` 等虚函数按名解析。`src/GameData.cpp` 新增 `NormalizeVirtualFunction`（kind = `MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION`，API 112），`MH_ResolveGameSymbol` 接受该 kind；不新增函数槽位。`scripts/validate-gamedata.py` 解析并校验 virtualFunction，新增 BulletPhysics 消费者门禁（engine 侧 function/global 每个 engine family 必需；client 侧 global 仅对 10 个发布了 client module 的 gameVersion 必需，6 个仅有 engine 产物的旧版本不强制）。`vtable` 记录仍为 unsupported kind。
- 消费端 required/optional 拆分（2026-09-12）：`Plugins/BulletPhysics` 的解析 helper 改为 `GamedataResolvePtr(..., bool required)` / `GamedataResolveScalar(..., bool required)`，`required=false` 时缺失返回 `nullptr`/`0` 而不 `Sys_Error`。validator 侧同步为 `BULLETPHYSICS_CLIENT_OPTIONAL_VFUNCS`（`GameStudioRenderer_StudioDrawModel`/`_StudioDrawPlayer`/`_StudioSetupBones`）：记录存在则校验 `kind`，缺失不报错；`g_pGameStudioRenderer` global 因在插件内无任何读取点（只写过一次、从未读）已从插件与 validator 必备列表中移除。
- 发布数据状态（2026-09-09 同步，release `v20260909c` / source `ecac7773`）：`scripts/validate-gamedata.py` 门禁通过（16 snapshots / 5 engine families），cvar 分支、blob 客户端符号与 #855 的 6 个符号已补齐；门禁按 kind 校验的 `COMMON_REQUIRED` 现含 `cl_resourcesonhand` global 与 `R_StudioDrawPlayer` / `studioapi_SetupPlayerModel` / `Host_IsSinglePlayerGame` function、`DM_PlayerState` / `cl_players_model` / `engine` global，`NUMBERED_PATCH_SETS` 为 `Sys_InitMemory_HeapLimitPatches`、`S_LoadSound_to_FS_Open_callsite`、`Mod_LoadModel_to_FS_Open_callsite`（从 `_0` 起连续、kind 必须为 patch）；含 windows 记录的为 cof-5936（46）、hl-10210（80）、hl-8684（80）、hl-6153（43）、hl-4554/hl-3647/hl-3329/hl-3266/hl-3248（各 44）、svencoop-10257（75），cstrike/czero/czeror 各版本仍为 0 记录空壳。发布流程注意：`release-build.yml` 的 `publish-release` 曾在创建 tag 时返回 403 `Resource not accessible by integration`（run 34360429634，临时性；重跑 run 34363903670 后成功），发布未完成时 `hlnd2t.github.io` 的 index 不会更新，`sync-gamedata.py` 也就拉不到新数据。
- `Build\svencoop\metahook\gamedata\` 被 gitignore，由 Pre-build 的 `sync-gamedata.py` 通过同卷 staging + 事务式目录交换生成；commit `6b8f6f65 "remove gamedata."` 删除了原先跟踪的 JSON 载荷。

## Callers

- `src/metahook.cpp` — `MH_LoadEngine_ResolveSymbol`（`MH_ResolveGameSymbol` / `MH_GetModuleCRC64` / `MH_GetGameSymbolStatusString`）、`MH_LoadEngine_ResolveGlobalOperand`（`MH_QueryGameSymbol`）、`MH_LoadEngine_DetermineEngineType`（`MH_GetModuleCRC64` + `GameData::GetGameVersion`）、`MH_LoadEngine_PatchCvarCallbacks` / `MH_LoadEngine_FindLoadBlobClient`（`MH_IsGameSymbolAvailable`）、`MH_LoadEngine`（`GameData::Initialize` / `RegisterModuleFileSource` / `RegisterMirrorAlias` / `ResetModuleIdentities`）。
- `src/LoadDllNotification.cpp` — 加/卸载通知中 `GameData::InvalidateModule(ctx.ImageBase, inCritRegion)`。
- V3/V4 插件 — 通过 `metahook_api_t` API 109 的 6 个函数槽调用；API 110 起可用 `IsGameSymbolAvailable`（第 7 槽）；API 111 起可用 `QueryGameSymbolScalar`（第 8 槽）。

Related: [[metahook-privatevars]] [[project-overview]] [[plugin-system]]

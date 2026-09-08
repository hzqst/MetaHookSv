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
- crc64
---

# GameData

## Overview

GameData 是 launcher 与 V3/V4 插件共享的本地游戏符号目录（catalog）组件：从 `<game>\<mod>\metahook\gamedata\index.json` 读取并冻结一份只读符号表，按 `(moduleCRC64, symbolName)` 查询符号元数据，并通过 `moduleBase` 懒计算模块原始文件的 CRC-64/XZ。公共接口以 MetaHook API 109 暴露（`metahook_api_t` 尾部的 6 个函数槽），API 110 追加 `MH_GAMESYMBOL_KIND_PATCH` 与 `IsGameSymbolAvailable` 槽位（不改动旧槽位偏移）。

## Responsibilities

- 构建并冻结只读 catalog（`GameData::Initialize`），初始化后不可重载，所有返回指针在进程退出前有效。
- 校验 index（schema v4）与每个 snapshot（schema v4 / `source.snapshotSchemaVersion` v7）的路径安全、size、SHA-256（`Chocobo1::SHA2_256`）。
- 只提取 `platform == "windows"` 的记录，将 `function` / `global` / `patch` payload 规范化为 `GameSymbolRecord`；单 snapshot 失败隔离为 diagnostics，不破坏整个 catalog。`patch` 记录正规化为 `MH_GAMESYMBOL_KIND_PATCH`，地址取 `patch_rva`，不把 signature 当函数长度。
- 将 signature 文本编译为 `(bytes, mask, legacyPattern)` 三态。
- 按 `moduleBase` 管理 `ModuleIdentity`（普通 PE / Blob 文件 / None），懒计算并缓存模块 CRC-64/XZ，处理 mirror alias 与 unload 失效。
- 实现公共 API：`MH_GetModuleCRC64`、`MH_QueryGameSymbol`、`MH_QueryGameSymbolByCRC64`、`MH_ResolveGameSymbol`、`MH_SearchPatternMasked`、`MH_GetGameSymbolStatusString`，以及 API 110 的 `MH_IsGameSymbolAvailable`（仅返回状态码：存在 `OK`、不存在 `SYMBOL_NOT_FOUND`、其它失败保留原状态；不返回地址）。
- 提供 launcher 内部 getter `GameData::GetGameVersion(moduleCRC64, &gameVersion)`（不进入 `metahook_api_t`），用于按引擎模块 CRC 反查 catalog 中的 gameVersion。
- 提供 `GameData::RegisterModuleFileSource`（Blob engine）与 `RegisterMirrorAlias` 供 launcher 注册模块来源。

## Involved Files & Symbols

- `src/GameData.h` — `GameData` namespace 声明（`Initialize`/`QueryByCRC64`/`GetGameVersion`/`GetModuleCRC64`/`RegisterModuleFileSource`/`RegisterMirrorAlias`/`InvalidateModule`/`ResetModuleIdentities`）与公共 `MH_*` 入口。
- `src/GameData.cpp` — 实现 `GameDataCatalog` 构建（`Initialize`/`LoadSnapshot`/`ValidateIndex`）、签名解析 `ParseSignature`、payload 规范化 `NormalizeFunction`/`NormalizeGlobal`/`NormalizePatch`、模块哈希状态机 `GetModuleCRC64`/`ComputeCrc64FromFile`、公共 API 与 `MH_GetGameSymbolStatusString`。
- `include/metahook.h` — `METAHOOK_API_VERSION 110`、`mh_gamesymbol_kind_t`（含 `MH_GAMESYMBOL_KIND_PATCH`）/`mh_gamesymbol_status_t`/`mh_pattern_t`/`mh_gamesymbol_t`、`metahook_api_t` 新增的 7 个函数槽（第 6 个为 `IsGameSymbolAvailable`）。
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
    E --> F["Parse snapshot JSON (schema v4 / source v7)"]
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
- `IsGameSymbolAvailable` 是纯存在性查询（`moduleBase` + 精确名字），不返回地址、不做 expected kind 检查；`OK`/`SYMBOL_NOT_FOUND` 之外的失败保留原状态码。连续编号的 call-site 由调用方自行拼接名字并循环探测，API 不做枚举。
- `cbSize` 契约：调用方先置 `cbSize = sizeof(mh_gamesymbol_t)`，过小返回 `OUTPUT_TOO_SMALL`；失败时输出字段清零但保留 `cbSize`。
- `ResolveGameSymbol` 只接受 `FUNCTION`/`GLOBAL`，并做 rva 与 `rva + symbolSize` 的溢出和映像边界检查；不校验内存 signature，也不做跨版本扫描。
- 迁移历史（规则见上文“架构约定”）：ResourceReplacer（计划见 `docs/plans/resource-replacer-gamedata-migration-plan.md`，2026-09-06 已实施——4 符号 gamedata-only、旧扫描全删、`COMMON_REQUIRED` 门禁已纳入，详见 [[resource-replacer-privatevars]]）；HeapPatch（2026-09-07 已实施——`Sys_InitMemory` gamedata-only、旧扫描全删、`COMMON_REQUIRED` 门禁已纳入，函数体内 heap-limit immediate 仍走有界 disasm，详见 [[heap-patch-privatevars]]）；launcher `src/metahook.cpp`（issue #850，2026-09-08 已实施——引擎族改为 catalog gameVersion → 前缀/版本阈值规则，`Cvar_DirectSet`、`cvar_hooks`、`Cvar_Set_to_Cvar_DirectSet_callsite_N`、`NLoadBlob`、`FreeBlob` 全部 gamedata-only，旧字符串/反查/pattern 扫描与 `MH_DisasmRanges` 定位全部删除，详见 [[metahook-privatevars]]）。仅对 gamedata 尚未提供且功能本体需要的指令信息保留相应操作；不能将 call-site 一概视为 gamedata 无法表达，已提供地址的符号必须直接 Resolve。
- 上游契约升级（2026-09-08，schema 7 / dataset schema 4）：每个 module/platform 增加必需布尔 `isBlob`（仅通过完整 Metahook blob 解密/重建/校验的 Windows 二进制为 true；非 Windows 必须 false），并移除旧 `path`。MetaHook 只消费 `binaries.*.windows.crc64`，不读取 `isBlob`（遵循上文“信任上游”约定）。
- 发布数据状态（2026-09-08 同步）：`scripts/validate-gamedata.py` 门禁通过（16 snapshots / 5 engine families），cvar 分支与 blob 客户端符号已补齐；含 windows 记录的为 cof-5936（31）、hl-10210（58）、hl-8684（59）、hl-6153/hl-4554/hl-3647/hl-3329/hl-3266/hl-3248（各 31）、svencoop-10257（48），cstrike/czero/czeror 各版本仍为 0 记录空壳。
- `Build\svencoop\metahook\gamedata\` 被 gitignore，由 Pre-build 的 `sync-gamedata.py` 通过同卷 staging + 事务式目录交换生成；commit `6b8f6f65 "remove gamedata."` 删除了原先跟踪的 JSON 载荷。

## Callers

- `src/metahook.cpp` — `MH_LoadEngine_ResolveSymbol`（`MH_ResolveGameSymbol` / `MH_GetModuleCRC64` / `MH_GetGameSymbolStatusString`）、`MH_LoadEngine_ResolveGlobalOperand`（`MH_QueryGameSymbol`）、`MH_LoadEngine_DetermineEngineType`（`MH_GetModuleCRC64` + `GameData::GetGameVersion`）、`MH_LoadEngine_PatchCvarCallbacks` / `MH_LoadEngine_FindLoadBlobClient`（`MH_IsGameSymbolAvailable`）、`MH_LoadEngine`（`GameData::Initialize` / `RegisterModuleFileSource` / `RegisterMirrorAlias` / `ResetModuleIdentities`）。
- `src/LoadDllNotification.cpp` — 加/卸载通知中 `GameData::InvalidateModule(ctx.ImageBase, inCritRegion)`。
- V3/V4 插件 — 通过 `metahook_api_t` API 109 的 6 个函数槽调用；API 110 起可用 `IsGameSymbolAvailable`（第 7 槽）。

Related: [[metahook-privatevars]] [[project-overview]] [[plugin-system]]

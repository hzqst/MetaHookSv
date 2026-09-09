# ResourceReplacer gamedata 迁移计划

## 文档状态

- 状态：已实施（2026-09-06，Step 1-6 完成；实机冒烟与失败路径验证待 Sven Co-op 环境执行）
- 最后更新：2026-09-09
- 后续变更：第 1 节目标 2/3 与第 107 行起的 `FindFSOpenCallSites` 走查已被 [issue #853](https://github.com/hzqst/MetaHookSv/issues/853) 取代——上游现已提供 `S_LoadSound_to_FS_Open_callsite_N` / `Mod_LoadModel_to_FS_Open_callsite_N` PATCH 记录，走查、`"rb"` 搜索与交叉校验全部删除。本文件仅保留为当时的实施记录。
- 目标仓库：`D:\MetaHookSv`
- 数据基线：2026-09-06 从 `https://hlnd2t.github.io/GoldSrc_VibeSignatures/gamesymbols/index.json` 同步的 gamedata（16 snapshot）
- 参考仓库（本地检出版）：
    - `D:\GoldSrc_VibeSignatures` — gamedata 生成仓库（符号权威数据源）
    - `D:\HLND2T_official` — GoldSrc 引擎源码仓库（函数签名与调用关系权威参考）
- 关联知识：memory 笔记 `metahooksv/privatevars/resource-replacer-privatevars`、`metahooksv/game-data`
- 实现参考：`src/metahook.cpp` 的 `MH_LoadEngine_ResolveSymbol`（失败即 fatal 的语义与本计划一致，错误诊断格式可直接借鉴）

> **通用原则（适用于后续所有插件的 gamedata 移植计划）**：凡 gamedata 已能准确定位的符号，一律 gamedata-only，**不保留** signature / 字符串 / 反查等 fallback routine；数据缺口由 `validate-gamedata.py` 构建期门禁与上游补数解决，而不是由运行时双路径兜底。

## 1. 背景与目标

`Plugins/ResourceReplacer/privatehook.cpp` 当前用字符串引用、机器码签名、函数头反查和有界 capstone 走查定位 4 个引擎私有函数：`S_LoadSound`、`Mod_LoadModel`、`FS_Open`、`CL_PrecacheResources`。

2026-09-06 同步后的上游 gamedata 已在所有非空 snapshot 中收录这 4 个符号的 windows 记录（`module: "engine"`，含 `func_rva` / `func_size` / `func_sig`，`binaries.engine.windows.crc64` 齐全）。svencoop-10257 基线：

| 符号 | RVA | size | 备注 |
| --- | --- | --- | --- |
| `S_LoadSound` | `0x99080` | `0x393` | windows sig 与现有 `S_LOADSOUND_SIG_SVENGINE` 吻合 |
| `Mod_LoadModel` | `0x51be0` | `0x46c` | |
| `FS_Open` | `0x4e8d0` | `0x16` | filesystem 虚调用 thunk |
| `CL_PrecacheResources` | `0x26540` | `0x376` | |

本计划目标：

1. 4 个函数入口全部经 `g_pMetaHookAPI->ResolveGameSymbol`（MetaHook API 109）解析，gamedata-only；查询失败即 `Sys_Error`，**删除**全部既有定位 fallback（含 `#GameUI_PrecachingResources` 字符串 + `ReverseSearchFunctionBegin`、各引擎分支 prologue 签名、字符串 push 模式反查）。
2. call-site 发现保留有界 capstone 走查——gamedata 符号模型（function rva/size/sig + global 指令元数据）无法表达函数内部的 `call FS_Open` 指令地址，而 `Engine_InstallHooks` 重定向的正是这些 call-site。走查是功能本体，不是 fallback。
3. 走查根地址与 `FS_Open` 均以 gamedata 为权威，disasm 恢复的 call 目标降级为交叉校验。

## 2. 非目标

- 不改变 hook 语义：call-site 重定向（`InlinePatchRedirectBranch`）与 `CL_PrecacheResources` inline hook 的行为、安装时机均不变。
- 不修改 GameData 组件、`metahook_api_t` 或公共头文件。
- 不做运行时入口 signature 强校验（仅列为可选增强，见 6.2）。
- 不为 gamedata 未收录的引擎（cstrike / czero / czeror 空 snapshot、GoldSrc 8308 等无数据版本）保留扫描 fallback。这些引擎不在 `validate-gamedata.py` 的 `ENGINE_FAMILIES` 声明支持范围内，gamedata 查询失败将以明确的 `Sys_Error`（含 CRC64 与状态码）终止——与 launcher 在无数据引擎上的行为一致。
- 不迁移 ResourceReplacer 之外的插件（本插件将是首个消费 API 109 gamedata 槽位的插件；后续插件迁移遵循文档头通用原则）。

## 3. 硬性设计决策

### 3.1 gamedata-only 与失败语义

- 每个符号仅经 `ResolveGameSymbol` 解析，失败即终止：保持现有 `Sig_FuncNotFound` 语义，错误信息对齐 `src/metahook.cpp:1591` `MH_LoadEngine_ResolveSymbol` 的诊断格式（符号名、模块路径、CRC64、`MH_GetGameSymbolStatusString` 状态），便于用户上报后直接定位到上游数据问题。
- 不存在 fallback 分支：`Engine_FillAddress_*` 中各符号的字符串搜索、`Search_Pattern` 签名、`ReverseSearchFunctionBegin[Ex]` 反查代码全部删除。
- 数据防线前移到构建期：`COMMON_REQUIRED` 门禁（3.6）是防止上游 snapshot 丢符号导致发布版运行时 fatal 的唯一防线，因此为必做项而非可选项。

### 3.2 地址空间与 mirror

- `ResolveGameSymbol` 一律传**真实引擎模块 base**（`g_EngineDLLInfo.ImageBase`）。launcher 已在 `src/metahook.cpp:1769` 注册 mirror alias（`RegisterMirrorAlias(MH_GetMirrorDLLBase(...), g_dwEngineBase)`），CRC 身份解析不受 mirror 影响。
- gamedata 返回值即真实镜像 VA，直接写入 `gPrivateFuncs`，不需要 `ConvertDllInfoSpace`。
- call-site 走查仍在"搜索空间"（mirror 存在时为 `g_MirrorEngineDLLInfo`，否则真实镜像）进行，理由与现状相同：SvEngine 场景下代码扫描在 mirror 副本上做。
- 走查根地址需要从真实 VA 映射到搜索空间：`ConvertDllInfoSpace(realVA, RealDllInfo, SearchDllInfo)`。现有 `ConvertDllInfoSpace(addr, Src, Target)` 本身就是通用 RVA 映射，直接换参复用，不新增函数。mirror 不存在时两个 DllInfo 相同，映射天然退化为恒等。
- call-site 结果仍按现状 `ConvertDllInfoSpace(addr, Search, Real)` 转回真实镜像后交给 `InlinePatchRedirectBranch`。

### 3.3 `FS_Open` 权威化

- `gPrivateFuncs.FS_Open` 直接取 gamedata 地址；走查中恢复的 call 目标**不用于赋值**，仅与 gamedata 地址做一致性比对（不一致打告警，不 fatal、不阻断）。
- 删除现有 `if (!gPrivateFuncs.FS_Open)` 的"首个合格 call 目标恢复"逻辑。
- 两个入口函数（`S_LoadSound` / `Mod_LoadModel`）的走查各自产生 call-site、任一为空即 `Sys_Error` 的现状保持不变。

### 3.4 走查边界

- 保持现有界：首 walk `0x1000`、分支 walk `0x300`、`max_insts = 1000`、`max_depth = 16`。
- 可选增强（P2，非必需）：用 gamedata 的 `symbolSize` 把线性 walk 上限收紧为 `min(0x1000, funcSize)`。首期不做，避免行为漂移。

### 3.5 API 版本

- 插件与宿主同仓库同批构建发布，`METAHOOK_API_VERSION >= 109` 由编译期 `static_assert` 保证；不做运行时版本探测。

### 3.6 发布门禁

- `scripts/validate-gamedata.py` 的 `COMMON_REQUIRED`（第 26 行列表）追加 4 个符号。当前所有声明引擎族（`ENGINE_FAMILIES`：svencoop-10257、hl-10210、hl-8684/6153、hl-3248~4554、cof-5936）的非空 snapshot 均已包含它们，不会引入新的门禁失败；上游未来丢符号时在构建期暴露，而不是运行时 fatal。cstrike/czero/czeror 不在 `ENGINE_FAMILIES` 声明内，不受影响。

## 4. 实施步骤

### Step 1：辅助函数（`privatehook.cpp`）

新增一个静态辅助：

```cpp
// gamedata 解析失败时打印诊断（符号名 / buildnum / CRC64 / 状态串）并 Sys_Error 终止。
// 返回值即真实镜像 VA，直接写入 gPrivateFuncs 对应字段。
static PVOID ResolveGameSymbolOrError(const char* symbolName);
```

内部封装 `g_pMetaHookAPI->ResolveGameSymbol(g_EngineDLLInfo.ImageBase, symbolName, MH_GAMESYMBOL_KIND_FUNCTION, &va)`，失败诊断格式对齐 `MH_LoadEngine_ResolveSymbol`。

### Step 2：改造 `Engine_FillAddress_S_LoadSound` / `Engine_FillAddress_Mod_LoadModel`

两个函数改造方式相同：

1. 入口定位：`ResolveGameSymbolOrError("S_LoadSound" / "Mod_LoadModel")`，写入 `gPrivateFuncs`。
2. **删除**既有定位路径：`S_LoadSound: Couldn't load %s` / `Mod_LoadModel: Could not load` / `Mod_NumForName: %s not found` 字符串搜索、`push; call; add esp` 模式匹配、`ReverseSearchFunctionBeginEx` 反查、`S_LOADSOUND_SIG_*` 全部五套引擎分支 prologue 签名（`privatehook.cpp:12-16` 宏定义一并删除）。
3. 走查根地址：`rootVA = ConvertDllInfoSpace(入口VA, RealDllInfo, SearchDllInfo)`（mirror 不存在时即原值）。
4. 走查逻辑改调 Step 3 提取的共享函数；`FS_Open` 按 3.3 权威化。

### Step 3：提取共享 call-site 走查（改造点，需确认）

`S_LoadSound` 与 `Mod_LoadModel` 的 `SearchContext` 结构体 + `DisasmRanges` 回调目前是约 90% 相同的复制粘贴（`privatehook.cpp:154-265` 与 `privatehook.cpp:335-447`），本次改造将两者提取为一个共享函数：

```cpp
static void FindFSOpenCallSites(PVOID rootVA, const mh_dll_info_t& SearchDllInfo,
                                const mh_dll_info_t& RealDllInfo,
                                PVOID fsOpenRealVA, size_t callWindowBytes,
                                std::set<PVOID>& outCallSites);
```

提取时统一两处现存差异（均为无害统一）：

- `"rb"` 匹配长度：`S_LoadSound` 走查用 `sizeof("rb") - 1`（2 字节），`Mod_LoadModel` 走查用 `sizeof("rb")`（3 字节，多比一个 NUL）。统一为 `sizeof("rb") - 1`。
- call 距离窗口：`0x30`（S_LoadSound）与 `0x50`（Mod_LoadModel）参数化为 `callWindowBytes`，保留各自现值，不改变行为。

（此为"改造现有方法以复用"：如不允许提取，两处各自独立改造亦可，代价是双份维护。）

### Step 4：改造 `Engine_FillAddress_CL_PrecacheResources`

改为 `gPrivateFuncs.CL_PrecacheResources = ResolveGameSymbolOrError("CL_PrecacheResources")`；**删除** `#GameUI_PrecachingResources` 字符串搜索、`push <string>; call` 模式匹配与 `ReverseSearchFunctionBegin` 反查。inline hook 安装逻辑不变。

### Step 5：编译期断言与门禁

- `privatehook.cpp` 增加 `static_assert(METAHOOK_API_VERSION >= 109, ...)`。
- `scripts/validate-gamedata.py` 的 `COMMON_REQUIRED` 追加 4 个符号（必做，见 3.1）。

### Step 6：文档与知识同步

- 检查 `docs/ResourceReplacer.md` / `ResourceReplacerCN.md` 是否描述符号定位机制，如有则同步更新（已核实当前未提及，预期无需改动）。
- 更新 memory 笔记 `metahooksv/privatevars/resource-replacer-privatevars` 的 Resolution mechanism 表（gamedata-only）。

## 5. 验证计划（Level 1 回归）

1. `Release|Win32` 构建通过（`ResourceReplacer.vcxproj`）。
2. svencoop-10257 实机冒烟：启动进图，`.gmr` / `.gsr` 地图替换列表生效；4 个符号均走 gamedata 命中，`FS_Open` 交叉校验无告警。
3. 失败路径验证（一次性，开发期）：临时改错 symbolName 或移走 gamedata 目录启动，确认 `Sys_Error` 信息包含符号名、engine buildnum、CRC64 与状态串，可据此定位上游数据问题；随后还原。
4. 一致性对拍（一次性，开发期）：在 SvEngine 上打印 gamedata 地址与改造前旧扫描地址，应逐字节相等（基线见第 1 节 RVA 表；`S_LOADSOUND_SIG_SVENGINE` 与上游 windows sig 已互相印证；若不符，以 `D:\GoldSrc_VibeSignatures` 的 `bin_artifacts` YAML 为权威复核，见 8.1）。

验收标准：gamedata 命中时 hook 行为与改造前一致；定位相关扫描代码全部移除后 `privatehook.cpp` 无残留死代码（`Search_Pattern_Data` / `Search_Pattern_Rdata` / `ReverseSearchFunctionBegin*` 引用清零）。

## 6. 风险与权衡

### 6.1 已识别风险

- **引擎覆盖收窄（有意决策）**：cstrike / czero / czeror（空 snapshot）、无 gamedata 目录的环境、未来新引擎版本在数据入库前，插件将以 `Sys_Error` 终止而非回退扫描。这是与 launcher 一致、且经确认接受的行为；声明支持范围以 `ENGINE_FAMILIES` 为准。
- mirror 空间映射是本次唯一的新地址变换（真实 → 搜索）。SvEngine mirror 场景必须实测（验证计划第 2 项覆盖）。映射错误的表现是走查找不到 call-site，会以现有 `Sys_Error("*.FS_Open not found")` 显式失败，不会静默错 hook。
- 上游 RVA 错位：gamedata 错误地址会导致 hook 装到错误位置。缓解：4 符号与现有签名/字符串定位互相印证过（svencoop-10257）；`FS_Open` 交叉校验提供运行时旁证；可选增强见 6.2。

### 6.2 可选增强（P2，首期不做）

- 入口 signature 校验：`QueryGameSymbol` 取 `signature.bytes/mask`，对解析出的入口做 `memcmp` 校验，不一致视为数据错误并 `Sys_Error`（无 fallback 可降级）。
- 用 `symbolSize` 收紧走查线性上限（见 3.4）。

### 6.3 长期策略

- 单一 gamedata 路径，无双路径维护成本。后续插件需要新私有符号时，流程为：先在 `D:\GoldSrc_VibeSignatures` 补符号数据并发布 → `COMMON_REQUIRED` 门禁跟进 → 再写消费代码；数据先行，代码不留 fallback。

## 7. 涉及文件与工作量

| 文件 | 改动 |
| --- | --- |
| `Plugins/ResourceReplacer/privatehook.cpp` | 主要改造：辅助函数、3 个 FillAddress 函数 gamedata 化并删除全部定位扫描、共享走查提取、`S_LOADSOUND_SIG_*` 宏删除 |
| `Plugins/ResourceReplacer/privatehook.h` | 共享走查函数声明（如需对外） |
| `scripts/validate-gamedata.py` | `COMMON_REQUIRED` +4 符号（必做） |
| memory `metahooksv/privatevars/resource-replacer-privatevars` | Resolution mechanism 更新 |

工作量评估：小。单插件文件改造 + 门禁列表一行，预计一次提交完成；删除 4 套定位扫描后 `privatehook.cpp` 净代码量显著下降（共享走查提取进一步收敛）。

## 8. 参考来源与用途

### 8.1 `D:\GoldSrc_VibeSignatures` — gamedata 生成仓库

基于 IDA 的可复现游戏二进制分析框架，是本计划所消费 gamedata 的权威数据源。每符号 YAML 位于 `bin_artifacts/<gamever>/<module>/`，与运行时 snapshot 中记录的 `id` 字段一一对应（例如 snapshot 记录 `engine/S_LoadSound.windows.yaml` 对应 `bin_artifacts/svencoop-10257/engine/S_LoadSound.windows.yaml`）。

本计划中的用途：

- **RVA 对拍基线核验**：第 5 节验证计划第 4 项的对拍基线（第 1 节 RVA 表）如与实机不符，以 `bin_artifacts/<gamever>/engine/*.windows.yaml` 为权威数据复核，排除本地 gamedata 同步滞后的可能。
- **上游数据修正入口**：若发现 4 个符号中任一记录错位（见 6.1 风险），修正应在生成仓库侧进行后重新发布，而不是在 MetaHookSv 侧打补丁。相关流程文档：`docs/en/snapshot-and-gamedata.md`（snapshot 与 gamedata 发布）、`docs/en/generator-contract.md`（生成器契约）。
- **数据补齐工作流入口**：cstrike / czero / czeror 空 snapshot、GoldSrc 8304/8308 及其他未收录引擎版本的补符号工作，沿用该仓库 `ida_analyze_bin.py` 分析工作流（配置见 `configs/`）。在 `COMMON_REQUIRED` 纳入 4 符号（3.6）后，补齐的门禁信号由 MetaHookSv 侧的 `validate-gamedata.py` 给出。这也是 6.3 长期策略"数据先行"的执行仓库。
- **mirror / blob 背景参考**：`decrypt_blob.py` 与 blob 引擎二进制的解密处理，是理解 SvEngine mirror 搜索空间与 `GameData::RegisterModuleFileSource` 设计背景的辅助材料。

### 8.2 `D:\HLND2T_official` — GoldSrc 引擎源码仓库

GoldSrc 引擎源码工程（CMake，含 `engine/`、`filesystem/`、`launcher/` 等目录），作为 4 个目标函数的**签名、调用关系与行为语义**的权威参考。

关键落点（已核对，与 `Plugins/ResourceReplacer/privatehook.h` 的 typedef 一致）：

| 符号 | 源码位置 | 说明 |
| --- | --- | --- |
| `S_LoadSound` | `engine/snd_mem.c:91` | `sfxcache_t *S_LoadSound(sfx_t *s, channel_t *ch)` |
| `Mod_LoadModel` | `engine/gl_model.c:215` | `model_t *Mod_LoadModel(model_t *mod, qboolean crash, qboolean trackCRC)`；注意 `engine/model.c:34` 另有两参软件渲染变体，插件 typedef 对应三参版本 |
| `CL_PrecacheResources` | `engine/cl_main.c`、`engine/cl_parse.c` | 调用侧与定义侧 |
| `FS_Open` | `filesystem/` 及各调用点 | 各处 `FS_Open(name, "rb")` 调用（`engine/cmd.c:1158`、`engine/cl_main.c:1755`、`engine/cl_parse.c:2065` 等） |

本计划中的用途：

- **走查启发式的源码依据**：disasm 走查识别的 `push "rb"; call FS_Open` 指令序列，在源码层就是上述 `FS_Open(name, "rb")` 调用点；评估 Step 3 提取共享走查时窗口参数（`0x30` / `0x50`）与 `max_insts`/`max_depth` 边界的合理性，以源码函数规模（`snd_mem.c` 的 `S_LoadSound`、`gl_model.c` 的 `Mod_LoadModel`）为准绳。
- **typedef 核对**：插件侧函数指针签名与源码声明保持一致的三参/两参辨析（如上表）。
- **hook 语义演进参考**：后续若扩展替换逻辑（如 `FS_Open` 之外的 `"wb"` 写路径、demo 相关调用点），以该仓库的调用面为分析起点。

注意：该仓库为参考用途，MetaHookSv 不依赖其构建产物；两仓库无代码级耦合。

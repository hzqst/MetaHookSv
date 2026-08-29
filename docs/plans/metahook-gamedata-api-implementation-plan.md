# MetaHook gamedata API 完整实现计划

## 文档状态

- 状态：设计已确认，待实现
- 最后更新：2026-08-29
- 目标仓库：`D:\MetaHookSv`
- gamedata 生成仓库：`D:\GoldSrc_VibeSignatures`
- 构建期 index URL：`https://hlnd2t.github.io/GoldSrc_VibeSignatures/gamesymbols/index.json`
- 运行时数据目录：`<game>\<mod>\metahook\gamedata\`
- 仓库内打包目录：`Build\svencoop\metahook\gamedata\`
- MetaHook API 目标版本：109

## 1. 背景与目标

当前 `src/metahook.cpp` 通过字符串、机器码、反汇编和交叉引用扫描定位引擎私有函数与全局变量。GoldSrc_VibeSignatures 已将这些符号导出为本地 gamedata JSON，包含模块 CRC64、符号名、RVA、signature 以及 global 地址提取元数据。

本计划的目标是实现一套由 launcher 与 V3/V4 插件共享的 MetaHook 公共 API：

1. 从本地 gamedata catalog 按模块 CRC64 与 canonical symbol name 查询符号。
2. 通过加载模块的 `moduleBase` 自动找到原始模块文件，并在首次查询时 lazy-compute CRC-64/XZ。
3. 返回稳定的 C ABI 元数据，包括 RVA、无损 signature、兼容旧扫描器的 signature，以及 global 指令元数据。
4. 提供将 RVA 安全转换为运行时 VA 的高层 resolver。
5. 以 gamedata 完全替代 launcher 当前的私有符号定位代码。
6. 保持旧插件二进制兼容，并为新 V3/V4 插件提供公共查询能力。

## 2. 非目标

首版明确不实现以下能力：

- launcher 运行时不从网络下载、更新或修复 gamedata；联网同步仅发生在 MetaHook Pre-build。
- 不扫描 `index.json` 未声明的 snapshot 文件。
- 不做未知 CRC64 的跨版本 signature 猜测。
- 不保留 Release 运行时旧扫描 fallback。
- 不支持 gamedata 热重载。
- 不允许插件注册或篡改 `moduleBase -> source` 映射。
- 不对 `patch`、`vtable`、`structMember`、`virtualFunction` 提供类型化结果。
- 不把 RapidJSON DOM 或任意 JSON key/value 暴露给插件。
- 不隐式验证已解析 RVA 对应的内存 signature。
- 不为纯内存且没有已登记来源的任意模块推断 CRC64。

## 3. 已确认的硬性设计决策

### 3.1 数据来源

- gamedata 仅从本地读取。
- `index.json` 是 catalog 的唯一入口。
- snapshot 必须由 `index.json` 声明。
- launcher 不访问 `index.json` 中的远程 URL。
- catalog 初始化后保持只读，生命周期持续到进程退出。
- MetaHook Pre-build 必须从固定 HTTPS index URL 同步 index 与其声明的全部 snapshots。
- Pre-build 只能在完整下载并校验全部文件后替换 `Build\svencoop\metahook\gamedata\`。
- 下载或校验失败时保留旧目录不变，并让 Pre-build 以非 0 exit code 失败。

### 3.2 查询策略

- canonical symbol name 区分大小写。
- 索引键为 `(moduleCRC64, symbolName)`。
- CRC64 命中后才能查询符号。
- 未知 CRC64 直接返回 `MODULE_NOT_FOUND`。
- 暂不支持跨版本 signature 扫描。
- 重复键内容完全一致时可以去重。
- 重复键内容冲突时标记为 `CATALOG_CONFLICT`。

### 3.3 符号类型

- 首版仅类型化支持 `function` 与 `global`。
- 其他合法 kind 不导致整个 snapshot 初始化失败。
- 查询到其他 kind 时返回 `UNSUPPORTED_KIND`。

### 3.4 launcher 迁移

- gamedata 是 launcher 私有符号的唯一权威来源。
- Release 不调用旧的硬编码定位器作为 fallback。
- launcher 只解析最终消费符号，不解析 intermediate symbols。
- 旧扫描器可在迁移早期作为临时 Debug 对照，但必须在最终交付前删除。
- 当前声明支持的全部引擎族都属于完成验收范围。

### 3.5 ABI 与生命周期

- 公共接口继续使用 C 风格结构体和函数指针表。
- `metahook_api_t` 只在尾部追加字段。
- `METAHOOK_API_VERSION` 从 108 升到 109。
- 返回的字符串、pattern bytes 和 mask 均由 MetaHook 持有。
- 插件只读这些指针，不释放、不修改。
- 指针有效期持续到 MetaHook 进程退出。

## 4. 外部数据契约

### 4.1 index.json

首版支持当前发布格式：

- `schemaVersion == 4`
- `versions` 为数组
- 每项至少包含：
  - `gameVersion`
  - `url`
  - `sha256`
  - `size`
- `snapshotSchemaVersion`

构建期通过固定 URL 下载 index。运行时只读取已经同步到本地目录的 `index.json`，不保留远程 URL fallback。

安全要求：

- `url` 必须是单个安全相对文件名。
- 拒绝绝对路径、盘符、UNC、`..`、目录分隔符和路径逃逸。
- `sha256` 必须是 64 个小写十六进制字符。
- `size` 必须是非负整数。
- index 缺失、JSON 非法或 schema 不支持时，整个 gamedata 标记为不可用。

### 4.2 snapshot JSON

首版支持当前 flattened snapshot 格式：

- `schemaVersion == 3`
- `source.snapshotSchemaVersion == 6`
- 顶层包含：
  - `source`
  - `binaries`
  - `modules`
  - `records`

加载顺序：

1. 检查 snapshot 文件存在。
2. 检查实际文件大小等于 index 中的 `size`。
3. 使用 `Chocobo1::SHA2_256` 校验完整文件 SHA-256。
4. SHA-256 一致后才解析 JSON。
5. 只构建 `platform == "windows"` 的运行时记录。

单个 snapshot 缺失、哈希错误、schema 不支持或内容非法时：

- 隔离该 snapshot。
- 保存内部诊断。
- 继续加载其他有效 snapshots。
- 不让一个无关版本破坏整个 catalog。

### 4.3 binary metadata

每个 Windows binary metadata 至少要求：

- `crc64`：16 个小写十六进制字符
- `size`：非负整数
- `sha256`：64 个小写十六进制字符

运行时模块身份仍以 CRC64 为公共查询键。`size` 用于数据合法性检查与碰撞诊断，不改变公共 `QueryGameSymbolByCRC64` 签名。

### 4.4 function payload

支持并规范化以下字段：

- `func_name`
- `func_rva`
- `func_sig`
- `func_size`
- `func_sig_allow_across_function_boundary`，可选

规范化结果：

- `kind = FUNCTION`
- `rva = func_rva`
- `symbolSize = func_size`
- `signatureRva = func_rva`
- `flags` 根据 `func_sig_allow_across_function_boundary` 设置

### 4.5 global payload

支持并规范化以下字段：

- `gv_name`
- `gv_rva`
- `gv_sig`
- `gv_sig_va`
- `gv_va`
- `gv_inst_offset`
- `gv_inst_disp`
- `gv_inst_length`

规范化计算：

```text
imageBase   = gv_va - gv_rva
signatureRva = gv_sig_va - imageBase
```

规范化结果：

- `kind = GLOBAL`
- `rva = gv_rva`
- `symbolSize = 0`
- `instructionOffset = gv_inst_offset`
- `operandOffset = gv_inst_disp`
- `instructionLength = gv_inst_length`

所有十六进制字段必须安全解析并检查减法、加法和 32 位范围溢出。

## 5. 公共 ABI 设计

### 5.1 头文件依赖

在 `include/metahook.h` 中显式加入：

```cpp
#include <stdint.h>
```

### 5.2 kind enum

```cpp
typedef enum mh_gamesymbol_kind_e
{
	MH_GAMESYMBOL_KIND_UNKNOWN = 0,
	MH_GAMESYMBOL_KIND_FUNCTION = 1,
	MH_GAMESYMBOL_KIND_GLOBAL = 2
} mh_gamesymbol_kind_t;
```

编号一经发布不得重排或复用。

### 5.3 status enum

```cpp
typedef enum mh_gamesymbol_status_e
{
	MH_GAMESYMBOL_OK = 0,
	MH_GAMESYMBOL_INVALID_ARGUMENT = 1,
	MH_GAMESYMBOL_OUTPUT_TOO_SMALL = 2,
	MH_GAMESYMBOL_GAMEDATA_UNAVAILABLE = 3,
	MH_GAMESYMBOL_MODULE_PATH_UNAVAILABLE = 4,
	MH_GAMESYMBOL_MODULE_HASH_FAILED = 5,
	MH_GAMESYMBOL_MODULE_NOT_FOUND = 6,
	MH_GAMESYMBOL_SYMBOL_NOT_FOUND = 7,
	MH_GAMESYMBOL_UNSUPPORTED_KIND = 8,
	MH_GAMESYMBOL_KIND_MISMATCH = 9,
	MH_GAMESYMBOL_RVA_OUT_OF_RANGE = 10,
	MH_GAMESYMBOL_CATALOG_CONFLICT = 11
} mh_gamesymbol_status_t;
```

未来状态只能从尾部追加。

### 5.4 flags

```cpp
#define MH_GAMESYMBOL_FLAG_SIGNATURE_ALLOW_ACROSS_FUNCTION_BOUNDARY 0x1
```

未知 flag bit 必须保留，不应由插件擅自解释。

### 5.5 pattern 结构

```cpp
typedef struct mh_pattern_s
{
	const char* text;
	const BYTE* bytes;
	const BYTE* mask;
	const char* legacyPattern;
	DWORD length;
} mh_pattern_t;
```

字段语义：

- `text`：JSON 中的 canonical signature 文本，NUL 结尾。
- `bytes`：解析后的原始字节数组。
- `mask[i] == 0`：wildcard。
- `mask[i] != 0`：精确匹配 `bytes[i]`。
- `legacyPattern`：将 wildcard 编码为字节 `0x2A` 的旧格式。
- `length`：三种二进制表示的共同长度。

安全降级：

- signature 不包含精确字节 `0x2A` 时生成 `legacyPattern`。
- signature 包含精确字节 `0x2A` 时，`legacyPattern == NULL`。
- `bytes + mask` 永远是权威、无损表示。

### 5.6 symbol 结构

```cpp
typedef struct mh_gamesymbol_s
{
	DWORD cbSize;
	mh_gamesymbol_kind_t kind;
	DWORD flags;
	uint64_t moduleCRC64;

	DWORD rva;
	DWORD symbolSize;
	DWORD signatureRva;
	mh_pattern_t signature;

	DWORD instructionOffset;
	DWORD operandOffset;
	DWORD instructionLength;
} mh_gamesymbol_t;
```

ABI 规则：

- 调用方必须将 `cbSize` 初始化为自身结构体大小。
- 实现只写入 `cbSize` 覆盖的字段。
- `cbSize` 小于首版最小结构时返回 `OUTPUT_TOO_SMALL`。
- 失败时清零输出字段，但保留调用方传入的 `cbSize`。
- 未来只允许在结构尾部追加字段。
- global 不适用的 `symbolSize` 为 0。
- function 不适用的 instruction 字段为 0。

### 5.7 追加到 metahook_api_t 的函数

在 `Terminator` 前追加以下 6 个函数指针：

```cpp
mh_gamesymbol_status_t (*GetModuleCRC64)(
	PVOID moduleBase,
	uint64_t* outCRC64);

mh_gamesymbol_status_t (*QueryGameSymbol)(
	PVOID moduleBase,
	const char* symbolName,
	mh_gamesymbol_t* outSymbol);

mh_gamesymbol_status_t (*QueryGameSymbolByCRC64)(
	uint64_t moduleCRC64,
	const char* symbolName,
	mh_gamesymbol_t* outSymbol);

mh_gamesymbol_status_t (*ResolveGameSymbol)(
	PVOID moduleBase,
	const char* symbolName,
	mh_gamesymbol_kind_t expectedKind,
	PVOID* outAddress);

PVOID (*SearchPatternMasked)(
	PVOID searchBase,
	DWORD searchLength,
	const BYTE* patternBytes,
	const BYTE* patternMask,
	DWORD patternLength);

const char* (*GetGameSymbolStatusString)(
	mh_gamesymbol_status_t status);
```

### 5.8 Legacy V2

- `gMetaHookAPI_LegacyV2` 对新增 6 个槽位显式填写 `NULL`。
- `Terminator` 继续保持最后一个 `NULL`。
- 已编译旧插件只访问原有结构前缀，因此保持二进制兼容。
- V3/V4 获得完整新实现。
- 新插件在调用新槽位前仍应检查 `MetaHookAPIVersion >= 109`。

## 6. 公共 API 精确语义

### 6.1 GetModuleCRC64

职责：

1. 验证参数。
2. 根据 `moduleBase` 获取或创建内部 ModuleIdentity。
3. 首次调用同步读取原始模块文件并计算 CRC-64/XZ。
4. 缓存成功值或失败状态。
5. 返回 numeric `uint64_t` CRC64。

不负责查询 symbol。

### 6.2 QueryGameSymbolByCRC64

职责：

1. 验证 `symbolName`、`outSymbol` 和 `cbSize`。
2. 按精确 `(moduleCRC64, symbolName)` 查询 catalog。
3. 返回规范化元数据和内部只读 pattern 指针。

该函数没有 module image，不能验证 RVA 是否落在实际加载映像内。

### 6.3 QueryGameSymbol

职责：

1. 调用 `GetModuleCRC64`。
2. 调用内部等价的 CRC64 查询路径。
3. 返回规范化元数据。

该函数不将 RVA 转换为 VA，也不隐式扫描 signature。

### 6.4 ResolveGameSymbol

职责：

1. 仅接受 `FUNCTION` 或 `GLOBAL` 作为 `expectedKind`。
2. 查询模块 CRC64 和 symbol metadata。
3. 实际 kind 不一致时返回 `KIND_MISMATCH`。
4. 获取模块 image size。
5. 检查 `rva`、`rva + symbolSize` 的整数溢出和映像边界。
6. 返回 `moduleBase + rva`。

限制：

- `expectedKind == UNKNOWN` 返回 `INVALID_ARGUMENT`。
- 不验证内存 signature。
- 不执行任何跨版本扫描。

### 6.5 SearchPatternMasked

精确规则：

- `mask[i] == 0`：wildcard。
- `mask[i] != 0`：要求目标字节等于 `bytes[i]`。
- 返回给定范围中的第一个匹配地址。
- 任一指针为 `NULL`、任一长度为 0 或 pattern 长于搜索范围时返回 `NULL`。
- 不自动跨 section。
- 字面量 `0x2A` 没有特殊含义。

旧 `SearchPattern` 保持不变。

### 6.6 GetGameSymbolStatusString

- 返回 MetaHook 持有的静态英文字符串。
- 不分配内存。
- 调用方不释放。
- 线程安全。
- 对未知 enum 返回 `"unknown game symbol status"`。
- 永不返回 `NULL`。

## 7. 内部架构

### 7.1 新增 GameData 模块

建议新增：

- `src/GameData.h`
- `src/GameData.cpp`

职责边界：

- `GameData.cpp` 负责 JSON、catalog、signature 编译、模块来源、hash cache 和公共 API 实现。
- `metahook.cpp` 负责 launcher 生命周期、必需符号策略和错误升级。
- `launcher.cpp` 负责把 launcher 已知的原始 engine 路径传入模块来源注册流程。

不把 JSON 解析、哈希状态机和 launcher 私有全局变量继续堆叠到 `metahook.cpp`。

### 7.2 Catalog 数据结构

建议内部模型：

```text
GameDataCatalog
  catalogState
  gamedataRoot
  crc64 -> ModuleCatalog
  diagnostics[]

ModuleCatalog
  crc64
  moduleName
  gameVersion
  sourceSnapshot
  symbolName -> GameSymbolRecord
  conflictState

GameSymbolRecord
  kind
  flags
  rva
  symbolSize
  signatureRva
  signatureText
  signatureBytes
  signatureMask
  optionalLegacyPattern
  globalInstructionMetadata
```

返回给插件的所有指针必须指向最终稳定存储。catalog 发布后不得修改字符串和 vector，避免指针失效。

### 7.3 Signature parser

parser 只接受由空格分隔的 token：

- 两位十六进制字节，例如 `55`、`8B`、`2A`
- wildcard `??`

拒绝：

- 单个十六进制字符
- `?`、`**` 等其他 wildcard 表示
- 非十六进制字符
- 多余前缀，例如 `0x55`
- 空 signature
- token 数量导致 `DWORD` 长度溢出

输出：

- canonical text 副本
- bytes
- mask
- 可表达时的 legacy pattern

### 7.4 ModuleIdentity 与 hash cache

建议状态机：

```text
UNCOMPUTED -> COMPUTING -> READY
                       -> FAILED
```

每个 ModuleIdentity 包含：

- source 类型
- 原始路径或文件系统虚拟路径
- 可选 source alias
- 预期 image base 和 image size
- hash 状态
- CRC64
- 失败 status
- mutex
- condition variable

并发规则：

- 第一个线程将状态切换为 `COMPUTING` 并在锁外执行文件 I/O。
- 同一模块的其他线程等待 condition variable。
- 不持有全局 catalog 锁执行哈希。
- READY 和 FAILED 均缓存到进程退出。

### 7.5 Module source 类型

#### 普通 PE

- 由 `moduleBase` 获取真实模块文件路径。
- 内部优先使用 Unicode Win32 文件 API。
- 对磁盘原始文件字节计算 CRC64。

#### Mirror DLL

- mirror base 注册为真实模块 ModuleIdentity 的 alias。
- 复用真实模块的 CRC64 与 hash 状态。
- 不对 relocation 后的 mirror image 计算 CRC64。

#### Blob engine

- `launcher.cpp` 已知 `pszEngineDLL`。
- 扩展 `MH_LoadEngine` 参数或在调用前执行内部来源注册，将该原始路径绑定到 Blob `ImageBase`。
- 首次查询时重新通过对应文件系统读取原始加密 blob 文件。
- 不对解密后的内存映像计算 CRC64。

#### 无来源纯内存模块

- 返回 `MODULE_PATH_UNAVAILABLE`。
- 首版不开放插件注册来源接口。
- `QueryGameSymbolByCRC64` 仍可在调用方已知 CRC64 时使用。

### 7.6 哈希实现

使用已经提交到 fork 的 Chocobo1Hash 扩展：

- fork：`https://github.com/hzqst/Chocobo1Hash`
- commit：`f455b0e350dce4c3b2415bad5f10484842b0a605`
- CRC64：`Chocobo1::CRC_64_XZ`
- SHA-256：`Chocobo1::SHA2_256`

读取要求：

- 使用固定大小块流式读取，建议 1 MiB，与导出端一致。
- CRC64 参数必须保持 CRC-64/XZ：
  - reflected polynomial：`0xC96C5795D7870F42`
  - init：`0xFFFFFFFFFFFFFFFF`
  - xorout：`0xFFFFFFFFFFFFFFFF`
- 输出统一按 16 位小写十六进制格式用于诊断。

## 8. 生命周期与加载顺序

`MH_LoadEngine` 中的目标顺序：

1. `MH_ResetAllVars`。
2. 设置 `g_szGameDirectory` 和 `gInterface`。
3. 建立 engine module source。
4. 建立普通 engine 与 mirror 的 alias 关系。
5. 初始化本地 GameData catalog。
6. 在当前 `MH_LoadEngine_FindBuildNumber` 调用点之前完成初始化。
7. 使用 `ResolveGameSymbol` 解析 `build_number`。
8. 调用 `build_number` 并结合 engine factory 判断 engine type。
9. 按 engine type 解析剩余最终消费符号。
10. 安装 launcher 自身 hooks。
11. 加载插件。
12. V3/V4 插件在 `Init`、`LoadEngine` 及后续阶段使用新 API。

catalog 初始化必须发生在任何 launcher gamedata 查询之前，但不得提前计算所有已加载模块 CRC64。

## 9. launcher 最终消费符号

### 9.1 全引擎公共必需符号

| Symbol | Kind | 目标变量/用途 |
| --- | --- | --- |
| `build_number` | function | `g_pfnbuild_number`、engine version/type |
| `Sys_Error` | function | `g_pfnSys_Error` |
| `ClientDLL_HudInit` | function | `g_pfnClientDLL_HudInit` hook |
| `g_ppEngfuncs` | global | engine funcs 槽 |
| `g_ppExportFuncs` | global | client export funcs 槽 |
| `g_phClientModule` | global | client module handle 槽 |
| `g_pClientFactory` | global | client factory 槽 |
| `videomode` | global | video mode 指针槽 |
| `gClientUserMsgs` | global | user message 链表头槽 |
| `cl_parsefuncs` | global | SVC parse table |

`ClientDLL_Init`、`DispatchDirectUserMsg`、`VideoMode_Create`、`CBaseUI__Initialize` 等旧定位锚点不再属于 required profile。

### 9.2 Cvar 分支

优先路径：

- `cvar_hooks` global 存在时直接使用。

替代路径：

- `cvar_hooks` 不存在时要求：
  - `Cvar_Set` function
  - `Cvar_DirectSet` function
  - 有效的 `Cvar_Set.symbolSize`
- 仅在已知 `Cvar_Set` 函数范围内反汇编。
- 只重定向明确指向 `Cvar_DirectSet` 的 branch。
- 不再通过字符串或 signature 寻找 `Cvar_Set`。

这里的 `Cvar_Set` 是实际 patch 操作的最终输入，不是用于定位其他符号的 intermediate symbol。

### 9.3 Blob client 分支

与当前行为一致，以下 engine type 要求 Blob client hooks：

- `ENGINE_GOLDSRC`
- `ENGINE_GOLDSRC_BLOB`
- `ENGINE_GOLDSRC_HL25`

所需最终符号：

- `NLoadBlob` function
- `FreeBlob` function

不再解析 `NLoadBlobFile` 作为中间锚点。

### 9.4 CoF 与 SvEngine

- `ENGINE_GOLDSRC_COF` 和 `ENGINE_SVENGINE` 仍必须通过公共 required profile。
- 仅按实际行为启用条件符号。
- validator 不能因为某个 engine family 不需要 Blob hooks 而强制要求 `NLoadBlob`。

## 10. launcher 错误处理

公共 API 本身保持静默：

- 不弹窗。
- 不调用 `Con_Printf`。
- 不主动调用 `MH_SysError`。

launcher 对必需符号查询失败时升级为致命错误，消息包含：

- gamedata 根目录或 snapshot 文件
- 模块路径
- 已计算时的 CRC64
- symbol name
- `GetGameSymbolStatusString` 返回的文本原因

错误消息不显示数字状态码。

示例：

```text
MH_LoadEngine: Failed to resolve "videomode"
Module: D:\Games\Sven Co-op\hw.dll
CRC64: 6ad36cd4373ebb45
Reason: symbol was not found in the matched gamedata snapshot
```

如果 `Sys_Error` 自身尚未解析，继续使用当前 `MH_SysError` 的 MessageBox fallback。

## 11. 预计文件改动

### 11.1 公共 ABI

- `include/metahook.h`
  - 加入 `<stdint.h>`。
  - API 版本升到 109。
  - 新增 enum、flags、`mh_pattern_t`、`mh_gamesymbol_t`。
  - 在 `metahook_api_t` 尾部追加 6 个函数。

### 11.2 launcher 核心

- `src/GameData.h`，新增
- `src/GameData.cpp`，新增
- `src/metahook.cpp`
  - 初始化 catalog。
  - 接入公共函数表。
  - 使用 resolver 替代私有定位器。
  - 删除最终不再使用的 intermediate locator 和本地变量。
- `src/launcher.cpp`
  - 将 engine 原始文件来源传递给 GameData 模块。
- `src/LoadBlob.cpp` / `src/LoadBlob.h`
  - 仅在来源注册需要 Blob handle 到 ImageBase 辅助时修改。

### 11.3 工程配置

- `src/MetaHook.vcxproj`
  - 添加 `GameData.cpp`。
  - 添加 RapidJSON include path。
  - 添加 Chocobo1Hash `src` include path。
  - 所有 MetaHook 配置的 Pre-build 在 `CapstoneCheckRequirements` 后调用统一 gamedata sync 命令。
- `src/MetaHook.vcxproj.filters`
  - 添加新文件过滤器项；文件存在时同步维护。
- `tools/global_common.props`
  - 定义固定 index URL。
  - 定义 gamedata 目标目录。
  - 定义统一的 `GameDataSyncCommand`，避免在多个配置中重复长命令。

### 11.4 打包数据

- `Build/svencoop/metahook/gamedata/index.json`
- `Build/svencoop/metahook/gamedata/<snapshot>.json`

数据文件由 Pre-build 同步脚本生成或替换，应保持 GoldSrc_VibeSignatures 发布产物原样，不手工改写 payload。

### 11.5 验证与文档

- `scripts/sync-gamedata.ps1`，新增
  - 从固定 HTTPS index URL 下载 index 和全部声明 snapshots。
  - 在同卷 staging 目录完成 size、SHA-256 和 JSON 基础校验。
  - 使用事务式目录交换更新目标目录。
  - 支持 `-ValidateOnly` 对现有目录执行离线完整性检查。
- `scripts/validate-gamedata.py`，建议新增
  - 校验本地 manifest、snapshot 完整性和 required profiles。
  - 作为发布门禁，不作为约束易变 JSON 内容的单元测试。
- `memory/metahook-privatevars.md`
  - 更新为 gamedata 最终消费符号与迁移后的定位方式。
- `docs/MetaHook.md`
- `docs/MetaHookCN.md`
  - 补充 API 109 和新公共 API。

### 11.6 submodule

- `.gitmodules` 已经指向 `https://github.com/hzqst/Chocobo1Hash`。
- 父仓库 gitlink 应提交到 `f455b0e350dce4c3b2415bad5f10484842b0a605` 或后续包含该 commit 的版本。

## 12. Pre-build 原子同步设计

### 12.1 MSBuild 接入

在 `tools/global_common.props` 定义：

```xml
<GameSymbolsIndexUrl>https://hlnd2t.github.io/GoldSrc_VibeSignatures/gamesymbols/index.json</GameSymbolsIndexUrl>
<GameDataOutputDirectory>$(MetaHookBaseDir)Build\svencoop\metahook\gamedata</GameDataOutputDirectory>
<GameDataSyncCommand>powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$(MetaHookBaseDir)scripts\sync-gamedata.ps1" -IndexUrl "$(GameSymbolsIndexUrl)" -Destination "$(GameDataOutputDirectory)"</GameDataSyncCommand>
```

每个 MetaHook `PreBuildEvent` 按以下顺序执行：

```xml
<Command>$(CapstoneCheckRequirements)
$(GameDataSyncCommand)</Command>
```

适用配置至少包括：

- `Debug|Win32`
- `Release|Win32`
- `Release_blob|Win32`

若保留其他有效 MetaHook 配置，也必须使用同一命令，不得复制并分叉下载逻辑。

### 12.2 下载集合

每次 Pre-build 下载：

1. 固定 URL 的 `index.json`。
2. `index.versions[]` 中每个条目的 `url` 文件。

不根据本地 engine type、`fileCount` 或当前构建配置裁剪 snapshots。全部下载可保证生成目录是完整、可独立发布的 catalog，并能让空 records snapshot 仍提供明确的已知模块身份。

### 12.3 网络与路径安全

sync script 必须：

- 只接受 HTTPS index URL。
- 使用 index URL 作为 snapshot URI base。
- 拒绝 snapshot URL 改变 scheme、host 或 port。
- 拒绝绝对路径、UNC、盘符、目录分隔符、`..` 和路径逃逸。
- 为 HTTP 请求设置有限 timeout。
- 对瞬时网络错误做有限次数重试和短暂 backoff。
- 禁止交互式凭证或确认提示。
- 任何最终失败都返回非 0 exit code。

### 12.4 staging 与校验

staging 目录必须创建在目标目录的同一父目录和卷，例如：

```text
Build\svencoop\metahook\.gamedata.staging.<pid>.<guid>\
```

流程：

1. 下载 index 到 staging。
2. 解析并校验 index schema。
3. 下载每个 snapshot 到 staging。
4. 检查实际文件长度等于 index `size`。
5. 使用 PowerShell/.NET SHA-256 检查 index `sha256`。
6. 解析 snapshot JSON 并检查顶层 schemaVersion。
7. 检查 staging 中不存在 index 未声明的额外文件。
8. 全部成功后才进入目录交换阶段。

构建期使用 PowerShell/.NET SHA-256，因为 launcher 尚未构建；运行时 catalog 仍使用 `Chocobo1::SHA2_256` 做独立校验。

### 12.5 并发构建锁

Visual Studio 或 CI 可能并行触发多个 MetaHook 配置。sync script 必须针对规范化 destination 获取跨进程互斥锁：

- 首选基于 destination 派生名称的 .NET named mutex。
- 未获得锁的进程等待当前同步结束，而不是启动第二次交换。
- 等待必须有 timeout；超时导致 Pre-build 失败。
- 持锁范围覆盖下载、校验、目录交换和自身临时文件清理。

### 12.6 事务式目录交换

下载期间不得写入当前 `gamedata` 目录。全部 staging 校验成功后：

1. 为旧目标生成同卷唯一 backup 路径。
2. 目标存在时，将目标目录 rename 到 backup。
3. 将 staging 目录 rename 为正式 `gamedata`。
4. 第二次 rename 失败时，立即把 backup rename 回目标。
5. 新目标就位后删除 backup。
6. backup 清理失败时保留有效新目标并打印 warning，不回滚成功更新。

同卷 rename 保证不会复制半成品；交换失败必须恢复旧目录。脚本对外保证构建成功时目标目录是完整新 generation，构建失败时旧 generation 仍可用。

### 12.7 清理与失败语义

- 脚本只删除自己创建且已验证位于目标父目录内的 staging/backup。
- 不对广泛路径、workspace root 或未解析变量执行递归删除。
- 下载、解析、hash、rename 或 rollback 任何一步失败都打印明确文件和原因。
- 网络失败不静默使用旧目录继续成功构建。
- 已存在旧目录只作为 rollback 数据，不作为成功下载的替代品。
- `finally` 释放 mutex 并清理仍属于本次执行的 staging。

### 12.8 可重复性与增量行为

首版优先保证正确性，每次 Pre-build 都重新获取 index 并构建完整 staging。后续如需优化，可在不改变交换语义的前提下：

- 对内容寻址 snapshot 文件复用已经验证的本地副本。
- 使用 ETag/If-None-Match 获取 index。

缓存优化不属于首版验收范围，也不得让未经本次 index 验证的文件进入 staging。

## 13. 分阶段实施步骤

### Phase 0：数据与依赖前置门禁

- [ ] 将父仓库 Chocobo1Hash gitlink 固定到已推送的 CRC64-XZ commit。
- [ ] 实现 `scripts/sync-gamedata.ps1`。
- [ ] 将 sync 命令接入 MetaHook 全部 Pre-build 配置。
- [ ] 通过 Pre-build 将 `index.json` 和全部 snapshots 原子同步到打包目录。
- [ ] 确认实际文件名与 index `url` 一致。
- [ ] 统计所有 Windows module CRC64，确认无冲突。
- [ ] 补齐当前 snapshots 中缺失的 launcher 最终消费符号。
- [ ] 明确每个 gameVersion 对应的 engine family。

完成标准：全部目标 engine family 都有可满足 required profile 的数据。

### Phase 1：公共 ABI 骨架

- [ ] 在 `metahook.h` 中加入已确认的类型、enum 和函数指针。
- [ ] 将 API 版本改为 109。
- [ ] 更新 `gMetaHookAPI` 初始化器。
- [ ] 更新 `gMetaHookAPI_LegacyV2`，新增槽位填 `NULL`。
- [ ] 为 enum 编号和首版结构尺寸添加编译期断言。
- [ ] 先提供返回 `GAMEDATA_UNAVAILABLE` 的最小 stub，保证工程可分阶段编译。

完成标准：MetaHook 和全部插件能在新 header 下编译，旧槽位顺序不变。

### Phase 2：Catalog 与 parser

- [ ] 实现安全路径拼接和 index 读取。
- [ ] 实现 index schema 校验。
- [ ] 使用 `SHA2_256` 校验 snapshot。
- [ ] 实现 snapshot schema 校验。
- [ ] 只提取 Windows records。
- [ ] 实现 function/global payload normalization。
- [ ] 实现 signature text -> bytes/mask/legacyPattern 编译。
- [ ] 实现重复记录去重与冲突标记。
- [ ] 实现 snapshot 单文件隔离和内部 diagnostics。
- [ ] catalog 构建完成后冻结内部存储。

完成标准：可从本地目录稳定构建只读 `(CRC64, symbolName)` 索引。

### Phase 3：Module source 与 lazy hash

- [ ] 实现普通 PE source discovery。
- [ ] 实现 Blob engine source registration。
- [ ] 实现 mirror alias registration。
- [ ] 实现 ModuleIdentity 状态机。
- [ ] 使用 `CRC_64_XZ` 流式读取原始文件。
- [ ] 保证同一 module 的并发查询只计算一次。
- [ ] 缓存 hash 成功值与失败状态。
- [ ] 文件在读取期间发生变化时返回 `MODULE_HASH_FAILED`。
- [ ] 确保 hash I/O 不持有 catalog 全局锁。

完成标准：首次查询发生一次 I/O，后续查询不再访问文件。

### Phase 4：公共 API 完整实现

- [ ] 实现 `GetModuleCRC64`。
- [ ] 实现 `QueryGameSymbolByCRC64`。
- [ ] 实现 `QueryGameSymbol`。
- [ ] 实现 `ResolveGameSymbol`。
- [ ] 实现 `SearchPatternMasked`。
- [ ] 实现 `GetGameSymbolStatusString`。
- [ ] 实现统一的参数校验与 out 清零规则。
- [ ] 实现 RVA 边界和整数溢出检查。
- [ ] 验证内部 pattern 指针的进程期稳定性。

完成标准：V3/V4 插件能通过 API 109 查询并解析已加载 engine 符号。

### Phase 5：launcher 严格迁移

- [ ] 在 `MH_LoadEngine_FindBuildNumber` 之前初始化 catalog。
- [ ] 先迁移 `build_number`。
- [ ] 保持 engine type 判定顺序。
- [ ] 迁移公共必需函数和 globals。
- [ ] 迁移 cvar_hooks 优先路径。
- [ ] 迁移 Cvar_Set/Cvar_DirectSet 条件路径。
- [ ] 迁移 NLoadBlob/FreeBlob 条件路径。
- [ ] 使用公共 status string 生成 launcher 错误原因。
- [ ] 删除仅作为 intermediate symbols 的定位流程。
- [ ] 删除 Release 旧扫描 fallback。
- [ ] 审核并删除不再使用的 mirror-space 转换、signature 宏和局部变量。

完成标准：launcher 本体不再通过旧 pattern/string/xref 逻辑定位任何已迁移私有符号。

### Phase 6：打包、validator 与文档

- [ ] 确认 clean workspace 中首次构建能生成完整 gamedata 目录。
- [ ] 确认已有目录时 Pre-build 能事务式替换整个 generation。
- [ ] 确认下载或校验失败时旧目录保持不变且构建失败。
- [ ] 实现并运行 required-symbol validator。
- [ ] 更新中英文 MetaHook API 文档。
- [ ] 更新 privatevars memory note。
- [ ] 记录每个 engine family 的 smoke test 环境与结果。
- [ ] 移除迁移期临时 Debug differential code。

完成标准：打包产物自包含，文档、ABI 和运行行为一致。

## 14. required-symbol validator

validator 至少检查：

1. index schema、路径安全、文件存在、size、SHA-256。
2. snapshot schema 与 Windows binary metadata。
3. signature token 合法性。
4. function/global required 字段完整性。
5. `(CRC64, symbolName)` 冲突。
6. 所有 engine family 的公共必需符号。
7. cvar alternative：
   - `cvar_hooks`
   - 或 `Cvar_Set + Cvar_DirectSet + Cvar_Set.symbolSize`
8. Blob 条件符号。
9. 所有 DWORD 字段范围和 global `signatureRva` 推导合法性。

validator 必须以非 0 exit code 表示失败，并打印 gameVersion、module、CRC64、symbol 和原因。

validator 是发布产物一致性检查，不应在普通单元测试中硬编码某个易变 snapshot 的具体 RVA 或 signature 文本。

## 15. 验证策略

仓库当前没有 MetaHook 第一方单元测试工程，因此本任务不单独引入一套新的测试框架。验证采用已有第三方测试、构建门禁、离线 validator、Debug 对照与真实启动 smoke test。

### 15.1 Pre-build 原子同步验证

至少验证以下场景：

- 目标目录不存在时，首次同步生成完整目录。
- 目标目录存在时，成功同步后整个 generation 被替换。
- index 下载失败时命令返回非 0，旧目录内容和时间戳保持不变。
- 任一 snapshot 返回 404 时命令返回非 0，旧目录保持不变。
- 任一 snapshot size 不符时命令返回非 0，旧目录保持不变。
- 任一 snapshot SHA-256 不符时命令返回非 0，旧目录保持不变。
- snapshot URL 包含路径穿越或改变 host 时被拒绝。
- 在 staging -> target rename 失败的故障注入下，backup 能恢复为目标。
- 两个并发 sync 进程针对同一 destination 时被互斥锁串行化。
- 成功后不存在属于本次运行的 staging 或 backup 残留。
- `-ValidateOnly` 在无网络情况下能验证现有目录。

测试应使用临时目标目录和受控 fixture index，不得对真实 `Build\svencoop\metahook\gamedata` 做破坏性故障注入。

### 15.2 已完成的 Chocobo1Hash 验证

- `CRC-64/XZ("123456789") == 995dc9bbdf1939fa`
- 空输入 CRC64 为 `0000000000000000`
- 分块输入与单块输入一致
- Chocobo 全量 Meson tests：`1/1 OK`
- CLI 构建成功
- 对 `README.md` 的 CRC64 与 GoldSrc_VibeSignatures Python 导出实现一致：
  - `4f7a620ad086d12f`

### 15.3 编译验证

至少执行：

```bat
MSBuild.exe MetaHook.sln /target:MetaHook /p:Configuration=Debug /p:Platform=Win32
scripts\build-MetaHook.bat
scripts\build-MetaHook-blob.bat
scripts\build-Plugins.bat
```

检查：

- Pre-build 成功完成 gamedata 原子同步。
- Debug、Release、Release_blob 均成功。
- 所有 V3/V4 插件在 API 109 header 下成功编译。
- Legacy V2 初始化器无字段错位。
- 无新增 warning 或未使用私有定位符号。

### 15.4 Catalog 验证

- 正常 index/snapshot 全部通过。
- index 缺失时返回 `GAMEDATA_UNAVAILABLE`。
- index schema 不支持时整体不可用。
- 单个 snapshot 缺失时其他 snapshots 仍可查询。
- snapshot size/SHA 错误时隔离。
- 非法路径被拒绝。
- malformed signature 被拒绝。
- identical duplicate 被去重。
- conflicting duplicate 返回 `CATALOG_CONFLICT`。
- unsupported kind 返回 `UNSUPPORTED_KIND`。

### 15.5 API 行为验证

- `cbSize` 太小返回 `OUTPUT_TOO_SMALL`。
- symbol name 大小写不同返回 `SYMBOL_NOT_FOUND`。
- `QueryGameSymbolByCRC64` 返回正确 metadata。
- `QueryGameSymbol` 只在首次调用触发 hash。
- 并发首次查询只读取一次文件。
- `ResolveGameSymbol` kind 不匹配返回 `KIND_MISMATCH`。
- `expectedKind == UNKNOWN` 返回 `INVALID_ARGUMENT`。
- RVA 越界返回 `RVA_OUT_OF_RANGE`。
- exact `0x2A` 导致 `legacyPattern == NULL`。
- `SearchPatternMasked` 能精确匹配 literal `0x2A`。
- 未知 status string 返回固定 fallback 文本。

### 15.6 launcher Debug differential 验证

在删除旧定位器前，临时 Debug 构建可对同一已知模块比较：

```text
gamedata resolved VA == legacy locator VA
```

限制：

- 仅用于迁移开发。
- 不进入 Release 行为。
- 最终提交前删除 differential 路径及 intermediate locator。

### 15.7 全引擎族 smoke matrix

| Engine family | 建议数据集/环境 | 必须验证 |
| --- | --- | --- |
| `ENGINE_SVENGINE` | `svencoop-10257` | 启动、client 初始化、插件加载 |
| `ENGINE_GOLDSRC_HL25` | `hl-10210` | 启动、videomode、cvar、Blob client hooks |
| `ENGINE_GOLDSRC` | `hl-8684` 或 legacy non-blob | 启动、cvar alternative、client 初始化 |
| `ENGINE_GOLDSRC_BLOB` | `hl-3248`/`hl-4554` | 原始 blob CRC、解密加载、Blob client hooks |
| `ENGINE_GOLDSRC_COF` | `cof-5936` | 启动、CoF HUD/client path |

没有可运行环境时必须明确记录未执行，不能宣称该 family 已完成运行验收。

## 16. 当前已知阻塞与数据缺口

截至 2026-08-29，当前发布 snapshots 尚不能直接满足严格迁移：

- `svencoop-10257` Windows snapshot 当前缺少：
  - `videomode`
  - `cvar_hooks`
  - `Cvar_DirectSet`
- 部分旧 HL snapshots 也缺少当前 launcher 所需的最终记录。
- `Build\svencoop\metahook\gamedata\` 当前尚未创建并填充发布 JSON。

处理原则：

- 先在 GoldSrc_VibeSignatures 补齐或修正导出。
- required-symbol validator 全部通过后再完成 launcher 严格切换。
- 不通过重新启用旧扫描 fallback 绕过数据缺口。

## 17. 风险与缓解

### 风险：构建网络故障破坏已有 gamedata

缓解：

- 所有下载和校验都在同卷 staging 中完成。
- 失败时不进入目录交换。
- 交换失败时从 backup 回滚。
- Pre-build 返回非 0，禁止继续产出看似成功但数据陈旧的构建。

### 风险：并行构建互相覆盖

缓解：

- 按 destination 使用 named mutex。
- 锁覆盖下载、校验、交换和清理全过程。
- 锁等待超时明确失败，不并行写入目标目录。

### 风险：磁盘文件与已加载映像来源不一致

缓解：

- 普通 PE 使用真实模块路径。
- mirror 显式 alias 到真实 source。
- Blob 使用 launcher 实际加载的原始文件来源。
- 读取前后检查可用的 size/metadata，变化时 hash 失败。

### 风险：catalog 指针失效

缓解：

- 初始化完成后不修改 catalog。
- 不支持 reload。
- 返回指向稳定 heap-owned record 的指针。

### 风险：CRC64 冲突或重复发布

缓解：

- 构建 catalog 时检测 CRC/module 和 symbol 冲突。
- 冲突键返回 `CATALOG_CONFLICT`。
- validator 在发布前阻断。

### 风险：signature 旧格式误解 literal 0x2A

缓解：

- `bytes + mask` 为权威格式。
- 存在 literal `0x2A` 时不生成 legacyPattern。
- 新代码优先使用 `SearchPatternMasked`。

### 风险：插件在错误阶段调用

缓解：

- catalog 在插件加载前初始化。
- 初始化失败时 API 返回 `GAMEDATA_UNAVAILABLE`。
- 文档明确 API 109 与生命周期。

### 风险：严格迁移扩大启动失败范围

缓解：

- 先执行覆盖 validator。
- 分 engine family 做 smoke test。
- 错误文本包含模块、CRC64、symbol 和文本原因。
- 数据不完整时不宣布迁移完成。

## 18. 完成验收标准

只有同时满足以下条件，才能声明实现完成：

1. API 109 公共 ABI 已实现且旧插件兼容。
2. MetaHook Pre-build 能从固定 HTTPS index 下载全部声明 JSON，并通过事务式目录交换更新打包目录。
3. Pre-build 下载、校验、并发锁和 rollback 故障场景均通过。
4. 本地 manifest、snapshot 完整性和路径安全已实现。
5. module CRC64 确认是同步 lazy-compute，并具有线程安全缓存。
6. `QueryGameSymbol*`、`ResolveGameSymbol`、`SearchPatternMasked` 和 status string 均符合本计划。
7. launcher Release 不再使用旧私有符号扫描 fallback。
8. launcher 不再解析 intermediate symbols。
9. required-symbol validator 对全部打包数据通过。
10. Debug、Release、Release_blob 和插件构建通过。
11. 所有声明支持的 engine family 完成 smoke test，或明确记录真实环境阻塞。
12. 中英文 API 文档和 privatevars memory note 已同步。
13. 关键验证命令、退出码与结果已如实记录。

## 19. 推荐执行顺序摘要

```text
实现 Pre-build 原子同步
  -> 补齐并校验 gamedata
  -> 固定 Chocobo1Hash gitlink
  -> 追加 API 109 ABI
  -> 实现 catalog/parser
  -> 实现 module source/lazy hash
  -> 实现公共查询 API
  -> 迁移 launcher 最终符号
  -> 删除旧 locator/fallback
  -> validator + 全配置构建
  -> 全引擎族 smoke
  -> 更新文档与 memory
```

该顺序的关键依赖是：数据覆盖必须先于最终移除旧定位器，公共 ABI 骨架必须先于插件编译验证，catalog 必须在 `MH_LoadEngine_FindBuildNumber` 之前可用，而模块 CRC64 仍必须保持首次查询时才计算。

# ResourceReplacer

## 概述
`ResourceReplacer` 是一个运行时资源重定向插件：在不改动磁盘原文件的前提下，通过 `.gmr/.gsr` 规则把模型/声音加载路径替换为指定资源路径。
插件通过引擎内部 `FS_Open` 调用点补丁实现替换，支持普通映射与正则映射两种规则。

## 职责
- 在客户端生命周期中加载/清理全局与地图级资源替换规则。
- 在模型与声音加载路径上拦截 `FS_Open("rb")` 并按规则改写文件名。
- 维护跨引擎版本（GoldSrc/SvEngine/HL25/blob）的地址发现与 hook 安装逻辑。
- 保证替换前后扩展名一致，避免跨类型资源替换。

## 涉及文件 (不要带行号)
- Plugins/ResourceReplacer/plugins.cpp
- Plugins/ResourceReplacer/plugins.h
- Plugins/ResourceReplacer/exportfuncs.cpp
- Plugins/ResourceReplacer/exportfuncs.h
- Plugins/ResourceReplacer/privatehook.cpp
- Plugins/ResourceReplacer/privatehook.h
- Plugins/ResourceReplacer/ResourceReplacer.cpp
- Plugins/ResourceReplacer/ResourceReplacer.h
- Plugins/ResourceReplacer/util.cpp
- Plugins/ResourceReplacer/util.h
- docs/ResourceReplacer.md
- docs/ResourceReplacerCN.md
- Build/svencoop/metahook/configs/plugins_goldsrc.lst
- scripts/build-Plugins.bat
- MetaHook.sln

## 架构
核心对象与分层：
- **规则层**：`CResourceReplacer`（`m_MapEntries` / `m_GlobalEntries`）管理规则集合；条目类型为 `CPlainResourceReplaceEntry` 与 `CRegexResourceReplaceEntry`。
- **生命周期层**：`HUD_Init` 加载全局规则，`HUD_VidInit` 清空地图规则，`HUD_Shutdown` 释放全部规则。
- **引擎 hook 层**：`Engine_FillAddress*` 负责签名/反汇编定位；`Engine_InstallHooks` 把引擎内目标调用重定向到插件包装函数。
- **替换执行层**：`Mod_LoadModel_FS_Open` / `S_LoadSound_FS_Open` 在 `rb` 读取场景调用 `ReplaceFileName`。

```mermaid
flowchart TD
  A[插件加载 IPluginsV4::LoadEngine] --> B[Engine_FillAddress* 定位 S_LoadSound/Mod_LoadModel/CL_PrecacheResources]
  B --> C[Engine_InstallHooks 安装分支补丁与 InlineHook]
  D[IPluginsV4::LoadClient] --> E[接管 HUD_Init/HUD_VidInit/HUD_Shutdown]
  E --> F[HUD_Init 加载 resreplacer/default_global.gmr/.gsr]
  G[每张地图预缓存 CL_PrecacheResources] --> H[加载 maps/<map>.gmr/.gsr]
  I[引擎调用 FS_Open("rb")] --> J[Mod_LoadModel_FS_Open 或 S_LoadSound_FS_Open]
  J --> K[ReplaceFileName: 地图规则优先, 全局规则后备]
  K --> L[命中: 用替换路径调用原 FS_Open]
  K --> M[未命中: 用原路径调用原 FS_Open]
```

关键行为细节：
- 规则解析：`LoadReplaceList` 逐行读取，忽略空行与首字符为 `#`/`/` 的行；支持可选引号；第三 token 为 `regex` 时走正则规则。
- 匹配顺序：`ReplaceFileName` 先遍历地图规则，再遍历全局规则（地图规则优先级更高）。
- 地图规则文件名来源：`pfnGetLevelName()` 去扩展名后拼接 `.gmr/.gsr`（例如 `maps/foo.bsp` -> `maps/foo.gmr/.gsr`）。

## 依赖
- MetaHook API：`SearchPattern*`、`ReverseSearchFunctionBegin*`、`DisasmRanges`、`InlinePatchRedirectBranch`、`InlineHook/UnHook`、`GetEngineType/GetEngineBuildnum`。
- Capstone：用于遍历指令流并定位 `FS_Open` 调用点。
- 引擎导出/接口：`COM_LoadFile`、`COM_FreeFile`、`pfnGetLevelName`、`cl_exportfuncs_t`。
- SourceSDK/工具函数：`V_GetFileExtension`、`stricmp`、`TrimString`、`RemoveFileExtension`。
- 工程接入：`plugins_goldsrc.lst`（加载）、`MetaHook.sln` 与 `scripts/build-Plugins.bat`（构建）。

## 注意事项
- 普通规则使用 `stricmp` 做**整串精确匹配**（大小写不敏感）；正则规则使用 `std::regex_match`（要求全串匹配，默认大小写敏感）。
- 替换前后扩展名必须一致（如 `.mdl/.spr/.wav`），否则即使命中规则也会拒绝替换。
- 插件不检查替换目标文件是否真实存在；若目标缺失，资源加载阶段可能报错/失败。
- 仅在 `FS_Open` 的 `pOptions == "rb"` 时执行替换，其他打开模式不参与。
- 地址发现依赖签名与反汇编启发式（尤其是通过 `"rb"` 字符串附近 call 识别 `FS_Open`）；遇到未知引擎构建可能触发 `Sys_Error`。
- `LoadGlobalReplaceList` 未在加载前主动清空全局规则，当前设计依赖 `HUD_Init` 生命周期通常仅执行一次。

## 调用方（可选）
- 插件装载器：`Build/svencoop/metahook/configs/plugins_goldsrc.lst` 中加载 `ResourceReplacer.dll`。
- 插件生命周期入口：`IPluginsV4::LoadEngine` / `LoadClient` / `ExitGame`。
- 客户端导出链路：`HUD_Init`、`HUD_VidInit`、`HUD_Shutdown`。
- 引擎内部被重定向调用点：`S_LoadSound` 与 `Mod_LoadModel` 中的 `FS_Open` 调用分支，以及 `CL_PrecacheResources` inline hook。
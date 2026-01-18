# UtilAssetsIntegrity（PluginLibs/UtilAssetsIntegrity）源码级分析

## 概述
`UtilAssetsIntegrity` 是一个独立的 DLL（`UtilAssetsIntegrity.dll`），通过 `IUtilAssetsIntegrity` 接口对“来自不可信来源的二进制资源”做基础完整性/越界校验，避免上层在解析/渲染/加载时因为畸形数据导致 OOB 读写或崩溃。

当前实现覆盖两类资源：
- GoldSrc/HL1 `StudioModel`（`IDST` 主模型、`IDSQ` 序列组）
- 8-bit（索引色）BMP（通过 FreeImage 解码并做尺寸约束）

## 职责
- 对外提供稳定 API：
  - `CheckStudioModel(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_StudioModel* out)`
  - `Check8bitBMP(const void* buf, size_t bufSize, UtilAssetsIntegrityCheckResult_BMP* out)`
- 返回 `UtilAssetsIntegrityCheckReason`（`OK/InvalidFormat/SizeTooLarge/SizeTooSmall/BogusHeader/VersionMismatch/OutOfBound/Unknown`），并尽量在 `out->ReasonStr` 填充可读原因。
- 作为“下载/导入前置过滤器”被插件/工具动态加载使用。

## 对外接口（include/Interface）
- 头文件：`include/Interface/IUtilAssetsIntegrity.h`
  - `UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION` = `"UtilAssetsIntegrityAPI_001"`（`include/Interface/IUtilAssetsIntegrity.h:61`）
  - `IUtilAssetsIntegrity` 继承 `IBaseInterface`（`include/Interface/IUtilAssetsIntegrity.h:51`）
  - `UtilAssetsIntegrityCheckResult`：仅 `ReasonStr[256]`（`include/Interface/IUtilAssetsIntegrity.h:17`）
  - `UtilAssetsIntegrityCheckResult_BMP`：额外约束字段 `MaxWidth/MaxHeight/MaxSize`（`include/Interface/IUtilAssetsIntegrity.h:34`）

## 架构
- 代码位置：`PluginLibs/UtilAssetsIntegrity/`
  - `dllmain.cpp`：空壳 `DllMain`（`PluginLibs/UtilAssetsIntegrity/dllmain.cpp:2`）
  - `UtilAssetsIntegrity.cpp`：核心实现（绝大多数逻辑）
- 实现类：`CUtilAssetsIntegrity : public IUtilAssetsIntegrity`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:24`）
- 导出方式：使用 `metahook.h` 中的接口注册宏 `EXPOSE_SINGLE_INTERFACE(...)` 导出 `CreateInterface` 工厂（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1159` 附近）。

## 核心实现与 Workflow

### 1) StudioModel 校验（IDST/IDSQ）
入口：`CUtilAssetsIntegrity::CheckStudioModel`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1069`）
- 步骤：
  1. `bufSize < sizeof(studiohdr_t)` => `SizeTooSmall`
  2. 读取前 4 字节 magic：
     - `IDSQ` => `CheckStudioModel_IDSQ`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1056`）
     - `IDST` => `CheckStudioModel_IDST`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:958`）
     - 否则 => `BogusHeader`

#### 1.1) IDSQ（序列组）
`CheckStudioModel_IDSQ`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1056`）
- 仅检查 `studiohdr->version == 10`，否则 `VersionMismatch`。

#### 1.2) IDST（主模型）
`CheckStudioModel_IDST`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:958`）
- 关键策略：区分“系统内存段(system_memory_length)”与“完整文件(bufSize)”
  - 若 `studiohdr->textureindex != 0`：用 `texturedataindex` 作为 system-memory 上限（即模型主体段的末尾）
  - 否则：用 `studiohdr->length` 作为 system-memory 上限
  - 目的：很多结构体/索引应位于模型主体段；而贴图像素数据可能位于文件尾部
- 校验流水线（任何一步失败立即返回）：
  - `version == 10`
  - `texturedataindex`、`length` 等字段必须在 `[0, bufSize]` 合理范围
  - 逐块校验（按需）：
    - `numtextures` => `CheckStudioModel_Textures`（结构表）+ `CheckStudioModel_TextureData`（像素数据）
    - `numskinfamilies && numskinref` => `CheckStudioModel_Skins`
    - `numbodyparts` => `CheckStudioModel_BodyParts`（递归到 model/mesh/tri cmd）
    - `numbones` => `CheckStudioModel_Bones`
    - `numseq` => `CheckStudioModel_Sequences`（递归到 events / anim data）
    - `numhitboxes` => `CheckStudioModel_Hitboxes`
    - `numbonecontrollers` => `CheckStudioModel_BoneControllers`

#### 1.3) 结构块细节（关键点）
- 贴图表（`mstudiotexture_t`）
  - `CheckStudioModel_Textures`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:29`）：
    - `numtextures` 范围（含上限常量）
    - `textureindex` 在 `[0, system_memory_length]`
    - `ptexture_base/end` 在 `[buf, buf+system_memory_length]`
  - `CheckStudioModel_TextureData`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:74`）：
    - 每个 `ptexture[i].index` 在 `[0, bufSize]`
    - `width/height` 非负，`width*height` 的像素段 `pal..pal+palsize` 在 `[buf, buf+bufSize]`
    - `ptexture[i].name` 必须 NUL 终止（用 `safe_strlen`）

- skin 引用（`skinindex` / `short` 索引矩阵）
  - `CheckStudioModel_Skins`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:129`）：
    - `pskinref` 表在 `[buf, buf+system_memory_length]`
    - 每个 `ref` 满足 `0 <= ref < numtextures`

- BodyParts -> Submodel -> Mesh -> tri commands 深度校验
  - `CheckStudioModel_BodyParts`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:739`）
  - `CheckStudioModel_BodyPart`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:678`）
  - `CheckStudioModel_Submodel`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:575`）
  - `CheckStudioModel_Mesh`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:449`）
  - 核心：
    - 各种 index（`bodypartindex/modelindex/meshindex/triindex/...`）必须非负且不超过 `bufSize`
    - 指针区间 `base..end` 必须落在 buffer 范围内
    - tri command 解码：读取 `short` 序列，`t==0` 结束；`t<0` fan，`t>0` strip；每个顶点条目步进 `ptricmds += 4`
    - 对 `vertindex/normindex` 做非负与指针越界保护（顶点、法线、骨骼索引数组）

- bones / controllers / hitboxes
  - `CheckStudioModel_Bones`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:832`） + `CheckStudioModel_Bone`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:794`）
    - bone 名称 NUL 终止
    - `parent` 必须 `-1` 或 `[0, numbones)`
    - `bonecontroller[j]` 必须 `-1` 或在合理范围（见“注意事项/潜在问题”）
  - `CheckStudioModel_Hitboxes`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:406`） + `CheckStudioModel_Hitbox`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:389`）
    - hitbox 表位置检查 + `pbbox->bone` 必须在 `[0, numbones)`
  - `CheckStudioModel_BoneControllers`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:903`） + `CheckStudioModel_BoneController`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:886`）
    - controller 表位置检查 + `pbonecontroller->bone` 合法

- sequences / events / anim
  - `CheckStudioModel_Sequences`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:334`）
  - `CheckStudioModel_SeqDesc`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:307`）
  - `CheckStudioModel_SeqDescEvents`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:199`） + `CheckStudioModel_Event`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:188`）
    - events 表位置检查
    - `pevent->options` NUL 终止
  - `CheckStudioModel_SeqDescAnim`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:255`）
    - `animindex` 合法
    - `panim_base..panim_base+numbones` 不越界
    - 对 `panim->offset[j+3]`（仅检查 3 个旋转通道）计算 `panimvalue`，并用 `(panimvalue + 255)` 做粗略上界验证

### 2) 8-bit BMP 校验
入口：`CUtilAssetsIntegrity::Check8bitBMP`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:1096`）
- workflow：
  1. `FreeImage_OpenMemory(buf)`
  2. `FreeImage_LoadFromMemory(FIF_BMP, ...)`
  3. 必须为 `FIC_PALETTE`（索引色），否则 `InvalidFormat`
  4. 若传入 `checkResult`，使用其 `MaxWidth/MaxHeight/MaxSize` 做硬限制；超过 => `SizeTooLarge`
  5. 全部通过 => `OK`

注意：这里的“尺寸限制”完全依赖调用方填写 `checkResult->Max*`；调用方示例见下节。

## 动态加载与调用方典型用法（外部）
该 DLL 通常由插件在运行时加载并通过接口版本号获取实例：
- `Plugins/SCModelDownloader/UtilAssetsIntegrity.cpp:12`：
  - `Sys_LoadModule("UtilAssetsIntegrity.dll")`
  - `Sys_GetFactory(hModule)`
  - `factory(UTIL_ASSETS_INTEGRITY_INTERFACE_VERSION, NULL)` 得到 `IUtilAssetsIntegrity*`
- 上层在下载资源落盘/解码前调用：
  - `Plugins/SCModelDownloader/SCModelDatabase.cpp:520` 设置 BMP `MaxWidth/MaxHeight/MaxSize`，随后 `Check8bitBMP`
  - `Plugins/SCModelDownloader/SCModelDatabase.cpp:504` 使用 `CheckStudioModel`

## 依赖
构建/头文件依赖主要来自：
- HLSDK / GoldSrc 结构体：`#include <studio.h>`（`studiohdr_t`、`mstudiomesh_t` 等）
- MetaHook/Valve interface 工厂：`#include <metahook.h>` + `EXPOSE_SINGLE_INTERFACE`
- FreeImage：`#include <FreeImage.h>`；链接项 `$(FreeImageLibraryFiles)`（来自 `PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.vcxproj:79/114`）
- ScopeExit：`#include <ScopeExit/ScopeExit.h>`（用 `SCOPE_EXIT` 做 `FreeImage_*` 资源回收）

## 注意事项 / 潜在问题（安全与正确性）
这些点与“防御式解析”强相关，后续如果要提升健壮性可优先关注：
- `CheckStudioModel_Hitboxes` 计算了 `pbbox_end` 但未验证其是否越界（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:406`）；理论上仍可能 OOB。
- `CheckStudioModel_BoneControllers`：
  - `pbonecontroller_end = base + studiohdr->numbones` 很可能应为 `+ studiohdr->numbonecontrollers`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:903`）。
- `CheckStudioModel_Bone`：对 `pbone->bonecontroller[j]` 的上界用的是 `studiohdr->numbones`，语义上更像应对照 `numbonecontrollers`（`PluginLibs/UtilAssetsIntegrity/UtilAssetsIntegrity.cpp:794`）。
- 多处边界判断使用 `> buf+bufSize` 而不是 `>=`，且有些检查只比较“元素指针”未覆盖元素大小；总体属于“尽量防崩”但不保证严格无漏洞。
- `CheckStudioModel_TextureData` 中 `palsize = width*height` 没有显式溢出防护；极端畸形值可能绕过部分检查。
- `CheckStudioModel_SeqDescAnim` 用 `(panimvalue + 255)` 作为粗略上界，属于经验性护栏，不是严格解析。

## 适用场景
- 下载器/模型管理器：对网络/第三方来源的 `mdl/bmp` 资源做落盘前过滤。
- 工具链（如 `toolsrc/studiocheck`）对资源包做批量扫描。

## 扩展建议（如果要继续完善）
- 为每个块补齐 `base/end` 的严格越界验证（特别是 hitboxes、bonecontrollers）。
- 所有 size 计算改用 `size_t` 并加 `mul_overflow` 风格防护。
- 统一把“上界常量限制”从 HLSDK 常量与项目自定义常量中梳理清楚（例如 `numtextures` 当前用 `MAXSTUDIOSKINS` 作为上限）。

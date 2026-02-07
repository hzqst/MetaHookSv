# MetaHookSv：插件系统与开发流程

## 关键目录
- 插件目录：`Plugins/XXXXX`，如：`Plugins/Renderer`
- 插件文档：`docs/XXXXX.md`，如：`docs/Renderer.md`, `docs/RendererCN.md`

### 核心插件（示例）
- `VGUI2Extension`：VGUI2 modding framework，许多 UI 相关插件的基础
- `Renderer`：图形增强渲染引擎
- `BulletPhysics`：基于 Bullet Physics 的客户端物理模拟，用于支持布娃娃等物理效果。
- `CaptionMod`：字幕/翻译/HiDPI 支持

### 工具类插件（示例）
- `HeapPatch`：解决GoldSrc引擎固定堆资源过小的问题
- `ResourceReplacer`：运行时资源替换
- `ThreadGuard`：线程管理与清理
- `PrecacheManager`：资源预缓存管理
- `SteamScreenshots`：Steam 截图集成（仅 Sven Co-op）

## PluginLibs（`PluginLibs/`）
- `UtilHTTPClient_SteamAPI`：基于 Steam API 的 HTTP client
- `UtilHTTPClient_libcurl`：基于 libcurl 的 HTTP client
- `UtilAssetsIntegrity`：资产完整性验证
- `UtilThreadTask`：线程工具

## 新增插件的常见流程
1. 在 VS 解决方案中创建新项目，放到 Plugins/XXXXXX
2. 引用需要的 `PluginLibs` 依赖 （可选）
3. 完善插件本体代码的实现（可参考其他插件）
4. 把插件纳入构建脚本 `scripts/build-Plugins.bat`
5. 更新配置中的插件加载列表 `plugins.lst`

## 常见开发模式/约定
- 在 `plugins.cpp!IPluginsV4::LoadClient` 中替换`pExportFunc->`函数指针以接管 `client.dll` 的导出函数，如：`pExportFunc->HUD_Init = HUD_Init;`。此处需要我们实现自己的`HUD_Init`以完成对client的HUD_Init的接管。
- 使用 MetaHook API 与引擎交互/安装 inline hook 或 vtable hook
- 遵循已有插件的结构与命名

## 测试与调试
- 用 `scripts\debug-*.bat` 设定调试环境
- 打开 `MetaHook.sln`，将目标插件设为启动项目，F5 调试

## 安全与边界
- 该项目用于合法的游戏增强/Modding（防御性安全工具定位）。
- 不应引入恶意代码或漏洞利用相关逻辑。

## 兼容性
- 某些功能在不同引擎类型上不可用；做特定操作前先判定引擎类型。

## 插件加载顺序
- `plugins.lst` 中的加载顺序对依赖与 hook 安装非常关键。

## 引擎兼容性

MetaHookSv 支持多种 GoldSrc / SvEngine 变体，常见类型与版本范围：
- `GoldSrc_blob`（buildnum 3248 ~ 4554）：legacy encrypted format
- `GoldSrc_legacy`（< 6153）
- `GoldSrc_new`（8684+） Half-Life Pre-25th Anniversary
- `SvEngine`（8832+）：Sven Co-op 修改版引擎
- `GoldSrc_HL25`（>= 9884）：Half-Life 25th Anniversary Update

注意点：
- 做特定引擎相关的逻辑之前，应当先用 `g_pMetaHookAPI->GetEngineType()` 判断引擎类型。
- legacy blob engines （非正常PE文件，直接以二进制形式内存加载） 需要使用 blob-specific APIs。

# MetaHookSv：插件系统与开发流程（来自仓库根目录原 `CLAUDE.md` 的迁移）

## 插件分布
- 插件目录：`Plugins/`

### 核心插件（示例）
- `VGUI2Extension`：VGUI2 modding framework，许多 UI 相关插件的基础
- `Renderer`：图形增强渲染引擎
- `BulletPhysics`：基于 Bullet Physics 的物理模拟
- `CaptionMod`：字幕/翻译/HiDPI 支持

### 工具类插件（示例）
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
1. 在 VS 解决方案中创建新项目，放到合适的 solution folder
2. 引用需要的 `PluginLibs` 依赖
3. 在 `exportfuncs.cpp` 实现插件接口
4. 把插件纳入构建脚本
5. 更新配置中的插件加载列表（如 `plugins.lst`）

## 常见开发模式/约定
- 插件通过 `exportfuncs.h` 导出标准入口点
- 使用 MetaHook API 与引擎交互/安装 hook
- 遵循已有插件的结构与命名

## 测试与调试
- 用 `scripts\debug-*.bat` 设定调试环境
- 打开 `MetaHook.sln`，将目标插件设为启动项目，F5 调试

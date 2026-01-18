# AGENTS.md

本文件用于指导在本仓库内以“渐进式披露”的方式进行 Agent Coding：优先从 Serena memories 获取高层信息，只在需要时再按需定位并读取具体文件/符号，避免一次性展开大量上下文。

## Serena memories（保持上下文精简）
1. 优先使用 `list_memories` 浏览当前项目已有 memories（不要默认全读）。
2. 仅在需要时，用 `read_memory` 精确读取某个 memory（按需加载）。
3. 如果 memory 信息不足/过期，再回退到读取仓库文件或用 Serena 的符号/搜索能力定点定位，并用 `write_memory` / `edit_memory` / `delete_memory` 维护记忆内容。

## 本仓库的高层信息（优先读对应 memories）
- 项目概览，代码库入口点：`project_overview`
- 插件系统与开发流程：`plugin_system`
- 重要注意事项：`metahooksv_notes`

## 当 memories 不足时的“源文件入口”（按需查询与读取）
- 解决方案与构建：`MetaHook.sln`、`scripts/`
- Loader 与核心逻辑：`src/`
- 公共 API / 接口：`include/metahook.h`、`include/Interface/`
- 插件与通用库：`Plugins/`、`PluginLibs/`
- 插件加载配置：`plugins.lst`

## 渐进式披露要点
- 先读 memories，再定位“单文件/单符号”；不要一次性读全仓库。
- 探索代码优先用 Serena（符号总览/引用/搜索），只在必要时读取文件内容。
- 外部依赖/库的用法优先用 Context7（按需查询）。

## Misc rules
- **ALWAYS** call Serena's `activate_project` on agent startup
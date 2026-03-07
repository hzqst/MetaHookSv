# CLAUDE.md

This file guides Agent Coding in this repository using a "progressive disclosure" approach: prioritize retrieving high-level information from Serena memories first, then locate and read specific files/symbols only when needed, instead of expanding a large amount of context at once.

## Serena memories (keep context concise)
1. Prefer using `list_memories` to browse existing memories in the current project (do not read all of them by default).
2. Use `read_memory` to precisely read a specific memory only when needed (on-demand loading).
3. If memory information is insufficient or outdated, fall back to reading repository files or use Serena's symbol/search capabilities for targeted lookup, and maintain memory content with `write_memory` / `edit_memory` / `delete_memory`.

## High-level information in this repository (read corresponding memories first)
- Project overview and codebase entry points: `project_overview`
- Plugin system and development workflow: `plugin_system`
- Important notes: `metahooksv_notes`

## "Source entry points" when memories are insufficient (query and read on demand)
- Solution and build: `MetaHook.sln`, `scripts/`
- Loader and core logic: `src/`
- Public API / interfaces: `include/metahook.h`, `include/Interface/`
- Plugins and shared libraries: `Plugins/`, `PluginLibs/`
- Plugin loading configuration: `plugins.lst`

## Progressive disclosure key points
- Read memories first, then locate a single file/symbol; do not read the whole repository at once.
- Prefer Serena for code exploration (symbol overview/references/search), and read file contents only when necessary.
- Prefer Context7 for external dependency/library usage (query on demand).

## Important rules
- **ALWAYS** call Serena's `activate_project` on agent startup

# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## Repository layout (aggregator repo)

This repository holds **no component source of its own**. Each component is a git submodule pointing to its own repository under the `MetaHookSv` GitHub org, and each builds independently.

| Path | Role | Build entry |
| --- | --- | --- |
| `MetaHook/` | Loader, core logic, public API, project memory, docs | CMake (`MetaHook/CMakeLists.txt`) |
| `Plugins/` | Game plugins (BulletPhysics, Renderer, CaptionMod, HeapPatch, ...) | CMake (per plugin) |
| `PluginLibs/` | Shared plugin libraries (`Util*`) | CMake (per library) |
| `toolsrc/` | Standalone tools: `BSPLocalizationTools`, `MetahookInstaller`, `SteamAppsLocation` | per repo (`.sln` / CMake) |

Submodule workflow:

- Clone recursively: `git clone --recursive <repo-url>`
- Initialize/refresh after clone or pull: `git submodule update --init --recursive`
- Each submodule tracks its own `main`; change code inside the submodule repo, then bump the gitlink here.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

#### Basic Memory knowledge base (project-scoped, `MetaHook/memory/`)

- Notes live in `MetaHook/memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git inside the MetaHook submodule repo.
- Basic Memory is registered as MCP server `basic-memory`, pinned to the `metahooksv` project (`--project metahooksv` via project-level `.mcp.json`).
- Prefer Basic Memory MCP tools (`search_notes` / `read_note` / `write_note` / `edit_note`) for project knowledge.

#### High-level information in this repository (read corresponding notes first)

- Project overview and codebase entry points: `project_overview`
- Plugin system and development workflow: `plugin_system`
- Build and verification: `build_and_verification`
- Suggested commands: `suggested_commands`

#### When notes are insufficient: source entry points (query and read on demand)

- Core loader and logic: `MetaHook/src/`
- Public API / interfaces: `MetaHook/include/metahook.h`, `MetaHook/include/Interface/`
- Plugins and shared libraries: `Plugins/`, `PluginLibs/`
- Build configuration: `MetaHook/CMakeLists.txt`, `MetaHook/cmake/`, per-component `CMakeLists.txt`
- Runtime plugin loading config: `plugins.lst` (generated under the target game's `metahook/configs/`, not a tracked repo file)

#### Progressive disclosure key points

- Read notes first, then locate a single file/symbol; do not read the whole repository at once.
- Prefer Basic Memory MCP tools for knowledge retrieval, and read file contents only when necessary.
- Prefer Context7 for external dependency/library usage (query on demand).

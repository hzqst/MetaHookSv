# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

#### Basic Memory knowledge base (project-scoped, `memory/`)

- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- Basic Memory is registered as MCP server `basic-memory`, pinned to the `metahooksv` project (`--project metahooksv` via project-level `.mcp.json`).
- Prefer Basic Memory MCP tools (`search_notes` / `read_note` / `write_note` / `edit_note`) for project knowledge.

#### High-level information in this repository (read corresponding notes first)

- Project overview and codebase entry points: `project_overview`
- Plugin system and development workflow: `plugin_system`

#### When notes are insufficient: source entry points (query and read on demand)

- Solution and build: `MetaHook.sln`, `scripts/`
- Loader and core logic: `src/`
- Public API / interfaces: `include/metahook.h`, `include/Interface/`
- Plugins and shared libraries: `Plugins/`, `PluginLibs/`
- Plugin loading configuration: `plugins.lst`

#### Progressive disclosure key points

- Read notes first, then locate a single file/symbol; do not read the whole repository at once.
- Prefer Basic Memory MCP tools for knowledge retrieval, and read file contents only when necessary.
- Prefer Context7 for external dependency/library usage (query on demand).

## Explore SKILLs

- Project-level skills always live in `.claude/skills` no matter what harness tool is being used.

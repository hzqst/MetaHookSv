# CLAUDE.md

This file guides Agent Coding in this repository using a "progressive disclosure" approach: prioritize retrieving high-level information from the Basic Memory knowledge base first, then locate and read specific files/symbols only when needed, instead of expanding a large amount of context at once.

## Basic Memory knowledge base (project-scoped, `memory/`)
- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- Basic Memory is registered as MCP server `basic-memory`, pinned to the `metahooksv` project (`--project metahooksv`).
- Prefer Basic Memory MCP tools (`search_notes` / `read_note` / `write_note` / `edit_note`) for project knowledge.

## High-level information in this repository (read corresponding notes first)
- Project overview and codebase entry points: `project_overview`
- Plugin system and development workflow: `plugin_system`

## "Source entry points" when notes are insufficient (query and read on demand)
- Solution and build: `MetaHook.sln`, `scripts/`
- Loader and core logic: `src/`
- Public API / interfaces: `include/metahook.h`, `include/Interface/`
- Plugins and shared libraries: `Plugins/`, `PluginLibs/`
- Plugin loading configuration: `plugins.lst`

## Progressive disclosure key points
- Read notes first, then locate a single file/symbol; do not read the whole repository at once.
- Prefer Basic Memory MCP tools (`search_notes` / `read_note` / `write_note`) for knowledge retrieval, and read file contents only when necessary.
- Prefer Context7 for external dependency/library usage (query on demand).

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

## Shared dependencies (the `thirdparty/` convention)

To avoid every component fetching or bundling its own copy, the aggregator keeps
**one shared copy per third-party dependency** under the repository-level
`thirdparty/` and injects it into each consumer. Components follow a two-level
convention:

- If the dependency's `<NAME>_SOURCE_PATH` variable is set, use that tree.
- Otherwise fetch the pinned commit with `FetchContent` — **never** bundle a
  submodule inside the component.

Shared `thirdparty/` submodules and the variables they feed:

| `thirdparty/…` | Variable | Consumers |
| --- | --- | --- |
| `ScopeExit` | `SCOPEEXIT_SOURCE_PATH` | BulletPhysics, Renderer, SCModelDownloader, UtilAssetsIntegrity, UtilHTTPClient_* |
| `glew_fork` | `GLEW_SOURCE_PATH` | BulletPhysics, Renderer, SteamScreenshots |
| `FreeImage_clone` | `FREEIMAGE_SOURCE_PATH` | Renderer, UtilAssetsIntegrity |
| `tinyobjloader` | `TINYOBJLOADER_SOURCE_PATH` | BulletPhysics, Renderer |
| `Chocobo1Hash` | `CHOCOBO1HASH_SOURCE_PATH` | BulletPhysics |
| `SteamSDK` | `STEAMSDK_SOURCE_PATH` | SteamScreenshots, UtilHTTPClient_SteamAPI |

Two more dependencies are shared without a submodule:

- **VC-LTL**: the aggregator downloads and verifies VC-LTL 5.3.1 once into
  `thirdparty/VC-LTL-5.3.1` and passes `-DVC_LTL_Root` to every component, so no
  component downloads its own copy. `thirdparty/VC-LTL-*` is gitignored.
- **SDL3/SDL2**: `Renderer` and `VGUI2Extension` consume the SDL headers that
  `MetaHook` builds and installs into the shared prefix; the aggregator passes
  `-DSDL2_INCLUDE_DIRS`/`-DSDL3_INCLUDE_DIRS` and adds a build-order dependency
  on `MetaHook`.

Component rule: when a component needs a shared dependency, add it to the
repository `thirdparty/` as a submodule (if not already present), accept
`<NAME>_SOURCE_PATH` in its `cmake/Dependencies.cmake`, and fall back to
`FetchContent` for the pinned commit — do **not** bundle a submodule inside the
component.

## Top-level CMake aggregator

The root `CMakeLists.txt` is a superbuild: each component is configured in its
own build tree via `ExternalProject_Add` (a single CMake tree would collide on
shared dependency target names such as `FreeImage`/`libglew_static`). It also
resolves the shared VC-LTL and injects every `<NAME>_SOURCE_PATH`.

```bash
cmake -S . -B build -A Win32             # MSVC x86 is required by all components
cmake --build build --config Release
cmake --install build --config Release   # stages into build/output
```

Per-component and per-group options (`METAHOOKSV_BUILD_*`) default to `ON`. The
two .NET tools under `toolsrc/` (`BSPLocalizationTools`, `MetahookInstaller`) are
not part of the CMake build; build them with `dotnet build` on their own
solution.

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

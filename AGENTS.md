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
- Each submodule tracks its own `main`; change code inside the submodule repo, then bump the gitlink in the root repo.

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
| `libxml2` | `LIBXML2_SOURCE_PATH` | BetterSpray |
| `Chocobo1Hash` | `CHOCOBO1HASH_SOURCE_PATH` | BulletPhysics (shares `MetaHook/thirdparty/Chocobo1Hash`) |
| `SteamSDK` | `STEAMSDK_SOURCE_PATH` | SteamScreenshots, UtilHTTPClient_SteamAPI |

BetterSpray also consumes the shared ScopeExit, FreeImage, SteamSDK and
MetaHook Chocobo1Hash sources, plus VGUI2Extension, UtilThreadTask and
UtilHTTPClient_libcurl public interface headers.

Two more dependencies are shared without a submodule:

- **VC-LTL**: the aggregator downloads and verifies VC-LTL 5.3.1 once into
  `thirdparty/VC-LTL-5.3.1` and sets `VC_LTL_Root` for every component, so no
  component downloads its own copy. `thirdparty/VC-LTL-*` is gitignored.
- **SDL3/SDL2**: `Renderer` and `VGUI2Extension` consume only SDL's public
  headers; the aggregator points `SDL2_INCLUDE_DIRS`/`SDL3_INCLUDE_DIRS` at
  MetaHook's SDL submodules (`MetaHook/thirdparty/{sdl2-compat-fork,SDL3_fork}/include`),
  which exist at configure time.

Component rule: when a component needs a shared dependency, add it to the
repository `thirdparty/` as a submodule (if not already present), accept
`<NAME>_SOURCE_PATH` in its `cmake/Dependencies.cmake`, and fall back to
`FetchContent` for the pinned commit — do **not** bundle a submodule inside the
component.

## Top-level CMake aggregator

The root `CMakeLists.txt` adds every enabled component to **one CMake tree**
with `add_subdirectory()` (binary dirs mirror the source paths, e.g.
`build/Plugins/Renderer`), so one configure yields one solution with the real
component targets. It resolves the shared VC-LTL and injects each component's
`<NAME>_SOURCE_PATH` set (see the registry in the file) as normal variables of
that component's scope, which shadow the component's own cache defaults.

```bash
cmake -S . -B build -A Win32             # MSVC x86 is required by all components
cmake --build build --config Release
cmake --install build --config Release   # stages into build/output (CMAKE_INSTALL_PREFIX)
```

Per-component and per-group options (`METAHOOKSV_BUILD_*`) default to `ON`.
Group targets `plugins`, `pluginlibs`, `tools` and `all-components` build a
subset; MetaHook is built through its own `MetaHook` target. The two .NET tools
under `toolsrc/` (`BSPLocalizationTools`, `MetahookInstaller`) are not part of
the CMake build; build them with `dotnet build` on their own solution.

Plugin and PluginLib primary targets use an empty `DEBUG_POSTFIX` in the
aggregator, independently of LaunchGame, so Debug DLL names match plugin lists
and runtime lookups. Vendor targets retain their own naming conventions.

Exception: `METAHOOKSV_ENABLE_LAUNCH_GAME=ON` adds an opt-in Visual Studio
`LaunchGame` startup project. Its deploy dependency builds InstallerCLI (not the
GUI), stages the complete enabled native install under
`build/launch-game/<config>/install/output`, and deploys with debug symbols.
Configuration builds/queries the CLI without modifying the game. Empty
`METAHOOKSV_GAME_DIRECTORY` uses InstallerCLI's Steam discovery by AppID.
See the root README debugging section and `cmake/LaunchGame.cmake`.

Standalone plugins reuse the same module through a small bootstrap: local override,
surrounding aggregator, then a pinned aggregate source archive without submodules.
`DeployGame` installs only that plugin's payload through CLI `-plugins-only` and
requires an existing MetaHook installation. Missing CLI sources use a cached,
self-contained Installer release; source builds and explicit executable overrides
take precedence. Keep the feature off by default and add targets only at top level.
The plugin bootstraps pin the shared module commit, retained by the
`launch-game-cmake-v1` tag. When updating shared behavior for standalone clones,
publish a new retained module commit and update all plugin pins together; do not
move an existing module tag or rely on a feature branch remaining available.

The Windows workflow explicitly installs to `install/output` and packages only
that tree plus the single-file MetahookInstaller GUI and MetahookInstallerCLI
executables. Default plugin list
templates belong to `assets/svencoop/metahook/configs/`; the aggregator installs
them, and the installer selects the template for the chosen game.

Because components share one tree, each component's CMake must stay
includable as a subproject (standalone builds are unaffected by these rules):

- Add a shared vendor target only if no sibling added it:
  `if(NOT TARGET libglew_static) add_subdirectory(...) endif()` (same for
  `FreeImage`).
- Keep imported targets directory-scoped (no `GLOBAL`); e.g. two components
  define their own `SteamSDK::SteamAPI` with different include roots.
- Anchor per-component build trees (gamedata, sync caches) to
  `PROJECT_BINARY_DIR`, not `CMAKE_BINARY_DIR`; MetaHook's gamedata directory
  is the parent of every plugin's, so a shared root would make syncs delete each
  other.
- Do not force generic cache variables such as `BUILD_SHARED_LIBS`; use
  directory-scoped variables instead.
- Target, cache-variable, function and FetchContent names stay
  component-prefixed so they cannot collide.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

#### Basic Memory knowledge base

- **Storage & Structure**:
  - Notes live in `<ProjectName>/memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git inside the MetaHook submodule repo.
  - Basic Memory is registered as MCP server `basic-memory`.
- **Project Discovery & Dynamic Routing**:
  - Available projects are managed dynamically. When uncertain about available projects or when starting a topic-specific task, call `list_memory_projects` first to discover available `<ProjectName>`s.
  - Autonomously select or switch to the appropriate project based on the user's intent, task context, or repository.
- **Tool Usage**:
  - Prefer Basic Memory MCP tools (`search_notes`, `read_note`, `write_note`, `edit_note`, `build_context`) for project knowledge.
  - **Always explicitly specify the `project` argument** (e.g., `project="<ProjectName>"`) in tool calls to target the intended project, rather than relying on the default.
  - Cross-project references should follow the `memory://<ProjectName>/...` URI format.

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

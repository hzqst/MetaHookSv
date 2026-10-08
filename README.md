# MetaHookSv

This is a porting of [MetaHook](https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven Co-op Team), as a client-side modding framework,

Mainly to keep you a good game experience in Sven Co-op or any other GoldSrc based games.

Most of plugins are still compatible with vanilla GoldSrc engine. please check each plugin's doc for plugin compatibility.

[中文README](README.zh-CN.md)

### Compatibility (metahook loader only)

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (< 6153)     | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

## Download

[GitHub Release](https://github.com/MetaHookSv/MetaHookSv/releases)

Download `MetaHookSv-windows-x86.7z`. It contains both normal and blob launchers.

SteamScreenshots, BetterSpray and UtilHTTPClient_SteamAPI share `SteamAPIBridge.dll`
under `metahook/dlls`. Its source is the `PluginLibs/SteamAPIBridge` submodule.
Keep this DLL when deploying these consumers; retain the game's own `steam_api.dll`.
The aggregate builds the bridge first and requires its pluginlibs option when a
consumer is enabled. SteamAPI compatibility does not extend a plugin's engine-hook support.

## Risk of VAC ?

There is no VAC ban reported yet.

The binaries or executables of Sven Co-op are not signed with digital signatures thus no integrity check from VAC would be applied for them.

## FAQ

Q. Why the game process hangs up / freeze occasionally for few seconds when playing on legacy / pirated version of GoldSrc game ?

A: This is because Valve uses `gethostbyname` with an non-existing hostname to query master servers. which is known to block the whole game loop for few seconds if the hostname is not available.

A: You can either add `-nomaster` to launch paramaters to prevent engine from querying invalid hostname or add `-steam` launch paramaters to force engine to use a valid master server source (which probably not gonna work on pirated game).

Q. Why the game process hangs up for tens of seconds on exiting / on restarting ?

A: This is because ThreadGuard.dll is waiting for Valve's network threads or similiar things to exit before actually exiting the game. See [ThreadGuard](https://github.com/MetaHookSv/ThreadGuard) for more details.

Q. Why I got black-screen in the main menu?

A: The [SDL3-over-SDL2 compatibility layer](https://github.com/libsdl-org/sdl2-compat) is not working well with software-rendering mode. please switch to OpenGL mode by adding `-gl` in the launch parameter.

Q. What if game crashes due to out of memory ?

A: try launch parameter: `-metahook_early_unload_mirrored_dll` (This saves ~120MB system memory)

A: try ConVars: `r_studio_lazy_load 1`, `r_leaf_lazy_load 1`

Q. Why the terrain become all black when `sv_cheats 1` ?

A: try `r_lightmap 1`.

## One Click Installation (GUI Installer)

1. Download the release and extract the complete archive.
2. Keep `MetahookInstaller.exe` and `install/output/` together. Run the installer, select the game, then click **Install**.
3. Launch Sven Co-op from Steam or the generated shortcut. Other games use the generated `MetaHook for [GameName].lnk`.

For scripted installation, `MetahookInstallerCLI.exe` from the same archive performs the same steps:

```powershell
.\MetahookInstallerCLI.exe -appid 225840                                         # Install to Sven Co-op (Steam edition)
.\MetahookInstallerCLI.exe -appid 70 -gamedir "D:\SteamLibrary\steamapps\common\Half-Life" -moddir gearbox  # Install to Half-Life : Opposing Force (Steam edition)
.\MetahookInstallerCLI.exe -appid 225840 -uninstall
```

See [MetahookInstaller](https://github.com/MetaHookSv/MetahookInstaller) for all arguments.

## Manual Installation

Runtime files are in `install/output/`. For Sven Co-op, merge `svencoop/`, its sibling resource directories and `platform/` into the game root. For other mods, merge the common `svencoop/` resources into the selected mod directory and add its matching resource directories.

Copy the required launcher, `libcurl.dll` and `steam_api.dll` into the game root. Rename the normal launcher to `svencoop.exe` for Sven Co-op; other games use `MetaHook.exe -insecure -game <mod-directory>`. Legacy blob engines use `MetaHook_blob.exe`.

In the mod's `metahook/configs/`, copy `plugins_svencoop.lst` for Sven Co-op or `plugins_goldsrc.lst` for other games to `plugins.lst`, unless a user list already exists. BetterSpray is enabled by default for Sven Co-op.

For a normal engine that imports SDL2, install both `install/output/SDL2.dll` and `SDL3.dll` together to enable IME candidate support. The GUI installer checks this condition automatically.

## Build Requirements

1. [Visual Studio 2022, with vc143 toolset](https://visualstudio.microsoft.com/)

2. [CMake](https://cmake.org/download/)

3. [Git for Windows](https://gitforwindows.org/)

4. Python 3.8+ for gamedata synchronization, and the .NET 8 SDK for MetahookInstaller.

## Build Instruction

Clone recursively, then run from the repository root in PowerShell:

```bash
git clone --recursive https://github.com/MetaHookSv/MetaHookSv
cd MetaHookSv
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 "-DCMAKE_INSTALL_PREFIX=$PWD/install/output"
cmake --build build --config Release --parallel 2
cmake --install build --config Release
```

The aggregator builds MetaHook, MetaHook_blob, all enabled plugins (including BetterSpray), shared plugin libraries and CMake tools. Plugin list templates are owned by `assets/svencoop/metahook/configs/` and installed with the output. The .NET tools build separately; see [MetahookInstaller](https://github.com/MetaHookSv/MetahookInstaller).

`windows.yml` builds, tests and packages the installer EXE plus the complete `install/output/` tree. Tag releases use this Windows artifact and the separate BSP tool artifact, with bilingual AI release notes.

## Debugging

Enable the optional Visual Studio startup project (building InstallerCLI from source requires a compatible .NET SDK and .NET 8 runtime):

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON -DMETAHOOKSV_GAME_APPID=225840
# For a custom installation/mod, also pass:
# "-DMETAHOOKSV_GAME_DIRECTORY=D:/Games/Half-Life" -DMETAHOOKSV_GAME_APPID=70 -DMETAHOOKSV_GAME_MOD=gearbox
```

Open `build/MetaHookSv.sln`, select Debug/Win32, set **Launch-debugging / LaunchGame** as the startup project if necessary, and press **F5**. This incrementally builds all enabled components and InstallerCLI, installs into a private staging directory, deploys through InstallerCLI, and starts the actual game launcher with the native C++ debugger. The dummy executable is never launched or installed. Release/Win32 also works.

`METAHOOKSV_GAME_DIRECTORY` defaults to empty (InstallerCLI discovers the Steam game by AppID); an explicit directory takes precedence. `METAHOOKSV_GAME_MOD` defaults to the app's base mod. `METAHOOKSV_GAME_ARGUMENTS` appends arguments after `-insecure -game <mod>`. Configuration builds the CLI and queries the game without deploying anything; invalid/missing games fail configuration. Reconfigure after changing the game/engine or moving its installation.

Each startup build redeploys, including when no source changed. The private payload in `build/launch-game/<Debug|Release>/install/output` is recreated, leaving the normal install prefix unchanged. The aggregator uses the normal module names for Debug plugins/shared plugin libraries as well, matching the game's plugin list and dynamic library lookups. InstallerCLI preserves existing plugin selections, maps resources and selects `svencoop.exe`, `MetaHook.exe` or `MetaHook_blob.exe`; root PDBs are also deployed with their original names. Existing game files are overwritten according to the normal installer rules. Disabled components disappear from the staged payload, but previously deployed files are not uninstalled from the game.

In **Tools / Options / Projects and Solutions / Build and Run**, enable building out-of-date projects before running and set **On Run, when build or deployment errors occur** to **Do not launch**. Stop the game before redeploying; locked files fail the build and no process is terminated automatically. Use the debugger's Modules window to check symbol loading if a breakpoint remains unbound. The workflow is opt-in and does not change normal builds or CI when disabled.

### Standalone plugin debugging

All repositories under `Plugins/` support the same options when configured independently, including outside this checkout:

```bash
cmake -S Plugins/HeapPatch -B build/heappatch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
cmake --build build/heappatch --config Debug --target LaunchGame
```

Install MetaHook and enable the plugin in the game's `plugins.lst` first. Standalone **DeployGame** builds the current plugin and its dependencies, stages its install, and updates only plugin files/resources, including plugin PDBs and mod-local dependency libraries. It does not update root launchers/runtime DLLs, create shortcuts, edit plugin lists or remove previously installed files. F5 uses the existing launcher; missing installations fail configuration. The aggregate **DeployGame** continues to deploy the complete enabled install.

Plugins share this repository's CMake module: `METAHOOKSV_LAUNCH_GAME_MODULE_DIR` overrides its directory; otherwise they use the surrounding aggregator checkout, or fetch a pinned source archive into the build tree without its submodules. They never configure the downloaded aggregate project. Disabled LaunchGame performs no module/CLI downloads.

`METAHOOKSV_INSTALLER_CLI_EXECUTABLE` can point to a self-contained CLI for offline use. Otherwise an available `toolsrc/MetahookInstaller/src` is built; when absent, CMake downloads `MetahookInstaller-windows-x64.7z` from the Installer release selected by `METAHOOKSV_INSTALLER_RELEASE` (default `latest`, or a fixed tag). The downloaded CLI needs no .NET installation. Plugin deployment requires a release supporting `-plugins-only` (v20261004c or later).

Downloads use the release's SHA-256 when provided and cache the executable and resolved tag under `build/launch-game/installer/<release>`. A valid cache is reused offline, without checking for newer releases. Select another tag or remove only that private cache directory to update; corrupt or incompatible caches report an error. GitHub API rate limits can be avoided with `GH_TOKEN` or `GITHUB_TOKEN` in the environment; tokens are not cached. Each configuration receives its own CLI and fresh payload under `build/launch-game/<config>`. Keep Visual Studio's build-before-run enabled and its build-error behavior set to **Do not launch**.

## MetaHook

The GoldSrc game launcher, mirroring `hl.exe` with capability of client-side modding.

[Link](https://github.com/MetaHookSv/MetaHook)

## Plugins

### VGUI2Extension

VGUI2Extension acts as a VGUI2 modding framework, providing capability for other plugins to install hooks / patches on VGUI2 components.

[Link](https://github.com/MetaHookSv/VGUI2Extension)

### CaptionMod

A plugin that adds closing-captioning, HUD text translatation, HiDpi support and Source2007-style chat dialog to game.

[Link](https://github.com/MetaHookSv/CaptionMod)

### BulletPhysics

A plugin that transform player model into ragdoll when player is dead or being caught by barnacle.

[Link](https://github.com/MetaHookSv/BulletPhysics)

### Renderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

[Link](https://github.com/MetaHookSv/Renderer)

### StudioEvents

This plugin can block studio-event sound spamming with controllable cvars.

[Link](https://github.com/MetaHookSv/StudioEvents)

### SteamScreenshots

This plugin intercepts `snapshot` command and replace it with `ISteamScreenshots` interface which will upload the snapshot to Steam Screenshot Manager.

[Link](https://github.com/MetaHookSv/SteamScreenshots)

### SCModelDownloader (Sven Co-op only)

This plugin downloads missing player models from https://wootguy.github.io/scmodels/ automatically and reload them once mdl files are ready.

Cvar : `scmodel_autodownload 0 / 1` Automatically download missing model from scmodel database.

Cvar : `scmodel_downloadlatest 0 / 1` Download latest version of this model if there are multiple ones with different version.

[Link](https://github.com/MetaHookSv/SCModelDownloader)

### PrecacheManager

This plugin provides a console command `fs_dump_precaches` to dump precache resource list into `[ModDirectory]\maps\[mapname].dump.res`.

* The SoundSystem from Sven Co-op uses `soundcache.txt` instead of engine's precache system to precache sound files.

[Link](https://github.com/MetaHookSv/PrecacheManager)

### ThreadGuard

This plugin intercepts Valve's Win32Thread creation and wait for all threads to exit before unloading the backed module, in case Valve's thread code gets running when the backed module is unloaded.

Managed modules that may create new threads and quit without waiting for thread termination : 

`hw.dll`, `GameUI.dll`, `ServerBrowser.dll`

[Link](https://github.com/MetaHookSv/ThreadGuard)

### ResourceReplacer

This plugin replaces in-game resources (mainly model and sound files) at runtime with customizable replace list files without actually manipulating the files, just like what Sven Co-op does with [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) and [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) files.

[Link](https://github.com/MetaHookSv/ResourceReplacer)

### InterpFix

This plugin keeps packet entities and their associated effects visible when high snapshot rates exhaust the position history required by `ex_interp`. It uses the oldest valid pose when the requested interpolation time is outside the history window.

[Link](https://github.com/MetaHookSv/InterpFix)

### SCCameraFix (Sven Co-op only)

This plugin fixes camera glitching in spectator-view for Sven Co-op.

The updated spectator-view code credits to [halflife-updated](https://github.com/SamVanheer/halflife-updated)

[Link](https://github.com/MetaHookSv/SCCameraFix)

### BetterSpray

BetterSpray is a plugin for MetaHookSV that enhances Sven Co-op and GoldSrc’s spray system with support for high-res images, dynamic reloading and cloud sharing.

[Link](https://github.com/MetaHookSv/BetterSpray)

### HUDColor

Changing HUD colors in game.

Also as a good template for you to build your own plugin.

[Link](https://github.com/MetaHookSv/HUDColor)

### ABCEnchance (third-party) (Sven Co-op only)

ABCEnchance is a metahook plugin that provides experience improvement for Sven co-op.

1. CSGO style health and ammo HUD
2. Annular weapon menu
3. Dynamic damage indicator 
4. Dynamic crosshair
5. Minimap which is rendered at real time
6. Teammate health, armor and name display with floattext.
7. Some useless blood efx

[Link](https://github.com/DrAbcrealone/ABCEnchance)

### halflife-cli (third-party)

halflife-cli turns Half-Life / Sven Co-op into a CLI-driven program for automated testing or agent operation: it keeps the game window hidden but alive so screenshots still work, accepts console commands over stdin, and serves Source RCON on a random localhost port for discovery and remote control. A bundled Python MCP server exposes the game as callable tools (`launch_game`, `run_command`, `send_key`, `snapshot`, ...).

[Link](https://github.com/DrAbcOfficial/halflife-cli)

### MetaAudio (third-party) (GoldSrc only)

This is a plugin for GoldSrc that adds OpenAL support to its sound system. This fork fixes some bugs and uses Alure instead of OpenAL directly for easier source management.

Since SvEngine uses FMOD as it's sound system, you really shouldn't use this plugin in Sven Co-op.

[Link](https://github.com/LAGonauta/MetaAudio)

* MetaAudio blocks goldsrc engine's sound system and replaces with it's own sound engine. You should always put `MetaAudio.dll` on top of any other plugins that rely on goldsrc engine's sound system (i.e CaptionMod) in the `plugins.lst` to prevent those plugins from being blocked by MetaAudio.

### Trinity-EngineSv (third-party) (GoldSrc only)

This is a Trinity Engine porting for Counter Strike 1.6

Client-Side part of the mod it´s introduced as a metahook plugin.

Server-Side part of the mod it´s done with a modifidied reGame dll.

[Link](https://github.com/ollerjoaco/Trinity-EngineSv)

### BetterSpray (third-party)

BetterSpray is a plugin for MetaHookSV that enhances Sven Co-op’s spray system with support for multiple images, true aspect ratios, and dynamic reloading.

[Link](https://github.com/KazamiiSC/BetterSpray-Sven-Coop)

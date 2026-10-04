# MetaHookSv

This is a porting of [MetaHook](https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven Co-op Team), as a client-side modding framework,

Mainly to keep you a good game experience in Sven Co-op or any other GoldSrc based games.

Most of plugins are still compatible with vanilla GoldSrc engine. please check each plugin's doc for plugin compatibility.

[中文README](READMECN.md)

### Compatibility (metahook loader only)

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (< 6153)     | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

## Download

[GitHub Release](https://github.com/hzqst/MetaHookSv/releases)

Download `MetaHookSv-windows-x86.7z`. It contains both normal and blob launchers.

## Risk of VAC ?

There is no VAC ban reported yet.

The binaries or executables of Sven Co-op are not signed with digital signatures thus no integrity check from VAC would be applied for them.

## FAQ

1. Why the game process hangs up / freeze occasionally for few seconds when playing on legacy / pirated version of GoldSrc game ?

A: This is because Valve uses `gethostbyname` with an non-existing hostname to query master servers. which is known to block the whole game loop for few seconds if the hostname is not available.

You can either add `-nomaster` to launch paramaters to prevent engine from querying invalid hostname or add `-steam` launch paramaters to force engine to use a valid master server source (which probably not gonna work on pirated game).

2. Why the game process hangs up for tens of seconds on exiting / on restarting ?

A: This is because ThreadGuard.dll is waiting for Valve's network threads or similiar things to exit before actually exiting the game. See [ThreadGuard](https://github.com/hzqst/MetaHookSv#threadguard) for more details.

3. Why I got black-screen in the main menu?

A. The [SDL3-over-SDL2 compatibility layer](https://github.com/libsdl-org/sdl2-compat) is not working well with software-rendering mode. please switch to OpenGL mode by adding `-gl` in the launch parameter.

4. What if game crashes due to out of memory ?

Try launch parameter: `-metahook_early_unload_mirrored_dll` (This saves ~120MB system memory)

Try ConVars: `r_studio_lazy_load 1`, `r_leaf_lazy_load 1`

5. Why the terrain become all black when `sv_cheats 1` ?

Try `r_lightmap 1`.

## One Click Installation (GUI Installer)

1. Download the release and extract the complete archive.
2. Keep `MetahookInstaller.exe` and `install/output/` together. Run the installer, select the game, then click **Install**.
3. Launch Sven Co-op from Steam or the generated shortcut. Other games use the generated `MetaHook for [GameName].lnk`.

The installer reads `install/output/` relative to its executable, independently of the current working directory. It selects the normal or blob launcher, maps common resources to the selected mod, and installs runtime DLLs. Existing `plugins.lst` selections are preserved.

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

```powershell
git clone --recursive https://github.com/hzqst/MetaHookSv
Set-Location MetaHookSv
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 "-DCMAKE_INSTALL_PREFIX=$PWD/install/output"
cmake --build build --config Release --parallel 2
cmake --install build --config Release
```

The aggregator builds MetaHook, MetaHook_blob, all enabled plugins (including BetterSpray), shared plugin libraries and CMake tools. Plugin list templates are owned by `assets/svencoop/metahook/configs/` and installed with the output. The .NET tools build separately; see [MetahookInstaller](toolsrc/MetahookInstaller/README.md).

`windows.yml` builds, tests and packages the installer EXE plus the complete `install/output/` tree. Tag releases use this Windows artifact and the separate BSP tool artifact, with bilingual AI release notes.

## Debugging

Build and install with `--config Debug`, then deploy the output to the game. Open `build/MetaHookSv.sln` in Visual Studio, select Debug/Win32, and configure the launcher's debugging command, working directory and `-insecure -game <mod-directory>` arguments for that game.

## MetaHook Docs

[Docs](docs/MetaHook.md) [中文文档](docs/MetaHookCN.md)

## Plugins

### VGUI2Extension

VGUI2Extension acts as a VGUI2 modding framework, providing capability for other plugins to install hooks / patches on VGUI2 components.

[DOC](docs/VGUI2Extension.md) [中文文档](docs/VGUI2ExtensionCN.md)

### CaptionMod

A plugin that adds closing-captioning, HUD text translatation, HiDpi support and Source2007-style chat dialog to game.

[DOC](docs/CaptionMod.md) [中文文档](docs/CaptionModCN.md)

### BulletPhysics

A plugin that transform player model into ragdoll when player is dead or being caught by barnacle.

[DOC](docs/BulletPhysics.md) [中文文档](docs/BulletPhysicsCN.md)

### MetaRenderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

[DOC](docs/Renderer.md) [中文文档](docs/RendererCN.md)

### StudioEvents

This plugin can block studio-event sound spamming with controllable cvars.

[DOC](docs/StudioEvents.md) [中文文档](docs/StudioEventsCN.md)

### SteamScreenshots (Sven Co-op / GoldSrc post-25th update)

This plugin intercepts `snapshot` command and replace it with `ISteamScreenshots` interface which will upload the snapshot to Steam Screenshot Manager.

### SCModelDownloader (Sven Co-op only)

This plugin downloads missing player models from https://wootguy.github.io/scmodels/ automatically and reload them once mdl files are ready.

Cvar : `scmodel_autodownload 0 / 1` Automatically download missing model from scmodel database.

Cvar : `scmodel_downloadlatest 0 / 1` Download latest version of this model if there are multiple ones with different version.

### PrecacheManager

This plugin provides a console command `fs_dump_precaches` to dump precache resource list into `[ModDirectory]\maps\[mapname].dump.res`.

* The SoundSystem from Sven Co-op uses `soundcache.txt` instead of engine's precache system to precache sound files.

### ThreadGuard

This plugin intercepts Valve's Win32Thread creation and wait for all threads to exit before unloading the backed module, in case Valve's thread code gets running when the backed module is unloaded.

Managed modules that may create new threads and quit without waiting for thread termination : 

`hw.dll`, `GameUI.dll`, `ServerBrowser.dll`

### ResourceReplacer

This plugin replaces in-game resources (mainly model and sound files) at runtime with customizable replace list files without actually manipulating the files, just like what Sven Co-op does with [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) and [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) files.

[DOC](docs/ResourceReplacer.md) [中文文档](docs/ResourceReplacerCN.md)

### SCCameraFix  (Sven Co-op only)

This plugin fixes camera glitching in spectator-view for Sven Co-op.

The updated spectator-view code credits to [halflife-updated](https://github.com/SamVanheer/halflife-updated)

### Better Spray (Sven Co-op / GoldSrc post-25th update)

BetterSpray is a plugin for MetaHookSV that enhances Sven Co-op and GoldSrc’s spray system with support for high-res images, dynamic reloading and cloud sharing.

https://github.com/hzqst/BetterSpray

### ABCEnchance (third-party) (Sven Co-op only)

ABCEnchance is a metahook plugin that provides experience improvement for Sven co-op.

1. CSGO style health and ammo HUD
2. Annular weapon menu
3. Dynamic damage indicator 
4. Dynamic crosshair
5. Minimap which is rendered at real time
6. Teammate health, armor and name display with floattext.
7. Some useless blood efx

https://github.com/DrAbcrealone/ABCEnchance

### HUDColor (third-party) (Sven Co-op only)

Changing HUD colors in game.

Also as a good template for you to build your own plugin.

https://github.com/hzqst/HUDColor

### MetaAudio (third-party) (GoldSrc only)

This is a plugin for GoldSrc that adds OpenAL support to its sound system. This fork fixes some bugs and uses Alure instead of OpenAL directly for easier source management.

Since SvEngine uses FMOD as it's sound system, you really shouldn't use this plugin in Sven Co-op.

https://github.com/LAGonauta/MetaAudio

* MetaAudio blocks goldsrc engine's sound system and replaces with it's own sound engine. You should always put `MetaAudio.dll` on top of any other plugins that rely on goldsrc engine's sound system (i.e CaptionMod) in the `plugins.lst` to prevent those plugins from being blocked by MetaAudio.

### Trinity-EngineSv (third-party) (GoldSrc only)

This is a Trinity Engine porting for Counter Strike 1.6

Client-Side part of the mod it´s introduced as a metahook plugin.

Server-Side part of the mod it´s done with a modifidied reGame dll.

https://github.com/ollerjoaco/Trinity-EngineSv

### Better Spray (third-party)

BetterSpray is a plugin for MetaHookSV that enhances Sven Co-op’s spray system with support for multiple images, true aspect ratios, and dynamic reloading.

https://github.com/KazamiiSC/BetterSpray-Sven-Coop

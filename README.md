# MetaHookSv

This is a porting of [MetaHook](https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven Co-op Team),

Mainly to keep you a good game experience in Sven Co-op.

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

* `MetaHookSv-windows-x86.zip` for most users.

* `MetaHookSv-windows-x86-blob-support.zip` for legacy GoldSrc engine with buildnum < 4554.

## Risk of VAC ?

There is no VAC ban reported yet.

The binaries or executables of Sven Co-op are not signed with digital signatures thus no integrity check from VAC would be applied for them.

## One Click Installation

1. Download from [GitHub Release](https://github.com/hzqst/MetaHookSv/releases), then unzip it.

2. Run `scripts\install-to-SvenCoop.bat` (or `scripts\install-to-(WhateverGameYouWant).bat`, depends on which you are going to play)

3. Launch game from either the generated shortcut `MetaHook for SvenCoop.lnk` (* Run `MetaHook for [GameName].lnk` for games other than Sven Co-op.)

* Other games follow the same instruction.

* You should have your Steam running otherwise the [SteamAppsLocation](toolsrc/README.md) will probably not going to find GameInstallDir.

## Manual Installation

1. Download from [GitHub Release](https://github.com/hzqst/MetaHookSv/releases), then unzip it.

2. All required executable and resource files are in `Build` folder, copy [whatever you want](Build/README.md) to `\SteamLibrary\steamapps\common\Sven Co-op\`.

3. Go to `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\`, rename `plugin_svencoop.lst` (or `plugin_goldsrc.lst`) to `plugin.lst` (depending on the engine you are going to run)

4. Rename `MetaHook.exe` to `(ModDirectory).exe`, Let's say `svencoop.exe` for Sven Co-op. or `cstrike.exe` for Counter-Strike.

* Use `MetaHook_blob.exe` instead of `MetaHook.exe` if you are on a legacy GoldSrc engine with buildnum < 4554.

* Plugins can be disabled or enabled in `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\plugins.lst`

* The SDL2.dll fixes a bug that the IME input handler from original SDL library provided by valve was causing buffer overflow and game crash when using non-english IME. you don't need to copy it if you don't have a non-english IME.

* Valve fixed this issue for SDL2 in HL25th patch, you don't have to replace SDL2 if you are running HL25th engine.

## Build Requirements

1. [Visual Studio 2017 / 2019 / 2022, with vc141 / vc142 / vc143 toolset](https://visualstudio.microsoft.com/)

2. [CMake](https://cmake.org/download/)

3. [Git for Windows](https://gitforwindows.org/)

## Build Instruction

Let's assume that you have all requirements installed correctly.

1. `git clone --recursive https://github.com/hzqst/MetaHookSv` to somewhere that doesn't contain space in the directory path.

2. Run `scripts\build-MetaHook.bat`, wait for metahook exe to generate. The generated exe should be under `Build` if no error(s) occurs.

3. Run `scripts\build-Plugins.bat`, wait for all plugins to generate. All generated dlls should be under `Build\svencoop\metahook\plugins\` if no error(s) occurs.

## Debugging

1. `git clone --recursive https://github.com/hzqst/MetaHookSv` to somewhere that doesn't contain space in the directory path.

2. Run `scripts\debug-SvenCoop.bat` (or `scripts\debug-(WhateverGameYouWant).bat`, depends on which you are going to debug with)

3. Open `MetaHook.sln` with Visual Studio IDE, set specified project as launch project, compile the project, then press F5 to start debugging.

* Other games follow the same instruction.

* You should restart Visual Studio IDE to apply changes to debugging profile if Visual Studio IDE was running.

## MetaHook Docs

[Docs](docs/MetaHook.md) [中文文档](docs/MetaHookCN.md)

## Plugins

### CaptionMod

A subtitle plugin designed for displaying subtitles and translate in-game HUD text.

[DOC](docs/CaptionMod.md) [中文文档](docs/CaptionModCN.md)

![](/img/1.png)

### BulletPhysics

A plugin that transform player model into ragdoll when player is dead or being caught by barnacle.

[DOC](docs/BulletPhysics.md) [中文文档](docs/BulletPhysicsCN.md)

![](/img/6.png)

### MetaRenderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

[DOC](docs/Renderer.md) [中文文档](docs/RendererCN.md)

![](/img/3.png)

### StudioEvents

This plugin can block studio-event sound spamming with controllable cvars.

[DOC](docs/StudioEvents.md) [中文文档](docs/StudioEventsCN.md)

![](/img/8.png)

### SteamScreenshots (Sven Co-op only)

This plugin intercepts `snapshot` command and replace it with `ISteamScreenshots` interface which will upload the snapshot to Steam Screenshot Manager.

### SCModelDownloader (Sven Co-op only)

This plugin downloads missing player models from https://wootguy.github.io/scmodels/ automatically and reload them once mdl files are ready.

Cvar : `scmodel_autodownload 0 / 1` Automatically download missing model from scmodel database.

Cvar : `scmodel_downloadlatest 0 / 1` Download latest version of this model if there are multiple ones with different version.

Cvar : `scmodel_usemirror 0 / 1` Use mirror (cdn.jsdelivr.net) if github is not available.

### CommunicationDemo (Sven Co-op only)

This plugin exposes an interface to communicate with Sven Co-op server.

### DontFlushSoundCache (Sven Co-op only) (Experimental)

* NOT READY FOR NON-DEVs

This plugin prevents client from flushing soundcache at `retry` (engine issues `retry` command everytime when HTTP download progress is finished), make it possible to preserve soundcache txt downloaded from fastdl resource server (sv_downloadurl).

The fastdl procedure only works when game server uploads current map's soundcache txt to the fastdl resource server. (I am using AliyunOSS)

The reason why I made this plugin is because transfering soundcache txt via UDP netchannel is really not a good idea as server bandwidth and file IO resource is expensive.

### PrecacheManager

This plugin provides a console command `fs_dump_precaches` to dump precache resource list into `[ModDirectory]\maps\[mapname].dump.res`.

* The SoundSystem from Sven Co-op uses `soundcache.txt` instead of engine's precache system to precache sound files.

### ThreadGuard

This plugin intercepts Valve's Win32Thread creation and wait for all threads to exit before unloading the backed module, in case Valve's thread code gets running when the backed module is unloaded.

Managed modules that may create new threads and quit without waiting for thread termination : 

`hw.dll`, `GameUI.dll`, `ServerBrowser.dll`

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

https://github.com/DrAbcrealone/HUDColor

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

### ModelViewer (third-party) (Sven Co-op only)

https://github.com/surf082/ModelViewer

This is a plugin for Sven Co-op that adds a button to the option menu to open the modelviewer for player model preview.

### ServerFilter (third-party) (GoldSrc only)

https://github.com/surf082/ServerFilter

This is a plugin for GoldSrc Games that provides server filter function.
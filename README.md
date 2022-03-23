# MetaHookSv

This is a porting of [MetaHook](https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven Co-op Team),

Mainly to keep you a good game experience in Sven Co-op.

Most of plugins are still compatible with vanilla GoldSrc engine. please check plugin docs for compatibility.

[中文README](READMECN.md)

## Risk of VAC ?

Although using hook is likely to be dangerous in games, there is no VAC ban reported yet.

Btw Sven Co-op is not a [game protected by VAC](https://store.steampowered.com/search/?term=Sven&category2=8)

You can even connect to a "VAC protected" server with `-insecure`, as VAC is not working at all in Sven Co-op.

Use a separate account to play Sven Co-op if you still worry about VAC ban wave. Sven Co-op is a free game.

## Manual Installation

1. git pull https://github.com/hzqst/MetaHookSv or download from https://github.com/hzqst/MetaHookSv/archive/main.zip

2. All required executable and resource files are in `Build` folder, pick [whatever resource you want](Build/README.md) and copy them to `\SteamLibrary\steamapps\common\Sven Co-op\`.

3. (Optional) Rename `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\plugins_svencoop.lst` to `plugins.lst`

4. Launch game from Steam Game Library or `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe`

* The new `svencoop.exe` is renamed from `metahook.exe`, you could run game from "metahook.exe -game svencoop" however it will cause game crash when changing video settings.

* Plugins can be disabled or enabled in `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\plugins.lst`

* The `SDL2.dll` fixes a bug that the IME input handler from original SDL library provided by Valve was causing buffer overflow and game crash when using non-english IME. you don't need to copy it if you don't have a non-english IME.

## One Click Installation

1. git pull https://github.com/hzqst/MetaHookSv or download from https://github.com/hzqst/MetaHookSv/archive/main.zip

2. Run `install-to-SvenCoop.bat`

3. Launch game from shortcut `MetaHook for SvenCoop` or `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe`

* Other games follow the same instruction.

* You should have your Steam running otherwise the [SteamAppsLocation](SteamAppsLocation/README.md) will probably not going to find GameInstallDir.

## Build Requirements

1. Visual Studio 2017 or 2019, with vc141 or vc142 toolset.

2. CMake

3. git client

## Build Instruction

Let's assume that you have all requirements installed correctly.

1. git clone https://github.com/hzqst/MetaHookSv

2. Run `build-initdeps.bat`, wait until all required submodules / dependencies are pulled. (this may takes couple of minutes, depending on your network connection and download speed)

3. Run `build-MetaHook.bat`, wait until `svencoop.exe` generated at `Build` directory.

4. Run `build-(SpecifiedPluginName).bat`, wait until `(SpecifiedPluginName).dll` generated. Current available plugins : CaptionMod.dll, CaptionMod, Renderer, StudioEvents, SteamScreenshots, SCModelDownloader, CommunicationDemo.

8. If generated without problem, plugins should be at `Build\svencoop\metahook\plugins\` directory.

## Debugging

1. git clone https://github.com/hzqst/MetaHookSv (Ignore if you have already done this before)

2. Run `build-initdeps.bat`, wait until all required submodules / dependencies are pulled.  (Ignore if you have already done this before) (this may takes couple of minutes, depending on your network connection and download speed)

3. Run `debug-SvenCoop.bat` (or `debug-(WhateverGameYouWant).bat`, depends on which you are going to debug with)

4. Open `MetaHook.sln` with Visual Studio IDE, set specified project as launch project, compile the project, then press F5 to start debugging.

* Other games follow the same instruction.

* You should restart Visual Studio IDE to apply changes to debugging profile, if Visual Studio IDE was running.

## MetaHookSv (V3) new features compare to nagist's old metahook (V2)

1. New and better capstone API to disassemble and analyze engine code, new API to reverse search the function begin.

2. Blocking duplicate plugins (which may introduce infinite-recursive calling) from loading.

3. A transaction will be there at stage `LoadEngine` and `LoadClient`, to prevent `InlineHook` issued by multiple plugins from immediately taking effect. Allowing multiple plugins to `SearchPattern` and `InlineHook` same function without conflict.

## Load Order

1. `\(GameDirectory)\metahook\configs\plugins_avx2.lst` (Only when AVX2 instruction set is supported)
2. `\(GameDirectory)\metahook\configs\plugins_svencoop_avx2.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_avx2.lst` (Only when AVX2 instruction set is supported)

3. `\(GameDirectory)\metahook\configs\plugins_avx.lst` (Only when AVX instruction set is supported)
4. `\(GameDirectory)\metahook\configs\plugins_svencoop_avx.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_avx.lst` (Only when AVX instruction set is supported)

5. `\(GameDirectory)\metahook\configs\plugins_sse2.lst` (Only when SSE2 instruction set is supported)
6. `\(GameDirectory)\metahook\configs\plugins_svencoop_sse2.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_sse2.lst` (Only when SSE2 instruction set is supported)

7. `\(GameDirectory)\metahook\configs\plugins_sse.lst` (Only when SSE instruction set is supported)
8. `\(GameDirectory)\metahook\configs\plugins_svencoop_sse.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_sse.lst` (Only when SSE instruction set is supported)

9. `\(GameDirectory)\metahook\configs\plugins.lst`
10. `\(GameDirectory)\metahook\configs\plugins_svencoop.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc.lst`

* MetahookSv loader will load the first available plugin list file, then load the dll(s) listed in the file.

## Plugins

### CaptionMod

A subtitle plugin designed for displaying subtitles and translate in-game HUD text.

[DOCUMENTATION](CaptionMod.md) [中文文档](CaptionModCN.md)

![](/img/1.png)

### BulletPhysics

A plugin that transform player model into ragdoll when player is dead or being caught by barnacle.

[DOCUMENTATION](BulletPhysics.md) [中文文档](BulletPhysicsCN.md)

![](/img/6.png)

### MetaRenderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

[DOCUMENTATION](Renderer.md) [中文文档](RendererCN.md)

![](/img/3.png)

### StudioEvents

This plugin can block studio-event sound spamming with controllable cvars.

[DOCUMENTATION](StudioEvents.md) [中文文档](StudioEventsCN.md)

![](/img/8.png)

### SteamScreenshots (Sven Co-op only)

This plugin intercepts `snapshot` command and replace it with `ISteamScreenshots` interface which will upload the snapshot to Steam Screenshot Manager.

### SCModelDownloader (Sven Co-op only)

This plugin downloads missing player models from https://wootguy.github.io/scmodels/ automatically and reload them once mdl files are ready.

Cvar : `scmodel_autodownload 0 / 1`

### CommunicationDemo (Sven Co-op only)

This plugin exposes an interface to communicate with Sven Co-op server.

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

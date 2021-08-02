# MetaHookSv

This is a porting of [MetaHook](https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven Co-op Team),

Mainly to keep you a good game experience in Sven Co-op.

Most of plugins are still compatible with vanilla GoldSrc engine. please check plugin docs for compatibility.

[中文README](READMECN.md)

## Risk of VAC ?

Although using hook is likely to be dangerous in games, there is no VAC ban reported yet.

Btw Sven Co-op is not a [game that protected by VAC](https://store.steampowered.com/search/?term=Sven&category2=8)

Use a separate account to play Sven Co-op if you worry about getting banned, since Sven Co-op is a free game.

## Manual Installation

1. git pull https://github.com/hzqst/MetaHookSv or download from https://github.com/hzqst/MetaHookSv/archive/main.zip

2. All required executable and resource files are in `Build` folder, pick [whatever resource you want](Build/README.md) and copy them to `\SteamLibrary\steamapps\common\Sven Co-op\`.

3. Launch game from `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe`

* The new `svencoop.exe` is renamed from `metahook.exe`, you could run game from "metahook.exe -game svencoop" however it will cause game crash when changing video settings.

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

4. Run `build-CaptionMod.bat`, wait until `CaptionMod.dll` generated.

5. Run `build-Renderer.bat`, wait until `Renderer.dll` generated.

6. Run `build-BulletPhysics.bat`, wait until `BulletPhysics.dll` generated.

7. Run `build-StudioEvents.bat`, wait until `StudioEvents.dll` generated.

8. All generated plugins should be at `Build\svencoop\metahook\plugins\` directory.

## Debugging

1. Run `debug-SvenCoop.bat`

2. Open `MetaHook.sln` with Visual Studio IDE, set specified project as launch project, compile the project, then press F5 to start debugging.

* Other games follow the same instruction.

* You should restart Visual Studio IDE to apply changes to debugging profile, if it is running.

## MetaHookSv (V3) new features compare to nagist's old metahook (V2)

1. New and better capstone API to disassemble and analyze engine code, new API to reverse search the function begin.

2. Blocking duplicate plugins (which may introduce infinite-recursive calling) from loading.

3. A transaction will be there at stage `LoadEngine` and `LoadClient`, to prevent `InlineHook` issued by multiple plugins from immediately taking effect. Allowing multiple plugins to `SearchPattern` and `InlineHook` same function without conflict.

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

### HUDColor (third-party) (SvEngine only)

This plugin basically changes HUD colors in game.

https://github.com/DrAbcrealone/HUDColor

### MetaAudio (third-party) (GoldSrc only)

This is a plugin for GoldSrc that adds OpenAL support to its sound system. This fork fixes some bugs and uses Alure instead of OpenAL directly for easier source management.

Since SvEngine uses FMOD as it's sound system, you really shouldn't use this plugin in Sven Co-op.

https://github.com/LAGonauta/MetaAudio

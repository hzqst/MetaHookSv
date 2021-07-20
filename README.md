# MetaHookSv

This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop Team),

Mainly to keep you a good game experience in Sven-Coop.

Most of plugins are still compatible with vanilla GoldSrc engine. please check plugin docs for compatibility.

[中文README](READMECN.md)

## Risk of VAC ?

Although using hook is likely to be dangerous in games, there is no VAC ban reported yet.

As you can see Sven-Coop is not protected by VAC : https://store.steampowered.com/search/?term=coop&category2=8

Use a separate account to play Sven-Coop if you worry about getting banned, since Sven-Coop is a free game.

## Installation

0. git pull https://github.com/hzqst/MetaHookSv or download from https://github.com/hzqst/MetaHookSv/archive/main.zip

1. All required executable and resource files are in "Build" folder, pick [whatever resource you need](Build/README.md) and copy them to "\SteamLibrary\steamapps\common\Sven Co-op\".

2. Launch game from "\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe"

* The new "svencoop.exe" is renamed from "metahook.exe", you could run game from "metahook.exe -game svencoop" however it will cause game crash when changing video settings.

* The SDL2.dll fixes a bug that the IME input handler from original SDL library provided by valve was causing buffer overflow and game crash when using non-english IME. you don't need to copy it if you don't have a non-english IME.

## Build Requirements

1. Visual Studio 2017 or higher, with C++ (MSVC toolset).

2. CMake

3. git client

## Build Instruction

Let's assume that you have all requirements installed correctly.

1. git clone https://github.com/hzqst/MetaHookSv

2. Run init-deps.bat, wait until all required submodules / dependencies are pulled. (this may takes couple of minutes, depending on your network connection and download speed)

3. Run "build-capstone.bat", wait until capstone's static lib(s) are generated.

4. Run "build-bullet3.bat", wait until bullet3's static lib(s) are generated.

5. Run "build-MetaHook.bat", wait until "svencoop.exe" is generated under "Build" directory. Remember to redirect the Windows SDK version from Visual Studio IDE if you have installed a different version of Windows SDK other than "10.0.17763.0".

6. Run "build-CaptionMod.bat", wait until "CaptionMod.dll" is generated under "Build\svencoop\metahook\plugins\" directory.

7. Run "build-Renderer.bat", wait until "Renderer.dll" is generated under "Build\svencoop\metahook\plugins\" directory.

8. Run "build-BulletPhysics.bat", wait until "BulletPhysics.dll" is generated under "Build\svencoop\metahook\plugins\" directory.

9. Run "build-StudioEvents.bat", wait until "StudioEvents.dll" is generated under "Build\svencoop\metahook\plugins\" directory.

## Plugins

### CaptionMod

A subtitle plugin designed for displaying subtitles and translate in-game HUD text.

[DOCUMENTATION](CaptionMod.md) [中文文档](CaptionModCN.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

### BulletPhysics

A plugin that transform player model into ragdoll when player is dead or being caught by barnacle.

[DOCUMENTATION](BulletPhysics.md) [中文文档](BulletPhysicsCN.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

### MetaRenderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

[DOCUMENTATION](Renderer.md) [中文文档](RendererCN.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

### StudioEvents

This plugin can block studio-event sound spamming with controllable cvars.

[DOCUMENTATION](StudioEvents.md) [中文文档](StudioEventsCN.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/8.png)

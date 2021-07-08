# MetaHookSv

This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop Team), 

mainly to keep you a good game experience in Sven-Coop.

It is currently not compatible with original GoldSrc engine, but it can be if broken signatures are fixed at future.

[中文README](READMECN.md)

## Risk of VAC ?

Although using hook is likely to be dangerous in games, there is no VAC ban reported yet.

As you can see Sven-Coop is not protected by VAC : https://store.steampowered.com/search/?term=coop&category2=8

Use a separate account to play Sven-Coop if you worry about getting banned, since Sven-Coop is a free game.

## Installation

0. git pull https://github.com/hzqst/MetaHookSv or download from https://github.com/hzqst/MetaHookSv/archive/main.zip

1. All required binaries and files are in "Build" folder, copy them to "\SteamLibrary\steamapps\common\Sven Co-op\".

2. Launch game from "\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe"

* The new "svencoop.exe" is renamed from "metahook.exe", you could run game from "metahook.exe -game svencoop" however it will cause game crash when changing video settings.

* The SDL2.dll fixes a bug that the IME input handler from original SDL library provided by valve was causing buffer overflow and game crash when using non-english IME. you don't need to copy it if you don't have a non-english IME.

## Build Instruction

1. git clone https://github.com/hzqst/MetaHookSv

2. Open "MetaHook.sln" with MSVC 2015 or MSVC 2017 (not tested yet with MSVC2019)

3. Build with Release configuration

4. Open "Plugins\CaptionMod\CaptionMod.sln" with MSVC 2015 or MSVC 2017 (not tested yet with MSVC2019)

5. Build with Release configuration

6. Open "Plugins\Renderer\Renderer.sln" with MSVC 2015 or MSVC 2017 (not tested yet with MSVC2019)

7. Build with Release configuration

9. If there is no compile error(s), all binaries should be generated under "Build" directory.

## Plugins

### CaptionMod

A subtitle plugin designed for displaying subtitles and translate in-game HUD text.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

[DOCUMENTATION](CaptionDoc.md)

### BulletPhysics

Bullet physics engine are introduced to perform ragdoll simulatation.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

[DOCUMENTATION](BPhysicsDoc.md)

### MetaRenderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/2.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/4.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/5.png)

[DOCUMENTATION](RendererDoc.md)
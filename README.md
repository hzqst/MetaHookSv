# MetaHookSv

This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop Team).

mainly to keep you a good game experience in Sven-Coop.

Most of the plugins are still compatible with vanilla GoldSrc engine. check plugin doc for compatibility.

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

## Build Instruction (launcher only, not plugins)

1. git clone https://github.com/hzqst/MetaHookSv

2. git clone https://github.com/aquynh/capstone somewhere if you don't have capstone installed in your dev environment.

3. Edit "MetaHook.vcxproj" and change `<CapstonePath>I:\code\capstone</CapstonePath>` to your capstone install path.

4. Open "MetaHook.sln"

5. Build with Release configuration

### Compatibility (launcher only, not plugins)

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | √    |
| GoldSrc_legacy (< 6153)  | √    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

## Plugins

### CaptionMod

A subtitle plugin designed for displaying subtitles and translate in-game HUD text.

[DOCUMENTATION](CaptionDoc.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

### BulletPhysics

Bullet physics engine are introduced to perform ragdoll simulatation.

[DOCUMENTATION](BPhysicsDoc.md) [中文文档](BPhysicsDocCN.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

### MetaRenderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

[DOCUMENTATION](RendererDoc.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

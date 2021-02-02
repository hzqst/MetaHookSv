# MetaHookSv
This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop), and it is still compatible with original GoldSrc engine.

Plugin porting is in progress as most signatures/patterns for GoldSrc engine are failed for SvEngine.

## Installation

All pre-compiled binary and required files are in "Build" folder, copy them to "\SteamLibrary\steamapps\common\Sven Co-op\".

and launch game from "\SteamLibrary\steamapps\common\Sven Co-op\" with commandline "metahook.exe -game svencoop"

the SDL2.dll fixes a bug that the original SDL's IME input handler was causing buffer overflow and game crash.

## Plugins

### FuckWorld

A simple demo plugin that pops MessageBox when load.

Current state : Ready to use.

### CaptionMod

A subtitle plugin designed for displaying subtitles in VGUI2 based games.

check https://github.com/hzqst/CaptionMod for detail.

Current state : Ready to use.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

### Renderer

A graphic enhancement plugin that modifiy the original render system.

Current state : Ready to use, more feature are coming soon.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/2.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/4.png)

#### Features

1. HDR (high-dynamic-range) post-processor.

2. Water reflection and refraction.

3. Per-Object Shadow. (each object render it's own shadow mapping just like how source engine does this.)

#### Console Vars

r_hdr 1 / 0 : to enable / disable HDR(high-dynamic-range) post-processor.

r_water 1 / 0 : to enable / disable water reflection and refraction.

r_water_fresnel (0.0 ~ 2.0) : to determine how to lerp and mix the refraction color and reflection color.

r_water_depthfactor (0.0 ~ 1000.0) : to determine if we can see through water in a close distance.

r_water_normfactor (0.0 ~ 1000.0) : to determine the size of water wave (offset to the normalmap).

r_shadow 1 / 0 : to enable / disable per-object shadow rendering.

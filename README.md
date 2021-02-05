# MetaHookSv
This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop), and it is still compatible with original GoldSrc engine.

Plugin porting is in progress as most signatures/patterns for GoldSrc engine are failed for SvEngine.

* You must shutdown and restart the game manually after changing the video setting, or the game may crash.

## Installation

All pre-compiled binary and required files are in "Build" folder, copy them to "\SteamLibrary\steamapps\common\Sven Co-op\".

and launch game from "\SteamLibrary\steamapps\common\Sven Co-op\metahook.exe"

the SDL2.dll fixes a bug that the original SDL's IME input handler was causing buffer overflow and game crash. you don't need to copy it if you don't have a non-english IME.

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

A graphic enhancement plugin that modifiy the original render engine.

Current state : Ready to use, more feature are coming soon.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/2.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/4.png)

#### Features

1. High-Dynamic-Range (HDR) post-processor.

2. Water reflection and refraction.

3. Per-Object Shadow. (each object render it's own shadow mapping just like how source engine does this.)

4. Screen Space Ambient Occlusion (SSAO) using horizon-based ambient occlusion (HBAO). the implementation is taken from nvidia.

5. MultiSampling Anti-Aliasing (MSAA)

#### Launch Parameters / Commmandline Parameters

-nofbo : disable FrameBufferObject rendering. add it if you caught some rendering error.

-nomsaa : disable MultiSampling Anti-Aliasing (MSAA).

-nohdr : disable High-Dynamic-Range (HDR).

-directblit : force to blit the FrameBufferObject to screen.

-nodirectblit : force to render backbuffer as a textured quad to screen.

-hdrcolor 8/16/32 : set the HDR internal framebufferobject/texture color.

-msaa 4/8/16 : set the sample count of MSAA.

-water_texture_size : set the maximum texture size for water reflect/refract view rendering.

#### Console Vars

r_hdr 1 / 0 : to enable / disable HDR(high-dynamic-range) post-processor.

r_hdr_blurwidth : to control the intensity of blur for HDR.

r_hdr_exposure : to control the intensity of exposure for HDR.

r_hdr_darkness : to control the darkness for HDR.

r_hdr_adaptation : to control the dark / bright adaptation speed for HDR.

r_water 1 / 0 : to enable / disable water reflection and refraction.

r_water_fresnel (0.0 ~ 2.0) : to determine how to lerp and mix the refraction color and reflection color.

r_water_depthfactor (0.0 ~ 1000.0) : to determine if we can see through water in a close distance.

r_water_normfactor (0.0 ~ 1000.0) : to determine the size of water wave (offset to the normalmap).

r_water_novis 1 / 0 : force engine to render the scene which should have been removed by visleaf when rendering refract or reflect view.

r_shadow 1 / 0 : to enable / disable Per-Object Shadow.

r_shadow_angle_pitch (0.0 ~ 360.0) : to control the angle(pitch) of shadow caster (light source).

r_shadow_angle_yaw (0.0 ~ 360.0) : to control the angle(yaw) of shadow caster (light source).

r_shadow_angle_roll (0.0 ~ 360.0) : to control the angle(roll) of shadow caster (light source).

r_shadow_texsize (must be power of 2) : the texture size of shadow map. larger texture supports bigger shadow-caster entity but uses more graphic RAM.

r_shadow_scale (must be power of 2) : scale factor when render shadow-caster entity in shadow map. larger scale factor gets better quality shadow but will cause incorrect render result when the entity is scaled too much.

r_shadow_fardist (0.0 ~ 1000.0) : to determine how far the shadow is going to fade out.

r_shadow_radius (0.0 ~ 1000.0) : entity don't cast shadow at this distance away from surface.

r_ssao 1 / 0 : to enable / disable Screen Space Ambient Occlusion.

r_ssao_intensity : to control the intensity of SSAO shadow.

r_ssao_radius : to control the size of SSAO shadow.

r_ssao_blur_sharpness : to control the sharpness of SSAO shadow.

r_ssao_bias : test it yourself.

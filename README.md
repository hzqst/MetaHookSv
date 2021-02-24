# MetaHookSv

This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop Team).

It is currently not compatible with original GoldSrc engine, but it can be if broken signatures are fixed at future.

## Installation

1. All pre-compiled binary and required files are in "Build" folder, copy them to "\SteamLibrary\steamapps\common\Sven Co-op\".

2. Launch game from "\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe"

* The new "svencoop.exe" is original called "metahook.exe", you can also run game from "metahook.exe -game svencoop" however it will cause game crash when changing video settings.

* The SDL2.dll fixes a bug that the original SDL's IME input handler was causing buffer overflow and game crash. you don't need to copy it if you don't have a non-english IME.

## Plugins

### FuckWorld

A simple demo plugin that pops MessageBox when load.

Current state : Ready to use.

### CaptionMod

A subtitle plugin designed for displaying subtitles in VGUI2 based games.

check https://github.com/hzqst/CaptionMod for detail.

#### Features

1. display subtitles when sound is played.

2. display subtitles when sentence is played.

3. display subtitles when there is a HUD TextMessage.

4. hook original client's HUD TextMessage and draw it with multi-byte character support. (new and only for SvEngine)

4. hook VGUI1 TextImage control paint procedure and draw it with multi-byte character support. (new and only for SvEngine)

Current state : Ready to use.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

### Renderer

A graphic enhancement plugin that modifiy the original render engine.

Current state : Ready to use, more feature are coming soon.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/2.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/4.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/5.png)

#### Features

1. High-Dynamic-Range (HDR) post-processor.

2. Water reflection and refraction. (Warning: this may cause a significant performance hit.)

3. Per-Object Shadow. (Warning: this may cause a significant performance hit when there are too many epolys in the view.)

4. Screen Space Ambient Occlusion (SSAO) using horizon-based ambient occlusion (HBAO). the implementation is taken from nvidia. (not support with -nofbo) (Warning: this may cause a significant performance hit when sampling radius is too large.)

5. MultiSampling Anti-Aliasing (MSAA)

6. Fast Approximate Anti-Aliasing (FXAA) when MSAA is not available.

7. Deferred-Shading and Per-Pixel-Lighting technique for all non-transparent objects. "unlimited" (maximum at 256 for SvEngine) dynamic lightsources are supported now. (not support with -nofbo)

9. Vertex-Buffer-Object (VBO) and GPU-Light-Shading technique for studio model. With VBO enabled you will get higher framerate and lower CPU usage. You can get maximum at 6x FramePerSeconds than non-VBO mode in extreme case (200k epolys, 6 wpolys).

#### Todo List

1. BSP terrain rendering optmization

2. Normal Texture? Texture Replacer?

3. StudioModel Decal

4. Ragdoll Engine

5. Particle System

#### Launch Parameters / Commmandline Parameters

-nofbo : disable FrameBufferObject rendering. add it if you caught some rendering error. SSAO and Deferred-Shading will not be available when FBO disabled.

-nomsaa : disable MultiSampling Anti-Aliasing (MSAA). add it if you caught some performance hit.

-nohdr : disable High-Dynamic-Range (HDR).

-directblit : force to blit the FrameBufferObject to screen.

-nodirectblit : force to render backbuffer as a textured quad to screen.

-hdrcolor 8/16/32 : set the HDR internal framebufferobject/texture color.

-msaa 4/8/16 : set the sample count of MSAA.

#### Console Vars

r_hdr 1 / 0 : to enable / disable HDR(high-dynamic-range) post-processor.

r_hdr_blurwidth : to control the intensity of blur for HDR.

r_hdr_exposure : to control the intensity of exposure for HDR.

r_hdr_darkness : to control the darkness for HDR.

r_hdr_adaptation : to control the dark / bright adaptation speed for HDR.

r_water 2 / 1 / 0 : to enable / disable water reflection and refraction. 2 = all entities and terrains in reflection view, 1 = only terrains in reflection view.

r_water_fresnel (0.0 ~ 2.0) : to determine how to lerp and mix the refraction color and reflection color.

r_water_depthfactor (0.0 ~ 1000.0) : to determine if we can see through water in a close distance.

r_water_normfactor (0.0 ~ 1000.0) : to determine the size of water wave (offset to the normalmap).

r_water_novis 1 / 0 : force engine to render the scene which should have been removed by visleaf when rendering refraction or reflection view.

r_water_texscale (0.1 ~ 1.0) : to control the size of refract or reflect view texture.

r_water_minheight : water entity which has height < this value will not be rendered with shader program.

r_shadow 1 / 0 : to enable / disable Per-Object Shadow.

r_shadow_angle_pitch (0.0 ~ 360.0) : to control the angle(pitch) of shadow caster (light source).

r_shadow_angle_yaw (0.0 ~ 360.0) : to control the angle(yaw) of shadow caster (light source).

r_shadow_angle_roll (0.0 ~ 360.0) : to control the angle(roll) of shadow caster (light source).

r_shadow_high_texsize (must be power of 4) : the texture size of high-quality shadow map. larger texture with bigger scale factor has better quality but uses more graphic RAM.

r_shadow_high_distance : entities within this distance are rendered into high-quality shadow map.

r_shadow_high_scale : scale factor when render shadow-caster entity in high-quality shadow map. larger scale factor gets better quality shadow but will cause incorrect render result when the entity is scaled too much.

r_shadow_medium_texsize (must be power of 4) : the texture size of medium-quality shadow map.

r_shadow_medium_distance : entities within this distance are rendered into medium-quality shadow map.

r_shadow_medium_scale : scale factor when render shadow-caster entity in low-quality shadow map.

r_shadow_low_texsize (must be power of 4) : the texture size of low-quality shadow map.

r_shadow_low_distance : entities within this distance are rendered into low-quality shadow map.

r_shadow_low_scale : scale factor when render shadow-caster entity in medium quality shadow map.

r_ssao 1 / 0 : to enable / disable Screen Space Ambient Occlusion.

r_ssao_intensity : to control the intensity of SSAO shadow.

r_ssao_radius : to control the sample size of SSAO shadow.

r_ssao_blur_sharpness : to control the sharpness of SSAO shadow.

r_ssao_bias : test it yourself.

r_light_dynamic : to enable / disable Deferred-Shading (Dynamic-LightSource support).

r_flashlight_cone : cosine of angle of flashlight cone.

r_flashlight_distance : flashlight's illumination distance.

r_light_ambient : ambient intensity of dynamic light.

r_light_diffuse : diffuse intensity of dynamic light.

r_light_specular : specular intensity of dynamic light.

r_light_specularpow : specular power of dynamic light.

r_studio_vbo 1 / 0 : enable / disable VBO rendering for studio model.

r_fxaa 1 / 0  : enable / disable Fast Approximate Anti-Aliasing (FXAA) when MSAA is not available.

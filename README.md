# MetaHookSv

This is a porting of MetaHook (https://github.com/nagist/metahook) for SvEngine (GoldSrc engine modified by Sven-Coop Team), 

mainly to keep you a good game experience in Sven-Coop.

It is currently not compatible with original GoldSrc engine, but it can be if broken signatures are fixed at future.

## Installation

1. All pre-compiled binary and required files are in "Build" folder, copy them to "\SteamLibrary\steamapps\common\Sven Co-op\".

2. Launch game from "\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe"

* The new "svencoop.exe" is original called "metahook.exe", you can also run game from "metahook.exe -game svencoop" however it will cause game crash when changing video settings.

* The SDL2.dll fixes a bug that the original SDL's IME input handler was causing buffer overflow and game crash. you don't need to copy it if you don't have a non-english IME.

## Plugins

### CaptionMod

A subtitle plugin designed for displaying subtitles and translate in-game text in VGUI1 or VGUI2 based games.

#### Features

1. Display subtitles when sound is played.

2. Display subtitles when sentence is played.

3. Display subtitles when there is a HUD TextMessage.

4. Translate HUD TextMessage dynamically (regex supported).

5. Hook original client's old-style HUD TextMessage and draw it with multi-byte character support.

6. Hook VGUI1's TextImage control paint procedure and draw it with multi-byte character support.

7. Custom dictionary for each map, put dictionary file at "/maps/[mapname]_dictionary.csv"

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

There is a example demo shows you how to translate dynamic HUD TextMessage into other language with regex in "svencoop\captionmod\dictionary_english.txt" called "#SVENCOOP_PLAYERINFO"

There is a example demo shows you how to translate constant HUD TextMessage into other language in "svencoop\maps\restrictionXX_dictionary.csv"

### Renderer

A graphic enhancement plugin that modifiy the original render engine.

You can even play with 200k epolys models and still keep a high framerate.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/2.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/4.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/5.png)

#### Features

1. High-Dynamic-Range (HDR) post-processor.

2. Simple water reflection and refraction. (Warning: this may cause a significant performance hit.)

3. Simple Per-Object Shadow. (Warning: this may cause a significant performance hit.)

4. Screen Space Ambient Occlusion (SSAO) using horizon-based ambient occlusion (HBAO). the implementation is taken from nvidia. (not support with -nofbo) (Warning: this may cause a significant performance hit when sampling radius is too large.)

5. MultiSampling Anti-Aliasing (MSAA)

6. Fast Approximate Anti-Aliasing (FXAA) when MSAA is not available.

7. Deferred-Shading and Per-Pixel-Dynamic-Lighting for all non-transparent objects. "unlimited" (maximum at 256 for SvEngine) dynamic lightsources are supported now  with almost no cost. (not support with -nofbo)

9. Vertex-Buffer-Object (VBO) "Batch-Draw" optimization and GPU-Lighting for studio model. With VBO enabled you will get higher framerate and lower CPU usage. You can get maximum at 8x FramePerSeconds than non-VBO mode in extreme case (200k+ epolys with no FPS drop).

10. Vertex-Buffer-Object (VBO) "Batch-Draw" optimization for BSP terrain. With VBO enabled you will get higher framerate and lower CPU usage. Warning: this feature may cause the render result differs from the one in original game that: random textures are gone, non-visible terrain in current BSP-node are always visible...

#### Todo List

1. StudioModel Decal

2. Ragdoll Engine

3. Particle System

#### Launch Parameters / Commmandline Parameters

-nofbo : disable FrameBufferObject rendering. SSAO and Deferred-Shading will not be available when FBO is disabled.

-nomsaa : disable MultiSampling Anti-Aliasing (MSAA). It is strongly recommended to disable MSAA if you are using SSAO.

-nohdr : disable High-Dynamic-Range (HDR).

-directblit : force to blit the FrameBufferObject to screen.

-nodirectblit : force to render backbuffer as a textured quad to screen.

-hdrcolor 8/16/32 : set the HDR internal framebufferobject/texture color.

-msaa 4/8/16 : set the sample count of MSAA.

#### Console Vars

r_hdr 1 / 0 : to enable / disable HDR(high-dynamic-range) post-processor. recommended value : 1

r_hdr_blurwidth : to control the intensity of blur for HDR. recommended value : 0.1

r_hdr_exposure : to control the intensity of exposure for HDR. recommended value : 5

r_hdr_darkness : to control the darkness for HDR. recommended value : 4

r_hdr_adaptation : to control the dark / bright adaptation speed for HDR. recommended value : 50

r_water 2 / 1 / 0 : to enable / disable water reflection and refraction. 2 = draw all entities and terrains in reflection view, 1 = draw only terrains in reflection view. recommended value : 1

r_water_fresnel (0.0 ~ 2.0) : to determine how to lerp and mix the refraction color and reflection color. recommended value : 1.5

r_water_depthfactor (0.0 ~ 1000.0) : to determine if we can see through water in a close distance. recommended value : 50

r_water_normfactor (0.0 ~ 1000.0) : to determine the size of water wave (offset to the normalmap). recommended value : 1.5

r_water_novis 1 / 0 : force engine to render the scene which should have been removed by visleaf when rendering refraction or reflection view. recommended value : 1

r_water_texscale (0.1 ~ 1.0) : to control the size of refract or reflect view texture. recommended value : 0.5

r_water_minheight : water entity which has height < this value will not be rendered with shader program. recommended value : 7.5

r_shadow 1 / 0 : to enable / disable Per-Object Shadow. recommended value : 1

r_shadow_angle_pitch (0.0 ~ 360.0) : to control the angle(pitch) of shadow caster (light source).

r_shadow_angle_yaw (0.0 ~ 360.0) : to control the angle(yaw) of shadow caster (light source).

r_shadow_angle_roll (0.0 ~ 360.0) : to control the angle(roll) of shadow caster (light source).

r_shadow_high_texsize (must be power of 4) : the texture size of high-quality shadow map. larger texture with bigger scale factor has better quality but uses more graphic RAM. recommended value : 2048

r_shadow_high_distance : entities within this distance are rendered into high-quality shadow map. recommended value : 400

r_shadow_high_scale : scale factor when render shadow-caster entity in high-quality shadow map. larger scale factor gets better quality shadow but will cause incorrect render result when the entity is scaled too much. recommended value : 4.0

r_shadow_medium_texsize (must be power of 4) : the texture size of medium-quality shadow map. recommended value : 2048

r_shadow_medium_distance : entities within this distance are rendered into medium-quality shadow map. recommended value : 1024

r_shadow_medium_scale : scale factor when render shadow-caster entity in low-quality shadow map. recommended value : 2.0

r_shadow_low_texsize (must be power of 4) : the texture size of low-quality shadow map. recommended value : 2048

r_shadow_low_distance : entities within this distance are rendered into low-quality shadow map. recommended value : 4096

r_shadow_low_scale : scale factor when render shadow-caster entity in medium quality shadow map. recommended value : 0.5

r_ssao 1 / 0 : to enable / disable Screen Space Ambient Occlusion. recommended value : 1

r_ssao_intensity : to control the intensity of SSAO shadow. recommended value : 0.6

r_ssao_radius : to control the sample size of SSAO shadow. recommended value : 30.0

r_ssao_blur_sharpness : to control the sharpness of SSAO shadow. recommended value : 1.0

r_ssao_bias : test it yourself. recommended value : 0.2

r_ssao_studio_model : 0 / 1 to enable / disable drawing SSAO shadow on studio model. recommended value : 0

r_light_dynamic : to enable / disable Deferred-Shading (Dynamic-LightSource support). recommended value : 1

r_flashlight_cone : cosine of angle of flashlight cone. recommended value : 0.9

r_flashlight_distance : flashlight's illumination distance. recommended value : 2000.0

r_light_ambient : ambient intensity of dynamic light. recommended value : 0.35

r_light_diffuse : diffuse intensity of dynamic light. recommended value : 0.35

r_light_specular : specular intensity of dynamic light. recommended value : 0.1

r_light_specularpow : specular power of dynamic light. recommended value : 10.0

r_studio_vbo 1 / 0 : enable / disable VBO batch-optmization draw for studio model. recommended value : 1

r_wsurf_vbo 1 / 0 : enable / disable VBO batch-optmization draw for BSP terrain. recommended value : 1

r_fxaa 1 / 0  : enable / disable Fast Approximate Anti-Aliasing (FXAA) when MSAA is not available. recommended value : 1

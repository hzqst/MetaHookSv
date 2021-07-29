# MetaRenderer documentation

[中文DOC](RendererCN.md)

## Features

1. High-Dynamic-Range (HDR) post-processor.

2. Simple water reflection and refraction. (Warning: this may cause a significant performance hit.)

3. Per-Object Dynamic Shadows.

4. Screen Space Ambient Occlusion (SSAO) using horizon-based ambient occlusion (HBAO). the implementation is taken from nvidia. (Warning: this may cause a significant performance hit when sampling radius is too large.) (Warning: you might see SSAO's black pixel fighting on wall or terrain if your defalt_fov is not 90)

5. MultiSampling Anti-Aliasing (MSAA) (High quality, low performance)

6. Fast Approximate Anti-Aliasing (FXAA) when MSAA is not available. (Low quality, high performance)

7. Deferred-Shading and Per-Pixel-Dynamic-Lighting for all non-transparent objects. "unlimited" (maximum at 256 for SvEngine) dynamic lightsources are supported with almost no cost.

9. Vertex-Buffer-Object (VBO) "Batch-Draw" optimization and GPU-Lighting for studio model. With VBO enabled you will get higher framerate and lower CPU usage.

10. Vertex-Buffer-Object (VBO) "Batch-Draw" optimization for BSP terrain and brush model. With VBO enabled you will get higher framerate and lower CPU usage.

11. Normal texture and Parallax texture support for BSP terrain and brush model. Check svencoop/maps/restriction02_detail.txt for usage sample.

12. Fixed an engine (both GoldSrc and SvEngine) bug that STUDIO_NF_MASKED was not working with remapped / colormap textures for studio-models.

![](/img/2.png)

![](/img/3.png)

![](/img/4.png)

![](/img/5.png)

## Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

## Console Vars

r_hdr 1 / 0 : To enable or disable HDR(high-dynamic-range) post-processor. recommended value : 1

r_hdr_blurwidth : To control the intensity of blur for HDR. recommended value : 0.1

r_hdr_exposure : To control the intensity of exposure for HDR. recommended value : 5

r_hdr_darkness : To control the darkness for HDR. recommended value : 4

r_hdr_adaptation : To control the dark / bright adaptation speed for HDR. recommended value : 50

r_water 2 / 1 / 0 : Enable or disable simple water reflection and refraction. 2 = draw all entities and terrains in reflection view, 1 = draw only terrains in reflection view. recommended value : 1

r_water_fresnelfactor (0.0 ~ 1.0) : To determine how to lerp and mix the refraction color and reflection color. recommended value : 0.4

r_water_depthfactor1 (0.0 ~ 1.0) : To determine the strength of water edge feathering. recommended value : 0.02

r_water_depthfactor2 (0.0 ~ 1.0) : To determine the base strength of water edge feathering. recommended value : 0.01

r_water_normfactor (0.0 ~ 1.0) : To determine the size of water wave. recommended value : 1.5

r_water_minheight : Water entity which has height < this value will not be rendered with shader program. recommended value : 7.5

r_shadow 1 / 0 : Enable or disable Per-Object Shadow. recommended value : 1

r_shadow_angle_pitch (0.0 ~ 360.0) : To control the angle(pitch) of shadow caster (light source).

r_shadow_angle_yaw (0.0 ~ 360.0) : To control the angle(yaw) of shadow caster (light source).

r_shadow_angle_roll (0.0 ~ 360.0) : To control the angle(roll) of shadow caster (light source).

r_shadow_high_distance : Entities within this distance are rendered into high-quality shadow map. recommended value : 400

r_shadow_high_scale : Scale factor when render shadow-caster entity in high-quality shadow map. larger scale factor gets better quality shadow but will cause incorrect render result when the entity is scaled too much. recommended value : 4.0

r_shadow_medium_distance : Entities within this distance are rendered into medium-quality shadow map. recommended value : 1024

r_shadow_medium_scale : Scale factor when render shadow-caster entity in low-quality shadow map. recommended value : 2.0

r_shadow_low_distance : Entities within this distance are rendered into low-quality shadow map. recommended value : 4096

r_shadow_low_scale : Scale factor when render shadow-caster entity in medium quality shadow map. recommended value : 0.5

r_ssao 1 / 0 : Enable or disable Screen Space Ambient Occlusion (SSAO). recommended value : 1

(Warning: you might see SSAO's black pixel fighting on wall or terrain if your defalt_fov is not 90. you have to either turn SSAO off or change default_fov back to 90)

r_ssao_intensity : Control the intensity of SSAO shadow. recommended value : 0.6 ~ 1.0

r_ssao_radius : Control the sample size of SSAO shadow. recommended value : 30.0 ~ 100.0

r_ssao_blur_sharpness : Control the sharpness of SSAO shadow. recommended value : 1.0

r_ssao_bias : unknown. recommended value : 0.1 ~ 0.2

r_light_dynamic 1 / 0 : Enable or disable Deferred-Shading. recommended value : 1

r_flashlight_cone : Cosine of angle of flashlight cone. recommended value : 0.9

r_flashlight_distance : Flashlight's illumination distance. recommended value : 2000.0

r_light_ambient : Ambient intensity of dynamic light. recommended value : 0.2

r_light_diffuse : Diffuse intensity of dynamic light. recommended value : 0.3

r_light_specular : Specular intensity of dynamic light. recommended value : 0.1

r_light_specularpow : Specular power of dynamic light. recommended value : 10.0

r_studio_vbo 1 / 0 : Enable or disable VBO batch-optmization draw for studio model. recommended value : 1

r_wsurf_vbo 1 / 0 : Enable or disable VBO batch-optmization draw for BSP terrain. recommended value : 1

r_wsurf_parallax_scale : Parallax textures' intensity factor. recommended value : 0.01 ~ 0.03

r_wsurf_detail 1 / 0 : Enable or disable detail textures, normal textures, parallax textures. recommended value : 1

r_wsurf_sky_occlusion 1 / 0 : When set to 1, scenes occluded by "sky" surfaces (surfaces with sky texture) will be invisible. this only takes effect when r_wsurf_vbo set to 1. recommended value : 1

r_fxaa 1 / 0 : Enable or disable Fast Approximate Anti-Aliasing (FXAA). recommended value : 1

r_msaa 0 / 2 / 4 / 8 / 16 : Enable or disable MultiSampling Anti-Aliasing (MSAA), number >= 2 for MSAA sample count. recommended value : 0 if SSAO enabled or 4 if SSAO disabled.

## Per-Object Dynamic Shadows

Dynamic Shadows are cast only by world models (Players, monsters, weaponbox, corpses, etc), and only onto brush surfaces. They are calculated at runtime, so they are quite crude and relatively expensive to render.

Dynamic Shadows can sometimes project through walls and floors, giving away the location of players or objects. Use info_no_dynamic_shadow to workaround this problem if you encounter it.

## New Entities

### shadow_control

`shadow_control` is a point entity used to control Dynamic Shadow projections for the entire map, including maximum distance cast, direction cast, and shadow color.

#### Available Keyvalues

`angles` is the direction of shadows, in PitchYawRoll format. for example `"angles" "90 0 0"`

`fade` is the distance where the shadow begins to fade-out and the maximum distance the shadow is allowed to cast and, in inches. for example `"fade" "64 128"`

`color` is the color of the shadows, in RGBA8 format. for example `"color" "0 0 0 128"`

`disableallshadows` disables shadows entirely. for example `"disableallshadows" "1"`

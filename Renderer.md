# MetaRenderer documentation

[中文DOC](RendererCN.md)

# Features

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

# Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

# Features

## HDR

HDR (High Dynamic Range) rendering simulates brightness above that which a computer monitor is actually capable of displaying. This mainly involves "blooming" colours above 100% brightness into neighbouring areas, and adjusting a virtual camera aperture to compensate for any over-exposure that results.

### Console vars

`r_hdr` Set to 1 to enable HDR post-processor rendering.

`r_hdr_blurwidth` Controls the intensity of blooming for HDR.

`r_hdr_exposure` Controls the intensity of exposure for HDR.

`r_hdr_darkness` Controls the intensity of darkness for HDR.

`r_hdr_adaptation` Controls the dark / bright adaptation speed for HDR.

## Water Shader

Water Shader basically creates water that realistically reflects and refracts the world.

All water surfaces fall into two types: expensive and cheap.

Expensive water reflects and refracts the whole world in real-time, while cheap water uses a cubemap to reflect.

### Console vars

`r_water` set to 1 to enable reflection and refraction for water. set to 2 to draw all visible entities in reflection (relatively expensive to render), otherwise only BSP world terrains are rendered in reflection.

`r_water_fresnelfactor` controls the intensity of reflection.

`r_water_depthfactor1` controls the strength of water edge feathering.

`r_water_depthfactor2` controls the base strength of water edge feathering.

`r_water_normfactor` controls the intensity of turbulence based on normalmap.

`r_water_minheight` Water entity with height smaller than this value will not be rendered with shader program.

## Per-Object Dynamic Shadow

Dynamic Shadows are cast only by world models (Players, monsters, weaponbox, corpses, etc), and only onto brush surfaces. They are calculated at runtime, so they are quite crude and relatively expensive to render.

Dynamic Shadows can sometimes project through walls and floors, giving away the location of players or objects. Use info_no_dynamic_shadow to workaround this problem if you encounter it.

### Console vars

`r_shadow` set to 1 to enable Per-Object Dynamic Shadow.

`r_shadow_angles` control the direction of shadows, in PitchYawRoll format. for example `r_shadow_angles 90 0 0`

`r_shadow_high_distance` is the maximum distance that entities are being rendered in high-quality shadow map. for example `r_shadow_high_distance 400`

`r_shadow_high_scale` is scale factor to scale the size of entity model up or down in high-quality shadow map. for example `r_shadow_high_scale 4`

`r_shadow_medium_distance` is the maximum distance that entities are being rendered in medium-quality shadow map. for example `r_shadow_medium_distance 800`

`r_shadow_medium_scale` is scale factor to scale the size of entity model up or down in medium-quality shadow map. for example `r_shadow_medium_scale 2`

`r_shadow_low_distance` is the maximum distance that entities are being rendered in low-quality shadow map. for example `r_shadow_low_distance 1200`

`r_shadow_low_scale` is scale factor to scale the size of entity model up or down in low-quality shadow map. for example `r_shadow_low_scale 0.5`

## Screen Space Ambient Occlusion

`r_ssao` Set to 1 to enable SSAO

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

# New Entities

## env_shadow_control

`env_shadow_control` is a point entity used to control Dynamic Shadow projections for the entire map, including maximum distance cast, direction cast, and shadow color.

### Keyvalues

`angles` is the direction of shadows, in PitchYawRoll format. for example `"angles" "90 0 0"`

`fade` is the distance where the shadow begins to fade-out and the maximum distance the shadow is allowed to cast and, in inches. for example `"fade" "64 128"`

`color` is the color of the shadows, in RGBA8 format. for example `"color" "0 0 0 128"`

`disableallshadows` disables shadows entirely. for example `"disableallshadows" "1"`

`high_distance` is the maximum distance that entities are being rendered in high-quality shadow map. for example `"high_distance" "400"`

`high_scale` is scale factor to scale the size of entity model up or down in high-quality shadow map. for example `"high_scale" "4"`

`medium_distance` is the maximum distance that entities are being rendered in medium-quality shadow map. for example `"medium_distance" "800"`

`medium_scale` is scale factor to scale the size of entity model up or down in medium-quality shadow map. for example `"medium_scale" "2"`

`low_distance` is the maximum distance that entities are being rendered in low-quality shadow map. for example `"low_distance" "1200"`

`low_scale` is scale factor to scale the size of entity model up or down in low-quality shadow map. for example `"low_scale" "0.5"`

## env_hdr_controller

env_hdr_controller is a point entity used to controls the HDR effects for local player.

### Keyvalues

`blurwidth` is the intensity of blooming for HDR, for example `"blurwidth" "0.1"`

`exposure` is the intensity of exposure for HDR, forexample `"exposure" "4.5"`

`darkness` is the intensity of darkness for HDR, forexample `"darkness" "4.5"`

`adaptation` is the brightness adaptation speed for HDR, for example `"adaptation" "50"`

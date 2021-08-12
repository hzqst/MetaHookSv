# MetaRenderer documentation

[中文DOC](RendererCN.md)

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

* Changes to some of the cvars will not take effect immediately, using `r_reload` to reload those cvars.

## Water Shader

Water Shader creates water that realistically reflects and refracts the world.

All water surfaces fall into two types: expensive and cheap.

Expensive water reflects and refracts the whole world in real-time using Planar Reflections, which basically renders the entire scene twice. while cheap water only reflects skybox.

### Console vars

`r_water` set to 1 to enable reflection and refraction for water. set to 2 to draw all visible entities in reflection (relatively expensive to render), otherwise only BSP world terrains are rendered in reflection.

## Per-Object Dynamic Shadow

Dynamic Shadows are cast only by world models (players, monsters, weaponbox, corpses, etc), and only onto brush surfaces. They are calculated at runtime, so they are quite crude and relatively expensive to render.

Dynamic Shadows can sometimes project through walls and floors, giving away the location of players or objects.

### Console vars

`r_shadow` set to 1 to enable Per-Object Dynamic Shadow.

`r_shadow_angles` control the direction of shadows, in PitchYawRoll format. for example `r_shadow_angles "90 0 0"`

`r_shadow_color` control the color of shadows, in RGBA8 format. for example `r_shadow_color "0 0 0 128"`

`r_shadow_distfade` is the distance where the shadow begins to fade-out, and the maximum distance the shadow is allowed to cast, in inches. for example `r_shadow_distfade 64 128`

`r_shadow_lumfade` is the luminance the shadow begins to fade-out, and minimum luminance the shadow is allowed to cast, must be between 0 ~ 255. for example `r_shadow_lumfade 64 32`

`r_shadow_high_distance` is the maximum distance that entities are being rendered in high-quality shadow map, in inches. for example `r_shadow_high_distance 400`

`r_shadow_high_scale` is scale factor to scale the size of entity model up or down in high-quality shadow map. for example `r_shadow_high_scale 4`

`r_shadow_medium_distance` is the maximum distance that entities are being rendered in medium-quality shadow map, in inches. for example `r_shadow_medium_distance 800`

`r_shadow_medium_scale` is scale factor to scale the size of entity model up or down in medium-quality shadow map. for example `r_shadow_medium_scale 2`

`r_shadow_low_distance` is the maximum distance that entities are being rendered in low-quality shadow map, in inches. for example `r_shadow_low_distance 1200`

`r_shadow_low_scale` is scale factor to scale the size of entity model up or down in low-quality shadow map. for example `r_shadow_low_scale 0.5`

* Changes to some of the cvars will not take effect immediately, using `r_reload` to reload those cvars.

## Screen Space Ambient Occlusion

SSAO or Screen-Space Ambient Occlusion is the type of post-processing effect that approximating the ambient occlusion effect in real time.

The implementation credits to [HBAO or Horizon-Based-Ambient-Occlusion](https://github.com/nvpro-samples/gl_ssao).

### Console vars

`r_ssao` Set to 1 to enable SSAO

`r_ssao_intensity` control the intensity of SSAO shadow.

`r_ssao_radius` control the sample size of SSAO shadow.

`r_ssao_blur_sharpness` control the sharpness of SSAO shadow.

`r_ssao_bias` bias to avoid incorrect occlusion effect on curved surfaces.

## Deferred Shading and Dynamic Lights

[Deferred-Shading](https://en.wikipedia.org/wiki/Deferred_shading) pipeline is used to render opaque objects, and lights are calculated in real time using [Blinn-Phong](https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model) model.

### Console vars

`r_light_dynamic` set to 1 to enable Deferred Shading and Dynamic Lights.

`r_flashlight_cone` is the cosine of angle of flashlight cone.

`r_flashlight_distance` is the max illumination distance of flashlight.

`r_light_ambient` is ambient intensity of dynamic lights.

`r_light_diffuse` is diffuse intensity of dynamic lights.

`r_light_specular` is specular intensity of dynamic lights.

`r_light_specularpow` is specular power of dynamic lights.

## Screen Space Reflection

Screen Space Reflection or SSR reflect pixels on screen by traces reflection rays in screen space in real time.

Only brush surfaces marked with specular textures (`_SPECULAR` suffix) in `/maps/[map name]_detail.txt` can reflect.

Green channel of `_SPECULAR` texture determines the intensity of reflection. 0 = no reflection, 1 = full reflection.

* Screen Space Reflection only works when `r_light_dynamic` set to 1.

### Console vars

`r_ssr` set to 1 to enable Screen Space Reflection

`r_ssr_ray_step` controls the step length to iterate the ray-marching. for example `r_ssr_ray_step 5.0`

`r_ssr_iter_count` controls the maximum iteration count for ray-marching. for example `r_ssr_iter_count 64`

`r_ssr_distance_bias` ray-marching hits object if distance smaller than this value. for example `r_ssr_distance_bias 0.2`

`r_ssr_adaptive_step` set to 1 to enable Adaptiveg-Step to accelerate the ray-marching procedure. for example `r_ssr_adaptive_step 1`

`r_ssr_exponential_step` set to 1 to enable Exponential-Step to accelerate the ray-marching procedure. for example `r_ssr_exponential_step 1`

`r_ssr_binary_search` set to 1 to enable Binary-Search to accelerate the ray-marching procedure. for example `r_ssr_binary_search 1`

`r_ssr_fade` controls the fade-out effect if the reflected ray hit a pixel close to the screen border. for example `r_ssr_fade "0.8 1.0"`

* Changes to some of the cvars will not take effect immediately, using `r_reload` to reload those cvars.

## Vertex Buffer Object (VBO) "Batch-Draw" optimization

`r_studio_vbo` set to 1 to enable VBO batch-draw optmization for studio model.

`r_wsurf_vbo` set to 1 to enable VBO batch-draw optmization for BSP terrain and brush entities.

## Detail textures

A detail texture is a high resolution external image (Supported format: BMP, TGA, DDS, JPG, PNG) that is placed over the top of a map texture. This gives the impression of a small details when you get up close to a texture instead of the usual blurred image you get when the texture fills the screen.

`r_wsurf_detail` set to 1 to enable detail textures, normal textures, parallax textures and specular textures.

Detail texture list is read from `/maps/[map name]_detail.txt`, with `_DETAIL` as suffix in basetexture name (basetexture with no suffix will be treated as detail texture).

Detail textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

### Normal textures

Normal textures are external images applied to specified brush surfaces and change the direction of surface normal.

Normal texture list is read from `/maps/[map name]_detail.txt`, with `_NORMAL` as suffix in basetexture name.

Normal textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

* Normal textures only work when `r_wsurf_detail` and `r_light_dynamic` both set to 1.

* Normal textures change nothing but the direction of surface normal, thus only work when there are dynamic lights or flashlights.

### Parallax textures

Parallax textures are external images applied to specified brush surfaces which will have more apparent depth.

Parallax texture list is read from `/maps/[map name]_detail.txt`, with `_PARALLAX` as suffix in basetexture name.

Parallax textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

* `r_wsurf_parallax_scale` controls the intensity (and direction if negative value is given) of parallax textures.

### Specular textures

Specular textures are external images applied to specified brush surfaces which will increase the intensity of specularity of surfaces.

Specular texture list is read from `/maps/[map name]_detail.txt`, with `_SPECULAR` as suffix in basetexture name.

Specular textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

* Red channel of specular texture controls the intensity of specular lighting, while green channel of specular texture controls intensity of Screen-Space-Reflection or SSR.

* Blue channel is not used yet.

## Misc

`r_wsurf_sky_occlusion` 1 / 0 : When set to 1, scenes occluded by "sky" surfaces (surfaces with sky texture) will be invisible. this only works when `r_wsurf_vbo` set to 1.

`r_fxaa` set to 1 to enable Fast Approximate Anti-Aliasing (FXAA).

* Since MultiSample Anti-Aliasing (or MSAA) is not able to works with Deferred Shading, it's completely disabled and removed from current version of renderer plugin.

# New Entities

Entities are loaded from two sources : internal and external. BSP entity chunk of current map is loaded as internal, `/maps/(CurrentMapName)_entity.txt` is loaded as external.

If `/maps/(CurrentMapName)_entity.txt` is not found, `/renderer/default_entity.txt` will be loaded instead.

Entity chunks should follow the format :

```
{
"origin" "123 456 789"
"angles" "0 0 0"
"classname" "info_player_start"
}
```

You can use console command `r_reload` to reload entities from both two sources.

You can use [bspguy](https://github.com/wootguy/bspguy) to add entity to BSP file or write entities into `/maps/(MapName)_entity.txt`.

## env_shadow_control

`env_shadow_control` is a point entity used to control Dynamic Shadow projections for the entire map, including maximum distance cast, direction cast, and shadow color.

### Keyvalues

`angles` is the direction of shadows, in PitchYawRoll format. for example `"angles" "90 0 0"`

`distfade` is the distance where the shadow begins to fade-out, and the maximum distance the shadow is allowed to cast, in inches. for example `"distfade" "64 128"`

`lumfade` is the luminance the shadow begins to fade-out, and minimum luminance the shadow is allowed to cast, must be between 0 ~ 255. for example `"lumfade" "64 32"`

`color` is the color of the shadows, in RGBA8 format. for example `"color" "0 0 0 128"`

`disableallshadows` disables shadows entirely. for example `"disableallshadows" "1"`

`high_distance` is the maximum distance that entities are being rendered in high-quality shadow map, in inches. for example `"high_distance" "400"`

`high_scale` is scale factor to scale the size of entity model up or down in high-quality shadow map. for example `"high_scale" "4"`

`medium_distance` is the maximum distance that entities are being rendered in medium-quality shadow map, in inches. for example `"medium_distance" "800"`

`medium_scale` is scale factor to scale the size of entity model up or down in medium-quality shadow map. for example `"medium_scale" "2"`

`low_distance` is the maximum distance that entities are being rendered in low-quality shadow map, in inches. for example `"low_distance" "1200"`

`low_scale` is scale factor to scale the size of entity model up or down in low-quality shadow map. for example `"low_scale" "0.5"`

## env_ssr_control

`env_ssr_control` is a point entity used to control the Screen Space Reflection of SSR effects.

### Keyvalues

`ray_step` controls the step length to iterate the ray-marching, for example `"ray_step" "5.0"`

`iter_count` controls the maximum iteration count for ray-marching. for example `"distance_bias" "0.2"`

`adaptive_step` enable or disable Adaptiveg-Step to accelerate the ray-marching procedure. for example `"adaptive_step" "1"` or `"adaptive_step" "0"`

`exponential_step` enable or disable Exponential-Step to accelerate the ray-marching procedure. for example `"exponential_step" "1"` or `"exponential_step" "0"`

`binary_search` enable or disable Binary-Search to accelerate the ray-marching procedure. for example `"binary_search" "1"` or `"binary_search" "0"`

`fade` controls the fade-out effect if the reflected ray hit a pixel close to the screen border.  for example `"fade" "0.8 1.0"`

## env_hdr_control

`env_hdr_control` is a point entity used to control the HDR effects.

### Keyvalues

`blurwidth` is the intensity of blooming for HDR, for example `"blurwidth" "0.1"`

`exposure` is the intensity of exposure for HDR, for example `"exposure" "4.5"`

`darkness` is the intensity of darkness for HDR, for example `"darkness" "4.5"`

`adaptation` is the brightness adaptation speed for HDR, for example `"adaptation" "50"`

`disablehdr` disables HDR entirely. for example `"disablehdr" "1"`

## env_water_control

`env_water_control` is a point entity used to control the water shader.

### Keyvalues

`basetexture` is the name of basetexture of water to control, `*` is for wildcard (only suffix wildcard supported). for example `"basetexture" "!radio"` or `"basetexture" "!toxi*"`

`disablewater` disables water shader entirely for water with specified `basetexture`. for example `"disablewater" "1"`

`normalmap` is the path to the normalmap for turbulence effect. for example `"normalmap" "renderer/texture/water_normalmap.tga"`

`fresnelfactor` controls the intensity of reflection. for example `"fresnelfactor" "0.4"`

`depthfactor` controls the strength of water edge feathering. for example `"depthfactor" "0.02 0.01"`

`normfactor` controls the intensity of turbulence based on normalmap. for example `"normfactor" "1.0"`

`minheight` water entity with height smaller than this value will not be rendered with shader program. for example `"minheight" "7.5"`

`maxtrans` controls the maximum transparency of water, must be between 0 and 255. for example `"maxtrans" "128"`

`level` . controls the reflection level. 0 = only skybox is reflected. 1 = skybox and world are reflected. 2 = both skybox, world and entities are reflected. 3 = use SSR to reflect. for example `"level" "1"`

## light_dynamic

`light_dynamic` is a point entity as an invisible point light source. 

Dynamic lights are calculated on the fly in the game, which means they have a higher processing cost but are much more flexible than static lighting. dynamic lights works only if `r_light_dynamic` is set to 1.

### Keyvalues

`origin` is the position of this entity's center in the world. for example `"origin" "123 456 789"`

`_light` is the RGB render color of the dynamic light, must be between 0 and 255. for example `"_light" "192 192 192"`

`_distance` is the distance that light is allowed to cast, in inches. for example `"_distance" "300"`

`_ambient` is the ambient intensity of dynamic light. for example `"_ambient" "0.0"`

`_diffuse` is the diffuse intensity of dynamic light. for example `"_diffuse" "0.1"`

`_specular` is the specular intensity of dynamic light. for example `"_specular" "0.1"`

`_specularpow` is the specular power of dynamic light. for example `"_specularpow" "10.0"`

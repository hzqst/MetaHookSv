# MetaRenderer documentation

[中文DOC](RendererCN.md)

# Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

* Warning: This plugin is not compatible with ReShade. 

# GPU Requirement

|       Vendor      |           Intel                  |          Nvidia            |          AMD               |
|       ----        |           ----                   |           ----             |           ----             |
|      Minimum      | Intel Haswell series with HD4600 |      Geforce GTX 650       |  AMD Radeon HD 7000 Series |
|     Recommend     |           ----                   |      Geforce GTX 1060 or better     |   AMD Radeon RX 560 or better        |

* It's recommended to run this plugin with at least 4GB of dedicated video memory. otherwise you might have bottleneck with VRAM bandwidth.

# Features

## HDR

HDR (High Dynamic Range) rendering simulates brightness above that which a computer monitor is actually capable of displaying. This mainly involves "blooming" colours above 100% brightness into neighbouring areas, and adjusting a virtual camera aperture to compensate for any over-exposure that results.

### Console vars

`r_hdr` Set to 1 to enable HDR post-processor rendering.

`r_hdr_blurwidth` Controls the intensity of blooming for HDR. recommend value : `0.0 ~ 0.1`

`r_hdr_exposure` Controls the intensity of exposure for HDR. recommend value : `0.0 ~ 1.2`

`r_hdr_darkness` Controls the intensity of darkness for HDR. recommend value : `0.0 ~ 1.6`

`r_hdr_adaptation` Controls the dark / bright adaptation speed for HDR. recommend value : `0 ~ 50`

## Water Shader

Water Shader creates water that realistically reflects and refracts the world.

All water surfaces fall into two types: reflective and legacy.

Reflective water reflects and refracts the whole world in real-time using Planar Reflections, which basically renders the entire scene twice, while legacy water rendered only with base texture just like what it does in vanilla GoldSrc.

Reflection level and shader parameters can be configured by using [env_water_control](Renderer.md#env_water_control).

### Console vars

`r_water` set to 1 to enable reflective water.

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

`r_flashlight_ambient` is ambient intensity of flashlights.

`r_flashlight_diffuse` is diffuse intensity of flashlights.

`r_flashlight_specular` is specular intensity of flashlights.

`r_flashlight_specularpow` is specular power of flashlights.

`r_dynlight_ambient` is ambient intensity of dynamic lights.

`r_dynlight_diffuse` is diffuse intensity of dynamic lights.

`r_dynlight_specular` is specular intensity of dynamic lights.

`r_dynlight_specularpow` is specular power of dynamic lights.

## Screen Space Reflection

Screen Space Reflection or SSR reflects pixels on screen by traces reflection rays in screen space in real time.

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

## Detail textures

A detail texture is a high resolution external image (Supported format: BMP, TGA, DDS, JPG, PNG) that is placed over the top of a map texture. This gives the impression of a small details when you get up close to a texture instead of the usual blurred image you get when the texture fills the screen.

`r_detailtextures` set to 1 to enable detail textures, normal textures, parallax textures and specular textures.

Detail texture list is read from `/maps/[map name]_detail.txt`, with `_DETAIL` as suffix in basetexture name (basetexture with no suffix will be treated as detail texture).

Detail textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

### Normal textures

Normal textures are external images applied to specified brush surfaces and change the direction of surface normal.

Normal texture list is read from `/maps/[map name]_detail.txt`, with `_NORMAL` as suffix in basetexture name.

Normal textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

* Normal textures change nothing but the direction of surface normal, thus only work with surfaces that illuminated by dynamic lights or flashlights.

* Normal textures only work when `r_detailtextures` and `r_light_dynamic` both set to 1.

### Parallax textures

Parallax textures are external images applied to specified brush surfaces which will have more apparent depth.

Parallax texture list is read from `/maps/[map name]_detail.txt`, with `_PARALLAX` as suffix in basetexture name.

Parallax textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

* `r_wsurf_parallax_scale` controls the intensity (and direction if negative value is given) of parallax textures.

* Parallax textures only work when `r_detailtextures` set to 1.

### Specular textures

Specular textures are external images applied to specified brush surfaces which will increase the intensity of specularity of surfaces.

Specular texture list is read from `/maps/[map name]_detail.txt`, with `_SPECULAR` as suffix in basetexture name.

Specular textures are loaded from `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` and `/Sven Co-op/svencoop/renderer/texture`.

* Red channel of specular texture controls the intensity of specular lighting, while green channel of specular texture controls intensity of Screen-Space-Reflection or SSR.

* Blue channel is not used yet.

* Specular textures only work when `r_detailtextures` set to 1.

## Outline / Celshade / RimLight / HairShadow / HairSpecular

The following flags are added to render Outline, Celshade, RimLight, HairShadow and HairSpecular effects for studiomodels.

You will have to create a txt file named `[modelname]_external.txt` along with `[modelname].mdl` file, with the following content:

```
{
    "classname" "studio_texture"
    "basetexture" "*"
    "flags" "STUDIO_NF_CELSHADE"
}
{
    "classname" "studio_texture"
    "basetexture" "face.bmp"
    "flags" "STUDIO_NF_CELSHADE_FACE"
}
{
    "classname" "studio_texture"
    "basetexture" "hair.bmp"
    "flags" "STUDIO_NF_CELSHADE_HAIR"
}
{
    "classname" "studio_efx"
    "flags" "EF_OUTLINE"
}
```

to enable those effects for `[modelname].mdl`.

Remember that `face.bmp` and `hair.bmp` should be the actual name of face and hair textures in `[modelname].mdl`.

Or use `Build\svencoop_addon\models\player\GFL_HK416\GFL_HK416_external.txt` as references.

Cvars for celshade will be overrided if sepcified key-values are filled in `[modelname]_external.txt`:

```
{
    "classname" "studio_celshade_control"
    "celshade_midpoint" "-0.1"
    "celshade_softness" "0.05"
    "celshade_shadow_color" "160 150 150"
    "outline_size" "3.0"
    "outline_dark" "0.5"
    "rimlight_power" "5.0"
    "rimlight_smooth" "0.1"
    "rimlight_smooth2" "0.0 0.3"
    "rimlight_color" "40 40 40"
    "rimdark_power" "5.0"
    "rimdark_smooth" "0.1"
    "rimdark_smooth2" "0.0 0.3"
    "rimdark_color" "50 50 50"
    "hair_specular_exp" "256"
    "hair_specular_exp2" "8"
    "hair_specular_intensity" "0.3 0.3 0.3"
    "hair_specular_intensity2" "0.12 0.12 0.12"
    "hair_specular_noise" "500 600 0.004 0.005"
    "hair_specular_noise2" "100 120 0.006 0.0.005"
    "hair_specular_smooth" "0.0 0.3"
    "hair_shadow_offset" "0.3 -0.3"
}
``

### Console vars

`r_studio_celshade` set to 1 to enable Outline / Celshade / RimLight / HairShadow / HairSpecular effects, set to 0 to disable them all.

`r_studio_celshade_midpoint` and `r_studio_celshade_softness` control the softness of celshade shadow.

`r_studio_celshade_shadow_color` control the color of celshade shadow.

`r_studio_outline_size` controls the size of outline.

`r_studio_outline_dark` controls the darkness of outline color.

`r_studio_rimlight_power` controls the intensity of Rim-Light at illuminated-side.

`r_studio_rimlight_smooth` controls the softness of Rim-Light at illuminated-side.

`r_studio_rimlight_smooth2` controls how does the Rim-Light at illuminated-side performance in dark area.

`r_studio_rimlight_color` controls the color of Rim-Light at illuminated-side.

`r_studio_rimdark_power` controls the intensity of Rim-Light at dark-side.

`r_studio_rimdark_smooth` controls the softness of Rim-Light at dark-side.

`r_studio_rimdark_smooth2` controls how does the Rim-Light at dark-side performance in dark area.

`r_studio_rimdark_color` controls the color of Rim-Light at dark-side.

`r_studio_hair_specular_exp` controls the size of area illuminated by primary hair specular, larger the hair_specular_exp is, smaller the illuminated area will be.

`r_studio_hair_specular_noise` controls the noise of primary hair specular.

`r_studio_hair_specular_intensity` controls the intensity of primary hair specular.

`r_studio_hair_specular_exp2` controls the size of area illuminated by secondary hair specular, larger the hair_specular_exp2 is, smaller the illuminated area will be.

`r_studio_hair_specular_noise2` controls the noise of secondary hair specular.

`r_studio_hair_specular_intensity2` controls the intensity of secondary hair specular.

`r_studio_hair_specular_smooth` controls how does the hair specular performance in dark area.

`r_studio_hair_shadow_offset` controls how the offset of hair shadow (horizontal and vertical offset in screen space).

## Vertex Buffer Object (aka VBO) "Batch-Draw" optimization

Brush surfaces, studio models, decals and sprites are rendered with Vertex Buffer Object, offering substantial performance gains over OpenGL 1.x immediate mode rendering primarily because the data reside in video memory rather than system memory and so it can be rendered directly by the video device.

## Order-Independent Transparency Blend (aka OIT blend)

Transparent pixels are stored in linked-list and sorted by GPU.

Add `-oitblend` to the launch parameters to enable Order-Independent Transparency Blend (it's disabled by default).

* Warning : It may dramatically hurt performance.

## Anti-Aliasing

`r_fxaa` set to 1 to enable Fast Approximate Anti-Aliasing (FXAA).

* Since MultiSample Anti-Aliasing (or MSAA) is not able to works with Deferred Shading, it's completely disabled and removed from current version of renderer plugin.

## Gamma Correction

Unlike vanilla GoldSrc, the gamma correction is applied on the fly (no restart required).

All textures are converted from texgamma color space to linear color space, and dynamic lights are calculated in linear color space to get correct lighting result.

### Console vars

`gamma` is used to control the final output gamma, convert colors from linear space to screen gamma space.

`texgamma` is used to convert textures from gamma color space to linear color space.

`lightgamma` is used to convert lightmaps from gamma color space to linear color space.

`brightness` is used to shift up the lightgamma and make lightmaps brighter.

## Misc

`r_wsurf_sky_occlusion` 1 / 0 : When set to 1, scenes occluded by "sky" surfaces (surfaces with sky texture) will be invisible.

`r_wsurf_zprepass` 1 / 0 : When set to 1, Z-Prepass will be enabled. The world will be rendered twice every frame. The first time with only depth write-in, the second time with actual fragment color write-in, which decreases the fragment shader cost when there is significant overdraw cost (like when shadow and SSR are calculated for unnecessary fragments ) for world rendering.

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

`exposure` is the intensity of exposure for HDR, for example `"exposure" "0.8"`

`darkness` is the intensity of darkness for HDR, for example `"darkness" "1.4"`

`adaptation` is the brightness adaptation speed for HDR, for example `"adaptation" "50"`

## env_water_control

`env_water_control` is a point entity used to control the water shader.

### Keyvalues

`basetexture` is the name of basetexture of water to control, `*` is for wildcard (only suffix wildcard supported). for example `"basetexture" "!radio"` or `"basetexture" "!toxi*"`

`normalmap` is the path to the normalmap for turbulence effect. for example `"normalmap" "renderer/texture/water_normalmap.tga"`

`fresnelfactor` controls the intensity of reflection. for example `"fresnelfactor" "0.4"`

`depthfactor` controls the strength of water edge feathering. for example `"depthfactor" "0.02 0.01"`

`normfactor` controls the intensity of turbulence based on normalmap. for example `"normfactor" "1.0"`

`minheight` water entity with height smaller than this value will not be rendered with shader program. for example `"minheight" "7.5"`

`maxtrans` controls the maximum transparency of water, must be between 0 and 255. for example `"maxtrans" "128"`

`speedrate` controls the speedrate of water turb in WATER_LEVEL_LEGACY mode, or speedrate of ripple effects in WATER_LEVEL_LEGACY_RIPPLE mode. for example `"speedrate" "1.0"`. value between 500 and 1000 is reasonable for WATER_LEVEL_LEGACY_RIPPLE mode water.

`level` controls the rendering level. 

|  Possible value                      | result                                                                      |
|        ----                          | ----                                                                        |
| "level" "0"                          | render as legacy water                                                      |
| "level" "WATER_LEVEL_LEGACY"         | render as legacy water                                                      |
| "level" "1"                          | only skybox is reflected                                                    |
| "level" "WATER_LEVEL_REFLECT_SKYBOX" | only skybox is reflected                                                    |
| "level" "2"                          | skybox and world are reflected                                              |
| "level" "WATER_LEVEL_REFLECT_WORLD"  | skybox and world are reflected                                              |
| "level" "3"                          | skybox, world, entities and particles are reflected                         |
| "level" "WATER_LEVEL_REFLECT_ENTITY" | skybox, world, entities and particles are reflected                         |
| "level" "4"                          | use SSR to reflect, only pixeles in screen-space are reflected              |
| "level" "WATER_LEVEL_REFLECT_SSR"    | use SSR to reflect, only pixeles in screen-space are reflected              |
| "level" "5"                          | use SSR to reflect, only pixeles in screen-space are reflected              |
| "level" "WATER_LEVEL_LEGACY_RIPPLE"  | render as legacy water, software-mode style, with pixel-art ripple effects  |

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

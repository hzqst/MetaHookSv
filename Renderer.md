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

* Changes to some of the cvars will not take effect immediately, unless using `r_reload` to force those cvars being reloaded.

## Water Shader

Water Shader creates water that realistically reflects and refracts the world.

All water surfaces fall into two types: expensive and cheap.

Expensive water reflects and refracts the whole world in real-time using Planar Reflections, which basically cause your entire scene to be rendered twice. while cheap water uses a simple cubemap to reflect.

### Console vars

`r_water` set to 1 to enable reflection and refraction for water. set to 2 to draw all visible entities in reflection (relatively expensive to render), otherwise only BSP world terrains are rendered in reflection.

## Per-Object Dynamic Shadow

Dynamic Shadows are cast only by world models (players, monsters, weaponbox, corpses, etc), and only onto brush surfaces. They are calculated at runtime, so they are quite crude and relatively expensive to render.

Dynamic Shadows can sometimes project through walls and floors, giving away the location of players or objects.

### Console vars

`r_shadow` set to 1 to enable Per-Object Dynamic Shadow.

`r_shadow_angles` control the direction of shadows, in PitchYawRoll format. for example `r_shadow_angles "90 0 0"`

`r_shadow_color` control the color of shadows, in RGBA8 format. for example `r_shadow_color "0 0 0 128"`

`r_shadow_minlum` is the minimum luminance the shadow is allowed to cast, in from 0 to 255. for example `r_shadow_minlum 64`

`r_shadow_high_distance` is the maximum distance that entities are being rendered in high-quality shadow map, in inches. for example `r_shadow_high_distance 400`

`r_shadow_high_scale` is scale factor to scale the size of entity model up or down in high-quality shadow map. for example `r_shadow_high_scale 4`

`r_shadow_medium_distance` is the maximum distance that entities are being rendered in medium-quality shadow map, in inches. for example `r_shadow_medium_distance 800`

`r_shadow_medium_scale` is scale factor to scale the size of entity model up or down in medium-quality shadow map. for example `r_shadow_medium_scale 2`

`r_shadow_low_distance` is the maximum distance that entities are being rendered in low-quality shadow map, in inches. for example `r_shadow_low_distance 1200`

`r_shadow_low_scale` is scale factor to scale the size of entity model up or down in low-quality shadow map. for example `r_shadow_low_scale 0.5`

* * Changes to some of the cvars will not take effect immediately, unless using `r_reload` to force those cvars being reloaded.

## Screen Space Ambient Occlusion

`r_ssao` Set to 1 to enable SSAO

(Warning: you might see SSAO's black pixel fighting on wall or terrain if your defalt_fov is not 90. you have to either turn SSAO off or change default_fov back to 90)

`r_ssao_intensity` control the intensity of SSAO shadow. recommended value : 0.6 ~ 1.0

`r_ssao_radius` control the sample size of SSAO shadow. recommended value : 30.0 ~ 100.0

`r_ssao_blur_sharpness` control the sharpness of SSAO shadow. recommended value : 1.0

`r_ssao_bias` unknown. recommended value : 0.1 ~ 0.2

## Deferred Shading

`r_light_dynamic` set to 1 to enable Deferred Shading.

`r_flashlight_cone` is the cosine of angle of flashlight cone.

`r_flashlight_distance` is the illumination distance of flashlight.

`r_light_ambient` is ambient intensity of dynamic light. recommended value : 0.2

`r_light_diffuse` is diffuse intensity of dynamic light. recommended value : 0.3

`r_light_specular` is specular intensity of dynamic light. recommended value : 0.1

`r_light_specularpow` is specular power of dynamic light. recommended value : 10.0

## Vertex Buffer Object (VBO) "Batch-Draw" optimization

`r_studio_vbo` set to 1 to enable VBO batch-draw optmization for studio model.

`r_wsurf_vbo` set to 1 to enable VBO batch-draw optmization for BSP terrain and brush entities.

## Detail textures

`r_wsurf_detail` set to 1 to enable detail textures, normal textures, parallax textures.

`r_wsurf_parallax_scale` is the intensity of parallax textures.

## Misc

`r_wsurf_sky_occlusion` 1 / 0 : When set to 1, scenes occluded by "sky" surfaces (surfaces with sky texture) will be invisible. this only takes effect when r_wsurf_vbo set to 1.

`r_fxaa` set to 1 to enable Fast Approximate Anti-Aliasing (FXAA).

`r_msaa` MultiSampling Anti-Aliasing (MSAA), number >= 2 for MSAA sample count. recommended value : 0 if SSAO enabled or 4 if SSAO disabled.

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

`fade` is the distance where the shadow begins to fade-out and the maximum distance the shadow is allowed to cast, in inches. for example `"fade" "64 128"`

`minlum` is the minimum luminance the shadow is allowed to cast, in from 0 to 255. for example `"minlum" "64"`

`color` is the color of the shadows, in RGBA8 format. for example `"color" "0 0 0 128"`

`disableallshadows` disables shadows entirely. for example `"disableallshadows" "1"`

`high_distance` is the maximum distance that entities are being rendered in high-quality shadow map, in inches. for example `"high_distance" "400"`

`high_scale` is scale factor to scale the size of entity model up or down in high-quality shadow map. for example `"high_scale" "4"`

`medium_distance` is the maximum distance that entities are being rendered in medium-quality shadow map, in inches. for example `"medium_distance" "800"`

`medium_scale` is scale factor to scale the size of entity model up or down in medium-quality shadow map. for example `"medium_scale" "2"`

`low_distance` is the maximum distance that entities are being rendered in low-quality shadow map, in inches. for example `"low_distance" "1200"`

`low_scale` is scale factor to scale the size of entity model up or down in low-quality shadow map. for example `"low_scale" "0.5"`

## env_hdr_control

`env_hdr_control` is a point entity used to control the HDR effects for local player.

### Keyvalues

`blurwidth` is the intensity of blooming for HDR, for example `"blurwidth" "0.1"`

`exposure` is the intensity of exposure for HDR, forexample `"exposure" "4.5"`

`darkness` is the intensity of darkness for HDR, forexample `"darkness" "4.5"`

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

`maxtrans` controls the maximum transparency of water, in from 0 ~ 255. for example `"maxtrans" "128"`

## light_dynamic

`light_dynamic` is a point entity as an invisible point light source, dynamic lights are calculated on the fly in the game, which means they have a higher processing cost but are much more flexible than static lighting. dynamic lights works only if `r_light_dynamic` is set to 1.

### Keyvalues

`origin` is the position of this entity's center in the world. for example `"origin" "123 456 789"`

`_light` is the RGB render color of the dynamic light. Colors must be between 0 and 255. for example `"_light" "192 192 192"`

`_distance` is the distance that light is allowed to cast, in inches. for example `"_distance" "300"`

`_ambient` is the ambient intensity of dynamic light. for example `"_ambient" "0.0"`

`_diffuse` is the diffuse intensity of dynamic light. for example `"_diffuse" "0.1"`

`_specular` is the specular intensity of dynamic light. for example `"_specular" "0.1"`

`_specularpow` is the specular power of dynamic light. for example `"_specularpow" "10.0"`

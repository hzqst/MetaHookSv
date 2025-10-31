# MetaRenderer documentation

[中文文档](/docs/RendererCN.md)

# Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

* Warning: This plugin is not compatible with ReShade. 

# GPU Requirement

|       Vendor      |           Intel                  |          Nvidia            |          AMD               |
|       ----        |           ----                   |           ----             |           ----             |
|      Minimum      | Intel Haswell series with HD4600 |      Geforce GTX 400 series       |  AMD Radeon HD 5000 series |
|     Recommend     |           ----                   |      Geforce GTX 1060 or better     |   AMD Radeon RX 560 or better        |

* OpenGL 4.4 Core Profile is required to run this plugin.

* It's recommended to run this plugin with at least 4GB of dedicated video memory. otherwise you might have bottleneck with VRAM bandwidth or get game crash due to out-of-VRAM.

* For higher video resolution (like 4K), you will have to get a GPU with higher VRAM bandwidth and VRAM capacity.

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

`r_water` set to 1 to enable water shader.

## Screen Space Ambient Occlusion

SSAO or Screen-Space Ambient Occlusion is the type of post-processing effect that approximating the ambient occlusion effect in real time.

The implementation is [HBAO or Horizon-Based-Ambient-Occlusion](https://github.com/nvpro-samples/gl_ssao).

* Note that Ambient Occlusion only works when deferred shading pipeline available and enabled.

### Console vars

`r_ssao` Set to 1 to enable SSAO

`r_ssao_intensity` control the intensity of SSAO shadow.

`r_ssao_radius` control the sample size of SSAO shadow.

`r_ssao_blur_sharpness` control the sharpness of SSAO shadow.

`r_ssao_bias` bias to avoid incorrect occlusion effect on curved surfaces.

## Deferred Shading and Dynamic Lights

[Deferred-Shading](https://en.wikipedia.org/wiki/Deferred_shading) pipeline is used to render opaque objects, and lights are calculated in real time using [Blinn-Phong](https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model) model.

### Console vars

`r_deferred_lighting` set to 1 to enable deferred shading pipeline.

`r_flashlight_enable` is to enable or disable spotlight-based flashlights. (only available when `r_light_dynamic` set to 1)

`r_flashlight_cone_consine` is the cosine of angle of flashlight cone. value between 0.8 and 0.99 is recommended.

`r_flashlight_distance` is the max illumination distance of flashlight. default : 2000

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

* Screen Space Reflection only works when deferred shading pipeline available and enabled.

### Console vars

`r_ssr` set to 1 to enable Screen Space Reflection

`r_ssr_ray_step` controls the step length to iterate the ray-marching. for example `r_ssr_ray_step 5.0`

`r_ssr_iter_count` controls the maximum iteration count for ray-marching. for example `r_ssr_iter_count 64`

`r_ssr_distance_bias` ray-marching hits object if distance smaller than this value. for example `r_ssr_distance_bias 0.2`

`r_ssr_adaptive_step` set to 1 to enable Adaptiveg-Step to accelerate the ray-marching procedure. for example `r_ssr_adaptive_step 1`

`r_ssr_exponential_step` set to 1 to enable Exponential-Step to accelerate the ray-marching procedure. for example `r_ssr_exponential_step 1`

`r_ssr_binary_search` set to 1 to enable Binary-Search to accelerate the ray-marching procedure. for example `r_ssr_binary_search 1`

`r_ssr_fade` controls the fade-out effect if the reflected ray hit a pixel close to the screen border. for example `r_ssr_fade "0.8 1.0"`

## Detail sky textures

`r_detailskytextures` set to 1 to enable detail sky textures, which loads `gfx/env/[skyname][direction].dds` or `renderer/texture/[skyname][direction].dds` as replacement.

For example :

`gfx/env/desertbk.bmp` -> `gfx/env/desertbk.dds`

`gfx/env/desertdn.bmp` -> `gfx/env/desertdn.dds`

`gfx/env/desertft.bmp` -> `gfx/env/desertft.dds`

`gfx/env/desertlf.bmp` -> `gfx/env/desertlf.dds`

`gfx/env/desertrt.bmp` -> `gfx/env/desertrt.dds`

`gfx/env/desertup.bmp` -> `gfx/env/desertup.dds`

## BSP detail textures

A detail texture is a high resolution external image (Supported format: BMP, TGA, DDS, JPG, PNG) that is placed over the top of a map texture. This gives the impression of a small details when you get up close to a texture instead of the usual blurred image you get when the texture fills the screen.

Detail texture list is parsed from `/maps/[mapname]_detail.txt`, with `_DETAIL` as suffix in basetexture name. names with no suffix will be treated as detailtexture.

Detail textures will be loaded from the following path (if exists):

1. `{GameDirectory}_addon/maps/{TextureName}` (only if texture name starts with "maps/" or "maps\", and only if your FileSystem supports "{GameDirectory}_adddon")

2. `{GameDirectory}_downloads/maps/{TextureName}` (only if texture name starts with "maps/" or "maps\", and only if your FileSystem supports "{GameDirectory}_downloads" )

3. `{GameDirectory}/maps/{TextureName}` (only if texture name starts with "maps/" or "maps\")

4. `{GameDirectory}/gfx/detail/{TextureName}`

5. `{GameDirectory}/renderer/texture/{TextureName}`

* `.tga` will be added to the filename by default, if no file extension is given.

### BSP texture replacement

You can replace WAD textures (that used by brush surfaces or decals) with external images.

For brush surfaces: replace list is parsed from `{GameDirectory}/maps/{MapName}_detail.txt`, with `_REPLACE` as suffix in basetexture name.

For decals: replace list is parsed from `{GameDirectory}/renderer/decal_textures.txt`, with `_REPLACE` as suffix in basetexture name.

The rules of texture searching follow the same way as "BSP detail textures"

### BSP normal textures

Normal textures are external images applied to specified brush surfaces and change the direction of surface normal.

For brush surfaces: texture list is parsed from `{GameDirectory}/maps/{MapName}_detail.txt`, with `_NORMAL` as suffix in basetexture name.

For decals: texture list is parsed from `{GameDirectory}/renderer/decal_textures.txt`, with `_NORMAL` as suffix in basetexture name.

The rules of texture searching follow the same way as "BSP detail textures"

* Normal textures change nothing but the direction of surface normal, thus only work with surfaces that illuminated by dynamic lights or flashlights.

### BSP parallax textures

Parallax textures are external images applied to specified brush surfaces which will have more apparent depth.

For brush surfaces: texture list is parsed from `{GameDirectory}/maps/{MapName}_detail.txt`, with `_PARALLAX` as suffix in basetexture name.

For decals: texture list is parsed from `{GameDirectory}/renderer/decal_textures.txt`, with `_PARALLAX` as suffix in basetexture name.

The rules of texture searching follow the same way as "BSP detail textures"

* `r_wsurf_parallax_scale` controls the intensity (and direction if negative value is given) of parallax textures.

### BSP specular textures

Specular textures are external images applied to specified brush surfaces which will increase the intensity of specularity of surfaces.

For brush surfaces: texture list is parsed from `{GameDirectory}/maps/{MapName}_detail.txt`, with `_SPECULAR` as suffix in basetexture name.

For decals: texture list is parsed from `{GameDirectory}/renderer/decal_textures.txt`, with `_SPECULAR` as suffix in basetexture name.

The rules of texture searching follow the same way as "BSP detail textures"

* Red channel of specular texture controls the intensity of specular lighting, while green channel of specular texture controls intensity of Screen-Space-Reflection or SSR.

* Blue channel is not used yet.

## Sprite external file

You may create a txt file named `[spritename]_external.txt` along with `[spritename].spr` file `[spritename]_external.txt` to override sprite properties without need to modify the content of `[spritename].spr` file itself.

The content of `[spritename]_external.txt` looks like this:

```
{
    "classname" "value"
    "key2" "value2"
    "key3" "value3"
}
```

## Sprite texture replacement

The following snippet replaces frame[0] of current sprite with `sprites/test.png`

```
{
  "classname" "sprite_frame_texture"
  "frame" "0"
  "replacetexture" "sprites/test.png"
}
```

## Sprite advanced effects

No bloom effect will be applied to current sprite:
```
{
  "classname" "sprite_efx"
  "flags" "FMODEL_NOBLOOM"
}
```

## StudioModel external file

You may create a txt file named `[modelname]_external.txt` along with `[modelname].mdl` file `[modelname]_external.txt` to override studiomodel properties without need to modify the content of `[modelname].mdl` file itself.

The content of `[modelname]_external.txt` looks like this:

```
{
    "classname" "value"
    "key2" "value2"
    "key3" "value3"
}
```

* Most advanced features about studio rendering are based on `_external.txt` file.

## StudioModel texture replacement

You will have to create a txt file named `[modelname]_external.txt` along with `[modelname].mdl` file, with the following content:

### If the replacetexture's name starts with "models/" or "models\"

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "replacetexture"  "models/mymodel/base_texture.dds"
    "replacescale" "1.0 1.0"
}
```

The following file will be used to replace basetexture if exists:

`{GameDirectory}/models/mymodel/base_texture.dds`

### Otherwise

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "replacetexture"  "base_texture.dds" 
    "replacescale" "1.0 1.0"
}
```

The following files will be used to replace basetexture if exists:

`{GameDirectory}/gfx/base_texture.dds`

`{GameDirectory}/renderer/texture/base_texture.dds`

* `.tga` will be added to the filename if no file extension is given.

### UV Controls

`"replacescale" "1.0 1.0"` : the width & height used to calculate the texcoord will be replaced with (1.0 x new texture's width, 1.0 x new texture's height).

`"replacescale" "1.0"` : equals to `"replacescale" "1.0 1.0"`

`"replacescale" "-1.0 -1.0"` : the width & height used to calculate the texcoord will be replaced with (1.0 x original texture's width, 1.0 x original texture's height).

* The UV scaling won't be affected if there is no "replacescale"

* The UVs in studiomodel are stored in unsigned short ranges from -32767 to 32768. The exact value of stored UV equals the pixel coordinate in the original BMP texture. The actual texcoord transfered to GPU has to be calculated first -- dividing the pixel coordinate by the width & height of the texture. the "replacescale" affects the saying "the width & height of the texture".

### Cvars

* Use cvar `r_studio_external_textures 0` to disable StudioModel texture replactment temporarily.

## StudioModel normal texture

You will have to create a txt file named `[modelname]_external.txt` along with `[modelname].mdl` file, with the following content:

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "normaltexture"  "normal_texture.dds" //The rules of texture searching follow the same way as "replacetexture"
}
```

### Cvars

* Use cvar `r_studio_external_textures 0` to disable StudioModel normal texture temporarily.

## StudioModel specular texture

You will have to create a txt file named `[modelname]_external.txt` along with `[modelname].mdl` file, with the following content:

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "speculartexture"  "specular_texture.dds" //The rules of texture searching follow the same way as "replacetexture"
}
```

The channel `RED` will be used as the intensity of the specular.

The channel `GREEN` will be used as the intensity of the Screen-Space-Reflection.

The channel `BLUE` will be used as the ratio of the Spherized-Normal on "face" textures。(This only affects textures with `STUDIO_NF_CELSHADE_FACE`)

### Cvars

* Use cvar `r_studio_external_textures 0` to disable StudioModel specular texture temporarily.

* Use cvar `r_studio_base_specular 1.0 2.0` to control the intensity and focus of the phong-model specular lighting.

* For base specular stuffs, higher "intensity" results in higher brightness, while higher "focus" gets the specular lighting focused into a shiny point.

* `classname` `studio_texture` with key-value pair `base_specular` `1.0 2.0` overrides the cvar one.

## StudioModel alpha-transparent texture

Just like `STUDIO_NF_ADDITIVE` but with alpha-blending instead of additive-blending. The rendering of meshes with `STUDIO_NF_ALPHA` will be defered to transparent pass if it's from an opaque entity.

You will have to replace the basetexture with an external texture with alpha-channel supported, like DXT5-BC7 DDS,TGA or PNG.

Add following content to the `[modelname]_external.txt`:

```
{
    "classname" "studio_texture"
    "basetexture" "basetexture.bmp"
    "replacetexture" "replacetexture.dds" // Alpha-channel required!!!
    "flags" "STUDIO_NF_ALPHA"
}
```

## StudioModel double-side face rendering

Both front and back faces will be rendered, when those meshes are using textures with `STUDIO_NF_DOUBLE_FACE`.

Add following content to the `[modelname]_external.txt`:

```
{
    "classname" "studio_texture"
    "basetexture" "basetexture.bmp"
    "flags" "STUDIO_NF_DOUBLE_FACE"
}
```

## StudioModel packed-texture

Diffuse texture, normal texture, parallax texture and specular texture can be packed into one texture.

The texture name in the .mdl file must be started with `Packed_`.

For example: `Packed_D0_N1_P2_S3_whatever.bmp` means the texture will be loaded as a packed texture. the whole image will be divided into 4 parts horizontally. The 1st part (aka region #0) will be treated as diffuse texture, the 2nd part (aka region #1) will be treated as normal texture, the 3rd part (aka region #2) will be treated as parallax texture, and the last part  (aka region #3) will be treated as specular texture.

`D0` : Diffuse texture at region #0

`N1` : Normal texture at region #1

`P2` : Parallax texture at region #2

`S3` : Specular texture at region #3

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
    "flags" "FMODEL_OUTLINE"
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
    "celshade_head_offset" "3.5 2 0"
    "celshade_lightdir_adjust" "0.01 0.001"
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
```

### Eyebrow-Passthrough

1. Textures that contain eyebrow must be marked with `"flags" "STUDIO_NF_CELSHADE_FACE"`:

```
{
    "classname" "studio_texture"
    "basetexture" "T_BA_hsn_001_face_C.bmp"
    "flags" "STUDIO_NF_CELSHADE_FACE"
    "replacetexture" "models/player/BA_Hoshino_HD/T_BA_hsn_001_face_C.png"
    "speculartexture" "models/player/BA_Hoshino_HD/T_BA_hsn_001_face_C_specular.png"
}
```

2. Textures that contain hair must be marked with `"flags" "STUDIO_NF_CELSHADE_HAIR"`:


```
{
    "classname" "studio_texture"
    "basetexture" "T_BA_hsn_001_hair_C.bmp"
    "flags" "STUDIO_NF_CELSHADE_HAIR"
}
```

3. Eyebrow pixels must be marked with alpha < 255, which can be done by taking advantage of `replacetexture` with a PNG texture:

![](/img/10.png)

You will be able to see eyebrow through hair if both 1, 2 and 3 are matched:

![](/img/9.png)

* Note that lower the alpha value of eyebrow pixel is, lower the visibility of eyebrow through hair will be.

### Draw lower body

Use cvar `r_drawlowerbody 1` to enable lower body rendering.

You will have to create a txt file named `[modelname]_external.txt` along with `[modelname].mdl` file, with the following content:

```
{
    "classname" "studio_bone"
    "name" "Bip01"
    "flags" "STUDIO_BF_LOWERBODY"
}
{
    "classname" "studio_bone"
    "name" "Bip01 Pelvis"
    "flags" "STUDIO_BF_LOWERBODY"
}
{
    "classname" "studio_bone"
    "name" "Bip01 Spine"
    "flags" "STUDIO_BF_LOWERBODY"
}
{
    "classname" "studio_lowerbody_control"
    "model_origin" "0 0 0"
    "duck_model_origin" "0 0 0"
    "model_scale" "1"
}
```

to make the specified bone-based bodypart visible when rendering `[modelname].mdl` as lowerbody model.

* Any bodypart without being marked as `STUDIO_BF_LOWERBODY` will be culled.

* You can check the actual bone name with Half-Life Asset Manager or similar tools.

* `model_origin` is for adjusting the position of lowerbody model with a given offset.

* `duck_model_origin` is for adjusting the position of lowerbody model with a given offset (when player is crouching).

* `model_scale` is for adjusting the curstate.scale of lowerbod model (Sven Co-op only).

* Use `r_drawlowerbodyattachments 0` to hide attachment entites attached to local player with MOVETYPE_FOLLOW.

* Use `r_drawlowerbodypitch 45` to hide lowerbody model when your viewangles.pitch > 45 degree.

### Console vars

`r_studio_debug` set to 1 to display debug draw for the studio models.

`r_studio_celshade` set to 1 to enable Outline / Celshade / RimLight / HairShadow / HairSpecular effects, set to 0 to disable them all.

`r_studio_celshade_midpoint` and `r_studio_celshade_softness` control the softness of celshade shadow.

`r_studio_celshade_shadow_color` controls the color of celshade shadow.

`r_studio_celshade_head_offset` controls the head's origin offset of from it's parent bone.

`r_studio_celshade_lightdir_adjust` scales the Z-axis of the light vector that being used to calculate the celshade lighting. The 1st value works for body-celshade and the 2nd value works for face-celshade.

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

## Studio _external.txt samples

### StudioTexture override sample

```
{
    "classname" "studio_texture"
    "basetexture" "123.bmp"
    "replacetexture" "123.dds"
    "normaltexture" "123.png"
    "parallaxtexture" "123.tga"
    "speculartexture" "123.jpg"
    "flags" "STUDIO_NF_FLATSHADE"
    "base_specular" "1.0 2.0"
}
```

* Possible studio_texture flags:

```
// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004
#define STUDIO_NF_NOMIPS        0x0008
#define STUDIO_NF_ALPHA         0x0010
#define STUDIO_NF_ADDITIVE      0x0020
#define STUDIO_NF_MASKED        0x0040

// renderer-private options
#define STUDIO_NF_CELSHADE					0x1000
#define STUDIO_NF_CELSHADE_FACE				0x2000
#define STUDIO_NF_CELSHADE_HAIR				0x4000
#define STUDIO_NF_HASOUTLINE				0x8000  //For internal unsage only.
#define STUDIO_NF_DOUBLE_FACE				0x10000

#define STUDIO_NF_NOOUTLINE					0x40000
```

### StudioEffects override sample

```
{
    "classname" "studio_efx"
    "flags" "FMODEL_OUTLINE"
}
{
    "classname" "studio_efx"
    "flags" "FMODEL_NOBLOOM"
}
{
    "classname" "studio_efx"
    "flags" "FMODEL_NOSHADOW"
}
{
    "classname" "studio_efx"
    "flags" "-FMODEL_ROCKET"
}
```

* Note that flags with `-FMODEL_ROCKET` means remove `FMODEL_ROCKET` from flags.

* Possible studio_efx flags:

```
#define	FMODEL_ROCKET			1			// leave a trail
#define	FMODEL_GRENADE			2			// leave a trail
#define	FMODEL_GIB				4			// leave a trail
#define	FMODEL_ROTATE			8			// rotate (bonus items)
#define	FMODEL_TRACER			0x10		// green split trail
#define	FMODEL_ZOMGIB			0x20		// small blood trail
#define	FMODEL_TRACER2			0x40		// orange split trail + rotate
#define	FMODEL_TRACER3			0x80		// purple trail
#define FMODEL_DYNAMIC_LIGHT	0x100			
#define FMODEL_TRACE_HITBOX		0x200		//Use hitbox collision
#define FMODEL_FORCESKYLIGHT	0x400		// forces the model to lit by sky
#define FMODEL_OUTLINE			0x1000		//Renderer-specified flags
#define FMODEL_NOBLOOM			0x2000		//Renderer-specified flags
#define FMODEL_NOSHADOW			0x4000		//Renderer-specified flags

//Vanilla GoldSrc flags for studio models, deprecated
#define EF_ROCKET			1			//! leave a trail
#define EF_GRENADE			2			//! leave a trail
#define EF_GIB				4			//! leave a trail
#define EF_ROTATE			8			//! rotate (bonus items)
#define EF_TRACER			0x10			//! green split trail
#define EF_ZOMGIB			0x20			//! small blood trail
#define EF_TRACER2			0x40			//! orange split trail + rotate
#define EF_TRACER3			0x80			//! purple trail
#define EF_NOSHADELIGHT		0x100			//! No shade lighting
#define EF_HITBOXCOLLISIONS	0x200			//! Use hitbox collisions
#define EF_FORCESKYLIGHT	0x400		//! Forces the model to be lit by skybox lighting
#define EF_OUTLINE			0x1000		//Renderer-specified flags

```

### StudioCelshade control sample

```
{
    "classname" "studio_celshade_control"
    "celshade_midpoint" "-0.1"
    "celshade_softness" "0.05"
    "celshade_shadow_color" "160 150 150"
    "celshade_head_offset" "3.5 2 0"
    "celshade_lightdir_adjust" "0.01 0.001"
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
```

### StudioLowerbody control sample

```
{
    "classname" "studio_lowerbody_control"
    "model_origin" "0 0 0"
    "duck_model_origin" "0 0 0"
    "model_scale" "1"
}
```

### StudioBone modification sample

```
{
    "classname" "studio_bone"
    "name" "Bip01"
    "flags" "STUDIO_BF_LOWERBODY"
}
{
    "classname" "studio_bone"
    "index" "0"
    "flags" "STUDIO_BF_LOWERBODY"
}
{
    "classname" "studio_bone"
    "index" "1"
    "flags" "-STUDIO_BF_LOWERBODY"
}
```

* Note that flags with `-STUDIO_BF_LOWERBODY` means remove `STUDIO_BF_LOWERBODY` from flags.

### StudioSequence modification sample

```
{
    "classname" "studio_sequence"
    "name" "swim"
    "flags" "STUDIO_SF_HIDE_LOWERBODY"
}
{
    "classname" "studio_sequence"
    "name" "idle"
    "flags" "-STUDIO_SF_HIDE_LOWERBODY"
}
```

* Note that flags with `-STUDIO_SF_HIDE_LOWERBODY` means remove `STUDIO_SF_HIDE_LOWERBODY` from flags.

## Networkable ViewEntity

Networkable ViewEntity is an entity that attach to your local viewmodel. server will have full control over it.

All you need to do is :

1. Create a networkable entity with `pev.effects |= EF_VIEWMODEL`, `pev.aiment = pPlayer`, `pev.movetype = MOVETYPE_FOLLOW`.  Don't forget `#define EF_VIEWMODEL 0x4000 //or 16384 in decimal`

2. Set a valid model for it (can be done with `SET_MODEL` from HLSDK or `g_EntityFuncs.SetModel` from Sven Co-op's angelscript system).

3. Make sure the networkable entity is only visible to no one other than `pPlayer`. (can be achieved with `AddToFullPack_Post` hook)

4. Make sure the number  `DEFINE_DELTA( effects, DT_INTEGER, 16, 1.0 ),` in `game_directory/delta.lst` >= 14. 

* The default value in `cstrike/delta.lst` is `DEFINE_DELTA( effects, DT_INTEGER, 8, 1.0 ),` which means the maximum value it can transfer to client is 2^8=256. so 16384 is definitely too large in this case. You should change 8 to any number larger than 14 to make `EF_VIEWMODEL` work.

* The default value in `svencoop/delta.lst` is `DEFINE_DELTA( effects, DT_INTEGER, 16, 1.0 ),` which means the maximum value it can transfer to client is 2^16=63356. so 16384 is good.

<img width="999" height="321" alt="image" src="https://github.com/user-attachments/assets/f0380c5b-270b-43d1-854e-2996f4068d49" />

<img width="1040" height="969" alt="image" src="https://github.com/user-attachments/assets/7986cd52-d837-40d6-b81d-83aa1b500695" />

## Vertex Buffer Object (aka VBO) "Batch-Draw" optimization

Brush surfaces, studio models and decals are rendered with Vertex Buffer Object, offering substantial performance gains over OpenGL 1.x immediate mode rendering primarily because the geometry data reside in video memory rather than system memory and so it can be rendered directly by the video device.

## Order-Independent Transparency Blend (aka OIT blend)

Transparent pixels are stored in linked-list and sorted by GPU.

Add `-oitblend` to the launch parameters to enable Order-Independent Transparency Blend (it's disabled by default).

* Warning : It may dramatically hurt performance.

## New texture loader

### Vanilla textures

The maximum size of WAD/SPR texture is extended to 4096 x 4096 if using new texture loader.

This is enabled by default and may produce inconsistencies of texture's visual presentation between the new and legacy texture loader.

You can add `-use_legacy_texloader` to the launch parameter to disable the new texture loader.

### External textures

The maximum size of external texture depends on your GPU and OpenGL driver implementation (typically 16384x16384).

Supported format :

BMP (Indexed / RGB8 / RGBA8)

TGA (RGB8 / RGBA8)

DDS (DX10 BC1 / DX10 BC2 / DX10 BC3 / DX10 BC7 / Legacy DXT1 / Legacy DXT3 / Legacy DXT5）

HDR (RGB8F / RGBA8F)

JPEG (RGB8)

PNG (RGB8 / RGBA8)

WEBP (RGB8 / RGBA8)

## Anti-Aliasing

`r_fxaa` set to 1 to enable Fast Approximate Anti-Aliasing (FXAA).

* Since MultiSample Anti-Aliasing (or MSAA) is not able to works with Deferred Shading, it's completely disabled and removed from current version of renderer plugin.

## Gamma Correction

Unlike vanilla GoldSrc, the gamma correction is applied on the fly (no restart required).

All textures are converted from texgamma color space to linear color space, and dynamic lights are calculated in linear color space to get correct lighting result.

### Console vars

`gamma` is to control the final output gamma, convert colors from linear space to screen gamma space.

`texgamma` is to convert textures from gamma color space to linear color space.

`lightgamma` is to convert lightmaps from gamma color space to linear color space.

`brightness` is to shift up the lightgamma and make lightmaps brighter.

`r_gamma_blend 0` blend transparent objects in linear space. This is may lead to brighter or darker blending result than vanilla GoldSrc.

`r_gamma_blend 1` blend transparent objects in gamma space instead of linear space. This is the default behavior from vanilla GoldSrc. Note that deferred shading pipeline will not be available when `r_gamma_blend` enabled. and some buggy graphic drivers may not work well with `r_gamma_blend 1`.

* Any mathematical operation (such as transparent blending) on gamma corrected color is physically incorrect! See: https://en.wikipedia.org/wiki/Gamma_correction

`r_linear_blend_shift 0`: Don't shift color/alpha for transparent object at all in linear space.

`r_linear_blend_shift 1`: Shift color/alpha for transparent object to how it looks like in vanilla engine. (Only works when r_gamma_blend off)

`r_linear_blend_shift 0.5`: Mix with 50% of result from `r_linear_blend_shift 1` and 50% of result from `r_linear_blend_shift 0`.

`r_linear_fog_shift 0` : Don't shift the fog density to lower value.

`r_linear_fog_shift 1` : Shift the fog density to lower value. (Only works when r_gamma_blend off)

`r_linear_fog_shift 0.5`: Mix with 50% of result from `r_linear_fog_shift 1` and 50% of result from `r_linear_fog_shift 0`.

`r_linear_fog_shiftz` : Shift the fog density to lower value with formula `distanceToPixel = distanceToPixel * r_linear_fog_shiftz` to mimic that the pixel is closer to camera.

## FOV (Field of View)

### Dedicated viewmodel FOV

Viewmodel's FOV can be individually adjusted by using cvar `viewmodel_fov [FOV value]`. Use `viewmodel_fov 0` to disable custom viewmodel FOV.

### Vertical FOV

This controls what we treat the input screen FOV as:

* The input screen FOV is typically defined by `default_fov`, and is locked to 90 in certain games (cstrike and czero).

`r_vertical_fov 1` Enable Vertical FOV. The input screen FOV will be treated as Vertical FOV. This is the default FOV policy used by Sven Co-op.

`r_vertical_fov 0` Disable Vertical FOV. The input screen FOV will be treated as Horizontal FOV just like what it was in vanilla GoldSrc.  This is the default FOV policy used by vanilla GoldSrc.

### WideScreen-Adapted FOV

This controls how to convert horizontal FOV to vertical FOV :

`gl_widescreen_yfov 1` is the default policy used by Xash3D-fwgs, Nexon's Counter-Strike : Online and Half-Life 25th anniversary update that expands the horizontal FOV while keep the vertical FOV as what it was when using resolution of 4:3

`gl_widescreen_yfov 2` is to stretch the original 4:3 FOV image to fit the current rendering resolution.

* Note: `gl_widescreen_yfov 1` will be overrided by `r_vertical_fov 1` .

## Sprite Interpolation

`r_sprite_lerping` 1 / 0: This provides smoother animation for sprites that have their rendermode set to texture and additive. This works as what it is in Xash3d-fwgs. 

* Credits to Xash3d-fwgs

* Note: For correct interpolation, make sure the server framerate is exactly 10 FPS (regardless of the sprite’s own FPS in pev | framerate). This parameter is dictated by the frame changer function’s think time, which is 0.1s and remains unchanged in most mods.

## Quake-Style Under Water Screen Effect

`r_under_water_effect` 1 / 0

`r_under_water_effect_wave_amount` default: 10.0

`r_under_water_effect_wave_speed` default: 1.0

`r_under_water_effect_wave_size` default: 0.01

## Misc

`r_wsurf_sky_fog 0` : Fog don't affect skybox (Vanilla behavior)

`r_wsurf_sky_fog 1` : Fog affects skybox

`r_studio_legacy_dlight 0`: Completely disable legacy dlights

`r_studio_legacy_dlight 1`: Legacy dlights are added up from CPU-side (Vanilla behavior)

`r_studio_legacy_dlight 2`: Use shader to calculate and add up all legacy dlights in GPU (with more precision, also with more GPU consumption)

`r_studio_legacy_elight 0`: Completely disable entity lights

`r_studio_legacy_elight 1`: Enable entity lights (Vanilla behavior)

`r_fog_trans 0`: Fog don't affect any transparent objects

`r_fog_trans 1`: Fog affects alpha blending objects, but not additive blending objects (Vanilla behavior)

`r_fog_trans 2`: Fog affects both alpha blending objects and additive blending objects

GPU resources for brushmodels and world are streamed in worker thread instead of main thread in asynchronous & in parallel too.

`r_leaf_lazy_load 0` (default) : All GPU resources for brushmodels and world are queued to worker thread and loaded asynchronously at level loading. (May comsume more VRAM and system memory)

`r_leaf_lazy_load 1` : GPU resources for world leaf are loaded only at the time you enter a new leaf. GPU resources for brushmodel are loaded only when it's being rendered.  (Comsume less VRAM and system memory)

* Note that `r_leaf_lazy_load 1` may lead to flickering at the first time you enter a new leaf.

* Note that the corresponding BSP model (including world) will be invisible until necessary GPU resouces arrive.

* Note that more CPU cores you have, faster the GPU resouces loading will be.

`r_studio_lazy_load 0`: Load GPU resouces for all studio models at once when loading map. (May comsume more VRAM and system memory, and may cause Out-of-Memory error in extreme cases)

`r_studio_lazy_load 1` (default): Load GPU resources for studio models only when they are being rendered. (Comsume less VRAM and system memory)

Resources for studiomodels are streamed in worker thread instead of main thread in asynchronous & in parallel, that said, we can utilize more CPU cores now.

* Note that `r_studio_lazy_load 1` makes studiomodels invisible at first few frames until all necessary GPU resources arrive.

* Note that with `r_studio_lazy_load 0`, VRAM and system memory comsumption can be very crazy if server precaches too many studiomodels even without actually using them.

`r_studio_parallel_load 0` (default): Disable multi-threaded loading for studiomodel.

`r_studio_parallel_load 1`: Enable multi-threaded loading for studiomodel. (Warning: may consume more system memory, and may cause Out-of-Memory error in extreme cases)

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

## env_shadow_proxy

`env_shadow_proxy` is a point entity used to set shadow proxy geometry for BSP models.

The following snippet is used to load "maps/de_dust2_shadow.obj" as the shadow proxy geometry for the `de_dust2.bsp` map.

```
{
    "classname" "env_shadow_proxy"
    "model" "maps/de_dust2.bsp"
    "objpath" "maps/de_dust2_shadow.obj"
}
```

You can export .obj from de_dust2.bsp via [newbspguy](https://github.com/UnrealKaraulov/newbspguy).

## env_deferredlighting_control

`env_deferredlighting_control` is a point entity used to control specific parameters under the deferred rendering pipeline.

### Keyvalues

`lightmap_pow` is used to flatten the brightness of lightmap (bringing bright and dark areas to the same level). For example `"lightmap_pow" "0.2"`. The closer the value is to 0, the stronger the flattening effect; when set to 1, it has no effect.

`lightmap_scale` is used to scale the lightmap that has been adjusted by `lightmap_pow`, used to simulate ambient light/indirect lighting. For example `"lightmap_scale" "0.2"`.

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
| "level" "1"                          | skybox will be rendered in reflection                                       |
| "level" "WATER_LEVEL_REFLECT_SKYBOX" | skybox will be rendered in reflection                                       |
| "level" "2"                          | skybox and world will be rendered in reflection                             |
| "level" "WATER_LEVEL_REFLECT_WORLD"  | skybox and world will be rendered in reflection                             |
| "level" "3"                          | skybox, world, entities and particles will be rendered in reflection (expensive)             |
| "level" "WATER_LEVEL_REFLECT_ENTITY" | skybox, world, entities and particles will be rendered in reflection (expensive)             |
| "level" "5"                          | render as legacy water, software-mode style, with pixel-art ripple effects  |
| "level" "WATER_LEVEL_LEGACY_RIPPLE"  | render as legacy water, software-mode style, with pixel-art ripple effects  |

## light_dynamic

`light_dynamic` is a point entity as light source. 

Dynamic lights are calculated on the fly in the game, which means they have a higher processing cost but are much more flexible than static lighting.

### Keyvalues

`origin` is the light source position. for example `"origin" "123 456 789"`

`color` is the RGB color of the dynamic light, RGB must be between 0 and 255. for example `"color" "192 192 192"`

`type` can be either "point", "spot" or "directional"

`size` is the size of dynamic light, in inches. for example `"size" "256"`

`distance` is the distance that spotlight is allowed to cast, in inches. for example `"distance" "3000"` (Only for spot light)

`ambient` is the ambient intensity of dynamic light. for example `"ambient" "0.0"`

`diffuse` is the diffuse intensity of dynamic light. for example `"diffuse" "0.1"`

`specular` is the specular intensity of dynamic light. for example `"specular" "0.1"`

`specularpow` is the specular power of dynamic light. for example `"specularpow" "10.0"`

`shadow` is to enable or disable shadow for this dynamic light. for example `"shadow" "1"`

`static_shadow_size` is the size of static shadow texture for this dynamic light. for example `"static_shadow_size" "256"`. Note that only BSP world will be baked into static shadow.

`dynamic_shadow_size` is the size of dynamic shadow texture for this dynamic light. for example `"dynamic_shadow_size" "256"`.

Example:

```
{
    "origin" "-30 68 72"
    "size" "1024.0"
    "color" "192 192 192"
    "classname" "light_dynamic"
    "type" "directional"
    "ambient" "0.1"
    "diffuse" "1.0"
    "specular" "1.0"
    "specularpow" "10.0"
    "shadow" "1"
}
```
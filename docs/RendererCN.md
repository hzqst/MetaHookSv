# MetaRenderer 文档

[English DOC](/docs/Renderer.md)

# 兼容性

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

* 警告: 该插件与ReShade不兼容.

# GPU 需求

|       厂商      |           Intel                  |          英伟达                  |          AMD               |
|       ----      |           ----                   |           ----                  |           ----             |
|      最低       | Intel Haswell 系列 HD4600         |      Geforce GTX 650            |  AMD Radeon HD 7000 系列 |
|     推荐        |           ----                   |      Geforce GTX 1060 或更高     |   AMD Radeon RX 560 或更高        |

* 该插件需要支持OpenGL4.3或支持更高版本OpenGL的GPU才能运行。

* 运行该插件推荐使用拥有最少4GB独立显存的GPU，否则可能会出现显存带宽瓶颈导致帧数过低，或显存不足导致游戏崩溃等问题。

* 以上建议仅针对1920x1080的分辨率。对于更高的显示分辨率（比如4K），需要的显存带宽和显存容量也会更多。

### 功能

## HDR

HDR (高动态范围) 模拟了超出显示器所能显示的亮度范围，将大于0-255的色彩范围映射到0-255的范围内，并且在超出100%亮度的地方施加Bloom（辉光）特效。

### 控制台参数

`r_hdr` 开启/关闭HDR后处理. 推荐值 : `1`

`r_hdr_blurwidth` 设置HDR的模糊强度. 推荐值 : `0.0 ~ 0.1`

`r_hdr_exposure` 控制HDR的曝光程度. 推荐值 : `0.0 ~ 1.2`

`r_hdr_darkness` 控制HDR的明暗程度. 推荐值 : `0.0 ~ 1.6`

`r_hdr_adaptation` 控制HDR的明暗适应速度. 推荐值 : `0 ~ 50`

## 水面渲染

水面将拥有简单的反射和折射效果

水面可选择“可反射”和“传统”两种渲染模式

“可反射的水面”会实时反射水面上的物体，并折射水面下的物理。该功能需要渲染整个世界两次，所以有一定的性能开销，请根据掉帧严重程度自行斟酌是否开启！

“传统水面”则只会使用基础纹理进行渲染，就像原版GoldSrc中的一样。

反射等级和渲染参数可以使用[env_water_control](RendererCN.md#env_water_control)进行配置。

### 控制台参数

`r_water` 设为1启用“可反射的水面”。

## 逐对象阴影

逐对象阴影只会由模型进行投射 (玩家, 怪物, 武器盒, 尸体, 其他一些模型), 并且只会投射在固体表面. 逐对象阴影是实时计算的, 所以有一定的性能开销，请根据掉帧严重程度自行斟酌是否开启！

* 动态阴影有时候会穿过它本不该穿过的墙和地面，从而暴露玩家或NPC的位置。

### 控制台参数

`r_shadow` 设为1启用逐对象阴影

`r_shadow_angles` 控制阴影投射的角度, 以PitchYawRoll的格式. 举例： `r_shadow_angles "90 0 0"`

`r_shadow_color` 控制阴影的颜色, 以 RGBA8 的格式. 举例： `r_shadow_color "0 0 0 128"`

`r_shadow_distfade` 控制阴影开始淡出的距离，以及阴影的最大投射距离，单位为游戏内的距离单位. 举例：`r_shadow_distfade 64 128`

`r_shadow_lumfade` 控制阴影开始淡出的环境亮度, 以及阴影允许投射的最小环境亮度, 必须在 0 至 255 之间. 举例 `r_shadow_lumfade 64 32`

`r_shadow_high_distance` 该距离内的实体使用高质量的阴影贴图. 举例： `r_shadow_high_distance 400`

`r_shadow_high_scale` 控制渲染高质量的阴影贴图时的模型的缩放大小，缩放大小越大阴影精度越高，但是太大会导致阴影出错。举例： `r_shadow_high_scale 4`

`r_shadow_medium_distance` 该距离内的实体使用中等质量的阴影贴图. 举例： `r_shadow_medium_distance 800`

`r_shadow_medium_scale` 控制渲染中等质量的阴影贴图时的模型的缩放大小，缩放大小越大阴影精度越高，但是太大会导致阴影出错。举例： `r_shadow_medium_scale 2`

`r_shadow_low_distance` 该距离内的实体使用低质量的阴影贴图. 举例： `r_shadow_low_distance 1200`

`r_shadow_low_scale` 控制渲染低质量的阴影贴图时的模型的缩放大小，缩放大小越大阴影精度越高，但是太大会导致阴影出错。举例： `r_shadow_low_scale 0.5`

## 屏幕空间环境光遮蔽

SSAO （屏幕空间环境光遮蔽）是一种在后处理阶段为场景添加环境光遮蔽阴影的特效。

该功能参考了 [HBAO or Horizon-Based-Ambient-Occlusion](https://github.com/nvpro-samples/gl_ssao).

### 控制台参数

`r_ssao` 设为1启用SSAO

`r_ssao_intensity` 控制了SSAO阴影的强度

`r_ssao_radius` 控制了SSAO阴影的采样半径

`r_ssao_blur_sharpness` 控制了SSAO阴影的锐利程度

`r_ssao_bias` 用来在圆滑的曲面上消除不应该产生的SSAO阴影

## 延迟着色渲染管线 和 动态灯光

[延迟着色](https://en.wikipedia.org/wiki/Deferred_shading) 渲染管线被用来渲染不透明物体

并且引入了使用 [Blinn-Phong](https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model) 模型进行计算的实时光照

### 控制台参数

`r_light_dynamic` 设为1启用 延迟着色渲染管线 和 动态灯光

`r_flashlight_cone` 控制手电筒圆锥光束的圆锥夹角cosine值，越接近1则夹角越小，越接近0则夹角越大

`r_flashlight_distance` 控制手电筒的最大照明距离

`r_flashlight_ambient` 控制手电筒的环境光强度

`r_flashlight_diffuse` 控制手电筒的漫反射强度

`r_flashlight_specular` 控制手电筒的高光反射强度

`r_flashlight_specularpow` 控制手电筒的高光反射强度

`r_dynlight_ambient` 控制动态光源的环境光强度

`r_dynlight_diffuse` 控制动态光源的漫反射强度

`r_dynlight_specular` 控制动态光源的高光反射强度

`r_dynlight_specularpow` 控制动态光源的高光反射强度

## 屏幕空间反射

屏幕空间反射（下称SSR） 对屏幕空间中存在的物体进行实时反射。

只有被高光贴图标记了的固体表面 (在`/maps/[map name]_detail.txt`中以`_SPECULAR` 结尾的贴图)才会启用屏幕空间反射

`_SPECULAR`高光贴图的GREEN（绿色）通道代表了反射强度. 0 = 没有反射, 1 = 完全反射.

* 屏幕空间反射只有在 `r_light_dynamic` 为 1 时生效.

### 控制台参数

`r_ssr` 设为1启用屏幕空间反射

`r_ssr_ray_step` 控制光线步进的迭代步长。 举例： `r_ssr_ray_step 5.0`

`r_ssr_iter_count` 控制光线步进的最大迭代次数。 举例：`r_ssr_iter_count 64`

`r_ssr_distance_bias` 光线步进命中的判定距离。 举例： `r_ssr_distance_bias 0.2`

`r_ssr_adaptive_step` 设为1启用自适应迭代步长，可提升迭代性能。 举例： `r_ssr_adaptive_step 1`

`r_ssr_exponential_step` 设为1启用指数迭代步长，可提升迭代性能。 举例：`r_ssr_exponential_step 1`

`r_ssr_binary_search` 设为1启用二分搜索加速，可提升迭代性能。 举例： `r_ssr_binary_search 1`

`r_ssr_fade` 控制SSR效果贴近屏幕边缘时的淡出效果。举例： `r_ssr_fade "0.8 1.0"`

## 高清天空贴图

`r_detailskytextures` 设为1启用

当存在 `gfx/env/[skyname][direction].dds` 或 `renderer/texture/[skyname][direction].dds` 时自动使用该版本的天空贴图。（目前只会使用dds格式）

例子：

`gfx/env/desertbk.bmp` -> `gfx/env/desertbk.dds`

`gfx/env/desertdn.bmp` -> `gfx/env/desertdn.dds`

`gfx/env/desertft.bmp` -> `gfx/env/desertft.dds`

`gfx/env/desertlf.bmp` -> `gfx/env/desertlf.dds`

`gfx/env/desertrt.bmp` -> `gfx/env/desertrt.dds`

`gfx/env/desertup.bmp` -> `gfx/env/desertup.dds`

## BSP细节贴图

细节贴图是一种将高分辨率外部图片 （支持格式: BMP, TGA, DDS, JPG, PNG）与基础贴图混合来提升纹理细节的效果。

`r_detailtextures` 设为1启用细节贴图、法线贴图、视差贴图和高光贴图。

贴图列表会自动从文件 `/maps/[mapname]_detail.txt` 中加载，以 `_DETAIL` 为后缀的贴图会被视为该基础贴图的细节贴图（如果基础贴图没有任何后缀则默认视为细节贴图）。

贴图会按顺序从以下位置尝试加载 (如果文件存在的话)：

1. `/(game_directory)/maps/[texturename]` (贴图名必须以 "maps/" 或 "maps\" 开头)

2. `/(game_directory)/gfx/detail/[texturename]`

3. `/(game_directory)/renderer/texture/[texturename]`

* 当路径不包含扩展名时，将默认添加`.tga`扩展名。

### BSP法线贴图

法线贴图是一种使用外部贴图作用于特定固定表面，以改变其法线朝向的一种效果。

贴图列表会自动从文件 `/maps/[map name]_detail.txt` 中加载, 以 `_NORMAL` 为后缀的贴图会被视为该基础贴图的法线贴图。

列表中指定的法线贴图文件会从 `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` 和 `/Sven Co-op/svencoop/renderer/texture` 中加载（支持格式: BMP, TGA, DDS, JPG, PNG）。

* BSP法线贴图只会改变固体表面的法线朝向, 因此只在被动态光源和手电筒照亮的表面起作用。

* BSP法线贴图只有在 `r_detailtextures` 和 `r_light_dynamic` 都设为 1 时有效。

### BSP视差贴图

视差贴图是一种使用外部贴图作用于特定固定表面，改变其视觉深度以营造一种凹陷突起的视觉效果。

贴图列表会自动从文件 `/maps/[map name]_detail.txt` 中加载, 以 `_PARALLAX` 为后缀的贴图会被视为该基础贴图的视差贴图。

列表中指定的视差贴图文件会从 `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` 和 `/Sven Co-op/svencoop/renderer/texture` 中加载（支持格式: BMP, TGA, DDS, JPG, PNG）。

* 控制台参数 `r_wsurf_parallax_scale` 可以用于控制视差(凹陷/突起)效果的最大强度（果为负则改变凹陷/突起的方向）。

* BSP视差贴图只有在 `r_detailtextures` 设为 1 时有效。

### BSP高光贴图

高光贴图是一种使用外部贴图作用于特定固定表面，以增强其高光反射强度的效果。

贴图列表会自动从文件 `/maps/[map name]_detail.txt` 中加载, 以 `_SPECULAR` 为后缀的贴图会被视为该基础贴图的高光贴图。

列表中指定的视差贴图文件会从 `/Sven Co-op/svencoop_(addon,downloads)/gfx/detail/` 和 `/Sven Co-op/svencoop/renderer/texture` 中加载（支持格式: BMP, TGA, DDS, JPG, PNG）。

* BSP高光贴图的红色分量（RGB的R）控制高光反射强度, 绿色分量（RGB的G）控制SSR（屏幕空间反射）的强度。

* 蓝色分量暂时未使用。

* BSP高光贴图只有在 `r_detailtextures` 设为 1 时有效。

## 模型贴图替换

你需要在`[modelname].mdl`模型的同目录下创建 `[modelname]_external.txt`文件，文件应包含以下内容：

### 如果"replacetexture"的贴图名以 "models/" 或 "models\" 开头：

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "replacetexture"  "models/mymodel/base_texture.dds" 
    "replacescale" "1.0 1.0"
}
```

那么以下位置的文件会被用来替换模型中自带的BMP贴图:

`(game_directory)\models\mymodel\base_texture.dds`

### 否则

以下位置的文件会被用来替换模型中自带的BMP贴图:

`(game_directory)\gfx\base_texture.dds`

`(game_directory)\renderer\texture\base_texture.dds`

* 当路径不包含扩展名时，将默认添加`.tga`扩展名。

### UV 控制

`"replacescale" "1.0 1.0"` : 计算纹理坐标时使用的贴图宽高会被替换为 (1.0 x 新贴图的宽, 1.0 x 新贴图的高).

`"replacescale" "1.0"` : 等价于 `"replacescale" "1.0 1.0"`

`"replacescale" "-1.0 -1.0"` : 计算纹理坐标时使用的贴图宽高会被替换为 (1.0 x 原始BMP贴图的宽, 1.0 x 原始BMP贴图的高).

* 没有"replacescale"时不对UV缩放做出任何调整

* MDL中存储的UV格式是-32767至32768的无符号USHORT整型，具体值等于在原始BMP贴图上的以像素计的绝对坐标。所以最终传递给GPU的取值范围0至1的真实UV需要由该USHORT类型除以贴图宽高来算出，上述"replacescale"修改的就是这里所谓的“贴图宽高”。

### 控制台参数

* 使用控制台参数 `r_studio_external_textures 0` 可以临时禁用贴图替换。

## 模型法线贴图

你需要在`[modelname].mdl`模型的同目录下创建 `[modelname]_external.txt`文件，文件应包含以下内容：

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "normaltexture"  "normal_texture.dds" //贴图搜索路径和搜索规则参考 "replacetexture"
}
```

### 控制台参数

* 使用控制台参数 `r_studio_external_textures 0` 可以临时禁用法线贴图。

## 模型高光贴图

你需要在`[modelname].mdl`模型的同目录下创建 `[modelname]_external.txt`文件，文件应包含以下内容：

```
{
    "classname" "studio_texture"
    "basetexture" "base_texture.bmp"
    "speculartexture"  "specular_texture.dds" //贴图搜索路径和搜索规则参考 "replacetexture"
}
```

高光贴图的红色通道会被用于表示高光强度。

绿色通道用于表示SSR（屏幕空间反射强度），一般情况下不使用绿色通道。

蓝色通道用于表示球面化法线的插值比例，越接近1插值比例越高，法线越接近于球面化。（需要拥有`STUDIO_NF_CELSHADE_FACE`属性才生效）

### 控制台参数

* 使用控制台参数 `r_studio_external_textures 0` 可以临时禁用法线贴图。

## 模型ALPHA半透明贴图支持

跟 `STUDIO_NF_ADDITIVE` 效果类似，只是使用ALPHA通道而非颜色叠加模式进行半透明混合。带 `STUDIO_NF_ALPHA` 属性的贴图会被延迟到半透明阶段再进行绘制（如果是来自不透明的实体的话）。

你需要使用模型贴图替换功能将基础贴图替换为带ALPHA通道的贴图，比如内部编码格式为DXT5-BC7的DDS，TGA或PNG。

需要添加以下内容到 `[modelname]_external.txt` 文件中:

```
{
    "classname" "studio_texture"
    "basetexture" "basetexture.bmp"
    "replacetexture" "replacetexture.dds" // 该贴图必须带ALHAP通道!!!
    "flags" "STUDIO_NF_ALPHA"
}
```

## 模型双面渲染

拥有 `STUDIO_NF_DOUBLE_FACE` 属性的贴图会被渲染正反两个表面。

需要添加以下内容到 `[modelname]_external.txt` 文件中:

```
{
    "classname" "studio_texture"
    "basetexture" "basetexture.bmp"
    "flags" "STUDIO_NF_DOUBLE_FACE"
}
```

## 卡通渲染 / 描边  / 边缘光 / 刘海阴影 / 头发高光

为了给指定的模型增加卡通渲染 / 描边  / 边缘光 / 刘海阴影 / 头发高光的效果，

你需要在`[modelname].mdl`模型的同目录下创建 `[modelname]_external.txt`文件，文件应包含以下内容：

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

来为 `[modelname].mdl` 模型启用上述特效。

需要注意的是 `face.bmp` 和 `hair.bmp` 应保证与  `[modelname].mdl` 模型中真正的面部和头发贴图名一致。

或者参考 `Build\svencoop_addon\models\player\GFL_HK416\GFL_HK416_external.txt` 中提供的示例文件。

卡通渲染的参数会优先使用 `[modelname]_external.txt` 中的 studio_celshade_control 键值对：（举例）

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
```

如果键值对不存在，则使用对应的控制台参数。

被`EF_OUTLINE`标记的模型会产生描边，具体参数见`r_studio_outline_size`和`r_studio_outline_dark`。

被`STUDIO_NF_CELSHADE`标记的表面会有一些特殊处理：

1. 光照和阴影使用特殊的CelShade算法进行计算

2. 光照计算所使用的光线方向的Z轴会被压缩为更小的值，来达到接近水平照射的效果（具体压缩比例见`r_studio_celshade_lightdir_adjust`）

被`STUDIO_NF_CELSHADE_FACE`标记的表面除了包含`STUDIO_NF_CELSHADE`的效果外，还有一些特殊处理：

1. 根据所使用高光贴图的蓝色通道的强度，其法线会被替换为球面化法线，蓝色通道越接近1插值程度越高。蓝色通道为0或者无高光贴图时则使用模型原生法线（法线贴图仍生效）。

2. 仅在该种类型的表面上才会投射来自头发的阴影。具体参数见`r_studio_hair_shadow_offset`。

被`STUDIO_NF_CELSHADE_HAIR`标记的表面除了包含`STUDIO_NF_CELSHADE`的效果外，还有一些特殊处理：

1. 其会往拥有`STUDIO_NF_CELSHADE_FACE`标记的表面上投射阴影。具体参数见`r_studio_hair_shadow_offset`。

2. 其表面会产生Kajiya-Kay Shading算法计算出的高光，具体参数见`r_studio_hair_specular_[...]`。该高光在头发加载了高光贴图的情况下，仅会在高光贴图的红色通道上有颜色的区域上显示头发高光，并且强度按红色通道强度进行插值。如果头发没有加载高光贴图则无显示区域限制。

### 下半身模型

使用控制台参数 `r_drawlowerbody 1` 来启用渲染下半身模型的功能。

你需要在`[modelname].mdl`模型的同目录下创建 `[modelname]_external.txt`文件，文件应包含以下内容：

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
    "model_scale" "1"
}
```

以使得 `[modelname].mdl` 可以作为下半身模型被正确渲染。

* 任何没有被标记为 `STUDIO_BF_LOWERBODY` 的部位都将在渲染时被剔除。

* 你可以使用 Half-Life Asset Manager 等工具查看真实的骨骼名称。

* `model_origin` 用于调整下半身模型的位置偏移。

* `model_scale` 用于下半身模型的缩放 (仅支持Sven Co-op)。

### 控制台参数

`r_studio_celshade` 设为 1 启用卡通渲染 / 描边  / 边缘光 / 刘海阴影 / 头发高光。

`r_studio_celshade_midpoint` 和 `r_studio_celshade_softness` 控制卡通渲染阴影的柔软程度。

`r_studio_celshade_shadow_color` 控制卡通渲染阴影的颜色。

`r_studio_celshade_head_offset` 控制人物头部相对父骨骼的坐标偏移

`r_studio_celshade_lightdir_adjust` 用于缩放卡通渲染光照处理环节时使用的光照的方向向量的Z轴值（即从z轴方向上对光线方向进行压缩，使得其趋近于水平照射）。第一个值作用于除面部外的其他部位，第二个值作用于面部。

`r_studio_outline_size` 控制描边的大小。

`r_studio_outline_dark` 控制描边的颜色。

`r_studio_rimlight_power` 控制亮侧边缘光的强度。

`r_studio_rimlight_smooth` 控制亮侧边缘光的柔软程度。

`r_studio_rimlight_smooth2` 控制亮侧边缘光的在黑暗环境下的淡出表现。

`r_studio_rimlight_color` 控制亮侧边缘光的颜色。

`r_studio_rimdark_power` 控制暗侧边缘光的强度。

`r_studio_rimdark_smooth` 控制暗侧边缘光的柔软程度。

`r_studio_rimdark_smooth2` 控制暗侧边缘光的在黑暗环境下的淡出表现。

`r_studio_rimdark_color` 控制暗侧边缘光的颜色。

`r_studio_hair_specular_exp` 控制头发主高光照亮的区域大小，hair_specular_exp的值越大，照亮的区域越小。

`r_studio_hair_specular_noise` 控制头发主高光的噪声值，噪声用于生成类似WWW的波纹形状

`r_studio_hair_specular_intensity` 控制头发主高光的RGB强度

`r_studio_hair_specular_exp2` 控制头发次高光照亮的区域大小，hair_specular_exp2的值越大，照亮的区域越小。

`r_studio_hair_specular_noise2` 控制头发次高光的噪声值，噪声用于生成类似WWW的波纹形状

`r_studio_hair_specular_intensity2` 控制头发次高光的RGB强度

`r_studio_hair_specular_smooth` 控制头发高光在黑暗环境下的淡出表现。

`r_studio_hair_shadow_offset` 控制刘海阴影在屏幕空间下相对水平和垂直方向的偏移。

## 顶点缓冲对象 (又称 VBO) 批量绘制优化

固体、模型、印花和Sprite均使用VBO进行渲染。渲染所需的顶点数据等信息都会提前保存在显存中，而非像原版一样每帧都从内存中重新提交到GPU，极大提升了绘制效率。

## Bindless-Texture优化

固体和印花可以采用Bindless-Texture技术进行绘制，可以大幅度减少绘制次数，但是在某些实现存在问题的显卡驱动上会出现显示异常等问题（尤其是AMD和Intel的显卡驱动）。

可以使用控制台参数 `gl_bindless 1` 来启用该功能。

启动项`-nobindless`可以永久关闭该功能，以防止某些显卡驱动在启动游戏时就出现问题。

## 顺序无关透明混合渲染 (又称OIT混合)

透明像素被保存在GPU链表中并由GPU进行排序，以达到像素级精确的半透明混合。

添加启动参数 `-oitblend` 来启用OIT混合 (默认不启用)

* 警告：该功能在透明物体较多的情况下会严重影响性能。

## 新的贴图加载器

### 原版贴图（WAD/SPR）

最大允许的WAD/SPR贴图尺寸提升至 4096 x 4096。

该功能默认启用，并且可能会导致最终加载的WAD/SPR贴图与原版引擎的有轻微的视觉上的差别。

如果要使用原版的贴图加载器，请在启动项中添加 `-use_legacy_texloader`。

### 外部贴图

### 最大允许的外部贴图尺寸取决于你的GPU和OpenGL驱动实现（稍老一点显卡的也有4096x4096，新显卡一般都支持16384x16384）。

支持的外部贴图格式：

BMP (Indexed / RGB8 / RGBA8)

TGA (RGB8 / RGBA8)

DDS (DX10 BC1 / DX10 BC2 / DX10 BC3 / DX10 BC7 / Legacy DXT1 / Legacy DXT3 / Legacy DXT5）

HDR (RGB8F / RGBA8F)

JPEG (RGB8)

PNG (RGB8 / RGBA8)

WEBP (RGB8 / RGBA8)

## 抗锯齿

`r_fxaa` 设为1启用快速近似抗锯齿 (FXAA).

* 由于多重采样抗锯齿（MSAA）与延迟渲染不兼容，因此MSAA被彻底移除。

## Gamma矫正

原版GoldSrc的Gamma矫正在初始化和贴图加载阶段就已完成，而新的Gamma矫正由shader在运行时完成，允许你在游戏中途修改gamma、texgamma等数值。

所有贴图会从gamma空间转换为线性空间后再进行光照计算

* 对gamma矫正后的颜色进行任何数学运算（如透明混合）得到的结果都是物理错误的，也就是说Valve默认的混合策略是物理错误的，只有在线性空间对颜色进行数学运算才是物理上正确的！具体请参考：https://zhuanlan.zhihu.com/p/510697986

* 可以使用控制台参数 `r_gamma_blend 1` 来启用原版的在gamma空间中进行半透明混合的行为。这将使得半透明的混合结果与原版引擎保持一致。但是由于某些显卡驱动的实现存在问题，该功能可能会导致出现画面出现显示异常，请自行斟酌是否开启该功能。

### 控制台参数

`gamma` 控制最终输出画面的gamma值, 用于将颜色从线性空间转换至gamma空间。

`texgamma` 用于将贴图的颜色从gamma空间转换至线性空间。

`lightgamma` 用于将光照贴图的颜色从gamma空间转换至线性空间。

`brightness` 用于偏移lightgamma来让光照贴图的结果更亮

`r_blend_gamma 0 / 1` 设为1时在gamma空间做透明混合（即使用原版GoldSrc使用的透明混合策略），设为0时在线性空间做透明混合。

## FOV (视场角度)

### 第一人称武器模型独立FOV

可以用控制台参数 `viewmodel_fov [数值]`来单独调整。 使用 `viewmodel_fov 0` 来关闭自定义第一人称武器FOV。

### 垂直FOV

金源默认使用水平FOV策略，你可以通过控制台参数 `r_vertical_fov 1` 启用垂直FOV策略。注：Sven Co-op默认使用垂直FOV策略，也就是说你输入的default_fov参数在Sven Co-op中会被当成屏幕垂直方向上的FOV而非金源的水平方向FOV。

### 宽屏自适应FOV

你可以通过控制台参数 `gl_widescreen_yfov 1` 或 `gl_widescreen_yfov 2` 启用宽屏自适应FOV。该策略控制在金源默认的水平FOV策略下如何从水平FOV计算垂直FOV：

`gl_widescreen_yfov 1` 是 Xash3D-fwgs 和 Counter-Strike : Online 以及 Half-Life 25周年更新版本中 默认使用的宽屏FOV策略，该策略下会维持垂直FOV为非宽屏即4:3长宽比的分辨率下的FOV值，并在水平方向上扩展FOV。 

`gl_widescreen_yfov 2` 则是拉伸4：3长宽比的分辨率的画面并拉伸至宽屏。

## Sprite插值

`r_sprite_lerping` 1 / 0: 使 rendermode 为 texture 和 additive 的 Sprite 播放的动画更加平滑。

* 代码来自 Xash3d-fwgs

* 注意: 为保证插值结果正确，sprite实体在服务器上的逻辑帧率 (不是sprite实体的pev.framerate）必须为10FPS。这里的10FPS指的是sprite实体两次think函数之间的执行间隔等于0.1秒，这在原版及大多数mod中都是默认设置。

## Quake风格的水下画面扭曲效果

`r_under_water_effect` 1 / 0

`r_under_water_effect_wave_amount` 默认值: 10.0

`r_under_water_effect_wave_speed` 默认值: 1.0

`r_under_water_effect_wave_size` 默认值: 0.01

## 其他

`r_wsurf_zprepass` 1 / 0 : 设为1时启用Z-Prepass优化。
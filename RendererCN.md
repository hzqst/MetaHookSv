# MetaRenderer 文档

[English DOC](Renderer.md)

### Features

1. HDR后处理

2. 简单水面反射折射 (警告：开启该功能可能导致严重的性能损失，请根据掉帧严重程度自行斟酌是否开启！)

3. 简单逐对象阴影 (警告：开启该功能可能导致严重的性能损失，请根据掉帧严重程度自行斟酌是否开启！）

4. 屏幕空间遮蔽 (SSAO) (警告：开启该功能可能导致严重的性能损失，请根据掉帧严重程度自行斟酌是否开启！）（警告：非90的default_fov可能会导致墙上隐约出现黑色裂痕）

5. 多重采样抗锯齿 (MSAA) (警告：开启该功能可能导致轻微的性能损失，但配合SSAO使用则可能严重影响帧数，请根据掉帧严重程度自行斟酌是否开启！）

6. 快速近似抗锯齿 (FXAA) （仅当MSAA不可用时）

7. 延迟光照技术和逐像素光照渲染（仅不透明物体），支持“无限”的动态光源(SvEngine最多256个)并且几乎没有性能损失

9. mdl模型渲染可使用顶点缓冲对象、合并DrawCall、GPU光照来进行优化，使用这些技术可以解放CPU算力，做到同屏渲染20万多边形的mdl仍能维持在可接受的帧数

10. bsp地形渲染可使用顶点缓冲对象、合并DrawCall来进行优化，使用这些技术可以解放CPU算力，做到同屏渲染10万多边形的bsp地形仍能维持在可接受的帧数

11. bsp地形渲染支持法线贴图、视差贴图，具体见svencoop/maps/restriction02_detail.txt里自带的例子

12. 修复一个引擎BUG：模型的remap贴图不支持STUDIO_NF_MASKED标记.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/2.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/4.png)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/5.png)

### 兼容性

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

GoldSrc引擎: 只兼容从 https://github.com/hzqst/MetaHookSv 构建的改进版metahook启动器(metahook.exe), 不兼容其他来源的metahook启动器。

#### 控制台参数

r_hdr 1 / 0 : 开启/关闭HDR后处理. 推荐值 : 1

r_hdr_blurwidth :设置HDR的模糊强度. 推荐值 : 0.1

r_hdr_exposure : 控制HDR的曝光程度. 推荐值 : 5

r_hdr_darkness : 控制HDR的明暗程度. 推荐值 : 4

r_hdr_adaptation : 控制HDR的明暗适应速度. 推荐值 : 50

r_water 2 / 1 / 0 : 控制是否开启水面反射/折射。 2 = 反射所有物体, 1 = 仅反射地形. 推荐值 : 1

r_water_fresnel (0.0 ~ 2.0) : 控制菲涅尔效应强度，如何混合折射和反射画面. 推荐值 : 1.5

r_water_depthfactor (0.0 ~ 1000.0) : 控制距离水底的深度对水体透明度的影响，原本完全不透明的水体不受此参数影响永远保持完全不透明. 推荐值 : 50

r_water_normfactor (0.0 ~ 1000.0) : 控制水面波纹大小. 推荐值 : 1.5

r_water_novis 1 / 0 : 强制让绘制折射反射贴图时关闭VIS可视区域裁剪，可能会导致性能严重下降。 推荐值 : 0

r_water_minheight : 小于这个高度的水体不会被应用折射和反射。 推荐值 : 7.5

r_shadow 1 / 0 : 开启/关闭 逐对象阴影. 推荐值 : 1

r_shadow_angle_pitch (0.0 ~ 360.0) : 控制阴影投射源的倾角（或叫攻角）(pitch).

r_shadow_angle_yaw (0.0 ~ 360.0) : 控制阴影投射源的左右偏转角(yaw).

r_shadow_angle_roll (0.0 ~ 360.0) : 控制阴影投射源的滚动旋转角（roll）.

r_shadow_high_distance : 这个距离内的实体使用高质量阴影贴图. 推荐值 : 400

r_shadow_high_scale : 使用高质量阴影贴图的实体的缩放大小，缩放过大可能导致一些体型较大的实体阴影出现渲染错误，缩放过小会严重影响阴影质量。 推荐值 : 4.0

r_shadow_medium_distance : 这个距离内的实体使用中等质量阴影贴图. 推荐值 : 1024

r_shadow_medium_scale : 使用中等质量阴影贴图的实体的缩放大小 推荐值 : 2.0

r_shadow_low_distance : 这个距离内的实体使用中等质量阴影贴图. 推荐值 : 4096

r_shadow_low_scale : 使用低质量阴影贴图的实体的缩放大小 推荐值 : 0.5

r_ssao 1 / 0 : 开启关闭屏幕空间遮蔽（SSAO）. 推荐值 : 1 （和MSAA一起使用可能导致严重的性能下降！）

r_ssao_intensity : SSAO阴影的强度. 推荐值 : 0.6 ~ 1.0

r_ssao_radius : SSAO阴影的采样半径. 推荐值 : 30.0 ~ 100.0

r_ssao_blur_sharpness : SSAO阴影的锐化程度， 推荐值 : 1.0

r_ssao_bias : 功能未知. 推荐值 : 0.2

r_ssao_studio_model : 0 / 1 SSAO是否对mdl模型本身生效. 推荐值 : 0

r_light_dynamic 1 / 0 : 开启/关闭延迟光照和动态光源. 推荐值 : 1

r_flashlight_cone : 手电筒光锥的锥体夹角cosine值. 推荐值 : 0.9

r_flashlight_distance : 手电筒照明距离. 推荐值 : 2000.0

r_light_ambient : 动态光照的环境光强度. 推荐值 : 0.2

r_light_diffuse : 动态光照的漫反射光强度. 推荐值 : 0.3

r_light_specular : 动态光照的高光反光强度. 推荐值 : 0.1

r_light_specularpow : 动态光照的高光反光强度. 推荐值 : 10.0

r_studio_vbo 1 / 0 : 开启/关闭mdl模型的VBO优化和DrawCall合批优化. 推荐值 : 1

r_wsurf_vbo 1 / 0 : 开启/关闭bsp地形的VBO优化和DrawCall合批优化. 推荐值 : 1

r_wsurf_parallax_scale : 控制视差贴图的作用强度. 推荐值 : 0.01 ~ 0.04

r_wsurf_sky_occlusion 1 / 0 : 设为1时被天空贴图遮挡的场景将不可见. 该选项仅在r_wsurf_vbo为1时有效. 推荐值 : 1

r_fxaa 1 / 0 : 开启快速近似抗锯齿 (FXAA) ，仅当 MSAA 不可用时有效. 推荐值 : 1

r_msaa 0 / 2 / 4 / 8 / 16 : 开启或关闭多重采样抗锯齿 (MSAA) . 推荐值 : 0 或 4 （和SSAO一起使用可能导致严重的性能下降！）
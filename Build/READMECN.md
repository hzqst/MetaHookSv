# 可安装资源

### MetaHook.exe / MetaHook_blob.exe (必须)

metahook主启动器

* 使用 MetaHook_blob.exe 而不是 MetaHook.exe 来加载旧版的加密引擎（如3248或3266版本的引擎）。

### SDL2.dll 与 SDL3.dll (可选)

* `Build`目录中的 `SDL3.dll` 用于支持输入法候选词功能，因为原生的 SDL2 不会将输入法候选事件传递给引擎。

* `SDL3.dll` 通过 [SDL3-over-SDL2](https://github.com/libsdl-org/sdl2-compat) 兼容层进行加载，这意味着您需要用 `Build/SDL2.dll` 替换游戏自带的SDL2，以使 SDL3 正常工作。

### svencoop/captionmod/* (可选)

CaptionMod.dll的依赖项，如果要使用CaptionMod.dll则必须安装

### svencoop/metahook/configs/plugins_svencoop.lst (必须，且需要重命名)

### svencoop/metahook/configs/plugins_goldsrc.lst (必须，且需要重命名)

必须重命名为 `plugins.lst`. 

插件加载列表

你可以编辑该文件以启用、禁用或添加新的插件

### svencoop/metahook/configs/plugins/*.dll (可选)

所有可用插件

### svencoop/metahook/renderer/shader/* (可选)

着色器，Renderer.dll的依赖项，如果要使用Renderer.dll则必须安装

### svencoop/metahook/renderer/texture/* (可选)

贴图，Renderer.dll的依赖项，如果要使用Renderer.dll则必须安装

### svencoop_downloads/maps/restriction02_detail.txt (可选)

用来演示在restriction02.bsp地图中添加视差贴图、法线贴图和高光贴图，需要启用Renderer.dll插件才能生效

### svencoop_downloads/maps/*.csv (可选)

用来翻译对应地图中的英文文本，需要启用CaptionMod.dll插件才能生效

### svencoop_downloads/models/* (可选)
### svencoop_downloads/sound/* (可选)

启用布娃娃效果时使用的资源，需要启用BulletPhysics.dll插件才能生效

# 可安装资源

### MetaHook.exe / MetaHook_blob.exe (必须)

metahook主启动器

* 使用 MetaHook_blob.exe 而不是 MetaHook.exe 来加载旧版的加密引擎（如3248或3266版本的引擎）。

### SDL.dll (可选)

`SDL2.dll`文件是用来修复原版SDL使用中文输入法进行游戏时可能发生的内存越界写入导致游戏崩溃的问题。如果你全程都关闭中文输入法的话也可以选择不替换`SDL2.dll`。

* Valve在HL25周年补丁中更新了修复了缓冲区越界问题的SDL2，所以如果你是HL25周年正版就不需要替换SDL2.dll

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

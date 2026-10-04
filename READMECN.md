# MetaHookSv

[MetaHook](https://github.com/nagist/metahook)的Sven Co-op移植版本

用黑科技提升你的Sven Co-op游戏体验

大部分插件都兼容原版GoldSrc引擎，具体每个插件的兼容性情况请逐个查阅插件文档。

[英文README](README.md)

### 兼容性 (仅metahook加载器本体)

|        引擎                 |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (< 6153)     | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

## 下载

[GitHub Release](https://github.com/hzqst/MetaHookSv/releases)

* 因某些国内线路问题无法下载或下载过慢的话可以百度搜索GitHub加速镜像.随便找一个国内能直接访问的加速镜像站，往里复制从Release页面上复制的下载地址即可加速下载。

* 下载 `MetaHookSv-windows-x86.7z`，统一包包含普通和 blob 启动器。

## VAC风险?

虽然在游戏中使用hook之类的行为可能看上去很危险，但是目前为止还没有人反馈因为使用此插件导致VAC封禁。

并且Sven Co-op并不属于[受VAC保护的游戏](https://store.steampowered.com/search/?term=Sven&category2=8)

你甚至可以以添加命令行参数`-insecure`的方式加所谓“受VAC保护的服务器”，因为Sven Co-op上的VAC根本就没有工作。

如果你实在不放心，那么请使用小号进行游戏，毕竟Sven Co-op是免费游戏。

## 常见问题

1. 为什么使用盗版/旧版引擎进行游戏时，游戏进程会周期性卡住几秒？

Q: 因为V社在引擎的主循环中使用了一个阻塞式API `gethostbyname` 来请求域名。该API在请求已失效的域名的时候就是会阻塞当前进程直到超时返回的，这是Windows的设定。

你可以通过在启动项中添加 `-nomaster` 或 `-steam` 来缓解该问题。（ `-steam` 在某些NoSteam盗版版本上可能导致游戏无法启动）

2. 为什么游戏进程会在退出/重启时卡住很久 ?

Q: 因为 ThreadGuard.dll 会在游戏退出时强制等待 V社创建的网络线程退出，以防游戏意外崩溃。具体见 [ThreadGuard](https://github.com/hzqst/MetaHookSv#threadguard)。

3. 为什么我进到主菜单界面之后就会黑屏？

A. 因为 [SDL3-over-SDL2 兼容层](https://github.com/libsdl-org/sdl2-compat) 在软件渲染模式下无法正常工作。请切换至 OpenGL 模式以解决问题（在游戏启动项中添加 `-gl` 即可）。

4. 如果游戏占用内存过多(超过2.7GB)导致崩溃怎么办？

尝试添加以下启动项：`-metahook_early_unload_mirrored_dll` （该选项可节约大约120MB内存）

尝试启用以下控制台参数：`r_studio_lazy_load 1`， `r_leaf_lazy_load 1`

5. 为什么在 `sv_cheats 1` 之后，地形变全黑了 ?

请设置 `r_lightmap 1`

## 一键安装方式 (GUI安装器)

1. 下载 release 并完整解压。
2. 保持 `MetahookInstaller.exe` 与 `install/output/` 在一起。运行安装器，选择游戏后点击 **安装**。
3. Sven Co-op 从 Steam 或生成的快捷方式启动；其他游戏使用生成的 `MetaHook for [GameName].lnk`。

安装器根据自身 EXE 的位置查找 `install/output/`，不依赖当前工作目录。它会选择普通或 blob 启动器，将通用资源映射到所选 Mod，并安装运行库 DLL。已有 `plugins.lst` 中的选择会保留。

如需脚本化安装，可使用同一压缩包中的 `MetahookInstallerCLI.exe`，执行的步骤与 GUI 相同：

```powershell
.\MetahookInstallerCLI.exe -appid 225840                                         # 安装到 Steam 版 Sven Co-op
.\MetahookInstallerCLI.exe -appid 70 -gamedir "D:\Games\Half-Life" -moddir gearbox  # 指定游戏根目录与 Mod
.\MetahookInstallerCLI.exe -appid 225840 -uninstall
```

完整参数见[安装器文档](toolsrc/MetahookInstaller/README.md#cli-usage)。

## 手动安装方式

运行文件位于 `install/output/`。安装到 Sven Co-op 时，将 `svencoop/`、其配套资源目录和 `platform/` 合入游戏根目录。其他 Mod 应将通用 `svencoop/` 资源合入所选 Mod 目录，再复制该 Mod 的配套目录。

将所需启动器、`libcurl.dll` 和 `steam_api.dll` 复制到游戏根目录。Sven Co-op 将普通启动器重命名为 `svencoop.exe`；其他游戏使用 `MetaHook.exe -insecure -game <Mod目录>` 启动。旧版 blob 引擎使用 `MetaHook_blob.exe`。

在 Mod 的 `metahook/configs/` 下，将 Sven Co-op 的 `plugins_svencoop.lst` 或其他游戏的 `plugins_goldsrc.lst` 复制为 `plugins.lst`。已有用户列表时保留原文件。Sven Co-op 默认启用 BetterSpray。

普通引擎导入 SDL2 时，应同时安装 `install/output/SDL2.dll` 和 `SDL3.dll`，以支持输入法候选词。GUI 安装器会自动判断此条件。

## 构建需求

1. Visual Studio 2022，以及 VC143工具集。

2. CMake

3. Git 客户端

4. Python 3.8+（同步 gamedata），以及 .NET 8 SDK（构建安装器）。

## 如何构建

递归克隆后，在仓库根目录使用 PowerShell 执行：

```powershell
git clone --recursive https://github.com/hzqst/MetaHookSv
Set-Location MetaHookSv
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 "-DCMAKE_INSTALL_PREFIX=$PWD/install/output"
cmake --build build --config Release --parallel 2
cmake --install build --config Release
```

聚合管线构建 MetaHook、MetaHook_blob、所有启用的插件（包括 BetterSpray）、共享插件库和 CMake 工具。插件列表模板由 `assets/svencoop/metahook/configs/` 管理并安装到输出目录。.NET 工具单独构建，详见[安装器文档](toolsrc/MetahookInstaller/README.md)。

`windows.yml` 构建、验证并打包安装器 EXE 和完整的 `install/output/`。标签发布复用统一 Windows artifact 与独立 BSP 工具 artifact，并继续生成中英文 AI release notes。

## 如何调试

使用 `--config Debug` 构建并安装，将输出部署到游戏目录。用 Visual Studio 打开 `build/MetaHookSv.sln`，选择 Debug/Win32，再按游戏位置设置启动器的调试命令、工作目录及 `-insecure -game <Mod目录>` 参数。

## 文档

[中文文档](docs/MetaHookCN.md) [ENGLISH DOC](docs/MetaHook.md)

## 插件列表

### CaptionMod

这是一个使用VGUI2来显示字幕、翻译英文HUD消息和VGUI文本的插件。

除此之外还为游戏添加了起源风格的聊天框以及高DPI支持。

对Sven Co-op而言，该插件修复了游戏中的汉字无法显示或者乱码的问题。

[中文文档](docs/CaptionModCN.md) [ENGLISH DOC](docs/CaptionMod.md)

### BulletPhysics

对游戏提供布娃娃支持。玩家死亡时以及玩家被藤壶、喷火怪抓住时将玩家模型转化为布娃娃。

[中文文档](docs/BulletPhysicsCN.md) [ENGLISH DOC](docs/BulletPhysics.md)

### MetaRenderer

替换了原版的图形渲染引擎，极大提升了渲染性能，使用了黑科技提升你的画质和帧率。

[中文文档](docs/RendererCN.md) [ENGLISH DOC](docs/Renderer.md)

### StudioEvents

该插件可以防止重复播放模型自带音效，防止音效反复刷屏。

[中文文档](docs/StudioEventsCN.md) [ENGLISH DOC](docs/StudioEvents.md)

### SteamScreenshots (只支持Sven Co-op)

该插件捕获了`snapshot`截图命令，将其重定向到Steam客户端自带的截图功能上。

### SCModelDownloader (只支持Sven Co-op)

该插件自动从 https://wootguy.github.io/scmodels/ 下载缺失的玩家模型。

控制台参数 : `scmodel_autodownload 0 / 1` 设为1时启用自动下载

控制台参数 : `scmodel_downloadlatest 0 / 1` 设为1时自动下载最新版本的模型（如果有多个版本的模型）

### CommunicationDemo (只支持Sven Co-op)

该插件开放了一个接口用于进行客户端-服务端双向通信。

### PrecacheManager

该插件提供了一个命令 `fs_dump_precaches` 用于dump预缓存的游戏资源列表到 `[ModDirectory]\maps\[mapname].dump.res` 文件中。

* Sven Co-op 的声音系统使用 `soundcache.txt` 而非引擎的预缓存系统来维护声音文件的预缓存列表。

### ThreadGuard

该插件接管了Valve的一些模块的线程创建行为，这些模块创建线程后在模块释放时不会等待线程结束，这可能会导致游戏退出或热重启时游戏进程随机崩溃。

目前接管的模块：

`hw.dll`, `GameUI.dll`, `ServerBrowser.dll`

### ResourceReplacer

该插件可以动态替换游戏内资源 (主要是模型和声音文件) 且无需修改磁盘上的文件，就像 Sven Co-op 的 [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) 和 [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) 文件提供资源替换功能一样。

[中文文档](docs/ResourceReplacerCN.md) [ENGLISH DOC](docs/ResourceReplacer.md)

### SCCameraFix  (只支持Sven Co-op)

该插件修复了Sven Co-op的观察者模式下摄像机视角/画面高频抖动的问题。

部分代码来自[halflife-updated](https://github.com/SamVanheer/halflife-updated)

### ABCEnchance (第三方) (只支持Sven Co-op)

该插件提供以下功能：

1. CSGO 风格的血量和弹药 HUD
2. 2077风格的转盘武器选择菜单
3. 伤害来源指示器
4. 动态准星
5. 实时更新的小地图（略微消耗渲染性能）
6. 漂浮文字显示队友的血量、护甲、名字.
7. 其他一些没什么用的特效

https://github.com/DrAbcrealone/ABCEnchance

### HUDColor (第三方) (只支持Sven Co-op)

该插件可以修改游戏中HUD的颜色。

也可以作为参考模板在该插件的基础上构建你自己的插件。

https://github.com/hzqst/HUDColor


### MetaAudio (第三方) (只支持GoldSrc)

该插件使用alure2+OpenAL替换了GoldSrc原本的声音系统

由于SvEngine已经使用FMOD作为声音引擎了，你不应该在Sven Co-op上使用该插件

https://github.com/LAGonauta/MetaAudio

* 由于 MetaAudio 会拦截引擎中所有播放声音的接口。`MetaAudio.dll` 在 `plugins.lst` 中必须处于任何依赖于引擎中声音组件的插件之前 (例如：CaptionMod) ，你需要调整加载顺序以防止这些插件的功能被 MetaAudio 干扰。使用错误的加载顺序可能会导致这些插件无法正常工作。

* 具体解释：如果两个插件都对同一个函数（比如引擎中播放声音的api）挂了hook，那么后安装的hook会先于先安装的hook执行，而我们必须确保hook的调用链为`hw.dll`->`CaptionMod.dll`->`MetaAudio.dll`才能让CaptionMod根据声音播放字幕的功能不被MetaAudio拦截，也就是说`CaptionMod.dll`必须在`MetaAudio.dll`之后安装hook。

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

* 大多数用户应下载 `MetaHookSv-windows-x86.zip` .

* 对于使用低于4554版本GoldSrc引擎（如3248、3266、3647）的用户，请下载 `MetaHookSv-windows-x86-blob-support.zip`

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

## 一键安装方式 (推荐)

1. 从 [GitHub Release](https://github.com/hzqst/MetaHookSv/releases) 下载压缩包。(可利用GitHub的国内加速镜像加速下载），然后解压。

2. 运行 `scripts\install-to-SvenCoop.bat`

3. 从生成的快捷方式 `MetaHook for SvenCoop.lnk` 或 Steam游戏库中  或 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe` 启动Sven Co-op。(*Sven Co-op之外的其他游戏只能通过快捷方式方式启动)

* 其他游戏也可以按照此种方式安装，只需要运行其他install-to-批处理即可。

* 请确保已经登录Steam否则 [SteamAppsLocation](toolsrc/README.md) 可能会无法寻找游戏安装目录，导致自动安装失败。

* 对于Steam游戏库中不存在的游戏（如盗版CS）可以复制一份`scripts\install-to-CustomGame.bat`，将里面的游戏路径修改为你自己的游戏路径，并正确修改Mod目录、Mod名等信息。这样双击你自己修改的这份bat也可以实现自动安装。

## 手动安装方式 (不推荐)

1. 从 [GitHub Release](https://github.com/hzqst/MetaHookSv/releases) 下载压缩包。(可利用GitHub的国内加速镜像加速下载），然后解压。

2. 复制Build目录下的所有[你认为你需要安装的文件](Build/READMECN.md)到 `\SteamLibrary\steamapps\common\Sven Co-op\` 下。

3. 打开 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\` 目录, 将 `plugin_svencoop.lst` (或 `plugin_goldsrc.lst`，取决于你当前使用的游戏引擎是SvEngine还是GoldSrc) 重命名为 `plugins.lst`

4. 从将 `MetaHook.exe` 重命名为对应游戏的mod目录名，如`svencoop.exe`、`cstrike.exe`，并从该exe启动游戏。

* 对于低于4554版本的GoldSrc引擎，请使用 `MetaHook_blob.exe` 而非 `MetaHook.exe`。

* 你可以在 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\plugins.lst` 中根据自己需求启用/禁用插件。

* `Build`目录中的`SDL2.dll`文件是用来修复原版SDL使用中文输入法进行游戏时可能发生的内存越界写入导致游戏崩溃的问题。如果你全程都关闭中文输入法的话也可以选择不替换`SDL2.dll`。

* Valve在HL25周年补丁中更新了修复了缓冲区越界问题的SDL2，所以如果你是HL25周年正版就不需要替换SDL2.dll。

* Sven Co-op开发团队在Sven Co-op 5.26中更新了修复了缓冲区越界问题的SDL2，所以如果你是Sven Co-op 5.26或以上版本就不需要替换SDL2.dll。

## 构建需求

1. Visual Studio 2022，以及 VC143工具集。

2. CMake

3. Git 客户端

## 如何构建

假设你已经正确安装了所有构建需求。

1. 执行 `git clone --recursive https://github.com/hzqst/MetaHookSv` 拉取代码到一处**路径不包含空格**的目录中。

2. 运行 `scripts\build-MetaHook.bat`, 等待`MetaHook.exe`生成完成。如果没有错误发生，生成的EXE应该会出现在`Build`目录下。

3. 运行 `scripts\build-Plugins.bat`, 等待所有插件生成完成。如果没有错误发生，生成的DLL应该会出现在`Build\svencoop\metahook\plugins\`目录下。

## 如何调试

1. 执行 `git clone --recursive https://github.com/hzqst/MetaHookSv` 拉取代码到一处**路径不包含空格**的目录中。

3. 运行 `scripts\debug-SvenCoop.bat`  (其他游戏就选择该游戏对应的批处理)

4. 打开 `MetaHook.sln`, 在解决方案资源管理器中找到对应的项目，右键设置为启动项目，然后以**Debug|Win32配置**重新生成该项目后，按F5即可启动本地调试。

* 如果运行 `scripts\debug-SvenCoop.bat` 时 Visual Studio 正在运行，请重启一次 Visual Studio，否则可能会导致新的调试设置不生效。

* 请确保已经登录Steam否则 [SteamAppsLocation](toolsrc/README.md) 可能会无法寻找游戏安装目录。

* 对于Steam游戏库中不存在的游戏（如盗版CS）可以复制一份`scripts\debug-CustomGame.bat`，将里面的游戏路径修改为你自己的游戏路径，并正确修改Mod目录、Mod名等信息。这样双击你自己修改的这份bat也可以实现自动设置调试路径。

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

该插件捕获了`snapshots`截图命令，将其重定向到Steam客户端自带的截图功能上。

### SCModelDownloader (只支持Sven Co-op)

该插件自动从 https://wootguy.github.io/scmodels/ 下载缺失的玩家模型。

控制台参数 : `scmodel_autodownload 0 / 1` 设为1时启用自动下载

控制台参数 : `scmodel_downloadlatest 0 / 1` 设为1时自动下载最新版本的模型（如果有多个版本的模型）

### CommunicationDemo (只支持Sven Co-op)

该插件开放了一个接口用于进行客户端-服务端双向通信。

### DontFlushSoundCache (只支持Sven Co-op) (实验性)

该插件阻止客户端在 `retry` 时清理 soundcache 缓存 (引擎在HTTP完成下载时会触发 `retry` 命令), 让客户端得以保留HTTP下载得到的 soundcache txt

服务端必须上传自己的soundcache到资源服务器来支持soundcache的HTTP下载

该插件的目的是节约服务器的带宽资源和磁盘IO资源（频繁读写文件不是什么好事，用UDP挂服下载更不是什么好事）

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

https://github.com/DrAbcrealone/HUDColor

### MetaAudio (第三方) (只支持GoldSrc)

该插件使用alure2+OpenAL替换了GoldSrc原本的声音系统

由于SvEngine已经使用FMOD作为声音引擎了，你不应该在Sven Co-op上使用该插件

https://github.com/LAGonauta/MetaAudio

* 由于 MetaAudio 会拦截引擎中所有播放声音的接口。`MetaAudio.dll` 在 `plugins.lst` 中必须处于任何依赖于引擎中声音组件的插件之前 (例如：CaptionMod) ，你需要调整加载顺序以防止这些插件的功能被 MetaAudio 干扰。使用错误的加载顺序可能会导致这些插件无法正常工作。

* 具体解释：如果两个插件都对同一个函数（比如引擎中播放声音的api）挂了hook，那么后安装的hook会先于先安装的hook执行，而我们必须确保hook的调用链为`hw.dll`->`CaptionMod.dll`->`MetaAudio.dll`才能让CaptionMod根据声音播放字幕的功能不被MetaAudio拦截，也就是说`CaptionMod.dll`必须在`MetaAudio.dll`之后安装hook。

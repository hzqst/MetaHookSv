# MetaHookSv

[MetaHook](https://github.com/nagist/metahook)的Sven Co-op移植版本

用黑科技提升你的Sven Co-op游戏体验

大部分插件都兼容原版GoldSrc引擎，具体每个插件的兼容性情况请逐个查阅插件文档。

[英文README](README.md)

### 兼容性 (仅metahook加载器本体)

|        引擎                 |      |
|        ----                 | ---- |
| GoldSrc_blob   (3266~4554)  | √    |
| GoldSrc_legacy (< 6153)     | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

## 下载

[GitHub Release](https://github.com/hzqst/MetaHookSv/releases)

[因为某些神秘原因导致无法下载的话可以使用加速镜像，往里复制下载地址即可下载](https://ghproxy.com/)

## VAC风险?

虽然在游戏中使用hook之类的行为可能看上去很危险，但是目前为止还没有人反馈因为使用此插件导致VAC封禁。

并且Sven Co-op并不属于[受VAC保护的游戏](https://store.steampowered.com/search/?term=Sven&category2=8)

你甚至可以以添加命令行参数`-insecure`的方式加所谓“受VAC保护的服务器”，因为Sven Co-op上的VAC根本就没有工作。

如果你实在不放心，那么请使用小号进行游戏，毕竟Sven Co-op是免费游戏。

## 一键安装方式

1. 从 [GitHub Release](https://github.com/hzqst/MetaHookSv/releases) 下载压缩包。(可利用GitHub的国内加速镜像加速下载），然后解压。

2. 运行 `scripts\install-to-SvenCoop.bat`

3. 从 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe` 启动游戏

* 其他游戏也可以按照此种方式安装，只需要运行其他install-to-批处理即可。

* 请确保已经登录Steam否则 [SteamAppsLocation](toolsrc/README.md) 可能会无法寻找游戏安装目录，导致自动安装失败。

## 手动安装方式

1. 从 [GitHub Release](https://github.com/hzqst/MetaHookSv/releases) 下载压缩包。(可利用GitHub的国内加速镜像加速下载），然后解压。

2. 复制Build目录下的所有[你认为你需要安装的文件](Build/READMECN.md)到 `\SteamLibrary\steamapps\common\Sven Co-op\` 下。

3. 打开 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop\metahook\configs\` 目录, 将 `plugin_svencoop.lst` (或 `plugin_goldsrc.lst`，取决于你当前使用的游戏引擎是SvEngine还是GoldSrc) 重命名为 `plugins.lst`

4. 从 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe` 启动游戏。

* 如果要运行Sven Co-op以外的游戏，请自行使用`-game`启动项参数的方式启动，如：`svencoop.exe -game valve`或`svencoop.exe -game cstrike`。或者将 `svencoop.exe` 重命名为对应游戏的mod目录名，如`cstrike.exe`

* `Build`目录中的 `svencoop.exe` 原来叫 `metahook.exe`，它会替换你自带的游戏启动器`svencoop.exe`，请注意备份。当然你也可以选择不替换`svencoop.exe`，而是手动安装并以命令行或启动项`metahook.exe -game svencoop`的方式启动游戏。不过不推荐这么做，因为这么做会导致更改视频模式的时候游戏闪退（可能是游戏自己对进程名有校验）。

* `Build`目录中的`SDL2.dll`文件是用来修复原版SDL使用中文输入法进行游戏时可能发生的内存越界写入导致游戏崩溃的问题。如果你全程都关闭中文输入法的话也可以选择不替换`SDL2.dll`。

## 构建需求

1. Visual Studio 2017, 2019 或 2022，以及 VC141，VC142 或 VC143工具集。

2. CMake

3. Git 客户端

## 如何构建

假设你已经正确安装了所有构建需求。

1. 执行 `git clone --recursive https://github.com/hzqst/MetaHookSv` 拉取代码到一处**路径不包含空格**的目录中。

2. 运行 `scripts\build-MetaHook.bat`, 等待metahook exe生成完成。如果没有错误发生，生成的EXE应该会出现在`Build`目录下。

3. 运行 `scripts\build-Plugins.bat`, 等待所有插件生成完成。如果没有错误发生，生成的DLL应该会出现在`Build\svencoop\metahook\plugins\`目录下。

## 如何调试

1. 执行 `git clone --recursive https://github.com/hzqst/MetaHookSv` 拉取代码到一处**路径不包含空格**的目录中。

3. 运行 `scripts\debug-SvenCoop.bat`  (其他游戏就选择该游戏对应的批处理)

4. 打开 `MetaHook.sln`, 在解决方案资源管理器中找到对应的项目，右键设置为启动项目，然后以**Debug|Win32配置**重新生成该项目后，按F5即可启动本地调试。

* 如果运行 `scripts\debug-SvenCoop.bat` 时 Visual Studio 正在运行，请重启一次 Visual Studio，否则可能会导致新的调试设置不生效。

* 请确保已经登录Steam否则 [SteamAppsLocation](toolsrc/README.md) 可能会无法寻找游戏安装目录。

## MetaHookSv (V3) 相比 MetaHook (V2) 的新功能

1. 提供反汇编 API 用于分析引擎代码，提供反向（往前）搜索函数头部的API。提供更多好用的API。

2. 防止插件重复加载（重复加载会导致插件自调用，引发无限递归）

3. `LoadEngine` 和 `LoadClient` 阶段会对所有`InlineHook`请求开启“事务”，直到所有插件的`LoadEngine` 和 `LoadClient`结束才会让`InlineHook`生效, 这样就可以允许不同插件`SearchPattern` 和 `InlineHook` 同一个函数，也不会引发冲突了

4. 新增启动项参数 `-metahook_legacy_v2_api` ，该启动项将提供以前V2版本的错误行为API给使用V2版本接口的插件。

* V2版本的 `g_pMetaHookAPI->GetEngineBase()` 对BLOB加密版本的引擎(如3266)会错误返回 0x1D01000 而非 0x1D00000，然而实际上 0x1D01000 是代码段起始地址而非引擎基址。因此某些依赖V2版本API的插件会依赖错误的引擎基址而硬编码一个错误的偏移（这些插件使用正确偏移-0x1000来达到抵消V2API错误行为的目的，但是这种抵消手法如果遇上返回正确结果的API就会导致算出的最终偏移比正常的大0x1000），该启动项专门用于解决这种情况，正常情况下不需要使用。

5. 新增启动项参数 `-metahook_check_vfthook` ，该启动项将屏蔽任何非法的 `g_pMetaHookAPI->MH_VFTHook` 调用。

* 有些来自插件作者检查不严格而产生的MH_VFTHook调用会对某些超出真实虚表范围的地址进行hook，这可能导致游戏随机崩溃等问题。该启动项专门用于解决这种情况，正常情况下不需要使用。

## 加载顺序

1. MetaHook启动器总是会以从上到下的顺序加载 `\(ModDirectory)\metahook\configs\plugins.lst` 中列出的插件。当插件名前面存在引号";"时该行会被忽略。

2. 当支持AVX2指令集时自动加载(插件名)_AVX2.dll

3. 当支持AVX指令集时且(2)失败时自动加载(PluginName)_AVX.dll

4. 当支持SSE2指令集时且(3)失败时自动加载(PluginName)_SSE2.dll

5. 当支持SSE指令集时且(4)失败时自动加载(PluginName)_SSE.dll

6. 当(2) (3) (4) (5)均失败时自动加载(PluginName).dll

## 插件列表

### CaptionMod

这是一个使用VGUI2来显示字幕、翻译英文HUD消息和VGUI文本的插件。

对Sven Co-op而言，该插件修复了游戏中大多数无法显示中文的乱码问题。

[中文文档](docs/CaptionModCN.md) [英文文档](docs/CaptionMod.md)

![](/img/1.png)

### BulletPhysics

对游戏提供布娃娃支持。玩家死亡时以及玩家被藤壶、喷火怪抓住时将玩家模型转化为布娃娃。

[中文文档](docs/BulletPhysicsCN.md) [DOCUMENTATION](docs/BulletPhysics.md)

![](/img/6.png)

### MetaRenderer

替换了原版的图形渲染引擎，极大提升了渲染性能，使用了黑科技提升你的画质和帧率。

[中文文档](docs/RendererCN.md) [英文文档](docs/Renderer.md)

![](/img/3.png)

### StudioEvents

该插件可以防止重复播放模型自带音效，防止音效反复刷屏。

[中文文档](docs/StudioEvents.md) [中文文档](docs/StudioEventsCN.md)

![](/img/8.png)

### SteamScreenshots (只支持Sven Co-op)

该插件捕获了`snapshots`截图命令，将其重定向到Steam客户端自带的截图功能上。

### SCModelDownloader (只支持Sven Co-op)

该插件自动从 https://wootguy.github.io/scmodels/ 下载缺失的玩家模型。

控制台参数 : `scmodel_autodownload 0 / 1` 设为1时启用自动下载

控制台参数 : `scmodel_downloadlatest 0 / 1` 设为1时自动下载最新版本的模型（如果有多个版本的模型）

控制台参数 : `scmodel_usemirror 0 / 1 / 2` 使用CDN镜像加速下载，1 = `cdn.jsdelivr.net`， 2 = `gh.api.99988866.xyz`

### CommunicationDemo (只支持Sven Co-op)

该插件开放了一个接口用于进行客户端-服务端双向通信。

### DontFlushSoundCache (只支持Sven Co-op) (实验性)

该插件阻止客户端在 `retry` 时清理 soundcache 缓存 (引擎在HTTP完成下载时会触发 `retry` 命令), 让客户端得以保留HTTP下载得到的 soundcache txt

服务端必须上传自己的soundcache到资源服务器来支持soundcache的HTTP下载

该插件的目的是节约服务器的带宽资源和磁盘IO资源（频繁读写文件不是什么好事，用UDP挂服下载更不是什么好事）

### PrecacheManager

该插件提供了一个命令 `fs_dump_precaches` 用于dump预缓存的游戏资源列表到 `[ModDirectory]\maps\[mapname].dump.res` 文件中。

* Sven Co-op 的声音系统使用 `soundcache.txt` 而非引擎的预缓存系统来维护声音文件的预缓存列表。

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

### ModelViewer (第三方) (只支持Sven Co-op)

https://github.com/surf082/ModelViewer

该插件在多人游戏选项页面添加了一个预览模型的按钮。

### ServerFilter (第三方) (只支持GoldSrc)

https://github.com/surf082/ServerFilter

该插件为GoldSrc游戏提供服务器过滤功能。
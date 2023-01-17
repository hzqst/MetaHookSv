# MetaHookSv

[MetaHook](https://github.com/nagist/metahook)的Sven Co-op移植版本

用黑科技提升你的Sven Co-op游戏体验

大部分插件都兼容原版GoldSrc引擎，具体的兼容性情况请逐个查阅插件文档。

[英文README](README.md)

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

2. 运行 `install-to-SvenCoop.bat`

3. 从 `\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe` 启动游戏

* 其他游戏也可以按照此种方式安装，只需要运行其他install-to-批处理即可。

* 请确保已经登录Steam否则 [SteamAppsLocation](SteamAppsLocation/README.md) 可能会无法寻找游戏安装目录，导致自动安装失败。

## 手动安装方式

1. 从 [GitHub Release](https://github.com/hzqst/MetaHookSv/releases) 下载压缩包。(可利用GitHub的国内加速镜像加速下载），然后解压。

2. 复制Build目录下的所有文件到 `\SteamLibrary\steamapps\common\Sven Co-op\`。

3. 从`\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe` 启动游戏。

* `Build`目录中的 `svencoop.exe` 原来叫 `metahook.exe`，它会替换你自带的游戏启动器`svencoop.exe`，请注意备份。当然你也可以选择不替换`svencoop.exe`，以命令行或启动项方式`metahook.exe -game svencoop`启动SvenCoop。不过不推荐这么做，因为这么做会导致更改视频模式的时候游戏闪退（可能是游戏自己对进程名有校验）。

* `Build`目录中的`SDL2.dll`文件是用来修复原版SDL使用中文输入法进行游戏时可能发生的内存越界写入导致游戏崩溃的问题。如果你全程都关闭中文输入法的话也可以选择不替换`SDL2.dll`。当然，你也可以从Steam根目录（默认位置`C:\Program Files (x86)\Steam`）复制`SDL2.dll`，效果是完全一样的。

## 构建需求

1. Visual Studio 2017 或 2019，以及VC141 或 VC142工具集。

2. CMake

3. git 客户端

## 如何构建

假设你已经正确安装了所有构建需求。

1. 执行 `git clone https://github.com/hzqst/MetaHookSv` 拉取代码到一处**不包含空格**的目录中。

2. 运行 `build-initdeps.bat`, 等待所有子模块和依赖项目下载完成。 (这一步可能需要花费几分钟时间, 具体取决于你的网速，如果速度很慢或者下载出错建议尝试使用魔法上网)

3. 运行 `build-MetaHook.bat`, 等待 `svencoop.exe` 生成到 `Build` 目录。

4. 运行 `build-(指定插件名).bat`, 等待 `(指定插件名).dll` 生成。目前可用的插件有：CaptionMod, Renderer, StudioEvents, SteamScreenshots, SCModelDownloader, CommunicationDemo, DontFlushSoundCache。

5. 如果构建成功，插件应该会生成到`Build\svencoop\metahook\plugins\`目录。

* 如果你的网络经常无法访问github导致clone失败，你可以从国内镜像gitee分别拉取[MetaHookSv](https://gitee.com/hzqst/MetaHookSv)、[Detours](https://gitee.com/mirrors/detours)、[Capstone](https://gitee.com/mirrors/capstone)、[Bullet3 Physics SDK](https://gitee.com/mirrors/bullet3)

* 上述国内镜像可能不是最新，如果需要最新版本可以自行创建gitee仓库并设置源镜像为对应的github仓库，然后从github同步到gitee，或使用魔法上网拉取代码。

## 如何调试

1. `git clone https://github.com/hzqst/MetaHookSv` (如果你之前已经执行过这一步的指令，则可以略过这一步)

2. 运行 `build-initdeps.bat`, 等待所有子模块和依赖项目下载完成。 (如果你之前已经执行过这一步的指令，则可以略过这一步) (这一步可能需要花费几分钟时间, 具体取决于你的网速，如果速度很慢或者下载出错建议尝试使用魔法上网)

3. 运行 `debug-SvenCoop.bat`  (其他游戏就选择该游戏对应的debug批处理)

4. 打开 `MetaHook.sln`, 在解决方案资源管理器中找到对应的项目，右键设置为启动项目，然后以Debug模式重新编译该项目后，按F5即可开启本地调试。

* 如果运行 `debug-SvenCoop.bat` 时 Visual Studio 正在运行，请重启一次 Visual Studio，否则可能会导致新的调试设置不生效。

* 请确保已经登录Steam否则 [SteamAppsLocation](SteamAppsLocation/README.md) 可能会无法寻找游戏安装目录。

## MetaHookSv (V3) 相比 MetaHook (V2) 的新功能

1. 提供反汇编 API 用于分析引擎代码，提供反向（往前）搜索函数头部的API。提供更多好用的API。

2. 防止插件重复加载（重复加载会导致插件自调用，引发无限递归）

3. `LoadEngine` 和 `LoadClient` 阶段会对所有`InlineHook`请求开启“事务”，直到所有插件的`LoadEngine` 和 `LoadClient`结束才会让`InlineHook`生效, 这样就可以允许不同插件`SearchPattern` 和 `InlineHook` 同一个函数，也不会引发冲突了

## 加载顺序

1. `\(GameDirectory)\metahook\configs\plugins_avx2.lst` (仅当CPU支持AVX2指令集时)
2. `\(GameDirectory)\metahook\configs\plugins_svencoop_avx2.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_avx2.lst` (仅当CPU支持AVX2指令集时)

3. `\(GameDirectory)\metahook\configs\plugins_avx.lst` (仅当CPU支持AVX指令集时)
4. `\(GameDirectory)\metahook\configs\plugins_svencoop_avx.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_avx.lst` (仅当CPU支持AVX指令集时)

5. `\(GameDirectory)\metahook\configs\plugins_sse2.lst` (仅当CPU支持SSE2指令集时)
6. `\(GameDirectory)\metahook\configs\plugins_svencoop_sse2.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_sse2.lst` (仅当CPU支持SSE2指令集时)

7. `\(GameDirectory)\metahook\configs\plugins_sse.lst` (仅当CPU支持SSE指令集时)
8. `\(GameDirectory)\metahook\configs\plugins_svencoop_sse.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc_sse.lst` (仅当CPU支持SSE指令集时)

9. `\(GameDirectory)\metahook\configs\plugins.lst`
10. `\(GameDirectory)\metahook\configs\plugins_svencoop.lst` or `\(GameDirectory)\metahook\configs\plugins_goldsrc.lst`

* 加载器会以上述顺序寻找第一个可用的插件列表文件(.lst), 并且从该列表中从上往下依次加载dll.

## 插件列表

### CaptionMod

这是一个使用VGUI2来显示字幕、翻译英文HUD消息和VGUI文本的插件。

对Sven Co-op而言，该插件修复了游戏中大多数无法显示中文的乱码问题。

[中文文档](CaptionModCN.md) [英文文档](CaptionMod.md)

![](/img/1.png)

### BulletPhysics

对游戏提供布娃娃支持。玩家死亡时以及玩家被藤壶、喷火怪抓住时将玩家模型转化为布娃娃。

[中文文档](BulletPhysicsCN.md) [DOCUMENTATION](BulletPhysics.md)

![](/img/6.png)

### MetaRenderer

替换了原版的图形渲染引擎，极大提升了渲染性能，使用了黑科技提升你的画质和帧率。

[中文文档](RendererCN.md) [英文文档](Renderer.md)

![](/img/3.png)

### StudioEvents

该插件可以防止重复播放模型自带音效，防止音效反复刷屏。

[DOCUMENTATION](StudioEvents.md) [中文文档](StudioEventsCN.md)

![](/img/8.png)

### SteamScreenshots (只支持Sven Co-op)

该插件捕获了`snapshots`截图命令，将其重定向到Steam客户端自带的截图功能上。

### SCModelDownloader (只支持Sven Co-op)

该插件自动从 https://wootguy.github.io/scmodels/ 下载缺失的玩家模型。

控制台参数 : `scmodel_autodownload 0 / 1` 设为1时启用自动下载

控制台参数 : `scmodel_downloadlatest 0 / 1` 设为1时自动下载最新版本的模型（如果有多个版本的模型）

控制台参数 : `scmodel_usemirror 0 / 1 / 2` 使用CDN镜像加速下载，1 = `cdn.jsdelivr.net`， 2 = `https://gh.api.99988866.xyz`

### CommunicationDemo (只支持Sven Co-op)

该插件开放了一个接口用于进行客户端-服务端双向通信。

### DontFlushSoundCache (只支持Sven Co-op) (实验性)

该插件阻止客户端在 `retry` 时清理 soundcache 缓存 (引擎在HTTP完成下载时会触发 `retry` 命令), 让客户端得以保留HTTP下载得到的 soundcache txt

服务端必须上传自己的soundcache到资源服务器来支持soundcache的HTTP下载

该插件的目的是节约服务器的带宽资源和磁盘IO资源（频繁读写文件不是什么好事，用UDP挂服下载更不是什么好事）

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

* 由于 MetaAudio 会拦截引擎中所有播放声音的接口。`MetaAudio.dll` 在 `plugins.lst` 或 `plugins_[blablabla].lst` 中必须处于任何依赖于引擎中声音组件的插件之前 (例如：CaptionMod) ，你需要调整加载顺序以防止这些插件被 MetaAudio 拦截。使用错误的加载顺序可能会导致这些插件无法正常工作。
# MetaHookSv

MetaHook的SvenCoop移植版本 (https://github.com/nagist/metahook)

用黑科技提升你的SvenCoop游戏体验

大部分插件都兼容原版GoldSrc引擎，具体的兼容性情况请逐个查阅插件文档。

[英文README](README.md)

## VAC风险?

虽然在游戏中使用hook之类的行为可能看上去很危险，但是目前为止还没有人反馈因为使用此插件导致VAC封禁。

并且SvenCoop并不属于受VAC保护的游戏：https://store.steampowered.com/search/?term=coop&category2=8

如果你实在不放心，那么请使用小号进行游戏，毕竟Sven-Coop是免费游戏。

## 手动安装方式

1. git pull https://github.com/hzqst/MetaHookSv （如果你有安装git的话） 或者 直接从 https://github.com/hzqst/MetaHookSv/archive/main.zip 下载压缩包。如果因为国内网络问题导致速度太慢或无法访问，也可以直接从国内镜像 https://gitee.com/hzqst/MetaHookSv 和 https://gitee.com/hzqst/MetaHookSv/repository/archive/main.zip 下载（gitee上可能不是最新版本，且从gitee下载压缩包可能需要注册账号，不过也可以使用QQ直接登录）

2. 复制Build目录下的所有文件到 "\你的Steam游戏库目录\steamapps\common\Sven Co-op\".

3. 从"\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe" 启动游戏。

* Build中的 "svencoop.exe" 原来叫 "metahook.exe"，它会替换你自带的游戏启动器svencoop.exe，请注意备份。当然你也可以选择不替换“svencoop.exe”，以命令行或启动项方式"metahook.exe -game svencoop"启动SvenCoop。不过不推荐这么做，因为这么做会导致更改视频模式的时候游戏闪退（可能是游戏自己对进程名有校验）。

* Build中的“SDL2.dll”文件是用来修复原版SDL使用中文输入法进行游戏时可能发生的内存越界写入导致游戏崩溃的问题。如果你全程都关闭中文输入法的话也可以选择不替换“SDL2.dll”。

## 一键安装方式

1. git pull https://github.com/hzqst/MetaHookSv （如果你有安装git的话） 或者 直接从 https://github.com/hzqst/MetaHookSv/archive/main.zip 下载压缩包。

2. 运行 "install-to-SvenCoop.bat"

3. 从 "\SteamLibrary\steamapps\common\Sven Co-op\svencoop.exe" 启动游戏

4. 其他游戏也可以按照此种方式安装，只需要运行其他install-to-批处理即可。

## 构建需求

1. Visual Studio 2017 或 2019，以及VC141 或 VC142工具集。

2. CMake

3. git 客户端

## 构建方法

假设你已经正确安装了所有构建需求。

1. git clone https://github.com/hzqst/MetaHookSv

2. 运行 init-deps.bat, 等待所有子模块和依赖项目下载完成。 (这一步可能需要花费几分钟时间, 具体取决于你的网速，如果速度很慢或者下载出错建议尝试使用proxy)

3. 运行 "build-capstone.bat", 等待 capstone 的静态库生成完毕

4. 运行 "build-bullet3.bat", 等待 bullet3 的静态库生成完毕

5. 运行 "build-MetaHook.bat", 等待 "svencoop.exe" 生成到 "Build" 目录

6. 运行 "build-CaptionMod.bat", 等待 "CaptionMod.dll" 生成

7. 运行 "build-Renderer.bat", 等待 "Renderer.dll" 生成

8. 运行 "build-BulletPhysics.bat", 等待 "BulletPhysics.dll" 生成

9. 运行 "build-StudioEvents.bat", 等待 "StudioEvents.dll" 生成

10. 所有插件应该会生成到"Build\svencoop\metahook\plugins\"目录

## 插件

### CaptionMod

这是一个使用VGUI2来显示字幕、翻译英文HUD消息和VGUI文本的插件

[中文文档](CaptionModCN.md) [英文文档](CaptionMod.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

### BulletPhysics

对游戏提供布娃娃支持，玩家死亡时以及玩家被藤壶抓住时将玩家模型转化为布娃娃

[中文文档](BulletPhysicsCN.md) [DOCUMENTATION](BulletPhysics.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/6.png)

### MetaRenderer

替换了原版的图形渲染引擎，极大提升了渲染性能，使用了黑科技提升你的游戏体验。

[中文文档](RendererCN.md) [英文文档](Renderer.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/3.png)

### StudioEvents

该插件可以防止重复播放模型自带音效，防止音效反复刷屏

[DOCUMENTATION](StudioEvents.md) [中文文档](StudioEventsCN.md)

![](https://github.com/hzqst/MetaHookSv/raw/main/img/8.png)

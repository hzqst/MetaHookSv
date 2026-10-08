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

[GitHub Release](https://github.com/MetaHookSv/MetaHookSv/releases)

* 因某些国内线路问题无法下载或下载过慢的话可以百度搜索GitHub加速镜像.随便找一个国内能直接访问的加速镜像站，往里复制从Release页面上复制的下载地址即可加速下载。

* 下载 `MetaHookSv-windows-x86.7z`，统一包包含普通和 blob 启动器。

## VAC风险?

虽然在游戏中使用hook之类的行为可能看上去很危险，但是目前为止还没有人反馈因为使用此插件导致VAC封禁。

并且Sven Co-op并不属于[受VAC保护的游戏](https://store.steampowered.com/search/?term=Sven&category2=8)

你甚至可以以添加命令行参数`-insecure`的方式加所谓“受VAC保护的服务器”，因为Sven Co-op上的VAC根本就没有工作。

如果你实在不放心，那么请使用小号进行游戏，毕竟Sven Co-op是免费游戏。

## 常见问题

Q. 为什么使用盗版/旧版引擎进行游戏时，游戏进程会周期性卡住几秒？

A: 因为V社在引擎的主循环中使用了一个阻塞式API `gethostbyname` 来请求域名。该API在请求已失效的域名的时候就是会阻塞当前进程直到超时返回的，这是Windows的设定。

你可以通过在启动项中添加 `-nomaster` 或 `-steam` 来缓解该问题。（ `-steam` 在某些NoSteam盗版版本上可能导致游戏无法启动）

Q. 为什么游戏进程会在退出/重启时卡住很久 ?

A: 因为 ThreadGuard.dll 会在游戏退出时强制等待 V社创建的网络线程退出，以防游戏意外崩溃。具体见 [ThreadGuard](https://github.com/MetaHookSv/MetaHookSv#threadguard)。

Q. 为什么我进到主菜单界面之后就会黑屏？

A. 因为 [SDL3-over-SDL2 兼容层](https://github.com/libsdl-org/sdl2-compat) 在软件渲染模式下无法正常工作。请切换至 OpenGL 模式以解决问题（在游戏启动项中添加 `-gl` 即可）。

Q. 如果游戏占用内存过多(超过2.7GB)导致崩溃怎么办？

A. 尝试添加以下启动项：`-metahook_early_unload_mirrored_dll` （该选项可节约大约120MB内存）

A. 尝试启用以下控制台参数：`r_studio_lazy_load 1`， `r_leaf_lazy_load 1`

Q. 为什么在 `sv_cheats 1` 之后，地形变全黑了 ?

A. 请设置 `r_lightmap 1`

## 一键安装方式

1. 下载 release 并完整解压。
2. 保持 `MetahookInstaller.exe` 与 `install/output/` 在一起。运行安装器，选择游戏后点击 **安装**。
3. Sven Co-op 从 Steam 或生成的快捷方式启动；其他游戏使用生成的 `MetaHook for [GameName].lnk`。

如需脚本化安装，可使用同一压缩包中的 `MetahookInstallerCLI.exe`，执行的步骤与 GUI 相同：

```powershell
.\MetahookInstallerCLI.exe -appid 225840                                         # 安装到 Steam 版 Sven Co-op
.\MetahookInstallerCLI.exe -appid 70 -gamedir "D:\SteamLibrary\steamapps\common\Half-Life" -moddir gearbox  # 安装到 Steam 版 Half-Life : Opposing Force
.\MetahookInstallerCLI.exe -appid 225840 -uninstall
```

完整参数见 [MetahookInstaller](https://github.com/MetaHookSv/MetahookInstaller).

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

```bash
git clone --recursive https://github.com/MetaHookSv/MetaHookSv
cd MetaHookSv
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 "-DCMAKE_INSTALL_PREFIX=$PWD/install/output"
cmake --build build --config Release --parallel 2
cmake --install build --config Release
```

聚合管线构建 MetaHook、MetaHook_blob、所有启用的插件（包括 BetterSpray）、共享插件库和 CMake 工具。插件列表模板由 `assets/svencoop/metahook/configs/` 管理并安装到输出目录。.NET 工具单独构建，详见[安装器文档](https://github.com/MetaHookSv/MetahookInstaller)。

`windows.yml` 构建、验证并打包安装器 EXE 和完整的 `install/output/`。标签发布复用统一 Windows artifact 与独立 BSP 工具 artifact，并继续生成中英文 AI release notes。

## 如何调试

启用可选的 Visual Studio 启动项目（从源码构建 InstallerCLI 时需要兼容的 .NET SDK 和 .NET 8 runtime）：

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON -DMETAHOOKSV_GAME_APPID=225840
# 自定义安装路径或 Mod 可另外传入：
# "-DMETAHOOKSV_GAME_DIRECTORY=D:/Games/Half-Life" -DMETAHOOKSV_GAME_APPID=70 -DMETAHOOKSV_GAME_MOD=gearbox
```

打开 `build/MetaHookSv.sln`，选择 Debug/Win32，将 **Launch-debugging / LaunchGame** 设为启动项目，然后按 **F5**。该流程会增量编译所有启用组件和 InstallerCLI，Install 到私有暂存目录，通过 InstallerCLI 部署，再由原生 C++ 调试器启动实际游戏启动器。dummy 程序不会被启动或安装。也支持 Release/Win32。

`METAHOOKSV_GAME_DIRECTORY` 默认留空，由 InstallerCLI 按 AppID 查找 Steam 游戏目录；显式填写时优先使用填写值。`METAHOOKSV_GAME_MOD` 默认使用该 AppID 的基础 Mod。`METAHOOKSV_GAME_ARGUMENTS` 追加到 `-insecure -game <mod>` 之后。配置阶段会构建 CLI 并只读查询游戏，不执行部署；游戏缺失或无效会使配置失败。更换游戏、引擎或安装位置后需重新配置 CMake。

每次构建启动项目均会部署，包括未修改源码时。私有 payload `build/launch-game/<Debug|Release>/install/output` 会重新生成，不改变普通 Install 的输出路径。聚合构建中的 Debug 插件和共享插件库统一使用正常模块名，与游戏插件列表及动态加载名称一致。InstallerCLI 保留已有插件选择，映射资源并选择 `svencoop.exe`、`MetaHook.exe` 或 `MetaHook_blob.exe`；根目录 PDB 也会按原文件名部署。游戏文件按既有 Installer 规则覆盖。停用组件后，它会从暂存 payload 消失，但不会自动卸载游戏目录中先前部署的文件。

在 **工具 / 选项 / 项目和解决方案 / 生成并运行** 中，启用运行前构建过期项目，将构建或部署出错时的行为设为 **不启动**。重新部署前请退出游戏；文件占用会导致构建失败，不会自动结束进程。断点未绑定时可在调试器的“模块”窗口检查符号加载。此功能默认关闭，不影响普通构建或 CI。

### 独立插件调试

`Plugins/` 下的全部插件在独立配置时支持相同选项，单独 clone 插件仓库也可使用：

```bash
cmake -S Plugins/HeapPatch -B build/heappatch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
cmake --build build/heappatch --config Debug --target LaunchGame
```

请先安装 MetaHook，并在游戏的 `plugins.lst` 中启用该插件。独立构建的 **DeployGame** 编译当前插件及依赖，暂存 Install 后只更新插件文件和资源，包括插件 PDB 及 mod 内的依赖库；不更新根目录 launcher/运行库，不创建快捷方式、不修改插件列表，也不删除已安装的旧文件。F5 启动已有 launcher，缺少既有安装会使配置失败。聚合构建的 **DeployGame** 仍部署全部启用组件。

插件共享本仓库的 CMake 模块：优先使用 `METAHOOKSV_LAUNCH_GAME_MODULE_DIR` 指定的目录，其次使用所在聚合仓库；独立 clone 时将固定提交的源码包下载到构建目录，不拉取子模块、不配置聚合工程。关闭 LaunchGame 时不下载模块或 CLI。

可通过 `METAHOOKSV_INSTALLER_CLI_EXECUTABLE` 指定自包含 CLI，以便离线使用。否则存在 `toolsrc/MetahookInstaller/src` 时从源码构建；缺失时从 Installer Release 下载 `MetahookInstaller-windows-x64.7z`，版本由 `METAHOOKSV_INSTALLER_RELEASE` 选择（默认 `latest`，也可填写固定 tag）。下载的 CLI 无需安装 .NET。插件部署要求支持 `-plugins-only` 的版本（v20261004c 或之后的版本）。

下载使用发布资产提供的 SHA-256 校验，将 EXE 和实际 tag 缓存到 `build/launch-game/installer/<release>`。有效缓存可离线复用，不自动查询升级；切换 tag 或仅清理该私有缓存目录后重新下载。缓存损坏或 CLI 过旧时会明确报错。GitHub API 限流时可通过环境变量 `GH_TOKEN` 或 `GITHUB_TOKEN` 提供凭据，凭据不会写入缓存。每个配置在 `build/launch-game/<config>` 下使用各自 CLI 和全新 payload。VS 必须开启运行前构建，并将构建失败策略设为 **不启动**。

## MetaHook

核心启动器，用于启动游戏并加载插件

[链接](https://github.com/MetaHookSv/MetaHook)

## 插件列表

### VGUI2Extension

VGUI2Extension 是一个 VGUI2 的 modding 框架，为其他插件提供对 VGUI2 组件安装 hook / patch 的能力。

[链接](https://github.com/MetaHookSv/VGUI2Extension)

### CaptionMod

这是一个使用VGUI2来显示字幕、翻译英文HUD消息和VGUI文本的插件，除此之外还为游戏添加了起源风格的聊天框以及高DPI支持。

对Sven Co-op而言，该插件修复了游戏中的汉字无法显示或者乱码的问题。

[链接](https://github.com/MetaHookSv/CaptionMod)

### BulletPhysics

对游戏提供布娃娃支持。玩家死亡时以及玩家被藤壶、喷火怪抓住时将玩家模型转化为布娃娃。

[链接](https://github.com/MetaHookSv/BulletPhysics)

### Renderer

替换了原版的图形渲染引擎，极大提升了渲染性能，使用了黑科技提升你的画质和帧率。

[链接](https://github.com/MetaHookSv/Renderer)

### StudioEvents

该插件可以防止重复播放模型自带音效，防止音效反复刷屏。

[链接](https://github.com/MetaHookSv/StudioEvents)

### SteamScreenshots

该插件捕获了`snapshot`截图命令，将其重定向到Steam客户端自带的截图功能上。

[链接](https://github.com/MetaHookSv/SteamScreenshots)

### SCModelDownloader (只支持Sven Co-op)

该插件自动从 https://wootguy.github.io/scmodels/ 下载缺失的玩家模型。

控制台参数 : `scmodel_autodownload 0 / 1` 设为1时启用自动下载

控制台参数 : `scmodel_downloadlatest 0 / 1` 设为1时自动下载最新版本的模型（如果有多个版本的模型）

[链接](https://github.com/MetaHookSv/SCModelDownloader)

### PrecacheManager

该插件提供了一个命令 `fs_dump_precaches` 用于dump预缓存的游戏资源列表到 `[ModDirectory]\maps\[mapname].dump.res` 文件中。

* Sven Co-op 的声音系统使用 `soundcache.txt` 而非引擎的预缓存系统来维护声音文件的预缓存列表。

[链接](https://github.com/MetaHookSv/PrecacheManager)

### ThreadGuard

该插件接管了Valve的一些模块的线程创建行为，这些模块创建线程后在模块释放时不会等待线程结束，这可能会导致游戏退出或热重启时游戏进程随机崩溃。

[链接](https://github.com/MetaHookSv/ThreadGuard)

### ResourceReplacer

该插件可以动态替换游戏内资源 (主要是模型和声音文件) 且无需修改磁盘上的文件，就像 Sven Co-op 的 [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) 和 [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) 的资源替换功能一样。

[链接](https://github.com/MetaHookSv/ResourceReplacer)

### InterpFix

该插件修复高频快照耗尽 `ex_interp` 所需位置历史时，实体及其关联效果消失的问题。请求的插值时间早于有效历史时，使用最旧有效姿态保持实体可渲染。

[链接](https://github.com/MetaHookSv/InterpFix)

### SCCameraFix (只支持Sven Co-op)

该插件修复了Sven Co-op的观察者模式下摄像机视角/画面高频抖动的问题。

部分代码来自[halflife-updated](https://github.com/SamVanheer/halflife-updated)

[链接](https://github.com/MetaHookSv/SCCameraFix)

### BetterSpray

为 Sven Co-op 和 GoldSrc 的喷漆系统提供高分辨率图像、动态重载和云分享支持。

[链接](https://github.com/MetaHookSv/BetterSpray)

### HUDColor

该插件可以修改游戏中HUD的颜色。

也可以作为参考模板在该插件的基础上构建你自己的插件。

[链接](https://github.com/MetaHookSv/HUDColor)

### ABCEnchance (第三方) (只支持Sven Co-op)

该插件提供以下功能：

1. CSGO 风格的血量和弹药 HUD
2. 2077风格的转盘武器选择菜单
3. 伤害来源指示器
4. 动态准星
5. 实时更新的小地图（略微消耗渲染性能）
6. 漂浮文字显示队友的血量、护甲、名字.
7. 其他一些没什么用的特效

[链接](https://github.com/DrAbcrealone/ABCEnchance)

### halflife-cli (第三方)

halflife-cli 将 Half-Life / Sven Co-op 变成一个可由 CLI 驱动的程序，用于自动化测试或 agent 操作：它隐藏游戏窗口但保持进程存活（以便截图），通过 stdin 接收控制台命令，并在本机随机端口上提供 Source RCON 服务以便发现与远程控制。随包提供的 Python MCP 服务器将游戏暴露为可调用的工具（`launch_game`、`run_command`、`send_key`、`snapshot` 等）。

[链接](https://github.com/DrAbcOfficial/halflife-cli)

### MetaAudio (第三方) (只支持GoldSrc)

该插件使用alure2+OpenAL替换了GoldSrc原本的声音系统

由于SvEngine已经使用FMOD作为声音引擎了，你不应该在Sven Co-op上使用该插件

[链接](https://github.com/LAGonauta/MetaAudio)

* 由于 MetaAudio 会拦截引擎中所有播放声音的接口。`MetaAudio.dll` 在 `plugins.lst` 中必须处于任何依赖于引擎中声音组件的插件之前 (例如：CaptionMod) ，你需要调整加载顺序以防止这些插件的功能被 MetaAudio 干扰。使用错误的加载顺序可能会导致这些插件无法正常工作。

* 具体解释：如果两个插件都对同一个函数（比如引擎中播放声音的api）挂了hook，那么后安装的hook会先于先安装的hook执行，而我们必须确保hook的调用链为`hw.dll`->`CaptionMod.dll`->`MetaAudio.dll`才能让CaptionMod根据声音播放字幕的功能不被MetaAudio拦截，也就是说`CaptionMod.dll`必须在`MetaAudio.dll`之后安装hook。

### Trinity-EngineSv (第三方) (只支持GoldSrc)

这是为 Counter-Strike 1.6 移植的 Trinity Engine

该 Mod 的客户端部分以 MetaHook 插件的形式引入。

该 Mod 的服务端部分通过一份修改过的 reGame dll 实现。

[链接](https://github.com/ollerjoaco/Trinity-EngineSv)

### BetterSpray (第三方)

BetterSpray 是 MetaHookSv 的一个插件，为 Sven Co-op 的喷漆系统提供多张图片、真实宽高比以及动态重载支持。

[链接](https://github.com/KazamiiSC/BetterSpray-Sven-Coop)

## C/C++ 格式化

使用 [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
共享工具及固定版本 **clang-format 23.1.3**，采用 DiligentCore 风格（4 空格，保留
include 顺序）。为 CMake 使用的 Python 解释器安装格式工具：

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

格式专用配置需要 CMake 3.21+、Git、Python 3.9+（CI 使用 3.12）及构建生成器；
使用 `-G Ninja` 可无需 Visual Studio。它不准备原生 SDK 或游戏依赖。
格式目标需显式执行，不加入普通 DLL 构建。使用 Visual Studio 生成器时，执行目标
需追加 `--config Debug` 或 `--config Release`。

聚合仓库注入 `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`。
独立组件支持该 CMake 参数及同名环境变量；为空时通过 FetchContent 获取固定工具
提交。相对路径应加引号，例如
`"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`。
配置时在仓库根目录生成被 gitignore 的 `.clang-format` 供编辑器使用；格式规则应
在共享仓库修改，不修改生成副本。可通过 `FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE`
指定工具路径，但版本仍须与固定版本一致。

检查覆盖 `src/`、`include/`、`tests/` 中维护的 C/C++ 文件，包括未被 Git 忽略的新文件。
相对仓库根目录的排除规则位于 `.clang-format-ignore`；第三方源和构建产物不纳入检查。
`clang-format` workflow 在 push、pull request 和手动运行时执行全量检查。

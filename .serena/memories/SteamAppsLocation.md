# SteamAppsLocation

## 概述
`toolsrc/SteamAppsLocation` 是一个 Windows 控制台工具：输入 Steam `AppId`，通过 Steamworks `SteamApps()->GetAppInstallDir` 输出游戏安装目录（stdout），供批处理脚本自动安装/调试流程消费。

## 职责
- 读取并校验命令行参数（至少需要 `argv[1]` 作为 AppId）。
- 校验 `steam_api.dll` 可用性与 `SteamAPI_IsSteamRunning` 导出存在性。
- 读取并在必要时修复注册表中的 `SteamClientDll` 路径（`HKCU\\Software\\Valve\\Steam\\ActiveProcess`）。
- 初始化 Steam API，查询 App 安装目录，并将结果写入标准输出。
- 使用明确的错误返回码（1~8）区分失败阶段。

## 涉及文件 (不要带行号)
- `toolsrc/SteamAppsLocation/SteamAppsLocation.cpp`
- `toolsrc/SteamAppsLocation/SteamAppsLocation.vcxproj`
- `scripts/debug-helper-AIO.bat`
- `scripts/install-helper-AIO.bat`
- `.github/workflows/windows.yml`
- `.github/workflows/windows_blob.yml`
- `MetaHook.sln`
- `toolsrc/README.md`
- `README.md`
- `READMECN.md`

## 架构
核心流程集中在 `SteamAppsLocation.cpp` 的 3 个函数：
- `ReadRegistryValue(keyPath, valueName, outValue)`：读取 `HKCU` 字符串值。
- `WriteRegistryValue(keyPath, valueName, value)`：写回 `HKCU` 字符串值。
- `main(argc, argv)`：参数校验 -> 运行时检查 -> 注册表路径修复 -> Steam API 查询 -> 输出目录。

流程（源码行为）：
```mermaid
flowchart TD
  A[启动 main] --> B{argc < 2?}
  B -- 是 --> E1[stderr: AppId must be specified\nreturn 1]
  B -- 否 --> C[GetModuleHandleA("steam_api.dll")]
  C --> D{steam_api.dll 可用?}
  D -- 否 --> E2[return 2]
  D -- 是 --> F[GetProcAddress("SteamAPI_IsSteamRunning")]
  F --> G{导出存在?}
  G -- 否 --> E3[return 3]
  G -- 是 --> H[读 HKCU\\Software\\Valve\\Steam : SteamPath]
  H --> I{成功?}
  I -- 否 --> E4[return 4]
  I -- 是 --> J[读 HKCU\\Software\\Valve\\Steam\\ActiveProcess : SteamClientDll]
  J --> K{成功?}
  K -- 否 --> E5[return 5]
  K -- 是 --> L[canonical(steamPath\\steamclient.dll) 与 canonical(SteamClientDll) 比较]
  L --> M{不一致?}
  M -- 是 --> N[写回 ActiveProcess\\SteamClientDll]
  N --> O{写回成功?}
  O -- 否 --> E6[return 6]
  O -- 是 --> P[SteamAPI_Init]
  M -- 否 --> P
  P --> Q{初始化成功?}
  Q -- 否 --> E8[return 8]
  Q -- 是 --> R[SteamApps()->GetAppInstallDir(appId)]
  R --> S{成功?}
  S -- 否 --> E7[return 7]
  S -- 是 --> T[stdout 输出安装目录]\nU[SteamAPI_Shutdown]\nV[return 0]
```

与仓库流程的关系：
- `scripts/debug-helper-AIO.bat` / `scripts/install-helper-AIO.bat` 通过
  `for /f ... ('"%SolutionDir%\\tools\\SteamAppsLocation" %GameAppId% InstallDir')` 捕获 stdout，并设置 `GameDir`。
- `SteamAppsLocation.vcxproj` 的 `PostBuildEvent` 会把生成的 exe 复制到 `$(SolutionDir)tools\\`，与脚本调用路径一致。
- CI (`windows.yml` / `windows_blob.yml`) 会把 `tools\\SteamAppsLocation.exe`、`tools\\steam_appid.txt`、`tools\\steam_api.dll` 打包到发布产物。

## 依赖
- 外部库/SDK：
  - Steamworks SDK（`include/SteamSDK`，链接 `steam_api.lib`，运行时依赖 `steam_api.dll`）。
  - Windows API（注册表：`RegOpenKeyExW/RegQueryValueExW/RegSetValueExW`）。
- C++ 标准库：`<filesystem>`（`canonical` 路径归一化比较）、`<string>`、`<iostream>`。
- 工程配置：
  - `ConfigurationType=Application`（控制台程序）
  - `LanguageStandard=stdcpp20`
  - `AdditionalIncludeDirectories` 包含 `include/SteamSDK` 与 `thirdparty/Detours_fork/src`
- 运行时上下文：
  - 依赖当前用户 `HKCU` 下 Steam 相关键值存在。
  - 仓库脚本会在调用前写 `tools/steam_appid.txt`，并依赖 stdout 仅返回目录字符串。

## 注意事项
- `main` 只使用 `argv[1]`（AppId）；脚本传入的第二参数 `InstallDir` 在源码中未被使用。
- README/脚本语义表明：Steam 未登录、未拥有游戏或未安装时，目录查询可能失败。

## 调用方（可选）
- `scripts/debug-helper-AIO.bat`
- `scripts/install-helper-AIO.bat`
- CI 打包流程：`.github/workflows/windows.yml`、`.github/workflows/windows_blob.yml`（发布时复制到 `Build-Output/tools`）
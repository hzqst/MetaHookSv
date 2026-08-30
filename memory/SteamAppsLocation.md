---
title: SteamAppsLocation
type: note
permalink: metahooksv/steam-apps-location
---

# SteamAppsLocation

## Overview
`toolsrc/SteamAppsLocation` is a Windows console tool. Given a Steam `AppId`, it outputs the game's installation directory to stdout through Steamworks `SteamApps()->GetAppInstallDir`, for consumption by automated installation/debugging flows in batch scripts.

## Responsibilities
- Reads and validates command-line arguments (requires at least `argv[1]` as the AppId).
- Validates availability of `steam_api.dll` and the presence of the `SteamAPI_IsSteamRunning` export.
- Reads and, when necessary, repairs the `SteamClientDll` path in the registry (`HKCU\\Software\\Valve\\Steam\\ActiveProcess`).
- Initializes the Steam API, queries the App installation directory, and writes the result to standard output.
- Uses explicit error return codes (1-8) to distinguish failure stages.

## Involved Files (No Line Numbers)
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

## Architecture
The core flow is concentrated in three functions in `SteamAppsLocation.cpp`:
- `ReadRegistryValue(keyPath, valueName, outValue)`: reads an `HKCU` string value.
- `WriteRegistryValue(keyPath, valueName, value)`: writes an `HKCU` string value back.
- `main(argc, argv)`: argument validation -> runtime checks -> registry-path repair -> Steam API query -> directory output.

Flow (source behavior):
```mermaid
flowchart TD
  A[Start main] --> B{argc < 2?}
  B -- Yes --> E1[stderr: AppId must be specified\nreturn 1]
  B -- No --> C[GetModuleHandleA("steam_api.dll")]
  C --> D{steam_api.dll available?}
  D -- No --> E2[return 2]
  D -- Yes --> F[GetProcAddress("SteamAPI_IsSteamRunning")]
  F --> G{Export present?}
  G -- No --> E3[return 3]
  G -- Yes --> H[Read HKCU\\Software\\Valve\\Steam : SteamPath]
  H --> I{Succeeded?}
  I -- No --> E4[return 4]
  I -- Yes --> J[Read HKCU\\Software\\Valve\\Steam\\ActiveProcess : SteamClientDll]
  J --> K{Succeeded?}
  K -- No --> E5[return 5]
  K -- Yes --> L[Compare canonical(steamPath\\steamclient.dll) with canonical(SteamClientDll)]
  L --> M{Mismatch?}
  M -- Yes --> N[Write back ActiveProcess\\SteamClientDll]
  N --> O{Write-back succeeded?}
  O -- No --> E6[return 6]
  O -- Yes --> P[SteamAPI_Init]
  M -- No --> P
  P --> Q{Initialization succeeded?}
  Q -- No --> E8[return 8]
  Q -- Yes --> R[SteamApps()->GetAppInstallDir(appId)]
  R --> S{Succeeded?}
  S -- No --> E7[return 7]
  S -- Yes --> T[stdout outputs installation directory]\nU[SteamAPI_Shutdown]\nV[return 0]
```

Relationship to repository workflows:
- `scripts/debug-helper-AIO.bat` / `scripts/install-helper-AIO.bat` use
  `for /f ... ('"%SolutionDir%\\tools\\SteamAppsLocation" %GameAppId% InstallDir')` to capture stdout and set `GameDir`.
- The `PostBuildEvent` in `SteamAppsLocation.vcxproj` copies the generated exe to `$(SolutionDir)tools\\`, matching the script invocation path.
- CI (`windows.yml` / `windows_blob.yml`) packages `tools\\SteamAppsLocation.exe`, `tools\\steam_appid.txt`, and `tools\\steam_api.dll` into release artifacts.

## Dependencies
- External libraries/SDKs:
  - Steamworks SDK (`include/SteamSDK`, links `steam_api.lib`, runtime dependency on `steam_api.dll`).
  - Windows API (registry: `RegOpenKeyExW/RegQueryValueExW/RegSetValueExW`).
- C++ standard library: `<filesystem>` (normalized path comparison with `canonical`), `<string>`, `<iostream>`.
- Project configuration:
  - `ConfigurationType=Application` (console application)
  - `LanguageStandard=stdcpp20`
  - `AdditionalIncludeDirectories` includes `include/SteamSDK` and `thirdparty/Detours_fork/src`
- Runtime context:
  - Depends on Steam-related values under the current user's `HKCU`.
  - Repository scripts write `tools/steam_appid.txt` before invocation and depend on stdout returning only the directory string.

## Notes
- The README/script semantics indicate that directory lookup may fail when Steam is not logged in, the game is not owned, or the game is not installed.

## Callers (Optional)
- `scripts/debug-helper-AIO.bat`
- `scripts/install-helper-AIO.bat`
- CI packaging workflow: `.github/workflows/windows.yml`, `.github/workflows/windows_blob.yml` (copied to `Build-Output/tools` at release time)

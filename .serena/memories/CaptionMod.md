# CaptionMod

## Overview
`CaptionMod` is a MetaHook plugin that adds a VGUI2-based subtitle system, HUD text translation, multi-byte text rendering support, and a Source 2007 style chat dialog for GoldSrc/SvEngine games. It is tightly coupled to `VGUI2Extension.dll` for VGUI2 callback registration, language lookup, scheme/DPI support, and GameUI option integration.

## Responsibilities
- Expose the `IPluginsV4` plugin interface and install engine/client hooks during MetaHook plugin loading.
- Import `VGUI2Extension` interfaces and register BaseUI, ClientVGUI, and GameUI callbacks.
- Load caption resources, localization files, and CSV subtitle dictionaries from `captionmod/` resources.
- Classify dictionary rows into sound, sentence, SendAudio, HUD message, and netmessage entries, including regex-based netmessage translation.
- Convert sound playback, Sven Co-op FMOD playback, SendAudio, HudText/HudTextArgs, TextMsg, SayText, and ShowMenu events into translated HUD output or subtitle display.
- Render subtitles through `CViewport` and `SubtitlePanel`, including line wrapping, timing, queueing, anti-spam, speaker prefix, fade, and panel background behavior.
- Provide GameUI option controls for `cap_subtitle_*` cvars and a VGUI2 chat dialog controlled by `cap_newchat`.

## Involved Files (no line numbers)
- `Plugins/CaptionMod/plugins.cpp`
- `Plugins/CaptionMod/plugins.h`
- `Plugins/CaptionMod/exportfuncs.cpp`
- `Plugins/CaptionMod/exportfuncs.h`
- `Plugins/CaptionMod/privatefuncs.cpp`
- `Plugins/CaptionMod/privatefuncs.h`
- `Plugins/CaptionMod/VGUI2ExtensionImport.cpp`
- `Plugins/CaptionMod/VGUI2ExtensionImport.h`
- `Plugins/CaptionMod/Viewport.cpp`
- `Plugins/CaptionMod/Viewport.h`
- `Plugins/CaptionMod/SubtitlePanel.cpp`
- `Plugins/CaptionMod/SubtitlePanel.h`
- `Plugins/CaptionMod/message.cpp`
- `Plugins/CaptionMod/message.h`
- `Plugins/CaptionMod/chatdialog.cpp`
- `Plugins/CaptionMod/chatdialog.h`
- `Plugins/CaptionMod/cstrikechatdialog.cpp`
- `Plugins/CaptionMod/cstrikechatdialog.h`
- `Plugins/CaptionMod/GameUI.cpp`
- `Plugins/CaptionMod/BaseUI.cpp`
- `Plugins/CaptionMod/ClientVGUI.cpp`
- `Plugins/CaptionMod/util.cpp`
- `Plugins/CaptionMod/util.h`
- `Plugins/CaptionMod/MurmurHash2.cpp`
- `Plugins/CaptionMod/MurmurHash2.h`
- `Plugins/CaptionMod/CaptionMod.vcxproj`
- `docs/CaptionMod.md`
- `docs/CaptionModCN.md`
- `Build/svencoop/captionmod/`
- `Build/svencoop_hidpi/captionmod/`
- `Build/gearbox/captionmod/`
- `Build/echoes/captionmod/`
- `Build/svencoop/metahook/configs/plugins_svencoop.lst`
- `Build/svencoop/metahook/configs/plugins_goldsrc.lst`

## Architecture
CaptionMod has four main layers:

1. Plugin lifecycle and hook installation:
   - `IPluginsV4::Init` stores MetaHook interfaces.
   - `IPluginsV4::LoadEngine` captures filesystem/video/engine metadata, copies engine functions, stores original `pfnTextMessageGet` and `pfnServerCmdUnreliable`, scans engine private functions, installs engine hooks, initializes `VGUI2Extension`, registers BaseUI/ClientVGUI/GameUI callbacks, and registers DLL load notifications.
   - `IPluginsV4::LoadClient` saves original `cl_exportfuncs_t`, replaces `HUD_Init/HUD_VidInit/HUD_Frame/HUD_Redraw/HUD_Shutdown`, scans client private functions, and installs client hooks.
   - `IPluginsV4::ExitGame` unregisters VGUI2 callbacks and shuts down the VGUI2Extension import path; `Shutdown` unregisters the DLL load notification callback.

2. VGUI2 and GameUI integration:
   - `VGUI2Extension_Init` requires `VGUI2Extension.dll` and imports `IVGUI2Extension`, DPI manager, surface, scheme, and input interfaces.
   - `ClientVGUI` callbacks initialize VGUI interfaces for module name `CaptionMod`, load `captionmod/CaptionScheme.res`, load `captionmod/dictionary_%language%.txt` with English fallback, create `CViewport`, set its parent, and forward client UI activate/hide events.
   - `BaseUI` obtains `IGameUIFuncs` from the engine factory and registers BaseUI callbacks.
   - `GameUI` callbacks load `captionmod/gameui_%language%.txt`, add a CaptionMod options tab, and route connect-to-server notifications into `CViewport`.

3. Dictionary loading and lookup:
   - `CViewport::LoadBaseDictionary` reads `captionmod/dictionary.csv` via the active filesystem abstraction and parses rows with `csv::CSVReader`.
   - `CViewport::LoadCustomDictionary` loads map-specific dictionaries such as `<map>_dictionary.csv` and `<map>_dictionary_<language>.csv`.
   - `CDictionary::LoadFromRow` classifies entries by sound extension, text message lookup, sentence prefix, `SENDAUDIO:`, `MESSAGE:`, `SENTENCE:`, `NETMESSAGE:`, and `NETMESSAGE_REGEX:` prefixes; it also resolves localized text, colors, durations, speakers, chained next entries, style flags, and optional regex objects.
   - `CViewport::LinkDictionary` resolves `Next` links after all dictionary entries are loaded.
   - Non-Sven paths load base/map dictionaries from `VidInit` and `ConnectToServer`; Sven Co-op reloads dictionaries after `ScClient_SoundEngine_LoadSoundList` succeeds.

4. Event-to-subtitle and rendering flow:

```mermaid
flowchart TD
  A[MetaHook loads CaptionMod.dll] --> B[IPluginsV4::LoadEngine]
  B --> C[Engine private address scan and engine hooks]
  B --> D[VGUI2Extension callbacks]
  A --> E[IPluginsV4::LoadClient]
  E --> F[HUD export replacement and client hooks]
  D --> G[ClientVGUI creates CViewport]
  G --> H[Load caption schemes and dictionaries]
  C --> I[Sound/Sentence/FMOD hooks]
  F --> J[HUD user message hooks]
  I --> K[CViewport dictionary lookup]
  J --> K
  K --> L[SubtitlePanel::StartSubtitle]
  L --> M[Queue lines, tick updates, paint subtitles]
```

Key event paths:
- Engine sound hooks call `S_StartSoundTemplate`, then `S_StartSentence` or `S_StartWave`, apply distance/volume filters, estimate duration, and call `CViewport::StartSubtitle`.
- Sven Co-op FMOD playback calls `ScClient_SoundEngine_PlayFMODSound`, which can resolve sentence indices through SC sound-engine helpers or fall back to wave lookup.
- HUD message hooks registered by `CHudMessage::Init` intercept `HudText`, `HudTextPro`, `HudTextArgs`, `SendAudio`, `SayText`, and `TextMsg`; handled messages return early, otherwise the original user message hook is called.
- `CHudMessage::MsgFunc_HudText` and `MsgFunc_HudTextArgs` can create dynamic `client_textmessage_t` objects for translated messages and then start chained subtitles through `StartNextSubtitle`.
- `CHudMenu::Init` hooks `ShowMenu` for multi-byte menu text support, excluding Counter-Strike paths and requiring the Sven Co-op `WeaponsResource_SelectSlot` private function.
- `SubtitlePanel::StartSubtitle` converts dictionary text into wrapped wide-character lines, derives display timing from sound/message duration and cvars, queues back lines, and `OnTick` promotes ready lines into active display lines.

## Dependencies
- `VGUI2Extension.dll` and interfaces from `include/Interface/IVGUI2Extension.h`.
- MetaHook API for plugin lifecycle, command hooks, inline hooks, DLL load notifications, pattern searches, disassembly, and engine/client metadata.
- GoldSrc/SvEngine client and engine interfaces: `cl_enginefunc_t`, `cl_exportfuncs_t`, user messages, `client_textmessage_t`, sound/sentence functions, and file system interfaces.
- HLSDK and Source SDK support code included by `CaptionMod.vcxproj`, including VGUI/VGUI controls, filesystem helpers, tier0/tier1/vstdlib, and mathlib.
- CSV parser include path `$(CSVParserDirectory)` for dictionary parsing.
- Runtime resources under `captionmod/`: `CaptionScheme.res`, `SubtitlePanel.res`, `ChatScheme.res`, `ChatDialog.res`, `OptionsSubAudioAdvancedDlg.res`, `dictionary.csv`, `dictionary_%language%.txt`, `gameui_%language%.txt`, materials, and fonts.
- Plugin load order: `VGUI2Extension.dll` must be loaded before `CaptionMod.dll` in the plugin list.

## Notes
- CaptionMod intentionally fails fast if `VGUI2Extension.dll` or required VGUI2Extension interfaces are missing.
- The plugin relies on private engine/client symbol discovery and binary layout assumptions in `privatefuncs.cpp`; engine, client, or mod binary changes can break hooks or trigger signature-not-found failures.
- Project documentation marks BugFixedHL as incompatible because it has a different VGUI2 component layout.
- Dictionary encoding matters: CSV dictionaries use UTF-8 BOM for multi-byte text handling, while localization text files are expected to be UTF-16 LE for non-ASCII content.
- Missing `captionmod/dictionary.csv` is fatal in `LoadBaseDictionary`; missing custom map dictionaries are treated as debug-print-only misses.
- Sven Co-op and non-Sven dictionary lifecycles differ, so subtitle availability can depend on whether `ScClient_SoundEngine_LoadSoundList` has run.
- Source-level review points observed during analysis: `BaseUI_InstallHooks` checks `fnCreateInterface` again after querying `gameuifuncs`; `CHudMessage::MsgFunc_HudTextArgs` uses a clamp expression that appears to always select `MAX_MESSAGE_ARGS`; `CDictionary::LoadFromRow` has a lowercase-left-align branch typo because the one-letter `L` condition repeats uppercase `L`. These are documented risks, not fixed here.
- `cap_debug` is the primary runtime evidence switch for dictionary hit/miss diagnostics for sounds, sentences, SendAudio, netmessages, and HUD text messages.

## Callers (optional)
- MetaHook loader calls `IPluginsV4` lifecycle methods after loading `CaptionMod.dll` through the plugin list.
- `Build/svencoop/metahook/configs/plugins_svencoop.lst` and `Build/svencoop/metahook/configs/plugins_goldsrc.lst` list `VGUI2Extension.dll` before `CaptionMod.dll`.
- `VGUI2Extension` invokes CaptionMod's registered BaseUI, ClientVGUI, and GameUI callbacks.
- Engine/client hook paths call into CaptionMod through replaced HUD exports, user-message hooks, inline sound hooks, and Sven Co-op sound-engine hooks.

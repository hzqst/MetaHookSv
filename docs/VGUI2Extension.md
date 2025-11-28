# VGUI2Extension documentation

[中文文档](/docs/VGUI2ExtensionCN.md)

### Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

* Note that since [BugFixedHL](https://github.com/tmp64/BugfixedHL-Rebased) uses a different VGUI2 component layout, it's not gonna be working with `VGUI2Extension.dll`

## Features

### VGUI2 modding framework

`VGUI2Extension.dll` Provides capability for other plugins to install hooks / patches on VGUI2 components.

For example: Plugins can insert their own buttons into the main menu, add their own sub-pages to the options dialog tabs, and modify the size and layout of the game's own VGUI2 controls.

### Game language Enforcement

You can force the engine and VGUI2-subsystem to use language settings from either Steam or launch parameters.

Check **Launch Parameters**

### HiDpi Support

All VGUI2 elements will be proportional (scaling up base on your game resolution)

The HiDpi Support is enabled by defalut if your system's dpi scaling > 100% and you are running on a non-HL25th engine

When HiDpi Support is enabled, the following paths will be added to the FileSystem's search paths with "SKIN" tag. VGUI2 control settings will be loaded from those sources.

1. `(GameDirectory)\(ModDirectory)_dpi(DpiScalingPercentage)`

e.g. `\Sven Co-op\svencoop_dpi150` or `\Half-Life\valve_dpi200`

2. `(GameDirectory)\(ModDirectory)_hidpi`

e.g. `\Sven Co-op\svencoop_hidpi` or `\Half-Life\valve_hidpi`

#### Launch Parameters

`-steamlang` : use Steam language as engine and VGUI2-subsystem language, ignore game language setting in Steam's game config panel. for Sven Co-op, it always uses Steam language as engine and VGUI2-subsystem language no matter if `-steamlang` is added or not.

`-forcelang [language]` : force engine and VGUI2-subsystem to use [language] as engine and vgui2 language, ignore game language setting in Steam's game config panel.

`-high_dpi` : Enable HiDpi Support

`-no_high_dpi` : Disable HiDpi Support
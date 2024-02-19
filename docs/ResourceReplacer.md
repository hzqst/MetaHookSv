# ResourceReplacer documentation

[中文文档](/docs/ResourceReplacerCN.md)

### Features

This plugin replaces in-game resources (mainly model and sound files) at runtime with customizable replace list files without actually manipulating the files, just like what Sven Co-op does with [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) and [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) files.

### Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | √    |
| GoldSrc_legacy (< 6153)  | √    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |
| GoldSrc_HL25   (>= 9884) | √    |

### How to set up replacement files

Check Sven Co-op's official [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) and [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) guides for basic format.

Lines start with # or / will be ignored.

* Both `.mdl` and `.spr` replacements are supported.

* For security reason, the replaced one must have exactly the same file extension (`.mdl`, `.spr`, `.wav`) with source filename, or the replacement will be refused.

### Replacement files location

1. `\SteamLibrary\steamapps\common\Half-Life\(mod_directory)\resreplacer\default_global.gmr(.gsr)` will be used at global model(sound) replacement files, and will be loaded only on client initialiaztion.

2. `\SteamLibrary\steamapps\common\Half-Life\(mod_directory)\maps\(current_map_name).gmr(.gsr)` with be used on each new map.

### Regex Support

Lines end with `regex` will be loaded as regex replacement.

For example:

`"models/v_(.*)\.mdl" "models/new_viewmodel/v_$1\.mdl" regex`

replaces `models/v_ak47.mdl` with `models/new_viewmodel/v_ak47.mdl`

* Make sure that the `models/new_viewmodel/v_*.mdl` actually exists in the disk drive before using regex replacement.
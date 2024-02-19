# ResourceReplacer 文档

[English DOC](/docs/ResourceReplacer.md)

### 功能

该插件可以动态替换游戏内资源 (主要是模型和声音文件) 且无需修改磁盘上的文件，就像 Sven Co-op 的 [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) 和 [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) 文件提供资源替换功能一样。

### 兼容性

|        引擎              |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | √    |
| GoldSrc_legacy (< 6153)  | √    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |
| GoldSrc_HL25   (>= 9884) | √    |

### 如何编写资源替换列表

请查阅 Sven Co-op 官方的 [gmr](https://wiki.svencoop.com/Mapping/Model_Replacement_Guide) 与 [gsr](https://wiki.svencoop.com/Mapping/Sound_Replacement_Guide) 文件编写教程。

* 由 # 或 / 开头的行会被自动忽略。

* 出于安全考虑，替换后的文件名必须与替换前文件保持扩展名一致 (`.mdl`, `.spr`, `.wav`)，否则替换会被拒绝。

### 替换列表的文件位置

1. `\SteamLibrary\steamapps\common\Half-Life\(mod_directory)\resreplacer\default_global.gmr(.gsr)` 会被用作全局资源替换列表，并且只在客户端初始化时加载。

2. `\SteamLibrary\steamapps\common\Half-Life\(mod_directory)\maps\(current_map_name).gmr(.gsr)` 在每张地图预缓存之前会被加载并使用。

### 正则表达式支持

由 `regex` 结尾的行会自动启用正则表达式支持。

举例:

`"models/v_(.*)\.mdl" "models/new_viewmodel/v_$1\.mdl" regex`

该行会将 `models/v_ak47.mdl` 替换为 `models/new_viewmodel/v_ak47.mdl`

* 如果`models/new_viewmodel/v_ak47.mdl`或某个被替换后的模型文件实际上在磁盘上不存在，而游戏正好又用到了这个模型，这种情况下游戏会在加载阶段报错！
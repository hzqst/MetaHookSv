# CaptionMod 文档

[English DOC](/docs/CaptionMod.md)

### 兼容性

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

## 功能

### 强制游戏语言

你现在可以强制使用 “Steam设置中的语言” 或者 “通过启动项设置的自定义语言” 作为引擎和VGUI2使用的语言。

详见 **启动项参数**

### 基于VGUI2的字幕系统

你可以：

1、在播放声音时显示字幕

2、在播放短句(sentence)时显示字幕

3、在收到HUD文字消息时显示字幕

4、在收到SendAudio（无线电）消息时显示字幕 （仅支持Counter-Strike及它的衍生mod如Counter-Strike : Condition Zero）

### HUD文字消息翻译系统

该插件可以在游戏运行时动态地翻译HUD文字消息 (支持正则表达式匹配原始语句)

我们提供了用于翻译[restriction](http://scmapdb.com/map:restriction)系列地图消息文本的字典文件作为参考（位于"\Build\svencoop_addons\maps\restrictionXX_dictionary.csv"），其他地图的消息文本也可以参照这个格式进行翻译

你可以动态翻译原版客户端的英文HUD文字消息（如友军敌人血量显示以及game_text实体显示的文本）到其他语言（如中文）

"svencoop\captionmod\dictionary_schinese.txt" 里有一段翻译文本叫 "#SVENCOOP_PLAYERINFO" 用于展示如何以正则表达式翻译HUD消息至简体中文。

每张地图支持单独的自定义翻译字典, 字典文件需要命名为`/maps/[地图名]_dictionary.csv`，如只为中文设定字典则需要命名为`/maps/[地图名]_dictionary_schinese.csv`（如果要支持其他语言就把_schinese换成对应的语言即可）。

### VGUI1、原版HUD的多字节字符支持

1、修改原版客户端的菜单消息，为其添加多字节字符支持，用于解决中文乱码问题。

服务器需要识别CaptionMod并发送多字节的文本作为菜单内容，否则依然只会显示英文菜单。

3、修改VGUI1的文字控件，为其添加多字节字符支持，用于解决Sven Co-op、TFC等一些使用VGUI1计分板的游戏的计分板中文乱码问题。

### 新的起源风格VGUI2聊天框

设置 `cap_newchat` 为1来启用聊天框

如何修改默认的聊天文本颜色： `\Sven Co-op\svencoop\captionmod\ChatScheme.res` -> `Colors` -> `TanLight`

![](/img/1.png)

### 高DPI缩放支持

启用后所有的VGUI2元素都会根据你的游戏分辨率进行缩放。

高DPI缩放支持仅在你的系统DPI缩放比例 > 100% 并且你使用的不是HL周年更新版本的引擎时才会默认启用，其他情况下默认不启用。

当高DPI缩放支持启用时，以下路径会被添加到文件系统的搜索列表里。VGUI2控件加载res时会优先搜索以下路径：

1. `(GameDirectory)\(ModDirectory)_dpi(DpiScalingPercentage)`

举例： `\Sven Co-op\svencoop_dpi150` 或 `\Half-Life\valve_dpi200`

2. `(GameDirectory)\(ModDirectory)_hidpi`

举例：`\Sven Co-op\svencoop_hidpi` 或 `\Half-Life\valve_hidpi`

#### 控制台参数

`cap_enabled` : 是否启用字幕

`cap_hudmessage` : 是否启用HUD文本消息的翻译功能

`cap_netmessage` : 是否启用`__NETMESSAGE__`文本消息的翻译功能

`cap_max_distance` : 说话的人超过该距离时不显示其对应的字幕

`cap_debug` : 在控制台输出调试信息

`cap_newchat` : 启用新的起源风格VGUI2聊天框

`cap_lang` : 用于指示当前游戏使用的语言。修改该cvar的无任何实际效果。服务器可以发送 "svc_sendcvarvalue" 或 "svc_sendcvarvalue2" 消息，或者从userinfo "cap_lang" 中获取服务器中的玩家当前使用的语言。

#### 启动项参数

`-steamlang` : 使用Steam客户端语言作为引擎和VGUI2使用的语言，忽略Steam游戏设置中的语言设置。 对于Sven Co-op这种本身没有多语言支持的游戏，无论是否添加-steamlang都将使用Steam客户端语言。

`-forcelang [language]` : 强制引擎和VGUI2使用[language]语言，忽略Steam游戏设置中的语言设置。

`-high_dpi` : 启用高DPI缩放支持

`-no_high_dpi` : 禁用高DPI缩放支持
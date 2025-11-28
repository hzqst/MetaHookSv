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

* 由于该插件高度依赖修改VGUI2子系统，所以必须和`VGUI2Extension.dll`配合使用。

* 注意： [BugFixedHL](https://github.com/tmp64/BugfixedHL-Rebased) 使用了不同的VGUI2对象布局, 因此无法兼容 `CaptionMod.dll` 。

## 功能

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

每张地图支持单独的自定义翻译字典, 字典文件需要命名为`/maps/[地图名]_dictionary.csv`，如只为中文设定字典则需要命名为`/maps/[地图名]_dictionary_schinese.csv`（如果要支持其他语言就把_schinese换成对应的语言即可）。

* 为了在 [启用了全局UTF8支持的Windows 11](https://learn.microsoft.com/zh-cn/windows/apps/design/globalizing/use-utf8-code-page) 上正常显示中文, 你应当将 `_dictionary.csv` 文件保存为 `UTF8-BOM` 编码。

"svencoop\captionmod\dictionary_schinese.txt" 里有一段翻译文本叫 "#SVENCOOP_PLAYERINFO" 用于展示如何以正则表达式翻译HUD消息至简体中文。

* 为了在本地化文件中支持非ASCII字符, 你应当将 `_%language.txt%` (`_schinese.txt` / `_korean.txt`) 文件保存为 `UTF16-LE` 编码.

### VGUI1、原版HUD的多字节字符支持

1、修改原版客户端的菜单消息，为其添加多字节字符支持，用于解决中文乱码问题。

服务器需要识别CaptionMod并发送多字节的文本作为菜单内容，否则依然只会显示英文菜单。

3、修改VGUI1的文字控件，为其添加多字节字符支持，用于解决Sven Co-op、TFC等一些使用VGUI1计分板的游戏的计分板中文乱码问题。

### 新的起源风格VGUI2聊天框

设置 `cap_newchat` 为1来启用聊天框

如何修改默认的聊天文本颜色： `\Sven Co-op\svencoop\captionmod\ChatScheme.res` -> `Colors` -> `TanLight`

![](/img/1.png)

#### 控制台参数

`cap_hudmessage` : 是否启用HUD文本消息的翻译功能

`cap_netmessage` : 是否启用`__NETMESSAGE__`文本消息的翻译功能

`cap_max_distance` : 说话的人超过该距离时不显示其对应的字幕

`cap_min_avol` : 音量小于该数值时不显示其对应的字幕

`cap_debug` : 在控制台输出调试信息，可以使用`cap_debug 1`启动调试信息来观察词条是否能在字幕系统的字典中找到。

`cap_newchat` : 启用新的起源风格VGUI2聊天框

`cap_lang` : 用于指示当前游戏使用的语言。修改该cvar的无任何实际效果。服务器可以发送 "svc_sendcvarvalue" 或 "svc_sendcvarvalue2" 消息，或者从userinfo "cap_lang" 中获取服务器中的玩家当前使用的语言。

#### 控制台参数 (字幕系统)

`cap_enabled` : 是否启用字幕，设为1时启用字幕（如果有字幕文件可用的话）

`cap_subtitle_prefix` : 是否在字幕前显示显示说话人的名字。

`cap_subtitle_waitplay` : 是否等待之前的字幕显示完再显示新的字幕，设为1时只有在旧字幕全部显示出来之后新的字幕才会被显示出来，为0时新字幕会绕过等待旧字幕的过程，直接插队到字幕队列的最前端。

`cap_subtitle_antispam` : 设为1时防止同样的字幕刷屏。

`cap_subtitle_fadein` : 新字幕的淡入时间，单位：秒。

`cap_subtitle_fadeout` : 旧字幕的淡出时间，单位：秒。

`cap_subtitle_holdtime` : 默认的字幕持续时间，单位：秒。

`cap_subtitle_stimescale` : "字幕起始时间"的缩放因子。

`cap_subtitle_htimescale` :  "字幕持续时间"的缩放因子。
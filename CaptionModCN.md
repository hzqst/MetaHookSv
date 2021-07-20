# CaptionMod 文档

[English DOC](CaptionMod.md)

### 功能

1. 播放声音时显示字幕

2. 播放短句(sentence)时显示字幕

3. 收到HUD文字消息时显示字幕

4. 收到SendAudio（无线电）消息时显示字幕

5. 翻译HUD文字消息 (支持正则表达式)

6. 修改原版客户端的HUD文字消息（如友军敌人血量显示以及game_text实体显示的文本），为其添加多字节字符支持。（用于解决HUD中文乱码问题）

7. 修改VGUI1的文字控件，为其添加多字节字符支持。（用于解决计分板中文乱码问题）

8. 每张地图支持单独的自定义翻译字典, 字典文件在"/maps/[地图名]_dictionary.csv"

我们提供了用于翻译潜行者系列地图消息文本的字典文件作为参考（位于"\Build\svencoop_addons\maps\restrictionXX_dictionary.csv"），其他地图的消息文本也可以参照这个格式进行翻译

"svencoop\captionmod\dictionary_schinese.txt" 里有一段翻译文本叫 "#SVENCOOP_PLAYERINFO" 用于展示如何以正则表达式翻译HUD消息，不过该翻译文本并不会在游戏中生效，因为 Sven-Coop 不支持其他语言的本地化语言文件，它只会读取 "dictionary_english.txt"。

所以如果你需要翻译游戏文本请直接用 "dictionary_[你的语言].txt" 替换 "dictionary_english.txt"。

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

### 兼容性

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

#### 控制台参数

cap_enabled 0 / 1 : 是否显示字幕

cap_netmessage 0 / 1 :是否显示HUD文字消息的翻译

cap_debug 0 / 1 : 当有声音播放或者HUD文字消息时在控制台输出调试信息

# StudioEvents 文档

[English DOC](/docs/StudioEvents.md)

### 功能

该插件可以防止重复播放模型自带音效，防止音效反复刷屏

![](/img/8.png)

### 兼容性

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | √    |
| GoldSrc_legacy (< 6153)  | √    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |
| GoldSrc_HL25   (>= 9884) | √    |

### 控制台参数

`cl_studiosnd_debug 0 / 1` : 是否在控制台输出调试消息

`cl_studiosnd_anti_spam_diff (时长，秒)` : 如果之前一段时间内有不同的模型自带音效已经播放过了，那么接下来一段时间里将不会再播放任何模型自带音效。

`cl_studiosnd_anti_spam_same (时长，秒)` : 如果之前一段时间内有相同的模型自带音效已经播放过了，那么接下来一段时间里将不会再播放任何相同的模型自带音效。

`cl_studiosnd_anti_spam_delay 0 / 1` : 设为1之后音效会被延时播放而不是直接阻止播放。

`cl_studiosnd_block_player 0 / 1` : 设为1之后所有来自玩家的模型自带音效会被阻止播放（不包括viewmodel）。

### Whitelist

`studioevents/sound_whitelist.txt` : 对音频文件名进行白名单匹配，匹配的音频将会完全绕过anti-spam检查，声音文件名必须完全匹配（大小写敏感）.

`studioevents/sourcemodel_whitelist.txt` : 对音频来源的模型文件名进行白名单匹配，匹配的音频将会完全绕过anti-spam检查，模型文件名必须完全匹配（大小写敏感）.
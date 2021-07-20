# StudioEvents 文档

[English DOC](StudioEvents.md)

### 功能

该插件可以防止重复播放模型自带音效，防止音效反复刷屏

![](https://github.com/hzqst/MetaHookSv/raw/main/img/8.png)

### 兼容性

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | √    |
| GoldSrc_legacy (< 6153)  | √    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

### 控制台参数

cl_studiosnd_debug 0 / 1 : 播放模型自带音效时在控制台输出消息

cl_studiosnd_anti_spam_diff (时间长度) : 如果之前一段时间内有不同的模型自带音效已经播放过了，那么接下来一段时间里将不会再播放任何模型自带音效。

cl_studiosnd_anti_spam_same (时间长度) : 如果之前一段时间内有相同的模型自带音效已经播放过了，那么接下来一段时间里将不会再播放任何相同的模型自带音效。

cl_studiosnd_anti_spam_delay 0 / 1 : 设为1之后音效会被延时播放而不是直接阻止播放。
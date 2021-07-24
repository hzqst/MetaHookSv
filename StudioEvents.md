# StudioEvents documentation

[中文DOC](StudioEventsCN.md)

### Features

This plugin can block studio-event sound spamming with controllable cvars.

![](/img/8.png)

### Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | √    |
| GoldSrc_legacy (< 6153)  | √    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

### Console Vars

cl_studiosnd_debug 0 / 1 : To output debug message to the game console when there is a studio-event sound playing or blocked.

cl_studiosnd_anti_spam_diff (duration) : Any studio-event sound will be blocked or delayed if another different studio-event sound was played within this duration before.

cl_studiosnd_anti_spam_same (duration) : Any studio-event sound will be blocked or delayed if another studio-event sound with same name was played within this duration before.

cl_studiosnd_anti_spam_delay 0 / 1 : When set to 1, studio-event sound will be delayed instead of blocked for anti-spam.
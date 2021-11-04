# CaptionMod documentation

[中文DOC](CaptionModCN.md)

## Features

### Use Steam language or any other customized language as game language

check **Command Arguments**

### Display subtitles when sound is played

### Display subtitles when sentence is played

### Display subtitles when client receives HudText message

### Display subtitles when client receives SendAudio message

### Translate HUDText message into other language dynamically (regex supported)

There is a example file demostrates how to translate constant HUD TextMessage to `schinese` , the file is at  `\Sven Co-op\svencoop_schinese\maps\restriction02_dictionary.csv`.

There is a example file demostrates how to translate dynamic HUD TextMessage to `schinese` using regex, the file is at `\Sven Co-op\svencoop\captionmod\dictionary_schinese.txt` called "#SVENCOOP_PLAYERINFO".

### Hook original client's old-style HudText and draw it with multi-byte character support.

### Hook VGUI1 TextImage's paint procedure and draw it with multi-byte character support.

* You can see multi-byte characters rendered correctly on scoreboard.

### Custom dictionary support for specified map, put dictionary file at "/maps/[mapname]_dictionary.csv"

### New Source2007 style VGUI2 ChatDialog

Set cvar `cap_newchat` to 1 to enable new chat dialog.

The default chat text color can be customized in `\Sven Co-op\svencoop\captionmod\ChatScheme.res` -> `Colors` -> `TanLight`

![](/img/1.png)

### Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

#### Console Vars

`cap_enabled` : To enable or disable CaptionMod's subtitle display.

`cap_hudmessage` : To enable or disable CaptionMod's HUD TextMessage translation.

`cap_netmessage` : To enable or disable CaptionMod's Network HUD TextMessage translation. (`__NETMESSAGE__`)

`cap_max_distance` : Ignore sound or sentences (which are supposed to play subtitles) whose speaker is too far away from this distance.

`cap_debug` : To output debug message when there is a HUD TextMessage or sound playing.

`cap_newchat` : To enable or disable Source2007 style VGUI2 chat dialog.

#### Command Arguments

`-steamlang` : use Steam language as engine and vgui2 language, ignore game language setting in Steam's game config panel. for Sven Co-op, it always uses Steam language as engine and vgui2 language no matter if -steamlang is added or not.

`-forcelang [language]` : force engine and vgui2 to use [language] as engine and vgui2 language, ignore game language setting in Steam's game config panel.

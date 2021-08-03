# CaptionMod documentation

[中文DOC](CaptionModCN.md)

## Features

## Display subtitles when sound is played

## Display subtitles when sentence is played

## Display subtitles when client receives HudText message

## Display subtitles when client receives SendAudio message

## Translate HUDText message into other language dynamically (regex supported)

There is a example demo shows you how to translate constant HUD TextMessage into other language in "Build\svencoop\maps\restriction02_dictionary.csv".

There is a example demo shows you how to translate dynamic HUD TextMessage into other language with regex in "Build\svencoop\captionmod\dictionary_schinese.txt" called "#SVENCOOP_PLAYERINFO", however it won't take effect in game since Sven-Coop only uses "dictionary_english.txt" as localization file.

Modify and overwrite "dictionary_english.txt" is the best option if you are expected to use non-English language in game.

## Hook original client's old-style HudText and draw it with multi-byte character support.

## Hook VGUI1 TextImage's paint procedure and draw it with multi-byte character support.

* You can see multi-byte characters rendered correctly on scoreboard.

## Custom dictionary support for specified map, put dictionary file at "/maps/[mapname]_dictionary.csv"

## New Source-Engine style ChatDialog

Set cvar `cap_newchat` to 1 to enable new chat dialog.

The default chat text color could be customized in `\svencoop\captionmod\ChatScheme.res` -> `Colors` -> `TanLight`

![](/img/1.png)

### Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | √    |
| SvEngine       (8832 ~)  | √    |

#### Console Vars

cap_enabled 0 / 1 : To enable or disable CaptionMod's subtitle display.

cap_netmessage 0 / 1 : To enable or disable CaptionMod's HUD TextMessage translation.

cap_debug 0 / 1 : To output debug message when there is a HUD TextMessage or sound playing.

####

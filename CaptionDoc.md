# CaptionMod documentation

### Compatibility

|        Engine            |      |
|        ----              | ---- |
| GoldSrc_blob   (< 4554)  | -    |
| GoldSrc_legacy (< 6153)  | -    |
| GoldSrc_new    (8684 ~)  | -    |
| SvEngine       (8832 ~)  | √    |

For GoldSrc engine : please try https://github.com/hzqst/CaptionMod

#### Features

1. Display subtitles when sound is played.

2. Display subtitles when sentence is played.

3. Display subtitles when there is a HUD TextMessage.

4. Translate HUD TextMessage dynamically (regex supported).

5. Hook original client's old-style HUD TextMessage and draw it with multi-byte character support.

6. Hook VGUI1's TextImage control paint procedure and draw it with multi-byte character support.

7. Custom dictionary for each map, put dictionary file at "/maps/[mapname]_dictionary.csv"

There is a example demo shows you how to translate constant HUD TextMessage into other language in "Build\svencoop\maps\restrictionXX_dictionary.csv".

There is a example demo shows you how to translate dynamic HUD TextMessage into other language with regex in "Build\svencoop\captionmod\dictionary_schinese.txt" called "#SVENCOOP_PLAYERINFO", however it won't take effect in game since Sven-Coop only uses "dictionary_english.txt" as localization file.

Modify and overwrite "dictionary_english.txt" is the best option if you are expected to use non-English language in game.

![](https://github.com/hzqst/MetaHookSv/raw/main/img/1.png)

#### Console Vars

cap_enabled 0 / 1 : to enable or disable CaptionMod's subtitle display.

cap_netmessage 0 / 1 : to enable or disable CaptionMod's HUD TextMessage translation.

cap_debug 0 / 1 : to output debug message when there is a HUD TextMessage or sound playing.

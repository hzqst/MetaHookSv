# CaptionMod documentation

[中文文档](/docs/CaptionModCN.md)

### Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

* Since this plugin heavily relies on modifying the VGUI2 subsystem, it must be used together with `VGUI2Extension.dll`.

* Note that since [BugFixedHL](https://github.com/tmp64/BugfixedHL-Rebased) uses a different VGUI2 component layout, it's not gonna be working with `CaptionMod.dll`

## Features

### VGUI2-based Subtitle System

You can:

1. Display subtitles when sound is played

2. Display subtitles when sentence is played

3. Display subtitles when client receives HudText message

4. Display subtitles when client receives SendAudio message

The subtitles are defined in `\Sven Co-op\svencoop\captionmod\dictionary.csv`, which can be opened with Microsoft Excel or any other open-sourced CSV editors.

### HUDText Message Translation System

You can translate HUDText message into other language dynamically (regex supported)

There is a example file demostrates how to translate static HUD TextMessage to `schinese` , the file is at  `\Sven Co-op\svencoop_schinese\maps\restriction02_dictionary.csv`.

* To support [Windows 11 with system-wide UTF-8 enabled](https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page), you should always save `_dictionary.csv` as `UTF8-BOM`.

There is a example file demostrates how to translate dynamic HUD TextMessage to `schinese` using regex, the file is at `\Sven Co-op\svencoop\captionmod\dictionary_schinese.txt` called `#SVENCOOP_PLAYERINFO`.

* To support non-ASCII charachers in localization file, you should always save `_%language.txt%` (`_schinese.txt` / `_korean.txt`) as `UTF16-LE`.

### Mutli-byte character support for legacy VGUI1 and HUD elements

1. Client's HudText are overhauled with multi-byte character support. (Sven Co-op and Half-Life)

2. VGUI1 TextImage's are overhauled with multi-byte character support.

* You can see multi-byte characters rendered correctly on VGUI1 scoreboard. (Sven Co-op and Half-Life)

3. Client's HUDMenu are overhauled with multi-byte character support. (Sven Co-op only)

* You can see multi-byte characters rendered correctly on HudMenu.

### Source2007 style VGUI2-based ChatDialog

Set cvar `cap_newchat` to 1 to enable the new VGUI2-based chat dialog.

The default chat text color can be customized in `\Sven Co-op\svencoop\captionmod\ChatScheme.res` -> `Colors` -> `TanLight`

![](/img/1.png)

#### Console Vars

`cap_hudmessage` : To enable or disable CaptionMod's HUD TextMessage translation.

`cap_netmessage` : To enable or disable CaptionMod's `__NETMESSAGE__` HUD TextMessage translation.

`cap_max_distance` : Ignore sound or sentences (which are supposed to play subtitles) whose speaker is too far away from this distance.

`cap_min_avol` : Ignore sound or sentences (which are supposed to play subtitles) which plays with volume smaller than this value.

`cap_debug` : To output debug message of the Subtitle System and HudMessage Translation System. You can set `cap_debug` to 1 to check wether an dictionary entry is found or not in the Subtitle System.

`cap_newchat` : To enable or disable Source2007 style VGUI2 chat dialog.

`cap_lang` : Indicate current game lanuage. Any changes to this cvar acutally do nothing. Provide the capability of getting client's current game language by sending network message "svc_sendcvarvalue" or "svc_sendcvarvalue2" or reading userinfo "cap_lang" from server.

#### Console Vars (Subtitle System)

`cap_enabled` : Set to 1 to display subtitles (if available).

`cap_subtitle_prefix` : Set to 1 to display speaker's name before subtitle lines.

`cap_subtitle_waitplay` : Set to 1 to wait for all previous lines from being displayed before playing next line.

`cap_subtitle_antispam` : Set to 1 to prevent the subtitle from spamming.

`cap_subtitle_fadein` : Fade-in duration for new subtitle lines. (unit: sec.)

`cap_subtitle_fadeout` : Fade-out duration for retired subtitle lines. (unit: sec.)

`cap_subtitle_holdtime` : Default hold duration for subtitle lines. (unit: sec.)

`cap_subtitle_stimescale` : Scaling factor for "StartTime".

`cap_subtitle_htimescale` : Scaling factor for "HoldTime".

`cap_subtitle_extraholdtime` : Extra holdtime added to the last line of subtitles. (unit: sec.)
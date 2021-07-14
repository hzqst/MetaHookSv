# Compiled Binaries and Resources

### svencoop.exe (required)

Main executable loader to bootstrap metahook plugins and start game.

### FreeImage.dll (required)

Imageloader required by MetaRenderer plugin.

### SDL.dll (required)

The SDL2.dll fixes a bug that the IME input handler from original SDL library provided by valve was causing buffer overflow and game crash when using non-english IME. you don't need to copy it if you don't have a non-english IME.

### svencoop/captionmod/* (required)

All resource files in this directory are required by CaptionMod plugin.

The diectionary files are ported from original GoldSrc version of [CaptionMod](https://github.com/hzqst/CaptionMod)

### svencoop/metahook/configs/plugins.lst (required)

Plugins load list. you can disable specified plugin or change load order by editing this file.

### svencoop/metahook/configs/plugins/*.dll (required)

Compiled binary file of all the plugins.

### svencoop/metahook/renderer/shader/* (required)

All shader files required by MetaRenderer plugin.

### svencoop/metahook/renderer/texture/* (required)

All texture files required by MetaRenderer plugin.

### svencoop_addons/maps/restriction02_detail.txt (optional)

An example for MetaRenderer plugin to use Parallax-Mapping and Normal-Mapping in restriction02.bsp

### svencoop_addons/maps/*.csv (optional)

An example for CaptionMod Plugin to display chinese translation of subtitles and HUDMessages.

### svencoop_addons/models/barnacle.mdl (optional)

### svencoop_addons/models/player/* (optional)

#### NSFW Warning! NSFW Warning! NSFW Warning!

The following models: [GFL_HK416](https://gamebanana.com/mods/167185), [GFL_M14](https://gamebanana.com/mods/167065), [GFL_M14-c2](https://gamebanana.com/mods/167065), [GI_Keqing](https://gamebanana.com/mods/290942), touhou_mystia, helmet

will be transformed into ragdolls when being caught by barnacle.

barnacle will emit piston sound every one second while chewing.

### svencoop_addons/sound/barnacle/* (optional)

#### NSFW Warning! NSFW Warning! NSFW Warning!

vore sound efx for barnacle.

### svencoop_addons/sound/player/* (optional)

#### NSFW Warning! NSFW Warning! NSFW Warning!

player vore sound efx when being caught by barnacle.

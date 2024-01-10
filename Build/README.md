# Compiled Binaries and Resources

### MetaHook.exe / MetaHook_blob.exe (required)

Main executable loader to bootstrap metahook plugins and start game.

* Use MetaHook_blob.exe instead of MetaHook.exe to load legacy blob engine, such as engine with buildnum 3248 or 3266.

### SDL.dll (optional)

The SDL2.dll fixes a bug that the IME input handler from original SDL library provided by valve was causing buffer overflow and game crash when using non-english IME. you don't need to copy it if you don't have a non-english IME.

* Valve fixed this issue for SDL2 in HL25th patch, you don't have to replace SDL2 if you are running HL25th engine.

### svencoop/captionmod/* (optional)

All resource files in this directory are required by CaptionMod plugin.

The diectionary files are ported from original GoldSrc version of [CaptionMod](https://github.com/hzqst/CaptionMod)

### svencoop/metahook/configs/plugins_svencoop.lst (required, rename)

### svencoop/metahook/configs/plugins_goldsrc.lst (required, rename)

Must be renamed to `plugins.lst`.

This is the plugin load list.

You can disable specified plugin or change load order by editing this file.

### svencoop/metahook/configs/plugins/*.dll (optional)

Compiled dll of all plugins.

### svencoop/metahook/renderer/shader/* (optional)

All shader files required by `Renderer.dll` plugin.

### svencoop/metahook/renderer/texture/* (optional)

All texture files required by `Renderer.dll` plugin.

### svencoop_addons/maps/restriction02_detail.txt (optional)

An example for `Renderer.dll` plugin to use Parallax-Mapping and Normal-Mapping in restriction02.bsp

### svencoop_addons/maps/*.csv (optional)

An example for `CaptionMod.dll` Plugin to display chinese translation of subtitles and HUDMessages.

### svencoop_addons/models/* (optional)
### svencoop_addons/sound/* (optional)

The following models: [GFL_HK416](https://gamebanana.com/mods/167185), [GFL_M14](https://gamebanana.com/mods/167065), [GFL_M14-c2](https://gamebanana.com/mods/167065), [GI_Keqing](https://gamebanana.com/mods/290942), touhou_mystia, helmet

will become ragdolls when being caught by barnacle, while playing vore sound.
# VGUI2Extension 文档

[English DOC](/docs/VGUI2Extension.md)

### 兼容性

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

* 注意： [BugFixHL](https://github.com/tmp64/BugfixedHL-Rebased) 使用了不同的`KeyValues`布局, 因此无法兼容 `VGUI2Extension.dll` 。

## 功能

### 作为VGUI2 MOD框架

`VGUI2Extension.dll` 以回调的方式为其他插件提供了修改VGUI2组件的能力。

比如：可以在主菜单中插入自己的按钮、可以在选项对话框的tab页中插入自己的子页面、可以修改游戏自己的VGUI2控件的大小和布局。

### 强制游戏语言

你现在可以强制使用 “Steam设置中的语言” 或者 “通过启动项设置的自定义语言” 作为引擎和VGUI2使用的语言。

详见 **启动项参数**

### 高DPI缩放支持

启用后所有的VGUI2元素都会根据你的游戏分辨率进行缩放。

高DPI缩放支持仅在你的系统DPI缩放比例 > 100% 并且你使用的不是HL周年更新版本的引擎时才会默认启用，其他情况下默认不启用。

当高DPI缩放支持启用时，以下路径会被添加到文件系统的搜索列表里。VGUI2控件加载res时会优先搜索以下路径：

1. `(GameDirectory)\(ModDirectory)_dpi(DpiScalingPercentage)`

举例： `\Sven Co-op\svencoop_dpi150` 或 `\Half-Life\valve_dpi200`

2. `(GameDirectory)\(ModDirectory)_hidpi`

举例：`\Sven Co-op\svencoop_hidpi` 或 `\Half-Life\valve_hidpi`

#### 启动项参数

`-steamlang` : 使用Steam客户端语言作为引擎和VGUI2使用的语言，忽略Steam游戏设置中的语言设置。 对于Sven Co-op这种本身没有多语言支持的游戏，无论是否添加-steamlang都将使用Steam客户端语言。

`-forcelang [language]` : 强制引擎和VGUI2使用[language]语言，忽略Steam游戏设置中的语言设置。

`-high_dpi` : 启用高DPI缩放支持

`-no_high_dpi` : 禁用高DPI缩放支持

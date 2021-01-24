字幕制作者以及DIY玩家必看

控制台参数cap_show 0 / 1
设为1显示调试信息，播放任何wav/SENTENCE/textmessage的时候都会显示到控制台。

控制台指令cap_version
查看当前插件版本

==文件说明==

gamedir/captionmod/CaptionScheme.res
里面是各种颜色和字体的设置，还有加载ttf字体也在这里。

gamedir/captionmod/SubtitlePanel.res
里面是字幕背景以及字幕一些的细节设置。

gamedir/captionmod/fonts/
自定义字体，使用除系统自带（黑体宋体楷体，vista以上再加上雅黑..）以外的字体都必须在这里放上对应的ttf文件才能加载。
加载方法：放入对应字体ttf文件后，在CaptionScheme.res的CustomFontFiles键下按照格式加入一行，前面那个名字随便填，后面那个路径必须填正确。
然后在Fonts键下加入你的自定义字体，或者直接修改"SubtitleFont"，将name填成自定义字体的真实字体名（如宋体是SimSun，黑体是SimHei，其他字体双击ttf文件，安装按钮下面那个就是）即可。

gamedir/captionmod/materials/
里面是实现字幕板圆角功能的贴图

gamedir/captionmod/dictionary_english.txt
gamedir/captionmod/dictionary_schinese.txt
gamedir/captionmod/dictionary_language.txt
本地化文件，用来支持多语言的翻译文本，一般用不上。

gamedir/captionmod/dictionary.csv(.xls .xlsx)
字典文件，该文件为整个插件和核心，编码方式默认为ANSI/GB2312，不建议你直接在csv里写非ASCII字符如中文，这样在别的语言的操作系统下打开就是乱码，除非你只是为简体中文做汉化。
触发字幕总共有三种方式：
1.wav触发 游戏一旦播放任何wav文件拿wav路径（不包括sound/）去扫描字典，扫到对应wav就会触发字幕
2.SENTENCE触发 gamedir\sound\sentences.txt文件里的比如HG_GREN0就是一个SENTENCE
3.“下一句字幕”方式，由别的字幕或textmessage的“下一句字幕”触发。（请注意不要自己触发自己或者让触发链成环了）

==字典文件详细介绍==

Title/标题：
.wav结尾代表该条字幕由声音触发，注意不要加sound/前缀！

titles.txt中的message可以直接修改message的文本和持续时间。
也可以使用“下一句字幕”功能直接显示一句字幕到字幕池，

注意任何触发形式的字幕的标题都对大小写敏感！包括声音路径、SENTENCE、MESSAGE、下一句字幕！

Sentence/翻译文本：
直接写最终显示的文本，支持换行（Office Excel和WPS都是Alt+Enter换行）
或者写“#”号加上语言文件dictionary_english/schinese.txt里的文本

Color/颜色：
必须是CaptionScheme.res里声明的颜色才有用，对大小写敏感
如果是textmessage型字幕，颜色可以填写两个，分别对应textmessage的$color和$color2设定。
（也可以只填一个颜色，表示只修改$color。不填就是不修改）

Duration/字幕持续时间：
单位为秒，0或不设则根据声音时长决定。
如果是textmessage型字幕，持续时间对应原textmessage的$holdtime设定，不填就是不修改。

Speaker/说话人：
在字幕前加上[XXXX]的前缀，可以在SubtitlePanel.res中关闭，支持#+文本的方式引用语言文件。

Next Subtitle/下一句字幕：
填对应字幕标题，大小写敏感！

Delay to Next Subtitle/下一句延迟：
当前字幕开始播放后过多少秒播放“下一句字幕”

Style/样式：
可以在此单独调整每个字幕的文本对齐方式和淡入方式，可填L / C / R / leftalign / centeralign / rightalign（不分大小写），分别代表左中右，不填默认左对齐
填alphafade / leftscan 代表设置淡入方式为alpha淡入/从左到右扫描出现
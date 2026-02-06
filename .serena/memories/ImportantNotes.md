# Important Notes

## 安全与边界
- 该项目用于合法的游戏增强/Modding（防御性安全工具定位）。
- 不应引入恶意代码或漏洞利用相关逻辑。

## 兼容性
- 某些功能在不同引擎类型上不可用；做特定操作前先判定引擎类型。

## 插件加载顺序
- `plugins.lst` 中的加载顺序对依赖与 hook 安装非常关键。

# 引擎兼容性

MetaHookSv 支持多种 GoldSrc / SvEngine 变体，常见类型与版本范围：
- `GoldSrc_blob`（buildnum 3248 ~ 4554）：legacy encrypted format
- `GoldSrc_legacy`（< 6153）
- `GoldSrc_new`（8684+）
- `SvEngine`（8832+）：Sven Co-op 修改版引擎
- `GoldSrc_HL25`（>= 9884）：Half-Life 25th Anniversary Update

注意点：
- 做特定引擎相关的逻辑之前，应当先用 `g_pMetaHookAPI->GetEngineType()` 判断引擎类型。
- legacy blob engines （非正常PE文件，直接以二进制形式内存加载） 需要使用 blob-specific APIs。

# StudioEvents 插件文档

## 概述

StudioEvents 是一个 MetaHook 插件，用于控制和过滤 Half-Life 游戏中的 Studio 模型事件声音。该插件实现了智能的防重复播放（anti-spam）机制，防止声音事件过度播放，提升游戏体验。

## 主要功能

### 1. 声音事件过滤
- 拦截并处理 Studio 模型事件（event 5004）
- 根据配置的规则决定是否播放、延迟或阻止声音
- 支持区分"相同声音"和"不同声音"的过滤策略

### 2. 防重复播放机制
- **相同声音过滤**：防止同一实体、同一帧、同一声音文件的重复播放
- **不同声音过滤**：控制不同声音事件之间的最小时间间隔
- **自动清理**：定期清理过期的声音事件记录，优化内存使用

### 3. 延迟播放系统
- 可选的声音延迟播放功能
- 当声音被防重复播放机制阻止时，可以延迟到允许的时间再播放
- 自动处理延迟队列，在合适的时机触发声音

### 4. 玩家声音阻止
- 可选择性阻止来自玩家实体的所有声音事件
- 适用于需要减少玩家声音干扰的场景

## 配置变量（CVARs）

### cl_studiosnd_anti_spam_diff
- **默认值**: 0.5
- **类型**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **说明**: 不同声音事件之间的最小时间间隔（秒）
- **用途**: 防止短时间内播放过多不同的声音

### cl_studiosnd_anti_spam_same
- **默认值**: 1.0
- **类型**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **说明**: 相同声音事件之间的最小时间间隔（秒）
- **用途**: 防止同一声音重复播放

### cl_studiosnd_anti_spam_delay
- **默认值**: 0
- **类型**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **说明**: 是否启用延迟播放模式（0=直接阻止，非0=延迟播放）
- **用途**: 控制被阻止的声音是直接丢弃还是延迟播放

### cl_studiosnd_block_player
- **默认值**: 0
- **类型**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **说明**: 是否阻止玩家实体的声音事件（0=不阻止，>0=阻止）
- **用途**: 完全屏蔽玩家相关的声音事件

### cl_studiosnd_debug
- **默认值**: 0
- **类型**: FCVAR_CLIENTDLL
- **说明**: 调试模式开关（0=关闭，>0=开启）
- **用途**: 在控制台输出声音事件的处理信息

## 核心数据结构

### studio_event_sound_t
```cpp
struct studio_event_sound_s {
    char name[64];      // 声音文件名
    int entindex;       // 实体索引
    int frame;          // 动画帧
    float time;         // 时间戳
}
```

存储声音事件的关键信息，用于判断是否为重复声音以及何时可以再次播放。

### 全局容器
- **g_StudioEventSoundPlayed**: 存储已播放的声音事件记录
- **g_StudioEventSoundDelayed**: 存储等待延迟播放的声音事件

## 核心函数

### HUD_Init()
- 插件初始化入口点
- 注册所有 CVAR 配置变量
- 清空声音事件记录列表
- 调用原始的 HUD_Init

### HUD_VidInit()
- 视频模式初始化时调用
- 清空所有声音事件记录（已播放和延迟播放）
- 确保切换关卡或重新连接时状态重置

### HUD_Frame(double a1)
- 每帧调用一次
- 处理延迟播放队列
- 检查延迟声音是否到达播放时间
- 验证实体有效性（通过 messagenum 判断）
- 调用 HUD_StudioEvent 播放到期的延迟声音

### HUD_StudioEvent(const mstudioevent_s* ev, const cl_entity_s* ent)
核心过滤逻辑函数，处理流程如下：

1. **事件类型检查**：只处理 event 5004（声音事件）
2. **玩家阻止检查**：如果启用了 cl_studiosnd_block_player 且是玩家实体，直接阻止
3. **历史记录遍历**：
   - 清理过期的声音记录
   - 检查是否与最近播放的声音冲突
   - 区分"相同声音"和"不同声音"的判断逻辑
4. **决策处理**：
   - 如果发现冲突且启用延迟：将声音加入延迟队列
   - 如果发现冲突但未启用延迟：直接阻止
   - 如果无冲突：立即播放并记录
5. **调用原始函数**：最终调用 gExportfuncs.HUD_StudioEvent

## 工作原理

### 反重复播放判断逻辑

```
对于每个新的声音事件：
├─ 是否是玩家且启用了阻止？
│  └─ 是 → 阻止
├─ 遍历历史记录：
│  ├─ 记录已过期？
│  │  └─ 是 → 删除记录
│  ├─ 是相同声音（实体+帧+名称都相同）？
│  │  └─ 是 → 检查是否在 anti_spam_same 时间内
│  │     └─ 是 → 标记冲突，记录最大等待时间
│  └─ 是不同声音？
│     └─ 检查是否在 anti_spam_diff 时间内
│        └─ 是 → 标记冲突，记录最大等待时间
└─ 发现冲突？
   ├─ 是 且 启用延迟 → 加入延迟队列
   ├─ 是 且 未启用延迟 → 直接阻止
   └─ 否 → 立即播放并记录
```

### 延迟播放机制

1. 被阻止的声音加入 `g_StudioEventSoundDelayed`，带有计算好的播放时间
2. 每帧在 `HUD_Frame` 中检查延迟队列
3. 时间到达时，构造 `mstudioevent_s` 结构并调用 `HUD_StudioEvent`
4. 验证实体仍然有效（messagenum 匹配）
5. 播放后从延迟队列中移除

## 使用场景

### 场景 1: 防止声音重叠
```
设置: cl_studiosnd_anti_spam_same 1.0
效果: 同一声音至少间隔 1 秒才能再次播放
```

### 场景 2: 控制声音密度
```
设置: cl_studiosnd_anti_spam_diff 0.5
效果: 任意声音事件之间至少间隔 0.5 秒
```

### 场景 3: 延迟而非丢弃
```
设置: cl_studiosnd_anti_spam_delay 1
效果: 被阻止的声音会在允许的时间自动播放
```

### 场景 4: 调试模式
```
设置: cl_studiosnd_debug 1
效果: 控制台输出所有声音事件的处理状态
     [StudioEvents] Played xxx.wav
     [StudioEvents] Blocked xxx.wav
     [StudioEvents] Delayed xxx.wav
```

## 技术要点

### 1. 实体有效性验证
使用 `messagenum` 字段判断实体是否仍然有效，防止播放已删除实体的声音。

### 2. 内存管理
- 使用 `std::vector` 动态管理声音记录
- 自动清理过期记录，防止内存泄漏
- 在关卡切换时完全清空

### 3. 时间管理
- 使用 `gEngfuncs.GetClientTime()` 获取客户端时间
- 所有时间比较基于浮点数，精度足够

### 4. 字符串处理
- 使用 `strcpy` 复制声音名称（固定 64 字节缓冲区）
- 使用 `strcmp` 比较声音是否相同

## 潜在改进方向

1. **TODO 注释**：代码中提到可能需要改用 `cl_parsecount` 而不是 `messagenum` 来判断实体有效性
2. **缓冲区安全**：考虑使用 `strncpy` 代替 `strcpy` 以增强安全性
3. **配置范围检查**：可以为 CVAR 添加边界值验证
4. **性能优化**：对于大量声音事件，可以考虑使用哈希表优化查找
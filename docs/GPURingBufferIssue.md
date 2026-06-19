# GPU Ring Buffer Issue Notes

本文记录 `CPMBRingBuffer` 当前实现中已确认或高度可疑的 4 个问题。重点背景是：TriAPI、HUD rect draw 等路径通过 PMB ring buffer 上传临时顶点/索引数据；一旦 `Allocate()` 持续失败，调用方会跳过绘制，表现为 TriAPI / HUD 相关内容消失。

## 1. 保留 3 帧导致耗尽后无法自然恢复

### 现象

`ReleaseCompletedFrames()` 固定保留最近 3 个 frame record：

- `Plugins/Renderer/gl_ringbuffer.cpp:245`
- `Plugins/Renderer/gl_ringbuffer.cpp:247`

但 `Allocate()` 在任何等待或释放之前，先检查：

- `Plugins/Renderer/gl_ringbuffer.cpp:132`

```cpp
if (m_UsedSize + size > m_BufferSize)
{
    return false;
}
```

如果当前只剩 3 个 completed frames，且这 3 帧占用接近 buffer 上限，则下一帧第一次 `Allocate()` 会直接失败。因为本帧没有成功分配，`EndFrame()` 不会新增第 4 个 frame record；下一帧 `ReleaseCompletedFrames()` 仍然看到 `m_CompletedFrames.size() == 3`，不会释放任何旧帧。

### 影响

这会形成死锁式状态：

1. `m_CompletedFrames.size() == 3`
2. `m_UsedSize + size > m_BufferSize`
3. `Allocate()` 直接失败
4. 本帧没有新 frame record
5. 下一帧仍然无法释放旧 frame

最终表现为 ring buffer 耗尽后无法自然恢复。对 TriAPI 而言，`triapi_End()` 会打印 `g_TriAPIVertexBuffer full!` 或 `g_TriAPIIndexBuffer full!` 后清空并跳过绘制。

### 建议验证

在 `Allocate()` 失败日志中打印：

- `m_BufferName`
- `m_UsedSize`
- `m_BufferSize`
- `m_CurrFrameSize`
- `m_CompletedFrames.size()`
- `m_Head`
- `m_Tail`

如果消失后长期保持 `m_CompletedFrames.size() == 3` 且 `m_UsedSize` 接近 `m_BufferSize`，即可确认此问题。

## 2. frame 外 Allocate 会丢失释放账目

### 现象

`CPMBRingBuffer::Allocate()` 不知道当前是否处于 `BeginFrame()` 和 `EndFrame()` 之间。它只做两件事：

- 增加 `m_UsedSize`
- 增加 `m_CurrFrameSize`

对应位置：

- `Plugins/Renderer/gl_ringbuffer.cpp:154`
- `Plugins/Renderer/gl_ringbuffer.cpp:155`
- `Plugins/Renderer/gl_ringbuffer.cpp:189`
- `Plugins/Renderer/gl_ringbuffer.cpp:190`

如果某次分配发生在 `EndFrame()` 之后、下一次 `BeginFrame()` 之前，下一次 `BeginFrame()` 会直接清零 `m_CurrFrameSize`：

- `Plugins/Renderer/gl_ringbuffer.cpp:222`

但 `m_UsedSize` 不会回退，这部分分配也不会进入任何 `m_CompletedFrames` record。

### 影响

这部分空间会变成“已使用但没有 fence 可释放”的账目泄漏。重复发生后，`m_UsedSize` 会持续增长，最终触发 ring buffer full。

当前 renderer 中一个需要特别注意的窗口是 `R_RenderEndFrame()`：

- `Plugins/Renderer/gl_rmain.cpp:3060`

它先对内置 ring buffer 调用 `EndFrame()`，然后才调用 `OnRenderEndFrame()` callback：

- `Plugins/Renderer/gl_rmain.cpp:3064`
- `Plugins/Renderer/gl_rmain.cpp:3089`

如果任何 callback 在 `OnRenderEndFrame()` 中调用 TriAPI、HUD rect draw，或直接使用 `IMetaRenderer::CreatePMBRingBuffer()` 创建的 ring buffer 执行 `Allocate()`，都可能进入此路径。

### 建议验证

给 `CPMBRingBuffer` 增加 `m_InFrame` 调试状态：

- `BeginFrame()` 设置为 true
- `EndFrame()` 设置为 false
- `Allocate()` 中如果 `!m_InFrame`，打印警告和调用栈/调试组

如果出现 frame 外分配日志，即可确认该路径可达。

## 3. glFenceSync 失败时会永久保留 m_UsedSize

### 现象

`EndFrame()` 只有在 `glFenceSync()` 返回非空 fence 时，才把当前帧记录进 `m_CompletedFrames`：

- `Plugins/Renderer/gl_ringbuffer.cpp:230`
- `Plugins/Renderer/gl_ringbuffer.cpp:231`
- `Plugins/Renderer/gl_ringbuffer.cpp:233`

但无论 fence 是否创建成功，函数最后都会清零 `m_CurrFrameSize`：

- `Plugins/Renderer/gl_ringbuffer.cpp:239`

如果 `glFenceSync()` 返回 `nullptr`，本帧已经计入 `m_UsedSize` 的分配不会进入 frame record，后续也没有对应记录可释放。

### 影响

这是一个低频但严重的账目泄漏路径。正常驱动上不一定容易触发，但一旦触发，症状和 frame 外分配类似：`m_UsedSize` 增长，`m_CompletedFrames` 中没有对应记录。

### 建议验证

在 `EndFrame()` 中检测 `fence == nullptr` 并打印：

- `m_BufferName`
- `m_CurrFrameSize`
- `glGetError()`

修复时不能静默清零 `m_CurrFrameSize`；需要选择等待 GPU 后回退/重置，或进入明确的错误处理路径。

## 4. 析构时删除了 buffer target 而不是 buffer object

### 现象

`CPMBRingBuffer::~CPMBRingBuffer()` 删除 OpenGL buffer 时传入的是 `m_GLBufferTarget`：

- `Plugins/Renderer/gl_ringbuffer.cpp:93`
- `Plugins/Renderer/gl_ringbuffer.cpp:95`

```cpp
if (m_GLBufferTarget)
{
    GL_DeleteBuffer(m_GLBufferTarget);
    m_GLBufferTarget = 0;
}
```

`GL_DeleteBuffer()` 需要的是 buffer object handle，即 `m_hGLBufferObject`，不是 `GL_ARRAY_BUFFER` / `GL_ELEMENT_ARRAY_BUFFER` target enum。

### 影响

这不会解释单局游戏内逐帧耗尽的主路径，但会导致 shutdown / reload / map lifecycle 中 GL buffer object 没有被正确释放。反复重载 renderer 或地图时可能造成 GPU 资源泄漏。

### 建议验证

修复前后使用 OpenGL debug label 或外部 GPU 调试工具检查 `TriAPIVertexBuffer`、`TriAPIIndexBuffer`、HUD rect buffers 是否在 `Destroy()` 后释放。

## 相关调用点

TriAPI 在分配失败时会直接跳过绘制：

- `Plugins/Renderer/gl_rmain.cpp:1540`
- `Plugins/Renderer/gl_rmain.cpp:1550`

TriAPI buffer 的 frame 生命周期由 renderer frame 驱动：

- `Plugins/Renderer/gl_rmain.cpp:2993`
- `Plugins/Renderer/gl_rmain.cpp:3060`

HUD rect draw 使用同一套 `IPMBRingBuffer` 实现：

- `Plugins/Renderer/gl_hud.cpp:479`
- `Plugins/Renderer/gl_hud.cpp:633`
- `Plugins/Renderer/gl_hud.cpp:785`

## 修复优先级

1. 优先修复 `MIN_FRAMES_TO_KEEP == 3` 与 `Allocate()` 早返回形成的耗尽死锁。
2. 增加 frame 内分配断言/日志，确认是否存在 `EndFrame()` 之后的分配。
3. 处理 `glFenceSync()` 失败路径，不允许静默丢失 frame record。
4. 修复析构时的 `GL_DeleteBuffer(m_GLBufferTarget)` 参数错误。


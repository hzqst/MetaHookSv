# UtilThreadTask（PluginLibs/UtilThreadTask）源码级分析

## 概述
`PluginLibs/UtilThreadTask` 构建出一个独立的 `UtilThreadTask.dll`，通过 Valve/HLSDK 的 `CreateInterface` 机制对外暴露 `IUtilThreadTaskFactory`（接口版本 `UtilThreadTaskFactory_001`），用于创建一个“可跨线程投递、在调用方线程执行”的任务调度器 `IThreadedTaskScheduler`。

核心价值：给其它插件/模块提供一个轻量的“主线程任务队列”（也可理解为 deferred callback queue），支持按时间（`ShouldRun(time)`）决定何时执行，并提供简单的 FIFO/LIFO（`bQueueToBegin`）控制。

## 职责
- **对外暴露工厂接口**：`IUtilThreadTaskFactory::CreateThreadedTaskScheduler()` 用于创建调度器实例（`PluginLibs/UtilThreadTask/UtilThreadTask.cpp:10`）。
- **提供调度器实现**：`CThreadedTaskScheduler` 维护任务队列、支持任意线程 `QueueTask`、并在调用 `RunTask/RunTasks` 的线程中执行任务（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:7`）。
- **定义任务/调度器 ABI**：接口在 `include/Interface/IUtilThreadTask.h`（`IThreadedTask` / `IThreadedTaskScheduler` / `IUtilThreadTaskFactory`）。

## 架构
- **接口层（公共 ABI）**：`include/Interface/IUtilThreadTask.h`
  - `IThreadedTask`：`ShouldRun(time)` + `Run(time)` + `Destroy()`（`include/Interface/IUtilThreadTask.h:7`）。
  - `IThreadedTaskScheduler`：入队、执行、等待清空、销毁、线程判定（`include/Interface/IUtilThreadTask.h:17`）。
  - `IUtilThreadTaskFactory`：创建调度器（`include/Interface/IUtilThreadTask.h:53`）。
- **实现层（DLL 内部实现）**：
  - `CThreadedTaskScheduler`：真正的队列与执行逻辑（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:7`）。
  - `ThreadedTaskScheduler_CreateInstance()`：创建实现实例（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:112`）。
  - `CUtilThreadTaskFactory` + `EXPOSE_SINGLE_INTERFACE`：把工厂作为单例接口导出（`PluginLibs/UtilThreadTask/UtilThreadTask.cpp:10`）。
- **加载/使用层（调用方示例，非本目录但决定实际 workflow）**：
  - `Plugins/Renderer/UtilThreadTask.cpp` 使用 `Sys_LoadModule`/`Sys_GetFactory` 获取工厂并创建 `g_pGameThreadTaskScheduler`（`Plugins/Renderer/UtilThreadTask.cpp:19`）。
  - 每帧调用 `GameThreadTaskScheduler()->RunTasks(time, 0);` 驱动执行（`Plugins/Renderer/exportfuncs.cpp:1904`）。
  - 退出时调用 `WaitForAllTasksToComplete()` + `Destroy()` + `Sys_FreeModule()`（`Plugins/Renderer/UtilThreadTask.cpp:48`，`Plugins/Renderer/exportfuncs.cpp:1930`）。

## 核心实现与 Workflow

### 1) 创建与线程归属
- `CThreadedTaskScheduler` 构造时记录创建线程 `m_thread_id = std::this_thread::get_id()`（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:16`）。
- `IsCurrentThreadCreatorThread()` 用于判断调用方是否为创建线程（常用于“必须在主线程执行”的断言/分支）（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:106`）。

### 2) 投递任务（可跨线程）
- `QueueTask(IThreadedTask* pTask, bool bQueueToBegin)`：持有 `std::recursive_mutex`，将任务放入 `std::list` 头/尾（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:37`）。
  - `bQueueToBegin=true`：`push_front`（更像“高优先级”插队）。
  - 否则 `push_back`：常规 FIFO。

### 3) 取出并执行（在调用 Run 的线程执行）
- `GetTaskFromQueue(time)`：持锁遍历队列，找到第一个 `ShouldRun(time)==true` 的任务并从队列移除返回（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:47`）。
- `RunTask(time)`：
  1. `GetTaskFromQueue(time)` 取出可运行任务；
  2. 调用 `pTask->Run(time)`；
  3. 调用 `pTask->Destroy()` 释放对象（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:68`）。

关键语义：
- 调度器不直接 `delete pTask`，而是要求任务实现 `Destroy()`（接口层要求）——这与 Valve/HLSDK 的对象生命周期风格一致。
- `RunTask` 在执行 `pTask->Run` 时不持有队列锁（锁在 `GetTaskFromQueue` 内部结束后释放），有利于避免长时间阻塞其它线程 `QueueTask`。

### 4) 批量执行
- `RunTasks(time, maxTasks)` 循环调用 `RunTask`，直到队列中无可运行任务，或达到 `maxTasks` 限制（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:80`）。

### 5) 退出/清理
- 调度器析构：持锁，对队列中残留任务逐个 `Destroy()` 并 `clear()`（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:21`）。
- 调用方典型流程（以 Renderer 为例）：
  - Init：加载 DLL -> `CreateInterface("UtilThreadTaskFactory_001")` -> `CreateThreadedTaskScheduler()`（`Plugins/Renderer/UtilThreadTask.cpp:19`）。
  - Frame：每帧用当前 game time 驱动 `RunTasks(time, 0)`（`Plugins/Renderer/exportfuncs.cpp:1904`）。
  - Shutdown：`WaitForAllTasksToComplete()` -> `Destroy()` -> 卸载 DLL（`Plugins/Renderer/UtilThreadTask.cpp:48`）。

## 依赖
- **C++ 标准库**：`<list> <mutex> <thread>`（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:1`）。
- **公共接口/Valve Interface 体系**：
  - `include/Interface/IUtilThreadTask.h`（对外 ABI）。
  - `include/HLSDK/common/interface.h` + `include/HLSDK/common/interface.cpp`（`CreateInterface`/`EXPOSE_*`/`Sys_LoadModule`/`Sys_GetFactory` 等约定；该 cpp 被 vcxproj 编译进 DLL，`PluginLibs/UtilThreadTask/UtilThreadTask.vcxproj`）。
- **MetaHookSv**：`<metahook.h>`（`PluginLibs/UtilThreadTask/UtilThreadTask.cpp:4`），调用方也通过 MetaHook 的系统接口加载/卸载模块（示例见 `Plugins/Renderer/UtilThreadTask.cpp:21`）。

## 注意事项 / 已知问题（源码级）

### 1) `WaitForAllTasksToComplete()` 当前实现存在严重缺陷
位置：`PluginLibs/UtilThreadTask/ThreadedTask.cpp:93`
- `for (auto itor = m_tasks.begin(); itor != m_tasks.end(); )` 循环体内**没有递增/erase**，若队列非空将导致**死循环**。
- 循环中仅 `pTask->Run(FLT_MAX)`，随后 `m_tasks.clear()`：即使不死循环，也会导致任务对象**未调用 `Destroy()` 而泄漏**（与 `RunTask` 路径不一致）。
- 此函数在 Renderer 的 shutdown 路径会被调用（`Plugins/Renderer/UtilThreadTask.cpp:52`），因此只要退出时队列里还有任务，就可能触发 hang/泄漏。

### 2) `RunTasks(time, maxTasks)` 存在 off-by-one 风险
位置：`PluginLibs/UtilThreadTask/ThreadedTask.cpp:80`
- `nRunTask` 在成功执行后才递增，但限制条件在递增前判断：`(maxTasks > 0 && nRunTask >= maxTasks)`。
- 当 `maxTasks>0` 时，实际可能执行 `maxTasks + 1` 个任务。
- 目前调用方常传 `maxTasks=0`（无限制），因此该问题可能长期未暴露。

### 3) “ThreadTask” 名称易误导：它不是线程池/后台线程
- 调度器内部没有创建 worker thread，也没有 `condition_variable`。
- “跨线程”只体现在 `QueueTask` 的线程安全；任务执行永远发生在调用 `RunTask/RunTasks` 的线程。

### 4) 任务实现必须正确实现生命周期
- 调度器依赖 `IThreadedTask::Destroy()` 释放资源（`include/Interface/IUtilThreadTask.h:10`）。
- 若任务不是 heap 分配或 `Destroy()` 未 `delete this`，会造成泄漏/崩溃。

### 5) `time` 语义
- 调度器把 `time` 原样传递给 `ShouldRun/Run`；调用方应保证一致性（通常是 game time，例如 Renderer 使用 `gEngfuncs.GetAbsoluteTime()`，`Plugins/Renderer/exportfuncs.cpp:1902`）。
- `WaitForAllTasksToComplete()` 传 `FLT_MAX` 给 `Run`（`PluginLibs/UtilThreadTask/ThreadedTask.cpp:101`），如果任务逻辑依赖真实时间，可能产生副作用。

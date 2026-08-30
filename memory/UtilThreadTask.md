---
title: UtilThreadTask
type: note
permalink: metahooksv/util-thread-task
---

# UtilThreadTask (PluginLibs/UtilThreadTask) Source-Level Analysis

## Overview
`PluginLibs/UtilThreadTask` builds a standalone `UtilThreadTask.dll` that exposes `IUtilThreadTaskFactory` (interface version `UtilThreadTaskFactory_001`) through Valve/HLSDK's `CreateInterface` mechanism. It creates an `IThreadedTaskScheduler`: a task scheduler that can accept submissions across threads and runs tasks on the caller's thread.

Its core value is providing other plugins/modules with a lightweight "main-thread task queue" (also understood as a deferred callback queue), which decides when to run based on time (`ShouldRun(time)`) and offers simple FIFO/LIFO control through `bQueueToBegin`.

## Responsibilities
- **Exposes the factory interface**: `IUtilThreadTaskFactory::CreateThreadedTaskScheduler()` creates scheduler instances (`PluginLibs/UtilThreadTask/UtilThreadTask.cpp`).
- **Provides the scheduler implementation**: `CThreadedTaskScheduler` maintains the task queue, supports `QueueTask` from any thread, and runs tasks on the thread calling `RunTask/RunTasks` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
- **Defines the task/scheduler ABI**: Interfaces are in `include/Interface/IUtilThreadTask.h` (`IThreadedTask` / `IThreadedTaskScheduler` / `IUtilThreadTaskFactory`).

## Architecture
- **Interface layer (public ABI)**: `include/Interface/IUtilThreadTask.h`
  - `IThreadedTask`: `ShouldRun(time)` + `Run(time)` + `Destroy()` (`include/Interface/IUtilThreadTask.h`).
  - `IThreadedTaskScheduler`: queueing, execution, waiting for completion, destruction, and thread detection (`include/Interface/IUtilThreadTask.h`).
  - `IUtilThreadTaskFactory`: scheduler creation (`include/Interface/IUtilThreadTask.h`).
- **Implementation layer (inside the DLL)**:
  - `CThreadedTaskScheduler`: the actual queue and execution logic (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
  - `ThreadedTaskScheduler_CreateInstance()`: creates implementation instances (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
  - `CUtilThreadTaskFactory` + `EXPOSE_SINGLE_INTERFACE`: exports the factory as a singleton interface (`PluginLibs/UtilThreadTask/UtilThreadTask.cpp`).
- **Loading/use layer (caller example; outside this directory but determines the actual workflow)**:
  - `Plugins/Renderer/UtilThreadTask.cpp` uses `Sys_LoadModule`/`Sys_GetFactory` to obtain the factory and create `g_pGameThreadTaskScheduler` (`Plugins/Renderer/UtilThreadTask.cpp`).
  - Every frame, `GameThreadTaskScheduler()->RunTasks(time, 0);` drives execution (`Plugins/Renderer/exportfuncs.cpp`).
  - At shutdown, it calls `WaitForAllTasksToComplete()` + `Destroy()` + `Sys_FreeModule()` (`Plugins/Renderer/UtilThreadTask.cpp`, `Plugins/Renderer/exportfuncs.cpp`).

## Core Implementation and Workflow

### 1) Creation and Thread Ownership
- `CThreadedTaskScheduler` records its creation thread in the constructor: `m_thread_id = std::this_thread::get_id()` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
- `IsCurrentThreadCreatorThread()` determines whether the caller is the creator thread (commonly for assertions/branches requiring main-thread execution) (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).

### 2) Task Submission (Cross-Thread Safe)
- `QueueTask(IThreadedTask* pTask, bool bQueueToBegin)`: holds `std::recursive_mutex` and adds the task to the front/back of a `std::list` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
  - `bQueueToBegin=true`: `push_front` (similar to inserting at high priority).
  - Otherwise, `push_back`: regular FIFO.

### 3) Dequeue and Execution (Runs on the Thread Calling Run)
- `GetTaskFromQueue(time)`: traverses the queue while locked, removes, and returns the first task for which `ShouldRun(time)==true` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
- `RunTask(time)`:
  1. `GetTaskFromQueue(time)` retrieves a runnable task;
  2. Calls `pTask->Run(time)`;
  3. Calls `pTask->Destroy()` to release the object (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).

Key semantics:
- The scheduler does not directly `delete pTask`; it requires tasks to implement `Destroy()` (an interface requirement), consistent with Valve/HLSDK object-lifecycle conventions.
- `RunTask` does not hold the queue lock while executing `pTask->Run` (the lock is released after `GetTaskFromQueue`), helping avoid long blocking of `QueueTask` calls from other threads.

### 4) Batch Execution
- `RunTasks(time, maxTasks)` repeatedly calls `RunTask` until no runnable task remains or the `maxTasks` limit is reached (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).

### 5) Shutdown/Cleanup
- Scheduler destruction: while locked, calls `Destroy()` on each remaining queued task, then `clear()` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
- Typical caller flow (Renderer example):
  - Init: Load the DLL -> `CreateInterface("UtilThreadTaskFactory_001")` -> `CreateThreadedTaskScheduler()` (`Plugins/Renderer/UtilThreadTask.cpp`).
  - Frame: Every frame, use current game time to drive `RunTasks(time, 0)` (`Plugins/Renderer/exportfuncs.cpp`).
  - Shutdown: `WaitForAllTasksToComplete()` -> `Destroy()` -> unload the DLL (`Plugins/Renderer/UtilThreadTask.cpp`).

## Dependencies
- **C++ standard library**: `<list> <mutex> <thread>` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`).
- **Public interfaces/Valve interface system**:
  - `include/Interface/IUtilThreadTask.h` (public ABI).
  - `include/HLSDK/common/interface.h` + `include/HLSDK/common/interface.cpp` (conventions including `CreateInterface`/`EXPOSE_*`/`Sys_LoadModule`/`Sys_GetFactory`; the cpp is compiled into the DLL by the vcxproj, `PluginLibs/UtilThreadTask/UtilThreadTask.vcxproj`).
- **MetaHookSv**: `<metahook.h>` (`PluginLibs/UtilThreadTask/UtilThreadTask.cpp`); callers also load/unload the module through MetaHook system interfaces (see `Plugins/Renderer/UtilThreadTask.cpp`).

## Notes / Known Issues (Source-Level)

### 1) The Former `WaitForAllTasksToComplete()` Defect Is Fixed
Location: `PluginLibs/UtilThreadTask/ThreadedTask.cpp`
- The current implementation traverses queued tasks, calling `Run(FLT_MAX)`, then `Destroy()`, and finally `clear()`; it no longer has an infinite-loop/leak issue.
- Note that this function executes `Run` while holding the queue lock. Long-running tasks can block concurrent `QueueTask` calls (it is normally called only during shutdown, so the impact is acceptable).

### 2) The `RunTasks(time, maxTasks)` Off-by-One Risk Is Fixed
Location: `PluginLibs/UtilThreadTask/ThreadedTask.cpp`
- The current loop condition is `maxTasks <= 0 || nRunTask < maxTasks`, so when `maxTasks>0`, at most `maxTasks` tasks are executed.
- When `maxTasks<=0`, it preserves unlimited semantics, consistent with callers passing `0`.

### 3) The Name “ThreadTask” Can Mislead: It Is Not a Thread Pool/Background Thread
- The scheduler creates no worker thread and has no `condition_variable`.
- Its cross-thread nature only refers to `QueueTask` thread safety; tasks always execute on the thread calling `RunTask/RunTasks`.

### 4) Task Implementations Must Implement the Lifecycle Correctly
- The scheduler relies on `IThreadedTask::Destroy()` to release resources (`include/Interface/IUtilThreadTask.h`).
- If a task was not heap-allocated or `Destroy()` does not `delete this`, leaks/crashes can result.

### 5) `time` Semantics
- The scheduler forwards `time` unchanged to `ShouldRun/Run`; callers must ensure consistency (normally game time; for example, the Renderer uses `gEngfuncs.GetAbsoluteTime()` in `Plugins/Renderer/exportfuncs.cpp`).
- `WaitForAllTasksToComplete()` passes `FLT_MAX` to `Run` (`PluginLibs/UtilThreadTask/ThreadedTask.cpp`), which can have side effects if task logic depends on real time.

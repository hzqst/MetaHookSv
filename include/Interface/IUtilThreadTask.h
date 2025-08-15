#pragma once

#include <interface.h>
#include <stdint.h>


class IThreadedTask : public IBaseInterface
{
public:
	virtual void Destroy() = 0;

	virtual bool ShouldRun(float time) = 0;

	virtual void Run(float time) = 0;
};

class IThreadedTaskScheduler : public IBaseInterface
{
public:
	/*
		Call on game shutdown, to destry all tasks in queue
	*/
	virtual void Destroy() = 0;

	/*
		Call from anywhere
	*/
	virtual void QueueTask(IThreadedTask* pTask, bool bQueueToBegin = false) = 0;

	/*
		Run one task, return true if any task was executed, otherwise false.
	*/

	virtual bool RunTask(float time) = 0;

	/*
		Run all tasks in the queue
	*/

	virtual void RunTasks(float time, int maxTasks) = 0;

	/*
		Wait for all tasks to complete
	*/
	virtual void WaitForAllTasksToComplete() = 0;

	/*
		Check if we are the creator thread
	*/
	virtual bool IsCurrentThreadCreatorThread() const = 0;
};

class IUtilThreadTaskFactory : public IBaseInterface
{
public:
    virtual IThreadedTaskScheduler* CreateThreadedTaskScheduler() = 0;
};

IUtilThreadTaskFactory* UtilThreadTaskFactory();

#define UTIL_THREAD_TASK_FACTORY_INTERFACE_VERSION "UtilThreadTaskFactory_001"
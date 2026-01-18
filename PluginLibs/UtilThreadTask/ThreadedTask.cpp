#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include "ThreadedTask.h"

class CThreadedTaskScheduler : public IThreadedTaskScheduler
{
private:
	std::recursive_mutex m_mutex;
	std::list<IThreadedTask*>m_tasks;
	std::thread::id m_thread_id;

public:

	CThreadedTaskScheduler()
	{
		m_thread_id = std::this_thread::get_id();
	}

	~CThreadedTaskScheduler()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		for (auto pTask : m_tasks)
		{
			pTask->Destroy();
		}
		m_tasks.clear();
	}

	void Destroy() override
	{
		delete this;
	}

	void QueueTask(IThreadedTask* pTask, bool bQueueToBegin) override
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		if(bQueueToBegin)
			m_tasks.push_front(pTask);
		else
			m_tasks.push_back(pTask);
	}

	IThreadedTask *GetTaskFromQueue(float time)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		for (auto itor = m_tasks.begin(); itor != m_tasks.end(); )
		{
			auto pTask = (*itor);

			if (pTask->ShouldRun(time))
			{
				itor = m_tasks.erase(itor);

				return pTask;
			}

			itor++;
		}

		return nullptr;
	}

	bool RunTask(float time) override
	{
		auto pTask = GetTaskFromQueue(time);

		if (!pTask)
			return false;

		pTask->Run(time);
		pTask->Destroy();
		return true;
	}

	void RunTasks(float time, int maxTasks) override
	{
		int nRunTask = 0;

		while (1)
		{
			if (!RunTask(time) || (maxTasks > 0 && nRunTask >= maxTasks))
				return;

			nRunTask++;
		}
	}

	void WaitForAllTasksToComplete() override
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		for (auto& pTask : m_tasks)
		{
			pTask->Run(FLT_MAX);
			pTask->Destroy();
		}
		m_tasks.clear();
	}

	bool IsCurrentThreadCreatorThread() const override
	{
		return std::this_thread::get_id() == m_thread_id;
	}
};

IThreadedTaskScheduler* ThreadedTaskScheduler_CreateInstance()
{
	return new CThreadedTaskScheduler();
}

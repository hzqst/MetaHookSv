#pragma once

#include "LambdaThreadedTask.h"

class CLambdaThreadedTask : public IThreadedTask
{
private:
	float m_gametime{};

public:
	CLambdaThreadedTask(const std::function<void()>& callback) : m_callback(callback)
	{

	}
	CLambdaThreadedTask(const std::function<void()>& callback, float time) : m_callback(callback), m_gametime(time)
	{

	}

	void Destroy() override
	{
		delete this;
	}

	bool ShouldRun(float time) override
	{
        return time >= m_gametime;
	}

	void Run(float time) override
	{
		m_callback();
	}

private:
	std::function<void()> m_callback;
};

IThreadedTask* LambdaThreadedTask_CreateInstance(const std::function<void()>& callback)
{
	return new CLambdaThreadedTask(callback);
}

IThreadedTask* LambdaThreadedTask_CreateInstance(const std::function<void()>& callback, float time)
{
	return new CLambdaThreadedTask(callback, time);
}
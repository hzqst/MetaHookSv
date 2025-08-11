#pragma once

#include <functional>

#include <IUtilThreadTask.h>

/*

//Run immediately

g_GameThreadTaskScheduler->QueueTask(
	LambdaThreadedTask_CreateInstance([](){

	//Do whatever you want

	}),
	false
);

//Delay for 5 secs:

g_GameThreadTaskScheduler->QueueTask(
	LambdaThreadedTask_CreateInstance([](){

	//Do whatever you want

	}),
	GetGameTime() + 5.0
);

*/

IThreadedTask* LambdaThreadedTask_CreateInstance(const std::function<void()>& callback);
IThreadedTask* LambdaThreadedTask_CreateInstance(const std::function<void()>& callback, float time);
#include <metahook.h>
#include "plugins.h"
#include "UtilThreadTask.h"

static HINTERFACEMODULE g_hUtilThreadTask = NULL;
static IUtilThreadTaskFactory* g_pUtilThreadTaskFactory = NULL;
static IThreadedTaskScheduler* g_pGameThreadTaskScheduler = NULL;

IThreadedTaskScheduler* GameThreadTaskScheduler()
{
	return g_pGameThreadTaskScheduler;
}

IUtilThreadTaskFactory* UtilThreadTaskFactory()
{
	return g_pUtilThreadTaskFactory;
}

void UtilThreadTask_Init()
{
	g_hUtilThreadTask = Sys_LoadModule("UtilThreadTask.dll");

	if (!g_hUtilThreadTask)
	{
		Sys_Error("Could not load UtilThreadTask.dll");
		return;
	}

	auto factory = Sys_GetFactory(g_hUtilThreadTask);

	if (!factory)
	{
		Sys_Error("Could not get factory from UtilThreadTask.dll");
		return;
	}

	g_pUtilThreadTaskFactory = (decltype(g_pUtilThreadTaskFactory))factory(UTIL_THREAD_TASK_FACTORY_INTERFACE_VERSION, NULL);

	if (!g_pUtilThreadTaskFactory)
	{
		Sys_Error("Could not get UtilThreadTaskFactory from UtilThreadTask.dll");
		return;
	}

	g_pGameThreadTaskScheduler = UtilThreadTaskFactory()->CreateThreadedTaskScheduler();
}

void UtilThreadTask_Shutdown()
{
	if (g_pGameThreadTaskScheduler)
	{
		g_pGameThreadTaskScheduler->Destroy();
		g_pGameThreadTaskScheduler = nullptr;
	}

	if (g_pUtilThreadTaskFactory)
	{
		g_pUtilThreadTaskFactory = nullptr;
	}

	if (g_hUtilThreadTask)
	{
		Sys_FreeModule(g_hUtilThreadTask);
		g_hUtilThreadTask = NULL;
	}
}

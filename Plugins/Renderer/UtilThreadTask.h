#pragma once

#include <IUtilThreadTask.h>

void UtilThreadTask_Init();
void UtilThreadTask_Shutdown();

IUtilThreadTaskFactory* UtilThreadTaskFactory();
IThreadedTaskScheduler* GameThreadTaskScheduler();
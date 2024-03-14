#ifndef IENGINEAPI_H
#define IENGINEAPI_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

enum
{
	ENGINE_RESULT_NONE,
	ENGINE_RESULT_RESTART,
	ENGINE_RESULT_UNSUPPORTEDVIDEO
};

class IEngineAPI : public IBaseInterface
{
public:
	virtual int Run(HINSTANCE instance, char *basedir, const char *cmdline, char *szCommand, CreateInterfaceFn launcherFactory, CreateInterfaceFn filesystemFactory);
};

#define VENGINE_LAUNCHER_API_VERSION "VENGINE_LAUNCHER_API_VERSION002"

#endif //IENGINEAPI_H
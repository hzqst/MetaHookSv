#pragma once

#include <interface.h>
#include <IDpiManager.h>

class IDpiManagerInternal : public IDpiManager
{
public:
	virtual void Init() = 0;
	virtual void InitFromMainHwnd() = 0;
	virtual void PostInit() = 0;
	virtual void Shutdown() = 0;
};

IDpiManagerInternal* DpiManagerInternal();
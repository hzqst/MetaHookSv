#pragma once

#include <interface.h>

const int DpiScalingSource_System = 1;
const int DpiScalingSource_Window = 2;
const int DpiScalingSource_SDL2 = 3;

class IDpiManager : public IBaseInterface
{
public:
	virtual float GetDpiScaling() const = 0;
	virtual int GetDpiScalingSource() const = 0;
	virtual bool IsHighDpiSupportEnabled() const = 0;
};

IDpiManager* DpiManager();

#define DPI_MANAGER_INTERFACE_VERSION "DpiManager_API_001"
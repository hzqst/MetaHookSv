#pragma once

#include <interface.h>

const int DpiScalingSource_System = 1;
const int DpiScalingSource_Window = 2;
const int DpiScalingSource_SDL2 = 3;

class IDpiManager : public IBaseInterface
{
public:
	virtual void Init() = 0;
	virtual void InitFromMainHwnd() = 0;
	virtual void PostInit() = 0;
	virtual void Shutdown() = 0;
	virtual float GetDpiScaling() const = 0;
	virtual int GetDpiScalingSource() const = 0;
	virtual bool IsHighDpiSupportEnabled() const = 0;
};

IDpiManager* dpimanager();
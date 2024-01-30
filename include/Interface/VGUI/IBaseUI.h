#ifndef IBASEUI_H
#define IBASEUI_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

struct cl_enginefuncs_s;
class IBaseSystem;

namespace vgui2
{
class Panel;
}

class IBaseUI_Legacy : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn* factories, int count) = 0;//1
	virtual void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion) = 0;//2
	virtual void Shutdown(void) = 0;//3
	virtual int Key_Event(int down, int keynum, const char* pszCurrentBinding) = 0;//4
	virtual void CallEngineSurfaceWndProc(void* hwnd, unsigned int msg, unsigned int wparam, long lparam) = 0;//5
	virtual void Paint(int x, int y, int right, int bottom) = 0;//6
	virtual void HideGameUI(void) = 0;//7
	virtual void ActivateGameUI(void) = 0;//8
	virtual void HideConsole(void) = 0;//9
	virtual void ShowConsole(void) = 0;//10
};

class IBaseUI : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count) = 0;//1
	virtual void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion) = 0;//2
	virtual void Shutdown(void) = 0;//3
	virtual int Key_Event(int down, int keynum, const char *pszCurrentBinding) = 0;//4
	virtual void CallEngineSurfaceAppHandler(void* pevent, void* userData) = 0;//5
	virtual void Paint(int x, int y, int right, int bottom) = 0;//6
	virtual void HideGameUI(void) = 0;//7
	virtual void ActivateGameUI(void) = 0;//8
	virtual void HideConsole(void) = 0;//9
	virtual void ShowConsole(void) = 0;//10
};

#define BASEUI_INTERFACE_VERSION "BaseUI001"
#endif
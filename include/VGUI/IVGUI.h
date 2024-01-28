#ifndef IVGUI_H
#define IVGUI_H

#ifdef _WIN32
#pragma once
#endif

#include <interface.h>
#include <vgui/VGUI.h>

class KeyValues;

namespace vgui
{

typedef unsigned long HPanel;
typedef int HContext;

enum
{
	DEFAULT_VGUI_CONTEXT = ((HContext)~0)
};

typedef unsigned long HPanel;

#ifdef PostMessage
#undef PostMessage
#endif

class IVGui : public IBaseInterface
{
public:
	virtual bool Init(CreateInterfaceFn *factoryList, int numFactories) = 0;//1
	virtual void Shutdown(void) = 0;//2
	virtual void Start(void) = 0;//3
	virtual void Stop(void) = 0;//4
	virtual bool IsRunning(void) = 0;//5
	virtual void RunFrame(void) = 0;//6
	virtual void ShutdownMessage(unsigned int shutdownID) = 0;;//7
	virtual VPANEL AllocPanel(void) = 0;//8
	virtual void FreePanel(VPANEL panel) = 0;//9
	virtual void DPrintf(const char *format, ...) = 0;//10
	virtual void DPrintf2(const char *format, ...) = 0;//11
	virtual void SpewAllActivePanelNames(void) = 0;//12
	virtual HPanel PanelToHandle(VPANEL panel) = 0;//13
	virtual VPANEL HandleToPanel(HPanel index) = 0;//14
	virtual void MarkPanelForDeletion(VPANEL panel) = 0;//15
	virtual void AddTickSignal(VPANEL panel, int intervalMilliseconds = 0) = 0;//16
	virtual void RemoveTickSignal(VPANEL panel);//17
	virtual void PostMessage(VPANEL target, KeyValues *params, VPANEL from, float delaySeconds = 0.0) = 0;//18
	virtual HContext CreateContext(void) = 0;//19
	virtual void DestroyContext(HContext context) = 0;//20
	virtual void AssociatePanelWithContext(HContext context, VPANEL pRoot) = 0;//21
	virtual void ActivateContext(HContext context) = 0;//22
	virtual void SetSleep(bool state) = 0;//23
	virtual bool GetShouldVGuiControlSleep(void) = 0;//24
};
}

#define VGUI_IVGUI_INTERFACE_VERSION "VGUI_ivgui006"

#endif
#ifndef IVGUI_H
#define IVGUI_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
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

class IVGui : public IBaseInterface
{
public:
	virtual bool Init(CreateInterfaceFn *factoryList, int numFactories) = 0;
	virtual void Shutdown(void) = 0;
	virtual void Start(void) = 0;
	virtual void Stop(void) = 0;
	virtual bool IsRunning(void) = 0;
	virtual void RunFrame(void) = 0;
	virtual void ShutdownMessage(unsigned int shutdownID) = 0;
	virtual VPANEL AllocPanel(void) = 0;
	virtual void FreePanel(VPANEL panel) = 0;
	virtual void DPrintf(const char *format, ...) = 0;
	virtual void DPrintf2(const char *format, ...) = 0;
	virtual void SpewAllActivePanelNames(void) = 0;
	virtual HPanel PanelToHandle(VPANEL panel) = 0;
	virtual VPANEL HandleToPanel(HPanel index) = 0;
	virtual void MarkPanelForDeletion(VPANEL panel) = 0;
	virtual void AddTickSignal(VPANEL panel, int intervalMilliseconds = 0) = 0;
	virtual void RemoveTickSignal(VPANEL panel) = 0;
	virtual void PostMessage(VPANEL target, KeyValues *params, VPANEL from, float delaySeconds = 0.0) = 0;
	virtual HContext CreateContext(void) = 0;
	virtual void DestroyContext(HContext context) = 0;
	virtual void AssociatePanelWithContext(HContext context, VPANEL pRoot) = 0;
	virtual void ActivateContext(HContext context) = 0;
	virtual void SetSleep(bool state) = 0;
	virtual bool GetShouldVGuiControlSleep(void) = 0;
};
}

#define VGUI_IVGUI_INTERFACE_VERSION "VGUI_ivgui006"

#endif
#ifndef IGAMEUI_H
#define IGAMEUI_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "tier1/interface.h"

class IGameUI : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count) = 0;//1
	virtual void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, void *system) = 0;//2
	virtual void Shutdown(void) = 0;//3
	virtual int ActivateGameUI(void) = 0;//4
	virtual int ActivateDemoUI(void) = 0;//5
	virtual int HasExclusiveInput(void) = 0;//6
	virtual void RunFrame(void) = 0;//7
	virtual void ConnectToServer(const char *game, int IP, int port) = 0;//8
	virtual void DisconnectFromServer(void) = 0;//9
	virtual void HideGameUI(void) = 0;//10
	virtual bool IsGameUIActive(void) = 0;//11
	virtual void LoadingStarted(const char *resourceType, const char *resourceName) = 0;//12
	virtual void LoadingFinished(const char *resourceType, const char *resourceName) = 0;//13
	virtual void StartProgressBar(const char *progressType, int progressSteps) = 0;//14
	virtual int ContinueProgressBar(int progressPoint, float progressFraction) = 0;//15
	virtual void StopProgressBar(bool bError, const char *failureReason, const char *extendedReason = NULL) = 0;//16
	virtual int SetProgressBarStatusText(const char *statusText) = 0;//17
	virtual void SetSecondaryProgressBar(float progress) = 0;//18
	virtual void SetSecondaryProgressBarText(const char *statusText) = 0;//19
};

#define GAMEUI_INTERFACE_VERSION "GameUI007"

#endif
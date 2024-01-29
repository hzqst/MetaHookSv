#ifndef ICLIENTVGUI_H
#define ICLIENTVGUI_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"

class IClientVGUI : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn *factories, int count) = 0;//1
	virtual void Start(void) = 0;//2
	virtual void SetParent(vgui::VPANEL parent) = 0;//3
	virtual bool UseVGUI1(void) = 0;//4
	virtual void HideScoreBoard(void) = 0;//5
	virtual void HideAllVGUIMenu(void) = 0;//6
	virtual void ActivateClientUI(void) = 0;//7
	virtual void HideClientUI(void) = 0;//8
	virtual void Unknown(void) = 0;
	virtual void Shutdown(void) = 0;//This will only be called for cstrike and czero
};

#define CLIENTVGUI_INTERFACE_VERSION "VClientVGUI001"

#endif
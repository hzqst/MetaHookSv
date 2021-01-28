#ifndef IENGINEVGUI_H
#define IENGINEVGUI_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "interface.h"

namespace vgui
{

enum VGUIPANEL
{
	PANEL_ROOT = 0,
	PANEL_CLIENTDLL,
	PANEL_GAMEUIDLL
};

class IEngineVGui : public IBaseInterface
{
public:
	virtual VPANEL GetPanel(VGUIPANEL type) = 0;
};
}

#define VENGINE_VGUI_VERSION "VEngineVGui001"

#endif
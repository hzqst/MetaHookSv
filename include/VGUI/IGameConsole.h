#ifndef IGAMECONSOLE_GS_H
#define IGAMECONSOLE_GS_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class Color;

class IGameConsole : public IBaseInterface
{
public:
	virtual void Activate(void) = 0;
	virtual void Initialize(void) = 0;
	virtual void Hide(void) = 0;
	virtual void Clear(void) = 0;
	virtual bool IsConsoleVisible(void) = 0;
	virtual void Printf(const char *format, ...) = 0;
	virtual void DPrintf(const char *format, ...) = 0;
	//virtual void ColorPrintf(Color &clr) = 0;	// Doesn't exist on GS
	virtual void SetParent(int parent) = 0;
};

#define GAMECONSOLE_INTERFACE_VERSION_GS "GameConsole003"
#endif
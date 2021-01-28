#ifndef IBORDER_H
#define IBORDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

class KeyValues;

namespace vgui
{

class IScheme;

class IBorder
{
public:
	enum sides_e
	{
		SIDE_LEFT = 0,
		SIDE_TOP = 1,
		SIDE_RIGHT = 2,
		SIDE_BOTTOM = 3
	};

public:
	virtual void Paint(VPANEL panel) = 0;
	virtual void Paint(int x0, int y0, int x1, int y1) = 0;
	virtual void Paint(int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop) = 0;
	virtual void SetInset(int left, int top, int right, int bottom) = 0;
	virtual void GetInset(int &left, int &top, int &right, int &bottom) = 0;
	virtual void AddLine(sides_e side, Color color, int startOffset, int endOffset) = 0;
	virtual void ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData) = 0;
	virtual void ParseSideSettings(int side_index, KeyValues *inResourceData, IScheme *pScheme) = 0;
	virtual const char *GetName(void) = 0;
	virtual void SetName(const char *name) = 0;
};
}

#endif
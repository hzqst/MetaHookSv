//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IBORDER_H
#define IBORDER_H

#ifdef _WIN32
#pragma once
#endif

#include "VGUI.h"

class Color;
class KeyValues;

namespace vgui
{

class IScheme;

//-----------------------------------------------------------------------------
// Purpose: Interface to panel borders
//			Borders have a close relationship with panels
//			They are the edges of the panel.
//-----------------------------------------------------------------------------
class IBorder
{
public:
	enum backgroundtype_e
	{
		BACKGROUND_FILLED,
		BACKGROUND_TEXTURED,
		BACKGROUND_ROUNDEDCORNERS,
	};

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
	virtual void ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData) = 0;
	virtual void ParseSideSettings(int side_index, KeyValues *inResourceData, IScheme *pScheme) = 0;
	virtual const char *GetName(void) = 0;
	virtual void SetName(const char *name) = 0;
	virtual backgroundtype_e GetBackgroundType(void) = 0;
	virtual bool PaintFirst(void) = 0;
};

} // namespace vgui

#endif // IBORDER_H

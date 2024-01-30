//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISCHEME_H
#define ISCHEME_H

#ifdef _WIN32
#pragma once
#endif

#include "VGUI.h"
#include <interface.h>

class Color;

namespace vgui
{

typedef unsigned long HScheme;
typedef unsigned long HTexture;

class IScheme;
class IBorder;
class IImage;

//-----------------------------------------------------------------------------
// Purpose: Holds all panel rendering data
//			This functionality is all wrapped in the Panel::GetScheme*() functions
//-----------------------------------------------------------------------------

class IScheme : public IBaseInterface
{
public:
	// gets a string from the default settings section
	virtual const char *GetResourceString(const char *stringName) = 0;

	// returns a pointer to an existing border
	virtual IBorder *GetBorder(const char *borderName) = 0;

	//  returns a pointer to an existing font
	virtual HFont GetFont(const char *fontName, bool proportional = false) = 0;

	// colors
	virtual Color GetColor(const char *colorName, Color defaultColor) = 0;

//public:
	//const char *GetName(void);
	//char const *GetFontName(const HFont &font);
};

class IScheme_HL25 : public IScheme
{
public:
	virtual HFont GetHDFont(const char* fontName, bool proportional, bool hd) = 0;
};

class ISchemeManager : public IBaseInterface
{
public:
	virtual HScheme LoadSchemeFromFile(const char *fileName, const char *tag) = 0;
	virtual void ReloadSchemes(void) = 0;
	virtual HScheme GetDefaultScheme(void) = 0;
	virtual HScheme GetScheme(const char *tag) = 0;
	virtual IImage *GetImage(const char *imageName, bool hardwareFiltered) = 0;
	virtual HTexture GetImageID(const char *imageName, bool hardwareFiltered) = 0;
	virtual IScheme *GetIScheme(HScheme scheme) = 0;
	virtual void Shutdown(bool full = true) = 0;
	virtual int GetProportionalScaledValue(int normalizedValue) = 0;
	virtual int GetProportionalNormalizedValue(int scaledValue) = 0;
};

class ISchemeManager_HL25 : public ISchemeManager
{
public:
	virtual float GetProportionalScale(void) = 0;
	virtual int GetHDProportionalScaledValue(int normalizedValue) = 0;
	virtual int GetHDProportionalNormalizedValue(int scaledValue) = 0;
};

#define VGUI_SCHEME_INTERFACE_VERSION "VGUI_Scheme009"

} // namespace vgui

#endif // ISCHEME_H
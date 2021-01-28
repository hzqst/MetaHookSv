#ifndef ISCHEME_H
#define ISCHEME_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "interface.h"

class Color;

namespace vgui
{

typedef unsigned long HScheme;
typedef unsigned long HTexture;

class IBorder;
class IImage;

class IScheme : public IBaseInterface
{
public:
	virtual const char *GetResourceString(const char *stringName) = 0;
	virtual IBorder *GetBorder(const char *borderName) = 0;
	virtual HFont GetFont(const char *fontName, bool proportional = false) = 0;
	virtual Color GetColor(const char *colorName, Color defaultColor) = 0;
};

class ISchemeManager: public IBaseInterface
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
}

#define VGUI_SCHEME_INTERFACE_VERSION "VGUI_Scheme009"

#endif
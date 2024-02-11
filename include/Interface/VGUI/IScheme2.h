//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISCHEME2_H
#define ISCHEME2_H

#ifdef _WIN32
#pragma once
#endif

#include "VGUI.h"
#include <IScheme.h>
#include <KeyValues.h>

namespace vgui
{
class IScheme2 : public IScheme_HL25
{
public:
	virtual void Shutdown(bool full) = 0;
	virtual void LoadFromFile(VPANEL sizingPanel, const char* filename, const char* tag, KeyValues* inKeys) = 0;
	virtual const char* GetName(void) = 0;
	virtual const char* GetFileName(void) = 0;
	virtual char const* GetFontName(const HFont& font) = 0;
	virtual void ReloadFontGlyphs(void) = 0;
	virtual VPANEL GetSizingPanel(void) = 0;
};

class ISchemeManager2 : public ISchemeManager_HL25
{
public:
	//LD
	virtual int GetProportionalScaledValueEx(HScheme scheme, int normalizedValue) = 0;
	virtual int GetProportionalNormalizedValueEx(HScheme scheme, int scaledValue) = 0;
	virtual int GetProportionalScaledValueEx(IScheme2* pScheme, int normalizedValue) = 0;
	virtual int GetProportionalNormalizedValueEx(IScheme2* pScheme, int scaledValue) = 0;
	//HD
	virtual int GetHDProportionalScaledValueEx(HScheme scheme, int normalizedValue) = 0;
	virtual int GetHDProportionalNormalizedValueEx(HScheme scheme, int scaledValue) = 0;
	virtual int GetHDProportionalScaledValueEx(IScheme2* pScheme, int normalizedValue) = 0;
	virtual int GetHDProportionalNormalizedValueEx(IScheme2* pScheme, int scaledValue) = 0;

	virtual HScheme LoadSchemeFromFileEx(VPANEL sizingPanel, const char* fileName, const char* tag) = 0;

	//For horizontal protportional
	virtual float GetHorizontalProportionalScale(void) = 0;
	virtual int GetHorizontalProportionalScaledValue(int normalizedValue) = 0;
	virtual int GetHorizontalProportionalNormalizedValue(int scaledValue) = 0;
	virtual int GetHDHorizontalProportionalScaledValue(int normalizedValue) = 0;
	virtual int GetHDHorizontalProportionalNormalizedValue(int scaledValue) = 0;

	//For altered protportional from CounterStrike client
	virtual float GetAlteredProportionalScale(void) = 0;
	virtual int GetAlteredProportionalScaledValue(int normalizedValue) = 0;
	virtual int GetAlteredProportionalNormalizedValue(int scaledValue) = 0;
	virtual int GetHDAlteredProportionalScaledValue(int normalizedValue) = 0;
	virtual int GetHDAlteredProportionalNormalizedValue(int scaledValue) = 0;
};

#define VGUI_SCHEME2_INTERFACE_VERSION  "VGUI_Scheme2_001"

} // namespace vgui

#endif // ISCHEME2_H
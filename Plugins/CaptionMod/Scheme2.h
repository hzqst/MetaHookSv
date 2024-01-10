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
#include <interface.h>
#include <utlrbtree.h>
#include <utlsymbol.h>

#include "Border.h"
#include "Color.h"

namespace vgui
{

class Bitmap;

typedef unsigned long HScheme;
typedef unsigned long HTexture;

class IScheme;
class IScheme_HL25;
class IBorder;
class IImage;

//Simple wrapper around original g_SchemeManager and g_SchemeManager_HL25

class CScheme : public IScheme_HL25
{
public:
	CScheme(void);

public:
	const char* GetResourceString(const char* stringName) override;
	IBorder* GetBorder(const char* borderName) override;
	HFont GetFont(const char* fontName, bool proportional) override;
	Color GetColor(const char* colorName, Color defaultColor) override;
	HFont GetHDFont(const char* fontName, bool proportional, bool hd) override;

public:
	void Shutdown(bool full);
	void LoadFromFile(VPANEL sizingPanel, const char* filename, const char* tag, KeyValues* inKeys);
	const char* GetName(void) { return tag; }
	const char* GetFileName(void) { return fileName; }
	char const* GetFontName(const HFont& font);
	void ReloadFontGlyphs(void);
	VPANEL GetSizingPanel(void) { return m_SizingPanel; }

private:
	const char* LookupSchemeSetting(const char* pchSetting);
	const char* GetMungedFontName(const char* fontName, const char* scheme, bool proportional);
	const char* GetMungedHDFontName(const char* fontName, const char* scheme, bool proportional, bool hd);
	void LoadFonts(void);
	void LoadBorders(void);
	HFont FindFontInAliasList(const char* fontName);
	int GetMinimumFontHeightForCurrentLanguage(void);

private:
	char fileName[256];
	char tag[64];

	KeyValues* m_pData;
	KeyValues* m_pkvBaseSettings;
	KeyValues* m_pkvColors;

	struct SchemeBorder_t
	{
		Border* border;
		int borderSymbol;
		bool bSharedBorder;
	};

	CUtlVector<SchemeBorder_t> m_BorderList;
	IBorder* m_pBaseBorder;
	KeyValues* m_pkvBorders;

#pragma pack(1)
	struct fontalias_t
	{
		CUtlSymbol _fontName;
		CUtlSymbol _trueFontName;
		unsigned short _font : 15;
		unsigned short m_bProportional : 1;
		unsigned short m_bHD : 1;
	};
#pragma pack()

	friend fontalias_t;

	CUtlVector<fontalias_t>	m_FontAliases;
	VPANEL m_SizingPanel;
	int m_nScreenWide;
	int m_nScreenTall;
};

class CSchemeManager : public ISchemeManager_HL25
{
public:
	CSchemeManager(void);
	~CSchemeManager(void);

public:
	HScheme LoadSchemeFromFile(const char* fileName, const char* tag) override;
	void ReloadSchemes(void) override;
	HScheme GetDefaultScheme(void) override;
	HScheme GetScheme(const char* tag) override;
	IImage* GetImage(const char* imageName, bool hardwareFiltered) override;
	HTexture GetImageID(const char* imageName, bool hardwareFiltered) override;
	IScheme* GetIScheme(HScheme scheme) override;
	void Shutdown(bool full = true) override;
	int GetProportionalScaledValue(int normalizedValue) override;
	int GetProportionalNormalizedValue(int scaledValue) override;

	//Added in HL25
	float GetProportionalScale(void) override;
	int GetHDProportionalScaledValue(int normalizedValue) override;
	int GetHDProportionalNormalizedValue(int scaledValue) override;

public:
	//LD
	int GetProportionalScaledValueEx(HScheme scheme, int normalizedValue);
	int GetProportionalNormalizedValueEx(HScheme scheme, int scaledValue);
	int GetProportionalScaledValueEx(CScheme* pScheme, int normalizedValue);
	int GetProportionalNormalizedValueEx(CScheme* pScheme, int scaledValue);
	//HD
	int GetHDProportionalScaledValueEx(HScheme scheme, int normalizedValue);
	int GetHDProportionalNormalizedValueEx(HScheme scheme, int scaledValue);
	int GetHDProportionalScaledValueEx(CScheme* pScheme, int normalizedValue);
	int GetHDProportionalNormalizedValueEx(CScheme* pScheme, int scaledValue);

	HScheme LoadSchemeFromFileEx(VPANEL sizingPanel, const char* fileName, const char* tag);

private:
	void ReloadFonts(void);
	int GetProportionalScaledValue_LD(int rootWide, int rootTall, int normalizedValue);
	int GetProportionalNormalizedValue_LD(int rootWide, int rootTall, int scaledValue);
	int GetProportionalScaledValue_HD(int rootWide, int rootTall, int normalizedValue);
	int GetProportionalNormalizedValue_HD(int rootWide, int rootTall, int scaledValue);
	HScheme FindLoadedScheme(const char* fileName);
	void DeleteImage(const char* pImageName);

private:
	CUtlVector<CScheme*> m_Schemes;
	static const char* s_pszSearchString;

	struct CachedBitmapHandle_t
	{
		Bitmap* bitmap;
	};

	static bool BitmapHandleSearchFunc(const CachedBitmapHandle_t&, const CachedBitmapHandle_t&);

	CUtlRBTree<CachedBitmapHandle_t, int> m_Bitmaps;
};

} // namespace vgui

#endif // ISCHEME_H
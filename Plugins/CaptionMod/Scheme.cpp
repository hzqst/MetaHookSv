#include <metahook.h>
#include <VGUI/ISurface.h>
#include <VGUI/IScheme.h>
#include <VGUI/ISystem.h>
#include <vgui_controls/Controls.h>
#include "vgui_internal.h"

#include <UtlVector.h>
#include <UtlRBTree.h>
#include <UtlSymbol.h>

#include "Color.h"
#include "Border.h"
#include "Bitmap.h"
#include "KeyValues.h"

//language
#include "engfuncs.h"

using namespace vgui;

#define FONT_ALIAS_NAME_LENGTH 64

class CScheme : public IScheme
{
public:
	CScheme(void);

public:
	virtual const char *GetResourceString(const char *stringName);
	virtual IBorder *GetBorder(const char *borderName);
	virtual HFont GetFont(const char *fontName, bool proportional);
	virtual Color GetColor(const char *colorName, Color defaultColor);

public:
	void Shutdown(bool full);
	void LoadFromFile(VPANEL sizingPanel, const char *filename, const char *tag, KeyValues *inKeys);
	const char *GetName(void) { return tag; }
	const char *GetFileName(void) { return fileName; }
	char const *GetFontName(const HFont &font);
	void ReloadFontGlyphs(void);
	VPANEL GetSizingPanel(void) { return m_SizingPanel; }

private:
	const char *LookupSchemeSetting(const char *pchSetting);
	const char *GetMungedFontName(const char *fontName, const char *scheme, bool proportional);
	void LoadFonts(void);
	void LoadBorders(void);
	HFont FindFontInAliasList(const char *fontName);
	int GetMinimumFontHeightForCurrentLanguage(void);

private:
	char fileName[256];
	char tag[64];

	KeyValues *m_pData;
	KeyValues *m_pkvBaseSettings;
	KeyValues *m_pkvColors;

	struct SchemeBorder_t
	{
		Border *border;
		int borderSymbol;
		bool bSharedBorder;
	};

	CUtlVector<SchemeBorder_t> m_BorderList;
	IBorder *m_pBaseBorder;
	KeyValues *m_pkvBorders;

#pragma pack(1)
	struct fontalias_t
	{
		CUtlSymbol _fontName;
		CUtlSymbol _trueFontName;
		unsigned short _font : 15;
		unsigned short m_bProportional : 1;
	};
#pragma pack()

	friend fontalias_t;

	CUtlVector<fontalias_t>	m_FontAliases;
	VPANEL m_SizingPanel;
	int m_nScreenWide;
	int m_nScreenTall;
};

class CSchemeManager : public ISchemeManager
{
public:
	CSchemeManager(void);
	~CSchemeManager(void);

public:
	virtual HScheme LoadSchemeFromFile(const char *fileName, const char *tag);
	virtual void ReloadSchemes(void);
	virtual HScheme GetDefaultScheme(void);
	virtual HScheme GetScheme(const char *tag);
	virtual IImage *GetImage(const char *imageName, bool hardwareFiltered);
	virtual HTexture GetImageID(const char *imageName, bool hardwareFiltered);
	virtual IScheme *GetIScheme(HScheme scheme);
	virtual void Shutdown(bool full = true);
	virtual int GetProportionalScaledValue(int normalizedValue);
	virtual int GetProportionalNormalizedValue(int scaledValue);

public:
	int GetProportionalScaledValueEx(HScheme scheme, int normalizedValue);
	int GetProportionalNormalizedValueEx(HScheme scheme, int scaledValue);
	int GetProportionalScaledValueEx(CScheme *pScheme, int normalizedValue);
	int GetProportionalNormalizedValueEx(CScheme *pScheme, int scaledValue);
	HScheme LoadSchemeFromFileEx(VPANEL sizingPanel, const char *fileName, const char *tag);

private:
	void ReloadFonts(void);
	int GetProportionalScaledValue_(int rootWide, int rootTall, int normalizedValue);
	int GetProportionalNormalizedValue_(int rootWide, int rootTall, int scaledValue);
	HScheme FindLoadedScheme(const char *fileName);
	void DeleteImage(const char *pImageName);

private:
	CUtlVector<CScheme *> m_Schemes;
	static const char *s_pszSearchString;

	struct CachedBitmapHandle_t
	{
		Bitmap *bitmap;
	};

	static bool BitmapHandleSearchFunc(const CachedBitmapHandle_t &, const CachedBitmapHandle_t &);

	CUtlRBTree<CachedBitmapHandle_t, int> m_Bitmaps;
};

const char *CSchemeManager::s_pszSearchString = NULL;

bool CSchemeManager::BitmapHandleSearchFunc(const CachedBitmapHandle_t &lhs, const CachedBitmapHandle_t &rhs)
{
	if (lhs.bitmap && rhs.bitmap)
		return stricmp(lhs.bitmap->GetName(), rhs.bitmap->GetName()) > 0;
	else if (lhs.bitmap)
		return stricmp(lhs.bitmap->GetName(), s_pszSearchString) > 0;

	return stricmp(s_pszSearchString, rhs.bitmap->GetName()) > 0;
}

CSchemeManager g_Scheme;

vgui::ISchemeManager *g_pVGuiSchemeManager = (ISchemeManager *)&g_Scheme;

CSchemeManager::CSchemeManager(void)
{
	CScheme *nullScheme = new CScheme();
	m_Schemes.AddToTail(nullScheme);
	m_Bitmaps.SetLessFunc(&BitmapHandleSearchFunc);
}

CSchemeManager::~CSchemeManager()
{
	int i;

	for (i = 0; i < m_Schemes.Count(); i++)
	{
		delete m_Schemes[i];
	}

	m_Schemes.RemoveAll();

	for (i = 0; i < m_Bitmaps.MaxElement(); i++)
	{
		if (m_Bitmaps.IsValidIndex(i))
		{
			delete m_Bitmaps[i].bitmap;
		}
	}

	m_Bitmaps.RemoveAll();

	Shutdown(false);
}

void CSchemeManager::ReloadFonts(void)
{
	for (int i = 1; i < m_Schemes.Count(); i++)
	{
		m_Schemes[i]->ReloadFontGlyphs();
	}
}

IScheme *CSchemeManager::GetIScheme(HScheme scheme)
{
	if (scheme >= (unsigned long)m_Schemes.Count())
	{
		AssertOnce(!"Invalid scheme requested.");
		return NULL;
	}
	else
	{
		return m_Schemes[scheme];
	}
}

void CSchemeManager::Shutdown(bool full)
{
	for (int i = full ? 0 : 1; i < m_Schemes.Count(); i++)
		m_Schemes[i]->Shutdown(full);

	if (full)
		m_Schemes.RemoveAll();
}

HScheme CSchemeManager::FindLoadedScheme(const char *fileName)
{
	for (int i = 1; i < m_Schemes.Count(); i++)
	{
		char const *schemeFileName = m_Schemes[i]->GetFileName();

		if (!stricmp(schemeFileName, fileName))
			return i;
	}

	return 0;
}

CScheme::CScheme(void)
{
	fileName[0] = 0;
	tag[0] = 0;

	m_pData = NULL;
	m_pkvBaseSettings = NULL;
	m_pkvColors = NULL;

	m_pBaseBorder = NULL;
	m_pkvBorders = NULL;
	m_SizingPanel = 0;
	m_nScreenWide = -1;
	m_nScreenTall = -1;
}

HScheme CSchemeManager::LoadSchemeFromFileEx(VPANEL sizingPanel, const char *fileName, const char *tag)
{
	HScheme hScheme = FindLoadedScheme(fileName);

	if (hScheme != 0)
	{
		CScheme *pScheme = static_cast<CScheme *>(GetIScheme(hScheme));

		if (IsPC() && pScheme)
			pScheme->ReloadFontGlyphs();

		return hScheme;
	}

	KeyValues *data;
	data = new KeyValues("Scheme");
	data->UsesEscapeSequences(true);

	bool result = data->LoadFromFile(g_pFullFileSystem, fileName, "GAME");

	if (!result)
	{
		result = data->LoadFromFile(g_pFullFileSystem, fileName, NULL);
	}

	if (!result)
	{
		data->deleteThis();
		return 0;
	}

	CScheme *newScheme = new CScheme();
	newScheme->LoadFromFile(sizingPanel, fileName, tag, data);

	return m_Schemes.AddToTail(newScheme);
}

HScheme CSchemeManager::LoadSchemeFromFile(const char *fileName, const char *tag)
{
	return LoadSchemeFromFileEx(0, fileName, tag);
}

struct SchemeEntryTranslation_t
{
	const char *pchNewEntry;
	const char *pchOldEntry;
	const char *pchDefaultValue;
};

SchemeEntryTranslation_t g_SchemeTranslation[] =
{
	{ "Border.Bright", "BorderBright", "200 200 200 196" },
	{ "Border.Dark" "BorderDark", "40 40 40 196" },
	{ "Border.Selection" "BorderSelection",	"0 0 0 196" },

	{ "Button.TextColor", "ControlFG", "White" },
	{ "Button.BgColor", "ControlBG", "Blank" },
	{ "Button.ArmedTextColor", "ControlFG" },
	{ "Button.ArmedBgColor", "ControlBG" },
	{ "Button.DepressedTextColor", "ControlFG" },
	{ "Button.DepressedBgColor", "ControlBG" },
	{ "Button.FocusBorderColor", "0 0 0 255" },

	{ "CheckButton.TextColor", "BaseText" },
	{ "CheckButton.SelectedTextColor", "BrightControlText" },
	{ "CheckButton.BgColor", "CheckBgColor" },
	{ "CheckButton.Border1", "CheckButtonBorder1" },
	{ "CheckButton.Border2", "CheckButtonBorder2" },
	{ "CheckButton.Check", "CheckButtonCheck" },

	{ "ComboBoxButton.ArrowColor", "LabelDimText" },
	{ "ComboBoxButton.ArmedArrowColor", "MenuButton/ArmedArrowColor" },
	{ "ComboBoxButton.BgColor", "MenuButton/ButtonBgColor" },
	{ "ComboBoxButton.DisabledBgColor", "ControlBG" },

	{ "Frame.TitleTextInsetX", NULL, "32" },
	{ "Frame.ClientInsetX", NULL, "8" },
	{ "Frame.ClientInsetY", NULL, "6" },
	{ "Frame.BgColor", "BgColor" },
	{ "Frame.OutOfFocusBgColor", "BgColor" },
	{ "Frame.FocusTransitionEffectTime", NULL, "0" },
	{ "Frame.TransitionEffectTime", NULL, "0" },
	{ "Frame.AutoSnapRange", NULL, "8" },
	{ "FrameGrip.Color1", "BorderBright" },
	{ "FrameGrip.Color2", "BorderSelection" },
	{ "FrameTitleButton.FgColor", "TitleButtonFgColor" },
	{ "FrameTitleButton.BgColor", "TitleButtonBgColor" },
	{ "FrameTitleButton.DisabledFgColor", "TitleButtonDisabledFgColor" },
	{ "FrameTitleButton.DisabledBgColor", "TitleButtonDisabledBgColor" },
	{ "FrameSystemButton.FgColor", "TitleBarBgColor" },
	{ "FrameSystemButton.BgColor", "TitleBarBgColor" },
	{ "FrameSystemButton.Icon", "TitleBarIcon" },
	{ "FrameSystemButton.DisabledIcon", "TitleBarDisabledIcon" },
	{ "FrameTitleBar.Font", NULL, "Default" },
	{ "FrameTitleBar.TextColor", "TitleBarFgColor" },
	{ "FrameTitleBar.BgColor", "TitleBarBgColor" },
	{ "FrameTitleBar.DisabledTextColor", "TitleBarDisabledFgColor" },
	{ "FrameTitleBar.DisabledBgColor", "TitleBarDisabledBgColor" },

	{ "GraphPanel.FgColor", "BrightControlText" },
	{ "GraphPanel.BgColor", "WindowBgColor" },

	{ "Label.TextDullColor", "LabelDimText" },
	{ "Label.TextColor", "BaseText" },
	{ "Label.TextBrightColor", "BrightControlText" },
	{ "Label.SelectedTextColor", "BrightControlText" },
	{ "Label.BgColor", "LabelBgColor" },
	{ "Label.DisabledFgColor1", "DisabledFgColor1" },
	{ "Label.DisabledFgColor2", "DisabledFgColor2" },

	{ "ListPanel.TextColor", "WindowFgColor" },
	{ "ListPanel.TextBgColor", "Menu/ArmedBgColor" },
	{ "ListPanel.BgColor", "ListBgColor" },
	{ "ListPanel.SelectedTextColor", "ListSelectionFgColor" },
	{ "ListPanel.SelectedBgColor", "Menu/ArmedBgColor" },
	{ "ListPanel.SelectedOutOfFocusBgColor", "SelectionBG2" },
	{ "ListPanel.EmptyListInfoTextColor", "LabelDimText" },
	{ "ListPanel.DisabledTextColor", "LabelDimText" },
	{ "ListPanel.DisabledSelectedTextColor", "ListBgColor" },

	{ "Menu.TextColor", "Menu/FgColor" },
	{ "Menu.BgColor", "Menu/BgColor" },
	{ "Menu.ArmedTextColor", "Menu/ArmedFgColor" },
	{ "Menu.ArmedBgColor", "Menu/ArmedBgColor" },
	{ "Menu.TextInset", NULL, "6" },

	{ "Panel.FgColor", "FgColor" },
	{ "Panel.BgColor", "BgColor" },

	{ "ProgressBar.FgColor", "BrightControlText" },
	{ "ProgressBar.BgColor", "WindowBgColor" },

	{ "PropertySheet.TextColor", "FgColorDim" },
	{ "PropertySheet.SelectedTextColor", "BrightControlText" },
	{ "PropertySheet.TransitionEffectTime",	NULL, "0" },

	{ "RadioButton.TextColor", "FgColor" },
	{ "RadioButton.SelectedTextColor", "BrightControlText" },

	{ "RichText.TextColor", "WindowFgColor" },
	{ "RichText.BgColor", "WindowBgColor" },
	{ "RichText.SelectedTextColor", "SelectionFgColor" },
	{ "RichText.SelectedBgColor", "SelectionBgColor" },

	{ "ScrollBar.Wide", NULL, "19" },

	{ "ScrollBarButton.FgColor", "DimBaseText" },
	{ "ScrollBarButton.BgColor", "ControlBG" },
	{ "ScrollBarButton.ArmedFgColor", "BaseText" },
	{ "ScrollBarButton.ArmedBgColor", "ControlBG" },
	{ "ScrollBarButton.DepressedFgColor", "BaseText" },
	{ "ScrollBarButton.DepressedBgColor", "ControlBG" },

	{ "ScrollBarSlider.FgColor", "ScrollBarSlider/ScrollBarSliderFgColor" },
	{ "ScrollBarSlider.BgColor", "ScrollBarSlider/ScrollBarSliderBgColor" },

	{ "SectionedListPanel.HeaderTextColor", "SectionTextColor" },
	{ "SectionedListPanel.HeaderBgColor", "BuddyListBgColor" },
	{ "SectionedListPanel.DividerColor", "SectionDividerColor" },
	{ "SectionedListPanel.TextColor", "BuddyButton/FgColor1" },
	{ "SectionedListPanel.BrightTextColor", "BuddyButton/ArmedFgColor1" },
	{ "SectionedListPanel.BgColor", "BuddyListBgColor" },
	{ "SectionedListPanel.SelectedTextColor", "BuddyButton/ArmedFgColor1" },
	{ "SectionedListPanel.SelectedBgColor", "BuddyButton/ArmedBgColor" },
	{ "SectionedListPanel.OutOfFocusSelectedTextColor", "BuddyButton/ArmedFgColor2" },
	{ "SectionedListPanel.OutOfFocusSelectedBgColor", "SelectionBG2" },

	{ "Slider.NobColor", "SliderTickColor" },
	{ "Slider.TextColor", "Slider/SliderFgColor" },
	{ "Slider.TrackColor", "SliderTrackColor"},
	{ "Slider.DisabledTextColor1", "DisabledFgColor1" },
	{ "Slider.DisabledTextColor2", "DisabledFgColor2" },

	{ "TextEntry.TextColor", "WindowFgColor" },
	{ "TextEntry.BgColor", "WindowBgColor" },
	{ "TextEntry.CursorColor", "TextCursorColor" },
	{ "TextEntry.DisabledTextColor", "WindowDisabledFgColor" },
	{ "TextEntry.DisabledBgColor", "ControlBG" },
	{ "TextEntry.SelectedTextColor", "SelectionFgColor" },
	{ "TextEntry.SelectedBgColor", "SelectionBgColor" },
	{ "TextEntry.OutOfFocusSelectedBgColor", "SelectionBG2" },
	{ "TextEntry.FocusEdgeColor", "BorderSelection" },

	{ "ToggleButton.SelectedTextColor", "BrightControlText" },

	{ "Tooltip.TextColor", "BorderSelection" },
	{ "Tooltip.BgColor", "SelectionBG" },

	{ "TreeView.BgColor", "ListBgColor" },

	{ "WizardSubPanel.BgColor", "SubPanelBgColor" },
};

void CScheme::LoadFromFile(VPANEL sizingPanel, const char *inFilename, const char *inTag, KeyValues *inKeys)
{
	Q_strncpy(fileName, inFilename, sizeof(fileName));

	m_SizingPanel = sizingPanel;

	m_pData = inKeys;
	m_pkvBaseSettings = m_pData->FindKey("BaseSettings", true);
	m_pkvColors = m_pData->FindKey("Colors", true);

	KeyValues *name = m_pData->FindKey("Name", true);
	name->SetString("Name", inTag);

	if (inTag)
	{
		Q_strncpy(tag, inTag, sizeof(tag));
	}
	else
	{
		Assert("You need to name the scheme!");
		Q_strncpy(tag, "default", sizeof(tag));
	}

	for (int i = 0; i < ARRAYSIZE(g_SchemeTranslation); i++)
	{
		if (!m_pkvBaseSettings->FindKey(g_SchemeTranslation[i].pchNewEntry, false))
		{
			const char *pchColor;

			if (g_SchemeTranslation[i].pchOldEntry)
			{
				pchColor = LookupSchemeSetting(g_SchemeTranslation[i].pchOldEntry);
			}
			else
			{
				pchColor = g_SchemeTranslation[i].pchDefaultValue;
			}

			Assert(pchColor);

			m_pkvBaseSettings->SetString(g_SchemeTranslation[i].pchNewEntry, pchColor);
		}
	}

	LoadFonts();
	LoadBorders();
}

void CScheme::LoadFonts(void)
{
	for (KeyValues *kv = m_pData->FindKey("CustomFontFiles", true)->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		const char *fontFile = kv->GetString();

		if (fontFile && *fontFile)
		{
			surface()->AddCustomFontFile(fontFile);
		}
	}

	for (KeyValues *kv = m_pData->FindKey("Fonts", true)->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		for (int i = 0; i < 2; i++)
		{
			bool proportionalFont = static_cast<bool>(i);
			const char *fontName = GetMungedFontName(kv->GetName(), tag, proportionalFont);
			HFont font = surface()->CreateFont();
			int j = m_FontAliases.AddToTail();
			m_FontAliases[j]._fontName = fontName;
			m_FontAliases[j]._trueFontName = kv->GetName();
			m_FontAliases[j]._font = font;
			m_FontAliases[j].m_bProportional = proportionalFont;
		}
	}

	ReloadFontGlyphs();
}

void CScheme::ReloadFontGlyphs(void)
{
	if (m_SizingPanel != 0)
	{
		ipanel()->GetSize(m_SizingPanel, m_nScreenWide, m_nScreenTall);
	}
	else
	{
		surface()->GetScreenSize(m_nScreenWide, m_nScreenTall);
	}

	int minimumFontHeight = GetMinimumFontHeightForCurrentLanguage();
	KeyValues *fonts = m_pData->FindKey("Fonts", true);

	for (int i = 0; i < m_FontAliases.Count(); i++)
	{
		KeyValues *kv = fonts->FindKey(m_FontAliases[i]._trueFontName.String(), true);

		for (KeyValues *fontdata = kv->GetFirstSubKey(); fontdata != NULL; fontdata = fontdata->GetNextKey())
		{
			int fontYResMin = 0, fontYResMax = 0;
			sscanf(fontdata->GetString("yres", ""), "%d %d", &fontYResMin, &fontYResMax);

			if (fontYResMin)
			{
				if (!fontYResMax)
				{
					fontYResMax = fontYResMin;
				}

				if (m_nScreenTall < fontYResMin || m_nScreenTall > fontYResMax)
					continue;
			}

			int flags = 0;

			if (fontdata->GetInt("italic"))
				flags |= ISurface::FONTFLAG_ITALIC;

			if (fontdata->GetInt("underline"))
				flags |= ISurface::FONTFLAG_UNDERLINE;

			if (fontdata->GetInt("strikeout"))
				flags |= ISurface::FONTFLAG_STRIKEOUT;

			if (fontdata->GetInt("symbol"))
				flags |= ISurface::FONTFLAG_SYMBOL;

			if (fontdata->GetInt("antialias") && surface()->SupportsFeature(CSurface::ANTIALIASED_FONTS))
				flags |= ISurface::FONTFLAG_ANTIALIAS;

			if (fontdata->GetInt("dropshadow") && surface()->SupportsFeature(CSurface::DROPSHADOW_FONTS))
				flags |= ISurface::FONTFLAG_DROPSHADOW;

			if (fontdata->GetInt("outline") && surface()->SupportsFeature(CSurface::OUTLINE_FONTS))
				flags |= ISurface::FONTFLAG_OUTLINE;

			if(fontdata->GetInt("outline2"))
				flags |= ISurface::FONTFLAG_OUTLINE2;				

			if (fontdata->GetInt("custom"))
				flags |= ISurface::FONTFLAG_CUSTOM;

			if (fontdata->GetInt("rotary"))
				flags |= ISurface::FONTFLAG_ROTARY;

			if (fontdata->GetInt("additive"))
				flags |= ISurface::FONTFLAG_ADDITIVE;

			int tall = fontdata->GetInt("tall");
			int blur = fontdata->GetInt("blur");
			int scanlines = fontdata->GetInt("scanlines");

			if ((!fontYResMin && !fontYResMax) && m_FontAliases[i].m_bProportional)
			{
				tall = g_Scheme.GetProportionalScaledValueEx(this, tall);
				blur = g_Scheme.GetProportionalScaledValueEx(this, blur);
				scanlines = g_Scheme.GetProportionalScaledValueEx(this, scanlines);
			}

			//Out maximum value in s_pFontPageSize is 256, so we have largest of 255
			if (tall > 127)
				tall = 127;

			if (tall < minimumFontHeight)
				tall = minimumFontHeight;

			surface()->AddGlyphSetToFont(m_FontAliases[i]._font, fontdata->GetString("name"), tall, fontdata->GetInt("weight"), blur, scanlines, flags);

			break;
		}
	}
}

void CScheme::LoadBorders(void)
{
	m_pkvBorders = m_pData->FindKey("Borders", true);

	for (KeyValues *kv = m_pkvBorders->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		if (kv->GetDataType() == KeyValues::TYPE_STRING)
		{
		}
		else
		{
			int i = m_BorderList.AddToTail();

			Border *border = new Border();
			border->SetName(kv->GetName());
			border->ApplySchemeSettings(this, kv);

			m_BorderList[i].border = border;
			m_BorderList[i].bSharedBorder = false;
			m_BorderList[i].borderSymbol = kv->GetNameSymbol();
		}

	}

	for (KeyValues *kv = m_pkvBorders->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		if (kv->GetDataType() == KeyValues::TYPE_STRING)
		{
			Border *border = (Border *)GetBorder(kv->GetString());
			Assert(border);

			int i = m_BorderList.AddToTail();
			m_BorderList[i].border = border;
			m_BorderList[i].bSharedBorder = true;
			m_BorderList[i].borderSymbol = kv->GetNameSymbol();
		}
	}

	m_pBaseBorder = GetBorder("BaseBorder");
}

void CSchemeManager::ReloadSchemes(void)
{
	int count = m_Schemes.Count();
	Shutdown(false);

	for (int i = 1; i < count; i++)
	{
		LoadSchemeFromFile(m_Schemes[i]->GetFileName(), m_Schemes[i]->GetName());
	}
}

void CScheme::Shutdown(bool full)
{
	for (int i = 0; i < m_BorderList.Count(); i++)
	{
		if (!m_BorderList[i].bSharedBorder)
		{
			Border *border = m_BorderList[i].border;
			delete border;
		}
	}

	m_pBaseBorder = NULL;
	m_BorderList.RemoveAll();
	m_pkvBorders = NULL;

	if (full && m_pData)
	{
		m_pData->deleteThis();
		m_pData = NULL;
		delete this;
	}
}

HScheme CSchemeManager::GetDefaultScheme(void)
{
	return 1;
}

HScheme CSchemeManager::GetScheme(const char *tag)
{
	for (int i = 1; i < m_Schemes.Count(); i++)
	{
		if (!stricmp(tag, m_Schemes[i]->GetName()))
		{
			return i;
		}
	}

	return 1;
}

int CSchemeManager::GetProportionalScaledValue_(int rootWide, int rootTall, int normalizedValue)
{
	int proH, proW;
	surface()->GetProportionalBase(proW, proH);

	double scale = (double)rootTall / (double)proH;
	return (int)(normalizedValue * scale);
}

int CSchemeManager::GetProportionalNormalizedValue_(int rootWide, int rootTall, int scaledValue)
{
	int proH, proW;
	surface()->GetProportionalBase(proW, proH);

	float scale = (double)rootTall / (double)proH;
	return (int)(scaledValue / scale);
}

int CSchemeManager::GetProportionalScaledValueEx(CScheme *pScheme, int normalizedValue)
{
	VPANEL sizing = pScheme->GetSizingPanel();

	if (!sizing)
		return GetProportionalScaledValue(normalizedValue);

	int w, h;
	ipanel()->GetSize(sizing, w, h);
	return GetProportionalScaledValue_(w, h, normalizedValue);
}

int CSchemeManager::GetProportionalNormalizedValueEx(CScheme *pScheme, int scaledValue)
{
	VPANEL sizing = pScheme->GetSizingPanel();

	if (!sizing)
		return GetProportionalNormalizedValue(scaledValue);

	int w, h;
	ipanel()->GetSize(sizing, w, h);
	return GetProportionalNormalizedValue_(w, h, scaledValue);
}

int CSchemeManager::GetProportionalScaledValueEx(HScheme scheme, int normalizedValue)
{
	IScheme *pscheme = GetIScheme(scheme);

	if (!pscheme)
	{
		Assert(0);
		return GetProportionalScaledValue(normalizedValue);
	}

	CScheme *p = static_cast<CScheme *>(pscheme);
	return GetProportionalScaledValueEx(p, normalizedValue);
}

int CSchemeManager::GetProportionalNormalizedValueEx(HScheme scheme, int scaledValue)
{
	IScheme *pscheme = GetIScheme(scheme);

	if (!pscheme)
	{
		Assert(0);
		return GetProportionalNormalizedValue(scaledValue);
	}

	CScheme *p = static_cast<CScheme *>(pscheme);
	return GetProportionalNormalizedValueEx(p, scaledValue);
}

int CSchemeManager::GetProportionalScaledValue(int normalizedValue)
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	return GetProportionalScaledValue_(wide, tall, normalizedValue);
}

int CSchemeManager::GetProportionalNormalizedValue(int scaledValue)
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	return GetProportionalNormalizedValue_(wide, tall, scaledValue);
}

const char *CScheme::GetResourceString(const char *stringName)
{
	return m_pkvBaseSettings->GetString(stringName);
}

IImage *CSchemeManager::GetImage(const char *imageName, bool hardwareFiltered)
{
	s_pszSearchString = imageName;

	if (!imageName /*|| strlen(imageName) <= 0*/)
	{
		return NULL;
	}

	CachedBitmapHandle_t searchBitmap;
	searchBitmap.bitmap = NULL;

	int i = m_Bitmaps.Find(searchBitmap);

	if (m_Bitmaps.IsValidIndex(i))
		return m_Bitmaps[i].bitmap;

	Bitmap *pBitmap = new Bitmap(imageName, hardwareFiltered);

	if (imageName[0] && !pBitmap->IsValid())
	{
		delete pBitmap;
		return NULL;
	}

	searchBitmap.bitmap = pBitmap;
	m_Bitmaps.Insert(searchBitmap);
	return pBitmap;
}

HTexture CSchemeManager::GetImageID(const char *imageName, bool hardwareFiltered)
{
	IImage *img = GetImage(imageName, hardwareFiltered);
	return ((Bitmap *)img)->GetID();
}

void CSchemeManager::DeleteImage(const char *pImageName)
{
	s_pszSearchString = pImageName;

	if (!pImageName)
		return;

	CachedBitmapHandle_t searchBitmap;
	searchBitmap.bitmap = NULL;

	int i = m_Bitmaps.Find(searchBitmap);

	if (m_Bitmaps.IsValidIndex(i))
	{
		delete m_Bitmaps[i].bitmap;
		m_Bitmaps.RemoveAt(i);
	}
}

IBorder *CScheme::GetBorder(const char *borderName)
{
	int symbol = KeyValuesSystem()->GetSymbolForString(borderName);

	for (int i = 0; i < m_BorderList.Count(); i++)
	{
		if (m_BorderList[i].borderSymbol == symbol)
			return m_BorderList[i].border;
	}

	return m_pBaseBorder;
}

HFont CScheme::FindFontInAliasList(const char *fontName)
{
	for (int i = m_FontAliases.Count(); --i >= 0;)
	{
		if (!strnicmp(fontName, m_FontAliases[i]._fontName.String(), FONT_ALIAS_NAME_LENGTH))
			return m_FontAliases[i]._font;
	}

	return 0;
}

char const *CScheme::GetFontName(const HFont &font)
{
	for (int i = m_FontAliases.Count(); --i >= 0;)
	{
		HFont fnt = (HFont)m_FontAliases[i]._font;

		if (fnt == font)
			return m_FontAliases[i]._trueFontName.String();
	}

	return "<Unknown font>";
}

HFont CScheme::GetFont(const char *fontName, bool proportional)
{
	return FindFontInAliasList(GetMungedFontName(fontName, tag, proportional));
}

const char *CScheme::GetMungedFontName(const char *fontName, const char *scheme, bool proportional)
{
	static char mungeBuffer[64];

	if (scheme)
		Q_snprintf(mungeBuffer, sizeof(mungeBuffer), "%s%s-%s", fontName, scheme, proportional ? "p" : "no");
	else
		Q_snprintf(mungeBuffer, sizeof(mungeBuffer), "%s-%s", fontName, proportional ? "p" : "no");

	return mungeBuffer;
}

Color CScheme::GetColor(const char *colorName, Color defaultColor)
{
	const char *pchT = LookupSchemeSetting(colorName);

	if (!pchT)
		return defaultColor;

	int r, g, b, a = 0;

	if (sscanf(pchT, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		return Color(r, g, b, a);

	return defaultColor;
}

const char *CScheme::LookupSchemeSetting(const char *pchSetting)
{
	int r, g, b, a = 0;
	int res = sscanf(pchSetting, "%d %d %d %d", &r, &g, &b, &a);

	if (res >= 3)
		return pchSetting;

	const char *colStr = m_pkvColors->GetString(pchSetting, NULL);

	if (colStr)
		return colStr;

	colStr = m_pkvBaseSettings->GetString(pchSetting, NULL);

	if (colStr)
		return LookupSchemeSetting(colStr);

	return pchSetting;
}

int CScheme::GetMinimumFontHeightForCurrentLanguage(void)
{
	char *language = gCapFuncs.szLanguage;

	if (IsPC())
	{
		if (!stricmp(language, "korean") || !stricmp(language, "tchinese") || !stricmp(language, "schinese") || !stricmp(language, "japanese"))
			return 13;

		if (!stricmp(language, "thai"))
			return 18;
	}

	return 0;
}

HScheme ISchemeManager::LoadSchemeFromFileEx(VPANEL sizingPanel, const char *fileName, const char *tag)
{
	return g_Scheme.LoadSchemeFromFileEx(sizingPanel, fileName, tag);
}

int ISchemeManager::GetProportionalScaledValueEx(HScheme scheme, int normalizedValue)
{
	return g_Scheme.GetProportionalScaledValueEx(scheme, normalizedValue);
}

int ISchemeManager::GetProportionalNormalizedValueEx(HScheme scheme, int scaledValue)
{
	return g_Scheme.GetProportionalNormalizedValueEx(scheme, scaledValue);
}

char const *IScheme::GetFontName(const HFont &font)
{
	return ((CScheme *)this)->GetFontName(font);
}

const char *IScheme::GetName(void)
{
	return ((CScheme *)this)->GetName();
}
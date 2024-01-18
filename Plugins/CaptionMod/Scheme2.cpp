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
#include "privatefuncs.h"
#include "DpiManager.h"

using namespace vgui;

const char *CSchemeManager::s_pszSearchString = NULL;

bool CSchemeManager::BitmapHandleSearchFunc(const CachedBitmapHandle_t &lhs, const CachedBitmapHandle_t &rhs)
{
	if (lhs.bitmap && rhs.bitmap)
		return stricmp(lhs.bitmap->GetName(), rhs.bitmap->GetName()) > 0;
	else if (lhs.bitmap)
		return stricmp(lhs.bitmap->GetName(), s_pszSearchString) > 0;

	return stricmp(s_pszSearchString, rhs.bitmap->GetName()) > 0;
}

static CSchemeManager g_SchemeManagerNew;

CSchemeManager* g_pVGuiSchemeManager = &g_SchemeManagerNew;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSchemeManager, ISchemeManager2, VGUI_SCHEME2_INTERFACE_VERSION, g_SchemeManagerNew);

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

	auto data = new KeyValues("Scheme");
	data->UsesEscapeSequences(true);

	bool result = false;

	//This is what vgui2.dll does
	if (!result)
	{
		result = data->LoadFromFile(g_pFullFileSystem, fileName, "SKIN");
	}

	//Why GAME ?
	if (!result)
	{
		result = data->LoadFromFile(g_pFullFileSystem, fileName, "GAME");
	}

	//Fallback
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
		//Assert("You need to name the scheme!");
		Q_strncpy(tag, "default", sizeof(tag));
	}

	//Added in HL25
	if (1)
	{
		//Why does Valve use GetFloat ???

		/*
			if ( (*(int (__thiscall **)(_DWORD, const char *, _DWORD))(**(_DWORD **)(this + 328) + 20))(
					 *(_DWORD *)(this + 328),
					 "ProportionalBaseWidth",
					 0)
				&& (*(int (__thiscall **)(_DWORD, const char *, _DWORD))(**(_DWORD **)(this + 328) + 20))(
					 *(_DWORD *)(this + 328),
					 "ProportionalBaseHeight",
					 0) )
			  {
				v10 = *(_DWORD *)(this + 328);
				v11 = *(_DWORD *)dword_100282B8;
				(*(void (__thiscall **)(int, const char *, float))(*(_DWORD *)v10 + 44))(v10, "ProportionalBaseHeight", 480.0);
				sub_1001E7E0();
				(*(void (__thiscall **)(int, const char *, float))(*(_DWORD *)v10 + 44))(v10, "ProportionalBaseWidth", 640.0);
				v12 = sub_1001E7E0();
				(*(void (__thiscall **)(int, int))(v11 + 412))(dword_100282B8, v12);
			  }
		*/

		/*
			auto ProportionalBaseHeight = m_pkvBaseSettings->GetFloat("ProportionalBaseHeight", 480.0f);
			auto ProportionalBaseWidth = m_pkvBaseSettings->GetFloat("ProportionalBaseWidth", 640.0f);
		*/

		auto ProportionalBaseWidth = m_pkvBaseSettings->GetInt("ProportionalBaseWidth", 640);
		auto ProportionalBaseHeight = m_pkvBaseSettings->GetInt("ProportionalBaseHeight", 480);

		surface()->SetProportionalBase(ProportionalBaseWidth, ProportionalBaseHeight);

		auto ProportionalBaseWidthHD = m_pkvBaseSettings->GetInt("ProportionalBaseWidthHD", 1280);
		auto ProportionalBaseHeightHD = m_pkvBaseSettings->GetInt("ProportionalBaseHeightHD", 720);

		surface()->SetHDProportionalBase(ProportionalBaseWidthHD, ProportionalBaseHeightHD);
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

	const bool c_proportional[] = {
		false, true, true
	};
	const bool c_hd[] = {
		false, false, true
	};

	for (KeyValues *kv = m_pData->FindKey("Fonts", true)->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		for (int i = 0; i < 3; i++)
		{
			bool proportional = c_proportional[i];

			//Added in HL25
			bool hd = c_hd[i];

			const char *fontName = GetMungedHDFontName(kv->GetName(), tag, proportional, hd);

			HFont hFont = surface()->CreateFont();

			int fontIndex = m_FontAliases.AddToTail();

			m_FontAliases[fontIndex]._fontName = fontName;
			m_FontAliases[fontIndex]._trueFontName = kv->GetName();
			m_FontAliases[fontIndex]._font = hFont;
			m_FontAliases[fontIndex].m_bProportional = proportional;
			m_FontAliases[fontIndex].m_bHD = hd;
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

			auto YResString = fontdata->GetString("yres", "");

			sscanf(YResString, "%d %d", &fontYResMin, &fontYResMax);

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
				flags |= FONTFLAG_ITALIC;

			if (fontdata->GetInt("underline"))
				flags |= FONTFLAG_UNDERLINE;

			if (fontdata->GetInt("strikeout"))
				flags |= FONTFLAG_STRIKEOUT;

			if (fontdata->GetInt("symbol"))
				flags |= FONTFLAG_SYMBOL;

			if (fontdata->GetInt("antialias") && surface()->SupportsFeature(ANTIALIASED_FONTS))
				flags |= FONTFLAG_ANTIALIAS;

			if (fontdata->GetInt("dropshadow") && surface()->SupportsFeature(DROPSHADOW_FONTS))
				flags |= FONTFLAG_DROPSHADOW;

			if (fontdata->GetInt("outline") && surface()->SupportsFeature(OUTLINE_FONTS))
				flags |= FONTFLAG_OUTLINE;

			if (fontdata->GetInt("outline2"))
				flags |= FONTFLAG_OUTLINE2;

			if (fontdata->GetInt("custom"))
				flags |= FONTFLAG_CUSTOM;

			if (fontdata->GetInt("rotary"))
				flags |= FONTFLAG_ROTARY;

			if (fontdata->GetInt("additive"))
				flags |= FONTFLAG_ADDITIVE;

			int tall = fontdata->GetInt("tall");
			int blur = fontdata->GetInt("blur");
			int scanlines = fontdata->GetInt("scanlines");

			if ((!fontYResMin && !fontYResMax) && m_FontAliases[i].m_bProportional)
			{
#if 0
				if (m_FontAliases[i].m_bHD)
				{
					tall = g_SchemeManagerNew.GetHDProportionalScaledValueEx(this, tall);
					blur = g_SchemeManagerNew.GetHDProportionalScaledValueEx(this, blur);
					scanlines = g_SchemeManagerNew.GetHDProportionalScaledValueEx(this, scanlines);
				}
				else
				{
					tall = g_SchemeManagerNew.GetProportionalScaledValueEx(this, tall);
					blur = g_SchemeManagerNew.GetProportionalScaledValueEx(this, blur);
					scanlines = g_SchemeManagerNew.GetProportionalScaledValueEx(this, scanlines);
				}
#else
				if (m_FontAliases[i].m_bHD)
				{
					tall = g_SchemeManagerNew.GetHDProportionalScaledValue(tall);
					blur = g_SchemeManagerNew.GetHDProportionalScaledValue(blur);
					scanlines = g_SchemeManagerNew.GetHDProportionalScaledValue(scanlines);
				}
				else
				{
					tall = g_SchemeManagerNew.GetProportionalScaledValue(tall);
					blur = g_SchemeManagerNew.GetProportionalScaledValue(blur);
					scanlines = g_SchemeManagerNew.GetProportionalScaledValue(scanlines);
				}
#endif
			}

			//Out maximum value in s_pFontPageSize is 256, so we have largest of 255
			if (tall > 127)
				tall = 127;

			if (tall < minimumFontHeight)
				tall = minimumFontHeight;

			auto fontName = fontdata->GetString("name");
			auto fontWeight = fontdata->GetInt("weight");

			surface()->AddGlyphSetToFont(m_FontAliases[i]._font, fontName, tall, fontdata->GetInt("weight"), blur, scanlines, flags);

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

int CSchemeManager::GetProportionalScaledValue_LD(int rootWide, int rootTall, int normalizedValue)
{
	int proH, proW;
	surface()->GetProportionalBase(proW, proH);

	double scale = (double)rootTall / (double)proH;
	return (int)(normalizedValue * scale);
}

int CSchemeManager::GetProportionalNormalizedValue_LD(int rootWide, int rootTall, int scaledValue)
{
	int proH, proW;
	surface()->GetProportionalBase(proW, proH);

	float scale = (double)rootTall / (double)proH;
	return (int)(scaledValue / scale);
}

//Added in HL25

int CSchemeManager::GetProportionalScaledValue_HD(int rootWide, int rootTall, int normalizedValue)
{
	int proH, proW;
	surface()->GetHDProportionalBase(proW, proH);

	double scale = (double)rootTall / (double)proH;
	return (int)(normalizedValue * scale);
}

int CSchemeManager::GetProportionalNormalizedValue_HD(int rootWide, int rootTall, int scaledValue)
{
	int proH, proW;
	surface()->GetHDProportionalBase(proW, proH);

	float scale = (double)rootTall / (double)proH;
	return (int)(scaledValue / scale);
}

//LD

int CSchemeManager::GetProportionalScaledValueEx(IScheme2*pScheme, int normalizedValue)
{
	VPANEL sizing = pScheme->GetSizingPanel();

	if (!sizing)
		return GetProportionalScaledValue(normalizedValue);

	int w, h;
	ipanel()->GetSize(sizing, w, h);
	return GetProportionalScaledValue_LD(w, h, normalizedValue);
}

int CSchemeManager::GetProportionalNormalizedValueEx(IScheme2*pScheme, int scaledValue)
{
	auto sizing = pScheme->GetSizingPanel();

	if (!sizing)
		return GetProportionalNormalizedValue(scaledValue);

	int w, h;
	ipanel()->GetSize(sizing, w, h);
	return GetProportionalNormalizedValue_LD(w, h, scaledValue);
}

int CSchemeManager::GetProportionalScaledValueEx(HScheme scheme, int normalizedValue)
{
	auto pScheme = GetIScheme(scheme);

	if (!pScheme)
	{
		Assert(0);
		return GetProportionalScaledValue(normalizedValue);
	}

	CScheme *p = static_cast<CScheme *>(pScheme);
	return GetProportionalScaledValueEx(p, normalizedValue);
}

int CSchemeManager::GetProportionalNormalizedValueEx(HScheme scheme, int scaledValue)
{
	auto pScheme = GetIScheme(scheme);

	if (!pScheme)
	{
		Assert(0);
		return GetProportionalNormalizedValue(scaledValue);
	}

	CScheme *p = static_cast<CScheme *>(pScheme);
	return GetProportionalNormalizedValueEx(p, scaledValue);
}

int CSchemeManager::GetProportionalScaledValue(int normalizedValue)
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	return GetProportionalScaledValue_LD(wide, tall, normalizedValue);
}

int CSchemeManager::GetProportionalNormalizedValue(int scaledValue)
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	return GetProportionalNormalizedValue_LD(wide, tall, scaledValue);
}

//HD

int CSchemeManager::GetHDProportionalScaledValueEx(IScheme2* pScheme, int normalizedValue)
{
	VPANEL sizing = pScheme->GetSizingPanel();

	if (!sizing)
		return GetHDProportionalScaledValue(normalizedValue);

	int w, h;
	ipanel()->GetSize(sizing, w, h);
	return GetProportionalScaledValue_HD(w, h, normalizedValue);
}

int CSchemeManager::GetHDProportionalNormalizedValueEx(IScheme2* pScheme, int scaledValue)
{
	auto sizing = pScheme->GetSizingPanel();

	if (!sizing)
		return GetHDProportionalNormalizedValue(scaledValue);

	int w, h;
	ipanel()->GetSize(sizing, w, h);
	return GetProportionalNormalizedValue_HD(w, h, scaledValue);
}

int CSchemeManager::GetHDProportionalScaledValueEx(HScheme scheme, int normalizedValue)
{
	auto pScheme = GetIScheme(scheme);

	if (!pScheme)
	{
		Assert(0);
		return GetHDProportionalScaledValue(normalizedValue);
	}

	CScheme* p = static_cast<CScheme*>(pScheme);
	return GetHDProportionalScaledValueEx(p, normalizedValue);
}

int CSchemeManager::GetHDProportionalNormalizedValueEx(HScheme scheme, int scaledValue)
{
	auto pScheme = GetIScheme(scheme);

	if (!pScheme)
	{
		Assert(0);
		return GetHDProportionalNormalizedValue(scaledValue);
	}

	CScheme* p = static_cast<CScheme*>(pScheme);
	return GetHDProportionalNormalizedValueEx(p, scaledValue);
}

//Added in HL25

float CSchemeManager::GetProportionalScale()
{
	int wide, tall;
	int propWide, propTall;
	surface()->GetScreenSize(wide, tall);
	surface()->GetProportionalBase(propWide, propTall);
	return (float)(tall / (double)propTall);
}

int CSchemeManager::GetHDProportionalScaledValue(int normalizedValue)
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	return GetProportionalScaledValue_HD(wide, tall, normalizedValue);
}

int CSchemeManager::GetHDProportionalNormalizedValue(int scaledValue)
{
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	return GetProportionalNormalizedValue_HD(wide, tall, scaledValue);
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

	if (!pBitmap)
	{
		return NULL;
	}

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
#define FONT_ALIAS_NAME_LENGTH 64

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

HFont CScheme::GetHDFont(const char* fontName, bool proportional, bool hd)
{
	return FindFontInAliasList(GetMungedHDFontName(fontName, tag, proportional, hd));
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

const char* CScheme::GetMungedHDFontName(const char* fontName, const char* scheme, bool proportional, bool hd)
{
	static char mungeBuffer[64];

	if (scheme)
		Q_snprintf(mungeBuffer, sizeof(mungeBuffer), "%s%s-%s", fontName, scheme, (hd) ? "hdp" : (proportional ? "p" : "no"));
	else
		Q_snprintf(mungeBuffer, sizeof(mungeBuffer), "%s-%s", fontName, (proportional) ? "p" : "no");

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
	char *language = m_szCurrentLanguage;

	if (IsPC())
	{
		if (!stricmp(language, "korean") || !stricmp(language, "tchinese") || !stricmp(language, "schinese") || !stricmp(language, "japanese"))
			return 13;

		if (!stricmp(language, "thai"))
			return 18;
	}

	return 0;
}

char const *IScheme::GetFontName(const HFont &font)
{
	return ((CScheme *)this)->GetFontName(font);
}

const char *IScheme::GetName(void)
{
	return ((CScheme *)this)->GetName();
}
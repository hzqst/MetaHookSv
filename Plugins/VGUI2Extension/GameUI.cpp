#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <IGameConsole.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>
#include <vgui/IClientPanel.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Menu.h>
#include <capstone.h>
#include <set>
#include <sstream>
#include <vector>

#include "plugins.h"
#include "privatefuncs.h"
#include "DpiManagerInternal.h"
#include "VGUI2ExtensionInternal.h"

static hook_t* g_phook_ServerBrowser_Panel_Init = NULL;
static hook_t* g_phook_ServerBrowser_KeyValues_LoadFromFile = NULL;
//static hook_t* g_phook_ServerBrowser_LoadControlSettingsAndUserConfig = NULL;
static hook_t* g_phook_GameUI_Panel_Init = NULL;
static hook_t* g_phook_GameUI_KeyValues_LoadFromFile = NULL;
static hook_t* g_phook_CGameConsoleDialog_ctor = NULL;
static hook_t* g_phook_CCreateMultiplayerGameDialog_ctor = NULL;
static hook_t* g_phook_COptionsDialog_ctor = NULL;
static hook_t* g_phook_COptionsSubVideo_ctor = NULL;
static hook_t* g_phook_COptionsSubVideo_OnApplyChanges = NULL;
static hook_t* g_phook_COptionsSubAudio_ctor = NULL;
static hook_t* g_phook_COptionsSubAudio_OnApplyChanges = NULL;
static hook_t* g_phook_COptionsSubMultiplayer_ctor = NULL;
static hook_t* g_phook_COptionsSubMultiplayer_OnApplyChanges = NULL;
static hook_t* g_phook_COptionsSubVideo_ApplyVidSettings = NULL;
static hook_t* g_phook_CBasePanel_ctor = NULL;
static hook_t* g_phook_CBasePanel_ApplySchemeSettings = NULL;
static hook_t* g_phook_CTaskBar_ctor = NULL;
static hook_t* g_phook_CTaskBar_OnCommand = NULL;
static hook_t* g_phook_GameUI_RichText_InsertChar = NULL;
static hook_t* g_phook_GameUI_RichText_InsertStringW = NULL;
static hook_t* g_phook_GameUI_RichText_OnThink = NULL;
static hook_t* g_phook_GameUI_TextEntry_OnKeyCodeTyped = NULL;
static hook_t* g_phook_GameUI_TextEntry_LayoutVerticalScrollBarSlider = NULL;
static hook_t* g_phook_GameUI_TextEntry_GetStartDrawIndex = NULL;
static hook_t* g_phook_GameUI_PropertySheet_PerformLayout = NULL;
static hook_t* g_phook_GameUI_PropertySheet_HasHotkey = NULL;
static hook_t* g_phook_GameUI_FocusNavGroup_GetCurrentFocus = NULL;
static hook_t* g_phook_GameUI_Menu_MakeItemsVisibleInScrollRange = NULL;
static hook_t* g_phook_CCareerProfileFrame_ctor = NULL;
static hook_t* g_phook_CCareerMapFrame_ctor = NULL;
static hook_t* g_phook_CCareerBotFrame_ctor = NULL;

namespace vgui
{
	bool VGui_InitInterfacesList(const char* moduleName, CreateInterfaceFn* factoryList, int numFactories);
}

#define LOAD_CONTROL_SETTINGS_FALLBACK(mod, panel, name) if(1) {\
	if (!gPrivateFuncs.mod##_LoadControlSettings)\
	{\
		auto panel_vftable = *(PVOID**)panel; \
		gPrivateFuncs.mod##_LoadControlSettings = (decltype(gPrivateFuncs.mod##_LoadControlSettings))panel_vftable[536 / 4];\
	}\
	auto bIsResourceExists = FILESYSTEM_ANY_FILEEXISTS("resource/"##name);\
	if (bIsResourceExists)\
	{\
		gPrivateFuncs.mod##_LoadControlSettings(panel, 0, "resource/"##name, NULL);\
	}\
	else\
	{\
		if (g_iEngineType == ENGINE_GOLDSRC_HL25)\
		{\
			bIsResourceExists = FILESYSTEM_ANY_FILEEXISTS("vgui2ext/resource_hl25/"##name);\
			if (bIsResourceExists)\
			{\
				gPrivateFuncs.mod##_LoadControlSettings(panel, 0, "vgui2ext/resource_hl25/"##name, NULL);\
			}\
		}\
		else\
		{\
			bIsResourceExists = FILESYSTEM_ANY_FILEEXISTS("vgui2ext/resource/"##name);\
			if (bIsResourceExists)\
			{\
				gPrivateFuncs.mod##_LoadControlSettings(panel, 0, "vgui2ext/resource/"##name, NULL);\
			}\
		}\
	}\
}

static int g_iPatchingPanelTall = 0;
static bool g_bPatchingGetFontTall = false;

int GetPatchedGetFontTall(int fontTall)
{
	if (g_bPatchingGetFontTall)
	{
		const int DRAW_OFFSET_X = 3, DRAW_OFFSET_Y = 1;

		//The displayLines should be always >= 1 otherwise the legacy VGUI2 controls may randomly crash

		/*
			int displayLines = g_iPatchingPanelTall / (fontTall + DRAW_OFFSET_Y);
		*/

		if (g_iPatchingPanelTall < fontTall + DRAW_OFFSET_Y)
		{
			return g_iPatchingPanelTall - DRAW_OFFSET_Y;
		}
	}

	return fontTall;
}

// Numbered callsites are contiguous within each module's gamedata.
static void PatchPanelSizeCallsites(PVOID moduleBase, const char* moduleName, const char* prefix, PVOID replacement, PVOID* original)
{
	for (int index = 0; ; ++index)
	{
		char symbolName[128];
		snprintf(symbolName, sizeof(symbolName), "%s%d", prefix, index);

		// Every pre-HL25 module publishes at least one call for each sizing method.
		auto address = index == 0
			? GamedataResolvePtr(moduleBase, moduleName, symbolName, MH_GAMESYMBOL_KIND_PATCH)
			: GamedataResolvePtrIfAvailable(moduleBase, moduleName, symbolName, MH_GAMESYMBOL_KIND_PATCH);
		if (!address)
			return;

		if (!g_pMetaHookAPI->InlinePatchRedirectBranch(address, replacement, original))
		{
			Sys_Error("Could not redirect gamedata patch: %s (module %s)\nEngine buildnum: %d", symbolName, moduleName, g_dwEngineBuildnum);
			return;
		}
	}
}

/*
====================================================================
ServerBrowser inline hook
====================================================================
*/

void __fastcall ServerBrowser_Panel_SetSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	auto pPanel = (vgui::IClientPanel*)pthis;

	if (pPanel->IsProportional())
	{
		width = g_pVGuiSchemeManager2->GetProportionalScaledValue(width);
		height = g_pVGuiSchemeManager2->GetProportionalScaledValue(height);
	}

	gPrivateFuncs.ServerBrowser_Panel_SetSize(pthis, 0, width, height);
}

void __fastcall ServerBrowser_Panel_SetMinimumSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	auto pPanel = (vgui::IClientPanel*)pthis;

	if (pPanel->IsProportional())
	{
		width = g_pVGuiSchemeManager2->GetProportionalScaledValue(width);
		height = g_pVGuiSchemeManager2->GetProportionalScaledValue(height);
	}

	gPrivateFuncs.ServerBrowser_Panel_SetMinimumSize(pthis, 0, width, height);
}

#if 0
void __fastcall CBaseGamesPage_OnButtonToggled(vgui::Panel* pthis, int dummy, vgui::Panel* a2, int state)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		return gPrivateFuncs.CBaseGamesPage_OnButtonToggled(pthis, dummy, a2, state);
	}

	gPrivateFuncs.CBaseGamesPage_OnButtonToggled(pthis, dummy, a2, state);
}

void* __fastcall CServerBrowserDialog_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.CServerBrowserDialog_ctor(pthis, dummy, parent);

	//TODO callbacks?



	return result;
}

void __fastcall ServerBrowser_LoadControlSettings(vgui::Panel* pthis, int dummy, const char* controlResourceName, const char* pathID)
{
	gPrivateFuncs.ServerBrowser_LoadControlSettings(pthis, 0, controlResourceName, pathID);
}

void __fastcall ServerBrowser_LoadControlSettingsAndUserConfig(vgui::Panel* pthis, int dummy, const char* dialogResourceName, int dialogID)
{
	if (!strcmp(dialogResourceName, "Servers/DialogServerBrowser.res"))
	{
		if (!gPrivateFuncs.ServerBrowser_LoadControlSettings)
		{
			auto panel_vftable = *(PVOID**)pthis;
			gPrivateFuncs.ServerBrowser_LoadControlSettings = (decltype(gPrivateFuncs.ServerBrowser_LoadControlSettings))panel_vftable[536 / 4];
			//Install_InlineHook(ServerBrowser_LoadControlSettings);
		}

		//int offset_AutoResize = 92;

		//*(int*)((PUCHAR)pthis + offset_AutoResize) = 0;

		//gPrivateFuncs.ServerBrowser_LoadControlSettings(pthis, 0, "Servers/DialogServerBrowser.res", NULL);

		gPrivateFuncs.ServerBrowser_LoadControlSettingsAndUserConfig(pthis, dummy, dialogResourceName, dialogID);



		return;
	}

	return gPrivateFuncs.ServerBrowser_LoadControlSettingsAndUserConfig(pthis, dummy, dialogResourceName, dialogID);
}

#endif

void __fastcall ServerBrowser_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.ServerBrowser_Panel_Init(pthis, 0, x, y, w, h);

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		auto pPanel = (vgui::IClientPanel*)pthis;
		pPanel->SetProportional(true);
	}
}

bool __fastcall ServerBrowser_KeyValues_LoadFromFile(void* pthis, int dummy, IFileSystem* pFileSystem, const char* resourceName, const char* pathId)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, "ServerBrowser", &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = gPrivateFuncs.ServerBrowser_KeyValues_LoadFromFile(pthis, dummy, pFileSystem, resourceName, pathId);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, "ServerBrowser", &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

/*
==================================================================================
GameUI vgui_controls hook
==================================================================================
*/

//This fixes a bug that GameUI.dll's RichText::InsertChar inserts "\r" into game console.
void __fastcall GameUI_RichText_InsertChar(void* pthis, int dummy, wchar_t ch)
{
	if (ch == L'\r')
		return;

	gPrivateFuncs.GameUI_RichText_InsertChar(pthis, 0, ch);
}

//This fixes a bug that GameUI.dll's RichText::InsertStringW inserts "\r" and "\0" into game console as an individual console message.
void __fastcall GameUI_RichText_InsertStringW(void* pthis, int dummy, wchar_t* ch)
{
	while (1)
	{
		std::wstringstream wss;

		while (1)
		{
			if ((*ch) == L'\r' || (*ch) == L'\0')
				break;

			wss << (*ch);

			ch++;
		}

		auto ws = wss.str();

		if (ws.size())
		{
			gPrivateFuncs.GameUI_RichText_InsertStringW(pthis, 0, ws.c_str());
		}

		if ((*ch) == L'\r' || (*ch) == L'\0')
			break;
	}
}

void __fastcall GameUI_RichText_OnThink(void* pthis, int dummy)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	gPrivateFuncs.GameUI_RichText_OnThink(pthis, 0);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;
}

void __fastcall GameUI_TextEntry_LayoutVerticalScrollBarSlider(void* pthis, int dummy)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	gPrivateFuncs.GameUI_TextEntry_LayoutVerticalScrollBarSlider(pthis, 0);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;
}

void __fastcall GameUI_TextEntry_OnKeyCodeTyped(void* pthis, int dummy, vgui::KeyCode code)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	gPrivateFuncs.GameUI_TextEntry_OnKeyCodeTyped(pthis, 0, (int)code);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;
}

int __fastcall GameUI_TextEntry_GetStartDrawIndex(void* pthis, int dummy, int& lineBreakIndexIndex)
{
	vgui::Panel* pPanel = (vgui::Panel*)pthis;
	g_iPatchingPanelTall = pPanel->GetTall();
	g_bPatchingGetFontTall = true;

	int result = gPrivateFuncs.GameUI_TextEntry_GetStartDrawIndex(pthis, 0, lineBreakIndexIndex);

	g_bPatchingGetFontTall = false;
	g_iPatchingPanelTall = 0;

	return result;
}

void __fastcall GameUI_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.GameUI_Panel_Init(pthis, 0, x, y, w, h);

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		auto pPanel = (vgui::IClientPanel*)pthis;
		pPanel->SetProportional(true);
	}
}

void __fastcall GameUI_Panel_SetSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	auto pPanel = (vgui::IClientPanel*)pthis;
	if (pPanel->IsProportional())
	{
		width = g_pVGuiSchemeManager2->GetProportionalScaledValue(width);
		height = g_pVGuiSchemeManager2->GetProportionalScaledValue(height);
	}
	gPrivateFuncs.GameUI_Panel_SetSize(pthis, 0, width, height);
}

void __fastcall GameUI_Panel_SetMinimumSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	auto pPanel = (vgui::IClientPanel*)pthis;
	if (pPanel->IsProportional())
	{
		width = g_pVGuiSchemeManager2->GetProportionalScaledValue(width);
		height = g_pVGuiSchemeManager2->GetProportionalScaledValue(height);
	}
	gPrivateFuncs.GameUI_Panel_SetMinimumSize(pthis, 0, width, height);
}

void __fastcall GameUI_MessageBox_ApplySchemeSettings_Panel_SetSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	auto pPanel = (vgui::IClientPanel*)pthis;

	if (pPanel->IsProportional())
	{
		int basewidth = width - 100;
		int baseheight = height - 100;

		width = basewidth + g_pVGuiSchemeManager2->GetProportionalScaledValue(100);
		height = baseheight + g_pVGuiSchemeManager2->GetProportionalScaledValue(100);

		return gPrivateFuncs.GameUI_Panel_SetSize(pthis, 0, width, height);
	}

	return gPrivateFuncs.GameUI_Panel_SetSize(pthis, 0, width, height);
}

class CScrollBar_Legacy : public vgui::IClientPanel
{
public:
	virtual void    SetValue(int value) = 0;//134
	virtual int     GetValue() = 0;//135
	virtual void    SetRange(int min, int max) = 0;//136
	virtual void    GetRange(int& min, int& max) = 0;//137
	virtual void    SetRangeWindow(int rangeWindow) = 0;//138
	virtual int    GetRangeWindow() = 0;//139
	virtual bool    IsVertical() = 0;//140
	virtual bool    HasFullRange() = 0;//141
	virtual void    SetButton(vgui::Panel* button, int index) = 0;
	virtual vgui::Panel* GetButton(int index) = 0;
	virtual void    SetSlider(vgui::Panel* slider) = 0;
	virtual vgui::Panel* GetSlider() = 0;
	virtual void    SetButtonPressedScrollValue(int value) = 0;
	virtual void    Validate() = 0;
};

class CMenu_Legacy
{
public:
	int 			m_iMenuItemHeight;
	int 			m_iFixedWidth;
	int 			m_iMinimumWidth; // a minimum width the menu has to be if it is not fixed width
	int 			m_iNumVisibleLines;	// number of items in menu before scroll bar adds on
	CScrollBar_Legacy* m_pScroller;

	CUtlLinkedList<vgui::Panel*, int> 	m_MenuItems;
	CUtlVector<int>					m_SortedItems;
};

class CMenu_HL25
{
public:
	int 			m_iMenuItemHeight;
	int 			m_iFixedWidth;
	int 			m_iMinimumWidth; // a minimum width the menu has to be if it is not fixed width
	int 			m_iNumVisibleLines;	// number of items in menu before scroll bar adds on
	int				m_iMenuItemBlurOffset;
	CScrollBar_Legacy* m_pScroller;

	CUtlLinkedList<vgui::Panel*, int> 	m_MenuItems;
	CUtlVector<int>					m_SortedItems;
};

template<class T>
void GameUI_Menu_MakeItemsVisibleInScrollRange_Template(vgui::Panel* pthis)
{
	T* pMenu = (T*)((PUCHAR)pthis + gPrivateFuncs.offset_ScrollBar - offsetof(T, m_pScroller));

	for (int i = 0; i < pMenu->m_MenuItems.Count(); i++)
	{
		pMenu->m_MenuItems[i]->SetVisible(false);
	}

	int count = 0;
	int startItem = pMenu->m_pScroller->GetValue();
	do
	{
		for (int i = startItem; count < pMenu->m_iNumVisibleLines && i < pMenu->m_SortedItems.Count(); i++)
		{
			auto iItemIndex = pMenu->m_SortedItems[i];
			if (iItemIndex < pMenu->m_MenuItems.Count())
			{
				pMenu->m_MenuItems[iItemIndex]->SetVisible(true);
				count++;
			}
		}

		if (count < pMenu->m_iNumVisibleLines)
		{
			startItem--;  // scroll up 
			count = 0;
		}

	} while (count < pMenu->m_iNumVisibleLines - 1);
}

void __fastcall GameUI_Menu_MakeItemsVisibleInScrollRange(vgui::Panel* pthis, int dummy)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		return GameUI_Menu_MakeItemsVisibleInScrollRange_Template<CMenu_HL25>(pthis);
	}

	return GameUI_Menu_MakeItemsVisibleInScrollRange_Template<CMenu_Legacy>(pthis);
}

/*
====================================================================
GameUI control hooks
====================================================================
*/

void* __fastcall CCareerProfileFrame_ctor(vgui::Panel* pthis, int dummy, void* parent)
{
	bool bOriginal = vgui::surface()->IsForcingHDProportional();
	vgui::surface()->SetForcingHDProportional(false);

	auto r = gPrivateFuncs.CCareerProfileFrame_ctor(pthis, dummy, parent);

	vgui::surface()->SetForcingHDProportional(bOriginal);

	return r;
}

void* __fastcall CCareerMapFrame_ctor(vgui::Panel* pthis, int dummy, void* parent)
{
	bool bOriginal = vgui::surface()->IsForcingHDProportional();
	vgui::surface()->SetForcingHDProportional(false);

	auto r = gPrivateFuncs.CCareerMapFrame_ctor(pthis, dummy, parent);

	vgui::surface()->SetForcingHDProportional(bOriginal);

	return r;
}

void* __fastcall CCareerBotFrame_ctor(vgui::Panel* pthis, int dummy, void* parent)
{
	bool bOriginal = vgui::surface()->IsForcingHDProportional();
	vgui::surface()->SetForcingHDProportional(false);

	auto r = gPrivateFuncs.CCareerBotFrame_ctor(pthis, dummy, parent);

	vgui::surface()->SetForcingHDProportional(bOriginal);

	return r;
}

void __fastcall COptionsSubVideo_ApplyVidSettings(vgui::Panel* pthis, int dummy, bool bForceRestart)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.COptionsSubVideo_ApplyVidSettings(_this, dummy, bForceRestart);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);
	}
}

//Fix GameUI_FocusNavGroup_GetCurrentFocus crash with nullptr access.
void* __fastcall GameUI_FocusNavGroup_GetCurrentFocus(void* pthis, int dummy)
{
	vgui::VPanelHandle* _currentFocus = (vgui::VPanelHandle*)((PUCHAR)pthis + gPrivateFuncs.offset_currentFocus);

	auto vpanel = _currentFocus->Get();

	if (vpanel)
	{
		auto pPanel = vgui::ipanel()->GetPanel(vpanel, "GameUI");

		if (!pPanel)
		{
			for (int i = 0; i < VGUI2ExtensionInternal()->GameUI_GetCallbackCount(); ++i)
			{
				pPanel = vgui::ipanel()->GetPanel(vpanel, VGUI2ExtensionInternal()->GameUI_GetControlModuleName(i));

				if (pPanel)
					break;
			}
		}

		return pPanel;
	}

	return NULL;
}

class CPropertySheet_Legacy
{
public:
	vgui::Dar_Legacy<vgui::Panel*> _pages;
	vgui::Dar_Legacy<vgui::Panel*> _pageTabs;
	vgui::Panel* _activePage;
	vgui::Panel* _activeTab;
	int _tabWidth;
	int _activeTabIndex;
	bool _showTabs;
	vgui::Panel* _combo;
	bool _tabFocus;
};

void __fastcall GameUI_PropertySheet_PerformLayout(vgui::Panel* pthis, int dummy)
{
	int offset_activePage = gPrivateFuncs.offset_activePage;

	gPrivateFuncs.GameUI_PropertySheet_PerformLayout(pthis, dummy);

	//TODO: gamedata for _pageTabs; _activePage alone does not describe the array layout.
	auto pPropertySheet = (CPropertySheet_Legacy*)((PUCHAR)pthis + offset_activePage - offsetof(CPropertySheet_Legacy, _activePage));

	int xtab = 0;
	int limit = pPropertySheet->_pageTabs.GetCount();

	for (int i = 0; i < limit; i++)
	{
		int width, tall;

		pPropertySheet->_pageTabs[i]->GetSize(width, tall);
		xtab += (width + 1);
	}

	auto parent = pthis->GetParent();

	if (parent)
	{
		int w, h;
		parent->GetSize(w, h);

		if (w < xtab)
		{
			parent->SetSize(xtab, h);
		}
	}
}

void* __fastcall GameUI_PropertySheet_HasHotkey(void* pthis, int dummy, wchar_t key)
{
	auto _activePage = *(vgui::Panel**)((PUCHAR)pthis + gPrivateFuncs.offset_activePage);

	if (!_activePage)
		return 0;

	int childCount = _activePage->GetChildCount();

	for (int i = 0; i < childCount; i++)
	{
		auto pChild = _activePage->GetChild(i);
#if 0
		if (!pChild)
		{
			pChild = _activePage->GetChildWithModuleName(i, "GameUI");
		}
		if (!pChild)
		{
			pChild = _activePage->GetChildWithModuleName(i, "CaptionMod");
		}
#endif
		if (pChild)
		{
			auto hot = pChild->HasHotkey(key);

			if (hot)
			{
				return (void*)hot;
			}
		}
	}

	return NULL;
}

class CGameUIOptionsDialogCtorCallbackContext : public IGameUIOptionsDialogCtorCallbackContext
{
public:
	CGameUIOptionsDialogCtorCallbackContext(vgui::Panel* pthis, vgui::Panel* _propertySheet)
	{
		m_pDialog = pthis;
		m_pPropertySheet = _propertySheet;
	}

	void InstallHooks()
	{
		if (!gPrivateFuncs.GameUI_FocusNavGroup_GetCurrentFocus)
		{
			gPrivateFuncs.offset_currentFocus = GamedataResolveStructMember(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::FocusNavGroup._currentFocus");
			gPrivateFuncs.GameUI_FocusNavGroup_GetCurrentFocus = (decltype(gPrivateFuncs.GameUI_FocusNavGroup_GetCurrentFocus))
				GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::FocusNavGroup::GetCurrentFocus()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
			Install_InlineHook(GameUI_FocusNavGroup_GetCurrentFocus);
		}

		//PropertySheet hooks are resolved and installed by GameUI_InstallHooks.
	}

	void* GetDialog() const override
	{
		return m_pDialog;
	}

	void* GetPropertySheet() const override
	{
		return m_pPropertySheet;
	}

	void AddPage(void* panel, const char* title) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnAddPage)(vgui::Panel * pthis, int dummy, vgui::Panel * panel, const char* title) =
			(decltype(pfnAddPage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::AddPage(vgui2::Panel*, char const*)")];

		pfnAddPage(m_pPropertySheet, 0, (vgui::Panel*)panel, title);
	}

	void SetActivePage(void* panel) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnSetActivePage)(vgui::Panel * pthis, int dummy, vgui::Panel * panel) =
			(decltype(pfnSetActivePage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::SetActivePage(vgui2::Panel*)")];

		pfnSetActivePage(m_pPropertySheet, 0, (vgui::Panel*)panel);
	}

	void SetTabWidth(int width) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnSetTabWidth)(vgui::Panel * pthis, int dummy, int width) =
			(decltype(pfnSetTabWidth))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::SetTabWidth(int)")];

		pfnSetTabWidth(m_pPropertySheet, 0, width);
	}

	void* GetActivePage() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void* (__fastcall * pfnGetActivePage)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetActivePage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetActivePage()")];

		return pfnGetActivePage(m_pPropertySheet, 0);
	}

	void ResetAllData() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnResetAllData)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnResetAllData))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::ResetAllData()")];

		pfnResetAllData(m_pPropertySheet, 0);
	}

	void ApplyChanges() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnApplyChanges)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnApplyChanges))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::ApplyChanges()")];

		pfnApplyChanges(m_pPropertySheet, 0);
	}

	void* GetPage(int i) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void* (__fastcall * pfnGetPage)(vgui::Panel * pthis, int dummy, int i) =
			(decltype(pfnGetPage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetPage(int)")];

		return pfnGetPage(m_pPropertySheet, 0, i);
	}

	void DeletePage(void* panel) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnDeletePage)(vgui::Panel * pthis, int dummy, void* panel) =
			(decltype(pfnDeletePage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::DeletePage(vgui2::Panel*)")];

		pfnDeletePage(m_pPropertySheet, 0, panel);
	}

	void* GetActiveTab() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void* (__fastcall * pfnGetActiveTab)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetActiveTab))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetActiveTab()")];

		return pfnGetActiveTab(m_pPropertySheet, 0);
	}

	void GetActiveTabTitle(char* textOut, int bufferLen) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnGetActiveTabTitle)(vgui::Panel * pthis, int dummy, char* textOut, int bufferLen) =
			(decltype(pfnGetActiveTabTitle))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetActiveTabTitle(char*, int)")];

		pfnGetActiveTabTitle(m_pPropertySheet, 0, textOut, bufferLen);
	}

	bool GetTabTitle(int i, char* textOut, int bufferLen) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		bool(__fastcall * pfnGetTabTitle)(vgui::Panel * pthis, int dummy, int i, char* textOut, int bufferLen) =
			(decltype(pfnGetTabTitle))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetTabTitle(int, char*, int)")];

		return pfnGetTabTitle(m_pPropertySheet, 0, i, textOut, bufferLen);
	}

	int GetActivePageNum() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		int(__fastcall * pfnGetActivePageNum)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetActivePageNum))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetActivePageNum()")];

		return pfnGetActivePageNum(m_pPropertySheet, 0);
	}

	int GetNumPages() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		int(__fastcall * pfnGetGetNumPages)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetGetNumPages))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::GetNumPages()")];

		return pfnGetGetNumPages(m_pPropertySheet, 0);
	}

	void DisablePage(const char* title) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnDisablePage)(vgui::Panel * pthis, int dummy, const char* title) =
			(decltype(pfnDisablePage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::DisablePage(char const*)")];

		pfnDisablePage(m_pPropertySheet, 0, title);
	}

	void EnablePage(const char* title) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnEnablePage)(vgui::Panel * pthis, int dummy, const char* title) =
			(decltype(pfnEnablePage))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::EnablePage(char const*)")];

		pfnEnablePage(m_pPropertySheet, 0, title);
	}

	void ChangeActiveTab(int index) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnChangeActiveTab)(vgui::Panel * pthis, int dummy, int index) =
			(decltype(pfnChangeActiveTab))_propertySheet_vftable[GamedataResolveVFuncIndex(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::ChangeActiveTab(int)")];

		pfnChangeActiveTab(m_pPropertySheet, 0, index);
	}

	vgui::Panel* m_pDialog;
	vgui::Panel* m_pPropertySheet;
};

void* __fastcall COptionsDialog_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.COptionsDialog_ctor(pthis, dummy, parent);

	vgui::Panel* _propertySheet = *(vgui::Panel**)((PUCHAR)pthis + gPrivateFuncs.offset_propertySheet);

	CGameUIOptionsDialogCtorCallbackContext CallbackContext(pthis, _propertySheet);

	CallbackContext.InstallHooks();

	VGUI2ExtensionInternal()->GameUI_COptionsDialog_ctor(&CallbackContext);

	//Load res to make it proportional
	LOAD_CONTROL_SETTINGS_FALLBACK(GameUI, pthis, "OptionsDialog.res");

	return result;
}

//hl-10210 inlined COptionsSubVideo::ApplyVidSettings into the OnApplyChanges
//virtual, so that identity installs no standalone ApplyVidSettings hook. Deliver
//the callback from the virtual instead, bracketing its original call the way the
//standalone hook brackets ApplyVidSettings, so plugins observe it exactly once on
//every identity. Everywhere else the standalone hook still delivers it, from
//inside this same virtual.
static void COptionsSubVideo_DispatchOnApplyChanges(void* _this)
{
	if (gPrivateFuncs.COptionsSubVideo_ApplyVidSettings)
	{
		gPrivateFuncs.COptionsSubVideo_OnApplyChanges(_this, 0);
	}
	else
	{
		VGUI2Extension_CallbackContext CallbackContext;

		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = false;

		//The inlined host takes no bForceRestart, so the value the standalone
		//signature would have received is unknowable here.
		bool bForceRestart = false;

		VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);

		if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
		{
			gPrivateFuncs.COptionsSubVideo_OnApplyChanges(_this, 0);
		}

		if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
		{
			CallbackContext.Result = VGUI2Extension_Result::UNSET;
			CallbackContext.IsPost = true;

			VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);
		}
	}
}

void __fastcall COptionsSubVideo_OnApplyChanges(void* pthis, int dummy)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_COptionsSubPage_OnApplyChanges(_this, "COptionsSubVideo", &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		COptionsSubVideo_DispatchOnApplyChanges(_this);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_COptionsSubPage_OnApplyChanges(_this, "COptionsSubVideo", &CallbackContext);
	}
}

class CGameUIOptionsDialogSubVideoCtorCallbackContext : public IGameUIOptionsDialogSubPageCtorCallbackContext
{
public:
	CGameUIOptionsDialogSubVideoCtorCallbackContext(vgui::Panel* pthis, vgui::Panel* parent)
	{
		m_pPage = pthis;
	}
	void InstallHooks()
	{
		if (!gPrivateFuncs.COptionsSubVideo_OnApplyChanges)
		{
			gPrivateFuncs.COptionsSubVideo_OnApplyChanges = (decltype(gPrivateFuncs.COptionsSubVideo_OnApplyChanges))
				GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "COptionsSubVideo::OnApplyChanges()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
			Install_InlineHook(COptionsSubVideo_OnApplyChanges);
		}
	}
	void* GetPage() const override
	{
		return m_pPage;
	}
	const char* GetName() const override
	{
		return "COptionsSubVideo";
	}

	void* m_pPage;
};

void* __fastcall COptionsSubVideo_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.COptionsSubVideo_ctor(pthis, dummy, parent);

	CGameUIOptionsDialogSubVideoCtorCallbackContext CallbackContext(pthis, parent);

	CallbackContext.InstallHooks();

	VGUI2ExtensionInternal()->GameUI_COptionsDialogSubPage_ctor(&CallbackContext);

	return result;
}

void __fastcall COptionsSubAudio_OnApplyChanges(void* pthis, int dummy)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_COptionsSubPage_OnApplyChanges(_this, "COptionsSubAudio", &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.COptionsSubAudio_OnApplyChanges(_this, dummy);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_COptionsSubPage_OnApplyChanges(_this, "COptionsSubAudio", &CallbackContext);
	}
}

class CGameUIOptionsDialogSubAudioCtorCallbackContext : public IGameUIOptionsDialogSubPageCtorCallbackContext
{
public:
	CGameUIOptionsDialogSubAudioCtorCallbackContext(vgui::Panel* pthis, vgui::Panel* parent)
	{
		m_pPage = pthis;
	}
	void InstallHooks()
	{
		if (!gPrivateFuncs.COptionsSubAudio_OnApplyChanges)
		{
			gPrivateFuncs.COptionsSubAudio_OnApplyChanges = (decltype(gPrivateFuncs.COptionsSubAudio_OnApplyChanges))
				GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "COptionsSubAudio::OnApplyChanges()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
			Install_InlineHook(COptionsSubAudio_OnApplyChanges);
		}
	}
	void* GetPage() const override
	{
		return m_pPage;
	}
	const char* GetName() const override
	{
		return "COptionsSubAudio";
	}

	void* m_pPage;
};

void* __fastcall COptionsSubAudio_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.COptionsSubAudio_ctor(pthis, dummy, parent);

	CGameUIOptionsDialogSubAudioCtorCallbackContext CallbackContext(pthis, parent);

	CallbackContext.InstallHooks();

	VGUI2ExtensionInternal()->GameUI_COptionsDialogSubPage_ctor(&CallbackContext);

	return result;
}

void __fastcall COptionsSubMultiplayer_OnApplyChanges(void* pthis, int dummy)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_COptionsSubPage_OnApplyChanges(_this, "COptionsSubMultiplayer", &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.COptionsSubMultiplayer_OnApplyChanges(_this, dummy);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_COptionsSubPage_OnApplyChanges(_this, "COptionsSubMultiplayer", &CallbackContext);
	}
}

class CGameUIOptionsDialogSubMultiplayerCtorCallbackContext : public IGameUIOptionsDialogSubPageCtorCallbackContext
{
public:
	CGameUIOptionsDialogSubMultiplayerCtorCallbackContext(vgui::Panel* pthis, vgui::Panel* parent)
	{
		m_pPage = pthis;
	}
	void InstallHooks()
	{
		if (!gPrivateFuncs.COptionsSubMultiplayer_OnApplyChanges)
		{
			gPrivateFuncs.COptionsSubMultiplayer_OnApplyChanges = (decltype(gPrivateFuncs.COptionsSubMultiplayer_OnApplyChanges))
				GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "COptionsSubMultiplayer::OnApplyChanges()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
			Install_InlineHook(COptionsSubMultiplayer_OnApplyChanges);
		}
	}
	void* GetPage() const override
	{
		return m_pPage;
	}
	const char* GetName() const override
	{
		return "COptionsSubMultiplayer";
	}

	void* m_pPage;
};

void* __fastcall COptionsSubMultiplayer_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.COptionsSubMultiplayer_ctor(pthis, dummy, parent);

	CGameUIOptionsDialogSubMultiplayerCtorCallbackContext CallbackContext(pthis, parent);

	CallbackContext.InstallHooks();

	VGUI2ExtensionInternal()->GameUI_COptionsDialogSubPage_ctor(&CallbackContext);

	return result;
}

void* __fastcall CCreateMultiplayerGameDialog_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto result = gPrivateFuncs.CCreateMultiplayerGameDialog_ctor(pthis, dummy, parent);

	//Load res to make it proportional
	LOAD_CONTROL_SETTINGS_FALLBACK(GameUI, pthis, "CreateMultiplayerGameDialog.res");

	return result;
}

static vgui::Panel* g_pCreatingGameConsoleDialog = NULL;
static int g_iCreatingGameConsoleDialogX = 0;
static int g_iCreatingGameConsoleDialogY = 0;
static int g_iCreatingGameConsoleDialogWidth = 0;
static int g_iCreatingGameConsoleDialogHeight = 0;

void* __fastcall CGameConsoleDialog_ctor(vgui::Panel* pthis, int dummy)
{
	auto result = gPrivateFuncs.CGameConsoleDialog_ctor(pthis, dummy);

	//Load res to make it proportional
	LOAD_CONTROL_SETTINGS_FALLBACK(GameUI, pthis, "GameConsoleDialog.res");

	g_pCreatingGameConsoleDialog = pthis;

	pthis->GetBounds(g_iCreatingGameConsoleDialogX, g_iCreatingGameConsoleDialogY, g_iCreatingGameConsoleDialogWidth, g_iCreatingGameConsoleDialogHeight);

	return result;
}

void __fastcall CTaskBar_OnCommand(void* pthis, int dummy, const char* command)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_CTaskBar_OnCommand(_this, command, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.CTaskBar_OnCommand(_this, dummy, command);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_CTaskBar_OnCommand(_this, command, &CallbackContext);
	}
}

class CGameUITaskBarCtorCallbackContext : public IGameUITaskBarCtorCallbackContext
{
public:
	CGameUITaskBarCtorCallbackContext(vgui::Panel* pthis, vgui::Panel* parent, const char* panelName)
	{
		m_pTaskBar = pthis;
		m_pParentPanel = parent;
		m_pszPanelName = panelName;
	}

	void* GetTaskBar() const override
	{
		return m_pTaskBar;
	}

	void* GetParentPanel() const override
	{
		return m_pParentPanel;
	}

	const char* GetParentName() const override
	{
		return m_pszPanelName;
	}

	vgui::Panel* m_pTaskBar;
	vgui::Panel* m_pParentPanel;
	const char* m_pszPanelName;
};

void* __fastcall CTaskBar_ctor(void* pthis, int dummy, void* parent, const char* panelName)
{
	auto result = gPrivateFuncs.CTaskBar_ctor(pthis, dummy, parent, panelName);

	CGameUITaskBarCtorCallbackContext CallbackContext((vgui::Panel*)pthis, (vgui::Panel*)parent, panelName);

	VGUI2ExtensionInternal()->GameUI_CTaskBar_ctor(&CallbackContext);

	return result;
}

void __fastcall CBasePanel_ApplySchemeSettings(void* pthis, int dummy, void* pScheme)
{
	void* _this = pthis;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_CBasePanel_ApplySchemeSettings(_this, pScheme, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.CBasePanel_ApplySchemeSettings(_this, dummy, pScheme);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_CBasePanel_ApplySchemeSettings(_this, pScheme, &CallbackContext);
	}
}

class CGameUIBasePanelCtorCallbackContext : public IGameUIBasePanelCtorCallbackContext
{
public:
	CGameUIBasePanelCtorCallbackContext(vgui::Panel* pthis)
	{
		m_pBasePanel = pthis;
	}

	void InstallHooks()
	{
		if (!gPrivateFuncs.CBasePanel_ApplySchemeSettings)
		{
			gPrivateFuncs.CBasePanel_ApplySchemeSettings = (decltype(gPrivateFuncs.CBasePanel_ApplySchemeSettings))
				GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "CBasePanel::ApplySchemeSettings(vgui2::IScheme*)", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

			Install_InlineHook(CBasePanel_ApplySchemeSettings);
		}
	}

	void* GetBasePanel() const
	{
		return m_pBasePanel;
	}

	vgui::Panel* m_pBasePanel;
};

void* __fastcall CBasePanel_ctor(void* pthis, int dummy)
{
	auto result = gPrivateFuncs.CBasePanel_ctor(pthis, dummy);

	CGameUIBasePanelCtorCallbackContext CallbackContext((vgui::Panel*)pthis);

	CallbackContext.InstallHooks();

	VGUI2ExtensionInternal()->GameUI_CBasePanel_ctor(&CallbackContext);

	return result;
}

bool __fastcall GameUI_KeyValues_LoadFromFile(void* pthis, int dummy, IFileSystem* pFileSystem, const char* resourceName, const char* pathId)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, "GameUI", &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = gPrivateFuncs.GameUI_KeyValues_LoadFromFile(pthis, dummy, pFileSystem, resourceName, pathId);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, "GameUI", &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}


/*
==================================================================================
IGameUI hook
==================================================================================
*/

IGameUI* g_pGameUI = NULL;

static void(__fastcall* g_pfnCGameUI_Initialize)(void* pthis, int edx, CreateInterfaceFn* factories, int count) = 0;
static void(__fastcall* g_pfnCGameUI_Start)(void* pthis, int edx, struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) = 0;
static void(__fastcall* g_pfnCGameUI_Shutdown)(void* pthis, int edx) = 0;
static int(__fastcall* g_pfnCGameUI_ActivateGameUI)(void* pthis, int edx) = 0;
static int(__fastcall* g_pfnCGameUI_ActivateDemoUI)(void* pthis, int edx) = 0;
static int(__fastcall* g_pfnCGameUI_HasExclusiveInput)(void* pthis, int edx) = 0;
static void(__fastcall* g_pfnCGameUI_RunFrame)(void* pthis, int edx) = 0;
static void(__fastcall* g_pfnCGameUI_ConnectToServer)(void* pthis, int edx, const char* game, int IP, int port) = 0;
static void(__fastcall* g_pfnCGameUI_DisconnectFromServer)(void* pthis, int edx) = 0;
static void(__fastcall* g_pfnCGameUI_HideGameUI)(void* pthis, int edx) = 0;
static bool(__fastcall* g_pfnCGameUI_IsGameUIActive)(void* pthis, int edx) = 0;
static void(__fastcall* g_pfnCGameUI_LoadingStarted)(void* pthis, int edx, const char* resourceType, const char* resourceName) = 0;
static void(__fastcall* g_pfnCGameUI_LoadingFinished)(void* pthis, int edx, const char* resourceType, const char* resourceName) = 0;
static void(__fastcall* g_pfnCGameUI_StartProgressBar)(void* pthis, int edx, const char* progressType, int progressSteps) = 0;
static int(__fastcall* g_pfnCGameUI_ContinueProgressBar)(void* pthis, int edx, int progressPoint, float progressFraction) = 0;
static void(__fastcall* g_pfnCGameUI_StopProgressBar)(void* pthis, int edx, bool bError, const char* failureReason, const char* extendedReason) = 0;
static int(__fastcall* g_pfnCGameUI_SetProgressBarStatusText)(void* pthis, int edx, const char* statusText) = 0;
static void(__fastcall* g_pfnCGameUI_SetSecondaryProgressBar)(void* pthis, int edx, float progress) = 0;
static void(__fastcall* g_pfnCGameUI_SetSecondaryProgressBarText)(void* pthis, int edx, const char* statusText) = 0;

class CGameUIProxy : public IGameUI
{
public:
	void Initialize(CreateInterfaceFn* factories, int count) override;
	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override;
	void Shutdown(void) override;
	int ActivateGameUI(void) override;
	int ActivateDemoUI(void) override;
	int HasExclusiveInput(void) override;
	void RunFrame(void) override;
	void ConnectToServer(const char* game, int IP, int port) override;
	void DisconnectFromServer(void) override;
	void HideGameUI(void) override;
	bool IsGameUIActive(void) override;
	void LoadingStarted(const char* resourceType, const char* resourceName) override;
	void LoadingFinished(const char* resourceType, const char* resourceName) override;
	void StartProgressBar(const char* progressType, int progressSteps) override;
	int ContinueProgressBar(int progressPoint, float progressFraction) override;
	void StopProgressBar(bool bError, const char* failureReason, const char* extendedReason) override;
	int SetProgressBarStatusText(const char* statusText) override;
	void SetSecondaryProgressBar(float progress) override;
	void SetSecondaryProgressBarText(const char* statusText) override;
};

static CGameUIProxy s_GameUIProxy;

void CGameUIProxy::Initialize(CreateInterfaceFn* factories, int count)
{
	g_pfnCGameUI_Initialize(this, 0, factories, count);

	if (!vgui::VGui_InitInterfacesList("VGUI2Extension", factories, count))
	{
		Sys_Error("Failed to VGui_InitInterfacesList");
		return;
	}

	VGUI2ExtensionInternal()->GameUI_Initialize(factories, count);
}

void CGameUIProxy::Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system)
{
	VGUI2ExtensionInternal()->GameUI_PreStart(engineFuncs, interfaceVersion, system);

	g_pfnCGameUI_Start(this, 0, engineFuncs, interfaceVersion, system);

	VGUI2ExtensionInternal()->GameUI_Start(engineFuncs, interfaceVersion, system);
}

void CGameUIProxy::Shutdown(void)
{
	VGUI2ExtensionInternal()->GameUI_Shutdown();

	g_pfnCGameUI_Shutdown(this, 0);

	VGUI2ExtensionInternal()->GameUI_PostShutdown();
}

int CGameUIProxy::ActivateGameUI(void)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_ActivateGameUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_ActivateGameUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->GameUI_ActivateGameUI(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

int CGameUIProxy::ActivateDemoUI(void)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_ActivateDemoUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_ActivateDemoUI(this, 0);
	}

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = true;
	CallbackContext.pRealReturnValue = &real_ret;

	VGUI2ExtensionInternal()->GameUI_ActivateDemoUI(&CallbackContext);

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

int CGameUIProxy::HasExclusiveInput(void)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_HasExclusiveInput(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_HasExclusiveInput(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->GameUI_HasExclusiveInput(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

void CGameUIProxy::RunFrame(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_RunFrame(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_RunFrame(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_RunFrame(&CallbackContext);
	}
}

void CGameUIProxy::ConnectToServer(const char* game, int IP, int port)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_ConnectToServer(game, IP, port, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_ConnectToServer(this, 0, game, IP, port);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_ConnectToServer(game, IP, port, &CallbackContext);
	}
}

void CGameUIProxy::DisconnectFromServer(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_DisconnectFromServer(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_DisconnectFromServer(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_DisconnectFromServer(&CallbackContext);
	}
}

void CGameUIProxy::HideGameUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_HideGameUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_HideGameUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = false;

		VGUI2ExtensionInternal()->GameUI_HideGameUI(&CallbackContext);
	}
}

bool CGameUIProxy::IsGameUIActive(void)
{
	bool fake_ret = 0;
	bool real_ret = 0;
	bool ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_IsGameUIActive(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_IsGameUIActive(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->GameUI_IsGameUIActive(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

void CGameUIProxy::LoadingStarted(const char* resourceType, const char* resourceName)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_LoadingStarted(resourceType, resourceName, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_LoadingStarted(this, 0, resourceType, resourceName);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_LoadingStarted(resourceType, resourceName, &CallbackContext);
	}
}

void CGameUIProxy::LoadingFinished(const char* resourceType, const char* resourceName)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_LoadingFinished(resourceType, resourceName, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_LoadingFinished(this, 0, resourceType, resourceName);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_LoadingFinished(resourceType, resourceName, &CallbackContext);
	}
}

void CGameUIProxy::StartProgressBar(const char* progressType, int progressSteps)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_StartProgressBar(progressType, progressSteps, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_StartProgressBar(this, 0, progressType, progressSteps);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_StartProgressBar(progressType, progressSteps, &CallbackContext);
	}
}

int CGameUIProxy::ContinueProgressBar(int progressPoint, float progressFraction)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_ContinueProgressBar(progressPoint, progressFraction, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_ContinueProgressBar(this, 0, progressPoint, progressFraction);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->GameUI_ContinueProgressBar(progressPoint, progressFraction, &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

void CGameUIProxy::StopProgressBar(bool bError, const char* failureReason, const char* extendedReason)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_StopProgressBar(bError, failureReason, extendedReason, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_StopProgressBar(this, 0, bError, failureReason, extendedReason);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_StopProgressBar(bError, failureReason, extendedReason, &CallbackContext);
	}
}

int CGameUIProxy::SetProgressBarStatusText(const char* statusText)
{
	int fake_ret = 0;
	int real_ret = 0;
	int ret = 0;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameUI_SetProgressBarStatusText(statusText, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameUI_SetProgressBarStatusText(this, 0, statusText);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->GameUI_SetProgressBarStatusText(statusText, &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

void CGameUIProxy::SetSecondaryProgressBar(float progress)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBar(progress, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_SetSecondaryProgressBar(this, 0, progress);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBar(progress, &CallbackContext);
	}
}

void CGameUIProxy::SetSecondaryProgressBarText(const char* statusText)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBarText(statusText, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameUI_SetSecondaryProgressBarText(this, 0, statusText);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_SetSecondaryProgressBarText(statusText, &CallbackContext);
	}
}

/*
==================================================================================
IGameConsole hook
==================================================================================
*/

IGameConsole* g_pGameConsole = NULL;
extern "C"
{
	void(__fastcall* g_pfnCGameConsole_Activate)(void* pthis, int) = NULL;
	void(__fastcall* g_pfnCGameConsole_Initialize)(void* pthis, int) = NULL;
	void(__fastcall* g_pfnCGameConsole_Hide)(void* pthis, int) = NULL;
	void(__fastcall* g_pfnCGameConsole_Clear)(void* pthis, int) = NULL;
	bool(__fastcall* g_pfnCGameConsole_IsConsoleVisible)(void* pthis, int) = NULL;
	void(__cdecl* g_pfnCGameConsole_Printf)(void* pthis, const char* format, ...) = NULL;
	void(__cdecl* g_pfnCGameConsole_DPrintf)(void* pthis, const char* format, ...) = NULL;
	void(__fastcall* g_pfnCGameConsole_SetParent)(void* pthis, int, vgui::VPANEL parent) = NULL;
};

class CGameConsoleProxy : public IGameConsole
{
public:
	void Activate(void) override;
	void Initialize(void) override;
	void Hide(void)  override;
	void Clear(void) override;
	bool IsConsoleVisible(void) override;
	void Printf(const char* format, ...) override;
	void DPrintf(const char* format, ...)  override;
	void SetParent(vgui::VPANEL parent) override;
};

void CGameConsoleProxy::Activate(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_Activate(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_Activate(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_Activate(&CallbackContext);
	}
}

void CGameConsoleProxy::Initialize(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_Initialize(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_Initialize(this, 0);

		if (g_pCreatingGameConsoleDialog)
		{
			int iBaseWidth = (g_pCreatingGameConsoleDialog->IsProportional()) ? g_pVGuiSchemeManager2->GetProportionalScaledValue(100) : 100;
			int iBaseHeight = (g_pCreatingGameConsoleDialog->IsProportional()) ? g_pVGuiSchemeManager2->GetProportionalScaledValue(100) : 100;

			if (g_iCreatingGameConsoleDialogWidth > iBaseWidth &&
				g_iCreatingGameConsoleDialogHeight > iBaseHeight)
			{
				g_pCreatingGameConsoleDialog->SetBounds(
					g_iCreatingGameConsoleDialogX,
					g_iCreatingGameConsoleDialogY,
					g_iCreatingGameConsoleDialogWidth,
					g_iCreatingGameConsoleDialogHeight);
			}
		}

		g_pCreatingGameConsoleDialog = NULL;
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_Initialize(&CallbackContext);
	}
}

void CGameConsoleProxy::Hide(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_Hide(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_Hide(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_Hide(&CallbackContext);
	}
}

void CGameConsoleProxy::Clear(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_Clear(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_Clear(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_Clear(&CallbackContext);
	}
}

bool CGameConsoleProxy::IsConsoleVisible(void)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->GameConsole_IsConsoleVisible(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = g_pfnCGameConsole_IsConsoleVisible(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->GameConsole_IsConsoleVisible(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret;
		break;
	}
	default:
	{
		ret = real_ret;
		break;
	}
	}

	return ret;
}

void CGameConsoleProxy::Printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	// Use vsnprintf to calculate the required length
	int length = vsnprintf(nullptr, 0, format, args);
	va_end(args);

	// Check for error
	if (length <= 0) {
		return;
	}

	// Create a vector with the required size (+1 for the null terminator)
	CVGUI2Extension_String str;

	str.resize(length);

	// Format the string again with the actual buffer
	va_start(args, format);
	vsnprintf((char*)str.c_str(), str.length() + 1, format, args);
	va_end(args);

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_Printf(&str, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_Printf(this, "%s", str.c_str());
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_Printf(&str, &CallbackContext);
	}
}

void CGameConsoleProxy::DPrintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	// Use vsnprintf to calculate the required length
	int length = vsnprintf(nullptr, 0, format, args);
	va_end(args);

	// Check for error
	if (length <= 0) {
		return;
	}

	// Create a vector with the required size (+1 for the null terminator)
	CVGUI2Extension_String str;

	str.resize(length);

	// Format the string again with the actual buffer
	va_start(args, format);
	vsnprintf((char*)str.c_str(), str.length() + 1, format, args);
	va_end(args);

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_DPrintf(&str, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_DPrintf(this, "%s", str.c_str());
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_DPrintf(&str, &CallbackContext);
	}
}

void CGameConsoleProxy::SetParent(vgui::VPANEL parent)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameConsole_SetParent(parent, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		g_pfnCGameConsole_SetParent(this, 0, parent);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameConsole_SetParent(parent, &CallbackContext);
	}
}

static CGameConsoleProxy s_GameConsoleProxy;

/*
======================================================================
End of hook proxy
======================================================================
*/

void GameUI_FillAddress_GameConsoleDialog(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.CGameConsoleDialog_ctor = (decltype(gPrivateFuncs.CGameConsoleDialog_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CGameConsoleDialog::CGameConsoleDialog()", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_CreateMultiplayerGameDialog(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.CCreateMultiplayerGameDialog_ctor = (decltype(gPrivateFuncs.CCreateMultiplayerGameDialog_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CCreateMultiplayerGameDialog::CCreateMultiplayerGameDialog(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_COptionsDialog(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.COptionsDialog_ctor = (decltype(gPrivateFuncs.COptionsDialog_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "COptionsDialog::COptionsDialog(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_CCareerProfileFrame(const mh_dll_info_t& RealDllInfo)
{
	//The career frames only exist in the GameUI.dll that Condition Zero and
	//CZDS share with Half-Life, so the resolve stays behind the CZ check.
	if (g_bIsCZero)
	{
		gPrivateFuncs.CCareerProfileFrame_ctor = (decltype(gPrivateFuncs.CCareerProfileFrame_ctor))
			GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CCareerProfileFrame::CCareerProfileFrame(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void GameUI_FillAddress_CCareerMapFrame(const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsCZero)
	{
		gPrivateFuncs.CCareerMapFrame_ctor = (decltype(gPrivateFuncs.CCareerMapFrame_ctor))
			GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CCareerMapFrame::CCareerMapFrame(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void GameUI_FillAddress_CCareerBotFrame(const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsCZero)
	{
		gPrivateFuncs.CCareerBotFrame_ctor = (decltype(gPrivateFuncs.CCareerBotFrame_ctor))
			GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CCareerBotFrame::CCareerBotFrame(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void GameUI_FillAddress_COptionsSubAudio(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.COptionsSubAudio_ctor = (decltype(gPrivateFuncs.COptionsSubAudio_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "COptionsSubAudio::COptionsSubAudio(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_COptionsSubVideo(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.COptionsSubVideo_ctor = (decltype(gPrivateFuncs.COptionsSubVideo_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "COptionsSubVideo::COptionsSubVideo(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);

	//hl-10210 inlined ApplyVidSettings into the OnApplyChanges() virtual, which the
	//sub-page ctor wrapper already hooks through its vtable slot; that identity is
	//the only one publishing no standalone ApplyVidSettings.
	gPrivateFuncs.COptionsSubVideo_ApplyVidSettings = (decltype(gPrivateFuncs.COptionsSubVideo_ApplyVidSettings))
		GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "gameui", "COptionsSubVideo::ApplyVidSettings(bool)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_COptionsSubMultiplayer_ctor(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.COptionsSubMultiplayer_ctor = (decltype(gPrivateFuncs.COptionsSubMultiplayer_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "COptionsSubMultiplayer::COptionsSubMultiplayer(vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_ConsoleHistory(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GameUI_RichText_OnThink = (decltype(gPrivateFuncs.GameUI_RichText_OnThink))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "vgui2::RichText::OnThink()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
}

void GameUI_FillAddress_RichText(const mh_dll_info_t& RealDllInfo)
{
	//Hook whichever function hosts the carriage-return filter branch: the standalone
	//RichText::InsertChar, or RichText::InsertString(wchar_t const*) on hl-10210, where
	//Valve inlined InsertChar into it and the catalog publishes no InsertChar.
	gPrivateFuncs.GameUI_RichText_InsertChar = (decltype(gPrivateFuncs.GameUI_RichText_InsertChar))
		GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "gameui", "vgui2::RichText::InsertChar(wchar_t)", MH_GAMESYMBOL_KIND_FUNCTION);

	if (!gPrivateFuncs.GameUI_RichText_InsertChar)
	{
		gPrivateFuncs.GameUI_RichText_InsertStringW = (decltype(gPrivateFuncs.GameUI_RichText_InsertStringW))
			GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "vgui2::RichText::InsertString(wchar_t const*)", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void GameUI_PatchAddress_RichText_InsertChar(const mh_dll_info_t& RealDllInfo)
{
	//The patch is the Jcc right after the compare against 0Dh that takes the native '\r' early-out.
	//The catalog only locates the branch, so the rewrite is chosen from its encoding here;
	//the InsertChar / InsertStringW hooks filter carriage returns themselves.
	auto address = (PUCHAR)GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "vgui2::RichText carriage-return filter branch", MH_GAMESYMBOL_KIND_PATCH);

	if (address[0] == 0x74)
	{
		//jz short label_exit
		g_pMetaHookAPI->WriteNOP(address, 2);
	}
	else if (address[0] == 0x0F && address[1] == 0x84)
	{
		//jz label_exit
		g_pMetaHookAPI->WriteNOP(address, 6);
	}
	else if (address[0] == 0x75)
	{
		//jnz short label_work, followed by jmp short label_exit
		g_pMetaHookAPI->WriteBYTE(address, 0xEB);
	}
	else
	{
		Sys_Error("Failed to patch GameUI!RichText_InsertChar: unexpected branch opcode %02X %02X.\nEngine buildnum: %d",
			address[0], address[1], g_dwEngineBuildnum);
	}
}

void GameUI_FillAddress_ConsoleEntry(const mh_dll_info_t&, const mh_dll_info_t&)
{
	//All three are TextEntry virtuals reached through the TabCatchingTextEntry
	//vtable. Resolving them by name removes the previous vftable scan, which
	//accepted the first .rdata pointer whose head landed in .text without
	//verifying it belonged to TabCatchingTextEntry.
	gPrivateFuncs.GameUI_TextEntry_OnKeyCodeTyped = (decltype(gPrivateFuncs.GameUI_TextEntry_OnKeyCodeTyped))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "TabCatchingTextEntry::OnKeyCodeTyped(vgui2::KeyCode)", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	gPrivateFuncs.GameUI_TextEntry_LayoutVerticalScrollBarSlider = (decltype(gPrivateFuncs.GameUI_TextEntry_LayoutVerticalScrollBarSlider))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::TextEntry::LayoutVerticalScrollBarSlider()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	gPrivateFuncs.GameUI_TextEntry_GetStartDrawIndex = (decltype(gPrivateFuncs.GameUI_TextEntry_GetStartDrawIndex))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::TextEntry::GetStartDrawIndex(int&)", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
}

void GameUI_FillAddress_Sheet(const mh_dll_info_t&, const mh_dll_info_t&)
{
	gPrivateFuncs.offset_propertySheet = (decltype(gPrivateFuncs.offset_propertySheet))
		GamedataResolveStructMember(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertyDialog._propertySheet");
}

void GameUI_FillAddress_PropertySheet(const mh_dll_info_t&, const mh_dll_info_t&)
{
	gPrivateFuncs.offset_activePage = GamedataResolveStructMember(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet._activePage");

	gPrivateFuncs.GameUI_PropertySheet_HasHotkey = (decltype(gPrivateFuncs.GameUI_PropertySheet_HasHotkey))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::HasHotkey(wchar_t)", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);

	gPrivateFuncs.GameUI_PropertySheet_PerformLayout = (decltype(gPrivateFuncs.GameUI_PropertySheet_PerformLayout))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::PropertySheet::PerformLayout()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
}

void GameUI_FillAddress_MessageBox(const mh_dll_info_t&, const mh_dll_info_t&)
{
	gPrivateFuncs.MessageBox_ctor = (decltype(gPrivateFuncs.MessageBox_ctor))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::MessageBox::MessageBox(char const*, char const*, vgui2::Panel*)", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.MessageBox_ApplySchemeSettings = (decltype(gPrivateFuncs.MessageBox_ApplySchemeSettings))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::MessageBox::ApplySchemeSettings(vgui2::IScheme*)", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
}

void GameUI_PatchPanelSize(void)
{
	// HL25 already scales these dimensions. SvEngine still uses raw constants.
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return;

	PatchPanelSizeCallsites(g_GameUIDllInfo.ImageBase, "gameui", "vgui2_Panel_SetSize_Const_callsite_",
		GameUI_Panel_SetSize, (PVOID*)&gPrivateFuncs.GameUI_Panel_SetSize);
	PatchPanelSizeCallsites(g_GameUIDllInfo.ImageBase, "gameui", "vgui2_Panel_SetMinimumSize_Const_callsite_",
		GameUI_Panel_SetMinimumSize, (PVOID*)&gPrivateFuncs.GameUI_Panel_SetMinimumSize);

	// MessageBox adds a fixed margin to measured content; scale only that margin.
	{
		const char* symbolName = "vgui2::MessageBox::ApplySchemeSettings to vgui2::Panel::SetSize callsite";
		auto address = GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", symbolName, MH_GAMESYMBOL_KIND_PATCH);
		if (!g_pMetaHookAPI->InlinePatchRedirectBranch(address,
			GameUI_MessageBox_ApplySchemeSettings_Panel_SetSize, (PVOID*)&gPrivateFuncs.GameUI_Panel_SetSize))
		{
			Sys_Error("Could not redirect gamedata patch: %s\nEngine buildnum: %d", symbolName, g_dwEngineBuildnum);
			return;
		}
	}
}

void GameUI_FillAddress_CBasePanel(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.CBasePanel_ctor = (decltype(gPrivateFuncs.CBasePanel_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CBasePanel::CBasePanel()", MH_GAMESYMBOL_KIND_FUNCTION);
}

void GameUI_FillAddress_CTaskBar(const mh_dll_info_t& RealDllInfo)
{
	//hl-10210 publishes the ctor without a func_sig; the resolver only consumes its rva.
	gPrivateFuncs.CTaskBar_ctor = (decltype(gPrivateFuncs.CTaskBar_ctor))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CTaskbar::CTaskbar(vgui2::Panel*, char const*)", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.CTaskBar_OnCommand = (decltype(gPrivateFuncs.CTaskBar_OnCommand))
		GamedataResolvePtr(RealDllInfo.ImageBase, "gameui", "CTaskbar::OnCommand(char const*)", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
}

void GameUI_FillAddress_KeyValues(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.GameUI_KeyValues_LoadFromFile = (decltype(gPrivateFuncs.GameUI_KeyValues_LoadFromFile))
		GamedataResolveKeyValuesLoadFromFileIfAvailable(RealDllInfo.ImageBase, "gameui");

	Sig_FuncNotFound(GameUI_KeyValues_LoadFromFile);
}

void GameUI_FillAddress(void)
{
	if (!g_GameUIDllInfo.ImageBase)
	{
		Sys_Error("GameUI.dll is not available!");
		return;
	}

	GameUI_FillAddress_GameConsoleDialog(g_GameUIDllInfo);

	GameUI_FillAddress_CreateMultiplayerGameDialog(g_GameUIDllInfo);

	GameUI_FillAddress_COptionsDialog(g_GameUIDllInfo);

	GameUI_FillAddress_CCareerProfileFrame(g_GameUIDllInfo);

	GameUI_FillAddress_CCareerMapFrame(g_GameUIDllInfo);

	GameUI_FillAddress_CCareerBotFrame(g_GameUIDllInfo);

	GameUI_FillAddress_COptionsSubAudio(g_GameUIDllInfo);

	GameUI_FillAddress_COptionsSubVideo(g_GameUIDllInfo);

	GameUI_FillAddress_COptionsSubMultiplayer_ctor(g_GameUIDllInfo);

	GameUI_FillAddress_ConsoleHistory(g_GameUIDllInfo);

	GameUI_FillAddress_RichText(g_GameUIDllInfo);

	GameUI_PatchAddress_RichText_InsertChar(g_GameUIDllInfo);

	GameUI_FillAddress_ConsoleEntry(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_Sheet(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_PropertySheet(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_MessageBox(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CBasePanel(g_GameUIDllInfo);

	GameUI_FillAddress_CTaskBar(g_GameUIDllInfo);

	GameUI_FillAddress_KeyValues(g_GameUIDllInfo);

	gPrivateFuncs.GameUI_Panel_Init = (decltype(gPrivateFuncs.GameUI_Panel_Init))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::Panel::Init(int, int, int, int)", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.offset_ScrollBar = GamedataResolveStructMember(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::Menu.m_pScroller");
	//The target GameUI method has no explicit arguments, unlike the newer SDK overload.
	gPrivateFuncs.GameUI_Menu_MakeItemsVisibleInScrollRange = (decltype(gPrivateFuncs.GameUI_Menu_MakeItemsVisibleInScrollRange))
		GamedataResolvePtr(g_GameUIDllInfo.ImageBase, "gameui", "vgui2::Menu::MakeItemsVisibleInScrollRange()", MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION);
}

bool GameUI_HasExclusiveInput()
{
	return g_pGameUI->HasExclusiveInput();
}

void GameUI_InstallHooks(void)
{
	auto hGameUI = g_hGameUI;

	if (!hGameUI)
	{
		Sys_Error("Failed to get GameUI.dll ");
		return;
	}

	CreateInterfaceFn GameUICreateInterface = Sys_GetFactory((HINTERFACEMODULE)hGameUI);

	if (!GameUICreateInterface)
	{
		Sys_Error("Failed to get interface factory from GameUI.dll");
		return;
	}

	g_pGameUI = (IGameUI*)GameUICreateInterface(GAMEUI_INTERFACE_VERSION, 0);

	if (!g_pGameUI)
	{
		Sys_Error("Failed to get interface \"" GAMEUI_INTERFACE_VERSION "\" from GameUI.dll");
		return;
	}

	g_pGameConsole = (IGameConsole*)GameUICreateInterface(GAMECONSOLE_INTERFACE_VERSION_GS, 0);

	if (!g_pGameConsole)
	{
		Sys_Error("Failed to get interface \"" GAMECONSOLE_INTERFACE_VERSION_GS "\" from GameUI.dll");
		return;
	}

	if (1)
	{
		PVOID* ProxyVFTable = *(PVOID**)&s_GameUIProxy;

		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 1, ProxyVFTable[1], (void**)&g_pfnCGameUI_Initialize);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 2, ProxyVFTable[2], (void**)&g_pfnCGameUI_Start);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 3, ProxyVFTable[3], (void**)&g_pfnCGameUI_Shutdown);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 4, ProxyVFTable[4], (void**)&g_pfnCGameUI_ActivateGameUI);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 5, ProxyVFTable[5], (void**)&g_pfnCGameUI_ActivateDemoUI);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 6, ProxyVFTable[6], (void**)&g_pfnCGameUI_HasExclusiveInput);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 7, ProxyVFTable[7], (void**)&g_pfnCGameUI_RunFrame);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 8, ProxyVFTable[8], (void**)&g_pfnCGameUI_ConnectToServer);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 9, ProxyVFTable[9], (void**)&g_pfnCGameUI_DisconnectFromServer);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 10, ProxyVFTable[10], (void**)&g_pfnCGameUI_HideGameUI);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 11, ProxyVFTable[11], (void**)&g_pfnCGameUI_IsGameUIActive);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 12, ProxyVFTable[12], (void**)&g_pfnCGameUI_LoadingStarted);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 13, ProxyVFTable[13], (void**)&g_pfnCGameUI_LoadingFinished);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 14, ProxyVFTable[14], (void**)&g_pfnCGameUI_StartProgressBar);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 15, ProxyVFTable[15], (void**)&g_pfnCGameUI_ContinueProgressBar);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 16, ProxyVFTable[16], (void**)&g_pfnCGameUI_StopProgressBar);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 17, ProxyVFTable[17], (void**)&g_pfnCGameUI_SetProgressBarStatusText);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 18, ProxyVFTable[18], (void**)&g_pfnCGameUI_SetSecondaryProgressBar);
		g_pMetaHookAPI->VFTHook(g_pGameUI, 0, 19, ProxyVFTable[19], (void**)&g_pfnCGameUI_SetSecondaryProgressBarText);
	}

	if (1)
	{
		PVOID* ProxyVFTable = *(PVOID**)&s_GameConsoleProxy;

		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 1, ProxyVFTable[1], (void**)&g_pfnCGameConsole_Activate);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 2, ProxyVFTable[2], (void**)&g_pfnCGameConsole_Initialize);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 3, ProxyVFTable[3], (void**)&g_pfnCGameConsole_Hide);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 4, ProxyVFTable[4], (void**)&g_pfnCGameConsole_Clear);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 5, ProxyVFTable[5], (void**)&g_pfnCGameConsole_IsConsoleVisible);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 6, ProxyVFTable[6], (void**)&g_pfnCGameConsole_Printf);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 7, ProxyVFTable[7], (void**)&g_pfnCGameConsole_DPrintf);
		g_pMetaHookAPI->VFTHook(g_pGameConsole, 0, 8, ProxyVFTable[8], (void**)&g_pfnCGameConsole_SetParent);
	}

	Install_InlineHook(GameUI_Panel_Init);
	Install_InlineHook(CGameConsoleDialog_ctor);
	Install_InlineHook(CCreateMultiplayerGameDialog_ctor);

	if (gPrivateFuncs.CTaskBar_ctor)
	{
		Install_InlineHook(CTaskBar_ctor);
	}

	if (gPrivateFuncs.CTaskBar_OnCommand)
	{
		Install_InlineHook(CTaskBar_OnCommand);
	}

	if (gPrivateFuncs.CBasePanel_ctor)
	{
		Install_InlineHook(CBasePanel_ctor);
	}

	if (gPrivateFuncs.CBasePanel_ApplySchemeSettings)
	{
		Install_InlineHook(CBasePanel_ApplySchemeSettings);
	}

	if (gPrivateFuncs.GameUI_KeyValues_LoadFromFile)
	{
		Install_InlineHook(GameUI_KeyValues_LoadFromFile);
	}

	Install_InlineHook(COptionsDialog_ctor);
	Install_InlineHook(COptionsSubVideo_ctor);
	Install_InlineHook(COptionsSubAudio_ctor);
	Install_InlineHook(COptionsSubMultiplayer_ctor);

	//hl-10210 has no standalone ApplyVidSettings to hook.
	if (gPrivateFuncs.COptionsSubVideo_ApplyVidSettings)
	{
		Install_InlineHook(COptionsSubVideo_ApplyVidSettings);
	}

	if (gPrivateFuncs.GameUI_RichText_InsertChar)
	{
		Install_InlineHook(GameUI_RichText_InsertChar);
	}

	if (gPrivateFuncs.GameUI_RichText_InsertStringW)
	{
		Install_InlineHook(GameUI_RichText_InsertStringW);
	}

	Install_InlineHook(GameUI_RichText_OnThink);
	Install_InlineHook(GameUI_TextEntry_OnKeyCodeTyped);
	Install_InlineHook(GameUI_TextEntry_LayoutVerticalScrollBarSlider);
	Install_InlineHook(GameUI_TextEntry_GetStartDrawIndex);

	if (gPrivateFuncs.GameUI_PropertySheet_HasHotkey)
	{
		Install_InlineHook(GameUI_PropertySheet_HasHotkey);
	}

	if (gPrivateFuncs.GameUI_PropertySheet_PerformLayout)
	{
		Install_InlineHook(GameUI_PropertySheet_PerformLayout);
	}

	if (gPrivateFuncs.GameUI_FocusNavGroup_GetCurrentFocus)
	{
		Install_InlineHook(GameUI_FocusNavGroup_GetCurrentFocus);
	}

	if (gPrivateFuncs.GameUI_Menu_MakeItemsVisibleInScrollRange)
	{
		Install_InlineHook(GameUI_Menu_MakeItemsVisibleInScrollRange);
	}

	if (gPrivateFuncs.CCareerProfileFrame_ctor)
	{
		Install_InlineHook(CCareerProfileFrame_ctor);
	}
	if (gPrivateFuncs.CCareerMapFrame_ctor)
	{
		Install_InlineHook(CCareerMapFrame_ctor);
	}
	if (gPrivateFuncs.CCareerBotFrame_ctor)
	{
		Install_InlineHook(CCareerBotFrame_ctor);
	}

	GameUI_PatchPanelSize();

}

void GameUI_UninstallHooks(void)
{
	Uninstall_Hook(GameUI_Panel_Init);
	Uninstall_Hook(CGameConsoleDialog_ctor);
	Uninstall_Hook(CCreateMultiplayerGameDialog_ctor);

	Uninstall_Hook(CTaskBar_ctor);
	Uninstall_Hook(CTaskBar_OnCommand);

	Uninstall_Hook(CBasePanel_ctor);
	Uninstall_Hook(CBasePanel_ApplySchemeSettings);

	Uninstall_Hook(GameUI_KeyValues_LoadFromFile);

	Uninstall_Hook(COptionsDialog_ctor);
	Uninstall_Hook(COptionsSubVideo_ctor);
	Uninstall_Hook(COptionsSubAudio_ctor);
	Uninstall_Hook(COptionsSubMultiplayer_ctor);
	Uninstall_Hook(COptionsSubVideo_ApplyVidSettings);

	Uninstall_Hook(GameUI_RichText_InsertChar);
	Uninstall_Hook(GameUI_RichText_InsertStringW);
	Uninstall_Hook(GameUI_RichText_OnThink);
	Uninstall_Hook(GameUI_TextEntry_OnKeyCodeTyped);
	Uninstall_Hook(GameUI_TextEntry_LayoutVerticalScrollBarSlider);
	Uninstall_Hook(GameUI_TextEntry_GetStartDrawIndex);

	Uninstall_Hook(GameUI_PropertySheet_HasHotkey);
	Uninstall_Hook(GameUI_PropertySheet_PerformLayout);
	Uninstall_Hook(GameUI_FocusNavGroup_GetCurrentFocus);
	Uninstall_Hook(GameUI_Menu_MakeItemsVisibleInScrollRange)

	Uninstall_Hook(CCareerProfileFrame_ctor);
	Uninstall_Hook(CCareerMapFrame_ctor);
	Uninstall_Hook(CCareerBotFrame_ctor);
}

void ServerBrowser_PatchPanelSize(void)
{
	// HL25 already scales these dimensions. SvEngine still uses raw constants.
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return;

	PatchPanelSizeCallsites(g_ServerBrowserDllInfo.ImageBase, "serverbrowser", "vgui2_Panel_SetSize_Const_callsite_",
		ServerBrowser_Panel_SetSize, (PVOID*)&gPrivateFuncs.ServerBrowser_Panel_SetSize);
	PatchPanelSizeCallsites(g_ServerBrowserDllInfo.ImageBase, "serverbrowser", "vgui2_Panel_SetMinimumSize_Const_callsite_",
		ServerBrowser_Panel_SetMinimumSize, (PVOID*)&gPrivateFuncs.ServerBrowser_Panel_SetMinimumSize);
}

void ServerBrowser_FillAddress_KeyValues(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.ServerBrowser_KeyValues_LoadFromFile = (decltype(gPrivateFuncs.ServerBrowser_KeyValues_LoadFromFile))
		GamedataResolveKeyValuesLoadFromFileIfAvailable(RealDllInfo.ImageBase, "serverbrowser");
	Sig_FuncNotFound(ServerBrowser_KeyValues_LoadFromFile);
}

void ServerBrowser_FillAddress_PanelInit(void)
{
	gPrivateFuncs.ServerBrowser_Panel_Init = (decltype(gPrivateFuncs.ServerBrowser_Panel_Init))
		GamedataResolvePtr(g_ServerBrowserDllInfo.ImageBase, "serverbrowser", "vgui2::Panel::Init(int, int, int, int)", MH_GAMESYMBOL_KIND_FUNCTION);
}

void ServerBrowser_FillAddress(void)
{
	if (!g_ServerBrowserDllInfo.ImageBase)
	{
		Sys_Error("ServerBrowser.dll is not available!");
		return;
	}

	ServerBrowser_FillAddress_KeyValues(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);

	ServerBrowser_FillAddress_PanelInit();
}

void ServerBrowser_InstallHooks(void)
{
	ServerBrowser_PatchPanelSize();

	Install_InlineHook(ServerBrowser_Panel_Init);
	Install_InlineHook(ServerBrowser_KeyValues_LoadFromFile);
}

void ServerBrowser_UninstallHooks(void)
{
	Uninstall_Hook(ServerBrowser_Panel_Init);
	Uninstall_Hook(ServerBrowser_KeyValues_LoadFromFile);
}

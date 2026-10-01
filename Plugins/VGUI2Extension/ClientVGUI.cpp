#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/IMouseControl.h>
#include <vgui.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <VGUI_controls/BuildGroup.h>
#include <IClientVGUI.h>
#include <ICounterStrikeViewport.h>
#include "plugins.h"
#include "privatefuncs.h"
#include "exportfuncs.h"

#include "DpiManagerInternal.h"
#include "VGUI2ExtensionInternal.h"

#include <sstream>

namespace vgui
{
	bool VGui_InitInterfacesList(const char* moduleName, CreateInterfaceFn* factoryList, int numFactories);
}

extern vgui::ISurface* g_pSurface;
extern vgui::ISurface_HL25* g_pSurface_HL25;

bool g_NativeClientHasVGUI1 = true;
bool g_IsNativeClientVGUI2 = false;
bool g_IsNativeClientUIHDProportional = false;

IClientVGUI* g_pClientVGUI = NULL;

static vgui::Panel* g_pCSBackGroundPanel = NULL;
static vgui::Panel* g_pWorldMapPanel = NULL;
static vgui::Panel* g_pWorldMapMissionSelectPanel = NULL;

//True only for the duration of CWorldMap::PaintBackground and
//CWorldMapMissionSelect::PaintBackground, so the ISurface::GetScreenSize override
//in SurfaceHook.cpp stays scoped to those two CZDS panels.
bool g_bIsPaintWorldMapBackground = false;

//True only for the duration of CTeamMenu::LoadMapPage, so the RichText::SetText
//sanitizer stays scoped to the one caller that trashes Chinese map descriptions.
static bool g_bIsLoadMapPage = false;

static hook_t* g_phook_ClientVGUI_Panel_Init = NULL;
static hook_t* g_phook_ClientVGUI_KeyValues_LoadFromFile = NULL;
static hook_t* g_phook_ClientVGUI_LoadControlSettings = NULL;
static hook_t* g_phook_ClientVGUI_RichText_SetTextW = NULL;
static hook_t* g_phook_TeamMenu_LoadMapPage = NULL;

static void(__fastcall* m_pfnCClientVGUI_Initialize)(void* pthis, int, CreateInterfaceFn* factories, int count) = NULL;
static void(__fastcall* m_pfnCClientVGUI_Start)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCClientVGUI_SetParent)(void* pthis, int, vgui::VPANEL parent) = NULL;
static bool(__fastcall* m_pfnCClientVGUI_UseVGUI1)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCClientVGUI_HideScoreBoard)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCClientVGUI_HideAllVGUIMenu)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCClientVGUI_ActivateClientUI)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCClientVGUI_HideClientUI)(void* pthis, int) = NULL;

bool ClientVGUI_NativeClientHasVGUI1()
{
	return g_NativeClientHasVGUI1;
}

/*
============================================================
ClientVGUI inline hook
============================================================
*/

//Valve populate SetTextW with invalid chars.
void __fastcall ClientVGUI_RichText_SetTextW(void* pthis, int dummy, const wchar_t* text)
{
	if (g_bIsLoadMapPage && !strcmp(GetCurrentGameLanguage(), "schinese"))
	{
		std::wstringstream wss;

		auto ch = text;
		const char* pbase = (const char*)text;
		auto totalLen = wcslen(text);

		while (1)
		{
			auto pch = (const char*)ch;
			int offset = (pch - pbase);

			if (offset >= totalLen * 2)
				break;

			if ((*ch) == L'\0')
				break;

			if ((BYTE)pch[0] == (BYTE)0xFF)
			{
				wss << L"��";
				pch += 1;
				ch = (const wchar_t*)pch;
				continue;
			}

			if ((WORD)(*ch) > (WORD)0x00FF)
			{
				if ((WORD)(*ch) >= (WORD)0x2000 && (WORD)(*ch) <= (WORD)0x206F)
				{
					//General Punctuation
				}
				else if ((WORD)(*ch) >= (WORD)0x20A0 && (WORD)(*ch) <= (WORD)0x20CF)
				{
					//Currency Symbols
				}
				else if ((WORD)(*ch) >= (WORD)0x2100 && (WORD)(*ch) <= (WORD)0x214F)
				{
					//Letterlike Symbols
				}
				else if ((WORD)(*ch) >= (WORD)0x2200 && (WORD)(*ch) <= (WORD)0x22FF)
				{
					//Mathematical Operators
				}
				else if ((WORD)(*ch) >= (WORD)0x2300 && (WORD)(*ch) <= (WORD)0x23FF)
				{
					//Miscellaneous Symbols
				}
				else if ((WORD)(*ch) >= (WORD)0x2600 && (WORD)(*ch) <= (WORD)0x26FF)
				{
					//Miscellaneous Symbols
				}
				else if ((WORD)(*ch) >= (WORD)0x4E00 && (WORD)(*ch) <= (WORD)0x9FA5)
				{
					//schinese
				}
				else
				{
					//Skip invalid character
					pch += 1;
					ch = (const wchar_t*)pch;
					continue;
				}
			}

			if ((*ch) == L'\0')
				break;

			wss << (*ch);
			pch += sizeof(wchar_t);
			ch++;
		}

		auto ws = wss.str();

		gPrivateFuncs.ClientVGUI_RichText_SetTextW(pthis, dummy, ws.c_str());
		return;
	}

	gPrivateFuncs.ClientVGUI_RichText_SetTextW(pthis, dummy, text);
}

void __fastcall TeamMenu_LoadMapPage(void* pthis, int dummy, const char* mapname)
{
	bool previous = g_bIsLoadMapPage;

	g_bIsLoadMapPage = true;

	gPrivateFuncs.TeamMenu_LoadMapPage(pthis, dummy, mapname);

	g_bIsLoadMapPage = previous;
}

void __fastcall ClientVGUI_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.ClientVGUI_Panel_Init(pthis, 0, x, y, w, h);

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		auto pPanel = (vgui::IClientPanel*)pthis;
		pPanel->SetProportional(true);
	}
}

void CSBuyMenu_ActivateInternal(vgui::Panel* pthis)
{
	pthis->MoveToFront();
	pthis->RequestFocus();
	pthis->SetVisible(true);
	pthis->SetEnabled(true);
	vgui::surface()->SetMinimized(pthis->GetVPanel(), false);

	int screenW, screenH;
	vgui::surface()->GetScreenSize(screenW, screenH);

	pthis->SetPos(0, 0);
	pthis->SetSize(screenW, screenH);

	int wide2 = vgui::scheme()->GetAlteredProportionalScaledValue(640);
	int tall2 = vgui::scheme()->GetAlteredProportionalScaledValue(480);

	int offsetX = (screenW - wide2) / 2;
	int offsetY = (screenH - tall2) / 2;

	int offset = min(offsetX, offsetY);

	pthis->SetPos(offset, offset);
}

void __fastcall CSBuyMenu_Activate(vgui::Panel* pthis, int dummy)
{
	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		if (g_IsNativeClientUIHDProportional)
		{
			auto original = g_pVGuiSurface2->IsForcingHDProportional();

			g_pVGuiSurface2->SetForcingHDProportional(true);

			CSBuyMenu_ActivateInternal(pthis);

			g_pVGuiSurface2->SetForcingHDProportional(original);

		}
		else
		{
			auto original = g_pVGuiSurface2->IsForcingHDProportional();

			g_pVGuiSurface2->SetForcingHDProportional(false);

			CSBuyMenu_ActivateInternal(pthis);

			g_pVGuiSurface2->SetForcingHDProportional(original);
		}
	}
	else
	{
		gPrivateFuncs.CSBuyMenu_Activate(pthis, dummy);
	}
}

void __fastcall ClientVGUI_LoadControlSettings(vgui::Panel* pthis, int dummy, const char* controlResourceName, const char* pathID)
{
	//The Counter-Strike buy menu. vgui2::Frame::Activate() is only published on the
	//CS-family client module, so this block must not run on other clients.
	if (g_bIsCounterStrike && !strcmp(controlResourceName, "Resource/UI/BuyMenu.res"))
	{
		if (!gPrivateFuncs.CSBuyMenu_vftable)
		{
			gPrivateFuncs.CSBuyMenu_vftable = *(PVOID**)pthis;

			//Frame::Activate's slot; CSBuyMenu keeps its own override in place, so
			//the base slot index also addresses CSBuyMenu's vtable.
			int index = (int)GamedataResolveVFuncIndex(g_ClientDLLInfo.ImageBase, "vgui2::Frame::Activate()");

			g_pMetaHookAPI->VFTHookEx(gPrivateFuncs.CSBuyMenu_vftable, index, CSBuyMenu_Activate, (void**)&gPrivateFuncs.CSBuyMenu_Activate);

			Sig_FuncNotFound(CSBuyMenu_Activate);
		}
	}

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		if (!strcmp(controlResourceName, "Resource/UI/MOTD.res") ||
			!strcmp(controlResourceName, "Resource/UI/TeamMenu.res") ||
			!strcmp(controlResourceName, "Resource/UI/ClassMenu_CT.res") ||
			!strcmp(controlResourceName, "Resource/UI/ClassMenu_TER.res"))
		{
			vgui::scheme()->SetForcingAlteredProportional(true);
			gPrivateFuncs.ClientVGUI_LoadControlSettings(pthis, 0, controlResourceName, pathID);
			vgui::scheme()->SetForcingAlteredProportional(false);
			return;
		}
	}

	gPrivateFuncs.ClientVGUI_LoadControlSettings(pthis, 0, controlResourceName, pathID);
}

void ClientVGUI_KeyValues_FitToFullScreenInternal(KeyValues* pControlKeyValues)
{
	int xpos = pControlKeyValues->GetInt("xpos");
	int ypos = pControlKeyValues->GetInt("ypos");

	int screenW, screenH;
	vgui::surface()->GetScreenSize(screenW, screenH);

	int wide2 = vgui::scheme()->GetAlteredProportionalScaledValue(640);
	int tall2 = vgui::scheme()->GetAlteredProportionalScaledValue(480);

	int offsetX = (screenW - wide2) / 2;
	int offsetY = (screenH - tall2) / 2;

	xpos += vgui::scheme()->GetAlteredProportionalNormalizedValue(offsetX);
	ypos += vgui::scheme()->GetAlteredProportionalNormalizedValue(offsetY);

	pControlKeyValues->SetInt("xpos", xpos);
	pControlKeyValues->SetInt("ypos", ypos);
}

void ClientVGUI_KeyValues_FitToFullScreen(KeyValues* pControlKeyValues)
{
	if (g_IsNativeClientUIHDProportional)
	{
		auto original = g_pVGuiSurface2->IsForcingHDProportional();

		g_pVGuiSurface2->SetForcingHDProportional(true);

		ClientVGUI_KeyValues_FitToFullScreenInternal(pControlKeyValues);

		g_pVGuiSurface2->SetForcingHDProportional(original);
	}
	else
	{
		auto original = g_pVGuiSurface2->IsForcingHDProportional();

		g_pVGuiSurface2->SetForcingHDProportional(false);

		ClientVGUI_KeyValues_FitToFullScreenInternal(pControlKeyValues);

		g_pVGuiSurface2->SetForcingHDProportional(original);
	}
}

void ClientVGUI_KeyValues_LoadFromFile_CounterStrike(KeyValues* pthis, const char* resourceName, const char* pathId)
{
	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		if (!strcmp(resourceName, "Resource/UI/MOTD.res"))
		{
			auto pFrame = pthis->FindKey("ClientMOTD");
			if (pFrame)
			{
				ClientVGUI_KeyValues_FitToFullScreen(pFrame);
			}
		}
		else if (!strcmp(resourceName, "Resource/UI/TeamMenu.res"))
		{
			auto pFrame = pthis->FindKey("TeamMenu");
			if (pFrame)
			{
				ClientVGUI_KeyValues_FitToFullScreen(pFrame);
			}
		}
		else if (!strcmp(resourceName, "Resource/UI/ClassMenu_CT.res") || !strcmp(resourceName, "Resource/UI/ClassMenu_TER.res"))
		{
			auto pFrame = pthis->FindKey("ClassMenu");
			if (pFrame)
			{
				ClientVGUI_KeyValues_FitToFullScreen(pFrame);
			}
		}
		else if (!strcmp(resourceName, "Resource/UI/Spectator.res"))
		{
			auto pBottomBar = pthis->FindKey("BottomBar");
			if (pBottomBar)
			{
				int ypos = pBottomBar->GetInt("ypos");
				int tall = pBottomBar->GetInt("tall");

				char szTemp[32];
				snprintf(szTemp, sizeof(szTemp), "r%d", tall);
				pBottomBar->SetString("ypos", szTemp);
			}

			auto pbottombarblank = pthis->FindKey("bottombarblank");
			if (pbottombarblank)
			{
				int ypos = pbottombarblank->GetInt("ypos");
				int tall = pbottombarblank->GetInt("tall");

				char szTemp[32];
				snprintf(szTemp, sizeof(szTemp), "r%d", tall);
				pbottombarblank->SetString("ypos", szTemp);
			}
		}
	}
}

void ClientVGUI_KeyValues_LoadFromFile_CZDS(KeyValues* pthis, const char* resourceName, const char* pathId)
{
	if (!strcmp(resourceName, "resource/UI/WorldMap.res"))
	{
		auto pWorldMap = pthis->FindKey("WorldMap");

		if (pWorldMap)
		{
			ClientVGUI_KeyValues_FitToFullScreen(pWorldMap);
		}

		auto pMissionSelect = pthis->FindKey("MissionSelect");

		if (pMissionSelect)
		{
			ClientVGUI_KeyValues_FitToFullScreen(pMissionSelect);
		}
	}
}

void __fastcall CCSBackGroundPanel_Activate(vgui::Panel* pthis, int dummy)
{
	gPrivateFuncs.CCSBackGroundPanel_Activate(pthis, dummy);

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		*(int*)((PUCHAR)pthis + gPrivateFuncs.CCSBackGroundPanel_m_offsetX) = 0;
		*(int*)((PUCHAR)pthis + gPrivateFuncs.CCSBackGroundPanel_m_offsetY) = 0;
	}
}

void __fastcall CWorldMap_PaintBackground(vgui::Panel* pthis, int dummy)
{
	bool previous = g_bIsPaintWorldMapBackground;
	g_bIsPaintWorldMapBackground = true;

	gPrivateFuncs.CWorldMap_PaintBackground(pthis, dummy);

	g_bIsPaintWorldMapBackground = previous;
}

void __fastcall CWorldMapMissionSelect_PaintBackground(vgui::Panel* pthis, int dummy)
{
	bool previous = g_bIsPaintWorldMapBackground;
	g_bIsPaintWorldMapBackground = true;

	gPrivateFuncs.CWorldMapMissionSelect_PaintBackground(pthis, dummy);

	g_bIsPaintWorldMapBackground = previous;
}

bool __fastcall ClientVGUI_KeyValues_LoadFromFile(void* pthis, int dummy, IFileSystem* pFileSystem, const char* resourceName, const char* pathId)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, "ClientUI", &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = gPrivateFuncs.ClientVGUI_KeyValues_LoadFromFile(pthis, dummy, pFileSystem, resourceName, pathId);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, "ClientUI", &CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret; break;
	}
	default:
	{
		ret = real_ret; break;
	}
	}

	if (ret)
	{
		if (g_bIsCounterStrike)
			ClientVGUI_KeyValues_LoadFromFile_CounterStrike((KeyValues*)pthis, resourceName, pathId);

		if (g_bIsCZDS)
			ClientVGUI_KeyValues_LoadFromFile_CZDS((KeyValues*)pthis, resourceName, pathId);
	}

	return ret;
}

/*
============================================================
ClientVGUI interface proxy
============================================================
*/

class CClientVGUIProxy : public IClientVGUI
{
public:
	void Initialize(CreateInterfaceFn* factories, int count) override;
	void Start(void) override;
	void SetParent(vgui::VPANEL parent) override;
	bool UseVGUI1(void) override;
	void HideScoreBoard(void) override;
	void HideAllVGUIMenu(void) override;
	void ActivateClientUI(void) override;
	void HideClientUI(void) override;
	void Unknown(void) override;
	void Shutdown(void) override;
};

static CClientVGUIProxy s_ClientVGUIProxy;

void CClientVGUIProxy::Initialize(CreateInterfaceFn* factories, int count)
{
	m_pfnCClientVGUI_Initialize(this, 0, factories, count);

	if (!vgui::VGui_InitInterfacesList("VGUI2Extension", factories, count))
	{
		Sys_Error("Failed to VGui_InitInterfacesList");
		return;
	}

	if (gEngfuncs.CheckParm("clientui_use_hdp", NULL))
	{
		g_IsNativeClientUIHDProportional = true;
	}
	else if (gEngfuncs.CheckParm("clientui_no_hdp", NULL))
	{
		g_IsNativeClientUIHDProportional = false;
	}

	VGUI2ExtensionInternal()->ClientVGUI_Initialize(factories, count);
}

void ClientUIProxy_Start_FillAddress(CClientVGUIProxy* pthis, const mh_dll_info_t& RealDllInfo);

void CClientVGUIProxy::Start(void)
{
	m_pfnCClientVGUI_Start(this, 0);

	ClientUIProxy_Start_FillAddress(this, g_ClientDLLInfo);

	VGUI2ExtensionInternal()->ClientVGUI_Start();

	g_NativeClientHasVGUI1 = UseVGUI1();
}

void CClientVGUIProxy::SetParent(vgui::VPANEL parent)
{
	m_pfnCClientVGUI_SetParent(this, 0, parent);

	VGUI2ExtensionInternal()->ClientVGUI_SetParent(parent);
}

bool CClientVGUIProxy::UseVGUI1(void)
{
	bool fake_ret = false;
	bool real_ret = false;
	bool ret = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		real_ret = m_pfnCClientVGUI_UseVGUI1(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret; break;
	}
	default:
	{
		ret = real_ret; break;
	}
	}

	return ret;
}

void CClientVGUIProxy::HideScoreBoard(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_HideScoreBoard(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);
	}
}

void CClientVGUIProxy::HideAllVGUIMenu(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_HideAllVGUIMenu(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);
	}
}

void CClientVGUIProxy::ActivateClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_ActivateClientUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);
	}
}

void CClientVGUIProxy::HideClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		m_pfnCClientVGUI_HideClientUI(this, 0);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);
	}
}

void CClientVGUIProxy::Unknown(void)
{

}

void CClientVGUIProxy::Shutdown(void)
{

}

//Implement the ClientVGUI interface for those mod with no ClientVGUI implemented

class NewClientVGUI : public IClientVGUI
{
public:
	void Initialize(CreateInterfaceFn* factories, int count) override;
	void Start(void) override;
	void SetParent(vgui::VPANEL parent) override;
	bool UseVGUI1(void) override;
	void HideScoreBoard(void) override;
	void HideAllVGUIMenu(void) override;
	void ActivateClientUI(void) override;
	void HideClientUI(void) override;
	void Unknown(void) override;
	void Shutdown(void) override;
};

void NewClientVGUI::Initialize(CreateInterfaceFn* factories, int count)
{
	if (!vgui::VGui_InitInterfacesList("VGUI2Extension", factories, count))
	{
		Sys_Error("Failed to VGui_InitInterfacesList");
		return;
	}

	VGUI2ExtensionInternal()->ClientVGUI_Initialize(factories, count);
}

void NewClientVGUI::Start(void)
{
	VGUI2ExtensionInternal()->ClientVGUI_Start();

	g_NativeClientHasVGUI1 = UseVGUI1();
}

void NewClientVGUI::SetParent(vgui::VPANEL parent)
{
	VGUI2ExtensionInternal()->ClientVGUI_SetParent(parent);
}

bool NewClientVGUI::UseVGUI1(void)
{
	bool fake_ret = true;
	bool real_ret = true;
	bool ret = true;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;
	CallbackContext.pPluginReturnValue = &fake_ret;

	VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		//Must be true for Sven Co-op and other VGUI1 games
		real_ret = true;
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;
		CallbackContext.pRealReturnValue = &real_ret;

		VGUI2ExtensionInternal()->ClientVGUI_UseVGUI1(&CallbackContext);
	}

	switch (CallbackContext.Result)
	{
	case VGUI2Extension_Result::OVERRIDE:
	case VGUI2Extension_Result::SUPERCEDE:
	case VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS:
	{
		ret = fake_ret; break;
	}
	default:
	{
		ret = real_ret; break;
	}
	}

	return ret;
}

void NewClientVGUI::HideScoreBoard(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{

	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideScoreBoard(&CallbackContext);
	}
}

void NewClientVGUI::HideAllVGUIMenu(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{

	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideAllVGUIMenu(&CallbackContext);
	}
}

void NewClientVGUI::ActivateClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{

	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_ActivateClientUI(&CallbackContext);
	}
}

void NewClientVGUI::HideClientUI(void)
{
	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{

	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->ClientVGUI_HideClientUI(&CallbackContext);
	}
}

void NewClientVGUI::Unknown(void)
{

}

void NewClientVGUI::Shutdown(void)
{

}

EXPOSE_SINGLE_INTERFACE(NewClientVGUI, IClientVGUI, CLIENTVGUI_INTERFACE_VERSION);

void ClientUIProxy_Start_FillAddress(CClientVGUIProxy *pthis ,const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsCounterStrike && !g_bIsCZDS)
	{
		DWORD offset_CSBackGroundPanel = GamedataResolveStructMember(g_ClientDLLInfo.ImageBase, "CounterStrikeViewport.m_pCSBackGround");

		g_pCSBackGroundPanel = *(vgui::Panel**)((PUCHAR)pthis + offset_CSBackGroundPanel);

		//CCSBackGroundPanel keeps its own Activate override; the field its body
		//zeroes is published as a member offset, so no vtable walk is needed.
		int index = (int)GamedataResolveVFuncIndex(g_ClientDLLInfo.ImageBase, "CounterStrikeViewport::CCSBackGroundPanel::Activate()");

		gPrivateFuncs.CCSBackGroundPanel_m_offsetX = (int)GamedataResolveStructMember(g_ClientDLLInfo.ImageBase, "CounterStrikeViewport::CCSBackGroundPanel.m_offsetX");
		gPrivateFuncs.CCSBackGroundPanel_m_offsetY = (int)GamedataResolveStructMember(g_ClientDLLInfo.ImageBase, "CounterStrikeViewport::CCSBackGroundPanel.m_offsetY");

		g_pMetaHookAPI->VFTHook(g_pCSBackGroundPanel, 0, index, CCSBackGroundPanel_Activate, (void**)&gPrivateFuncs.CCSBackGroundPanel_Activate);

		Sig_FuncNotFound(CCSBackGroundPanel_Activate);
	}

	if (g_bIsCZDS)
	{
		DWORD offset_WorldMapPanel = GamedataResolveStructMember(g_ClientDLLInfo.ImageBase, "CZEROViewPort.m_pWorldMapPanel");

		g_pWorldMapPanel = *(vgui::Panel**)((PUCHAR)pthis + offset_WorldMapPanel);

		//The handler raises g_bIsPaintWorldMapBackground around the original call, so
		//the ISurface::GetScreenSize override applies only to this paint pass.
		int index = (int)GamedataResolveVFuncIndex(g_ClientDLLInfo.ImageBase, "CWorldMap::PaintBackground()");

		g_pMetaHookAPI->VFTHook(g_pWorldMapPanel, 0, index, CWorldMap_PaintBackground, (void**)&gPrivateFuncs.CWorldMap_PaintBackground);

		Sig_FuncNotFound(CWorldMap_PaintBackground);
	}

	if (g_bIsCZDS)
	{
		g_pWorldMapMissionSelectPanel = g_pWorldMapPanel->FindChildByName("MissionSelect");

		if (!g_pWorldMapMissionSelectPanel)
		{
			Sig_NotFound("WorldMapMissionSelectPanel");
		}

		int index = (int)GamedataResolveVFuncIndex(g_ClientDLLInfo.ImageBase, "CWorldMapMissionSelect::PaintBackground()");

		g_pMetaHookAPI->VFTHook(g_pWorldMapMissionSelectPanel, 0, index, CWorldMapMissionSelect_PaintBackground, (void**)&gPrivateFuncs.CWorldMapMissionSelect_PaintBackground);

		Sig_FuncNotFound(CWorldMapMissionSelect_PaintBackground);
	}
}

/*
	Purpose : Install hooks for native ClientUI interface
*/

void NativeClientUI_FillAddress(const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.ClientVGUI_Panel_Init = (decltype(gPrivateFuncs.ClientVGUI_Panel_Init))GamedataResolvePtrIfAvailable(
		RealDllInfo.ImageBase, "vgui2::Panel::Init(int, int, int, int)", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.ClientVGUI_KeyValues_LoadFromFile = (decltype(gPrivateFuncs.ClientVGUI_KeyValues_LoadFromFile))
		GamedataResolveKeyValuesLoadFromFileIfAvailable(RealDllInfo.ImageBase);

	if (g_bIsCounterStrike)
	{
		gPrivateFuncs.ClientVGUI_LoadControlSettings = (decltype(gPrivateFuncs.ClientVGUI_LoadControlSettings))GamedataResolvePtr(
			RealDllInfo.ImageBase, "vgui2::Frame::LoadControlSettings(char const*, char const*)", MH_GAMESYMBOL_KIND_FUNCTION);
	}

	if (g_bIsCounterStrike)
	{
		gPrivateFuncs.TeamMenu_LoadMapPage = (decltype(gPrivateFuncs.TeamMenu_LoadMapPage))GamedataResolvePtr(
			RealDllInfo.ImageBase, "CTeamMenu::LoadMapPage(char const*)", MH_GAMESYMBOL_KIND_FUNCTION);
		gPrivateFuncs.ClientVGUI_RichText_SetTextW = (decltype(gPrivateFuncs.ClientVGUI_RichText_SetTextW))GamedataResolvePtr(
			RealDllInfo.ImageBase, "vgui2::RichText::SetText(wchar_t const*)", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void NativeClientUI_InstallHooks(void)
{
	Install_InlineHook(ClientVGUI_LoadControlSettings);
	if (gPrivateFuncs.ClientVGUI_KeyValues_LoadFromFile)
	{
		Install_InlineHook(ClientVGUI_KeyValues_LoadFromFile);
	}
	if (gPrivateFuncs.ClientVGUI_Panel_Init)
	{
		Install_InlineHook(ClientVGUI_Panel_Init);
	}
	if (gPrivateFuncs.TeamMenu_LoadMapPage)
	{
		Install_InlineHook(TeamMenu_LoadMapPage);
	}
	if (gPrivateFuncs.ClientVGUI_RichText_SetTextW)
	{
		Install_InlineHook(ClientVGUI_RichText_SetTextW);
	}
}

void NativeClientUI_UninstallHooks(void)
{
	Uninstall_Hook(ClientVGUI_LoadControlSettings);
	Uninstall_Hook(ClientVGUI_KeyValues_LoadFromFile);
	Uninstall_Hook(ClientVGUI_Panel_Init);
	Uninstall_Hook(TeamMenu_LoadMapPage);
	Uninstall_Hook(ClientVGUI_RichText_SetTextW);
}

bool ClientVGUI_UseVGUI1()
{
	if (g_pClientVGUI)
		return g_pClientVGUI->UseVGUI1();

	return true;
}

void ClientVGUI_InstallHooks(cl_exportfuncs_t* pExportFunc)
{
	CreateInterfaceFn ClientVGUICreateInterface = NULL;

	if (g_hClientModule)
	{
		ClientVGUICreateInterface = (CreateInterfaceFn)Sys_GetFactory((HINTERFACEMODULE)g_hClientModule);
	}

	if (!ClientVGUICreateInterface && gExportfuncs.ClientFactory)
	{
		ClientVGUICreateInterface = (CreateInterfaceFn)gExportfuncs.ClientFactory();
	}

	if (ClientVGUICreateInterface)
	{
		g_pClientVGUI = (IClientVGUI*)ClientVGUICreateInterface(CLIENTVGUI_INTERFACE_VERSION, NULL);

		if (g_pClientVGUI)
		{
			PVOID* ProxyVFTable = *(PVOID**)&s_ClientVGUIProxy;

			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 1, (void*)ProxyVFTable[1], (void**)&m_pfnCClientVGUI_Initialize);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 2, (void*)ProxyVFTable[2], (void**)&m_pfnCClientVGUI_Start);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 3, (void*)ProxyVFTable[3], (void**)&m_pfnCClientVGUI_SetParent);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 4, (void*)ProxyVFTable[4], (void**)&m_pfnCClientVGUI_UseVGUI1);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 5, (void*)ProxyVFTable[5], (void**)&m_pfnCClientVGUI_HideScoreBoard);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 6, (void*)ProxyVFTable[6], (void**)&m_pfnCClientVGUI_HideAllVGUIMenu);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 7, (void*)ProxyVFTable[7], (void**)&m_pfnCClientVGUI_ActivateClientUI);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 8, (void*)ProxyVFTable[8], (void**)&m_pfnCClientVGUI_HideClientUI);

			NativeClientUI_FillAddress(g_ClientDLLInfo);
			NativeClientUI_InstallHooks();

			g_IsNativeClientVGUI2 = true;
		}
	}

	if (!g_IsNativeClientVGUI2)
	{
		pExportFunc->ClientFactory = NewClientFactory;
	}
}

void ClientVGUI_UninstallHooks()
{
	//TODO: uninstall VFTHooks
}

PVOID VGUIClient001_CreateInterface(HINTERFACEMODULE hModule)
{
	if (hModule == (HINTERFACEMODULE)g_hClientModule && !g_IsNativeClientVGUI2)
	{
		return NewCreateInterface;
	}

	return Sys_GetFactory(hModule);
}

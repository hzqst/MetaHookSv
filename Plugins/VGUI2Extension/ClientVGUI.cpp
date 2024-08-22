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

#include <capstone.h>
#include <sstream>

namespace vgui
{
bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories);
}

extern vgui::ISurface* g_pSurface;
extern vgui::ISurface_HL25* g_pSurface_HL25;

bool g_IsNativeClientVGUI2 = false;
bool g_IsNativeClientUIHDProportional = false;

IClientVGUI* g_pClientVGUI = NULL;
CounterStrikeViewport* g_pCounterStrikeViewport = NULL;

static vgui::Panel* g_pCSBackGroundPanel = NULL;

static hook_t* g_phook_ClientVGUI_Panel_Init = NULL;
static hook_t* g_phook_ClientVGUI_KeyValues_LoadFromFile = NULL;
static hook_t* g_phook_ClientVGUI_LoadControlSettings = NULL;

static void (__fastcall *m_pfnCClientVGUI_Initialize)(void *pthis, int,CreateInterfaceFn *factories, int count) = NULL;
static void (__fastcall *m_pfnCClientVGUI_Start)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_SetParent)(void *pthis, int, vgui::VPANEL parent) = NULL;
static bool (__fastcall *m_pfnCClientVGUI_UseVGUI1)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideScoreBoard)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideAllVGUIMenu)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_ActivateClientUI)(void *pthis, int) = NULL;
static void (__fastcall *m_pfnCClientVGUI_HideClientUI)(void *pthis, int) = NULL;

/*
============================================================
ClientVGUI inline hook
============================================================
*/

vgui::BuildGroup_Legacy *GetLegacyBuildGroup(vgui::Panel* pWindow)
{
	auto vftable = *(PVOID**)(pWindow);

	auto pfnGetBuildGroup = (vgui::BuildGroup_Legacy *(__fastcall *)(vgui::Panel * pthis, int))vftable[139];
	
	return pfnGetBuildGroup(pWindow, 0);
}

//Fuck Valve
#if 1
void __fastcall ClientVGUI_RichText_SetTextW_Proxy(void* pthis, int dummy, const wchar_t* text)
{
	if (!strcmp(GetCurrentGameLanguage(), "schinese"))
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
				wss << L"£º";
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

		return gPrivateFuncs.ClientVGUI_RichText_SetTextW(pthis, dummy, ws.c_str());
	}

	gPrivateFuncs.ClientVGUI_RichText_SetTextW(pthis, dummy, text);
}
#endif

void __fastcall ClientVGUI_Panel_Init(vgui::Panel* pthis, int dummy, int x, int y, int w, int h)
{
	gPrivateFuncs.ClientVGUI_Panel_Init(pthis, 0, x, y, w, h);

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		PVOID* PanelVFTable = *(PVOID**)pthis;
		void(__fastcall * pfnSetProportional)(vgui::Panel * pthis, int dummy, bool state) = (decltype(pfnSetProportional))PanelVFTable[113];
		pfnSetProportional(pthis, 0, true);
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

typedef struct
{
	int OffsetCandidates[2];
	int OffsetCandidatesCount;
	bool bFoundCall22Ch;
}VGUI2_IsCSBackGroundPanelActivate_SearchContext;

bool VGUI2_IsCSBackGroundPanelActivate(PVOID Candidate, int *pOffsetBase)
{
	VGUI2_IsCSBackGroundPanelActivate_SearchContext ctx = { 0 };

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsCSBackGroundPanelActivate_SearchContext*)context;

		if (!ctx->bFoundCall22Ch &&
			pinst->id == X86_INS_CALL &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base != 0 &&
			pinst->detail->x86.operands[0].mem.disp == 0x22C)
		{
			ctx->bFoundCall22Ch = true;
		}

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base != 0 &&
			pinst->detail->x86.operands[0].mem.disp >= 0x130 &&
			pinst->detail->x86.operands[0].mem.disp <= 0x140 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG)
		{
			if (ctx->OffsetCandidatesCount < 2)
			{
				ctx->OffsetCandidates[ctx->OffsetCandidatesCount] = pinst->detail->x86.operands[0].mem.disp;
				ctx->OffsetCandidatesCount++;
			}
		}

		if (ctx->bFoundCall22Ch)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	if (ctx.bFoundCall22Ch && ctx.OffsetCandidatesCount >= 2)
	{
		(*pOffsetBase) = min(ctx.OffsetCandidates[0], ctx.OffsetCandidates[1]);
		return true;
	}

	return false;
}

typedef struct
{
	PVOID CallCandidates[4];
	int CallCount;
	bool bFound280h;
	bool bFound1E0h;
}VGUI2_IsFitToScreen_SearchContext;

bool VGUI2_IsFitToScreenInternal(PVOID Candidate, VGUI2_IsFitToScreen_SearchContext *ctx)
{

	g_pMetaHookAPI->DisasmRanges(ctx->CallCandidates[1], 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsFitToScreen_SearchContext*)context;

		if (!ctx->bFound280h &&
			pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM &&
			pinst->detail->x86.operands[0].imm == 0x280)
		{
			ctx->bFound280h = true;
		}

		if (!ctx->bFound1E0h &&
			pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM &&
			pinst->detail->x86.operands[0].imm == 0x1E0)
		{
			ctx->bFound1E0h = true;
		}

		if (ctx->bFound1E0h && ctx->bFound280h)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, ctx);

	return ctx->bFound1E0h && ctx->bFound280h;
}

bool VGUI2_IsFitToScreen(PVOID Candidate)
{
	VGUI2_IsFitToScreen_SearchContext ctx = {0};

	if (VGUI2_IsFitToScreenInternal(Candidate, &ctx))
		return true;

	memset(&ctx, 0, sizeof(ctx));

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsFitToScreen_SearchContext*)context;

		if (address[0] == 0xE8 && ctx->CallCount < 4)
		{
			ctx->CallCandidates[ctx->CallCount] = (PVOID)GetCallAddress(address);
			ctx->CallCount++;
		}

		if (ctx->bFound1E0h && ctx->bFound280h)
			return TRUE;

		if (ctx->CallCount >= 4)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);


	if (ctx.CallCount == 2)
	{
		VGUI2_IsFitToScreenInternal(ctx.CallCandidates[1], &ctx);

		if (ctx.bFound1E0h && ctx.bFound280h)
		{
			return true;
		}
	}

	return false;
}

void __fastcall ClientVGUI_LoadControlSettings(vgui::Panel* pthis, int dummy, const char* controlResourceName, const char* pathID)
{
	if (!strcmp(controlResourceName, "Resource/UI/BuyMenu.res"))
	{
		if (!gPrivateFuncs.CSBuyMenu_vftable)
		{
			gPrivateFuncs.CSBuyMenu_vftable = *(PVOID**)pthis;

			if (gPrivateFuncs.ClientVGUI_Frame_Activate_vftable_index)
			{
				int index = gPrivateFuncs.ClientVGUI_Frame_Activate_vftable_index;
				g_pMetaHookAPI->VFTHookEx(gPrivateFuncs.CSBuyMenu_vftable, index, CSBuyMenu_Activate, (void**)&gPrivateFuncs.CSBuyMenu_Activate);
			}
			else
			{
				for (int index = 159; index <= 160; ++index)
				{
					if (VGUI2_IsFitToScreen(gPrivateFuncs.CSBuyMenu_vftable[index]))
					{
						gPrivateFuncs.ClientVGUI_Frame_Activate_vftable_index = index;
						g_pMetaHookAPI->VFTHookEx(gPrivateFuncs.CSBuyMenu_vftable, index, CSBuyMenu_Activate, (void**)&gPrivateFuncs.CSBuyMenu_Activate);
						break;
					}
				}
			}
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

void __fastcall CCSBackGroundPanel_Activate(vgui::Panel* pthis, int dummy)
{
	gPrivateFuncs.CCSBackGroundPanel_Activate(pthis, dummy);

	if (DpiManagerInternal()->IsHighDpiSupportEnabled())
	{
		*(int*)((PUCHAR)pthis + gPrivateFuncs.CCSBackGroundPanel_XOffsetBase) = 0;
		*(int*)((PUCHAR)pthis + gPrivateFuncs.CCSBackGroundPanel_XOffsetBase + 4) = 0;
	}
}

#if 0
void ResizeWindowControls(vgui::Panel* pWindow, int offsetX, int offsetY)
{
	if (!pWindow || !GetLegacyBuildGroup(pWindow) || !GetLegacyBuildGroup(pWindow)->GetPanelList())
		return;

	CUtlVector<vgui::PHandle>* panelList = GetLegacyBuildGroup(pWindow)->GetPanelList();
	CUtlVector<vgui::Panel*> resizedPanels;
	CUtlVector<vgui::Panel*> movedPanels;

	// Resize to account for 1.25 aspect ratio (1280x1024) screens
	{
		for (int i = 0; i < panelList->Size(); ++i)
		{
			vgui::PHandle handle = (*panelList)[i];

			vgui::Panel* panel = handle.GetWithControlModuleName("ClientUI");

			bool found = false;
			for (int j = 0; j < resizedPanels.Size(); ++j)
			{
				if (panel == resizedPanels[j])
					found = true;
			}

			if (!panel || found)
			{
				continue;
			}

			resizedPanels.AddToTail(panel); // don't move a panel more than once

			if (panel != pWindow)
			{
			
			}
		}
	}

	// and now re-center them.  Woohoo!
	for (int i = 0; i < panelList->Size(); ++i)
	{
		vgui::PHandle handle = (*panelList)[i];

		vgui::Panel* panel = handle.GetWithControlModuleName("ClientUI");

		bool found = false;
		for (int j = 0; j < movedPanels.Size(); ++j)
		{
			if (panel == movedPanels[j])
				found = true;
		}

		if (!panel || found)
		{
			continue;
		}

		movedPanels.AddToTail(panel); // don't move a panel more than once

		if (panel != pWindow)
		{
			int x, y;

			panel->GetPos(x, y);
			panel->SetPos(x + offsetX, y + offsetY);
		}
	}
}

void __fastcall CBuySubMenu_OnDisplay(vgui::Panel* pthis, int dummy)
{
	gPrivateFuncs.CBuySubMenu_OnDisplay(pthis, dummy);
}

#endif

#if 0
void *__fastcall CCSBackGroundPanel_ctor(vgui::Panel* pthis, int dummy, vgui::Panel* parent)
{
	auto r = gPrivateFuncs.CCSBackGroundPanel_ctor(pthis, dummy, parent);

	if (!gPrivateFuncs.CCSBackGroundPanel_vftable)
	{
		gPrivateFuncs.CCSBackGroundPanel_vftable = *(decltype(gPrivateFuncs.CCSBackGroundPanel_vftable)*)pthis;
		g_pMetaHookAPI->VFTHook(pthis, 0, 160, CCSBackGroundPanel_Activate, (void**)&gPrivateFuncs.CCSBackGroundPanel_Activate);
	}

	return r;
}
#endif

#if 0

void __fastcall ClientVGUI_BuildGroup_LoadControlSettings(vgui::BuildGroup_Legacy* pthis, int dummy, const char* controlResourceName, const char* pathID)
{
	if (!strcmp(controlResourceName, "Resource/UI/MOTD.res") ||
		!strcmp(controlResourceName, "Resource/UI/TeamMenu.res") ||
		!strcmp(controlResourceName, "Resource/UI/ClassMenu_CT.res") ||
		!strcmp(controlResourceName, "Resource/UI/ClassMenu_TER.res"))
	{
		vgui::scheme()->SetForcingAlteredProportional(true);
		gPrivateFuncs.ClientVGUI_BuildGroup_LoadControlSettings(pthis, dummy, controlResourceName, pathID);
		vgui::scheme()->SetForcingAlteredProportional(false);
		return;
	}

	gPrivateFuncs.ClientVGUI_BuildGroup_LoadControlSettings(pthis, dummy, controlResourceName, pathID);
}

void __fastcall ClientVGUI_BuildGroup_ApplySettings(vgui::BuildGroup_Legacy* pthis, int dummy, KeyValues* resourceData)
{
	gPrivateFuncs.ClientVGUI_BuildGroup_ApplySettings(pthis, dummy, resourceData);
}
#endif

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
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
	}
	}

	if (ret && g_bIsCounterStrike)
	{
		ClientVGUI_KeyValues_LoadFromFile_CounterStrike((KeyValues *)pthis, resourceName, pathId);
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
	void Initialize(CreateInterfaceFn *factories, int count) override;
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

void CClientVGUIProxy::Initialize(CreateInterfaceFn *factories, int count)
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

void CClientVGUIProxy::Start(void)
{
	m_pfnCClientVGUI_Start(this, 0);

	if (g_bIsCounterStrike)
	{
		const int offset_CSBackGroundPanel = 0x72C;

		g_pCSBackGroundPanel = *(vgui::Panel**)((PUCHAR)this + offset_CSBackGroundPanel);

		gPrivateFuncs.CCSBackGroundPanel_vftable = *(PVOID**)g_pCSBackGroundPanel;

		for (int index = 159; index <= 160; ++index)
		{
			if (VGUI2_IsCSBackGroundPanelActivate(gPrivateFuncs.CCSBackGroundPanel_vftable[index], &gPrivateFuncs.CCSBackGroundPanel_XOffsetBase))
			{
				gPrivateFuncs.ClientVGUI_Frame_Activate_vftable_index = index;
				g_pMetaHookAPI->VFTHook(g_pCSBackGroundPanel, 0, index, CCSBackGroundPanel_Activate, (void**)&gPrivateFuncs.CCSBackGroundPanel_Activate);
				break;
			}
		}
		Sig_FuncNotFound(CCSBackGroundPanel_Activate);
	}

	VGUI2ExtensionInternal()->ClientVGUI_Start();
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
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
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
	void Initialize(CreateInterfaceFn *factories, int count) override;
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

void NewClientVGUI::Initialize(CreateInterfaceFn *factories, int count)
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
		ret = fake_ret;
	}
	default:
	{
		ret = real_ret;
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

/*
	Purpose : Install hooks for native ClientUI interface
*/

typedef struct
{
	bool bIsUnicode;
}NativeClientUI_RichText_SearchContext;

void NativeClientUI_RichText_Search(PVOID Candidate, bool bIsUnicode)
{
	NativeClientUI_RichText_SearchContext ctx = { 0 };

	ctx.bIsUnicode = bIsUnicode;

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (NativeClientUI_RichText_SearchContext*)context;

		if ((pinst->id == X86_INS_JE) &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM)
		{
			PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;

			NativeClientUI_RichText_Search(imm, true);
			return FALSE;
		}

		if (address[0] == 0xE8)
		{
			if (ctx->bIsUnicode)
			{
				if (!gPrivateFuncs.ClientVGUI_RichText_SetTextW)
				{
					gPrivateFuncs.ClientVGUI_RichText_SetTextW = (decltype(gPrivateFuncs.ClientVGUI_RichText_SetTextW))GetCallAddress(address);
					g_pMetaHookAPI->InlinePatchRedirectBranch(address, ClientVGUI_RichText_SetTextW_Proxy, NULL);
				}
			}
			else
			{
				if (!gPrivateFuncs.ClientVGUI_RichText_SetTextA)
				{
					gPrivateFuncs.ClientVGUI_RichText_SetTextA = (decltype(gPrivateFuncs.ClientVGUI_RichText_SetTextA))GetCallAddress(address);
				}
			}
			return TRUE;
		}

		if(instCount > 10)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);
}

void NativeClientUI_FillAddress(void)
{
	ULONG ClientTextSize = g_dwClientTextSize;
	PVOID ClientTextBase = g_dwClientTextBase;

	ULONG ClientDataSize = 0;
	auto ClientDataBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".data\0\0\0", &ClientDataSize);

	ULONG ClientRdataSize = 0;
	auto ClientRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwClientBase, ".rdata\0\0", &ClientRdataSize);

	if (1)
	{
		gPrivateFuncs.ClientVGUI_Panel_Init = (decltype(gPrivateFuncs.ClientVGUI_Panel_Init))VGUI2_FindPanelInit(ClientTextBase, ClientTextSize);
		Sig_FuncNotFound(ClientVGUI_Panel_Init);

		gPrivateFuncs.ClientVGUI_KeyValues_vftable = (decltype(gPrivateFuncs.ClientVGUI_KeyValues_vftable))gPrivateFuncs.ClientVGUI_KeyValues_vftable = VGUI2_FindKeyValueVFTable(
			ClientTextBase, ClientTextSize,
			ClientRdataBase, ClientRdataSize,
			ClientDataBase, ClientDataSize);
		Sig_FuncNotFound(ClientVGUI_KeyValues_vftable);

		gPrivateFuncs.ClientVGUI_KeyValues_LoadFromFile = (decltype(gPrivateFuncs.ClientVGUI_KeyValues_LoadFromFile))gPrivateFuncs.ClientVGUI_KeyValues_vftable[2];
	}

	if (1)
	{
		const char sigs1[] = "Resource/UI/TeamMenu.res";
		auto TeamMenu_res_String = Search_Pattern_From_Size(ClientRdataBase, ClientRdataSize, sigs1);
		if (!TeamMenu_res_String)
			TeamMenu_res_String = Search_Pattern_From_Size(ClientDataBase, ClientDataSize, sigs1);
		Sig_VarNotFound(TeamMenu_res_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)TeamMenu_res_String;
		auto TeamMenu_res_PushString = Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
		Sig_VarNotFound(TeamMenu_res_PushString);

		g_pMetaHookAPI->DisasmRanges(TeamMenu_res_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;

			if (address[0] == 0xE8 && instCount <= 8)
			{
				gPrivateFuncs.ClientVGUI_LoadControlSettings = (decltype(gPrivateFuncs.ClientVGUI_LoadControlSettings))GetCallAddress(address);

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, NULL);

		Sig_FuncNotFound(ClientVGUI_LoadControlSettings);
	}

	if (g_bIsCounterStrike)
	{
		const char sigs1[] = "maps/%s.txt";
		auto MAPS_String = Search_Pattern_From_Size(ClientRdataBase, ClientRdataSize, sigs1);
		if (!MAPS_String)
			MAPS_String = Search_Pattern_From_Size(ClientDataBase, ClientDataSize, sigs1);
		Sig_VarNotFound(MAPS_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x8D";
		*(DWORD*)(pattern + 1) = (DWORD)MAPS_String;
		auto MAPS_PushString = Search_Pattern_From_Size(ClientTextBase, ClientTextSize, pattern);
		Sig_VarNotFound(MAPS_PushString);

		typedef struct
		{
			PVOID InstAddress_FEFF;
		}CTeamMenu_LoadMapPage_SearchContext;

		CTeamMenu_LoadMapPage_SearchContext ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(MAPS_PushString, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (CTeamMenu_LoadMapPage_SearchContext*)context;

			if(pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0xFEFF)
			{
				ctx->InstAddress_FEFF = (decltype(ctx->InstAddress_FEFF))address;

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		if (!ctx.InstAddress_FEFF)
		{
			Sig_NotFound(ctx.InstAddress_FEFF);
		}

		NativeClientUI_RichText_Search(ctx.InstAddress_FEFF, false);

		Sig_FuncNotFound(ClientVGUI_RichText_SetTextW);
		Sig_FuncNotFound(ClientVGUI_RichText_SetTextA);
	}
}

void NativeClientUI_InstallHooks(void)
{
	Install_InlineHook(ClientVGUI_LoadControlSettings);
	Install_InlineHook(ClientVGUI_KeyValues_LoadFromFile);
	Install_InlineHook(ClientVGUI_Panel_Init);
}

void NativeClientUI_UninstallHooks(void)
{
	Uninstall_Hook(ClientVGUI_LoadControlSettings);
	Uninstall_Hook(ClientVGUI_KeyValues_LoadFromFile);
	Uninstall_Hook(ClientVGUI_Panel_Init);
}

bool ClientVGUI_UseVGUI1()
{
	if(g_pClientVGUI)
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
		g_pClientVGUI = (IClientVGUI *)ClientVGUICreateInterface(CLIENTVGUI_INTERFACE_VERSION, NULL);

		if (g_pClientVGUI)
		{
			if (g_bIsCounterStrike)
			{
				g_pCounterStrikeViewport = (CounterStrikeViewport*)(g_pClientVGUI - 1);
			}

			PVOID* ProxyVFTable = *(PVOID**)&s_ClientVGUIProxy;

			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 1, (void*)ProxyVFTable[1], (void**)&m_pfnCClientVGUI_Initialize);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 2, (void*)ProxyVFTable[2], (void**)&m_pfnCClientVGUI_Start);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 3, (void*)ProxyVFTable[3], (void**)&m_pfnCClientVGUI_SetParent);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 4, (void*)ProxyVFTable[4], (void**)&m_pfnCClientVGUI_UseVGUI1);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 5, (void*)ProxyVFTable[5], (void**)&m_pfnCClientVGUI_HideScoreBoard);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 6, (void*)ProxyVFTable[6], (void**)&m_pfnCClientVGUI_HideAllVGUIMenu);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 7, (void*)ProxyVFTable[7], (void**)&m_pfnCClientVGUI_ActivateClientUI);
			g_pMetaHookAPI->VFTHook(g_pClientVGUI, 0, 8, (void*)ProxyVFTable[8], (void**)&m_pfnCClientVGUI_HideClientUI);

			NativeClientUI_FillAddress();
			NativeClientUI_InstallHooks();

			g_IsNativeClientVGUI2 = true;
		}
	}

	if (!g_IsNativeClientVGUI2)
	{
		pExportFunc->ClientFactory = NewClientFactory;
	}
}

PVOID VGUIClient001_CreateInterface(HINTERFACEMODULE hModule)
{
	if (hModule == (HINTERFACEMODULE)g_hClientModule && !g_IsNativeClientVGUI2)
	{
		return NewCreateInterface;
	}

	return Sys_GetFactory(hModule);
}

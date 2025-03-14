#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <IGameConsole.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>
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

bool VGUI2_IsMenuMakeItemsVisibleInScrollRange(PVOID Candidate, int* poffset_ScrollBar)
{
	typedef struct VGUI2_IsMenuMakeItemsVisibleInScrollRange_SearchContext_s
	{
		int offset_ScrollBar{};
		bool bFoundCall21Ch{};
	}VGUI2_IsMenuMakeItemsVisibleInScrollRange_SearchContext;

	VGUI2_IsMenuMakeItemsVisibleInScrollRange_SearchContext ctx = {  };

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsMenuMakeItemsVisibleInScrollRange_SearchContext*)context;

		if (!ctx->offset_ScrollBar &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base &&
			pinst->detail->x86.operands[1].mem.base != X86_REG_ESP &&
			pinst->detail->x86.operands[1].mem.base != X86_REG_EBP &&
			pinst->detail->x86.operands[1].mem.disp >= 0x80 &&
			pinst->detail->x86.operands[1].mem.disp <= 0x90)
		{
			ctx->offset_ScrollBar = pinst->detail->x86.operands[1].mem.disp;
		}

		//call  [exx+21Ch]
		if (!ctx->bFoundCall21Ch &&
			pinst->id == X86_INS_CALL &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base &&
			pinst->detail->x86.operands[0].mem.base != X86_REG_ESP &&
			pinst->detail->x86.operands[0].mem.base != X86_REG_EBP &&
			pinst->detail->x86.operands[0].mem.disp == 0x21C)
		{
			ctx->bFoundCall21Ch = true;
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	if (ctx.bFoundCall21Ch)
	{
		if (poffset_ScrollBar)
			(*poffset_ScrollBar) = ctx.offset_ScrollBar;

		return TRUE;
	}

	return FALSE;
}

bool VGUI2_IsPanelSetSize(PVOID Candidate)
{
	typedef struct VGUI2_IsPanelSetSize_SearchContext_s
	{
		bool bFoundCall10h{};
		bool bAdd10h{};
		bool bMov10h{};
		int instCount_Add10h{};
		int instCount_Mov10h{};
		int reg_Add10h{};
		int reg_Mov10h{};
	}VGUI2_IsPanelSetSize_SearchContext;

	VGUI2_IsPanelSetSize_SearchContext ctx = { };

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsPanelSetSize_SearchContext*)context;

		//call  [exx+10h]
		if (!ctx->bFoundCall10h &&
			pinst->id == X86_INS_CALL &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base &&
			pinst->detail->x86.operands[0].mem.base != X86_REG_ESP &&
			pinst->detail->x86.operands[0].mem.base != X86_REG_EBP &&
			pinst->detail->x86.operands[0].mem.disp == 0x10)
		{
			ctx->bFoundCall10h = true;
			return TRUE;
		}

		//mov     exx, [exx+10h]
		if (!ctx->bMov10h &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base &&
			pinst->detail->x86.operands[1].mem.base != X86_REG_ESP &&
			pinst->detail->x86.operands[1].mem.base != X86_REG_EBP &&
			pinst->detail->x86.operands[1].mem.disp == 0x10)
		{
			ctx->bMov10h = true;
			ctx->instCount_Mov10h = instCount;
			ctx->reg_Mov10h = pinst->detail->x86.operands[0].reg;
		}

		//add     exx, 10
		if (!ctx->bAdd10h &&
			pinst->id == X86_INS_ADD &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 0x10)
		{
			ctx->bAdd10h = true;
			ctx->instCount_Add10h = instCount;
			ctx->reg_Add10h = pinst->detail->x86.operands[0].reg;
		}

		//mov     exx, [exx]
		if (ctx->bAdd10h &&
			!ctx->bMov10h &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == ctx->reg_Add10h)
		{
			ctx->bMov10h = true;
			ctx->instCount_Mov10h = instCount;
			ctx->reg_Mov10h = pinst->detail->x86.operands[0].reg;
		}

		//call     exx
		if (ctx->bMov10h &&
			instCount > ctx->instCount_Mov10h &&
			instCount < ctx->instCount_Mov10h + 10 &&
			pinst->id == X86_INS_CALL &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == ctx->reg_Mov10h)
		{
			ctx->bFoundCall10h = true;
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	return ctx.bFoundCall10h;
}

bool VGUI2_IsPanelSetMinimumSize(PVOID Candidate)
{
	typedef struct VGUI2_IsPanelSetMinimumSize_SearchContext_s
	{
		bool bFoundCall18h{};
		bool bAdd18h{};
		bool bMov18h{};
		int instCount_Add18h{};
		int instCount_Mov18h{};
		int reg_Add18h{};
		int reg_Mov18h{};
	}VGUI2_IsPanelSetMinimumSize_SearchContext;

	VGUI2_IsPanelSetMinimumSize_SearchContext ctx = { };

	g_pMetaHookAPI->DisasmRanges(Candidate, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (VGUI2_IsPanelSetMinimumSize_SearchContext*)context;

		if (!ctx->bFoundCall18h &&
			pinst->id == X86_INS_CALL &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base &&
			pinst->detail->x86.operands[0].mem.base != X86_REG_ESP &&
			pinst->detail->x86.operands[0].mem.base != X86_REG_EBP &&
			pinst->detail->x86.operands[0].mem.disp == 0x18)
		{
			ctx->bFoundCall18h = true;
			return TRUE;
		}

		//mov     exx, [exx+18h]
		if (!ctx->bMov18h &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base &&
			pinst->detail->x86.operands[1].mem.base != X86_REG_ESP &&
			pinst->detail->x86.operands[1].mem.base != X86_REG_EBP &&
			pinst->detail->x86.operands[1].mem.disp == 0x18)
		{
			ctx->bMov18h = true;
			ctx->instCount_Mov18h = instCount;
			ctx->reg_Mov18h = pinst->detail->x86.operands[0].reg;
		}

		if (!ctx->bAdd18h &&
			pinst->id == X86_INS_ADD &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 0x18)
		{
			ctx->bAdd18h = true;
			ctx->instCount_Add18h = instCount;
			ctx->reg_Add18h = pinst->detail->x86.operands[0].reg;
		}

		if (ctx->bAdd18h &&
			!ctx->bMov18h &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == ctx->reg_Add18h)
		{
			ctx->bMov18h = true;
			ctx->instCount_Mov18h = instCount;
			ctx->reg_Mov18h = pinst->detail->x86.operands[0].reg;
		}

		if (ctx->bMov18h &&
			instCount > ctx->instCount_Mov18h &&
			instCount < ctx->instCount_Mov18h + 12 &&
			pinst->id == X86_INS_CALL &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == ctx->reg_Mov18h)
		{
			ctx->bFoundCall18h = true;
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	return ctx.bFoundCall18h;
}

/*
====================================================================
ServerBrowser inline hook
====================================================================
*/

void __fastcall ServerBrowser_Panel_SetSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	if (pthis->IsProportional())
	{
		width = g_pVGuiSchemeManager2->GetProportionalScaledValue(width);
		height = g_pVGuiSchemeManager2->GetProportionalScaledValue(height);
	}

	gPrivateFuncs.ServerBrowser_Panel_SetSize(pthis, 0, width, height);
}

void __fastcall ServerBrowser_Panel_SetMinimumSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	if (pthis->IsProportional())
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
		PVOID* PanelVFTable = *(PVOID**)pthis;
		void(__fastcall * pfnSetProportional)(vgui::Panel * pthis, int dummy, bool state) = (decltype(pfnSetProportional))PanelVFTable[113];
		pfnSetProportional(pthis, 0, true);
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
	}
	default:
	{
		ret = real_ret;
	}
	}

	return ret;
}

/*
==================================================================================
GameUI vgui_controls hook
==================================================================================
*/

void __fastcall GameUI_RichText_InsertChar(void* pthis, int dummy, wchar_t ch)
{
	if (ch == L'\r')
		return;

	gPrivateFuncs.GameUI_RichText_InsertChar(pthis, 0, ch);
}

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
		PVOID* PanelVFTable = *(PVOID**)pthis;
		void(__fastcall * pfnSetProportional)(vgui::Panel * pthis, int dummy, bool state) = (decltype(pfnSetProportional))PanelVFTable[113];
		pfnSetProportional(pthis, 0, true);
	}
}

void __fastcall GameUI_MessageBox_ApplySchemeSettings_Panel_SetSize(vgui::Panel* pthis, int dummy, int width, int height)
{
	if (pthis->IsProportional())
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

void __fastcall COptionsSubVideo_ApplyVidSettings_HL25(vgui::Panel* pthis, int dummy)
{
	void* _this = pthis;
	bool bForceRestart = false;

	VGUI2Extension_CallbackContext CallbackContext;

	CallbackContext.Result = VGUI2Extension_Result::UNSET;
	CallbackContext.IsPost = false;

	VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);

	if (CallbackContext.Result < VGUI2Extension_Result::SUPERCEDE)
	{
		gPrivateFuncs.COptionsSubVideo_ApplyVidSettings_HL25(pthis, dummy);
	}

	if (CallbackContext.Result != VGUI2Extension_Result::SUPERCEDE_SKIP_PLUGINS)
	{
		CallbackContext.Result = VGUI2Extension_Result::UNSET;
		CallbackContext.IsPost = true;

		VGUI2ExtensionInternal()->GameUI_COptionsSubVideo_ApplyVidSettings(_this, bForceRestart, &CallbackContext);
	}
}

void* __fastcall GameUI_FocusNavGroup_GetCurrentFocus(void* pthis, int dummy)
{
	vgui::VPanelHandle* _currentFocus = (vgui::VPanelHandle*)((PUCHAR)pthis + 12);

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
	int offset_activePage = 144;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		offset_activePage = 148;
	}

	gPrivateFuncs.GameUI_PropertySheet_PerformLayout(pthis, dummy);

	PVOID* _propertySheet_vftable = *(PVOID**)pthis;

	void(__fastcall * pfnChangeActiveTab)(vgui::Panel * pthis, int dummy, int index) =
		(decltype(pfnChangeActiveTab))_propertySheet_vftable[149];

	auto pPropertySheet = (CPropertySheet_Legacy*)((PUCHAR)pthis + offset_activePage - offsetof(CPropertySheet_Legacy, _activePage));
#if 0
	if (!pPropertySheet->_activePage)
	{
		// first page becomes the active page
		pfnChangeActiveTab(pthis, 0, 0);

		if (pPropertySheet->_activePage)
			pPropertySheet->_activePage->RequestFocus(0);
	}

	int x, y, wide, tall;
	pthis->GetBounds(x, y, wide, tall);
	if (pPropertySheet->_activePage)
	{
		if (pPropertySheet->_showTabs)
		{
			pPropertySheet->_activePage->SetBounds(
				0,
				vgui::scheme()->GetProportionalScaledValue(28),
				wide,
				tall - vgui::scheme()->GetProportionalScaledValue(28));
		}
		else
		{
			pPropertySheet->_activePage->SetBounds(0, 0, wide, tall);
		}
		pPropertySheet->_activePage->InvalidateLayout();
	}

	int limit = pPropertySheet->_pageTabs.GetCount();

	int xtab = 0;

	// draw the visible tabs
	if (pPropertySheet->_showTabs)
	{
		for (int i = 0; i < limit; i++)
		{
			int width, tall;

			pPropertySheet->_pageTabs[i]->GetSize(width, tall);
			if (pPropertySheet->_pageTabs[i] == pPropertySheet->_activeTab)
			{
				// active tab is taller
				pPropertySheet->_activeTab->SetBounds(xtab,
					vgui::scheme()->GetProportionalScaledValue(2),
					width,
					vgui::scheme()->GetProportionalScaledValue(27));
			}
			else
			{
				pPropertySheet->_pageTabs[i]->SetBounds(xtab,
					vgui::scheme()->GetProportionalScaledValue(4),
					width,
					vgui::scheme()->GetProportionalScaledValue(25));
			}
			pPropertySheet->_pageTabs[i]->SetVisible(true);
			xtab += (width + vgui::scheme()->GetProportionalScaledValue(1));
		}
	}
	else
	{
		for (int i = 0; i < limit; i++)
		{
			pPropertySheet->_pageTabs[i]->SetVisible(false);
		}
	}
#endif

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
	int offset_activePage = 144;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		offset_activePage = 148;
	}

	auto pPropertySheet = (CPropertySheet_Legacy*)((PUCHAR)pthis + offset_activePage - offsetof(CPropertySheet_Legacy, _activePage));

	auto _activePage = pPropertySheet->_activePage;

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
			PVOID* COptionsDialog_vftable = *(PVOID**)m_pDialog;
			void* (__fastcall * pfnGetFocusNavGroup)(vgui::Panel * pthis, int dummy) = (decltype(pfnGetFocusNavGroup))COptionsDialog_vftable[612 / 4];

			auto FocusNavGroup = pfnGetFocusNavGroup(m_pDialog, 0);
			PVOID* FocusNavGroup_vftable = *(PVOID**)FocusNavGroup;

			gPrivateFuncs.GameUI_FocusNavGroup_GetCurrentFocus = (decltype(gPrivateFuncs.GameUI_FocusNavGroup_GetCurrentFocus))FocusNavGroup_vftable[7];
			Install_InlineHook(GameUI_FocusNavGroup_GetCurrentFocus);
		}

		if (!gPrivateFuncs.GameUI_PropertySheet_HasHotkey)
		{
			PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

			gPrivateFuncs.GameUI_PropertySheet_HasHotkey = (decltype(gPrivateFuncs.GameUI_PropertySheet_HasHotkey))_propertySheet_vftable[73];
			Install_InlineHook(GameUI_PropertySheet_HasHotkey);
		}

		if (!gPrivateFuncs.GameUI_PropertySheet_PerformLayout)
		{
			PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

			gPrivateFuncs.GameUI_PropertySheet_PerformLayout = (decltype(gPrivateFuncs.GameUI_PropertySheet_PerformLayout))_propertySheet_vftable[111];
			Install_InlineHook(GameUI_PropertySheet_PerformLayout);
		}
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
			(decltype(pfnAddPage))_propertySheet_vftable[134];

		pfnAddPage(m_pPropertySheet, 0, (vgui::Panel*)panel, title);
	}

	void SetActivePage(void* panel) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnSetActivePage)(vgui::Panel * pthis, int dummy, vgui::Panel * panel) =
			(decltype(pfnSetActivePage))_propertySheet_vftable[135];

		pfnSetActivePage(m_pPropertySheet, 0, (vgui::Panel*)panel);
	}

	void SetTabWidth(int width) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnSetTabWidth)(vgui::Panel * pthis, int dummy, int width) =
			(decltype(pfnSetTabWidth))_propertySheet_vftable[136];

		pfnSetTabWidth(m_pPropertySheet, 0, width);
	}

	void* GetActivePage() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void* (__fastcall * pfnGetActivePage)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetActivePage))_propertySheet_vftable[137];

		return pfnGetActivePage(m_pPropertySheet, 0);
	}

	void ResetAllData() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnResetAllData)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnResetAllData))_propertySheet_vftable[138];

		pfnResetAllData(m_pPropertySheet, 0);
	}

	void ApplyChanges() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnApplyChanges)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnApplyChanges))_propertySheet_vftable[139];

		pfnApplyChanges(m_pPropertySheet, 0);
	}

	void* GetPage(int i) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void* (__fastcall * pfnGetPage)(vgui::Panel * pthis, int dummy, int i) =
			(decltype(pfnGetPage))_propertySheet_vftable[140];

		return pfnGetPage(m_pPropertySheet, 0, i);
	}

	void DeletePage(void* panel) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnDeletePage)(vgui::Panel * pthis, int dummy, void* panel) =
			(decltype(pfnDeletePage))_propertySheet_vftable[141];

		pfnDeletePage(m_pPropertySheet, 0, panel);
	}

	void* GetActiveTab() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void* (__fastcall * pfnGetActiveTab)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetActiveTab))_propertySheet_vftable[142];

		return pfnGetActiveTab(m_pPropertySheet, 0);
	}

	void GetActiveTabTitle(char* textOut, int bufferLen) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnGetActiveTabTitle)(vgui::Panel * pthis, int dummy, char* textOut, int bufferLen) =
			(decltype(pfnGetActiveTabTitle))_propertySheet_vftable[143];

		pfnGetActiveTabTitle(m_pPropertySheet, 0, textOut, bufferLen);
	}

	bool GetTabTitle(int i, char* textOut, int bufferLen) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		bool(__fastcall * pfnGetTabTitle)(vgui::Panel * pthis, int dummy, int i, char* textOut, int bufferLen) =
			(decltype(pfnGetTabTitle))_propertySheet_vftable[144];

		return pfnGetTabTitle(m_pPropertySheet, 0, i, textOut, bufferLen);
	}

	int GetActivePageNum() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		int(__fastcall * pfnGetActivePageNum)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetActivePageNum))_propertySheet_vftable[145];

		return pfnGetActivePageNum(m_pPropertySheet, 0);
	}

	int GetNumPages() override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		int(__fastcall * pfnGetGetNumPages)(vgui::Panel * pthis, int dummy) =
			(decltype(pfnGetGetNumPages))_propertySheet_vftable[146];

		return pfnGetGetNumPages(m_pPropertySheet, 0);
	}

	void DisablePage(const char* title) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnDisablePage)(vgui::Panel * pthis, int dummy, const char* title) =
			(decltype(pfnDisablePage))_propertySheet_vftable[147];

		pfnDisablePage(m_pPropertySheet, 0, title);
	}

	void EnablePage(const char* title) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnEnablePage)(vgui::Panel * pthis, int dummy, const char* title) =
			(decltype(pfnEnablePage))_propertySheet_vftable[148];

		pfnEnablePage(m_pPropertySheet, 0, title);
	}

	void ChangeActiveTab(int index) override
	{
		PVOID* _propertySheet_vftable = *(PVOID**)m_pPropertySheet;

		void(__fastcall * pfnChangeActiveTab)(vgui::Panel * pthis, int dummy, int index) =
			(decltype(pfnChangeActiveTab))_propertySheet_vftable[149];

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

	void InstallHooks()
	{
		if (!gPrivateFuncs.CTaskBar_OnCommand)
		{
			gPrivateFuncs.CTaskBar_vftable = *(PVOID**)m_pTaskBar;
			gPrivateFuncs.CTaskBar_OnCommand = (decltype(gPrivateFuncs.CTaskBar_OnCommand))gPrivateFuncs.CTaskBar_vftable[348 / 4];

			Install_InlineHook(CTaskBar_OnCommand);
		}
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

	CallbackContext.InstallHooks();

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
		if (!gPrivateFuncs.CBasePanel_vftable)
		{
			gPrivateFuncs.CBasePanel_vftable = *(PVOID**)m_pBasePanel;
			gPrivateFuncs.CBasePanel_ApplySchemeSettings = (decltype(gPrivateFuncs.CBasePanel_ApplySchemeSettings))gPrivateFuncs.CBasePanel_vftable[0x13C / 4];

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

#if 0
void* __fastcall QueryBox_ctor(vgui::Panel* pthis, int dummy, const char* title, const char* queryText, vgui::Panel* parent)
{
	//Load res to make it proportional
	if (!strcmp(queryText, "#GameUI_QuitConfirmationText"))
	{
		//gPrivateFuncs.GameUI_LoadControlSettings(pthis, 0, "Resource/QuitConfirmationBox.res", NULL);

		return gPrivateFuncs.QueryBox_ctor(pthis, dummy, title, "#GameUI_QuitConfirmationText\n\n", parent);
	}
	auto result = gPrivateFuncs.QueryBox_ctor(pthis, dummy, title, queryText, parent);

	return result;
}
#endif

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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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
	}
	default:
	{
		ret = real_ret;
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

void GameUI_FillAddress_GameConsoleDialog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "#GameUI_Console\0";
	auto GameUI_Console_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs);
	if (!GameUI_Console_String)
		GameUI_Console_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs);
	Sig_VarNotFound(GameUI_Console_String);

	char pattern[] = "\x6A\x01\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 3) = (DWORD)GameUI_Console_String;
	auto GameUI_Console_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(GameUI_Console_PushString);

	PVOID CGameConsoleDialog_ctor_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(GameUI_Console_PushString, 0x450, [](PUCHAR Candidate) {

		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC)
			return TRUE;

		//.text:10027EC0 53                                                  push    ebx
		//.text : 10027EC1 8B DC                                               mov     ebx, esp
		if (Candidate[0] == 0x53 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xDC)
			return TRUE;

		//.text:10033D30 81 EC 00 08 00 00                                   sub     esp, 800h
		if (Candidate[0] == 0x81 &&
			Candidate[1] == 0xEC &&
			Candidate[4] == 0x00 &&
			Candidate[5] == 0x00)
		{
			return TRUE;
		}
		return FALSE;
		});

	gPrivateFuncs.CGameConsoleDialog_ctor = (decltype(gPrivateFuncs.CGameConsoleDialog_ctor))ConvertDllInfoSpace(CGameConsoleDialog_ctor_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(CGameConsoleDialog_ctor);
}

void GameUI_FillAddress_CreateMultiplayerGameDialog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "CreateMultiplayerGameDialog\0";
	auto CreateMultiplayerGameDialog_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!CreateMultiplayerGameDialog_String)
		CreateMultiplayerGameDialog_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
	Sig_VarNotFound(CreateMultiplayerGameDialog_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)CreateMultiplayerGameDialog_String;
	auto CreateMultiplayerGameDialog_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(CreateMultiplayerGameDialog_PushString);

	PVOID CCreateMultiplayerGameDialog_ctor_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(CreateMultiplayerGameDialog_PushString, 0x120, [](PUCHAR Candidate) {

		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC)
			return TRUE;

		//8B 44 24 04                                         mov     eax, [esp+arg_0]
		if (Candidate[0] == 0x8B &&
			Candidate[1] == 0x44 &&
			Candidate[2] == 0x24)
		{
			//.text:1001B472 68 CC 01 00 00                                      push    1CCh
			//text : 1001B477 68 5C 01 00 00                                     push    15Ch
			if (Search_Pattern_From_Size(Candidate, 0x30, "\x68\xCC\x01\x00\x00\x68\x5C\x01\x00\x00"))
			{
				return TRUE;
			}
		}
		return FALSE;
		});

	gPrivateFuncs.CCreateMultiplayerGameDialog_ctor = (decltype(gPrivateFuncs.CCreateMultiplayerGameDialog_ctor))ConvertDllInfoSpace(CCreateMultiplayerGameDialog_ctor_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(CCreateMultiplayerGameDialog_ctor);
}

void GameUI_FillAddress_COptionsDialog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "#GameUI_Options";
	auto GameUI_Options_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!GameUI_Options_String)
		GameUI_Options_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
	Sig_VarNotFound(GameUI_Options_String);

	char pattern[] = "\x6A\x01\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 3) = (DWORD)GameUI_Options_String;
	auto GameUI_Options_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(GameUI_Options_Call);

	gPrivateFuncs.COptionsDialog_ctor = (decltype(gPrivateFuncs.COptionsDialog_ctor))
		g_pMetaHookAPI->ReverseSearchFunctionBeginEx(GameUI_Options_Call, 0x300, [](PUCHAR Candidate) {
		//.text : 10016CB0 55                                                  push    ebp
		//.text : 10016CB1 8B EC                                               mov     ebp, esp
		//.text : 10016CB3 6A FF
		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC &&
			Candidate[3] == 0x6A &&
			Candidate[4] == 0xFF)
			return TRUE;

		//8B 44 24 04                                         mov     eax, [esp+arg_0]
		if (Candidate[0] == 0x8B &&
			Candidate[1] == 0x44 &&
			Candidate[2] == 0x24)
		{
			//.text : 100377D2 68 96 01 00 00                                      push    196h
			//.text : 100377D7 68 00 02 00 00                                      push    200h
			if (g_pMetaHookAPI->SearchPattern(Candidate, 0x30, "\x68\x96\x01\x00\x00\x68\x00\x02\x00\x00", sizeof("\x68\x96\x01\x00\x00\x68\x00\x02\x00\x00") - 1))
			{
				return TRUE;
			}
		}
		return FALSE;
			});

	Sig_FuncNotFound(COptionsDialog_ctor);
}

#if 0
void GameUI_FillAddress_QueryBox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "#GameUI_QuitConfirmationTitle";
	auto GameUI_QuitConfirmationTitle_String = g_pMetaHookAPI->SearchPattern(GameUIRdataBase, GameUIRdataSize, sigs1, sizeof(sigs1) - 1);
	if (!GameUI_QuitConfirmationTitle_String)
		GameUI_QuitConfirmationTitle_String = g_pMetaHookAPI->SearchPattern(GameUIDataBase, GameUIDataSize, sigs1, sizeof(sigs1) - 1);
	Sig_VarNotFound(GameUI_QuitConfirmationTitle_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 6) = (DWORD)GameUI_QuitConfirmationTitle_String;
	auto GameUI_QuitConfirmationTitle_PushString = g_pMetaHookAPI->SearchPattern(GameUITextBase, GameUITextSize, pattern, sizeof(pattern) - 1);

	Sig_VarNotFound(GameUI_QuitConfirmationTitle_PushString);

	g_pMetaHookAPI->DisasmRanges(GameUI_QuitConfirmationTitle_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;

		if (address[0] == 0xE8 && instCount <= 8)
		{
			gPrivateFuncs.QueryBox_ctor = (decltype(gPrivateFuncs.QueryBox_ctor))GetCallAddress(address);

			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, NULL);

	Sig_FuncNotFound(QueryBox_ctor);
}
#endif

void GameUI_FillAddress_CCareerProfileFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsCZero)
	{
		const char sigs1[] = "ProfileSelectionBackground";
		auto ProfileSelectionBackground_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
		if (!ProfileSelectionBackground_String)
			ProfileSelectionBackground_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
		Sig_VarNotFound(ProfileSelectionBackground_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 6) = (DWORD)ProfileSelectionBackground_String;
		auto ProfileSelectionBackground_PushString = Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, pattern);
		Sig_VarNotFound(ProfileSelectionBackground_PushString);

		PVOID CCareerProfileFrame_ctor_VA = g_pMetaHookAPI->ReverseSearchFunctionBegin(ProfileSelectionBackground_PushString, 0x150);
		gPrivateFuncs.CCareerProfileFrame_ctor = (decltype(gPrivateFuncs.CCareerProfileFrame_ctor))ConvertDllInfoSpace(CCareerProfileFrame_ctor_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(CCareerProfileFrame_ctor);
	}
}

void GameUI_FillAddress_CCareerMapFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsCZero)
	{
		const char sigs1[] = "MapSelectionBackground";
		auto MapSelectionBackground_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
		if (!MapSelectionBackground_String)
			MapSelectionBackground_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
		Sig_VarNotFound(MapSelectionBackground_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 6) = (DWORD)MapSelectionBackground_String;
		auto MapSelectionBackground_PushString = Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, pattern);
		Sig_VarNotFound(MapSelectionBackground_PushString);

		PVOID CCareerMapFrame_ctor_VA = g_pMetaHookAPI->ReverseSearchFunctionBegin(MapSelectionBackground_PushString, 0x150);
		gPrivateFuncs.CCareerMapFrame_ctor = (decltype(gPrivateFuncs.CCareerMapFrame_ctor))ConvertDllInfoSpace(CCareerMapFrame_ctor_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(CCareerMapFrame_ctor);
	}
}

void GameUI_FillAddress_CCareerBotFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsCZero)
	{
		const char sigs1[] = "PoolBackground";
		auto PoolBackground_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
		if (!PoolBackground_String)
			PoolBackground_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
		Sig_VarNotFound(PoolBackground_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 6) = (DWORD)PoolBackground_String;
		auto PoolBackground_PushString = Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, pattern);
		Sig_VarNotFound(PoolBackground_PushString);

		PVOID CCareerBotFrame_ctor_VA = g_pMetaHookAPI->ReverseSearchFunctionBegin(PoolBackground_PushString, 0x150);
		gPrivateFuncs.CCareerBotFrame_ctor = (decltype(gPrivateFuncs.CCareerBotFrame_ctor))ConvertDllInfoSpace(CCareerBotFrame_ctor_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(CCareerBotFrame_ctor);
	}
}

void GameUI_FillAddress_COptionsSubAudio(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "#GameUI_Audio";
	auto GameUI_Audio_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!GameUI_Audio_String)
		GameUI_Audio_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
	Sig_VarNotFound(GameUI_Audio_String);

	char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x2A\x2A\x33\xC0\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 10) = (DWORD)GameUI_Audio_String;
	auto GameUI_Audio_Call = Search_Pattern_From_Size(gPrivateFuncs.COptionsDialog_ctor, 0x300, pattern);
	Sig_VarNotFound(GameUI_Audio_Call);

	gPrivateFuncs.COptionsSubAudio_ctor = (decltype(gPrivateFuncs.COptionsSubAudio_ctor))GetCallAddress(GameUI_Audio_Call);
	Sig_FuncNotFound(COptionsSubAudio_ctor);
}

void GameUI_FillAddress_COptionsSubVideo(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	{
		const char sigs1[] = "_setvideomode";
		auto SetVideoMode_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
		if (!SetVideoMode_String)
			SetVideoMode_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
		Sig_VarNotFound(SetVideoMode_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)SetVideoMode_String;
		auto SetVideoMode_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(SetVideoMode_PushString);

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			PVOID COptionsSubVideo_ApplyVidSettings_HL25_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(SetVideoMode_PushString, 0x300, [](PUCHAR Candidate) {
				//  .text : 1001D2C0 55                                                  push    ebp
				//	.text : 1001D2C1 8B EC                                               mov     ebp, esp
				//	.text : 1001D2C3 81 EC 0C 02 00 00                                   sub     esp, 20Ch
				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x81 &&
					Candidate[4] == 0xEC)
					return TRUE;

				//.text:1003DDB0 81 EC 08 02 00 00                                   sub     esp, 208h
				if (Candidate[0] == 0x81 &&
					Candidate[1] == 0xEC &&
					Candidate[4] == 0x00 &&
					Candidate[5] == 0x00)
				{
					return TRUE;
				}

				return FALSE;
				});
			gPrivateFuncs.COptionsSubVideo_ApplyVidSettings_HL25 = (decltype(gPrivateFuncs.COptionsSubVideo_ApplyVidSettings_HL25))ConvertDllInfoSpace(COptionsSubVideo_ApplyVidSettings_HL25_VA, DllInfo, RealDllInfo);
			Sig_FuncNotFound(COptionsSubVideo_ApplyVidSettings_HL25);
		}
		else
		{

			PVOID COptionsSubVideo_ApplyVidSettings_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(SetVideoMode_PushString, 0x300, [](PUCHAR Candidate) {
				//  .text : 1001D2C0 55                                                  push    ebp
				//	.text : 1001D2C1 8B EC                                               mov     ebp, esp
				//	.text : 1001D2C3 81 EC 0C 02 00 00                                   sub     esp, 20Ch
				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x81 &&
					Candidate[4] == 0xEC)
					return TRUE;

				//.text:1003DDB0 81 EC 08 02 00 00                                   sub     esp, 208h
				if (Candidate[0] == 0x81 &&
					Candidate[1] == 0xEC &&
					Candidate[4] == 0x00 &&
					Candidate[5] == 0x00)
				{
					return TRUE;
				}

				return FALSE;
				});

			gPrivateFuncs.COptionsSubVideo_ApplyVidSettings = (decltype(gPrivateFuncs.COptionsSubVideo_ApplyVidSettings))ConvertDllInfoSpace(COptionsSubVideo_ApplyVidSettings_VA, DllInfo, RealDllInfo);
			Sig_FuncNotFound(COptionsSubVideo_ApplyVidSettings);
		}
	}
	{
		const char sigs1[] = "#GameUI_Video";
		auto GameUI_Video_String = g_pMetaHookAPI->SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, sigs1, sizeof(sigs1) - 1);
		if (!GameUI_Video_String)
			GameUI_Video_String = g_pMetaHookAPI->SearchPattern(DllInfo.DataBase, DllInfo.DataSize, sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(GameUI_Video_String);

		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\x2A\x2A\x33\xC0\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 10) = (DWORD)GameUI_Video_String;
		auto GameUI_Video_Call = g_pMetaHookAPI->SearchPattern(gPrivateFuncs.COptionsDialog_ctor, 0x300, pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(GameUI_Video_Call);

		PVOID COptionsSubVideo_ctor = GetCallAddress(GameUI_Video_Call);
		gPrivateFuncs.COptionsSubVideo_ctor = (decltype(gPrivateFuncs.COptionsSubVideo_ctor))ConvertDllInfoSpace(COptionsSubVideo_ctor, DllInfo, RealDllInfo);
		Sig_FuncNotFound(COptionsSubVideo_ctor);
	}
}

void GameUI_FillAddress_ConsoleHistory(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "ConsoleHistory\0";
	auto ConsoleHistory_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!ConsoleHistory_String)
		ConsoleHistory_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
	Sig_VarNotFound(ConsoleHistory_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)ConsoleHistory_String;
	auto ConsoleHistory_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(ConsoleHistory_PushString);

	typedef struct ConsoleHistorySearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

		PVOID* ConsoleHistory_vftable{};
	}ConsoleHistorySearchContext;

	ConsoleHistorySearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(ConsoleHistory_PushString, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (ConsoleHistorySearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
		{
			auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
			if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
			{
				ctx->ConsoleHistory_vftable = candidate;
			}
		}

		if (ctx->ConsoleHistory_vftable)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_VarNotFound(ctx.ConsoleHistory_vftable);

	gPrivateFuncs.GameUI_RichText_OnThink = (decltype(gPrivateFuncs.GameUI_RichText_OnThink))
		GetVFunctionFromVFTable(ctx.ConsoleHistory_vftable, 0x158 / 4, DllInfo, RealDllInfo, RealDllInfo);

	Sig_FuncNotFound(GameUI_RichText_OnThink);
}

void GameUI_FillAddress_RichText(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs2[] = "Unable to condump to \0";
	auto UnableToCondump_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs2);
	if (!UnableToCondump_String)
		UnableToCondump_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs2);
	Sig_VarNotFound(UnableToCondump_String);

	char pattern2[] = "\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern2 + 1) = (DWORD)UnableToCondump_String;
	auto UnableToCondump_PushString = Search_Pattern_From_Size(DllInfo.TextBase, DllInfo.TextSize, pattern2);
	Sig_VarNotFound(UnableToCondump_PushString);

	typedef struct UnableToCondumpSearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

		bool bFound118h{};				//it's 128h for HL25
		bool bFound_RichText_Print{};	//it's 128h for HL25
	}UnableToCondumpSearchContext;

	UnableToCondumpSearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(UnableToCondump_PushString, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (UnableToCondumpSearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.disp >= 0x118 && pinst->detail->x86.operands[1].mem.disp <= 0x120)
		{
			ctx->bFound118h = true;
		}

		if (address[0] == 0xE8 && instCount <= 5)
		{
			if (ctx->bFound118h)
			{
				PVOID GameUI_RichText_InsertStringA = GetCallAddress(address);

				gPrivateFuncs.GameUI_RichText_InsertStringA = (decltype(gPrivateFuncs.GameUI_RichText_InsertStringA))
					ConvertDllInfoSpace(GameUI_RichText_InsertStringA, ctx->DllInfo, ctx->RealDllInfo);
			}
			else
			{
				PVOID GameUI_RichText_Print = GetCallAddress(address);

				gPrivateFuncs.GameUI_RichText_Print = (decltype(gPrivateFuncs.GameUI_RichText_Print))
					ConvertDllInfoSpace(GameUI_RichText_Print, ctx->DllInfo, ctx->RealDllInfo);
			}

			ctx->bFound_RichText_Print = true;

			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_VarNotFound(ctx.bFound_RichText_Print);
}

void GameUI_PatchAddress_RichText_InsertChar(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID RecursiveWalkBase = ConvertDllInfoSpace(
		(gPrivateFuncs.GameUI_RichText_Print) ? gPrivateFuncs.GameUI_RichText_Print : gPrivateFuncs.GameUI_RichText_InsertStringA,
		RealDllInfo,
		DllInfo
	);

	typedef struct RichText_Print_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID base{};
		size_t max_insts{};
		int max_depth{};
		std::set<PVOID> code{};
		std::set<PVOID> branches{};
		std::vector<walk_context_t> walks{};

		PVOID FunctionBeginCandidate{};
		int FunctionBeginCandidateDepth{};
		PVOID Found0xDCandidate{};
		int Found0xDCandidateInstCount{};
		bool Is0xDCandidatePatched{};
	}RichText_Print_SearchContext;

	RichText_Print_SearchContext ctx = { DllInfo, RealDllInfo };

	ctx.base = RecursiveWalkBase;
	ctx.max_insts = 1000;
	ctx.max_depth = 16;
	ctx.walks.emplace_back(ctx.base, 0x1000, 0);

	while (ctx.walks.size())
	{
		auto walk = ctx.walks[ctx.walks.size() - 1];
		ctx.walks.pop_back();

		g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (RichText_Print_SearchContext*)context;

			if (ctx->code.size() > ctx->max_insts)
				return TRUE;

			if (ctx->code.find(address) != ctx->code.end())
				return TRUE;

			ctx->code.emplace(address);

			/*
			Engine: 3266
.text:1006BE4D 0F BE 45 08                                         movsx   eax, byte ptr [ebp+arg_0]
.text:1006BE51 83 F8 0D                                            cmp     eax, 0Dh
.text:1006BE54 75 02                                               jnz     short loc_1006BE58 label_work
.text:1006BE56 EB 67                                               jmp     short loc_1006BEBF label_exit
				*/

				/*
				Engine: 4554, 6153
.text:100573B0 8A 44 24 04                                         mov     al, byte ptr [esp+arg_0]
.text:100573B4 55                                                  push    ebp
.text:100573B5 3C 0D                                               cmp     al, 0Dh
.text:100573B7 8B E9                                               mov     ebp, ecx
.text:100573B9 0F 84 8C 00 00 00                                   jz      loc_1005744B label_exit
				*/

				/*
				Engine: SvEngine
.text:10047463 80 7D 08 0D                                         cmp     [ebp+arg_0], 0Dh

.text:1004746A 74 3C                                               jz      short loc_100474A8 label_exit
				*/

				/*
				Engine: 9920
.text:1005D664 0F B7 C1                                            movzx   eax, cx
.text:1005D667 89 45 08                                            mov     [ebp+arg_0], eax
.text:1005D66A 80 F9 0D                                            cmp     cl, 0Dh
.text:1005D66D 74 3B                                               jz      short loc_1005D6AA label_exit
				*/

			if (instCount == 1)
			{
				ctx->FunctionBeginCandidate = address;
				ctx->FunctionBeginCandidateDepth = depth;
			}

			if (!ctx->Found0xDCandidate &&
				instCount < 25 &&
				depth == ctx->FunctionBeginCandidateDepth &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				(pinst->detail->x86.operands[0].type == X86_OP_REG || pinst->detail->x86.operands[0].type == X86_OP_MEM) &&
				pinst->detail->x86.operands[1].imm == 0x0D)
			{
				ctx->Found0xDCandidate = address;
				ctx->Found0xDCandidateInstCount = instCount;

				typedef struct RichText_InsertChar_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					bool IsFetchWord{};
				}RichText_InsertChar_SearchContext;

				RichText_InsertChar_SearchContext ctx2 = { ctx->DllInfo, ctx->RealDllInfo };

				g_pMetaHookAPI->DisasmRanges(ctx->FunctionBeginCandidate, address - ctx->FunctionBeginCandidate, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (RichText_InsertChar_SearchContext*)context;

					//66 8B 06                                            mov     ax, [esi]
					//0F B7 07                                            movzx   eax, word ptr [edi]
					//66 8B 08                                            mov     cx, [eax]

					if ((pinst->id == X86_INS_MOV || pinst->id == X86_INS_MOVZX) &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(pinst->detail->x86.operands[0].size == 2 || pinst->detail->x86.operands[1].size == 2) &&
						pinst->detail->x86.operands[1].mem.base != 0 &&
						pinst->detail->x86.operands[1].mem.disp == 0 &&
						pinst->detail->x86.operands[1].mem.index == 0 &&
						pinst->detail->x86.operands[1].mem.scale == 1)
					{
						ctx->IsFetchWord = true;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx2);

				if (ctx2.IsFetchWord)
				{
					gPrivateFuncs.GameUI_RichText_InsertStringW = (decltype(gPrivateFuncs.GameUI_RichText_InsertStringW))
						ConvertDllInfoSpace(ctx->FunctionBeginCandidate, ctx->DllInfo, ctx->RealDllInfo);
				}
				else
				{
					gPrivateFuncs.GameUI_RichText_InsertChar = (decltype(gPrivateFuncs.GameUI_RichText_InsertChar))
						ConvertDllInfoSpace(ctx->FunctionBeginCandidate, ctx->DllInfo, ctx->RealDllInfo);
				}
			}

			if (!ctx->Is0xDCandidatePatched &&
				ctx->Found0xDCandidateInstCount > 0 &&
				instCount > ctx->Found0xDCandidateInstCount &&
				instCount < ctx->Found0xDCandidateInstCount + 5 &&
				(pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				if (pinst->id == X86_INS_JE)
				{
					auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);
					g_pMetaHookAPI->WriteNOP(address_RealDllBased, instLen);
					ctx->Is0xDCandidatePatched = true;
				}
				else if (pinst->id == X86_INS_JNE)
				{
					if (instLen == 2)
					{
						//redirect jmp short
						auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);
						g_pMetaHookAPI->WriteBYTE(address_RealDllBased, 0xEB);
						ctx->Is0xDCandidatePatched = true;
					}
					else if (instLen == 5)
					{
						//redirect jmp
						auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);
						g_pMetaHookAPI->WriteBYTE(address_RealDllBased, 0xE9);
						ctx->Is0xDCandidatePatched = true;
					}
				}
			}

			if (ctx->Is0xDCandidatePatched)
				return TRUE;

			if ((pinst->id == X86_INS_CALL || pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
				if (imm >= (PUCHAR)ctx->DllInfo.TextBase && imm < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
				{
					auto foundbranch = ctx->branches.find(imm);
					if (foundbranch == ctx->branches.end())
					{
						ctx->branches.emplace(imm);
						if (depth + 1 < ctx->max_depth)
							ctx->walks.emplace_back(imm, 0x1000, depth + 1);
					}
				}

				if (pinst->id == X86_INS_JMP)
					return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, walk.depth, &ctx);
	}

	if (!ctx.Is0xDCandidatePatched)
	{
		Sys_Error("Failed to patch GameUI!RichText_InsertChar.");
	}

	if (!gPrivateFuncs.GameUI_RichText_InsertChar && !gPrivateFuncs.GameUI_RichText_InsertStringW)
	{
		Sys_Error("Failed to locate GameUI!RichText_InsertChar or RichText_InsertStringW.");
	}
}

void GameUI_FillAddress_ConsoleEntry(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "ConsoleEntry\0";
	auto ConsoleEntry_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!ConsoleEntry_String)
		ConsoleEntry_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
	Sig_VarNotFound(ConsoleEntry_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)ConsoleEntry_String;
	auto ConsoleEntry_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(ConsoleEntry_PushString);

	typedef struct ConsoleEntrySearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

		PVOID* ConsoleEntry_vftable{};
	} ConsoleEntrySearchContext;

	ConsoleEntrySearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(ConsoleEntry_PushString, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (ConsoleEntrySearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
		{
			auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
			if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
			{
				ctx->ConsoleEntry_vftable = candidate;
			}
		}

		if (ctx->ConsoleEntry_vftable)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_VarNotFound(ctx.ConsoleEntry_vftable);

	gPrivateFuncs.GameUI_TextEntry_OnKeyCodeTyped = (decltype(gPrivateFuncs.GameUI_TextEntry_OnKeyCodeTyped))
		GetVFunctionFromVFTable(ctx.ConsoleEntry_vftable, 0x194 / 4, DllInfo, RealDllInfo, RealDllInfo);

	//gPrivateFuncs.GameUI_TextEntry_InsertChar = (decltype(gPrivateFuncs.GameUI_TextEntry_InsertChar))
	// GetVFunctionFromVFTable(ctx.ConsoleEntry_vftable, 0x250 / 4, DllInfo, RealDllInfo, RealDllInfo);

	gPrivateFuncs.GameUI_TextEntry_LayoutVerticalScrollBarSlider = (decltype(gPrivateFuncs.GameUI_TextEntry_LayoutVerticalScrollBarSlider))
		GetVFunctionFromVFTable(ctx.ConsoleEntry_vftable, 0x2C0 / 4, DllInfo, RealDllInfo, RealDllInfo);

	gPrivateFuncs.GameUI_TextEntry_GetStartDrawIndex = (decltype(gPrivateFuncs.GameUI_TextEntry_GetStartDrawIndex))
		GetVFunctionFromVFTable(ctx.ConsoleEntry_vftable, 0x2F8 / 4, DllInfo, RealDllInfo, RealDllInfo);
}

void GameUI_FillAddress_Sheet(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID Sheet_PushString = NULL;

	const char sigs[] = "Sheet\0";
	auto PropertySheet_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs);

	if (PropertySheet_String)
	{
		//We got string in .rdata section
		PUCHAR SearchBegin = (PUCHAR)DllInfo.RdataBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.RdataBase + DllInfo.RdataSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs);
			if (pFound)
			{
				char pattern[] = "\x74\x2A\x68\x2A\x2A\x2A\x2A";

				*(DWORD*)(pattern + 3) = (DWORD)pFound;

				Sheet_PushString = Search_Pattern(pattern, DllInfo);

				if (Sheet_PushString)
				{
					break;
				}

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		//We got string in .data section
		PropertySheet_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs);
		if (PropertySheet_String)
		{
			PUCHAR SearchBegin = (PUCHAR)DllInfo.DataBase;
			PUCHAR SearchLimit = (PUCHAR)DllInfo.DataBase + DllInfo.DataSize;
			while (SearchBegin < SearchLimit)
			{
				PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, sigs);
				if (pFound)
				{
					char pattern[] = "\x74\x2A\x68\x2A\x2A\x2A\x2A";

					*(DWORD*)(pattern + 3) = (DWORD)pFound;

					Sheet_PushString = Search_Pattern(pattern, DllInfo);

					if (Sheet_PushString)
					{
						break;
					}

					SearchBegin = pFound + Sig_Length(sigs);
				}
				else
				{
					break;
				}
			}
		}
	}

	Sig_VarNotFound(Sheet_PushString);

	typedef struct SheetCtor_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int instCount_Sheet_ctor{};
	}SheetCtor_SearchContext;

	SheetCtor_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(Sheet_PushString, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (SheetCtor_SearchContext*)context;

		if (!gPrivateFuncs.GameUI_Sheet_ctor && address[0] == 0xE8 && instCount <= 8)
		{
			PVOID GameUI_Sheet_ctor_VA = GetCallAddress(address);

			gPrivateFuncs.GameUI_Sheet_ctor = (decltype(gPrivateFuncs.GameUI_Sheet_ctor))
				ConvertDllInfoSpace(GameUI_Sheet_ctor_VA, ctx->DllInfo, ctx->RealDllInfo);

			ctx->instCount_Sheet_ctor = instCount;
		}

		if (!gPrivateFuncs.offset_propertySheet && instCount > ctx->instCount_Sheet_ctor && instCount < ctx->instCount_Sheet_ctor + 10)
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 260 &&
				pinst->detail->x86.operands[0].mem.disp <= 280 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				gPrivateFuncs.offset_propertySheet = (decltype(gPrivateFuncs.offset_propertySheet))pinst->detail->x86.operands[0].mem.disp;
			}
		}

		if (gPrivateFuncs.GameUI_Sheet_ctor && gPrivateFuncs.offset_propertySheet)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_FuncNotFound(GameUI_Sheet_ctor);
	Sig_FuncNotFound(offset_propertySheet);
}

void GameUI_FillAddress_PropertySheet(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID GameUI_Sheet_ctor_VA = ConvertDllInfoSpace(gPrivateFuncs.GameUI_Sheet_ctor, RealDllInfo, DllInfo);

	typedef struct SheetSearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;

	}SheetSearchContext;

	SheetSearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(GameUI_Sheet_ctor_VA, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (SheetSearchContext*)context;

		if (!gPrivateFuncs.GameUI_Sheet_vftable)
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
			{
				auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
				if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
				{
					gPrivateFuncs.GameUI_Sheet_vftable = (decltype(gPrivateFuncs.GameUI_Sheet_vftable))ConvertDllInfoSpace(candidate, ctx->DllInfo, ctx->RealDllInfo);
				}
			}
		}

		if (gPrivateFuncs.GameUI_Sheet_vftable)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_FuncNotFound(GameUI_Sheet_vftable);

	gPrivateFuncs.GameUI_PropertySheet_HasHotkey = (decltype(gPrivateFuncs.GameUI_PropertySheet_HasHotkey))
		GetVFunctionFromVFTable(gPrivateFuncs.GameUI_Sheet_vftable, 73, DllInfo, RealDllInfo, RealDllInfo);

	gPrivateFuncs.GameUI_PropertySheet_PerformLayout = (decltype(gPrivateFuncs.GameUI_PropertySheet_PerformLayout))
		GetVFunctionFromVFTable(gPrivateFuncs.GameUI_Sheet_vftable, 111, DllInfo, RealDllInfo, RealDllInfo);

	Sig_FuncNotFound(GameUI_PropertySheet_HasHotkey);
	Sig_FuncNotFound(GameUI_PropertySheet_PerformLayout);
}

void GameUI_FillAddress_MessageBox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "MessageBoxText";
	auto MessageBoxText_String = g_pMetaHookAPI->SearchPattern(DllInfo.RdataBase, DllInfo.RdataSize, sigs1, sizeof(sigs1) - 1);
	if (!MessageBoxText_String)
	{
		MessageBoxText_String = g_pMetaHookAPI->SearchPattern(DllInfo.DataBase, DllInfo.DataSize, sigs1, sizeof(sigs1) - 1);
	}
	Sig_VarNotFound(MessageBoxText_String);
	char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)MessageBoxText_String;
	auto MessageBoxText_PushString = g_pMetaHookAPI->SearchPattern(DllInfo.TextBase, DllInfo.TextSize, pattern, sizeof(pattern) - 1);
	Sig_VarNotFound(MessageBoxText_PushString);

	{
		typedef struct MessageBoxText_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}MessageBoxText_SearchContext;

		MessageBoxText_SearchContext ctx = { DllInfo , RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(MessageBoxText_PushString, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (MessageBoxText_SearchContext*)context;

			if (address[0] == 0xE8 && instCount <= 8)
			{
				PVOID MessageBox_ctor_VA = GetCallAddress(address);
				gPrivateFuncs.MessageBox_ctor = (decltype(gPrivateFuncs.MessageBox_ctor))ConvertDllInfoSpace(MessageBox_ctor_VA, ctx->DllInfo, ctx->RealDllInfo);

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_FuncNotFound(MessageBox_ctor);
	}

	{
		typedef struct MessageBoxSearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} MessageBoxSearchContext;

		MessageBoxSearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(ConvertDllInfoSpace(gPrivateFuncs.MessageBox_ctor, RealDllInfo, DllInfo), 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (MessageBoxSearchContext*)context;

			if (!gPrivateFuncs.MessageBox_vftable)
			{
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.disp == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
				{
					auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;
					if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
					{
						gPrivateFuncs.MessageBox_vftable = (decltype(gPrivateFuncs.MessageBox_vftable))ConvertDllInfoSpace(candidate, ctx->DllInfo, ctx->RealDllInfo);
					}
				}
			}

			if (gPrivateFuncs.MessageBox_vftable)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_FuncNotFound(MessageBox_vftable);
	}

	gPrivateFuncs.MessageBox_ApplySchemeSettings = (decltype(gPrivateFuncs.MessageBox_ApplySchemeSettings))
		GetVFunctionFromVFTable(gPrivateFuncs.MessageBox_vftable, 0x13C / 4, DllInfo, RealDllInfo, RealDllInfo);

	Sig_FuncNotFound(MessageBox_ApplySchemeSettings);
}

void GameUI_PatchAddress_MessageBox_ApplySchemeSettings(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType != ENGINE_GOLDSRC_HL25)
	{
		PVOID MessageBox_ApplySchemeSettings_VA = ConvertDllInfoSpace(gPrivateFuncs.MessageBox_ApplySchemeSettings, RealDllInfo, DllInfo);

		typedef struct MessageBox_ApplySchemeSettings_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			std::set<PVOID> addr_SetSize;
			int instCount_Add64h{};
		}MessageBox_ApplySchemeSettings_SearchContext;

		MessageBox_ApplySchemeSettings_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(MessageBox_ApplySchemeSettings_VA, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (MessageBox_ApplySchemeSettings_SearchContext*)context;

			if (!ctx->instCount_Add64h &&
				pinst->id == X86_INS_ADD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x64)
			{
				ctx->instCount_Add64h = instCount;
			}

			if (address[0] == 0xE8 && ctx->instCount_Add64h && instCount > ctx->instCount_Add64h && instCount < ctx->instCount_Add64h + 15)
			{
				auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

				auto Candidate = GetCallAddress(address);

				auto Candidate_RealDllBased = ConvertDllInfoSpace(Candidate, ctx->DllInfo, ctx->RealDllInfo);

				if (Candidate_RealDllBased)
				{
					if (Candidate_RealDllBased == gPrivateFuncs.GameUI_Panel_SetSize)
					{
						ctx->addr_SetSize.emplace(address_RealDllBased);
					}
					else if (!gPrivateFuncs.GameUI_Panel_SetSize && VGUI2_IsPanelSetSize(Candidate))
					{
						gPrivateFuncs.GameUI_Panel_SetSize = (decltype(gPrivateFuncs.GameUI_Panel_SetSize))Candidate_RealDllBased;

						ctx->addr_SetSize.emplace(address_RealDllBased);
					}
				}

				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_FuncNotFound(GameUI_Panel_SetSize);

		for (auto addr : ctx.addr_SetSize)
		{
			g_pMetaHookAPI->InlinePatchRedirectBranch(addr, GameUI_MessageBox_ApplySchemeSettings_Panel_SetSize, NULL);
		}
	}
}

void GameUI_FillAddress_CBasePanel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "Resource/gameui_%language%.txt";
	const char sigs2[] = "resource/gameui_%language%.txt";

	auto GameUI_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!GameUI_String)
	{
		GameUI_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
	}
	if (!GameUI_String)
	{
		GameUI_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs2);
		if (!GameUI_String)
		{
			GameUI_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs2);
		}
	}
	Sig_VarNotFound(GameUI_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)GameUI_String;

	auto GameUI_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(GameUI_PushString);

	typedef struct CGameUIInitializeSearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID OperatorNewAddress{};
	} CGameUIInitializeSearchContext;

	CGameUIInitializeSearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(GameUI_PushString, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (CGameUIInitializeSearchContext*)context;

		if (pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM &&
			pinst->detail->x86.operands[0].imm >= 0x90 && pinst->detail->x86.operands[0].imm <= 0x100)
		{
			auto nextaddr = address + instLen;

			if (nextaddr[0] == 0xE8)
			{
				ctx->OperatorNewAddress = nextaddr;
			}
		}
		else if (ctx->OperatorNewAddress && address > ctx->OperatorNewAddress && address[0] == 0xE8)
		{
			PVOID callTarget = GetCallAddress(address);

			gPrivateFuncs.CBasePanel_ctor = (decltype(gPrivateFuncs.CBasePanel_ctor))
				ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);

			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	Sig_FuncNotFound(CBasePanel_ctor);
}

void GameUI_FillAddress_CTaskBar(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs1[] = "GameMenuButton\0";
	auto GameMenuButton_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
	if (!GameMenuButton_String)
		GameMenuButton_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);

	Sig_VarNotFound(GameMenuButton_String);

	char pattern[] = "\x74\x2A\x68\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 3) = (DWORD)GameMenuButton_String;

	auto GameMenuButton_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(GameMenuButton_PushString);

	PVOID CTaskBar_ctor_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(GameMenuButton_PushString, 0x350, [](PUCHAR Candidate) {

		if (Candidate[0] == 0x55 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xEC &&
			Candidate[3] == 0x83 &&
			Candidate[4] == 0xEC)
			return TRUE;

		if (Candidate[0] == 0x53 &&
			Candidate[1] == 0x8B &&
			Candidate[2] == 0xDC &&
			Candidate[3] == 0x83 &&
			Candidate[4] == 0xEC)
			return TRUE;

		//8B 44 24 04                                         mov     eax, [esp+arg_0]
		if (Candidate[0] == 0x8B &&
			Candidate[1] == 0x44 &&
			Candidate[2] == 0x24)
		{
			//.text:1002AD81 83 C8 FF                                            or      eax, 0FFFFFFFFh
			if (Search_Pattern_From_Size(Candidate, 0x100, "\x83\xC8\xFF"))
			{
				return TRUE;
			}
		}
		return FALSE;
	});
	
	gPrivateFuncs.CTaskBar_ctor = (decltype(gPrivateFuncs.CTaskBar_ctor))
		ConvertDllInfoSpace(CTaskBar_ctor_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(CTaskBar_ctor);
}

void GameUI_FillAddress_CTaskBarKeyValues(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID CTaskBar_ctor_VA = ConvertDllInfoSpace(gPrivateFuncs.CTaskBar_ctor, RealDllInfo, DllInfo);

	if (!CTaskBar_ctor_VA)
	{
		Sig_NotFound(CTaskBar_ctor_VA);
	}

	typedef struct CTaskBarCtorSearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} CTaskBarCtorSearchContext;

	CTaskBarCtorSearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(CTaskBar_ctor_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (CTaskBarCtorSearchContext*)context;

		if (!gPrivateFuncs.CTaskBar_vftable)
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.disp == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize))
			{
				auto candidate = (PVOID*)pinst->detail->x86.operands[1].imm;

				if (candidate[0] >= (PUCHAR)ctx->DllInfo.TextBase && candidate[0] < (PUCHAR)ctx->DllInfo.TextBase + ctx->DllInfo.TextSize)
				{
					gPrivateFuncs.CTaskBar_vftable = (decltype(gPrivateFuncs.CTaskBar_vftable))ConvertDllInfoSpace(candidate, ctx->DllInfo, ctx->RealDllInfo);
					gPrivateFuncs.CTaskBar_OnCommand = (decltype(gPrivateFuncs.CTaskBar_OnCommand))GetVFunctionFromVFTable(gPrivateFuncs.CTaskBar_vftable, 348 / 4, ctx->DllInfo, ctx->RealDllInfo, ctx->RealDllInfo);
				}
			}
		}

		if (address[0] == 0xE8)
		{
			PVOID call_candidate = (decltype(call_candidate))GetCallAddress(address);

			typedef struct CTaskBarCtor_SearchContext2_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;

				bool bHasPush18h{};
				bool bHasPushGameMenu{};
				int instCount_PushGameMenu{};
			} CTaskBarCtor_SearchContext2;

			CTaskBarCtor_SearchContext2 ctx2 = { ctx->DllInfo, ctx->RealDllInfo };

			g_pMetaHookAPI->DisasmRanges(call_candidate, 0x350, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx2 = (CTaskBarCtor_SearchContext2*)context;

				if (!ctx2->bHasPush18h &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0x18)
				{
					ctx2->bHasPush18h = true;
				}

				if (ctx2->bHasPush18h &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					(
						((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx2->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx2->DllInfo.DataBase + ctx2->DllInfo.DataSize) ||
						((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx2->DllInfo.RdataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx2->DllInfo.RdataBase + ctx2->DllInfo.RdataSize)
						))
				{
					auto pString = (PCHAR)pinst->detail->x86.operands[0].imm;

					if (!memcmp(pString, "GameMenu\0", sizeof("GameMenu\0") - 1))
					{
						ctx2->bHasPushGameMenu = true;
						ctx2->instCount_PushGameMenu = instCount;
					}
				}

				if (!gPrivateFuncs.GameUI_KeyValues_ctor)
				{
					if (address[0] == 0xE8)
					{
						if (ctx2->bHasPushGameMenu && instCount > ctx2->instCount_PushGameMenu && instCount < ctx2->instCount_PushGameMenu + 5)
						{
							PVOID GameUI_KeyValues_ctor_VA = GetCallAddress(address);

							gPrivateFuncs.GameUI_KeyValues_ctor = (decltype(gPrivateFuncs.GameUI_KeyValues_ctor))ConvertDllInfoSpace(GameUI_KeyValues_ctor_VA, ctx2->DllInfo, ctx2->RealDllInfo);

							g_pMetaHookAPI->DisasmRanges(GameUI_KeyValues_ctor_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

								auto pinst = (cs_insn*)inst;
								auto ctx2 = (CTaskBarCtor_SearchContext2*)context;

								if (!gPrivateFuncs.GameUI_KeyValues_vftable)
								{
									if (pinst->id == X86_INS_MOV &&
										pinst->detail->x86.op_count == 2 &&
										pinst->detail->x86.operands[0].type == X86_OP_MEM &&
										pinst->detail->x86.operands[1].type == X86_OP_IMM &&
										((PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx2->DllInfo.RdataBase &&
											(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx2->DllInfo.RdataBase + ctx2->DllInfo.RdataSize))
									{
										auto candidate_vftable = (PVOID*)pinst->detail->x86.operands[1].imm;

										if (candidate_vftable[0] >= (PUCHAR)ctx2->DllInfo.TextBase && candidate_vftable[0] < (PUCHAR)ctx2->DllInfo.TextBase + ctx2->DllInfo.TextSize)
										{
											gPrivateFuncs.GameUI_KeyValues_vftable = (decltype(gPrivateFuncs.GameUI_KeyValues_vftable))
												ConvertDllInfoSpace(candidate_vftable, ctx2->DllInfo, ctx2->RealDllInfo);

											gPrivateFuncs.GameUI_KeyValues_LoadFromFile = (decltype(gPrivateFuncs.GameUI_KeyValues_LoadFromFile))
												GetVFunctionFromVFTable(candidate_vftable, 2, ctx2->DllInfo, ctx2->RealDllInfo, ctx2->RealDllInfo);
										}
									}
								}

								if (gPrivateFuncs.GameUI_KeyValues_vftable &&
									gPrivateFuncs.GameUI_KeyValues_LoadFromFile)
									return TRUE;

								if (address[0] == 0xCC)
									return TRUE;

								if (pinst->id == X86_INS_RET)
									return TRUE;

								return FALSE;

								}, 0, ctx2);
						}
					}
				}

				if (gPrivateFuncs.GameUI_KeyValues_ctor &&
					gPrivateFuncs.GameUI_KeyValues_vftable &&
					gPrivateFuncs.GameUI_KeyValues_LoadFromFile)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

			}, 0, &ctx2);
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	Sig_FuncNotFound(GameUI_KeyValues_ctor);
	Sig_FuncNotFound(GameUI_KeyValues_LoadFromFile);
}

void GameUI_FillAddress(void)
{
	if (!g_GameUIDllInfo.ImageBase)
	{
		Sys_Error("GameUI.dll is not available!");
		return;
	}

	GameUI_FillAddress_GameConsoleDialog(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CreateMultiplayerGameDialog(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_COptionsDialog(g_GameUIDllInfo, g_GameUIDllInfo);

	//GameUI_FillAddress_QueryBox(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CCareerProfileFrame(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CCareerMapFrame(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CCareerBotFrame(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_COptionsSubAudio(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_COptionsSubVideo(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_ConsoleHistory(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_RichText(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_PatchAddress_RichText_InsertChar(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_ConsoleEntry(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_Sheet(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_PropertySheet(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_MessageBox(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_PatchAddress_MessageBox_ApplySchemeSettings(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CBasePanel(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CTaskBar(g_GameUIDllInfo, g_GameUIDllInfo);

	GameUI_FillAddress_CTaskBarKeyValues(g_GameUIDllInfo, g_GameUIDllInfo);

	gPrivateFuncs.GameUI_Panel_Init = (decltype(gPrivateFuncs.GameUI_Panel_Init))VGUI2_FindPanelInit(g_GameUIDllInfo, g_GameUIDllInfo);
	Sig_FuncNotFound(GameUI_Panel_Init);

	gPrivateFuncs.GameUI_Menu_vftable = (decltype(gPrivateFuncs.GameUI_Menu_vftable))VGUI2_FindMenuVFTable(g_GameUIDllInfo, g_GameUIDllInfo);
	Sig_FuncNotFound(GameUI_Menu_vftable);

	for (int index = 175; index < 182; ++index)
	{
		int offset_ScrollBar = 0;
		if (VGUI2_IsMenuMakeItemsVisibleInScrollRange(gPrivateFuncs.GameUI_Menu_vftable[index], &offset_ScrollBar))
		{
			gPrivateFuncs.offset_ScrollBar = offset_ScrollBar;
			gPrivateFuncs.GameUI_Menu_MakeItemsVisibleInScrollRange =
				(decltype(gPrivateFuncs.GameUI_Menu_MakeItemsVisibleInScrollRange))
				gPrivateFuncs.GameUI_Menu_vftable[index];
			break;
		}
	}

	Sig_FuncNotFound(GameUI_Menu_MakeItemsVisibleInScrollRange);
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
	Install_InlineHook(COptionsSubVideo_ApplyVidSettings);

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

void ServerBrowser_PatchAddress_BaseGamesPage(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType != ENGINE_GOLDSRC_HL25)
	{
		const char sigs1[] = "servers/%sPage_Filters.res";
		auto sPage_Filters_String = Search_Pattern_From_Size(DllInfo.RdataBase, DllInfo.RdataSize, sigs1);
		if (!sPage_Filters_String)
			sPage_Filters_String = Search_Pattern_From_Size(DllInfo.DataBase, DllInfo.DataSize, sigs1);
		if (sPage_Filters_String)
		{
			char pattern[] = "\x68\x16\x01\x00\x00\x68\x70\x02\x00";
			auto CBaseGamesPage_OnButtonToggled_SetSizeImm = Search_Pattern(pattern, DllInfo);
			Sig_VarNotFound(CBaseGamesPage_OnButtonToggled_SetSizeImm);

			//gPrivateFuncs.CServerBrowserDialog_ctor = (decltype(gPrivateFuncs.CServerBrowserDialog_ctor))g_pMetaHookAPI->ReverseSearchFunctionBegin(DialogServerBrowser_Call, 0x800);
			//Sig_FuncNotFound(CServerBrowserDialog_ctor);

			typedef struct OnButtonToggled_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
				std::set<PVOID> addrSets_SetSize;
				int instCount_push270h{};
			}OnButtonToggled_SearchContext;

			OnButtonToggled_SearchContext ctx = { DllInfo, RealDllInfo };

			ctx.instCount_push270h = 0;

			g_pMetaHookAPI->DisasmRanges(CBaseGamesPage_OnButtonToggled_SetSizeImm, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (OnButtonToggled_SearchContext*)context;

				if (address[0] == 0xE8 && instCount <= 8)
				{
					auto Candidate = GetCallAddress(address);

					if (VGUI2_IsPanelSetSize(Candidate))
					{
						gPrivateFuncs.ServerBrowser_Panel_SetSize = (decltype(gPrivateFuncs.ServerBrowser_Panel_SetSize))
							ConvertDllInfoSpace(Candidate, ctx->DllInfo, ctx->RealDllInfo);

						ctx->addrSets_SetSize.emplace(address);
					}

					return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

				}, 0, &ctx);

			Sig_FuncNotFound(ServerBrowser_Panel_SetSize);

			char pattern2[] = "\x68\x16\x01\x00\x00";
			PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
			PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
			while (SearchBegin < SearchLimit)
			{
				PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern2);
				if (pFound)
				{
					if (ctx.addrSets_SetSize.find(pFound) == ctx.addrSets_SetSize.end())
					{
						ctx.instCount_push270h = 0;
						g_pMetaHookAPI->DisasmRanges(pFound, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

							auto pinst = (cs_insn*)inst;
							auto ctx = (OnButtonToggled_SearchContext*)context;

							if (!ctx->instCount_push270h &&
								pinst->id == X86_INS_PUSH &&
								pinst->detail->x86.op_count == 1 &&
								pinst->detail->x86.operands[0].type == X86_OP_IMM &&
								pinst->detail->x86.operands[0].imm == 0x270)
							{
								ctx->instCount_push270h = instCount;
							}

							if (address[0] == 0xE8 && instCount > ctx->instCount_push270h && instCount <= ctx->instCount_push270h + 5)
							{
								PVOID callTarget = GetCallAddress(address);
								PVOID callTarget_RealDllInfoBased = ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);

								if (callTarget_RealDllInfoBased == gPrivateFuncs.ServerBrowser_Panel_SetSize)
								{
									ctx->addrSets_SetSize.emplace(address);
									return TRUE;
								}

								if (!gPrivateFuncs.ServerBrowser_Panel_SetSize && VGUI2_IsPanelSetSize(callTarget))
								{
									gPrivateFuncs.ServerBrowser_Panel_SetSize = (decltype(gPrivateFuncs.ServerBrowser_Panel_SetSize))callTarget_RealDllInfoBased;

									ctx->addrSets_SetSize.emplace(address);

									return TRUE;
								}
							}

							if (address[0] == 0xCC)
								return TRUE;

							if (pinst->id == X86_INS_RET)
								return TRUE;

							return FALSE;

						}, 0, &ctx);
					}

					SearchBegin = pFound + Sig_Length(pattern2);
				}
				else
				{
					break;
				}
			}

			for (auto addr : ctx.addrSets_SetSize)
			{
				g_pMetaHookAPI->InlinePatchRedirectBranch(addr, ServerBrowser_Panel_SetSize, NULL);
			}
		}
	}
}

void ServerBrowser_PatchAddress_ServerBrowserDialog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType != ENGINE_GOLDSRC_HL25)
	{
		typedef struct CServerBrowserDialog_ctor_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			std::set<PVOID> addrSets_SetSize;
			std::set<PVOID> addrSets_SetMinimumSize;
			int instCount_push280h{};
		}CServerBrowserDialog_ctor_SearchContext;

		CServerBrowserDialog_ctor_SearchContext ctx = { DllInfo, RealDllInfo };

		char pattern[] = "\x68\x80\x01\x00\x00\x68\x80\x02\x00\x00";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				if (ctx.addrSets_SetSize.find(pFound) == ctx.addrSets_SetSize.end() &&
					ctx.addrSets_SetMinimumSize.find(pFound) == ctx.addrSets_SetMinimumSize.end())
				{
					ctx.instCount_push280h = 0;

					g_pMetaHookAPI->DisasmRanges(pFound, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (CServerBrowserDialog_ctor_SearchContext*)context;

						if (!ctx->instCount_push280h &&
							pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_IMM &&
							pinst->detail->x86.operands[0].imm == 0x280)
						{
							ctx->instCount_push280h = instCount;
						}

						if (address[0] == 0xE8 && instCount > ctx->instCount_push280h && instCount <= ctx->instCount_push280h + 5)
						{
							PVOID callTarget = GetCallAddress(address);
							PVOID callTarget_RealDllInfoBased = ConvertDllInfoSpace(callTarget, ctx->DllInfo, ctx->RealDllInfo);

							if (gPrivateFuncs.ServerBrowser_Panel_SetSize == callTarget_RealDllInfoBased)
							{
								ctx->addrSets_SetSize.emplace(address);
								return TRUE;
							}

							if (gPrivateFuncs.ServerBrowser_Panel_SetMinimumSize == callTarget_RealDllInfoBased)
							{
								ctx->addrSets_SetMinimumSize.emplace(address);
								return TRUE;
							}

							if (!gPrivateFuncs.ServerBrowser_Panel_SetSize && VGUI2_IsPanelSetSize(callTarget))
							{
								gPrivateFuncs.ServerBrowser_Panel_SetSize = (decltype(gPrivateFuncs.ServerBrowser_Panel_SetSize))callTarget_RealDllInfoBased;
								ctx->addrSets_SetSize.emplace(address);
								return TRUE;
							}

							if (!gPrivateFuncs.ServerBrowser_Panel_SetMinimumSize && VGUI2_IsPanelSetMinimumSize(callTarget))
							{
								gPrivateFuncs.ServerBrowser_Panel_SetMinimumSize = (decltype(gPrivateFuncs.ServerBrowser_Panel_SetMinimumSize))callTarget_RealDllInfoBased;
								ctx->addrSets_SetMinimumSize.emplace(address);
								return TRUE;
							}
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;

					}, 0, &ctx);
				}

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}

		for (auto insn : ctx.addrSets_SetSize)
		{
			g_pMetaHookAPI->InlinePatchRedirectBranch(insn, ServerBrowser_Panel_SetSize, NULL);
		}

		for (auto insn : ctx.addrSets_SetMinimumSize)
		{
			g_pMetaHookAPI->InlinePatchRedirectBranch(insn, ServerBrowser_Panel_SetMinimumSize, NULL);
		}
	}
}

void ServerBrowser_FillAddress_KeyValues(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.ServerBrowser_KeyValues_vftable = VGUI2_FindKeyValueVFTable(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);
	Sig_FuncNotFound(ServerBrowser_KeyValues_vftable);

	gPrivateFuncs.ServerBrowser_KeyValues_LoadFromFile = (decltype(gPrivateFuncs.ServerBrowser_KeyValues_LoadFromFile))
		GetVFunctionFromVFTable(
			gPrivateFuncs.ServerBrowser_KeyValues_vftable,
			2,
			g_ServerBrowserDllInfo,
			g_ServerBrowserDllInfo,
			g_ServerBrowserDllInfo);
	Sig_FuncNotFound(ServerBrowser_KeyValues_LoadFromFile);

}

void ServerBrowser_FillAddress_PanelInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	gPrivateFuncs.ServerBrowser_Panel_Init = (decltype(gPrivateFuncs.ServerBrowser_Panel_Init))VGUI2_FindPanelInit(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);
	Sig_FuncNotFound(ServerBrowser_Panel_Init);
}

void ServerBrowser_FillAddress(void)
{
	if (!g_ServerBrowserDllInfo.ImageBase)
	{
		Sys_Error("ServerBrowser.dll is not available!");
		return;
	}

	ServerBrowser_PatchAddress_BaseGamesPage(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);

	ServerBrowser_PatchAddress_ServerBrowserDialog(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);

	ServerBrowser_FillAddress_KeyValues(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);

	ServerBrowser_FillAddress_PanelInit(g_ServerBrowserDllInfo, g_ServerBrowserDllInfo);
}

void ServerBrowser_InstallHooks(void)
{
	Install_InlineHook(ServerBrowser_Panel_Init);
	Install_InlineHook(ServerBrowser_KeyValues_LoadFromFile);
}

void ServerBrowser_UninstallHooks(void)
{
	Uninstall_Hook(ServerBrowser_Panel_Init);
	Uninstall_Hook(ServerBrowser_KeyValues_LoadFromFile);
}